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
int spi_flash_cb_add (spi_flash_cb *self, uint32_t magicNum, uint16_t elemSizeByte, uint16_t numElems)
{	
	/** help variables **/
	const uint8_t		uint8NumHeadBytes = 2*sizeof(((spi_flash_cb_elem *)0)->uint32MagicNum) + sizeof(((spi_flash_cb_elem *)0)->uint32HighSeriesNum);
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
	cb_elem[uint8FreeCbEntry].uint8Used = 1;	// occupied
	cb_elem[uint8FreeCbEntry].uint16NumPagesPerElem = spi_flash_cb_ceildivide(elemSizeByte+uint8NumHeadBytes, SPI_FLASH_CB_TYPES[self->uint8FlashType].uint32FlashTopoPageSizeByte);	// calculate in multiple of pages
	cb_elem[uint8FreeCbEntry].uint32StartSector = uint32StartSector;
	uint32NumSectors = spi_flash_cb_max(2, spi_flash_cb_ceildivide(numElems*cb_elem[uint8FreeCbEntry].uint16NumPagesPerElem, SPI_FLASH_CB_TYPES[self->uint8FlashType].uint32FlashTopoSectorSizeByte));
	cb_elem[uint8FreeCbEntry].uint32StopSector = cb_elem[uint8FreeCbEntry].uint32StartSector+uint32NumSectors-1;
	/* succesfull */
	return 0;
}




