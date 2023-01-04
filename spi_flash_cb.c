/***********************************************************************
 @copyright     : Siemens AG
 @license       : Siemens Inner Source License 1.4
 @author        : Andreas Kaeberlein
 @address       : Clemens-Winkler-Strasse 3, 09116 Chemnitz

 @maintainer    : Andreas Kaeberlein
 @telephone     : +49 371 4810-2108
 @email         : andreas.kaeberlein@siemens.com

 @file          : spi_flash_cb.c
 @date          : July 22, 2022

 @brief         : SPI FLash Circular Buffer
                  Driver for handling Circular Buffers in SPI Flashes
***********************************************************************/



/** Includes **/
/* Standard libs */
#include <stdint.h>     // defines fixed data types: int8_t...
#include <stddef.h>     // various variable types and macros: size_t, offsetof, NULL, ...
#include <string.h>     // string operation: memset, memcpy
/* Self */
#include "sfcb_flash_types.h"	// supported spi flashes
#include "spi_flash_cb.h"		// function prototypes



/**
 *  @defgroup SFCB_PRINTF_EN
 *
 *  redirect sfcb_printf to printf
 *
 *  @{
 */
#ifdef SFCB_PRINTF_EN
    #include <stdio.h>	// allow outputs in unit test
	#define sfcb_printf(...) printf(__VA_ARGS__)
#else
    //#define sfcb_printf(...) neorv32_uart0_printf(__VA_ARGS__)
	#define sfcb_printf(...)
#endif
/** @} */   // DEBUG



/** @brief ceildivide
 *
 *  dividing with always rounding up
 *
 *  @param[in]      dividend        input to be divided
 *  @param[in]      divisor         divider of input
 *  @return         uint32_t        rounded up quotient
 *
 */
static uint32_t sfcb_ceildivide_uint32(uint32_t dividend, uint32_t divisor)
{   
    if ( dividend != 0 ) {
        return ( 1 + ( (dividend-1) / divisor ) );
    };
    return ( ( dividend + divisor - 1) / divisor );
}



/** @brief max
 *
 *  compare and retun bigger number
 *
 *  @param[in]      val1            comparison value 1
 *  @param[in]      val2            comparison value 2
 *  @return         uint16_t        bigger number of both inputs
 *
 */
static uint16_t sfcb_max_uint16(uint16_t val1, uint16_t val2)
{
    if ( val1 > val2 ) {
		return val1;
	}
    return val2;
}



/** @brief address translation
 *
 *  converts 32bit SPI Flash address to SPI packet convenient byte sequence 
 *
 *  @param[in]      adr             Flash address
 *  @param[in,out]  *spi            SPI buffer, takes converted address place
 *  @param[in]      adrBytes        Number of bytes for address
 *  @return         void
 *
 */
static inline void sfcb_adr32_uint8(uint32_t adr, uint8_t *spi, uint8_t adrBytes)
{
	/* on highest index is lowest byte placed */
	for ( int8_t i = (int8_t) (adrBytes-1); i >= 0; i-- ) {
		spi[i] = (uint8_t) (adr & 0xFF);	// get lowest byte
		adr = adr >> 8;	// shift one byte right
	}
}



/**
 *  sfcb_init
 *    initializes handle
 */
int sfcb_init (t_sfcb *self, void *cb, uint8_t cbLen, void *spi, uint16_t spiLen)
{
    /* Function call message */
	sfcb_printf("__FUNCTION__ = %s\n", __FUNCTION__);
	/* check if provided flash type is valid */
    if ( 0 == (sizeof(SFCB_FLASH_NAME) - 1) ) {
		sfcb_printf("  ERROR:%s: no flash type selected\n", __FUNCTION__);
		return 1;	// no flash type selected, use proper compile switch
	}		
	sfcb_printf("  INFO:%s: flash '%s' selected\n", __FUNCTION__, SFCB_FLASH_NAME);
    /* set up list of flash circular buffers */
    self->uint8FlashPresent = 0;	// not flash found
    self->uint8NumCbs = cbLen;
    self->uint16SpiLen = 0;
    self->uint8Busy = 0;
	self->cmd = IDLE;	// free for request
	self->stage = STG00;
    self->ptrCbs = (t_sfcb_cb*) cb;		// circular buffer element array
	self->uint8PtrSpi = (uint8_t*) spi;	// uint8 array
	self->uint16SpiMax = spiLen;		// max array length
	self->error = NOERO;
    self->ptrCbElemPl = NULL;
	self->uint16CbElemPlSize = 0;
	/* memory addresses */
	sfcb_printf("  INFO:%s:sfcb:spi_p            = %p\n", __FUNCTION__, self->uint8PtrSpi);	// spi buffer
	/* SPI buffer needs at least space for one page and address and instruction */
	if ( (SFCB_FLASH_TOPO_PAGE_SIZE + SFCB_FLASH_TOPO_ADR_BYTE + 1) > self->uint16SpiMax ) {
		sfcb_printf("  ERROR:%s: spi buffer to small, is=%d byte, req=%d byte\n", __FUNCTION__, self->uint16SpiMax, SFCB_FLASH_TOPO_PAGE_SIZE + SFCB_FLASH_TOPO_ADR_BYTE + 1);
		return 2;	// not enough SPI buffer to write at least one complete page to flash
	}
	/* init circular buffer handles */
	for ( uint8_t i = 0; i < (self->uint8NumCbs); i++ ) {
		(self->ptrCbs[i]).uint8Used = 0;
		(self->ptrCbs[i]).uint8Init = 0;
		sfcb_printf("  INFO:%s:ptrCbs[%i]_p           = %p\n", __FUNCTION__, i, (&self->ptrCbs[i]));				// unit test output
		sfcb_printf("  INFO:%s:ptrCbs[%i].uint8Used_p = %p\n", __FUNCTION__, i, &((self->ptrCbs[i]).uint8Used));	// output address
		sfcb_printf("  INFO:%s:ptrCbs[%i].uint8Init_p = %p\n", __FUNCTION__, i, &((self->ptrCbs[i]).uint8Init));	// output address
	}
	/* normal end */
	return 0;
}



/**
 *  spi_flash_cb_worker
 *    executes request from ... 
 */
void sfcb_worker (t_sfcb *self)
{
	/** Variables **/
	spi_flash_cb_elem_head*	cbHead = (spi_flash_cb_elem_head*) (self->uint8PtrSpi+SFCB_FLASH_TOPO_ADR_BYTE+1);	// assign spi buffer to head structure, +: 1 byte instruction + address bytes
	uint8_t					uint8Good;				// check was good
	uint16_t				uint16PagesBytesAvail;	// number of used page bytes
	uint16_t				uint16CpyLen;			// number of Bytes to copy
	uint32_t				uint32Temp;				// temporaray 32bit variable
	spi_flash_cb_elem_head	head;					// circular buffer element header
	
    /* Function call message */
	sfcb_printf("__FUNCTION__ = %s\n", __FUNCTION__);
	sfcb_printf("  INFO:%s:sfcb_p            = %p\n", __FUNCTION__, self);
	sfcb_printf("  INFO:%s:sfcb:spi_p        = %p\n", __FUNCTION__, self->uint8PtrSpi);	// spi buffer
	sfcb_printf("  INFO:%s:sfcb:spi:cbHead_p = %p\n", __FUNCTION__, cbHead);			// spi packate casted to header
	/* select part of FSM */
    switch (self->cmd) {
		/* 
		 * 
		 * nothing todo 
		 * 
		 */
		case IDLE:
			sfcb_printf("  INFO:%s:IDLE\n", __FUNCTION__);
			return;
		/* 
		 * 
		 * Allocate Next Free Element for Circular Buffers 
		 * 
		 */
		case MKCB:
			/* select excution state, allows break & cont */
			switch (self->stage) {
				/* check for WIP */
				case STG00:
					/* Debug message */
					sfcb_printf (  "  INFO:%s:MKCB:STG0:check for WIP, uint16SpiLen=%d, uint8PtrSpi[1]=0x%02x\n", 
					               __FUNCTION__,
								   self->uint16SpiLen,
							       self->uint8PtrSpi[1]
								);
					/* check */
					if ( (0 == self->uint16SpiLen) || (0 != (self->uint8PtrSpi[1] & SFCB_FLASH_MNG_WIP_MSK)) ) {
						/* First Request or WIP */
						self->uint8PtrSpi[0] = SFCB_FLASH_IST_RD_STATE_REG;
						self->uint8PtrSpi[1] = 0;
						self->uint16SpiLen = 2;
						return;
					}
					self->uint16SpiLen = 0;	// no data available
					self->stage = STG01;	// Go one with search for Free Segment
					__attribute__((fallthrough));	// Go one with next
				/* Find Page for next Circulat buffer element */
				case STG01:
					/* Debug message */
					sfcb_printf("  INFO:%s:MKCB:STG1: find empty page for new element\n", __FUNCTION__);
					/* check last response */
					if ( 0 != self->uint16SpiLen ) {
						/* Debug Message */
						sfcb_printf("  INFO:%s:MKCB:STG1: SPI ", __FUNCTION__);
						for ( uint8_t i = 0; i < 12; i++ ) {
							sfcb_printf("0x%x ", self->uint8PtrSpi[i]);
						}
						sfcb_printf("\n");
						sfcb_printf("  INFO:%s:MKCB:STG1: CBHEAD,magicnum=0x%x\n", __FUNCTION__, cbHead->uint32MagicNum);
						/* Flash Area is used by circular buffer, check magic number 
						 *   +4: Read instruction + 32bit address
						 */
						if ( cbHead->uint32MagicNum == ((self->ptrCbs)[self->uint8IterCb]).uint32MagicNum ) {
							/* Debug Message */
							sfcb_printf("  INFO:%s:MKCB:STG1: Valid Entry Found\n", __FUNCTION__);
							/* count available elements */
							(((self->ptrCbs)[self->uint8IterCb]).uint16NumEntries)++;
							/* get highest number of numbered circular buffer elements, needed for next entry */
							if ( cbHead->uint32IdNum > ((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMax ) {
								/* save new highest number in circular buffer */
								((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMax = cbHead->uint32IdNum;
							} 
							/* get lowest number of circular buffer, needed for erase sector, and start get function */
							if ( cbHead->uint32IdNum < ((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMin ) {
								((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMin = cbHead->uint32IdNum;
								((self->ptrCbs)[self->uint8IterCb]).uint32StartPageIdMin = self->uint32IterPage;
							}
						} else {
							/* check for unused header 
							 * first unused pages is alocated, iterate over all elements to get all IDs
							 */
							if ( 0 == ((self->ptrCbs)[self->uint8IterCb]).uint8Init ) {
								uint8Good = 1;
								for ( uint8_t i = SFCB_FLASH_TOPO_ADR_BYTE + 1; i < SFCB_FLASH_TOPO_ADR_BYTE + 1 + sizeof(spi_flash_cb_elem_head); i++ ) {	// +1: IST
									/* corrupted empty page found, leave as it is */
									if ( 0xFF != self->uint8PtrSpi[i] ) {
										uint8Good = 0;	// try to find next free clean page
										sfcb_printf ("  ERROR:%s:MKCB:STG1: corrupted empty page found at 0x%0x\n", __FUNCTION__, self->uint32IterPage);
										break;
									}	
								}
								/* save page number for next circular buffer entry */
								if ( 0 != uint8Good ) {
									((self->ptrCbs)[self->uint8IterCb]).uint32StartPageWrite = self->uint32IterPage;	
									((self->ptrCbs)[self->uint8IterCb]).uint8Init = 1;	// prepared for writing next element
								}
							}
						}
					}
					/* Current Status Message */
					sfcb_printf ( "  INFO:%s:MKCB:STG1: cb=%d, elem=%d, flashadr=0x%x, idmin=0x%x, idmax=0x%x\n", 
								  __FUNCTION__, 
								  self->uint8IterCb,
								  self->uint16IterElem,
								  self->uint32IterPage,
								  ((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMin, 
								  ((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMax
								);	
					/* request next header of circular buffer */
					self->uint32IterPage = 	((self->ptrCbs)[self->uint8IterCb]).uint32StartSector * (uint32_t) SFCB_FLASH_TOPO_SECTOR_SIZE
											+
											((self->ptrCbs)[self->uint8IterCb]).uint16NumPagesPerElem *
											(uint32_t) SFCB_FLASH_TOPO_PAGE_SIZE *
											self->uint16IterElem;
					self->uint16SpiLen = SFCB_FLASH_TOPO_ADR_BYTE + 1 + sizeof(spi_flash_cb_elem_head);	// +1: IST, + Address bytes		
					memset(self->uint8PtrSpi, 0, self->uint16SpiLen);
					self->uint8PtrSpi[0] = SFCB_FLASH_IST_RD_DATA;
					sfcb_adr32_uint8(self->uint32IterPage, self->uint8PtrSpi+1, SFCB_FLASH_TOPO_ADR_BYTE);	// +1 first byte is instruction
					/* prepare iterator for next */
					if ( self->uint16IterElem < ((self->ptrCbs)[self->uint8IterCb]).uint16NumEntriesMax ) {
						/* next element in current queue */
						(self->uint16IterElem)++;
					} else {
						/* Free Page Found */
						if ( 0 != ((self->ptrCbs)[self->uint8IterCb]).uint8Init) {
							/* prepare for next queue */
							self->uint16IterElem = 0;	// reset element counter
							(self->uint8IterCb)++;		// process next queue
							/* look ahead if service of some queue can skipped */
							for ( uint8_t i=self->uint8IterCb; i<self->uint8NumCbs; i++ ) {
								/* all active circular buffer queues processed? */
								if ( 0 == ((self->ptrCbs)[i]).uint8Used ) {
									self->uint16SpiLen = 0;
									self->cmd = IDLE;
									self->stage = STG00;
									self->uint8Busy = 0;
									return;
								/* active queue found */
								} else {
									if ( 0 == ((self->ptrCbs)[i]).uint8Init ) {
										self->uint8IterCb = i;	// search for unintializied queue, in case of only on circular buffer needes to rebuild
										break;
									}
								}
							}
							/* all available queues processed, go in idle */
							if ( !(self->uint8IterCb < self->uint8NumCbs) ) {
								self->uint16SpiLen = 0;
								self->cmd = IDLE;
								self->stage = STG00;
								self->uint8Busy = 0;
								return;
							}
						/* Go on with sector erase */
						} else {
							self->uint8PtrSpi[0] = SFCB_FLASH_IST_WR_ENA;	// enable write
							self->uint16SpiLen = 1;
							self->stage = STG02;
						}
					}
					return;	// DONE or SPI transfer is required
					break;
				/* Assemble Command for Sector ERASE */	
				case STG02:
					sfcb_printf( "  INFO:%s:MKCB:STG2: Assemble Command for Sector ERASE\n", __FUNCTION__);
					sfcb_printf( "  INFO:%s:MKCB:STG2: cb=%d, uint32StartPageIdMin=0x%x\n", 
								 __FUNCTION__, 
								 self->uint8IterCb,
								 ((self->ptrCbs)[self->uint8IterCb]).uint32StartPageIdMin
							    );
					uint32Temp = (self->ptrCbs[self->uint8IterCb]).uint32StartPageIdMin;		// get startpage of oldest entry, prepare for delete
					uint32Temp = (uint32Temp & (uint32_t) ~(SFCB_FLASH_TOPO_SECTOR_SIZE - 1));	// align to sub sector address
					self->uint8PtrSpi[0] = SFCB_FLASH_IST_ERASE_SECTOR;
					sfcb_adr32_uint8(uint32Temp, self->uint8PtrSpi+1, SFCB_FLASH_TOPO_ADR_BYTE);	// +1 first byte is instruction
					self->uint16SpiLen = SFCB_FLASH_TOPO_ADR_BYTE + 1;	// address + instruction
					self->stage = STG03;
					return;	// DONE or SPI transfer is required
					break;
				/* Wait for Sector Erase */	
				case STG03:
					sfcb_printf("  INFO:%s:MKCB:STG3: Wait for Sector Erase\n", __FUNCTION__);
					/* Start at zero Element with search for free page */
					self->uint16IterElem = 0;
					/* Assemble command for WIP */
					self->uint8PtrSpi[0] = SFCB_FLASH_IST_RD_STATE_REG;
					self->uint8PtrSpi[1] = 0;
					self->uint16SpiLen = 2;
					self->stage = STG00;	// wait for erase, and search for free page for next element
					return;	// DONE or SPI transfer is required
					break;				
				/* something strange happend */
				default:
					sfcb_printf("  ERROR:%s:MKCB: unexpected use of default path\n", __FUNCTION__);
					break;
			}
			break;
		/* 
		 * 
		 * Add New Element to Circular Buffer
		 * 
		 */
		case ADD:
			switch (self->stage) {
				/* check for WIP */
				case STG00:
					sfcb_printf("  INFO:%s:ADD:STG0: check for WIP\n", __FUNCTION__);
					if ( (0 == self->uint16SpiLen) || (0 != (self->uint8PtrSpi[1] & SFCB_FLASH_MNG_WIP_MSK)) ) {
						/* First Request or WIP */
						self->uint8PtrSpi[0] = SFCB_FLASH_IST_RD_STATE_REG;
						self->uint8PtrSpi[1] = 0;
						self->uint16SpiLen = 2;
						return;
					}
					self->stage = STG01;			// Go one with search for Free Segment
					__attribute__((fallthrough));	// Go one with next
				/* Circular Buffer written, if not write enable */
				case STG01:
					sfcb_printf("  INFO:%s:ADD:STG1: Circular Buffer completly written, if not write enable\n", __FUNCTION__);
					/* Page Requested to program */
					if ( self->uint16IterElem < self->uint16CbElemPlSize ) {
						/* enable WRITE Latch */
						self->uint8PtrSpi[0] = SFCB_FLASH_IST_WR_ENA;	// uint8FlashIstWrEnable
						self->uint16SpiLen = 1;
						self->stage = STG02;	// Page Write as next
						return;
					/* circular buffer written */
					} else {
						self->uint16SpiLen = 0;
						self->cmd = IDLE;
						self->stage = STG00;
						self->uint8Busy = 0;
						return;
					}
					break;
				/* Page Write to Circular Buffer */
				case STG02:
					sfcb_printf("  INFO:%s:ADD:STG2: Page Write to Circular Buffer\n", __FUNCTION__);
					/* assemble Flash Instruction packet */
					self->uint8PtrSpi[0] = SFCB_FLASH_IST_WR_PAGE;	// write page
					sfcb_adr32_uint8(self->uint32IterPage, self->uint8PtrSpi+1, SFCB_FLASH_TOPO_ADR_BYTE);	// +1 first byte is instruction
					self->uint16SpiLen = SFCB_FLASH_TOPO_ADR_BYTE + 1;	// +1: IST
					/* on first packet add header */
					uint16PagesBytesAvail = SFCB_FLASH_TOPO_PAGE_SIZE;
					if ( 0 == self->uint16IterElem ) {
						memset(&head, 0, sizeof(head));	// make empty
						head.uint32MagicNum = ((self->ptrCbs)[self->uint8IterCb]).uint32MagicNum;
						head.uint32IdNum = ((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMax + 1;
						memcpy((self->uint8PtrSpi+self->uint16SpiLen), &head, sizeof(head));
						self->uint16SpiLen = (uint16_t) (self->uint16SpiLen + sizeof(head));
						uint16PagesBytesAvail = (uint16_t) (uint16PagesBytesAvail - sizeof(head));
					}
					/* determine number of bytes to copy */
					if ( self->uint16CbElemPlSize - self->uint16IterElem > uint16PagesBytesAvail ) {
						uint16CpyLen = uint16PagesBytesAvail;
					} else {
						uint16CpyLen = (uint16_t) (self->uint16CbElemPlSize - self->uint16IterElem);
					}
					/* assemble packet */
					memcpy((self->uint8PtrSpi+self->uint16SpiLen), (self->ptrCbElemPl+self->uint16IterElem), uint16CpyLen);
					self->uint16SpiLen = (uint16_t) (self->uint16SpiLen + uint16CpyLen);
					self->uint16IterElem = (uint16_t) (self->uint16IterElem + uint16CpyLen);
					/* increment iterators */
					(self->uint32IterPage)++;
					/* Go to wait WIP */
					self->stage = STG00;
					return;
					break;
				/* something strange happend */
				default:
					sfcb_printf("  ERROR:%s:ADD: unexpected use of default path\n", __FUNCTION__);
					break;
			}
			break;
		/* 
		 * 
		 * Get Element from circular buffer
		 * 
		 */
		 
		 
		/* 
		 * 
		 * Raw Flash Read
		 * 
		 */
		case RAW:
			switch (self->stage) {
				/* check for WIP */
				case STG00:
					sfcb_printf("  INFO:%s:RAW:STG0: check for WIP\n", __FUNCTION__);
					if ( (0 == self->uint16SpiLen) || (0 != (self->uint8PtrSpi[1] & SFCB_FLASH_MNG_WIP_MSK)) ) {
						/* First Request or WIP */
						self->uint8PtrSpi[0] = SFCB_FLASH_IST_RD_STATE_REG;
						self->uint8PtrSpi[1] = 0;
						self->uint16SpiLen = 2;
						return;
					}
					self->uint16SpiLen = 0;	// no data available
					self->stage = STG01;	// Go one with search for Free Segment
					__attribute__((fallthrough));	// Go one with stage
				/* Prepare raw read */
				case STG01:
					sfcb_printf("  INFO:%s:RAW:STG1: Prepare RAW read\n", __FUNCTION__);
					/* check for enough spi buf */
					if ( self->uint16SpiMax < (self->uint16CbElemPlSize + SFCB_FLASH_TOPO_ADR_BYTE + 1) ) {	// IST (+1) + ADR_BYTE: caused by read instruction
						self->uint8Busy = 0;
						self->cmd = IDLE;		// go in idle
						self->stage = STG00;
						self->error = BUFSIZE;	// requested operation ends with an error
					}
					/* SPI package is zero */
					self->uint16SpiLen = (uint16_t) (self->uint16CbElemPlSize + SFCB_FLASH_TOPO_ADR_BYTE + 1);	// +1: for instruction
					memset(self->uint8PtrSpi, 0, self->uint16SpiLen);	// written data is zero
					/* Flash instrcution */
					self->uint8PtrSpi[0] = SFCB_FLASH_IST_RD_DATA;	// read data
					sfcb_adr32_uint8(self->uint32IterPage, self->uint8PtrSpi+1, SFCB_FLASH_TOPO_ADR_BYTE);	// +1 first byte is instruction
					/* go on with next stage */
					self->stage = STG02;	// now wait for transfer
					return;
				/* copy data from SPI back */
				case STG02:
					sfcb_printf("  INFO:%s:RAW:STG2: copy data from SPI back\n", __FUNCTION__);
					memcpy(self->ptrCbElemPl, self->uint8PtrSpi+SFCB_FLASH_TOPO_ADR_BYTE+1, self->uint16CbElemPlSize);	// skip header from answer of read instruction
					self->uint16SpiLen = 0;
					self->cmd = IDLE;
					self->stage = STG00;
					self->uint8Busy = 0;
					return;
				/* something strange happend */
				default:
					sfcb_printf("  ERROR:%s:RAW: unexpected use of default path\n", __FUNCTION__);
					break;
			}
			break;
		
		/* something strange happend */
		default:
			break;
	}
}



/**
 *  sfcb_flash_size
 *    total flashsize
 */
uint32_t sfcb_flash_size (void)
{
	return (uint32_t) SFCB_FLASH_TOPO_FLASH_SIZE;
}



/**
 *  sfcb_new_cb 
 *    creates new circular buffer entry
 */
int sfcb_new_cb (t_sfcb *self, uint32_t magicNum, uint16_t elemSizeByte, uint16_t numElems, uint8_t *cbID)
{	
	/** help variables **/
	const uint8_t	uint8PagesPerSector = (uint8_t) (SFCB_FLASH_TOPO_SECTOR_SIZE / SFCB_FLASH_TOPO_PAGE_SIZE);
	const uint16_t	elemTotalSize = (uint16_t) (elemSizeByte + sizeof(spi_flash_cb_elem_head));	// payload size + header size
	uint8_t			cbNew;				// queue of circular buffer new entry number
	uint32_t		uint32StartSector;
	uint16_t		uint16NumSectors;
	
	
    /* Function call message */
	sfcb_printf("__FUNCTION__ = %s\n", __FUNCTION__);
	sfcb_printf("  INFO:%s:sfcb_p = %p\n", __FUNCTION__, self);
	/* check for free Slot number */
	uint32StartSector = 0;
	for ( cbNew = 0; cbNew < (self->uint8NumCbs); cbNew++ ) {
		if ( 0 != (self->ptrCbs[cbNew]).uint8Used ) {	// queue is used
			uint32StartSector = (self->ptrCbs[cbNew]).uint32StopSector + 1;	// calculate start address of next queue
		} else {
			break;	// empty queue found
		}
	}
	if ( cbNew == (self->uint8NumCbs) ) {
		sfcb_printf("  ERROR:%s:sfcb_cb exceeded total available number of %i cbs\n", __FUNCTION__, (self->uint8NumCbs));
		return 1;	// no free circular buffer slots
	}	
	/* prepare slot */
	(self->ptrCbs[cbNew]).uint8Used = 1;		// occupied
	(self->ptrCbs[cbNew]).uint32IdNumMax = 0;	// in case of uninitialized memory
	(self->ptrCbs[cbNew]).uint32IdNumMin = (uint32_t) (~0);	// assign highest number
	(self->ptrCbs[cbNew]).uint32MagicNum = magicNum;	// used magic number for the circular buffer
	(self->ptrCbs[cbNew]).uint16NumPagesPerElem = (uint16_t) sfcb_ceildivide_uint32(elemTotalSize, SFCB_FLASH_TOPO_PAGE_SIZE);	// calculate in multiple of pages
	(self->ptrCbs[cbNew]).uint32StartSector = uint32StartSector;
	uint16NumSectors = sfcb_max_uint16(2, (uint16_t) sfcb_ceildivide_uint32((uint32_t) (numElems*((self->ptrCbs[cbNew]).uint16NumPagesPerElem)), uint8PagesPerSector));
	(self->ptrCbs[cbNew]).uint32StopSector = (self->ptrCbs[cbNew]).uint32StartSector+uint16NumSectors-1;
	(self->ptrCbs[cbNew]).uint16NumEntriesMax = (uint16_t) (uint16NumSectors*uint8PagesPerSector) / (self->ptrCbs[cbNew]).uint16NumPagesPerElem;
	(self->ptrCbs[cbNew]).uint16NumEntries = 0;
	*cbID = cbNew;
	/* check if stop sector is in total size */
	if ( ((self->ptrCbs[cbNew]).uint32StopSector+1) * SFCB_FLASH_TOPO_SECTOR_SIZE > SFCB_FLASH_TOPO_FLASH_SIZE ) {
		sfcb_printf("  ERROR:%s flash size exceeded\n", __FUNCTION__);
		return 2;	// Flash capacity exceeded
	}
	/* print slot config */
	sfcb_printf("  INFO:%s:ptrCbs[%i]_p                     = %p\n",   __FUNCTION__, cbNew, (&self->ptrCbs[cbNew]));
	sfcb_printf("  INFO:%s:ptrCbs[%i].uint8Used             = %d\n",   __FUNCTION__, cbNew, (self->ptrCbs[cbNew]).uint8Used);
	sfcb_printf("  INFO:%s:ptrCbs[%i].uint16NumPagesPerElem = %d\n",   __FUNCTION__, cbNew, (self->ptrCbs[cbNew]).uint16NumPagesPerElem);
	sfcb_printf("  INFO:%s:ptrCbs[%i].uint32StartSector     = 0x%x\n", __FUNCTION__, cbNew, (self->ptrCbs[cbNew]).uint32StartSector);
	sfcb_printf("  INFO:%s:ptrCbs[%i].uint32StopSector      = 0x%x\n", __FUNCTION__, cbNew, (self->ptrCbs[cbNew]).uint32StopSector);
	sfcb_printf("  INFO:%s:ptrCbs[%i].uint16NumEntriesMax   = %d\n",   __FUNCTION__, cbNew, (self->ptrCbs[cbNew]).uint16NumEntriesMax);
	/* succesfull */
	return 0;
}



/**
 *  sfcb_busy
 *    checks if #sfcb_worker is free for new requests
 */
int sfcb_busy (t_sfcb *self)
{
	if ( 0 != self->uint8Busy ) {
		return 1;	// busy
	}
	return 0;	
}



/**
 *  sfcb_spi_len
 *    gets length of next spi packet
 */
uint16_t sfcb_spi_len (t_sfcb *self)
{
	return self->uint16SpiLen;
}



/**
 *  sfcb_mkcb
 *    build up queues with circular buffer
 */
int sfcb_mkcb (t_sfcb *self)
{	
    /* Function call message */
	sfcb_printf("__FUNCTION__ = %s\n", __FUNCTION__);
	sfcb_printf("  INFO:%s:sfcb_p = %p\n", __FUNCTION__, self);
	/* no jobs pending */
	if ( 0 != self->uint8Busy ) {
		return 1;	// busy
	}
	/* check for at least one active queue */
	if ( 0 == ((self->ptrCbs)[0]).uint8Used ) {
		return 2;	// no active queue
	}
	/* Find first queue which needs an build */
	self->uint8IterCb = 0;
	for ( uint8_t i=0; i<self->uint8NumCbs; i++ ) {
		if ( (0 == ((self->ptrCbs)[i]).uint8Used) || (0 == ((self->ptrCbs)[i]).uint8Init) ) {
			break;
		}
		self->uint8IterCb = i;	// search for unintializied queue, in case of only on circular buffer needes to rebuild
	}
	/* reset idmin/idmax counter to enable select of correct page to erase */
	for ( uint8_t i = 0; i < (self->uint8NumCbs); i++ ) {
		/* not used, leave */
		if ( 0 == ((self->ptrCbs)[i]).uint8Used ) {
			break;
		}
		/* dirty buffer, needs to rebuild absoulte idmin/idmax for next write, otherwise will the last written element deleted */
		if ( 0 == ((self->ptrCbs)[self->uint8IterCb]).uint8Init ) {	
			(self->ptrCbs[i]).uint32IdNumMax = 0;				// in case of uninitialized memory
			(self->ptrCbs[i]).uint32IdNumMin = (uint32_t) (~0);	// assign highest number
		}
	}
	/* Setup new Job */
	self->cmd = MKCB;
	self->uint16IterElem = 0;
	self->stage = STG00;
	self->error = NOERO;
	self->uint8Busy = 1;
	/* fine */
	return 0;
}



/**
 *  sfcb_add
 *    inserts element into circular buffer
 */
int sfcb_add (t_sfcb *self, uint8_t cbID, void *data, uint16_t len)
{	
	/* no jobs pending */
	if ( 0 != self->uint8Busy ) {
		return 1;	// Worker is busy, wait for processing last job
	}
	/* check if CB is init for request */
	if ( (0 == ((self->ptrCbs)[cbID]).uint8Used) || (0 == ((self->ptrCbs)[cbID]).uint8Init) ) {
		return 2;	// Circular Buffer is not prepared for adding new element, run #sfcb_worker
	}
	/* check for match into circular buffer size */
	if ( len > (((self->ptrCbs)[cbID]).uint16NumPagesPerElem * SFCB_FLASH_TOPO_PAGE_SIZE) ) {
		return 4;	// data segement is larger then reserved circular buffer space
	}
	/* store information for insertion */
	self->uint8IterCb = cbID;	// used as pointer to queue
	((self->ptrCbs)[self->uint8IterCb]).uint8Init = 0;	// mark as dirty, for next write run #sfcb_mkcb
	self->uint32IterPage = ((self->ptrCbs)[self->uint8IterCb]).uint32StartPageWrite;	// select page to write
	self->ptrCbElemPl = data;
	self->uint16CbElemPlSize = len;
	self->uint16IterElem = 0;	// used as ptrCbElemPl written pointer
	/* Setup new Job */
	self->uint8Busy = 1;
	self->cmd = ADD;
	self->stage = STG00;
	self->error = NOERO;
	/* fine */
	return 0;
}



/**
 *  sfcb_get
 *    inserts element into circular buffer
 */
int sfcb_get (t_sfcb *self, uint8_t cbID, void *data, uint16_t len, uint16_t lenMax)
{
	/* no jobs pending */
	if ( 0 != self->uint8Busy ) {
		return 1;	// Worker is busy, wait for processing last job
	}
	/* check if CB is init for request */
	if ( (0 == ((self->ptrCbs)[cbID]).uint8Used) || (0 == ((self->ptrCbs)[cbID]).uint8Init) ) {
		return 2;	// Circular Buffer is not prepared for adding new element, run #sfcb_worker
	}
	/* prepare job */



	/* fine */
	return 0;
}



/**
 *  sfcb_flash_read
 *    reads raw binary data from flash
 */
int sfcb_flash_read (t_sfcb *self, uint32_t adr, void *data, uint16_t len)
{
	/* no jobs pending */
	if ( 0 != self->uint8Busy ) {
		return 1;	// Worker is busy, wait for processing last job
	}
	/* prepare job */
	self->ptrCbElemPl = data;
	self->uint16CbElemPlSize = len;
	self->uint32IterPage = adr;	// Flash RAW address
	/* Setup new Job */
	self->uint8Busy = 1;
	self->cmd = RAW;	// RAW read from Flash
	self->stage = STG00;
	self->error = NOERO;
	/* fine */
	return 0;
}



/**
 *  sfcb_idmax
 *    get maximum id in selected circular buffer queue
 */
uint32_t sfcb_idmax (t_sfcb *self, uint8_t cbID)
{
	/* check if selected queue is used */
	if ( 0 == ((self->ptrCbs)[cbID]).uint8Used ) {
		return 0;
	}		
	return ((self->ptrCbs)[cbID]).uint32IdNumMax;
}
