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


/**
 * @defgroup SFCB_CFG driver configuration
 *
 * sinu diag character interface register and offset defintion
 *
 * @{
 */
#ifndef SFCB_SPI_BUF
	#define SFCB_SPI_BUF	266		/**<  SPI buffer between logical, and HW layer; Page Size + some bytes for instruction  */
#endif
/** @} */


/**
 * @defgroup SFCB_CMD
 *
 * States for Worker
 *
 * @{
 */
#define SFCB_CMD_IDLE	(0x00)      /**<  Nothing to do */
#define SFCB_CMD_MKCB	(0x01)      /**<  Make Circular Buffers */
#define SFCB_CMD_ADD	(0x02)      /**<  Add Element into Circular Buffer */
/** @} */


/**
 * @defgroup SFCB_STG
 *
 * Execution Stages
 *
 * @{
 */
#define SFCB_STG_00	(0x00)      /**<  Stage 0 */
#define SFCB_STG_01	(0x01)      /**<  Stage 1 */
#define SFCB_STG_02	(0x02)      /**<  Stage 2 */
/** @} */


/**
 * @defgroup SFCB_ERO
 *
 * Execution Stages
 *
 * @{
 */
#define SFCB_ERO_NO	(0x00)      /**<  No Error */
/** @} */





/* C++ compatibility */
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus


typedef struct spi_flash_cb_elem_head
{
    uint32_t	uint32MagicNum;	/**< Magic Number for marking valid block */
    uint32_t	uint32IdNum;	/**< Series Number of ID */
} __attribute__((packed)) spi_flash_cb_elem_head;



typedef struct spi_flash_cb_elem
{
    uint8_t		uint8Used;					/**< Entry is used */
    uint8_t		uint8Init;					/**< Data structure initialized */
    uint32_t	uint32MagicNum;				/**< Magic Number for marking valid block */
    uint32_t	uint32IdNumMax;				/**< Contents highest elment number in logical circular buffer. Circular buffer elements are ascending numbered starting at zero */
    uint32_t	uint32IdNumMin;				/**< lowest element number in circular buffer */
    uint32_t	uint32StartSector;			/**< Start Sector of the Circular Buffer */
    uint32_t	uint32StopSector;			/**< Stop Sector. At least two sectors are required, otherwise will sector erase delete complete buffer */
    uint32_t	uint32StartPageWrite;		/**< Start Page number of next entry */
    uint32_t	uint32StartPageIdMin;		/**< Start page of Circular buffer entry with lowest number, used for sector erase */
    uint16_t	uint16NumPagesPerElem;		/**< Number of pages per element */
    uint16_t	uint16NumEntries;			/**< Number of entries in circular buffer */
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
    uint8_t		uint8FlashType;			/**< pointer to flashtype. see #t_spi_flash_cb_type_descr */
    uint8_t		uint8NumCbs;			/**< number of circular buffers */
    void*		ptrCbs;					/**< List with flash circular buffers */
    uint8_t		uint8Spi[SFCB_SPI_BUF];	/**< transceive buffer between SPI/CB layer */
    uint16_t	uint16SpiLen;			/**< used buffer length */
    uint8_t		uint8Busy;				/**< Performing splitted interaction of circular buffers */
    uint8_t		uint8Cmd;				/**< Command to be executed */
    uint8_t		uint8IterCb;			/**< Iterator for splitted interaction, iterator over Circular buffers */
    uint16_t	uint16IterElem;			/**< Iterator for splitted interaction, iteartor over elements in circular buffer */
    uint32_t	uint32IterPage;			/**< Page Iterator, f. e. captures last header page, next page write */
    uint8_t		uint8Stg;				/**< Execution stage, from last interaction */
    uint8_t		uint8Error;				/**< Error code if somehting strange happend */
    void*		ptrCbElemPl;			/**< Pointer to Payload data of CB Element */
    uint16_t	uint16CbElemPlSize;		/**< Size of payload data in bytes */
    
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
 *  @param[in,out]  *cbID            	Circular buffer number
 *  @return         int                 state
 *  @retval         0                   OKAY
 *  @retval         1                   No free circular buffer slots, allocate more static memory
 *  @since          2022-07-25
 *  @author         Andreas Kaeberlein
 */
int spi_flash_cb_add (spi_flash_cb *self, uint32_t magicNum, uint16_t elemSizeByte, uint16_t numElems, uint8_t *cbID);



/**
 *  @brief worker
 *
 *  Services Circular buffer layer, request/processes SPI packets.
 *  Executes request from #sfcb_mkcb, 
 *
 *  @param[in,out]  self                handle
 *  @return         void                state
 *  @since          2022-07-27
 *  @author         Andreas Kaeberlein
 */
void sfcb_worker (spi_flash_cb *self);



/**
 *  @brief build-up
 *
 *  Reads from Flash and builds-up queues with circular buffer structure
 *
 *  @param[in,out]  self                handle
 *  @return         int                	state
 *  @retval         0                   Request accepted
 *  @retval         1                   Not Free for new Requests, wait
 *  @retval         2                   no active queue, add via #spi_flash_cb_add
 *  @since          2022-07-27
 *  @author         Andreas Kaeberlein
 */
int sfcb_mkcb (spi_flash_cb *self);



/**
 *  @brief add element
 *
 *  adds element to circular buffer structure
 *
 *  @param[in,out]  self                handle
 *  @param[in]      cbID            	Logical Number of Cicular Buffer queue
 *  @param[in]  	*data            	Pointer to data array which should add
 *  @param[in]  	len           		size of *data in bytes
 *  @return         int                	state
 *  @retval         0                   Request accepted.
 *  @retval         1                   Worker is busy, wait for processing last job.
 *  @retval         2                   Circular Buffer is not prepared for adding new element, run #sfcb_worker.
 * 	@retval         4                   Data segement is larger then reserved circular buffer space.
 *  @since          2022-07-28
 *  @author         Andreas Kaeberlein
 */
int sfcb_add (spi_flash_cb *self, uint8_t cbID, void *data, uint16_t len);





#ifdef __cplusplus
}
#endif // __cplusplus


#endif // __SPI_FLASH_CB_H
