# **********************************************************************
#  @copyright   : Siemens AG
#  @license     : BSDv3
#  @author      : Andreas Kaeberlein
#  @address     : Clemens-Winkler-Strasse 3, 09116 Chemnitz
#
#  @maintainer  : Andreas Kaeberlein
#  @telephone   : +49 371 4810-2108
#  @email       : andreas.kaeberlein@siemens.com
#
#  @file        : Makefile
#  @date        : 2022-12-13
#
#  @brief       : Build
#                 builds sources with all dependencies
# **********************************************************************



# select compiler
CC = gcc

# set linker
LINKER = gcc

# set compiler flags
ifeq ($(origin CFLAGS), undefined)
  CFLAGS = -c -O -Wall -Wextra -Wconversion -I . -I ../ -DW25Q16JV
endif

# linking flags here
ifeq ($(origin LFLAGS), undefined)
  LFLAGS = -Wall -Wextra -I. -lm
endif


all: sfcb_test

sfcb_test: sfcb_test.o sfcb.o spi_flash_model.o
	$(LINKER) ./test/sfcb_test.o ./test/sfcb.o ./test/spi_flash_model.o $(LFLAGS) -o ./test/sfcb_test

sfcb_test.o: ./test/sfcb_test.c
	$(CC) $(CFLAGS) ./test/sfcb_test.c -o ./test/sfcb_test.o

sfcb.o: ./spi_flash_cb.c
	$(CC) $(CFLAGS) -DSFCB_PRINTF_EN ./spi_flash_cb.c -o ./test/sfcb.o
	
spi_flash_model.o: ./test/spi_flash_model/spi_flash_model.c
	$(CC) $(CFLAGS)  ./test/spi_flash_model/spi_flash_model.c -o ./test/spi_flash_model.o

ci: ./spi_flash_cb.c
	$(CC) $(CFLAGS) -Werror ./spi_flash_cb.c -o ./test/sfcb.o

clean:
	rm -f ./test/*.o ./test/sfcb_test
