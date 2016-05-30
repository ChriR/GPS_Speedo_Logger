/*
 * spi_wrapper.h
 *
 *  Created on: 06.05.2015
 *      Author: Christoph Ringl
 *
 *       Brief: spi_wrapper.h provides wrapping and abstraction functionality for the TIVA SSI interface.
 *       		Processes can be registered in a way that each process asserts and deasserts its own chip
 *       		select lines so that multiple process can access multiple devices on the SPI bus.
 */

#ifndef SPI_WRAPPER_H_
#define SPI_WRAPPER_H_

#include <stdint.h>
#include <stdbool.h>

#include "oled_ssd1351.h"
#define clearCS()		clearOLEDCS()
#define setCS()			setOLEDCS()

#define SPI_MAXPROC		10

#define SPI_CLKSLOW		(2*1e5)


/* Initialise SPI (SSI3) with given clock frequency and data bit length. */
void SPI_init(uint32_t g_ui32SysClock, uint32_t freq, uint8_t data_length);

/* Sets SPI bus speed to value set in SPI_init(). */
void SPI_setSlow(void);

/* Sets SPI bus speed to value set in SPI_CLKSLOW. */
void SPI_setFast(void);

/* Reserves SPI ressource so that access through this module is inhibitat for other processes. */
void SPI_reserve(uint8_t process);

/* Releases SPI ressource so that SPI can be used by other processes again. */
void SPI_release(uint8_t process);

/* Registers a process with CS assert and deassert functions and returns the assigned process number. */
uint8_t SPI_registerProc(void (* pfnAssertCS)(void), void (* pfnDeAssertCS)(void));

/* Put n bytes of data in the transmit buffer and start transmission. */
void SPI_Put(uint8_t process, uint8_t * data, uint8_t cnt);

/* Put 1 byte of data in the transmit buffer and start transmission. */
void SPI_Put8(uint8_t process, uint8_t data);

/* Put 2 bytes of data in the transmit buffer and start transmission. */
void SPI_Put16(uint8_t process, uint8_t data1, uint8_t data2);

/* Put 3 bytes of data in the transmit buffer and start transmission. */
void SPI_Put24(uint8_t process, uint8_t data1, uint8_t data2, uint8_t data3);

/* Asserts the Chip Select line and reserves the SPI ressource. */
void SPI_SelectCS(uint8_t process);

/* Deasserts the Chip Select line and releases the SPI ressource. */
void SPI_DeselectCS(uint8_t process);

/* Transmits and receives 1 byte of data. */
uint8_t SPI_xchg(uint8_t process, uint8_t data);

/* Transmits an arbitrary amount of data bytes through SPI. */
void SPI_xmit(uint8_t process, uint32_t cnt, const uint8_t * buffXmit);

/* Receives an arbitrary amount of data bytes through SPI. */
void SPI_rcv(uint8_t process, uint32_t cnt, uint8_t * buffRcv);

#endif /* SPI_WRAPPER_H_ */
