[![Test](https://github.com/andkae/SPI-Flash-Circular-Buffer/actions/workflows/test.yml/badge.svg)](https://github.com/andkae/SPI-Flash-Circular-Buffer/actions/workflows/test.yml)


# SPI Flash Circular Buffer
C Library to transform a physical SPI Flash into a logical circular buffer. The interface between _[SFCB](https://github.com/andkae/SPI-Flash-Circular-Buffer)_
and SPI core is realized as shared memory.


## Features
* Arbitrary SPI [Flash](https://github.com/andkae/SPI-Flash-Circular-Buffer/blob/main/sfcb_flash_types.h) support
* Arbitary number of circular buffers in one SPI flash
* Interaction between circular buffer and SPI interface is ealized as shared memory


## Releases
| Version                                                | Date       | Source                                                                                                                 | Change log                                                                                                                                         |
| ------------------------------------------------------ | ---------- | ---------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| latest                                                 |            | <a id="raw-url" href="https://github.com/andkae/SPI-Flash-Circular-Buffer/archive/refs/heads/main.zip">latest.zip</a>  |                                                                                                                                                    |


## How to use


## [API](./spi_flash_cb.h)

### Init
```c
int sfcb_init (t_sfcb *self, void *cb, uint8_t cbLen, void *spi, uint16_t spiLen);
```

Initializes _SFCB_ common handle and assigns memory.

#### Arguments:
| Arg    | Description                       |
| ------ | --------------------------------- |
| self   | _SFCB_ storage element            |
| cb     | Circular buffer queue memory      |
| cbLen  | max number of _cb_ queues         |
| spi    | _SFCB_ / SPI core exchange buffer |
| spiLen | _spi_ buffer size in bytes        |

#### Return:
Success: *== 0*
Failure: *!= 0*






## Memory organisation
The [SFCB](https://github.com/andkae/SPI-Flash-Circular-Buffer) supports an arbitrary number of circular buffer queues.
Each circular buffer starts at the lowest free SPI Flash address. The Flash architecture requires an dedicated data clear -
so called _Sector Erase_. Through this limitation needs to be at least two sectors allocated. Otherwise would the overwrite
of the first written element result in an complete circular buffer queue overwrite without keeping any previous entries.
Every new entry is marked with the incremented highest 32bit _IdNum_ and _MagicNum_. The _MagicNum_ ensures the detection
of an occupied circular buffer queue element.

An exemplary memory organisation for 240 bytes payload and 32 elements (= two sectors) shows the figure below:
<center><img src="/doc/readme/sfcb_mem_org_cb0.svg" height="75%" width="75%" alt="Example circular buffer queue 0 FLASH memory organization" title="Exemplary circular buffer memory organization" /></center>
<br/>


## References
* [W25Q16JV](https://www.winbond.com/hq/support/documentation/downloadV2022.jsp?__locale=en&xmlPath=/support/resources/.content/item/DA00-W25Q16JV_1.html&level=1)
* [Siemens Open Source Manifesto](https://blog.siemens.com/2023/05/open-source-manifesto/)
