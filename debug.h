/*
 * debug.h
 *
 *  Created on: 22.03.2016
 *      Author: Christoph Ringl
 *
 * 		 Brief: debug.h provides basic debugging functionality: Strings can be printed
 *				and execution run time can be measured and printed.
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inc/hw_memmap.h>

//#define DEBUG_OFF
//#define DEBUG_TIMEROFF

#define TMRBASE		TIMER1_BASE
#define TMRPERIPH	SYSCTL_PERIPH_TIMER1

/* Initialises the debug unit. */
void debug_init(uint32_t debug_g_ui32SysClock);

/* Sets a function to be called when a character was received over debug interface. */
void debug_RXHook(void (* pfnRXHook)(char c));

/* Prints data to the debug interface. */
void debug_print(char * str);

/* Checks the debug RX buffer for received data. Must be called periodically. */
void debug_checkRX(void);

/* Starts the measurement of execution runtime. */
void debug_startMeas(void);

/* Stops the measurement of execution runtime and returns the runtime in µs. */
uint32_t debug_getMeas(void);

/* Prints the measured execution runtime (cnt) or, if cnt==0, calls debug_getMeas() internally. */
void debug_printMeas(uint32_t cnt);


#endif /* DEBUG_H_ */
