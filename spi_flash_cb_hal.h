/***********************************************************************
 @copyright     : Siemens AG
 @license       : Siemens Inner Source License 1.4
 @author        : Andreas Kaeberlein
 @address       : Clemens-Winkler-Strasse 3, 09116 Chemnitz

 @maintainer    : Andreas Kaeberlein
 @telephone     : +49 371 4810-2108
 @email         : andreas.kaeberlein@siemens.com

 @file          : spi_flash_cb_hal.h
 @date          : July 22, 2022

 @brief         : Flash Type Defintion
                  Defines Flash Type Instruction Set
***********************************************************************/



//--------------------------------------------------------------
// Define Guard
//--------------------------------------------------------------
#ifndef __SPI_FLASH_CB_HAL_H
#define __SPI_FLASH_CB_HAL_H


/**
 *  @defgroup SPI Backends
 *
 *  spi backend selection
 *
 *  @{
 */
#ifdef NEORV32
    #define spi_flash_cb_spi_rw(...) neorv32_spi_rw(__VA_ARGS__)
#else
    #define spi_flash_cb_spi_rw(...)
#endif
/** @} */   // SPI


/* C++ compatibility */
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus


/**
 *  @typedef t_spi_flash_cb_type_descr
 *
 *  @brief  Supported Flash description
 *
 *  store info for handling flash types
 *
 *  @since  July 22, 2022
 *  @author Andreas Kaeberlein
 */
typedef struct t_spi_flash_cb_type_descr {
    char        charFlashName[15];              /**<  Flash: Name                                       */
    char        charFlashIdHex[20];             /**<  Flash: ID in Ascii-Hex                            */
    uint8_t     uint8FlashIstRdID;              /**<  Flash IST: Read ID                                */
    uint8_t     uint8FlashIstWrEnable;          /**<  Flash IST: write enable                           */
    uint8_t     uint8FlashIstWrDisable;         /**<  Flash IST: write disable                          */
    uint8_t     uint8FlashIstEraseBulk;         /**<  Flash IST: Bulk erase                             */
    uint8_t     uint8FlashIstEraseSector;       /**<  Flash IST: erase smallest possible sector         */
    uint8_t     uint8FlashIstRdStateReg;        /**<  Flash IST: Read status reg                        */
    uint8_t     uint8FlashIstRdData;            /**<  Flash IST: Read data from flash                   */
    uint8_t     uint8FlashIstWrPage;            /**<  Flash IST: Write page                             */
    uint8_t     uint8FlashTopoAdrBytes;         /**<  Flash Topo: Read data from flash                  */
    uint32_t    uint32FlashTopoSectorSizeByte;  /**<  Flash Topo: Flash Sector Size in Byte             */
    uint32_t    uint32FlashTopoPageSizeByte;    /**<  Flash Topo: Flash Page Size in Byte               */
    uint32_t    uint32FlashTopoTotalSizeByte;   /**<  Flash Topo: Total Flash Size in Byte              */
    uint8_t     uint8FlashTopoRdIdDummyByte;    /**<  Flash Topo: Number of Dummy bytes after RD ID IST */
    uint8_t     uint8FlashMngWipMsk;            /**<  Flash MNG: Write-in-progress                      */
    uint8_t     uint8FlashMngWrEnaMsk;          /**<  Flash MNG: Write enable latch, 1: set, 0: clear   */

} __attribute__((packed)) t_spi_flash_cb_type_descr;



/** Look up table **/
/* Supported Flash Types */
const struct t_spi_flash_cb_type_descr SPI_FLASH_CB_TYPES[] = {
  /* W25Q16JV */
  {
    {"W25Q16JV"},	// charFlashName[15]                Micron_MT25QU128, p. 1
    {"20bb18"},     // charFlashIdHex                   Micron_MT25QU128, p. 38
    0x9e,           // uint8FlashIstRdID                Micron_MT25QU128, p. 46
    0x06,           // uint8FlashIstWrEnable            Micron_MT25QU128, p. 55
    0x04,           // uint8FlashIstWrDisable           Micron_MT25QU128, p. 55
    0xc7,           // uint8FlashIstEraseBulk           Micron_MT25QU128, p. 64
    0x20,           // uint8FlashIstEraseSector         Micron_MT25QU128, p. 64
    0x05,           // uint8FlashIstRdStateReg          Micron_MT25QU128, p. 40/26
    0x03,           // uint8FlashIstRdData              Micron_MT25QU128, p. 48
    0x02,           // uint8FlashIstWrPage              Micron_MT25QU128, p. 60
    3,              // uint8FlashTopoAdrBytes           Micron_MT25QU128, p. 40
    4096,           // uint32FlashTopoSectorSizeByte    Micron_MT25QU128, p. 41,        4KB SUBSECTOR ERASE
    256,            // uint32FlashTopoPageSizeByte      Micron_MT25QU128, p. 9
    2097152,        // uint32FlashTopoTotalSizeByte     Micron_MT25QU128, p. 1
    0,              // uint8FlashTopoRdIdDummyByte
    0x01,           // uint8FlashMngWipMsk              Micron_MT25QU128, p. 26
    0x02,           // uint8FlashMngWrEnaMsk            Micron_MT25QU128, p. 26
  },

  /* ...  */

  /* NULL */
  {
    {""},   // charFlashName
    {""},   // charFlashIdHex
    0,      // uint8FlashIstRdID
    0,      // uint8FlashIstWrEnable
    0,      // uint8FlashIstWrDisable
    0,      // uint8FlashIstEraseBulk
    0,      // uint8FlashIstEraseSector
    0,      // uint8FlashIstRdStateReg
    0,      // uint8FlashIstRdData
    0,      // uint8FlashIstWrPage
    0,      // uint8FlashTopoAdrBytes
    0,      // uint32FlashTopoSectorSizeByte
    0,      // uint32FlashTopoPageSizeByte
    0,      // uint32FlashTopoTotalSizeByte
    0,      // uint8FlashTopoRdIdDummyByte
    0,      // uint8FlashMngWipMsk
    0,      // uint8FlashMngWrEnaMsk
  }
};


#ifdef __cplusplus
}
#endif // __cplusplus


#endif // __SPI_FLASH_CB_HAL_H
