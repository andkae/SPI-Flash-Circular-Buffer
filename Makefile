# **********************************************************************
#  @copyright	: Siemens AG
#  @license		: Siemens Inner Source License 1.4
#  @author		: Andreas Kaeberlein
#  @address		: Clemens-Winkler-Strasse 3, 09116 Chemnitz
#
#  @maintainer	: Andreas Kaeberlein
#  @telephone	: +49 371 4810-2108
#  @email		: andreas.kaeberlein@siemens.com
#
#  @file		: Makefile
#  @date		: 2022-12-13
#
#  @brief		: Build
#				  builds sources with all dependencies
# **********************************************************************



# select compiler
CC = gcc

# set linker
LINKER = gcc

# set compiler flags
ifeq ($(origin CFLAGS), undefined)
  CFLAGS = -c -O -Wall -Wextra -Wconversion -I . -I ../
endif

# linking flags here
ifeq ($(origin LFLAGS), undefined)
  LFLAGS = -Wall -Wextra -I. -lm
endif


all: sfcb_test

sfcb_test: sfcb_test.o sfcb.o
	$(LINKER) ./test/sfcb_test.o ./test/sfcb.o $(LFLAGS) -o ./test/sfcb_test

sfcb_test.o: ./test/sfcb_test.c
	$(CC) $(CFLAGS) ./test/sfcb_test.c -o ./test/sfcb_test.o

sfcb.o: ./spi_flash_cb.c
	$(CC) $(CFLAGS) -DSFCB_PRINTF_EN  ./spi_flash_cb.c -o ./test/sfcb.o

ci: ./spi_flash_cb.c
	$(CC) $(CFLAGS) -Werror ./spi_flash_cb.c -o ./test/sfcb.o

clean:
	rm -f ./test/*.o ./test/sfcb_test
