/*
 * debug.c
 *
 *  Created on: 22.03.2016
 *      Author: Christoph Ringl
 */


#include "debug.h"
#include "uart.h"

#include <driverlib/timer.h>
#include <driverlib/sysctl.h>
#include <driverlib/interrupt.h>
#include "conversion.h"
#include "inc/hw_types.h"
#include "inc/hw_timer.h"

uint8_t uart_debug;
char bufferMeas[10][10];

//volatile uint32_t tmpCnt = 0;
volatile uint32_t Cnt = 0;

void (* pfnRX)(char c);

/* The default RX handle. */
void debug_handleRX(char c);

/* Initialises timer functionality for measuring runtime. */
void debug_timer_init(uint32_t debug_g_ui32SysClock);

// void debugTimerIntHandler(void);


/* Initialises the debug unit. */
void debug_init(uint32_t debug_g_ui32SysClock)
{
#ifndef DEBUG_OFF
	pfnRX = debug_handleRX;

    /* Initialise UART for Debugging: 115200, 8N1*/
	uart_debug = UART_init(0, debug_g_ui32SysClock, 115200, (UART_CONFIG_WLEN_8|UART_CONFIG_STOP_ONE|UART_CONFIG_PAR_NONE));

	debug_timer_init(debug_g_ui32SysClock);
#endif
}

/* Sets a function to be called when a character was received over debug interface. */
void debug_RXHook(void (* pfnRXHook)(char c))
{
#ifndef DEBUG_OFF
	pfnRX = pfnRXHook;
#endif
}

/* Prints data to the debug interface. */
void debug_print(char * str)
{
#ifndef DEBUG_OFF
	UARTPut(uart_debug, str);
#endif
}

/* Checks the debug RX buffer for received data. Must be called periodically. */
void debug_checkRX(void)
{
#ifndef DEBUG_OFF
	char buffer[2];
	if(UARTDataAvailable(uart_debug))
	{
		uint8_t getCnt;

		getCnt = UARTGet(uart_debug, buffer, 1);
		buffer[1] = 0;
		if(getCnt==1)
		{
			pfnRX(buffer[0]);
		}
	}
#endif
}

/* Starts the measurement of execution runtime. */
void debug_startMeas(void)
{
#ifndef DEBUG_OFF
 #ifndef DEBUG_TIMEROFF
	//Cnt = tmpCnt;
	Cnt = HWREG(TMRBASE + TIMER_O_TAR); //TimerValueGet(TMRBASE, TIMER_A);
 #endif
#endif
}

/* Stops the measurement of execution runtime and returns the runtime in clock cycles. */
uint32_t debug_getMeas(void)
{
#ifndef DEBUG_OFF
 #ifndef DEBUG_TIMEROFF
	//return tmpCnt - Cnt;
	uint32_t tmp = HWREG(TMRBASE + TIMER_O_TAR); //TimerValueGet(TMRBASE, TIMER_A);
	return Cnt - tmp;
 #else
	return 0;
 #endif
#else
	return 0;
#endif
}

/* Prints the measured execution runtime (cnt) or, if cnt==0, calls debug_getMeas() internally. */
void debug_printMeas(uint32_t cnt)
{
#ifndef DEBUG_OFF
 #ifndef DEBUG_TIMEROFF
	static uint8_t j = 0;
	uint32_t _cnt = cnt==0 ? debug_getMeas() : cnt;

	ui32ToA(_cnt, &bufferMeas[j][0], 8);

	bufferMeas[j][7] = 0;	/* don't print last digit (hence number is divided by 10) */

	debug_print(&bufferMeas[j++][0]);

	if(j==10) j=0;
 #endif
#endif
}

/* ---=== PRIVAT ===--- */

/* Initialises timer functionality for measuring runtime. */
void debug_timer_init(uint32_t debug_g_ui32SysClock)
{
#ifndef DEBUG_OFF
 #ifndef DEBUG_TIMEROFF
	SysCtlPeripheralEnable(TMRPERIPH);

	/* Timer 0 konfigurieren */
	TimerConfigure(TMRBASE, TIMER_CFG_PERIODIC);
	//TimerLoadSet(TMRBASE, TIMER_A, debug_g_ui32SysClock/1e6);		/* run with 1 MHz */

	/* Timer einschalten */
	TimerEnable(TMRBASE, TIMER_A);
 #endif
#endif
}

/* The default RX handle. */
void debug_handleRX(char c)
{
#ifndef DEBUG_OFF
	/* default behaviour is loopback of received characters */
	char buffer[2];
	buffer[0] = c;
	buffer[1] = 0;
	UARTPut(uart_debug, buffer);
#endif
}
