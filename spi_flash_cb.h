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



/**
 *  @typedef t_sfcb_cmd
 *
 *  @brief  Command class
 *
 *  Current performed command class
 *
 *  @since  2022-12-08
 *  @author Andreas Kaeberlein
 */
typedef enum
{
    SFCB_CMD_IDLE,  /**<  Nothing to do */
    SFCB_CMD_MKCB,  /**<  Make Circular Buffers */
    SFCB_CMD_ADD,   /**<  Add Element into Circular Buffer */
    SFCB_CMD_GET,   /**<  Get Data from Element of Circular buffer, there is no pop from the stack after get */
    SFCB_CMD_RAW    /**<  Read Raw Data from flash */
} t_sfcb_cmd;



/**
 *  @typedef t_sfcb_stage
 *
 *  @brief  stage of command
 *
 *  Every command is divided into stages of exectuion.
 *  The execution starts always on stage 0. If an hardware
 *  interaction is required then the stage ends and the
 *  data packet will sent to hardware. This makes the
 *  driver interruptible and frees CPU time.
 *
 *  @since  2022-12-08
 *  @author Andreas Kaeberlein
 */
typedef enum
{
    SFCB_STG00, /**<  Stage 0, different meanings based on executed command */
    SFCB_STG01, /**<  Stage 1, different meanings based on executed command */
    SFCB_STG02, /**<  Stage 2, different meanings based on executed command */
    SFCB_STG03  /**<  Stage 3, different meanings based on executed command */
} t_sfcb_stage;



/**
 *  @typedef t_sfcb_error
 *
 *  @brief  Errors
 *
 *  Ocured Erros while driver execution
 *
 *  @since  2022-12-08
 *  @author Andreas Kaeberlein
 */
typedef enum
{
    SFCB_E_NOERO,   /**<  No Error occured */
    SFCB_E_BUFSIZE  /**<  Buffer too small for operation */
} t_sfcb_error;



/**
 *  @typedef spi_flash_cb_elem_head
 *
 *  @brief  circular buffer element head
 *
 *  Header to mark the start of an circular buffer element
 *  takes place in SPI Flash
 *
 *  @since  2022-07-22
 *  @author Andreas Kaeberlein
 */
typedef struct spi_flash_cb_elem_head
{
    uint32_t    uint32MagicNum; /**< Magic Number for marking valid block */
    uint32_t    uint32IdNum;    /**< Series Number of ID */
} __attribute__((packed)) spi_flash_cb_elem_head;



/**
 *  @typedef t_sfcb_cb
 *
 *  @brief  queue management
 *
 *  Cicrcular buffer queue managament entry.
 *  This structure is used in an array and for every
 *  circular buffer queue is such an entry mandatory
 *
 *  @since  2022-07-22
 *  @author Andreas Kaeberlein
 */
typedef struct t_sfcb_cb
{
    uint8_t     uint8Used;                  /**< Managment slot in #t_sfcb_cb table is used, try next index */
    uint8_t     uint8Init;                  /**< Entry in management table is valid/invalid. In case of invalid run #sfcb_worker to update the management info in this entry */
    uint32_t    uint32MagicNum;             /**< Magic Number for marking valid block */
    uint32_t    uint32IdNumMax;             /**< Contents highest elment number in logical circular buffer. Circular buffer elements are ascending numbered starting at zero */
    uint32_t    uint32IdNumMin;             /**< lowest element number in circular buffer */
    uint32_t    uint32StartSector;          /**< Start Sector of the Circular Buffer */
    uint32_t    uint32StopSector;           /**< Stop Sector. At least two sectors are required, otherwise will sector erase delete complete buffer */
    uint32_t    uint32StartPageWrite;       /**< Start Page number of next entry */
    uint32_t    uint32StartPageIdMin;       /**< Start page of Circular buffer entry with lowest number, used for sector erase */
    uint16_t    uint16NumPagesPerElem;      /**< Number of pages per element */
    uint16_t    uint16NumEntriesMax;        /**< Maximal Number of entries in circular buffer caused by partition table */
    uint16_t    uint16NumEntries;           /**< Number of entries in circular buffer */
} t_sfcb_cb;



/**
 *  @typedef t_sfcb
 *
 *  @brief  circular buffer
 *
 *  handle for circular buffer
 *
 *  @since  July 22, 2022
 *  @author Andreas Kaeberlein
 */
typedef struct t_sfcb
{
    uint8_t         uint8FlashPresent;      /**< checks if selected flashtype is available */
    uint8_t         uint8NumCbs;            /**< number of circular buffers */
    t_sfcb_cb*      ptrCbs;                 /**< List with flash circular buffer management info, #t_sfcb_cb */
    uint8_t*        uint8PtrSpi;            /**< SPI/CB layer interaction buffer */
    uint16_t        uint16SpiLen;           /**< used buffer length */
    uint16_t        uint16SpiMax;           /**< maximum spi buffer length */
    uint8_t         uint8Busy;              /**< Performing splitted interaction of circular buffers */
    t_sfcb_cmd      cmd;                    /**< Command to be executed, #t_sfcb_cmd */
    uint8_t         uint8IterCb;            /**< Iterator for splitted interaction, iterator over Circular buffers */
    uint16_t        uint16IterElem;         /**< Iterator for splitted interaction, iteartor over elements in circular buffer */
    uint32_t        uint32IterPage;         /**< Page Iterator. Contents full byte address but in multiple of pages. F. e. captures last header page, next page write */
    t_sfcb_stage    stage;                  /**< Execution stage, from last interaction, #t_sfcb_stage */
    t_sfcb_error    error;                  /**< Error code if somehting strange happend, #t_sfcb_error */
    void*           ptrCbElemPl;            /**< Pointer to Payload data of CB Element */
    uint16_t        uint16DataLen;          /**< data length */
    uint16_t        uint16CbElemPlSize;     /**< Size of payload data in bytes */

} t_sfcb;



/**
 *  @brief init
 *
 *  initializes flash circular buffer
 *
 *  @param[in,out]  self                handle
 *  @param[in,out]  *cb                 pointer to allocated memory for circulat buffer list, see #t_sfcb
 *  @param[in]      cbLen               number of maximum allowed circular buffers
 *  @param[in,out]  *spi                pointer to uint8_t spi interaction buffer, buffer betwween spi core and flash driver
 *  @param[in]      spiLen              maimum number of elements in buffer => size in byte
 *  @return         int                 state
 *  @retval         0                   OKAY
 *  @retval         1                   Invalid Flash Type
 *  @since          2022-07-25
 *  @author         Andreas Kaeberlein
 */
int sfcb_init (t_sfcb *self, void *cb, uint8_t cbLen, void *spi, uint16_t spiLen);



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
void sfcb_worker (t_sfcb *self);



/**
 *  @brief flash size
 *
 *  Total Flash Size in bytes
 *
 *  @return         uint32_t            flash size in byte
 *  @since          2023-01-04
 *  @author         Andreas Kaeberlein
 */
uint32_t sfcb_flash_size (void);



/**
 *  @brief new_cb
 *
 *  creates new circular buffer entry in flash parition table
 *
 *  @param[in,out]  self                handle
 *  @param[in]      magicNum            Magic Number for marking enties valid, should differ between different Circular buffer entries
 *  @param[in]      elemSizeByte        Size of one element in the circular buffer in byte
 *  @param[in]      numElems            minimal number of elements in the circular buffer, through need of sector erase and not deleting all data can be this number higher then requested
 *  @param[in,out]  *cbID               Circular buffer number
 *  @return         int                 state
 *  @retval         0                   OKAY
 *  @retval         1                   No free circular buffer slots, allocate more static memory
 *  @since          2022-07-25
 *  @author         Andreas Kaeberlein
 */
int sfcb_new_cb (t_sfcb *self, uint32_t magicNum, uint16_t elemSizeByte, uint16_t numElems, uint8_t *cbID);



/**
 *  @brief busy
 *
 *  checks if #sfcb_worker is free for new requests
 *
 *  @param[in,out]  self                handle
 *  @return         int                 state
 *  @retval         0                   idle
 *  @retval         1                   busy
 *  @since          2022-07-29
 *  @author         Andreas Kaeberlein
 */
int sfcb_busy (t_sfcb *self);



/**
 *  @brief Spi Length
 *
 *  gets length of next spi packet, created by #sfcb_worker
 *
 *  @param[in,out]  self                handle
 *  @return         uint16_t            current length of SPI packet, created by #sfcb_worker
 *  @since          2022-08-19
 *  @author         Andreas Kaeberlein
 */
uint16_t sfcb_spi_len (t_sfcb *self);



/**
 *  @brief build-up
 *
 *  Reads from Flash and builds-up queues with circular buffer structure
 *
 *  @param[in,out]  self                handle
 *  @return         int                 state
 *  @retval         0                   Request accepted
 *  @retval         1                   Not Free for new Requests, wait
 *  @retval         2                   no active queue, add via #spi_flash_cb_add
 *  @since          2022-07-27
 *  @author         Andreas Kaeberlein
 */
int sfcb_mkcb (t_sfcb *self);



/**
 *  @brief add element
 *
 *  adds element to circular buffer structure
 *
 *  @param[in,out]  self                handle
 *  @param[in]      cbID                Logical Number of Cicular Buffer queue
 *  @param[in]      *data               Pointer to data array which should add
 *  @param[in]      len                 size of *data in bytes
 *  @return         int                 state
 *  @retval         0                   Request accepted.
 *  @retval         1                   Worker is busy, wait for processing last job.
 *  @retval         2                   Circular Buffer is not prepared for adding new element, run #sfcb_worker.
 *  @retval         4                   Data segement is larger then reserved circular buffer space.
 *  @since          2022-07-28
 *  @author         Andreas Kaeberlein
 */
int sfcb_add (t_sfcb *self, uint8_t cbID, void *data, uint16_t len);



/**
 *  @brief RAW Read
 *
 *  read raw data from flash
 *
 *  @param[in,out]  self                handle
 *  @param[in]      adr                 start address for read
 *  @param[in,out]  *data               pointer to data array with read data
 *  @param[in]      len                 size of *data in bytes
 *  @return         int                 state
 *  @retval         0                   Request accepted.
 *  @retval         1                   Worker is busy, wait for processing last job.
 *  @since          2023-01-05
 *  @author         Andreas Kaeberlein
 */
int sfcb_flash_read (t_sfcb *self, uint32_t adr, void *data, uint16_t len);



/**
 *  @brief idmax
 *
 *  get maximum id in selected circular buffer queue
 *
 *  @param[in,out]  self                handle
 *  @param[in]      cbID                Logical Number of Cicular Buffer queue
 *  @return         uint32_t            highest id number of selected circular buffer queue
 *  @since          2023-01-01
 *  @author         Andreas Kaeberlein
 */
uint32_t sfcb_idmax (t_sfcb *self, uint8_t cbID);



#ifdef __cplusplus
}
#endif // __cplusplus


#endif // __SPI_FLASH_CB_H
