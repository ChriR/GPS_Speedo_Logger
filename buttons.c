/*
 * buttons.c
 *
 *  Created on: 03.05.2015
 *      Author: Christoph Ringl
 */

#include <stdint.h>
#include <stdbool.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include "driverlib/interrupt.h"
#include <inc/hw_memmap.h>

#include "buttons.h"

volatile uint8_t keyState;			/* debounced and inverted key state: bit = 1: key pressed */
volatile uint8_t keyPress;			/* key press detect */
volatile uint8_t keyRpt;			/* key long press and repeat */

/* Initialises key/button debounce functionality. */
void Key_init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
	GPIOPinTypeGPIOInput(GPIO_PORTL_BASE, GPIO_PIN_3|GPIO_PIN_2|GPIO_PIN_1|GPIO_PIN_0);

	GPIOIntUnregister(GPIO_PORTL_BASE);

	GPIOPadConfigSet(GPIO_PORTL_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
	GPIOPadConfigSet(GPIO_PORTL_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
	GPIOPadConfigSet(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
	GPIOPadConfigSet(GPIO_PORTL_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

/* Hardware abstraction function */
static inline uint8_t getKeyState(void)
{
	return GPIOPinRead(GPIO_PORTL_BASE, GPIO_PIN_3|GPIO_PIN_2|GPIO_PIN_1|GPIO_PIN_0) & 0x0F;
}

static inline void intMasterOff(void)
{
	IntMasterDisable();
}

static inline void intMasterOn(void)
{
	IntMasterEnable();
}

/* ---=== no hardware specific functions from below on ===--- */

/* Debounces key inputs. Must be called periodically (e.g. every 10 ms). */
void key_debounce(void)
{
	static uint8_t ct0, ct1, rpt;
	uint8_t i;

	i = keyState ^ ~getKeyState();		/* key changed ? */
	ct0 = ~( ct0 & i );					/* reset or count ct0 */
	ct1 = ct0 ^ (ct1 & i);				/* reset or count ct1 */
	i &= ct0 & ct1;						/* count until roll over ? */
	keyState ^= i;						/* then toggle debounced state */
	keyPress |= keyState & i;			/* 0->1: key press detect */

	if( (keyState & 0x0F) == 0 )		/* check repeat function */
		rpt = REPEAT_START;				/* start delay */
	if( --rpt == 0 ){
	    rpt = REPEAT_NEXT;				/* repeat delay */
	    keyRpt |= keyState;
	}
}

uint8_t Key_getState(uint8_t keyMask)
{
	return keyMask & (keyPress | keyRpt);
}
void Key_clear(uint8_t keyMask)
{
	intMasterOff();
	keyPress &= !keyMask;
	keyRpt &= !keyMask;
	intMasterOn();
}

/* Returns 1 on the specific bit position, if key is pressed at the moment */
uint8_t Key_getPress(uint8_t keyMask)
{
	intMasterOff();						/* read and clear atomic ! */
	keyMask &= keyPress;				/* read key(s) */
	keyPress ^= keyMask;				/* clear key(s) */
	intMasterOn();
	return keyMask;
}

/* Returns 1 on the specific bit position, if key is long-pressed every time the repeat interval overflows */
uint8_t Key_getRpt(uint8_t keyMask)
{
	intMasterOff();						/* read and clear atomic ! */
	keyMask &= keyRpt;					// read key(s) */
	keyRpt ^= keyMask;					// clear key(s) */
	intMasterOn();
	return keyMask;
}

/* Returns 1 on the specific bit position, if key has been pressed short */
uint8_t Key_getShort(uint8_t keyMask)
{
	intMasterOff();						/* read and clear atomic ! */
	return Key_getPress(~keyState & keyMask);
}

/* Returns 1 on the specific bit position, if key has been pressed long */
uint8_t Key_getLong(uint8_t keyMask)
{
	return Key_getPress( Key_getRpt(keyMask) );
}

