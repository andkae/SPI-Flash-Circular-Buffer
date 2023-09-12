/***********************************************************************
 @copyright     : Siemens AG
 @license       : BSDv3
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
 *  @brief print hexdump
 *
 *  dumps memory segment as ascii hex
 *
 *  @param[in]      leadBlank       number of leading blanks in hex line
 *  @param[in,out]  *mem            pointer to memory segment
 *  @param[in,out]  size            number of bytes in *mem to print
 *  @return         void
 *  @since          July 7, 2023
 */
static void print_hexdump (char leadBlank[], void *mem, size_t size)
{
    /** Variables **/
    char        charBuf[32];        // character buffer
    uint8_t     uint8NumAdrDigits;  // number of address digits


    /* check for size */
    if ( 0 == size ) {
        return;
    }
    /* acquire number of digits for address */
    snprintf(charBuf, sizeof(charBuf), "%zx", size-1);  // convert to ascii for address digits calc, -1: addresses start at zero
    uint8NumAdrDigits = (uint8_t) strlen(charBuf);
    /* print to console */
    for ( size_t i = 0; i < size; i = (i + 16) ) {
        /* print address line */
        printf("%s%0*zx:  ", leadBlank, uint8NumAdrDigits, i);
        /* print hex values */
        for ( uint8_t j = 0; j < 16; j++ ) {
            /* out of memory */
            if ( !((i+j) < size) ) {
                break;
            }
            /* print */
            printf("%02x ", ((uint8_t*) mem)[i+j]);
            /* divide high / low byte */
            if ( 7 == j ) {
                printf(" ");
            }
        }
        printf("\n");
    }
}



/**
 *  prints raw data of SFCB
 */
static void print_raw_sfcb_cb (void* sfcb_cb, uint8_t num_sfcb_cb )
{
	/* print to console */
	for ( uint8_t i = 0; i < num_sfcb_cb; i++ ) {
		printf("INFO:sfcb_cb[%i]: ", i);
		for ( uint8_t j = 0; j < sizeof(t_sfcb_cb); j++ ) {
			printf("%02x", *(((uint8_t*) ((t_sfcb_cb*) sfcb_cb))+i*sizeof(t_sfcb_cb)+j));
		}
		printf("\n");
	}
}



/**
 *  Main
 *  ----
 */
int main ()
{
    /** Variables **/
	const uint32_t	uint32SpiFlashCycleOut = 1000;		// abort calling SPI flash
	const uint16_t	uint16CbQ0Size = 244;				// CB Q0 Payload size
	const uint16_t	uint16CbQ1Size = 12280;				// CB Q1 Payload size
	uint32_t		uint32Counter;						// counter
	t_sfm       	spiFlash;   						// SPI flash model
	int				sfm_state;							// SPI Flash model return state
	t_sfcb			sfcb;								// SPI Flash as circular buffer
	t_sfcb_cb		sfcb_cb[5];							// five logical parts in SPI Flash
	uint8_t			uint8Spi[266];						// SPI packet buffer
	uint8_t			uint8Temp;							// help variable
	uint8_t			uint8FlashData[] = {0,1,2,3,4,5};	// SPI test data
	uint8_t			uint8Buf[1024];						// help buffer
	uint8_t*		uint8PtrDat1 = NULL;				// temporary data buffer
	uint8_t*		uint8PtrDat2 = NULL;				// temporary data buffer
	

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
	printf("INFO:%s:uint8Spi_p      = %p\n", 		__FUNCTION__, &uint8Spi);
	printf("INFO:%s:sfcb_p          = %p\n", 		__FUNCTION__, &sfcb);
	printf("INFO:%s:sfcb_cb_p       = %p\n", 		__FUNCTION__, &sfcb_cb);
	printf("INFO:%s:sfcb_size       = %d byte\n",	__FUNCTION__, (int) sizeof(sfcb));
	printf("INFO:%s:sfcb_cb[0]_size = %d byte\n",	__FUNCTION__, (int) sizeof(sfcb_cb[0]));
	memset(sfcb_cb, 0xaf, sizeof(sfcb_cb));	// mess-up memory to check init
	/* int sfcb_init (t_sfcb *self, void *cb, uint8_t cbLen, void *spi, uint16_t spiLen) */
	sfcb_init (&sfcb, &sfcb_cb, sizeof(sfcb_cb)/sizeof(sfcb_cb[0]), &uint8Spi, sizeof(uint8Spi)/sizeof(uint8Spi[0]));
	/* check for error */
	for ( uint8_t i = 0; i < sizeof(sfcb_cb)/sizeof(sfcb_cb[0]); i++ ) {
		/* check flags */
		if ( 0 != (sfcb_cb[i]).uint8Used ) {
			printf("ERROR:%s:sfcb_init:uint8Used", __FUNCTION__);
			goto ERO_END;
		}
		if ( 0 != (sfcb_cb[i]).uint8MgmtValid) {
			printf("ERROR:%s:sfcb_init:uint8MgmtValid", __FUNCTION__);
			goto ERO_END;
		}
	}
	//print_raw_sfcb_cb(&sfcb_cb, sizeof(sfcb_cb)/sizeof(sfcb_cb[0]));	// raw dump
	
	
	/* sfcb_new_cb 
	 *   adds two new circular buffers to the SPI Flash
	*/
	printf("INFO:%s:sfcb_new_cb\n", __FUNCTION__);
	sfcb_new_cb (&sfcb, 0x47114711, uint16CbQ0Size, 32, &uint8Temp);	// start-up counter with operation
	sfcb_new_cb (&sfcb, 0x08150815, uint16CbQ1Size, 16, &uint8Temp);	// error data collection 12KiB
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
	/* check highest id */
	uint32Counter = sfcb_idmax(&sfcb, 0);
	if ( 63 != uint32Counter ) {
        printf("ERROR:%s:sfcb_idmax:q0 exp,idmax=63, is,idmax=%d\n", __FUNCTION__, uint32Counter);
        goto ERO_END;
	}
	
	
	/* sfcb_flash_read 
	 *   reads raw binary data from flash
	 */
	printf("INFO:%s:sfcb_flash_read\n", __FUNCTION__);	
		// int sfcb_flash_read (t_sfcb *self, uint32_t adr, void *data, uint16_t len)
	if ( 0 != sfcb_flash_read (&sfcb, 0, &uint8Buf, 256) ) {
		printf("ERROR:%s:sfcb_flash_read failed to start", __FUNCTION__);
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
	/* compare data */
	for ( uint32_t i = 0; i < 256; i++ ) {
		if ( spiFlash.uint8PtrMem[i] != uint8Buf[i] ) {
			printf("ERROR:%s:sfcb_flash_read: byte=%d, exp=0x%02x, is=0x%02x\n", __FUNCTION__, i, spiFlash.uint8PtrMem[i], uint8Buf[i]);
			goto ERO_END;
		}
	}


	/* sfcb_get_last
	 *   reads last written element back
	 */
	printf("INFO:%s:sfcb_get_last\n", __FUNCTION__);
	/* prepare data set */
	uint8PtrDat1 = malloc(uint16CbQ0Size);	// reference buffer
	uint8PtrDat2 = malloc(uint16CbQ0Size);	// data buffer
	for ( uint16_t i = 0; i < uint16CbQ0Size; i++ ) {	// create random numbers
		uint8PtrDat1[i] = (uint8_t) (rand() % 256);
	}
	memcpy(uint8PtrDat2, uint8PtrDat1, uint16CbQ0Size);
	/* write into Q */
	if ( 0 != sfcb_add(&sfcb, 0, uint8PtrDat1, uint16CbQ0Size) ) {
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
	/* compare if write data does not destroy */
	for ( uint16_t i = 0; i < uint16CbQ0Size; i++ ) {
		if ( uint8PtrDat1[i] != uint8PtrDat2[i] ) {
			printf("ERROR:%s:sfcb_add write data destroyed", __FUNCTION__);
			goto ERO_END;
		}
	}
	/* rebuild managment data */
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
	/* get last element from spi flash */
	memset(uint8PtrDat2, 0, uint16CbQ0Size);	// destroy buffer
		// int sfcb_get_last (t_sfcb *self, uint8_t cbID, void *data, uint16_t len)
	if ( 0 != sfcb_get_last(&sfcb, 0, uint8PtrDat2, uint16CbQ0Size) ) {
		printf("ERROR:%s:sfcb_get_last failed to start", __FUNCTION__);
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
	/* compare for corect read */
	for ( uint16_t i = 0; i < uint16CbQ0Size; i++ ) {
		if ( uint8PtrDat1[i] != uint8PtrDat2[i] ) {
			printf("ERROR:%s:sfcb_get_last: wrong data ofs=0x%x is=0x%02x, exp=0x%02x\n", __FUNCTION__, i, uint8PtrDat2[i], uint8PtrDat1[i]);
			printf("  Is-Dump:\n");
			printf("  --------\n");
			print_hexdump("    ", uint8PtrDat2, uint16CbQ0Size);
			printf("  Exp-Dump:\n");
			printf("  ---------\n");
			print_hexdump("    ", uint8PtrDat1, uint16CbQ0Size);
			goto ERO_END;
		}
	}
	
	

	/* write to file */
	sfm_store(&spiFlash, "./flash.dif");
	
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
