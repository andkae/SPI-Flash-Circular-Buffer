[![Test](https://github.com/andkae/SPI-Flash-Circular-Buffer/actions/workflows/test.yml/badge.svg)](https://github.com/andkae/SPI-Flash-Circular-Buffer/actions/workflows/test.yml)


# SPI Flash Circular Buffer
C Library to transform a physical SPI Flash into a logical circular buffer. The interface between _[SFCB](https://github.com/andkae/SPI-Flash-Circular-Buffer)_
and SPI core is realized as shared memory.



## Features
* Arbitrary SPI [Flash](/sfcb_flash_types.h) support
* Arbitrary number of circular buffer queues (_cbID_) in one SPI flash
* Interaction between circular buffer and SPI interface is realized as shared memory



## Releases
| Version                                                | Date       | Source                                                                                                                 | Change log                                                                                                                                         |
| ------------------------------------------------------ | ---------- | ---------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| latest                                                 |            | <a id="raw-url" href="https://github.com/andkae/SPI-Flash-Circular-Buffer/archive/refs/heads/main.zip">latest.zip</a>  |                                                                                                                                                    |



## How-to

### Clone
```bash
git clone --recursive https://github.com/andkae/SPI-Flash-Circular-Buffer.git
```

### Build
The [Makefile](/Makefile) builds the repository with [unit test](/test/sfcb_test.c):
```bash
$ make
gcc -c -O -Wall -Wextra -Wconversion -I . -I ../ -DW25Q16JV ./test/sfcb_test.c -o ./test/sfcb_test.o
gcc -c -O -Wall -Wextra -Wconversion -I . -I ../ -DW25Q16JV -DSFCB_PRINTF_EN ./spi_flash_cb.c -o ./test/sfcb.o
gcc -c -O -Wall -Wextra -Wconversion -I . -I ../ -DW25Q16JV  ./test/spi_flash_model/spi_flash_model.c -o ./test/spi_flash_model.o
gcc ./test/sfcb_test.o ./test/sfcb.o ./test/spi_flash_model.o -Wall -Wextra -I. -lm -o ./test/sfcb_test
```

The library part itself can be built with:
```bash
gcc -c -O -Wall -Wextra -Wconversion -I . -DW25Q16JV -Werror ./spi_flash_cb.c -o ./test/sfcb.o
```

The flash memory [_W25Q16JV_](/sfcb_flash_types.h) was selected via compile switch ```-D```.



### Example



### Test
To run the unit test call:
```bash
$ ./test/sfcb_test
```



## [API](./spi_flash_cb.h)

### Init
Initializes _SFCB_ common handle and assigns memory.

```c
int sfcb_init (t_sfcb *self, void *cb, uint8_t cbLen, void *spi, uint16_t spiLen);
```

#### Arguments:
| Arg    | Description                       |
| ------ | --------------------------------- |
| self   | _SFCB_ storage element            |
| cb     | Circular buffer queue memory      |
| cbLen  | max. number of _cb_ queues        |
| spi    | _SFCB_ / SPI core exchange buffer |
| spiLen | _spi_ buffer size in bytes        |

#### Return:
* Success: *== 0*
* Failure: *!= 0*


### New queue
Creates a new logical independent circular buffer queue in the SPI Flash.

```c
int sfcb_new_cb (t_sfcb *self, uint32_t magicNum, uint16_t elemSizeByte, uint16_t numElems, uint8_t *cbID);
```

#### Arguments:
| Arg          | Description                                                                    |
| ------------ | ------------------------------------------------------------------------------ |
| self         | _SFCB_ storage element                                                         |
| magicNum     | Magic number, needs to be unique for every queue on the same SPI Flash         |
| elemSizeByte | Payload size in byte, header/footer causes                                     |
| numElems     | minimal number of required elements in queue, rounded up to next full _SECTOR_ |
| cbID         | assigned _ID_ to this queue, needed for all further requests                   |

#### Return:
| Value | Description                                                      |
| ----- | ---------------------------------------------------------------- |
| 0     | Success                                                          |
| 1     | Fail: Not enough management memory, assign more memory in _init_ |


### Busy
Checks if _SFCB_ is processing another request.

```c
int sfcb_busy (t_sfcb *self);
```

#### Arguments:
| Arg  | Description            |
| ---- | ---------------------- |
| self | _SFCB_ storage element |

#### Return:
| Value | Description |
| ----- | ----------- |
| 0     | Free        |
| 1     | Busy        |


### Build
Acquires all queue information from SPI flash. Needed after calling _sfcb_add_ to update all management information.

```c
int sfcb_mkcb (t_sfcb *self);
```

#### Arguments:
| Arg  | Description            |
| ---- | ---------------------- |
| self | _SFCB_ storage element |

#### Return:
| Value | Description                         |
| ----- | ----------------------------------- |
| 0     | Okay                                |
| 1     | _SFCB_ busy                         |
| 2     | no active queues, use _sfcb_new_cb_ |


### Add (Append)

Append bytes to the current selected circular buffer queue element.
```c
int sfcb_add (t_sfcb *self, uint8_t cbID, void *data, uint16_t len);
```

#### Arguments:
| Arg   | Description                       |
| ----- | --------------------------------- |
| self  | _SFCB_ storage element            |
| cbID  | circular buffer queue to interact |
| *data | pointer to write data             |
| len   | number of bytes in _*data_        |

#### Return:
[Exit codes](#return--exit-codes)


### Worker
Services circular buffer layer request as well SPI packet processing.
This function should called in a time based matter.
The SPI data packet transfer should use an ISR based dataflow.

```c
void sfcb_worker (t_sfcb *self);
```

#### Arguments:
| Arg  | Description            |
| ---- | ---------------------- |
| self | _SFCB_ storage element |


### SPI packet size
By _sfcb_worker_ created SPI packet size in bytes.

```c
uint16_t sfcb_spi_len (t_sfcb *self);
```

#### Arguments:
| Arg  | Description            |
| ---- | ---------------------- |
| self | _SFCB_ storage element |

#### Return:
Byte Count of SPI packet.



### Flash Size
Get _SFCB_ compiled flash type total size.

```c
uint32_t sfcb_flash_size (void);
```

#### Return:
Size in bytes.


### Return: Exit codes
| Value                                    | Description                                                                   |
| ---------------------------------------- | ----------------------------------------------------------------------------- |
| [SFCB_OK](/spi_flash_cb.h#L30)           | Request accepted                                                              |
| [SFCB_E_NO_FLASH](/spi_flash_cb.h#L31)   | no flash type selected, use ```-D```                                          |
| [SFCB_E_MEM](/spi_flash_cb.h#L32)        | not enough memory assigned in ```sfcb_init```                                 |
| [SFCB_E_FLASH_FULL](/spi_flash_cb.h#L33) | Flash capacity exceeded                                                       |
| [SFCB_E_WKR_BSY](/spi_flash_cb.h#L34)    | ```sfcb_worker``` is busy, wait                                               |
| [SFCB_E_NO_CB_Q](/spi_flash_cb.h#L35)    | circular buffer queue ```cbID``` not existent                                 |
| [SFCB_E_WKR_REQ](/spi_flash_cb.h#L36)    | circular buffer management data not prepared for request, run ```sfcb_mkcb``` |
| [SFCB_E_CB_Q_MTY](/spi_flash_cb.h#L37)   | no valid entries in queue                                                     |



## Memory organization
The [SFCB](https://github.com/andkae/SPI-Flash-Circular-Buffer) supports an arbitrary number of circular buffer queues.
Each circular buffer starts at the lowest free SPI Flash address. The Flash architecture requires an dedicated data clear -
so called _Sector Erase_. Through this limitation needs to be at least two sectors allocated. Otherwise would the overwrite
of the first written element result in an complete circular buffer queue overwrite without keeping any previous entries.
Every new entry is marked with the incremented highest 32bit _IdNum_ and _MagicNum_. The _MagicNum_ ensures the detection
of an occupied circular buffer queue element.

An exemplary memory organization for 240 bytes payload and 32 elements (= two sectors) shows the figure below:
<center><img src="/doc/readme/sfcb_mem_org_cb0.svg" height="75%" width="75%" alt="Example circular buffer queue 0 FLASH memory organization" title="Exemplary circular buffer memory organization" /></center>
<br/>


## References
* [W25Q16JV](https://www.winbond.com/hq/support/documentation/downloadV2022.jsp?__locale=en&xmlPath=/support/resources/.content/item/DA00-W25Q16JV_1.html&level=1)
* [Siemens Open Source Manifesto](https://blog.siemens.com/2023/05/open-source-manifesto/)
