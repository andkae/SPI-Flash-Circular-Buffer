/***********************************************************************
 @copyright     : Siemens AG
 @license       : Siemens Inner Source License 1.4
 @author        : Andreas Kaeberlein
 @address       : Clemens-Winkler-Strasse 3, 09116 Chemnitz

 @maintainer    : Andreas Kaeberlein
 @telephone     : +49 371 4810-2108
 @email         : andreas.kaeberlein@siemens.com

 @file          : sfcb_flash_types.h
 @date          : July 22, 2022

 @brief         : Flash Type Defintion
                  Defines Flash Types, instructions and topology
***********************************************************************/



//--------------------------------------------------------------
// Define Guard
//--------------------------------------------------------------
#ifndef __SFCB_FLASH_TYPES_H
#define __SFCB_FLASH_TYPES_H


/**
 * @defgroup SFCB_FLASH_TYPES
 *
 * Flash Type Definition. Selection via compile switch
 *
 * @{
 */
#if defined(W25Q16JV)
    /* @brief W25Q16JV
    *
    *  Winbond SPI Flash W25Q16JV, 2MByte SPI Flash
    *
    *  @see https://www.winbond.com/resource-files/w25q16jv%20spi%20revh%2004082019%20plus.pdf
    *
    */
    #define SFCB_FLASH_NAME                 "W25Q16JV"  /**<  Flash name                                                                                            */
    #define SFCB_FLASH_ID_HEX               "ef14"      /**<  HexID as asccii-hex                   W25Q16JV_Rev_H: p.19, Manufacturer and Device Identification    */
    #define SFCB_FLASH_IST_RDID             0x90        /**<  Instruction Read ID                   W25Q16JV_Rev_H: p.44, Read Manufacturer / Device ID (90h)       */
    #define SFCB_FLASH_IST_WR_ENA           0x06        /**<  Instruction Write enable              W25Q16JV_Rev_H: p.22, Write Enable (06h)                        */
    #define SFCB_FLASH_IST_WR_DSBL          0x04        /**<  Instruction Write disable             W25Q16JV_Rev_H: p.23, Write Disable (04h)                       */
    #define SFCB_FLASH_IST_ERASE_BULK       0xc7        /**<  Instruction Chip Erase                W25Q16JV_Rev_H: p.38, Chip Erase (C7h / 60h)                    */
    #define SFCB_FLASH_IST_ERASE_SECTOR     0x20        /**<  Instruction Sector Erase              W25Q16JV_Rev_H: p.35, Sector Erase (20h)                        */
    #define SFCB_FLASH_IST_RD_STATE_REG     0x05        /**<  Instruction Read Status Register      W25Q16JV_Rev_H: p.23, Read Status Register-1 (05h)              */
    #define SFCB_FLASH_IST_RD_DATA          0x03        /**<  Instruction Read Data                 W25Q16JV_Rev_H: p.26, Read Data, Single SPI Mode (03h)          */
    #define SFCB_FLASH_IST_WR_PAGE          0x02        /**<  Instruction Write Page                W25Q16JV_Rev_H: p.33, Page Program (02h)                        */
    #define SFCB_FLASH_TOPO_ADR_BYTE        3           /**<  Topology Number address bytes         W25Q16JV_Rev_H: p.26, Read Data                                 */
    #define SFCB_FLASH_TOPO_SECTOR_SIZE     4096        /**<  Topology Sector Size in bytes         W25Q16JV_Rev_H: p.35, Sector Erase (20h)
                                                                #SFCB_FLASH_IST_ERASE_SECTOR                                                                        */
    #define SFCB_FLASH_TOPO_PAGE_SIZE       256         /**<  Topology Page Size in bytes           W25Q16JV_Rev_H: p.33, Page Program (02h)
                                                                #SFCB_FLASH_IST_WR_PAGE                                                                             */
    #define SFCB_FLASH_TOPO_FLASH_SIZE      2097152     /**<  Topology Total flash size in bytes    W25Q16JV_Rev_H: p.71, ORDERING INFORMATION                      */
    #define SFCB_FLASH_TOPO_RDID_DUMMY      3           /**<  Topology Number of dummy bytes        W25Q16JV_Rev_H: p.44, Read Manufacturer / Device ID (90h)
                                                                #SFCB_FLASH_IST_RDID                                                                                */
    #define SFCB_FLASH_MNG_WIP_MSK          0x01        /**<  MGMT: write-in-progress               W25Q16JV_Rev_H: p.11, Erase/Write In Progress (BUSY) - RO       */
    #define SFCB_FLASH_MNG_WRENA_MSK        0x02        /**<  MGMT: write enable                    W25Q16JV_Rev_H: p.11, Write Enable Latch (WEL) - RO             */

#elif defined(NEWFLASH)
    /* @brief NEWFLASH
    *
    *  Add Here
    *
    *  @see link to datasheet
    *
    */

#else
    /* @brief Empty
    *
    *  Protection Entry if no compile option is set
    *  use as compile flags '-D<FLASHTYP>'
    *
    */
    #define SFCB_FLASH_NAME                 ""      /**<  Flash name                                                    */
    #define SFCB_FLASH_ID_HEX               ""      /**<  HexID as asccii-hex                                           */
    #define SFCB_FLASH_IST_RDID             0x0     /**<  Instruction Read ID                                           */
    #define SFCB_FLASH_IST_WR_ENA           0x0     /**<  Instruction Write enable                                      */
    #define SFCB_FLASH_IST_WR_DSBL          0x0     /**<  Instruction Write disable                                     */
    #define SFCB_FLASH_IST_ERASE_BULK       0x0     /**<  Instruction Chip Erase                                        */
    #define SFCB_FLASH_IST_ERASE_SECTOR     0x0     /**<  Instruction Sector Erase                                      */
    #define SFCB_FLASH_IST_RD_STATE_REG     0x0     /**<  Instruction Read Status Register                              */
    #define SFCB_FLASH_IST_RD_DATA          0x0     /**<  Instruction Read Data                                         */
    #define SFCB_FLASH_IST_WR_PAGE          0x0     /**<  Instruction Write Page                                        */
    #define SFCB_FLASH_TOPO_ADR_BYTE        0       /**<  Topology Number address bytes                                 */
    #define SFCB_FLASH_TOPO_SECTOR_SIZE     0       /**<  Topology Sector Size in bytes, #SFCB_FLASH_IST_ERASE_SECTOR   */
    #define SFCB_FLASH_TOPO_PAGE_SIZE       0       /**<  Topology Page Size in bytes, #SFCB_FLASH_IST_WR_PAGE          */
    #define SFCB_FLASH_TOPO_FLASH_SIZE      0       /**<  Topology Total flash size in bytes                            */
    #define SFCB_FLASH_TOPO_RDID_DUMMY      0       /**<  Topology Number of dummy bytes, #SFCB_FLASH_IST_RDID          */
    #define SFCB_FLASH_MNG_WIP_MSK          0x0     /**<  MGMT: write-in-progress                                       */
    #define SFCB_FLASH_MNG_WRENA_MSK        0x0     /**<  MGMT: write enable                                            */

#endif
/** @} */



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
    uint8_t     uint8FlashTopoAdrBytes;         /**<  Flash Topo: Number of address bytes               */
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
    {"W25Q16JV"},   // charFlashName[15]                Micron_MT25QU128, p. 1
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


#endif // __SFCB_FLASH_TYPES_H
