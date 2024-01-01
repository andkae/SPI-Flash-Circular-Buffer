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
tbd.


## Memory organisation

<br/>
<center><img src="/doc/readme/sfcb_mem_org_cb0.svg" height="75%" width="75%" alt="Example circular buffer queue 0 FLASH memory organization" title="Exemplary circular buffer memory organization" /></center>
<br/>


## References
* [W25Q16JV](https://www.winbond.com/hq/support/documentation/downloadV2022.jsp?__locale=en&xmlPath=/support/resources/.content/item/DA00-W25Q16JV_1.html&level=1)
* [Siemens Open Source Manifesto: Our Commitment to the Open Source Ecosystem](https://blog.siemens.com/2023/05/open-source-manifesto/)
