/***********************************************************************
 @copyright     : Siemens AG
 @license       : BSDv3
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

#endif // __SFCB_FLASH_TYPES_H
