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


// Define Guard
#ifndef __SPI_FLASH_CB_H
#define __SPI_FLASH_CB_H


/* C++ compatibility */
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus


typedef struct spi_flash_cb_elem
{
    uint8_t		uint8Used : 1;				/**< Entry is used */
    uint8_t		uint8Init : 1;				/**< Data structure initialized */
    uint32_t	uint32MagicNum;				/**< Magic Number for marking valid block */
    uint32_t	uint32HighSeriesNum;		/**< Highest Number in series number field */
    uint32_t	uint32StartSector;			/**< Start Sector of the Circular Buffer */
    uint32_t	uint32StopSector;			/**< Stop Sector. At least two sectors are required, otherwise will sector erase delete complete buffer */
    uint32_t	uint32StartPageNewEntry;	/**< Start Page number of next entry */
    uint16_t	uint16NumPagesPerElem;		/**< Number of pages per element */
} __attribute__((packed)) spi_flash_cb_elem;



/**
 *  @typedef spi_flash_cb
 *
 *  @brief  circular buffer
 *
 *  handle for circular buffer
 *
 *  @since  July 22, 2022
 *  @author Andreas Kaeberlein
 */
typedef struct spi_flash_cb
{
    uint8_t		uint8FlashType;	/**< pointer to flashtype. see #t_spi_flash_cb_type_descr */
    uint8_t		uint8NumCbs;	/**< number of circular buffers */
    void*		ptrCbs;			/**< List with flash circular buffers */
} spi_flash_cb;



/**
 *  @brief init
 *
 *  initializes flash circular buffer
 *
 *  @param[in,out]  self                handle
 *  @param[in]      flashType           index of flash type in #SPI_FLASH_CB_TYPES
 *  @param[in,out]  *cbMem              pointer to allocated memory for circulat buffer list
 *  @param[in]      numCbs              number of maximum allowed circular buffers
 *  @return         int                 state
 *  @retval         0                   OKAY
 *  @retval         1                   Invalid Flash Type
 *  @since          2022-07-25
 *  @author         Andreas Kaeberlein
 */
int spi_flash_cb_init (spi_flash_cb *self, uint8_t flashType, void *cbMem, uint8_t numCbs);


/**
 *  @brief add
 *
 *  add entry to flash circular buffer parition table
 *
 *  @param[in,out]  self                handle
 *  @param[in]      magicNum            Magic Number for marking enties valid, should differ between different Circular buffer entries
 *  @param[in]      elemSizeByte        Size of one element in the circular buffer in byte
 *  @param[in]      numElems            minimal number of elements in the circular buffer, through need of sector erase and not deleting all data can be this number higher then requested
 *  @return         int                 state
 *  @retval         0                   OKAY
 *  @retval         1                   No free circular buffer slots, allocate more static memory
 *  @since          2022-07-25
 *  @author         Andreas Kaeberlein
 */
int spi_flash_cb_add (spi_flash_cb *self, uint32_t magicNum, uint16_t elemSizeByte, uint16_t numElems);






#ifdef __cplusplus
}
#endif // __cplusplus


#endif // __SPI_FLASH_CB_H
