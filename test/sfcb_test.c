/***********************************************************************
 @copyright     : Siemens AG
 @license       : GPLv3
 @author        : Andreas Kaeberlein
 @address       : Clemens-Winkler-Strasse 3, 09116 Chemnitz

 @maintainer    : Andreas Kaeberlein
 @telephone     : +49 371 4810-2108
 @email         : andreas.kaeberlein@siemens.com

 @file          : sfcb_test.c
 @date          : 2022-12-13

 @brief         : SPI flash driver unit test

***********************************************************************/



/** Standard libs **/
#include <stdio.h>          // f.e. printf
#include <stdlib.h>         // defines four variables, several macros,
                            // and various functions for performing
                            // general functions
#include <stdint.h>         // defines fiexd data types, like int8_t...
#include <unistd.h>         // system call wrapper functions such as fork, pipe and I/O primitives (read, write, close, etc.).
#include <string.h>         // string handling functions
#include <stdarg.h>         // variable parameter lists
#include <fcntl.h>          // manipulate file descriptor
#include <ctype.h>          // used for testing and mapping characters

/** User Libs **/
#include "spi_flash_cb.h"



/**
 *  Main
 *  ----
 */
int print ()
{
	

}



/**
 *  Main
 *  ----
 */
int main ()
{
    /** Variables **/
	spi_flash_cb		sfcb;		// SPI Flash as circular buffer
	spi_flash_cb_elem	sfcb_cb[5];	// five logical parts in SPI Flash
	
	
	uint32_t            uint32Temp;
    uint32_t            i;                  // iterartor
    uint8_t             uint8Buf[10];
    uint32_t            uint32Buf8Len;
    uint8_t*            uint8PtrBuf16M;
    uint8_t*            uint8PtrBuf16M_2;
    int                 intFexit;           // functions exit code


	/* entry message */
	printf("INFO:%s: unit test started\n", __FUNCTION__);
	
	/* sfcb_init */
	printf("INFO:%s:sfcb_init\n", __FUNCTION__);
	printf("INFO:%s:sfcb_p       = %p\n", __FUNCTION__, &sfcb);
	printf("INFO:%s:sfcb_cb_p    = %p\n", __FUNCTION__, &sfcb_cb);
	printf("INFO:%s:sfcb_cb[0]_s = %i\n", __FUNCTION__, sizeof(sfcb_cb[0]));
	memset(sfcb_cb, 0xaf, sizeof(sfcb_cb));	// mess-up memory to check init
	/* int spi_flash_cb_init (spi_flash_cb *self, uint8_t flashType, void *cbMem, uint8_t numCbs) */
	sfcb_init (&sfcb, 0, &sfcb_cb, sizeof(sfcb_cb)/sizeof(sfcb_cb[0]));
	/* check for errror */
	for ( uint8_t i = 0; i < sizeof(sfcb_cb)/sizeof(sfcb_cb[0]); i++ ) {
		/* check flags */
		if ( 0 != (sfcb_cb[i]).uint8Used ) {
			printf("ERROR:%s:sfcb_init:uint8Used", __FUNCTION__);
			goto ERO_END;
		}
		if ( 0 != (sfcb_cb[i]).uint8Init) {
			printf("ERROR:%s:sfcb_init:uint8Init", __FUNCTION__);
			goto ERO_END;
		}
		/* raw dump */
		printf("  INFO:%s:sfcb_cb[%i]: ", __FUNCTION__, i);
		for ( uint8_t j = 0; j < sizeof(sfcb_cb[0]); j++ ) {
			printf("%02x", *(((uint8_t*) &sfcb_cb)+i*sizeof(sfcb_cb[0])+j));
		}
		printf("\n");
	}
	
	
	
	







//    /* allocate Mmemory */
//    uint8PtrBuf16M = (uint8_t*) malloc(uint32PtrBuf16MLen);
//    uint8PtrBuf16M_2 = (uint8_t*) malloc(uint32PtrBuf16MLen);
//    if ( (NULL == uint8PtrBuf16M) || (NULL == uint8PtrBuf16M_2) ) {
//        printf("ERROR:%s: memory allocation for test\n", __FUNCTION__);
//        goto ERO_END;
//    }
//
//
//    /* init handle */
//    spiflashdrv_init( &handle );
//
//
//    /* spiflashdrv_rdid_unique */
//    printf("INFO:%s:spiflashdrv_rdid_unique\n", __FUNCTION__);
//    if ( 0 != spiflashdrv_rdid_unique(&handle) ) {
//        printf("ERROR:%s:spiflashdrv_rdid_unique\n", __FUNCTION__);
//        goto ERO_END;
//    }
//    if ( 2 != handle.uint32UniqueRdIdLen) {
//        printf("ERROR:%s:spiflashdrv_rdid_unique, exp=2, is=%d\n", __FUNCTION__, handle.uint32UniqueRdIdLen);
//        goto ERO_END;
//    }
//    if ( 0x9e != handle.uint8PtrUniqueRdId[0] ) {
//        printf("ERROR:%s:spiflashdrv_rdid_unique, exp=0x9e, is=0x%02x\n", __FUNCTION__, handle.uint8PtrUniqueRdId[0]);
//        goto ERO_END;
//    }
//    if ( 0x9f != handle.uint8PtrUniqueRdId[1] ) {
//        printf("ERROR:%s:spiflashdrv_rdid_unique, exp=0x9f, is=0x%02x\n", __FUNCTION__, handle.uint8PtrUniqueRdId[1]);
//        goto ERO_END;
//    }
//


    /* gracefull end */
    printf("INFO:%s: Module test SUCCESSFUL :-)\n", __FUNCTION__);
    exit(EXIT_SUCCESS);

    /* abnormal end */
    ERO_END:
        printf("FAIL:%s: Module test FAILED :-(\n", __FUNCTION__);
        exit(EXIT_FAILURE);

}
