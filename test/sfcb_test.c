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
#include "spi_flash_model/spi_flash_model.h"	// spi flash model
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
	const uint32_t		uint32SpiFlashCycleOut = 1000;		// abort calling SPI flash
	uint32_t			uint32Counter;						// counter
	t_sfm       		spiFlash;   						// SPI flash model
	int					sfm_state;							// SPI Flash model return state
	spi_flash_cb		sfcb;								// SPI Flash as circular buffer
	spi_flash_cb_elem	sfcb_cb[5];							// five logical parts in SPI Flash
	uint8_t				uint8Spi[266];						// SPI packet buffer
	uint8_t				uint8Temp;							// help variable
	uint8_t				uint8FlashData[] = {0,1,2,3,4,5};	// SPI test data
	

	/* entry message */
	printf("INFO:%s: unit test started\n", __FUNCTION__);
	
	
	/* init flash model */
    printf("INFO:%s: Init Flash model W25Q16JV\n", __FUNCTION__);
    if ( 0 != sfm_init( &spiFlash, "W25Q16JV" ) ) {
        printf("ERROR:%s:sfm_init\n", __FUNCTION__);
        goto ERO_END;
    }
	
	
	/* sfcb_init */
	printf("INFO:%s:sfcb_init\n", __FUNCTION__);
	printf("INFO:%s:uint8Spi_p      = %p\n", 	__FUNCTION__, &uint8Spi);
	printf("INFO:%s:sfcb_p          = %p\n", 	__FUNCTION__, &sfcb);
	printf("INFO:%s:sfcb_cb_p       = %p\n", 	__FUNCTION__, &sfcb_cb);
	printf("INFO:%s:sfcb_cb[0]_size = 0x%x\n",	__FUNCTION__, (int) sizeof(sfcb_cb[0]));
	memset(sfcb_cb, 0xaf, sizeof(sfcb_cb));	// mess-up memory to check init
	/* int sfcb_init (spi_flash_cb *self, void *cb, uint8_t cbLen, void *spi, uint16_t spiLen) */
	sfcb_init (&sfcb, &sfcb_cb, sizeof(sfcb_cb)/sizeof(sfcb_cb[0]), &uint8Spi, sizeof(uint8Spi)/sizeof(uint8Spi[0]));
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
	//print_raw_sfcb_cb(&sfcb_cb, sizeof(sfcb_cb)/sizeof(sfcb_cb[0]));	// raw dump
	
	
	/* sfcb_new_cb 
	 *   adds two new circular buffers to the SPI Flash
	*/
	printf("INFO:%s:sfcb_new_cb\n", __FUNCTION__);
	sfcb_new_cb (&sfcb, 0x47114711, 244, 32, &uint8Temp);				// start-up counter with operation
	sfcb_new_cb (&sfcb, 0x08150815, 12280, 16, &uint8Temp);				// error data collection 12KiB
	print_raw_sfcb_cb(&sfcb_cb, sizeof(sfcb_cb)/sizeof(sfcb_cb[0]));	// raw dump of handling indo
	
	
	/* sfcb_mkcb 
	 *   access spi flash and build circular buffers
	*/
	printf("INFO:%s:sfcb_mkcb\n", __FUNCTION__);
	if ( 0 != sfcb_mkcb(&sfcb) ) {
		printf("ERROR:%s:sfcb_mkcb failed to start", __FUNCTION__);
		goto ERO_END;
	}
	uint32Counter = 0;
	while ( (0 != sfcb_busy(&sfcb)) && ((uint32Counter++) < uint32SpiFlashCycleOut) ) {
		/* SFCB Worker */
		sfcb_worker (&sfcb);
		/* interact SPI Flash Model */
		sfm_state = sfm(&spiFlash, (uint8_t*) &uint8Spi, sfcb_spi_len(&sfcb));
		if ( 0 != sfm_state ) {
			printf("ERROR:%s:spi_flash_model ero=%d", __FUNCTION__, sfm_state);
			goto ERO_END;
		}
	}
	if ( uint32Counter == uint32SpiFlashCycleOut ) {
		printf("ERROR:%s:sfcb_mkcb tiout reached", __FUNCTION__);
		goto ERO_END;
	}
	sfm_dump( &spiFlash, 0, 256 );	// dump SPI flash content


	/* SFCB add, queue 0 */
	for ( uint8_t i = 0; i < 63; i++ ) {
		printf("INFO:%s:sfcb_add:i=%d:add\n", __FUNCTION__, i);
		/* add element */
		if ( 0 != sfcb_add(&sfcb, 0, &uint8FlashData, sizeof(uint8FlashData)/sizeof(uint8FlashData[0])) ) {
			printf("ERROR:%s:sfcb_add failed to start", __FUNCTION__);
			goto ERO_END;
		}
		uint32Counter = 0;
		while ( (0 != sfcb_busy(&sfcb)) && ((uint32Counter++) < uint32SpiFlashCycleOut) ) {
			/* SFCB Worker */
			sfcb_worker (&sfcb);
			/* interact SPI Flash Model */
			sfm_state = sfm(&spiFlash, (uint8_t*) &uint8Spi, sfcb_spi_len(&sfcb));
			if ( 0 != sfm_state ) {
				printf("ERROR:%s:spi_flash_model ero=%d\n", __FUNCTION__, sfm_state);
				goto ERO_END;
			}
		}
		if ( uint32Counter == uint32SpiFlashCycleOut ) {
			printf("ERROR:%s:sfcb_mkcb tiout reached", __FUNCTION__);
			goto ERO_END;
		}
		/* rebuild circular buffer, prepare for next add */
		printf("INFO:%s:sfcb_add:i=%d:mkcb\n", __FUNCTION__, i);
		if ( 0 != sfcb_mkcb(&sfcb) ) {
			printf("ERROR:%s:sfcb_mkcb failed to start", __FUNCTION__);
			goto ERO_END;
		}
		uint32Counter = 0;
		while ( (0 != sfcb_busy(&sfcb)) && ((uint32Counter++) < uint32SpiFlashCycleOut) ) {
			/* SFCB Worker */
			sfcb_worker (&sfcb);
			/* interact SPI Flash Model */
			sfm_state = sfm(&spiFlash, (uint8_t*) &uint8Spi, sfcb_spi_len(&sfcb));
			if ( 0 != sfm_state ) {
				printf("ERROR:%s:spi_flash_model ero=%d\n", __FUNCTION__, sfm_state);
				/* print spi packet */
				printf("SPI Packet: ");
				for ( uint32_t j = 0; j < sfcb_spi_len(&sfcb); j++ ) {
					printf(" %02x", uint8Spi[j]);
				}
				printf("\n");
				goto ERO_END;
			}
		}
		if ( uint32Counter == uint32SpiFlashCycleOut ) {
			printf("ERROR:%s:sfcb_mkcb tiout reached", __FUNCTION__);
			goto ERO_END;
		}
	}
	if ( 0 != sfm_cmp(&spiFlash, "./test/sfcb_flash_q0_i63.dif") ) {
        printf("ERROR:%s:sfm_cmp:q0 Mismatch\n", __FUNCTION__);
        goto ERO_END;
    }
	sfm_dump( &spiFlash, 0, 256 );	// dump SPI flash content
	
	
	
	
	
	
	
	sfm_store(&spiFlash, "./flash.dif");	// write to file
	
	
	
	/* avoid warning */
	goto OK_END;

    /* gracefull end */
    OK_END:
		printf("INFO:%s: Module test SUCCESSFUL :-)\n", __FUNCTION__);
		exit(EXIT_SUCCESS);

    /* abnormal end */
    ERO_END:
        printf("FAIL:%s: Module test FAILED :-(\n", __FUNCTION__);
        exit(EXIT_FAILURE);

}
