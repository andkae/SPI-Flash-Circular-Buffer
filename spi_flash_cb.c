/***********************************************************************
 @copyright     : Siemens AG
 @license       : Siemens Inner Source License 1.4
 @author        : Andreas Kaeberlein
 @address       : Clemens-Winkler-Strasse 3, 09116 Chemnitz

 @maintainer    : Andreas Kaeberlein
 @telephone     : +49 371 4810-2108
 @email         : andreas.kaeberlein@siemens.com

 @file          : spi_flash_cb.h
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
#include "spi_flash_cb_hal.h"	// supported spi flashes
#include "spi_flash_cb.h"		// function prototypes



/**
 *  spi_flash_cb_ceildivide2
 *    divides to the power 2
 *    round always up
 */
uint32_t spi_flash_cb_ceildivide(uint32_t dividend, uint32_t divisor)
{   
    if ( dividend != 0 ) {
        return ( 1 + ( (dividend-1) / divisor ) );
    };
    return ( ( dividend + divisor - 1) / divisor );
}


/**
 *  spi_flash_cb_max
 *    get maximum of number
 */
uint32_t spi_flash_cb_max(uint16_t val1, uint16_t val2)
{
    if ( val1 > val2 ) {
		return val1;
	}
    return val2;
}


/**
 *  spi_flash_cb_init
 *    initializes handle
 */
int spi_flash_cb_init (spi_flash_cb *self, uint8_t flashType, void *cbMem, uint8_t numCbs)
{
    /* check if provided flash type is valid */
    if ( flashType > ((sizeof(SPI_FLASH_CB_TYPES)/sizeof(SPI_FLASH_CB_TYPES[0]))-1) ) {
		return 1;
	}
    /* set up list of flash circular buffers */
    self->uint8FlashType = flashType;
    self->uint8NumCbs = numCbs;
    self->uint16SpiLen = 0;
    self->uint8Busy = 0;
	self->uint8Cmd = SFCB_CMD_IDLE;	// free for request
	self->uint8Stg = SFCB_STG_00;
    self->ptrCbs = cbMem;
    /* init list */
    for ( uint8_t i=0; i<self->uint8NumCbs; i++ ) {
		((spi_flash_cb_elem*) self->ptrCbs)[i].uint8Used = 0;
		((spi_flash_cb_elem*) self->ptrCbs)[i].uint8Init = 0;
	}
	/* normal end */
	return 0;
}


/**
 *  spi_flash_cb_add
 *    add circular buffer entry
 */
int spi_flash_cb_add (spi_flash_cb *self, uint32_t magicNum, uint16_t elemSizeByte, uint16_t numElems, uint8_t *cbID)
{	
	/** help variables **/
	const uint8_t		uint8NumHeadBytes = 2*sizeof(((spi_flash_cb_elem *)0)->uint32MagicNum) + sizeof(((spi_flash_cb_elem *)0)->uint32IdNumMax);
	spi_flash_cb_elem*	cb_elem = self->ptrCbs;
	uint8_t				uint8FreeCbEntry;
	uint32_t			uint32StartSector;
	uint32_t			uint32NumSectors;
	
	/* check for free Slot number */
	uint32StartSector = 0;
	for ( uint8FreeCbEntry = 0; uint8FreeCbEntry < (self->uint8NumCbs); uint8FreeCbEntry++ ) {
		if ( 0 == cb_elem[uint8FreeCbEntry].uint8Used ) {
			break;
		} else {
			uint32StartSector = cb_elem[uint8FreeCbEntry].uint32StopSector + 1;	// next sector is used 
		}
	}
	if ( uint8FreeCbEntry == (self->uint8NumCbs) ) {
		return 1;	// no free circular buffer slots
	}	
	/* prepare slot */
	cb_elem[uint8FreeCbEntry].uint8Used = 1;		// occupied
	cb_elem[uint8FreeCbEntry].uint32IdNumMax = 0;	// in case of uninitialized memory
	cb_elem[uint8FreeCbEntry].uint32IdNumMin = (uint32_t) (~0);	// assign highest number
	cb_elem[uint8FreeCbEntry].uint32MagicNum = magicNum;	// used magic number for the circular buffer
	cb_elem[uint8FreeCbEntry].uint16NumPagesPerElem = spi_flash_cb_ceildivide(elemSizeByte+uint8NumHeadBytes, SPI_FLASH_CB_TYPES[self->uint8FlashType].uint32FlashTopoPageSizeByte);	// calculate in multiple of pages
	cb_elem[uint8FreeCbEntry].uint32StartSector = uint32StartSector;
	uint32NumSectors = spi_flash_cb_max(2, spi_flash_cb_ceildivide(numElems*cb_elem[uint8FreeCbEntry].uint16NumPagesPerElem, SPI_FLASH_CB_TYPES[self->uint8FlashType].uint8FlashTopoPagesPerSector));
	cb_elem[uint8FreeCbEntry].uint32StopSector = cb_elem[uint8FreeCbEntry].uint32StartSector+uint32NumSectors-1;
	cb_elem[uint8FreeCbEntry].uint16NumEntriesMax = (uint16_t) (uint32NumSectors*SPI_FLASH_CB_TYPES[self->uint8FlashType].uint8FlashTopoPagesPerSector);
	cb_elem[uint8FreeCbEntry].uint16NumEntries = 0;
	*cbID = uint8FreeCbEntry;
	/* succesfull */
	return 0;
}



/**
 *  spi_flash_cb_worker
 *    executes request from ... 
 */
void sfcb_worker (spi_flash_cb *self)
{
	/** Variables **/
	uint8_t					uint8Good;				// check was good
	uint16_t				uint16PagesBytesAvail;	// number of used page bytes
	uint16_t				uint16CpyLen;			// number of Bytes to copy
	spi_flash_cb_elem_head	head;					// circular buffer element header
	
	/* select part of FSM */
    switch (self->uint8Cmd) {
		/* 
		 * 
		 * nothing todo 
		 * 
		 */
		case SFCB_CMD_IDLE:
			return;
		/* 
		 * 
		 * Allocate Next Free Element for Circular Buffers 
		 * 
		 */
		case SFCB_CMD_MKCB:
			switch (self->uint8Stg) {
				/* check for WIP */
				case SFCB_STG_00:
					if ( (0 == self->uint16SpiLen) || (0 != (self->uint8Spi[1] & SPI_FLASH_CB_TYPES[self->uint8FlashType].uint8FlashMngWipMsk)) ) {
						/* First Request or WIP */
						self->uint8Spi[0] = SPI_FLASH_CB_TYPES[self->uint8FlashType].uint8FlashIstRdStateReg;
						self->uint8Spi[1] = 0;
						self->uint16SpiLen = 2;
						return;
					}
					self->uint16SpiLen = 0;			// no data available
					self->uint8Stg = SFCB_STG_01;	// Go one with search for Free Segment
					__attribute__ ((fallthrough));	// Go one with next
				/* Find Page for next Circulat buffer element */
				case SFCB_STG_01:
					/* check last response */
					if ( 0 != self->uint16SpiLen ) {
						/* For circular buffer used flash area? */
						if ( ((spi_flash_cb_elem_head*) (((void*)self->uint8Spi)+4))->uint32MagicNum == ((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint32MagicNum ) {
							/* count available elements */
							(((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint16NumEntries)++;
							/* get highest number of numbered circular buffer elements, needed for next entry */
							if ( ((spi_flash_cb_elem_head*) (((void*)self->uint8Spi)+4))->uint32IdNum > ((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint32IdNumMax ) {
								/* save new highest number in circular buffer */
								((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint32IdNumMax = ((spi_flash_cb_elem_head*) (((void*)self->uint8Spi)+4))->uint32IdNum;
							} 
							/* get lowest number of circular buffer, needed for erase sector, and start get function */
							if ( ((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint32IdNumMin > ((spi_flash_cb_elem_head*) (((void*)self->uint8Spi)+4))->uint32IdNum ) {
								((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint32IdNumMin = ((spi_flash_cb_elem_head*) (((void*)self->uint8Spi)+4))->uint32IdNum;
								((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint32StartPageIdMin = self->uint32IterPage;
							}
						} else {
							/* check for unused header 
							 * first unused pages is alocated, iterate over all elements to get all IDs
							 */
							if ( 0 == ((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint8Init ) {
								uint8Good = 1;
								for ( uint8_t i = 4; i<4+sizeof(spi_flash_cb_elem_head); i++ ) {	// 4: IST + 3 ADR Bytes
									/* corrupted empty page found, leave as it is */
									if ( 0xFF != self->uint8Spi[i] ) {
										uint8Good = 0;	// try to find next free clean page
									}	
								}
								/* save page number for next circular buffer entry */
								if ( 0 != uint8Good ) {
									((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint32StartPageWrite = self->uint32IterPage;	
									((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint8Init = 1;	// prepared for writing next element
								}
							}
						}
					}
					/* request next header of circular buffer */
					self->uint32IterPage = 	((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint32StartSector * 
											SPI_FLASH_CB_TYPES[self->uint8FlashType].uint32FlashTopoSectorSizeByte
											+
											((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint16NumPagesPerElem *
											SPI_FLASH_CB_TYPES[self->uint8FlashType].uint32FlashTopoPageSizeByte *
											self->uint16IterElem;
					self->uint16SpiLen = 4 + sizeof(spi_flash_cb_elem_head);	// +4: IST + 24Bit ADR			
					memset(self->uint8Spi, 0, self->uint16SpiLen);
					self->uint8Spi[0] = SPI_FLASH_CB_TYPES[self->uint8FlashType].uint8FlashIstRdData;
					self->uint8Spi[1] = ((self->uint32IterPage >> 16) & 0xFF);	// High Address
					self->uint8Spi[2] = ((self->uint32IterPage >> 8) & 0xFF);
					self->uint8Spi[3] = (self->uint32IterPage & 0xFF);			// Low Address
					/* prepare iterator for next */
					if ( self->uint16IterElem < ((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint16NumEntriesMax ) {
						/* next element in current queue */
						(self->uint16IterElem)++;
					} else {
						/* Free Page Found */
						if ( 0 != ((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint8Init) {
							/* prepare for next queue */
							self->uint16IterElem = 0;	// reset element counter
							(self->uint8IterCb)++;		// process next queue
							/* look ahead if service of some queue can skipped */
							for ( uint8_t i=self->uint8IterCb; i<self->uint8NumCbs; i++ ) {
								/* all active queues processed */
								if ( 0 == ((spi_flash_cb_elem*) self->ptrCbs)[i].uint8Used ) {
									self->uint16SpiLen = 0;
									self->uint8Cmd = SFCB_CMD_IDLE;
									self->uint8Stg = SFCB_STG_00;
									self->uint8Busy = 0;
									return;
								/* active queue found */
								} else {
									if ( 0 == ((spi_flash_cb_elem*) self->ptrCbs)[i].uint8Init ) {
										self->uint8IterCb = i;	// search for unintializied queue, in case of only on circular buffer needes to rebuild
										break;
									}
								}
							}
							/* all available queues processed, go in idle */
							if ( !(self->uint8IterCb < self->uint8NumCbs) ) {
								self->uint16SpiLen = 0;
								self->uint8Cmd = SFCB_CMD_IDLE;
								self->uint8Stg = SFCB_STG_00;
								self->uint8Busy = 0;
								return;
							}
						/* Go on with sector erase */
						} else {
							self->uint8Spi[0] = SPI_FLASH_CB_TYPES[self->uint8FlashType].uint8FlashIstWrEnable;	// enable write
							self->uint16SpiLen = 1;
							self->uint8Stg = SFCB_STG_02;
						}
					}
					return;	// DONE or SPI transfer is required
					break;
				/* Prepare Sector ERASE */	
				case SFCB_STG_02:
					neorv32_uart0_printf("ERASE");
					self->uint16SpiLen = 0;	// TODO
					break;
				/* something strange happend */
				default:
					break;
			}
		/* 
		 * 
		 * Add New Element to Circular Buffer
		 * 
		 */
		case SFCB_CMD_ADD:
			switch (self->uint8Stg) {
				/* check for WIP */
				case SFCB_STG_00:
					if ( (0 == self->uint16SpiLen) || (0 != (self->uint8Spi[1] & SPI_FLASH_CB_TYPES[self->uint8FlashType].uint8FlashMngWipMsk)) ) {
						/* First Request or WIP */
						self->uint8Spi[0] = SPI_FLASH_CB_TYPES[self->uint8FlashType].uint8FlashIstRdStateReg;
						self->uint8Spi[1] = 0;
						self->uint16SpiLen = 2;
						return;
					}
					self->uint8Stg = SFCB_STG_01;	// Go one with search for Free Segment
					__attribute__ ((fallthrough));	// Go one with next
				/* Circular Buffer written, if not write enable */
				case SFCB_STG_01:
					/* Page Requested to program */
					if ( self->uint16IterElem < self->uint16CbElemPlSize ) {
						/* enable WRITE Latch */
						self->uint8Spi[0] = SPI_FLASH_CB_TYPES[self->uint8FlashType].uint8FlashIstWrEnable;	// uint8FlashIstWrEnable
						self->uint16SpiLen = 1;
						self->uint8Stg = SFCB_STG_02;	// Page Write as next
						return;
					/* circular buffer written */
					} else {
						self->uint16SpiLen = 0;
						self->uint8Cmd = SFCB_CMD_IDLE;
						self->uint8Stg = SFCB_STG_00;
						self->uint8Busy = 0;
						return;
					}
					break;
				/* Page Write to Circular Buffer */
				case SFCB_STG_02:
					/* assemble Flash Instruction packet */
					self->uint8Spi[0] = SPI_FLASH_CB_TYPES[self->uint8FlashType].uint8FlashIstWrPage;	// write page
					self->uint8Spi[1] = ((self->uint32IterPage >> 16) & 0xFF);	// High Address
					self->uint8Spi[2] = ((self->uint32IterPage >> 8) & 0xFF);
					self->uint8Spi[3] = (self->uint32IterPage & 0xFF);			// Low Address
					self->uint16SpiLen = 4;
					/* on first packet add header */
					uint16PagesBytesAvail = SPI_FLASH_CB_TYPES[self->uint8FlashType].uint32FlashTopoPageSizeByte;
					if ( 0 == self->uint16IterElem ) {
						memset(&head, 0, sizeof(head));	// make empty
						head.uint32MagicNum = ((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint32MagicNum;
						head.uint32IdNum = ((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint32IdNumMax + 1;
						memcpy((self->uint8Spi+self->uint16SpiLen), &head, sizeof(head));
						self->uint16SpiLen += sizeof(head);
						uint16PagesBytesAvail -= sizeof(head);
					}
					/* determine number of bytes to copy */
					if ( self->uint16CbElemPlSize - self->uint16IterElem > uint16PagesBytesAvail ) {
						uint16CpyLen = uint16PagesBytesAvail;
					} else {
						uint16CpyLen = self->uint16CbElemPlSize - self->uint16IterElem;
					}
					/* assemble packet */
					memcpy((self->uint8Spi+self->uint16SpiLen), (self->ptrCbElemPl+self->uint16IterElem), uint16CpyLen);
					self->uint16SpiLen += uint16CpyLen;
					self->uint16IterElem += uint16CpyLen;
					/* increment iterators */
					(self->uint32IterPage)++;
					/* Go to wait WIP */
					self->uint8Stg = SFCB_STG_00;
					return;
					break;
				/* something strange happend */
				default:
					break;
			}
		
		
		
		
		
		

		/* something strange happend */
		default:
			break;
	}
}


/**
 *  sfcb_mkcb
 *    build up queues with circular buffer
 */
int sfcb_mkcb (spi_flash_cb *self)
{	
	/* no jobs pending */
	if ( 0 != self->uint8Busy ) {
		return 1;	// busy
	}
	/* check for at least one active queue */
	if ( 0 == ((spi_flash_cb_elem*) self->ptrCbs)[0].uint8Used ) {
		return 2;	// no active queue
	}
	/* Find first queue which needs an build */
	self->uint8IterCb = 0;
	for ( uint8_t i=0; i<self->uint8NumCbs; i++ ) {
		if ( (0 == ((spi_flash_cb_elem*) self->ptrCbs)[i].uint8Used) || (0 == ((spi_flash_cb_elem*) self->ptrCbs)[i].uint8Init) ) {
			break;
		}
		self->uint8IterCb = i;	// search for unintializied queue, in case of only on circular buffer needes to rebuild
	}
	/* Setup new Job */
	self->uint8Cmd = SFCB_CMD_MKCB;
	self->uint16IterElem = 0;
	self->uint8Stg = SFCB_STG_00;
	self->uint8Error = SFCB_ERO_NO;
	self->uint8Busy = 1;
	/* fine */
	return 0;
}


/**
 *  sfcb_add
 *    inserts element into circular buffer
 */
int sfcb_add (spi_flash_cb *self, uint8_t cbID, void *data, uint16_t len)
{	
	/* no jobs pending */
	if ( 0 != self->uint8Busy ) {
		return 1;	// Worker is busy, wait for processing last job
	}
	/* check if CB is init for request */
	if ( (0 == ((spi_flash_cb_elem*) self->ptrCbs)[cbID].uint8Used) || (0 == ((spi_flash_cb_elem*) self->ptrCbs)[cbID].uint8Init) ) {
		return 2;	// Circular Buffer is not prepared for adding new element, run #sfcb_worker
	}
	/* check for match into circular buffer size */
	if ( len > (((spi_flash_cb_elem*) self->ptrCbs)[cbID].uint16NumPagesPerElem * SPI_FLASH_CB_TYPES[self->uint8FlashType].uint32FlashTopoPageSizeByte) ) {
		return 4;	// data segement is larger then reserved circular buffer space
	}
	/* store information for insertion */
	self->uint8IterCb = cbID;	// used as pointer to queue
	((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint8Init = 0;	// mark as dirty, for next write run #sfcb_mkcb
	self->uint32IterPage = ((spi_flash_cb_elem*) self->ptrCbs)[self->uint8IterCb].uint32StartPageWrite;	// select page to write
	self->ptrCbElemPl = data;
	self->uint16CbElemPlSize = len;
	self->uint16IterElem = 0;	// used as ptrCbElemPl written pointer
	/* Setup new Job */
	self->uint8Busy = 1;
	self->uint8Cmd = SFCB_CMD_ADD;
	self->uint8Stg = SFCB_STG_00;
	self->uint8Error = SFCB_ERO_NO;
	/* fine */
	return 0;
}



