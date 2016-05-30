/*
 * uart.h
 *
 *  Created on: 23.04.2015
 *      Author: Christoph Ringl
 *
 *  	 Brief: uart.h provides basic interrupt driven buffered UART functions. This include reading from
 *  			and writing to multiple UART modules.
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "inc/tm4c1294ncpdt.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"


#define UART_BUFF_LEN 	512		/* Buffer length for UART TX and RX */
#define UART_MAX		7		/* maximum number of UARTs that can be initialized */

typedef struct uart_str{
	uint8_t UARTNo;
	uint32_t ui32Base;
	bool initialized;

	char *w_buffer;
	uint16_t w_start;
	uint16_t w_end;

	char *r_buffer;
	uint16_t r_start;
	uint16_t r_end;
} uart_t;

 /* Initialises a specified UART Module including GPIO and interrupt init. 
  * Returns: UART Handler */
uint8_t UART_init(uint8_t UARTNo, uint32_t _g_ui32SysClock, uint32_t _ui32Baud, uint32_t _ui32Config);

/* Puts chars into the UART transmit buffer */
void UARTPut(uint8_t UART_handler, const char *pui8Buffer);

/* Sends data from the UART buffer */
bool UARTSend(uint8_t UART_handler);

/* Returns wheter unread data are in the UART receive buffer  */
bool UARTDataAvailable(uint8_t UART_handler);

/* Reads data from the UART receive buffer to buffer and returns the number of successfully read data */
uint8_t UARTGet(uint8_t UART_handler, char * buffer, uint8_t numToRead);

/* Convert a 16 bit unsigned integer to an ascii character array (string) */
char * int16ToA(uint16_t int16, char * buffer);
/* Convert a 8 bit unsigned integer to an ascii character array (string) */
char * int8ToA(uint8_t int8, char * buffer);
#endif /* UART_H_ */
