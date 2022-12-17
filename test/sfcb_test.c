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
 *  prints raw data of SFCB
 */
int print_raw_sfcb_cb (void* sfcb_cb, uint8_t num_sfcb_cb )
{
	/* print to console */
	for ( uint8_t i = 0; i < num_sfcb_cb; i++ ) {
		printf("INFO:sfcb_cb[%i]: ", i);
		for ( uint8_t j = 0; j < sizeof(spi_flash_cb_elem); j++ ) {
			printf("%02x", *(((uint8_t*) ((spi_flash_cb_elem*) sfcb_cb))+i*sizeof(spi_flash_cb_elem)+j));
		}
		printf("\n");
	}
	return 0;
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
	uint8_t				uint8Temp;	// help variable
	

	/* entry message */
	printf("INFO:%s: unit test started\n", __FUNCTION__);
	
	/* sfcb_init */
	printf("INFO:%s:sfcb_init\n", __FUNCTION__);
	printf("INFO:%s:sfcb_p          = %p\n", 	__FUNCTION__, &sfcb);
	printf("INFO:%s:sfcb_cb_p       = %p\n", 	__FUNCTION__, &sfcb_cb);
	printf("INFO:%s:sfcb_cb[0]_size = 0x%x\n",	__FUNCTION__, (int) sizeof(sfcb_cb[0]));
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
	}
	/* raw dump */
	print_raw_sfcb_cb(&sfcb_cb, sizeof(sfcb_cb)/sizeof(sfcb_cb[0]));
	
	
	/* sfcb_new_cb 
	 *   adds two new circular buffers to the SPI Flash
	*/
	printf("INFO:%s:sfcb_new_cb\n", __FUNCTION__);
	sfcb_new_cb (&sfcb, 0x47114711, 244, 32, &uint8Temp);	// start-up counter with operation
	sfcb_new_cb (&sfcb, 0x08150815, 12280, 16, &uint8Temp);	// error data collection 12KiB







	print_raw_sfcb_cb(&sfcb_cb, sizeof(sfcb_cb)/sizeof(sfcb_cb[0]));


	/* sfcb_worker
	 *   services flash circular buffer layer, creates and evaluates SPI packets
	 */
	printf("INFO:%s:sfcb_worker\n", __FUNCTION__);
	sfcb_worker (&sfcb);
	


    /* gracefull end */
    printf("INFO:%s: Module test SUCCESSFUL :-)\n", __FUNCTION__);
    exit(EXIT_SUCCESS);

    /* abnormal end */
    ERO_END:
        printf("FAIL:%s: Module test FAILED :-(\n", __FUNCTION__);
        exit(EXIT_FAILURE);

}
