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



/** Globals **/
const uint32_t	g_uint32SpiFlashCycleOut = 1000;	// abort calling SPI flash
const uint16_t	g_uint16CbQ0Size = 244;				// CB Q0 Payload size
const uint16_t	g_uint16CbQ1Size = 12280;			// CB Q1 Payload size
uint8_t			g_uint8Spi[266];					// SPI packet buffer



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
 *  @brief mem cmp
 *
 *  Compare to memory segments and output in case of difference
 *
 *  @param[in,out]  *is            		is data segement
 *  @param[in,out]  *exp            	expected data segement
 *  @param[in]  	len             	number of bytes in *is and *exp
 *  @return         int                 test state
 *  @retval         0                   Success
 *  @retval         -1                  Fail
 *  @since          September 13, 2023
 */
static int mem_cmp (uint8_t* is, uint8_t* exp, uint16_t len)
{
	/* compare for corect read */
	for ( uint16_t i = 0; i < len; i++ ) {
		if ( is[i] != exp[i] ) {
			printf("ERROR:%s:sfcb_get_last: wrong data ofs=0x%x is=0x%02x, exp=0x%02x\n", __FUNCTION__, i, is[i], exp[i]);
			printf("  Is-Dump:\n");
			printf("  --------\n");
			print_hexdump("    ", is, len);
			printf("  Exp-Dump:\n");
			printf("  ---------\n");
			print_hexdump("    ", exp, len);
			return -1;
		}
	}
	return 0;
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
 *  @brief run_sfm_update
 *
 *  update SPI Flash Model
 *
 *  @param[in,out]  flash       		spi flash model handle, #t_sfm
 *  @param[in,out]  sfcb            	spi flash circular buffer handle, #t_sfcb
 *  @return         int                 test state
 *  @retval         0                   Success
 *  @retval         -1                  Fail
 *  @since          September 13, 2023
 *  @author         Andreas Kaeberlein
 */
static int run_sfm_update (t_sfm* flash, t_sfcb* sfcb)
{
	/** Variables **/
	uint32_t	uint32Counter;			// counter for time out
	int			sfm_state;				// SPI Flash model return state	
	
	/* entry message */
	printf("__FUNCTION__ = %s\n", __FUNCTION__);	
	/* update sfm */
	uint32Counter = 0;
	while ( (0 != sfcb_busy(sfcb)) && ((uint32Counter++) < g_uint32SpiFlashCycleOut) ) {
		/* SFCB Worker */
		sfcb_worker (sfcb);
		/* interact SPI Flash Model */
		sfm_state = sfm(flash, (uint8_t*) &g_uint8Spi, sfcb_spi_len(sfcb));
		if ( 0 != sfm_state ) {
			printf("ERROR:%s:spi_flash_model ero=%d\n", __FUNCTION__, sfm_state);
			/* print spi packet */
			printf("SPI Packet: ");
			for ( uint32_t i = 0; i < sfcb_spi_len(sfcb); i++ ) {
				printf(" %02x", g_uint8Spi[i]);
			}
			printf("\n");
			return -1;
		}
	}
	if ( uint32Counter == g_uint32SpiFlashCycleOut ) {
		printf("ERROR:%s:sfcb_mkcb tiout reached", __FUNCTION__);
		return -1;
	}
	/* all done */
	return 0;
}



/**
 *  @brief run_sfcb_add
 *
 *  adds element to SFCB buffer queue and perform update managament data
 *
 *  @param[in,out]  flash       		spi flash model handle, #t_sfm
 *  @param[in,out]  sfcb            	spi flash circular buffer handle, #t_sfcb
 *  @param[in]  	qNum           	 	number of tested circular buffer queue
 *  @param[in]  	data           		array with data to write into circular buffer
 *  @param[in]  	len           		number of bytes in data
 *  @return         int                 test state
 *  @retval         0                   Success
 *  @retval         -1                  Fail
 *  @since          September 12, 2023
 *  @author         Andreas Kaeberlein
 */
static int run_sfcb_add (t_sfm* flash, t_sfcb* sfcb, uint8_t qNum, uint8_t* data, uint16_t len)
{
	/** Variables **/
	uint8_t*	uint8PtrData = NULL;	// temporary data buffer, backup to check for destroy
	
	/* entry message */
	printf("__FUNCTION__ = %s\n", __FUNCTION__);
	/* make backup */
	uint8PtrData = malloc(len);
	if ( NULL == uint8PtrData ) {
		printf("ERROR:%s: malloc fail", __FUNCTION__);
		return -1;
	}
	memcpy(uint8PtrData, data, len);
	/* write into Q */
	if ( 0 != sfcb_add(sfcb, qNum, data, len) ) {
		printf("ERROR:%s:sfcb_add failed to start", __FUNCTION__);
		return -1;
	}
		// run_sfm_update (t_sfm* flash, t_sfcb* sfcb)
	if ( 0 != run_sfm_update(flash, sfcb) ) {
		printf("ERROR:%s:run_sfm_update\n", __FUNCTION__);
		return -1;
	}
	/* compare if write data does not destroy */
	for ( uint16_t i = 0; i < len; i++ ) {
		if ( data[i] != uint8PtrData[i] ) {
			printf("ERROR:%s:sfcb_add write data destroyed", __FUNCTION__);
			return -1;
		}
	}
	/* rebuild managment data */
	if ( 0 != sfcb_mkcb(sfcb) ) {
		printf("ERROR:%s:sfcb_mkcb failed to start", __FUNCTION__);
		return -1;
	}
		// run_sfm_update (t_sfm* flash, t_sfcb* sfcb)
	if ( 0 != run_sfm_update(flash, sfcb) ) {
		printf("ERROR:%s:run_sfm_update\n", __FUNCTION__);
		return -1;
	}
	/* all done */
	free(uint8PtrData);
	return 0;
}



/**
 *  @brief run_sfcb_add_append
 *
 *  adds element to cirrcular buffer entry in portions of bytes
 *    1) append(1) -> write one byte to payload segment OFS=0
 *    2) append(1) -> write one byte to payload segment OFS=1
 *    and so on...
 *
 *  @param[in,out]  flash       		spi flash model handle, #t_sfm
 *  @param[in,out]  sfcb            	spi flash circular buffer handle, #t_sfcb
 *  @param[in]  	qNum           	 	number of tested circular buffer queue
 *  @param[in]  	data           		array with data to write into circular buffer
 *  @param[in]  	len           		number of bytes in data
 *  @return         int                 test state
 *  @retval         0                   Success
 *  @retval         -1                  Fail
 *  @since          September 12, 2023
 *  @author         Andreas Kaeberlein
 */
static int run_sfcb_add_append (t_sfm* flash, t_sfcb* sfcb, uint8_t qNum, uint8_t* data, uint16_t len)
{
	/** Variables **/
	uint8_t*	uint8PtrData = NULL;	// temporary data buffer, backup to check for destroy
	
	/* entry message */
	printf("__FUNCTION__ = %s\n", __FUNCTION__);
	/* make backup */
	uint8PtrData = malloc(len);
	if ( NULL == uint8PtrData ) {
		printf("ERROR:%s: malloc fail", __FUNCTION__);
		return -1;
	}
	memcpy(uint8PtrData, data, len);
	/* write into Q */
	if ( 0 != sfcb_add_append(sfcb, qNum, data, len) ) {
		printf("ERROR:%s:sfcb_add failed to start", __FUNCTION__);
		return -1;
	}
		// run_sfm_update (t_sfm* flash, t_sfcb* sfcb)
	if ( 0 != run_sfm_update(flash, sfcb) ) {
		printf("ERROR:%s:run_sfm_update\n", __FUNCTION__);
		return -1;
	}
	/* compare if write data does not destroy */
	for ( uint16_t i = 0; i < len; i++ ) {
		if ( data[i] != uint8PtrData[i] ) {
			printf("ERROR:%s:sfcb_add write data destroyed", __FUNCTION__);
			return -1;
		}
	}
	/* all done */
	free(uint8PtrData);
	return 0;
}



/**
 *  @brief run_sfcb_get_last
 *
 *  adds element to SFCB buffer queue and perform update managament data
 *
 *  @param[in,out]  flash       		spi flash model handle, #t_sfm
 *  @param[in,out]  sfcb            	spi flash circular buffer handle, #t_sfcb
 *  @param[in]  	qNum           	 	number of tested circular buffer queue
 *  @param[in]  	data           		array with data to write into circular buffer
 *  @param[in]  	len           		number of bytes in data
 *  @return         int                 test state
 *  @retval         0                   Success
 *  @retval         -1                  Fail
 *  @since          September 13, 2023
 *  @author         Andreas Kaeberlein
 */
static int run_sfcb_get_last (t_sfm* flash, t_sfcb* sfcb, uint8_t qNum, uint8_t* data, uint16_t len)
{
	/* entry message */
	printf("__FUNCTION__ = %s\n", __FUNCTION__);
		// int sfcb_get_last (t_sfcb *self, uint8_t cbID, void *data, uint16_t len)
	if ( 0 != sfcb_get_last(sfcb, qNum, data, len) ) {
		printf("ERROR:%s:sfcb_get_last failed to start", __FUNCTION__);
		return -1;
	}
		// run_sfm_update (t_sfm* flash, t_sfcb* sfcb)
	if ( 0 != run_sfm_update(flash, sfcb) ) {
		printf("ERROR:%s:run_sfm_update\n", __FUNCTION__);
		return -1;
	}
	/* all done */
	return 0;
}



/**
 *  @brief check sfcb_get_last
 *
 *  writes element in circular buffer
 *  reads element back and compares the data
 *
 *  @param[in,out]  flash       		spi flash model handle, #t_sfm
 *  @param[in,out]  sfcb            	spi flash circular buffer handle, #t_sfcb
 *  @param[in]  	qNum           	 	number of tested circular buffer queue
 *  @param[in]  	qSize           	test data size for selected circular buffer queue
 *  @return         int                 test state
 *  @retval         0                   Success
 *  @retval         -1                  Fail
 *  @since          September 12, 2023
 *  @author         Andreas Kaeberlein
 */
static int test_get_last (t_sfm* flash, t_sfcb* sfcb, uint8_t qNum, uint16_t qSize)
{
	/** Variables **/
	uint8_t*	uint8PtrDat1 = NULL;	// temporary data buffer
	uint8_t*	uint8PtrDat2 = NULL;	// temporary data buffer
	
	/* entry message */
	printf("__FUNCTION__ = %s\n", __FUNCTION__);
	/* prepare data set */
	uint8PtrDat1 = malloc(qSize);	// reference buffer
	uint8PtrDat2 = malloc(qSize);	// data buffer
	for ( uint16_t i = 0; i < qSize; i++ ) {	// create random numbers
		uint8PtrDat1[i] = (uint8_t) (rand() % 256);
	}
	memcpy(uint8PtrDat2, uint8PtrDat1, qSize);
	/* write into Q */
		// run_sfcb_add (t_sfm* flash, t_sfcb* sfcb, uint8_t qNum, uint8_t* data, uint16_t len)
	if ( 0 != run_sfcb_add(flash, sfcb, qNum, uint8PtrDat1, qSize) ) {
		printf("ERROR:%s:run_sfcb_add failed", __FUNCTION__);
		return -1;
	}
	/* get last element from spi flash */
	memset(uint8PtrDat2, 0, qSize);	// destroy buffer
		// run_sfcb_get_last (t_sfm* flash, t_sfcb* sfcb, uint8_t qNum, uint8_t* data, uint16_t len)
	if ( 0 != run_sfcb_get_last(flash, sfcb, qNum, uint8PtrDat2, qSize) ) {
		printf("ERROR:%s:run_sfcb_get_last failed to start", __FUNCTION__);
		return -1;		
	}
	/* compare for corect read */
		// mem_cmp (uint8_t* is, uint8_t* exp, uint16_t len)
	if ( 0 != mem_cmp(uint8PtrDat2, uint8PtrDat1, qSize) ) {
		printf("ERROR:%s:mem_cmp", __FUNCTION__);
		return -1;
	}
	/* all done */
	free(uint8PtrDat1);
	free(uint8PtrDat2);	
	return 0;
}



/**
 *  @brief test_add_append
 *
 *  writes bytewise into payload element
 *  reads element back and compares the data
 *
 *  @param[in,out]  flash       		spi flash model handle, #t_sfm
 *  @param[in,out]  sfcb            	spi flash circular buffer handle, #t_sfcb
 *  @param[in]  	qNum           	 	number of tested circular buffer queue
 *  @param[in]  	qSize           	test data size for selected circular buffer queue
 *  @return         int                 test state
 *  @retval         0                   Success
 *  @retval         -1                  Fail
 *  @since          September 13, 2023
 *  @author         Andreas Kaeberlein
 */
static int test_add_append (t_sfm* flash, t_sfcb* sfcb, uint8_t qNum, uint16_t qSize)
{
	/** Variables **/
	uint8_t*	uint8PtrDat1 = NULL;	// temporary data buffer
	uint8_t*	uint8PtrDat2 = NULL;	// temporary data buffer
	
	/* entry message */
	printf("__FUNCTION__ = %s\n", __FUNCTION__);
	/* prepare data set */
	uint8PtrDat1 = malloc(qSize);	// reference buffer
	uint8PtrDat2 = malloc(qSize);	// data buffer
	for ( uint16_t i = 0; i < qSize; i++ ) {	// create random numbers
		uint8PtrDat1[i] = (uint8_t) (rand() % 256);
	}
	memcpy(uint8PtrDat2, uint8PtrDat1, qSize);
	/* write into Q, bytewise */
	for ( uint16_t i = 0; i < qSize; i++ ) {
		// run_sfcb_add_append (t_sfm* flash, t_sfcb* sfcb, uint8_t qNum, uint8_t* data, uint16_t len)
		if ( 0 != run_sfcb_add_append (flash, sfcb, qNum, uint8PtrDat1+i, 1) ) {
			printf("ERROR:%s:run_sfcb_add: add byte=%d failed", __FUNCTION__, i);
			return -1;
		}
	}
	/* rebuild sfcb */
	if ( 0 != sfcb_mkcb(sfcb) ) {
		printf("ERROR:%s:sfcb_mkcb failed to start", __FUNCTION__);
		return -1;
	}
		// run_sfm_update (t_sfm* flash, t_sfcb* sfcb)
	if ( 0 != run_sfm_update(flash, sfcb) ) {
		printf("ERROR:%s:run_sfm_update\n", __FUNCTION__);
		return -1;
	}
	/* get last element from spi flash */
	memset(uint8PtrDat2, 0, qSize);	// destroy buffer
		// run_sfcb_get_last (t_sfm* flash, t_sfcb* sfcb, uint8_t qNum, uint8_t* data, uint16_t len)
	if ( 0 != run_sfcb_get_last(flash, sfcb, qNum, uint8PtrDat2, qSize) ) {
		printf("ERROR:%s:run_sfcb_get_last failed to start", __FUNCTION__);
		return -1;		
	}
	/* compare for corect read */
		// mem_cmp (uint8_t* is, uint8_t* exp, uint16_t len)
	if ( 0 != mem_cmp(uint8PtrDat2, uint8PtrDat1, qSize) ) {
		printf("ERROR:%s:mem_cmp", __FUNCTION__);
		return -1;
	}
	/* all done */
	free(uint8PtrDat1);
	free(uint8PtrDat2);	
	return 0;
}





/**
 *  Main
 *  ----
 */
int main ()
{
    /** Variables **/
	t_sfm       	spiFlash;   						// SPI flash model
	t_sfcb			sfcb;								// SPI Flash as circular buffer
	t_sfcb_cb		sfcb_cb[5];							// five logical parts in SPI Flash
	uint8_t			uint8Temp;							// help variable
	uint8_t			uint8FlashData[] = {0,1,2,3,4,5};	// SPI test data
	uint8_t			uint8Buf[1024];						// help buffer
	

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
	printf("INFO:%s:uint8Spi_p      = %p\n", 		__FUNCTION__, &g_uint8Spi);
	printf("INFO:%s:sfcb_p          = %p\n", 		__FUNCTION__, &sfcb);
	printf("INFO:%s:sfcb_cb_p       = %p\n", 		__FUNCTION__, &sfcb_cb);
	printf("INFO:%s:sfcb_size       = %d byte\n",	__FUNCTION__, (int) sizeof(sfcb));
	printf("INFO:%s:sfcb_cb[0]_size = %d byte\n",	__FUNCTION__, (int) sizeof(sfcb_cb[0]));
	memset(sfcb_cb, 0xaf, sizeof(sfcb_cb));	// mess-up memory to check init
	/* int sfcb_init (t_sfcb *self, void *cb, uint8_t cbLen, void *spi, uint16_t spiLen) */
	sfcb_init (&sfcb, &sfcb_cb, sizeof(sfcb_cb)/sizeof(sfcb_cb[0]), &g_uint8Spi, sizeof(g_uint8Spi)/sizeof(g_uint8Spi[0]));
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
	sfcb_new_cb (&sfcb, 0x47114711, g_uint16CbQ0Size, 32, &uint8Temp);	// start-up counter with operation
	sfcb_new_cb (&sfcb, 0x08150815, g_uint16CbQ1Size, 16, &uint8Temp);	// error data collection 12KiB
	print_raw_sfcb_cb(&sfcb_cb, sizeof(sfcb_cb)/sizeof(sfcb_cb[0]));	// raw dump of handling indo
	
	
	/* sfcb_mkcb 
	 *   access spi flash and build circular buffers
	 */
	printf("INFO:%s:sfcb_mkcb\n", __FUNCTION__);
	if ( 0 != sfcb_mkcb(&sfcb) ) {
		printf("ERROR:%s:sfcb_mkcb failed to start", __FUNCTION__);
		goto ERO_END;
	}
		// run_sfm_update (t_sfm* flash, t_sfcb* sfcb)
	if ( 0 != run_sfm_update(&spiFlash, &sfcb) ) {
		printf("ERROR:%s:run_sfm_update\n", __FUNCTION__);
		goto ERO_END;
	}
	sfm_dump( &spiFlash, 0, 256 );	// dump SPI flash content



	////////////////////////////////////////////
	//
	//  Single Pages Payloads
	//
	////////////////////////////////////////////

	/* SFCB add, queue 0 */
	for ( uint8_t i = 0; i < 63; i++ ) {
		printf("INFO:%s:sfcb_add:i=%d:add\n", __FUNCTION__, i);
		/* add element */
			// run_sfcb_add (t_sfm* flash, t_sfcb* sfcb, uint8_t qNum, uint8_t* data, uint16_t len)
		if ( 0 != run_sfcb_add(&spiFlash, &sfcb, 0, (uint8_t*) &uint8FlashData, sizeof(uint8FlashData)/sizeof(uint8FlashData[0])) ) {
			printf("ERROR:%s:run_sfcb_add failed", __FUNCTION__);
			return -1;
		}
	}
	if ( 0 != sfm_cmp(&spiFlash, "./test/sfcb_flash_q0_i63.dif") ) {
        printf("ERROR:%s:sfm_cmp:q0 Mismatch\n", __FUNCTION__);
        goto ERO_END;
    }
	/* check highest id */
	if ( 63 != sfcb_idmax(&sfcb, 0) ) {
        printf("ERROR:%s:sfcb_idmax:q0 exp,idmax=63, is,idmax=%d\n", __FUNCTION__, sfcb_idmax(&sfcb, 0));
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
		// run_sfm_update (t_sfm* flash, t_sfcb* sfcb)
	if ( 0 != run_sfm_update(&spiFlash, &sfcb) ) {
		printf("ERROR:%s:run_sfm_update\n", __FUNCTION__);
		goto ERO_END;
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
	printf("INFO:%s:sfcb_get_last:q0: payload size = %d bytes\n", __FUNCTION__, g_uint16CbQ0Size);
		// static int test_get_last (t_sfm* flash, t_sfcb* sfcb, uint8_t qNum, uint16_t qSize)
	if ( 0 != test_get_last(&spiFlash, &sfcb, 0, g_uint16CbQ0Size) ) {
		goto ERO_END;
	}


	/* sfcb_add_append
	 *   write to SPI flash in portions of bytes
	 */
	printf("INFO:%s:sfcb_add_append:q0: payload size = %d bytes\n", __FUNCTION__, g_uint16CbQ0Size);
		// static int test_add_append (t_sfm* flash, t_sfcb* sfcb, uint8_t qNum, uint16_t qSize)
	if ( 0 != test_add_append(&spiFlash, &sfcb, 0, g_uint16CbQ0Size) ) {
		goto ERO_END;
	}	

	
	////////////////////////////////////////////
	//
	//  Multiple Pages Payloads
	//
	////////////////////////////////////////////
	
	/* sfcb_get_last
	 *   reads last written element back
	 */
	printf("INFO:%s:sfcb_get_last:q1: payload size = %d bytes\n", __FUNCTION__, g_uint16CbQ1Size);
		// static int test_get_last (t_sfm* flash, t_sfcb* sfcb, uint8_t qNum, uint16_t qSize)
	if ( 0 != test_get_last(&spiFlash, &sfcb, 1, g_uint16CbQ1Size) ) {
		goto ERO_END;
	}	
	
	
	
	
	
	
	////////////////////////////////////////////
	//
	//  Minor Stuff at End
	//
	////////////////////////////////////////////	
	
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
		sfm_store(&spiFlash, "./flash_error.dif");
        printf("FAIL:%s: Module test FAILED :-(\n", __FUNCTION__);
        exit(EXIT_FAILURE);

}
