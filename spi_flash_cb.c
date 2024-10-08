/***********************************************************************
 @copyright     : Siemens AG
 @license       : BSDv3
 @author        : Andreas Kaeberlein
 @address       : Clemens-Winkler-Strasse 3, 09116 Chemnitz

 @maintainer    : Andreas Kaeberlein
 @telephone     : +49 371 4810-2108
 @email         : andreas.kaeberlein@siemens.com

 @file          : spi_flash_cb.c
 @date          : July 22, 2022
 @see           : https://github.com/andkae/SPI-Flash-Circular-Buffer

 @brief         : SPI FLash Circular Buffer
                  Driver for handling Circular Buffers in SPI Flashes
***********************************************************************/



/** Includes **/
/* Standard libs */
#include <stdint.h>     // defines fixed data types: int8_t...
#include <stddef.h>     // various variable types and macros: size_t, offsetof, NULL, ...
#include <string.h>     // string operation: memset, memcpy
/* Self */
#include "sfcb_flash_types.h"   // supported spi flashes
#include "spi_flash_cb.h"       // function prototypes



/**
 *  @defgroup SFCB_PRINTF_EN
 *
 *  redirect sfcb_printf to printf
 *
 *  @{
 */
#ifdef SFCB_PRINTF_EN
    #include <stdio.h>  // allow outputs in unit test
    #define sfcb_printf(...) printf(__VA_ARGS__)
#else
    #define sfcb_printf(...)
#endif
/** @} */   // DEBUG



/**
 *  @defgroup FALL_THROUGH
 *
 *  @brief Fallthrough
 *
 *  Defines fallthrough only for newer compiler
 *  avoids warning 'error: empty declaration __attribute__((fallthrough))'
 *
 *  @since  2023-01-07
 *  @see https://stackoverflow.com/questions/45349079/how-to-use-attribute-fallthrough-correctly-in-gcc
 */
#if defined(__GNUC__) && __GNUC__ >= 7
    #define FALL_THROUGH __attribute__ ((fallthrough))
#else
    #define FALL_THROUGH ((void)0)
#endif /* __GNUC__ >= 7 */
/** @} */   // FALL_THROUGH




/**
 *  @defgroup MIN_MAX
 *
 *  @brief MIN/MAX
 *
 *  Calculates MIN/MAX of two numbers
 *
 *  @since  2023-08-15
 *  @see https://stackoverflow.com/questions/3437404/min-and-max-in-c
 */
#define sfcb_max(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

#define sfcb_min(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })
/** @} */   // MIN_MAX



/**
 *  @brief ceildivide
 *
 *  dividing with always rounding up
 *
 *  @param[in]      dividend        input to be divided
 *  @param[in]      divisor         divider of input
 *  @return         uint32_t        rounded up quotient
 *  @since          July 25, 2022
 *  @author         Andreas Kaeberlein
 */
static uint32_t sfcb_ceildivide_uint32 (uint32_t dividend, uint32_t divisor)
{
    if ( dividend != 0 ) {
        return ( 1 + ( (dividend-1) / divisor ) );
    };
    return ( ( dividend + divisor - 1) / divisor );
}



/**
 *  @brief address translation
 *
 *  converts 32bit SPI Flash address to SPI packet convenient byte sequence
 *
 *  @param[in]      adr             Flash address
 *  @param[in,out]  *spi            SPI buffer, takes converted address place
 *  @param[in]      adrBytes        Number of bytes for address
 *  @return         void
 *  @since          December 31, 2022
 *  @author         Andreas Kaeberlein
 */
static void sfcb_adr32_uint8 (uint32_t adr, uint8_t *spi, uint8_t adrBytes)
{
    /* on highest index is lowest byte placed */
    for ( int8_t i = (int8_t) (adrBytes-1); i >= 0; i-- ) {
        spi[i] = (uint8_t) (adr & 0xFF);    // get lowest byte
        adr = adr >> 8; // shift one byte right
    }
}



/**
 *  @brief write-in-progress check
 *
 *  assembles SPI packet and checks if in flash is write operation active
 *
 *  @param[in,out]  self                handle, #t_sfcb
 *  @return         int                 WIP packet required?
 *  @retval         0                   No New WIP Request packet necessary
 *  @retval         -1                  Issue new WIP poll request
 *  @since          August 16, 2023
 *  @author         Andreas Kaeberlein
 */
static int sfcb_spi_wip_poll (t_sfcb *self)
{
    if ( (0 == self->uint16SpiLen) || (0 != (self->uint8PtrSpi[1] & SFCB_FLASH_MNG_WIP_MSK)) ) {
        /* First Request or WIP */
        self->uint8PtrSpi[0] = SFCB_FLASH_IST_RD_STATE_REG;
        self->uint8PtrSpi[1] = 0;
        self->uint16SpiLen = 2;
        return -1;
    }
    self->uint16SpiLen = 0;
    return 0;
}



/**
 *  @brief flash address queue element header calculation
 *
 *  calculates based on flash topology and circular buffer settings the belonging
 *  physical flash address
 *
 *  @param[in,out]  self                handle, #t_sfcb
 *  @param[in]      elem                physical element number in queue
 *  @return         uint32_t            flash address of header from circular buffer element
 *  @since          August 17, 2024
 *  @author         Andreas Kaeberlein
 */
static uint32_t sfcb_flash_adr_head (t_sfcb *self, uint16_t elem)
{
    /** Variables **/
    uint32_t    adr;
    /* calculate address */
    adr =   ((self->ptrCbs)[self->uint8IterCb]).uint32StartSector * (uint32_t) SFCB_FLASH_TOPO_SECTOR_SIZE  // start address of circular buffer queue
            +
            ((self->ptrCbs)[self->uint8IterCb]).uint16NumPagesPerElem * (uint32_t) SFCB_FLASH_TOPO_PAGE_SIZE * elem;    // offset based on element count
    /* return header address */
    return adr;
}



/**
 *  @brief SPI packet header request
 *
 *  assembles SPI packet to request circular buffer element header based on internal state
 *  the spi packet takes places in the internal SPI buffer variable
 *  @see #spi_flash_cb_elem_head
 *
 *  @param[in,out]  self                handle, #t_sfcb
 *  @return         void
 *  @since          August 17, 2024
 *  @author         Andreas Kaeberlein
 */
static void sfcb_spi_get_head (t_sfcb *self)
{
    self->uint16SpiLen = SFCB_FLASH_TOPO_ADR_BYTE + 1 + sizeof(spi_flash_cb_elem_head);     // +1: IST, + Address bytes
    memset(self->uint8PtrSpi, 0, self->uint16SpiLen);   // make empty
    self->uint8PtrSpi[0] = SFCB_FLASH_IST_RD_DATA;      // Flash read instruction
    sfcb_adr32_uint8(self->uint32IterAdr, self->uint8PtrSpi+1, SFCB_FLASH_TOPO_ADR_BYTE);   // serialize address into bytes, +1 first byte is instruction
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
        return SFCB_E_NO_FLASH; // no flash type selected, use proper compile switch
    }
    sfcb_printf("  INFO:%s: flash '%s' selected\n", __FUNCTION__, SFCB_FLASH_NAME);
    /* set up list of flash circular buffers */
    self->uint8NumCbs = cbLen;
    self->uint16SpiLen = 0;
    self->uint8Busy = 0;
    self->cmd = SFCB_CMD_IDLE;  // free for request
    self->stage = SFCB_STG00;
    self->ptrCbs = (t_sfcb_cb*) cb;     // circular buffer element array
    self->uint8PtrSpi = (uint8_t*) spi; // uint8 array
    self->uint16SpiMax = spiLen;        // max array length
    self->error = SFCB_E_NOERO;
    self->ptrCbElemPl = NULL;
    self->uint16CbElemPlSize = 0;
    /* memory addresses */
    sfcb_printf("  INFO:%s:sfcb:spi_p            = %p\n", __FUNCTION__, self->uint8PtrSpi); // spi buffer
    /* SPI buffer needs at least space for one page and address and instruction */
    if ( (SFCB_FLASH_TOPO_PAGE_SIZE + SFCB_FLASH_TOPO_ADR_BYTE + 1) > self->uint16SpiMax ) {
        sfcb_printf("  ERROR:%s: spi buffer to small, is=%d byte, req=%d byte\n", __FUNCTION__, self->uint16SpiMax, SFCB_FLASH_TOPO_PAGE_SIZE + SFCB_FLASH_TOPO_ADR_BYTE + 1);
        return SFCB_E_MEM;  // not enough SPI buffer to write at least one complete page to flash
    }
    /* init circular buffer handles */
    for ( uint8_t i = 0; i < (self->uint8NumCbs); i++ ) {
        (self->ptrCbs[i]).uint8Used = 0;
        (self->ptrCbs[i]).uint8MgmtValid = 0;
        sfcb_printf("  INFO:%s:ptrCbs[%i]_p           = %p\n", __FUNCTION__, i, (&self->ptrCbs[i]));                    // unit test output
        sfcb_printf("  INFO:%s:ptrCbs[%i].uint8Used_p = %p\n", __FUNCTION__, i, &((self->ptrCbs[i]).uint8Used));        // output address
        sfcb_printf("  INFO:%s:ptrCbs[%i].uint8Init_p = %p\n", __FUNCTION__, i, &((self->ptrCbs[i]).uint8MgmtValid));   // output address
    }
    /* normal end */
    return SFCB_OK;
}



/**
 *  spi_flash_cb_worker
 *    executes request from ...
 */
void sfcb_worker (t_sfcb *self)
{
    /** Variables **/
    uint8_t     uint8Good;              // check was good
    uint16_t    uint16PagesBytesAvail;  // number of used page bytes
    uint16_t    uint16CpyLen;           // number of Bytes to copy
    uint32_t    uint32Temp;             // temporary 32bit variable

    /* Function call message */
    sfcb_printf("__FUNCTION__ = %s\n", __FUNCTION__);
    sfcb_printf("  INFO:%s:sfcb_p            = %p\n", __FUNCTION__, self);
    sfcb_printf("  INFO:%s:sfcb:spi_p        = %p\n", __FUNCTION__, self->uint8PtrSpi); // spi buffer
    /* select part of FSM */
    switch (self->cmd) {
        /*
         *
         * nothing to do
         *
         */
        case SFCB_CMD_IDLE:
            sfcb_printf("  INFO:%s:IDLE\n", __FUNCTION__);
            return;
        /*
         *
         * Allocate Next Free Element for Circular Buffers
         *
         */
        case SFCB_CMD_MKCB:
            /* select execution state, allows break & cont */
            switch (self->stage) {
                /* check for WIP */
                case SFCB_STG00:
                    /* Debug message */
                    sfcb_printf("  INFO:%s:MKCB:STG0: check for WIP, request first header\n", __FUNCTION__);
                    /* WIP Check */
                    if ( 0 != sfcb_spi_wip_poll(self) ) return;
                    /* Request first header of circular buffer element */
                    self->uint32IterAdr = sfcb_flash_adr_head(self, self->uint16Iter);  // calculate physical flash address of first header of circular buffer queue
                    sfcb_spi_get_head(self);    // assemble SPI packet
                    /* debug message */
                    sfcb_printf("  INFO:%s:MKCB:STG0:FLASH: adr=0x%x, len=%i", __FUNCTION__, self->uint32IterAdr, (uint32_t) sizeof(spi_flash_cb_elem_head));
                    /* build overview of circular buffer queue */
                    self->stage = SFCB_STG01;   // Go one with search for Free Segment
                    return;
                /* check header, find empty page for new element */
                case SFCB_STG01:
                    /* Check header of CBQ element */
                    sfcb_printf("  INFO:%s:MKCB:STG1: check header, request footer of queue element, find empty start page for new element\n", __FUNCTION__);
                    sfcb_printf("  INFO:%s:MKCB:STG1:SPI: ", __FUNCTION__);
                    for ( uint8_t i = 0; i < (uint8_t) (sizeof(spi_flash_cb_elem_head) + SFCB_FLASH_TOPO_ADR_BYTE + 1); i++ ) { // +1: SPI Flash IST
                        sfcb_printf("0x%x ", self->uint8PtrSpi[i]);
                    }
                    sfcb_printf("\n");
                    /* copy head from SPI packet*/
                    memcpy(&(self->head), self->uint8PtrSpi+SFCB_FLASH_TOPO_ADR_BYTE+1, sizeof(self->head));  // ensure alignment to processor architecture
                    sfcb_printf("  INFO:%s:MKCB:STG1: RDHEAD,magicnum=0x%x\n", __FUNCTION__, (self->head).uint32MagicNum);
                    /* Flash Area is used by circular buffer, check magic number
                     *   +4: Read instruction + 32bit address
                     */
                    if ( (self->head).uint32MagicNum == ((self->ptrCbs)[self->uint8IterCb]).uint32MagicNum ) {
                        /* Debug Message */
                        sfcb_printf("  INFO:%s:MKCB:STG1: Valid Entry Found\n", __FUNCTION__);
                        /* count available elements */
                        (((self->ptrCbs)[self->uint8IterCb]).uint16NumEntries)++;
                        /* get highest number of numbered circular buffer elements, needed for next entry */
                        if ( (self->head).uint32IdNum > ((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMax ) {
                            /* save new highest number in circular buffer */
                            ((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMax = (self->head).uint32IdNum;
                            self->uint32LastElemAdr = self->uint32IterAdr;      // store temporary, footer check pending, needed by sfcb_get_last
                            self->uint32LastElemNum = (self->head).uint32IdNum; // store temporary
                        }
                        /* get lowest number of circular buffer, needed for erase sector, and start get function */
                        if ( (self->head).uint32IdNum < ((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMin ) {
                            ((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMin = (self->head).uint32IdNum;
                            ((self->ptrCbs)[self->uint8IterCb]).uint32StartPageIdMin = self->uint32IterAdr; // needed by Sector-Erase
                        }
                    } else {
                        /* check for unused header
                         * first unused pages is allocated, iterate over all elements to get all IDs
                         */
                        if ( 0 == ((self->ptrCbs)[self->uint8IterCb]).uint8MgmtValid ) {
                            uint8Good = 1;
                            for ( uint8_t i = SFCB_FLASH_TOPO_ADR_BYTE + 1; i < SFCB_FLASH_TOPO_ADR_BYTE + 1 + sizeof(spi_flash_cb_elem_head); i++ ) {  // +1: IST
                                /* corrupted empty page found, leave as it is */
                                if ( 0xFF != self->uint8PtrSpi[i] ) {
                                    uint8Good = 0;  // try to find next free clean page
                                    sfcb_printf ("  ERROR:%s:MKCB:STG1: corrupted empty page found at 0x%0x\n", __FUNCTION__, self->uint32IterAdr);
                                    break;
                                }
                            }
                            /* save page number for next circular buffer entry */
                            if ( 0 != uint8Good ) {
                                ((self->ptrCbs)[self->uint8IterCb]).uint32StartPageWrite = self->uint32IterAdr;
                                ((self->ptrCbs)[self->uint8IterCb]).uint8MgmtValid = 1; // prepared for writing next element
                            }
                        }
                    }
                    /* Current Status Message */
                    sfcb_printf ( "  INFO:%s:MKCB:STG1: cb=%d, elem=%d, flashadr=0x%x, idIs=0x%x, idMin=0x%x, idMax=0x%x\n",
                                  __FUNCTION__,
                                  self->uint8IterCb,
                                  self->uint16Iter,
                                  self->uint32IterAdr,
                                  (self->head).uint32IdNum,
                                  ((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMin,
                                  ((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMax
                                );
                    /* assemble footer request of circular buffer element
                     *   +1: head from the next queue element touches footer from the current queue element
                     *       therefore going for the next head and subtract the head will lead to the flash address
                     *       of the footer in the current queue element
                     */
                    self->uint32IterAdr = sfcb_flash_adr_head(self, (uint16_t) ((self->uint16Iter) + 1)) - (uint32_t) sizeof(spi_flash_cb_elem_head);
                    sfcb_spi_get_head(self);
                    /* go on with footer evaluation and requesting next header */
                    self->stage = SFCB_STG02;
                    /* debug message */
                    sfcb_printf("  INFO:%s:MKCB:STG1:FLASH: adr=0x%x, len=%i\n", __FUNCTION__, self->uint32IterAdr, (uint32_t) sizeof(spi_flash_cb_elem_head));
                    return;
                /* check footer, needed to ensure that #sfcb_get_last will get an complete circular buffer queue element */
                case SFCB_STG02:
                    /* Debug Message */
                    sfcb_printf("  INFO:%s:MKCB:STG2: check footer, request header of next queue element\n", __FUNCTION__);
                    sfcb_printf("  INFO:%s:MKCB:STG2:SPI: ", __FUNCTION__);
                    for ( uint8_t i = 0; i < (uint8_t) (sizeof(spi_flash_cb_elem_head) + SFCB_FLASH_TOPO_ADR_BYTE + 1); i++ ) { // +1: SPI Flash IST
                        sfcb_printf("0x%x ", self->uint8PtrSpi[i]);
                    }
                    sfcb_printf("\n");
                    /* copy from SPI packet */
                    memcpy(&(self->foot), self->uint8PtrSpi+SFCB_FLASH_TOPO_ADR_BYTE+1, sizeof(self->foot));
                    /* header = footer? if yes, cb element completely written */
                    if (    (0 == memcmp(&(self->foot), &(self->head), sizeof(self->head)))
                         && (self->foot).uint32MagicNum == ((self->ptrCbs)[self->uint8IterCb]).uint32MagicNum
                    ) {
                        sfcb_printf (   "  INFO:%s:MKCB:STG2:head/foot: compare pass, successful last written element is at flash adr=0x%x\n",
                                        __FUNCTION__, self->uint32LastElemAdr
                                    );
                        ((self->ptrCbs)[self->uint8IterCb]).uint32StartPageIdMax = self->uint32LastElemAdr; // needed by sfcb_get_last
                        ((self->ptrCbs)[self->uint8IterCb]).uint32ElemIdLastCpl  = self->uint32LastElemNum; // needed by sfcb_get_last
                    }
                    /* request next header of circular buffer */
                    self->uint32IterAdr = sfcb_flash_adr_head(self, (uint16_t) ((self->uint16Iter) + 1));
                    sfcb_spi_get_head(self);    // assemble SPI packet for request next head
                    /* debug message */
                    sfcb_printf("  INFO:%s:MKCB:STG2:FLASH: adr=0x%x, len=%i\n", __FUNCTION__, self->uint32IterAdr, (uint32_t) sizeof(spi_flash_cb_elem_head));
                    /* prepare iterator for next */
                    if ( self->uint16Iter < (((self->ptrCbs)[self->uint8IterCb]).uint16NumEntriesMax-1) ) {
                        /* next element in current queue */
                        (self->uint16Iter)++;
                        self->stage = SFCB_STG01;   // process next header
                    } else {
                        /* Free Page Found */
                        if ( 0 != ((self->ptrCbs)[self->uint8IterCb]).uint8MgmtValid) {
                            /* prepare for next queue */
                            self->uint16Iter = 0;   // reset element counter
                            (self->uint8IterCb)++;      // process next queue
                            /* look ahead if service of some queue can skipped */
                            for ( uint8_t i=self->uint8IterCb; i<self->uint8NumCbs; i++ ) {
                                /* all active circular buffer queues processed? */
                                if ( 0 == ((self->ptrCbs)[i]).uint8Used ) {
                                    self->uint16SpiLen = 0;
                                    self->cmd = SFCB_CMD_IDLE;
                                    self->stage = SFCB_STG00;
                                    self->uint8Busy = 0;
                                    return;
                                /* active queue found */
                                } else {
                                    if ( 0 == ((self->ptrCbs)[i]).uint8MgmtValid ) {
                                        self->uint8IterCb = i;  // search for uninitialized queue, in case of only on circular buffer needes to rebuild
                                        break;
                                    }
                                }
                            }
                            /* all available queues processed, go in idle */
                            if ( !(self->uint8IterCb < self->uint8NumCbs) ) {
                                self->uint16SpiLen = 0;
                                self->cmd = SFCB_CMD_IDLE;
                                self->stage = SFCB_STG00;
                                self->uint8Busy = 0;
                                return;
                            }
                        /* Go on with sector erase */
                        } else {
                            self->uint8PtrSpi[0] = SFCB_FLASH_IST_WR_ENA;   // enable write
                            self->uint16SpiLen = 1;
                            self->stage = SFCB_STG03;
                        }
                    }
                    return; // DONE or SPI transfer is require
                /* Assemble Command for Sector ERASE */
                case SFCB_STG03:
                    sfcb_printf( "  INFO:%s:MKCB:STG3: Assemble Command for Sector ERASE\n", __FUNCTION__);
                    sfcb_printf( "  INFO:%s:MKCB:STG3: cb=%d, uint32StartPageIdMin=0x%x\n",
                                 __FUNCTION__,
                                 self->uint8IterCb,
                                 ((self->ptrCbs)[self->uint8IterCb]).uint32StartPageIdMin
                                );
                    uint32Temp = (self->ptrCbs[self->uint8IterCb]).uint32StartPageIdMin;        // get startpage of oldest entry, prepare for delete
                    uint32Temp = (uint32Temp & (uint32_t) ~(SFCB_FLASH_TOPO_SECTOR_SIZE - 1));  // align to sub sector address
                    self->uint8PtrSpi[0] = SFCB_FLASH_IST_ERASE_SECTOR;
                    sfcb_adr32_uint8(uint32Temp, self->uint8PtrSpi+1, SFCB_FLASH_TOPO_ADR_BYTE);    // +1 first byte is instruction
                    self->uint16SpiLen = SFCB_FLASH_TOPO_ADR_BYTE + 1;  // address + instruction
                    self->stage = SFCB_STG04;
                    return; // DONE or SPI transfer is required
                /* Wait for Sector Erase */
                case SFCB_STG04:
                    sfcb_printf("  INFO:%s:MKCB:STG4: Wait for Sector Erase\n", __FUNCTION__);
                    /* Start at zero Element with search for free page */
                    self->uint16Iter = 0;
                    /* Assemble command for WIP */
                    self->uint8PtrSpi[0] = SFCB_FLASH_IST_RD_STATE_REG;
                    self->uint8PtrSpi[1] = 0;
                    self->uint16SpiLen = 2;
                    self->stage = SFCB_STG00;   // wait for erase, and search for free page for next element
                    return; // DONE or SPI transfer is required
                /* something strange happened */
                default:
                    sfcb_printf("  ERROR:%s:MKCB: unexpected use of default path\n", __FUNCTION__);
                    self->error = SFCB_E_UNKBEH;
                    break;
            }
            return;
        /*
         *
         * Add New Element to Circular Buffer
         *
         */
        case SFCB_CMD_ADD:
            switch (self->stage) {
                /* check for WIP */
                case SFCB_STG00:
                    sfcb_printf("  INFO:%s:ADD:STG0: check for WIP\n", __FUNCTION__);
                    /* WIP Check */
                    if ( 0 != sfcb_spi_wip_poll(self) ) return;
                    /* free for new request */
                    self->stage = SFCB_STG01;   // go on with enable write
                    FALL_THROUGH;               // no SPI request required, therefore go on
                /* Circular Buffer written, if not write enable */
                case SFCB_STG01:
                    sfcb_printf("  INFO:%s:ADD:STG1: Circular Buffer completly written, if not write enable\n", __FUNCTION__);
                    /* Speculative expect Write, Enable Write Latch */
                    self->uint8PtrSpi[0] = SFCB_FLASH_IST_WR_ENA;   // uint8FlashIstWrEnable
                    self->uint16SpiLen = 1;
                    /* Header/Footer write required */
                    if (    (self->uint32IterAdr == ((self->ptrCbs)[self->uint8IterCb]).uint32StartPageWrite)   // Start of Circular Buffer Write
                         || (((self->ptrCbs)[self->uint8IterCb]).uint16PlFlashOfs == (((self->ptrCbs)[self->uint8IterCb]).uint16PlSize + sizeof(spi_flash_cb_elem_head)))   // End of Circular Buffer Write
                    ) {
                        self->stage = SFCB_STG02;   // Write Header/Footer
                        return;
                    /* Page Requested to program or footer write */
                    } else if ( (self->uint16Iter < self->uint16CbElemPlSize) ) {
                        self->stage = SFCB_STG03;   // Write Payload as next
                        return;
                    /* circular buffer written */
                    } else {
                        self->uint16SpiLen = 0;
                        self->cmd = SFCB_CMD_IDLE;
                        self->stage = SFCB_STG00;
                        self->uint8Busy = 0;
                        return;
                    }
                    break;
                /* Page Write to Circular Buffer */
                case SFCB_STG02:
                    sfcb_printf("  INFO:%s:ADD:STG2: Write Header/Footer to Flash, adr=0x%x, payload,len=%d\n", __FUNCTION__, self->uint32IterAdr, (uint32_t) sizeof(self->head));
                    /* assemble Header/ Footer */
                    memset(&(self->head), 0, sizeof(self->head)); // make empty
                    (self->head).uint32MagicNum = ((self->ptrCbs)[self->uint8IterCb]).uint32MagicNum;
                    (self->head).uint32IdNum = ((self->ptrCbs)[self->uint8IterCb]).uint32IdNumMax + 1;
                    /* Page Write */
                    self->uint8PtrSpi[0] = SFCB_FLASH_IST_WR_PAGE;
                    self->uint16SpiLen = 1;
                    /* Footer? */
                    if ( ((self->ptrCbs)[self->uint8IterCb]).uint16PlFlashOfs == (((self->ptrCbs)[self->uint8IterCb]).uint16PlSize + sizeof(spi_flash_cb_elem_head)) ) {
                        self->uint32IterAdr =   ((self->ptrCbs)[self->uint8IterCb]).uint32StartPageWrite
                                              + ((self->ptrCbs)[self->uint8IterCb]).uint16NumPagesPerElem * (uint32_t) SFCB_FLASH_TOPO_PAGE_SIZE
                                              - (uint32_t) sizeof(self->head);
                        ++(((self->ptrCbs)[self->uint8IterCb]).uint16PlFlashOfs);   // footer write is only entered one time
                    } else {    // Header
                        ((self->ptrCbs)[self->uint8IterCb]).uint16PlFlashOfs = (uint16_t) (((self->ptrCbs)[self->uint8IterCb]).uint16PlFlashOfs + sizeof(self->head));
                    }
                    /* SPI Packet: Set address */
                    sfcb_adr32_uint8(self->uint32IterAdr, self->uint8PtrSpi+self->uint16SpiLen, SFCB_FLASH_TOPO_ADR_BYTE);
                    (self->uint16SpiLen) = (uint16_t) ((self->uint16SpiLen) + SFCB_FLASH_TOPO_ADR_BYTE);
                    /* SPI Packet: Copy Payload*/
                    memcpy((self->uint8PtrSpi+self->uint16SpiLen), &(self->head), sizeof(self->head));
                    (self->uint16SpiLen) = (uint16_t) ((self->uint16SpiLen) + sizeof(self->head));
                    /* Update Flash Address Counter */
                    (self->uint32IterAdr) += (uint32_t) sizeof(self->head);
                    /* Go to wait for WIP */
                    self->stage = SFCB_STG04;
                    return;
                /* Page Write to Circular Buffer */
                case SFCB_STG03:
                    sfcb_printf("  INFO:%s:ADD:STG3: Page Write to Circular Buffer, adr=0x%x, payload,len=%d\n", __FUNCTION__, self->uint32IterAdr, self->uint16CbElemPlSize);
                    /* assemble Flash Instruction packet */
                    self->uint8PtrSpi[0] = SFCB_FLASH_IST_WR_PAGE;  // write page
                    sfcb_adr32_uint8(self->uint32IterAdr, self->uint8PtrSpi+1, SFCB_FLASH_TOPO_ADR_BYTE);   // +1 first byte is instruction
                    self->uint16SpiLen = SFCB_FLASH_TOPO_ADR_BYTE + 1;  // +1: IST
                    /* get available bytes in page */
                    uint16PagesBytesAvail = (uint16_t) (SFCB_FLASH_TOPO_PAGE_SIZE - (self->uint32IterAdr % SFCB_FLASH_TOPO_PAGE_SIZE));
                    /* determine number of bytes to copy */
                    if ( (self->uint16CbElemPlSize - self->uint16Iter) > uint16PagesBytesAvail ) {
                        uint16CpyLen = uint16PagesBytesAvail;
                    } else {
                        uint16CpyLen = (uint16_t) (self->uint16CbElemPlSize - self->uint16Iter);
                    }
                    /* assemble packet */
                    memcpy((self->uint8PtrSpi+self->uint16SpiLen), (self->ptrCbElemPl+self->uint16Iter), uint16CpyLen);
                    self->uint16SpiLen = (uint16_t) (self->uint16SpiLen + uint16CpyLen);
                    self->uint16Iter = (uint16_t) (self->uint16Iter + uint16CpyLen);
                    /* increment iterators */
                    ((self->ptrCbs)[self->uint8IterCb]).uint16PlFlashOfs = (uint16_t) (((self->ptrCbs)[self->uint8IterCb]).uint16PlFlashOfs + self->uint16SpiLen - SFCB_FLASH_TOPO_ADR_BYTE - 1);   // payload internal flash offset
                    self->uint32IterAdr = self->uint32IterAdr + self->uint16SpiLen - SFCB_FLASH_TOPO_ADR_BYTE - 1;  // inc flash address by written data, reduced by SPI Flash instruction
                    /* Go to wait WIP */
                    self->stage = SFCB_STG04;
                    return;
                /* check for WIP, #sfcb_spi_wip_poll works only for spiLen=0, therefore additional state necessary */
                case SFCB_STG04:
                    self->uint16SpiLen = 0; // needed for WIP poll packet generation in STG00
                    self->stage = SFCB_STG00;
                    return;
                /* something strange happend */
                default:
                    sfcb_printf("  ERROR:%s:ADD: default, something strange happend\n", __FUNCTION__);
                    self->error = SFCB_E_UNKBEH;
                    break;
            }
            return;
        /*
         *
         * Get Element from circular buffer
         *
         */
        case SFCB_CMD_GET:
            switch (self->stage) {
                /* check for WIP */
                case SFCB_STG00:
                    sfcb_printf("  INFO:%s:GET:STG0: check for WIP\n", __FUNCTION__);
                    /* WIP Check */
                    if ( 0 != sfcb_spi_wip_poll(self) ) return;
                    /* free for new request */
                    self->stage = SFCB_STG01;   // Go one with search for Free Segment
                    FALL_THROUGH;               // Go one with next
                /* Copy SPI Packet to data buffer */
                case SFCB_STG01:
                    /* debug message */
                    sfcb_printf("  INFO:%s:GET:STG1:\n", __FUNCTION__);
                    /* copy data available? */
                    if ( 0 != self->uint16SpiLen ) {
                        sfcb_printf("  INFO:%s:GET:STG1: Copy bytes in payload buffer\n", __FUNCTION__);
                        uint16CpyLen = (uint16_t) (self->uint16SpiLen - SFCB_FLASH_TOPO_ADR_BYTE - 1);  // -1: for instruction
                        memcpy(self->ptrCbElemPl+self->uint16Iter, self->uint8PtrSpi + SFCB_FLASH_TOPO_ADR_BYTE + 1, uint16CpyLen);
                        self->uint16Iter = (uint16_t) (self->uint16Iter + uint16CpyLen);        // payload byte counter
                        self->uint32IterAdr = (uint32_t) (self->uint32IterAdr + uint16CpyLen);  // flash address byte counter
                    }
                    /* next chunk */
                    self->stage = SFCB_STG02;   // fetch next chunk from flash
                    FALL_THROUGH;               // Request next spi packet
                /* Circular buffer element read-out complete, if not go on with next chunk */
                case SFCB_STG02:
                    /* Request next segment for read */
                    if ( self->uint16Iter < self->uint16CbElemPlSize ) {
                        /* Prepare Package for request */
                        uint16CpyLen = (uint16_t) sfcb_min(SFCB_FLASH_TOPO_PAGE_SIZE, self->uint16CbElemPlSize - self->uint16Iter); // pending bytes, or max page size
                        self->uint16SpiLen = (uint16_t) (uint16CpyLen + SFCB_FLASH_TOPO_ADR_BYTE + 1);  // +1: for instruction
                        memset(self->uint8PtrSpi, 0, self->uint16SpiLen);   // written data is zero
                        /* Flash instruction */
                        self->uint8PtrSpi[0] = SFCB_FLASH_IST_RD_DATA;  // read data
                        sfcb_adr32_uint8(self->uint32IterAdr, self->uint8PtrSpi+1, SFCB_FLASH_TOPO_ADR_BYTE);   // +1 first byte is instruction
                        /* User Message */
                        sfcb_printf("  INFO:%s:GET:STG2: Request next segment from Flash, adr=0x%x, len=%d\n", __FUNCTION__, self->uint32IterAdr, self->uint16SpiLen);
                        /* wait for HW */
                        self->stage = SFCB_STG01;   // Copy read data back
                    /* CB Element Read complete */
                    } else {
                        /* User Message */
                        sfcb_printf("  INFO:%s:GET:STG1: Transfer done\n", __FUNCTION__);
                        /* finish */
                        self->uint16SpiLen = 0;
                        self->cmd = SFCB_CMD_IDLE;
                        self->stage = SFCB_STG00;
                        self->uint8Busy = 0;
                    }
                    return; // Wait for SPI
                /* something strange happend */
                default:
                    sfcb_printf("  ERROR:%s:RAW: unexpected use of default path\n", __FUNCTION__);
                    self->error = SFCB_E_UNKBEH;
                    break;
            }
            return;
        /*
         *
         * Raw Flash Read
         *
         */
        case SFCB_CMD_RAW:
            switch (self->stage) {
                /* check for WIP */
                case SFCB_STG00:
                    sfcb_printf("  INFO:%s:RAW:STG0: check for WIP\n", __FUNCTION__);
                    /* WIP Check */
                    if ( 0 != sfcb_spi_wip_poll(self) ) return;
                    /* free for new request */
                    self->stage = SFCB_STG01;   // Go one with search for Free Segment
                    FALL_THROUGH;               // Go one with stage
                /* Prepare raw read */
                case SFCB_STG01:
                    sfcb_printf("  INFO:%s:RAW:STG1: Prepare RAW read\n", __FUNCTION__);
                    /* check for enough spi buf */
                    if ( self->uint16SpiMax < (self->uint16CbElemPlSize + SFCB_FLASH_TOPO_ADR_BYTE + 1) ) { // IST (+1) + ADR_BYTE: caused by read instruction
                        self->uint8Busy = 0;
                        self->cmd = SFCB_CMD_IDLE;  // go in idle
                        self->stage = SFCB_STG00;
                        self->error = SFCB_E_BUFSIZE;   // requested operation ends with an error
                    }
                    /* SPI package is zero */
                    self->uint16SpiLen = (uint16_t) (self->uint16CbElemPlSize + SFCB_FLASH_TOPO_ADR_BYTE + 1);  // +1: for instruction
                    memset(self->uint8PtrSpi, 0, self->uint16SpiLen);   // written data is zero
                    /* Flash instruction */
                    self->uint8PtrSpi[0] = SFCB_FLASH_IST_RD_DATA;  // read data
                    sfcb_adr32_uint8(self->uint32IterAdr, self->uint8PtrSpi+1, SFCB_FLASH_TOPO_ADR_BYTE);   // +1 first byte is instruction
                    /* go on with next stage */
                    self->stage = SFCB_STG02;   // now wait for transfer
                    return;
                /* copy data from SPI back */
                case SFCB_STG02:
                    sfcb_printf("  INFO:%s:RAW:STG2: copy data from SPI back\n", __FUNCTION__);
                    memcpy(self->ptrCbElemPl, self->uint8PtrSpi+SFCB_FLASH_TOPO_ADR_BYTE+1, self->uint16CbElemPlSize);  // skip header from answer of read instruction
                    self->uint16SpiLen = 0;
                    self->cmd = SFCB_CMD_IDLE;
                    self->stage = SFCB_STG00;
                    self->uint8Busy = 0;
                    return;
                /* something strange happend */
                default:
                    sfcb_printf("  ERROR:%s:RAW: unexpected use of default path\n", __FUNCTION__);
                    self->error = SFCB_E_UNKBEH;
                    break;
            }
            return;

        /* something strange happened */
        default:
            return;
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
    const uint8_t   uint8PagesPerSector = (uint8_t) (SFCB_FLASH_TOPO_SECTOR_SIZE / SFCB_FLASH_TOPO_PAGE_SIZE);
    const uint16_t  elemTotalSize = (uint16_t) (elemSizeByte + 2*sizeof(spi_flash_cb_elem_head));   // payload size + header/footer size
    uint8_t         cbNew;              // queue of circular buffer new entry number
    uint32_t        uint32StartSector;
    uint16_t        uint16NumSectors;

    /* Function call message */
    sfcb_printf("__FUNCTION__ = %s\n", __FUNCTION__);
    sfcb_printf("  INFO:%s:sfcb_p = %p\n", __FUNCTION__, self);
    /* check for free Slot number */
    uint32StartSector = 0;
    for ( cbNew = 0; cbNew < (self->uint8NumCbs); cbNew++ ) {
        if ( 0 != (self->ptrCbs[cbNew]).uint8Used ) {   // queue is used
            uint32StartSector = (self->ptrCbs[cbNew]).uint32StopSector + 1; // calculate start address of next queue
        } else {
            break;  // empty queue found
        }
    }
    if ( cbNew == (self->uint8NumCbs) ) {
        sfcb_printf("  ERROR:%s:sfcb_cb exceeded total available number of %i cbs\n", __FUNCTION__, (self->uint8NumCbs));
        return SFCB_E_MEM;  // no free circular buffer slots, allocate more memory in #t_sfcb_cb table
    }
    /* prepare slot */
    (self->ptrCbs[cbNew]).uint8Used = 1;        // occupied
    (self->ptrCbs[cbNew]).uint32IdNumMax = 0;   // in case of uninitialized memory
    (self->ptrCbs[cbNew]).uint32IdNumMin = __UINT32_MAX__;  // assign highest number
    (self->ptrCbs[cbNew]).uint32MagicNum = magicNum;        // used magic number for the circular buffer
    (self->ptrCbs[cbNew]).uint16NumPagesPerElem = (uint16_t) sfcb_ceildivide_uint32(elemTotalSize, SFCB_FLASH_TOPO_PAGE_SIZE);  // calculate in multiple of pages
    (self->ptrCbs[cbNew]).uint32StartSector = uint32StartSector;
    uint16NumSectors = (uint16_t) sfcb_max(2, (uint16_t) sfcb_ceildivide_uint32((uint32_t) (numElems*((self->ptrCbs[cbNew]).uint16NumPagesPerElem)), uint8PagesPerSector));
    (self->ptrCbs[cbNew]).uint32StopSector = (self->ptrCbs[cbNew]).uint32StartSector+uint16NumSectors-1;
    (self->ptrCbs[cbNew]).uint16NumEntriesMax = (uint16_t) (uint16NumSectors*uint8PagesPerSector) / (self->ptrCbs[cbNew]).uint16NumPagesPerElem;
    (self->ptrCbs[cbNew]).uint16NumEntries = 0;
    (self->ptrCbs[cbNew]).uint16PlSize = elemSizeByte;  // element size, needed to determine footer write
    *cbID = cbNew;
    /* check if stop sector is in total size */
    if ( ((self->ptrCbs[cbNew]).uint32StopSector+1) * SFCB_FLASH_TOPO_SECTOR_SIZE > SFCB_FLASH_TOPO_FLASH_SIZE ) {
        sfcb_printf("  ERROR:%s flash size exceeded\n", __FUNCTION__);
        return SFCB_E_FLASH_FULL;   // Flash capacity exceeded
    }
    /* print slot config */
    sfcb_printf("  INFO:%s:ptrCbs[%i]_p                     = %p\n",   __FUNCTION__, cbNew, (&self->ptrCbs[cbNew]));
    sfcb_printf("  INFO:%s:ptrCbs[%i].uint8Used             = %d\n",   __FUNCTION__, cbNew, (self->ptrCbs[cbNew]).uint8Used);
    sfcb_printf("  INFO:%s:ptrCbs[%i].uint16NumPagesPerElem = %d\n",   __FUNCTION__, cbNew, (self->ptrCbs[cbNew]).uint16NumPagesPerElem);
    sfcb_printf("  INFO:%s:ptrCbs[%i].uint32StartSector     = 0x%x\n", __FUNCTION__, cbNew, (self->ptrCbs[cbNew]).uint32StartSector);
    sfcb_printf("  INFO:%s:ptrCbs[%i].uint32StopSector      = 0x%x\n", __FUNCTION__, cbNew, (self->ptrCbs[cbNew]).uint32StopSector);
    sfcb_printf("  INFO:%s:ptrCbs[%i].uint16NumEntriesMax   = %d\n",   __FUNCTION__, cbNew, (self->ptrCbs[cbNew]).uint16NumEntriesMax);
    /* succesfull */
    return SFCB_OK;
}



/**
 *  sfcb_busy
 *    checks if #sfcb_worker is free for new requests
 */
int sfcb_busy (t_sfcb *self)
{
    if ( 0 != self->uint8Busy ) {
        return -1;  // busy
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
        sfcb_printf("  ERROR:%s: Worker is busy\n", __FUNCTION__);
        return SFCB_E_WKR_BSY;  // busy
    }
    /* check for at least one active queue */
    if ( 0 == ((self->ptrCbs)[0]).uint8Used ) {
        sfcb_printf("  ERROR:%s: Circular buffer queue not active or present\n", __FUNCTION__);
        return SFCB_E_NO_CB_Q;  // circular buffer queue not active
    }
    /* Find first queue which needs an build */
    self->uint8IterCb = 0;
    for ( uint8_t i = 0; i < self->uint8NumCbs; i++ ) {
        if ( (0 == ((self->ptrCbs)[i]).uint8Used) || (0 == ((self->ptrCbs)[i]).uint8MgmtValid) ) {
            break;
        }
        self->uint8IterCb = i;  // search for queue with uninitialized or invalid managment data, in case of only on circular buffer needes to rebuild
    }
    /* reset idmin/idmax counter to enable select of correct page to erase */
    for ( uint8_t i = self->uint8IterCb; i < (self->uint8NumCbs); i++ ) {
        /* not used, leave */
        if ( 0 == ((self->ptrCbs)[i]).uint8Used ) {
            break;
        }
        /* dirty buffer, needs to rebuild absoulte idmin/idmax for next write, otherwise will the last written element deleted */
        if ( 0 == ((self->ptrCbs)[i]).uint8MgmtValid ) {
            (self->ptrCbs[i]).uint32IdNumMax = 0;               // in case of uninitialized memory
            (self->ptrCbs[i]).uint32IdNumMin = __UINT32_MAX__;  // assign highest number
            (self->ptrCbs[i]).uint16PlFlashOfs = 0;             // reset payload offset counter
        }
    }
    /* Setup new Job */
    self->cmd = SFCB_CMD_MKCB;
    self->uint16Iter = 0;       // start with first element on lowest
    self->stage = SFCB_STG00;
    self->error = SFCB_E_NOERO;
    self->uint8Busy = 1;
    /* fine */
    return SFCB_OK;
}



/**
 *  sfcb_add
 *    inserts element into circular buffer in multiple writes
 *    in case of prematurely finish circular buffer element run #sfcb_add_done
 */
int sfcb_add (t_sfcb *self, uint8_t cbID, void *data, uint16_t len)
{
    /* Function call message */
    sfcb_printf("__FUNCTION__ = %s\n", __FUNCTION__);
    /* no jobs pending */
    if ( 0 != self->uint8Busy ) {
        sfcb_printf("  ERROR:%s: Worker is busy\n", __FUNCTION__);
        return SFCB_E_WKR_BSY;  // Worker is busy, wait for processing last job
    }
    /* check if requested circular buffer queue exist */
    if ( !(cbID < self->uint8NumCbs) ) {
        sfcb_printf("  ERROR:%s: Circular buffer queue not active or present\n", __FUNCTION__);
        return SFCB_E_NO_CB_Q;  // circular buffer queue not present
    }
    /* check if CB is init for request */
    if (    (0 == ((self->ptrCbs)[cbID]).uint8Used)
         || ( ((self->ptrCbs)[cbID]).uint16PlFlashOfs >= (((self->ptrCbs)[cbID]).uint16PlSize + sizeof(spi_flash_cb_elem_head)) )
    ) {
        sfcb_printf("  ERROR:%s: Circular Buffer is not prepared for request\n", __FUNCTION__);
        return SFCB_E_WKR_REQ;  // Circular Buffer is not prepared for adding new element, run #sfcb_worker
    }
    /* check for match into circular buffer size */
    if ( (len + ((self->ptrCbs)[cbID]).uint16PlFlashOfs) > (((self->ptrCbs)[cbID]).uint16NumPagesPerElem * SFCB_FLASH_TOPO_PAGE_SIZE) ) {
        sfcb_printf("  ERROR:%s: data segement is larger then reserved circular buffer space\n", __FUNCTION__);
        return SFCB_E_MEM;  // data segement is larger then reserved circular buffer space
    }
    /* store information for insertion */
    self->uint8IterCb = cbID;   // used as pointer to queue
    ((self->ptrCbs)[self->uint8IterCb]).uint8MgmtValid = 0; // mark queue as dirty, for next write run #sfcb_mkcb
    self->uint32IterAdr = ((self->ptrCbs)[self->uint8IterCb]).uint32StartPageWrite + ((self->ptrCbs)[self->uint8IterCb]).uint16PlFlashOfs;  // select page for write
    self->ptrCbElemPl = data;
    self->uint16CbElemPlSize = len;
    self->uint16Iter = 0;   // used as ptrCbElemPl written pointer
    /* Setup new Job */
    self->uint8Busy = 1;
    self->cmd = SFCB_CMD_ADD;
    self->stage = SFCB_STG00;
    self->error = SFCB_E_NOERO;
    /* fine */
    return 0;
}



/**
 *  sfcb_add_done
 *    force footer write into flash if at leat one payload byte is written
 *    and the nominal payload size isn't reached
 */
int sfcb_add_done (t_sfcb *self, uint8_t cbID)
{
    /* Function call message */
    sfcb_printf("__FUNCTION__ = %s\n", __FUNCTION__);
    /* Footer still written? */
    if ( ((self->ptrCbs)[self->uint8IterCb]).uint16PlFlashOfs > (((self->ptrCbs)[self->uint8IterCb]).uint16PlSize + sizeof(spi_flash_cb_elem_head)) ) {
        return SFCB_OK; // footer is still written, nothing to do
    }
    /* no jobs pending */
    if ( 0 != self->uint8Busy ) {
        sfcb_printf("  ERROR:%s: Worker is busy\n", __FUNCTION__);
        return SFCB_E_WKR_BSY;  // Worker is busy, wait for processing last job
    }
    if ( !(cbID < self->uint8NumCbs) ) {
        sfcb_printf("  ERROR:%s: Circular buffer queue not active or present\n", __FUNCTION__);
        return SFCB_E_NO_CB_Q;  // circular buffer queue not present
    }
    /* check if CB is init for request */
    if ( (0 != ((self->ptrCbs)[cbID]).uint8Used) && (0 != ((self->ptrCbs)[cbID]).uint8MgmtValid) ) {
        sfcb_printf("  ERROR:%s: Circular buffer queue existent, but no payload bytes are present\n", __FUNCTION__);
        return SFCB_E_CB_Q_MTY;  // Circular Buffer is not prepared for adding new element, run #sfcb_worker
    }
    /* store information for insertion */
    self->uint8IterCb = cbID;   // used as pointer to queue
    self->uint32IterAdr = ((self->ptrCbs)[self->uint8IterCb]).uint32StartPageWrite + ((self->ptrCbs)[self->uint8IterCb]).uint16PlFlashOfs;  // select page for write
    self->ptrCbElemPl = NULL;
    self->uint16CbElemPlSize = 0;
    self->uint16Iter = 0;   // used as ptrCbElemPl written pointer
    ((self->ptrCbs)[self->uint8IterCb]).uint16PlFlashOfs = (uint16_t) (((self->ptrCbs)[self->uint8IterCb]).uint16PlSize + sizeof(spi_flash_cb_elem_head));   // force condition for footer writer, #sfcb_worker
    /* Setup new Job */
    self->uint8Busy = 1;
    self->cmd = SFCB_CMD_ADD;
    self->stage = SFCB_STG00;
    self->error = SFCB_E_NOERO;
    /* fine */
    return 0;
}



/**
 *  sfcb_get_pl_wrcnt
 *    returns number of payload bytes written out to flash
 *    these bytes are fixed up to an erase
 */
uint16_t sfcb_get_pl_wrcnt (t_sfcb *self, uint8_t cbID)
{
    if ( 0 == ((self->ptrCbs)[cbID]).uint16PlFlashOfs ) // exception in case of no write is done before, counter is equal zero
        return 0;
    return (uint16_t) (((self->ptrCbs)[cbID]).uint16PlFlashOfs - sizeof(spi_flash_cb_elem_head));   // header is not part of the payload data
}



/**
 *  sfcb_get_last
 *    get last written element from circular buffer
 */
int sfcb_get_last (t_sfcb *self, uint8_t cbID, void *data, uint16_t len, uint32_t *elemID)
{
    /* default */
    *elemID = 0;
    /* no jobs pending */
    if ( 0 != self->uint8Busy ) {
        sfcb_printf("  ERROR:%s: Worker is busy\n", __FUNCTION__);
        return SFCB_E_WKR_BSY;  // Worker is busy, wait for processing last job
    }
    /* check if CB queue is available */
    if ( !(cbID < self->uint8NumCbs) ) {
        sfcb_printf("  ERROR:%s: Circular buffer queue not active or present\n", __FUNCTION__);
        return SFCB_E_NO_CB_Q;  // circular buffer queue not present
    }
    /* check if CB is init for request */
    if ( (0 == ((self->ptrCbs)[cbID]).uint8Used) || (0 == ((self->ptrCbs)[cbID]).uint8MgmtValid) ) {
        sfcb_printf("  ERROR:%s: Circular Buffer is not prepared for request\n", __FUNCTION__);
        return SFCB_E_WKR_REQ;  // Circular Buffer is not prepared for reading element, run #sfcb_worker
    }
    /* check if at least one entry is present for getting last element */
    if ( 0 == ((self->ptrCbs)[cbID]).uint16NumEntries ) {
        sfcb_printf("  ERROR:%s: Cirular buffer queue has no valid entries\n", __FUNCTION__);
        return SFCB_E_CB_Q_MTY;
    }
    /* limit to size of last circular buffer element */
    if ( (len + sizeof(spi_flash_cb_elem_head)) > ((self->ptrCbs[cbID]).uint16NumPagesPerElem * SFCB_FLASH_TOPO_PAGE_SIZE) ) {
        len = (uint16_t) (((self->ptrCbs[cbID]).uint16NumPagesPerElem * (uint16_t) SFCB_FLASH_TOPO_PAGE_SIZE) - (uint16_t) sizeof(spi_flash_cb_elem_head));
    }
    /* Debug message */
    sfcb_printf (  "  INFO:%s: read from flash adr=%x\n",
                   __FUNCTION__,
                   ((self->ptrCbs)[cbID]).uint32StartPageIdMax
                );
    /* prepare job */
    self->ptrCbElemPl = data;
    self->uint16CbElemPlSize = len; // read number of requested bytes, but limited to last element size
    self->uint32IterAdr = (uint32_t) (((self->ptrCbs)[cbID]).uint32StartPageIdMax + sizeof(spi_flash_cb_elem_head));    // Start address of last written element, newest circular buffer entry, header is not part of payload
    self->uint16Iter = 0;   // used as ptrCbElemPl written pointer
    /* Setup new Job */
    self->uint8Busy = 1;
    self->cmd = SFCB_CMD_GET;   // read last element in queue from flash
    self->stage = SFCB_STG00;
    self->error = SFCB_E_NOERO;
    /* return element ID of read queue element */
    *elemID = (self->ptrCbs[cbID]).uint32ElemIdLastCpl;
    /* fine */
    return SFCB_OK;
}



/**
 *  sfcb_flash_read
 *    reads raw binary data from flash
 */
int sfcb_flash_read (t_sfcb *self, uint32_t adr, void *data, uint16_t len)
{
    /* no jobs pending */
    if ( 0 != self->uint8Busy ) {
        return SFCB_E_WKR_BSY;  // Worker is busy, wait for processing last job
    }
    /* prepare job */
    self->ptrCbElemPl = data;
    self->uint16CbElemPlSize = len;
    self->uint32IterAdr = adr;  // Flash RAW address
    /* Setup new Job */
    self->uint8Busy = 1;
    self->cmd = SFCB_CMD_RAW;   // RAW read from Flash
    self->stage = SFCB_STG00;
    self->error = SFCB_E_NOERO;
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



/**
 *  sfcb_isero
 *    error happend in last interaction
 */
int sfcb_isero (t_sfcb *self)
{
    /* error occured? */
    if ( SFCB_E_NOERO == self->error ) {
        return 0;   // no error
    }
    return -1;  // error
}
