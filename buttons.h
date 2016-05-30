/*
 * buttons.h
 *
 *  Created on: 03.05.2015
 *      Author: Christoph Ringl
 *
 *       Brief: buttons.h provides key, button or switch debouncing rountines.
 *  			No hardware abstration layer is included so adjusting to different
 *  			keys must be done in buttons.c
 */

#ifndef BUTTONS_H_
#define BUTTONS_H_


// Key Handling
#define REPEAT_START    100			// after 100x key_debounce()
#define REPEAT_NEXT     20			// every 20x key_debounce()

/* Initialises key/button debounce functionality. */
void Key_init(void);

/* Debounces key inputs. Must be called periodically (e.g. every 10 ms). */
void key_debounce(void);

uint8_t Key_getState(uint8_t keyMask);
void Key_clear(uint8_t keyMask);

/* Returns 1 on the specific bit position, if key is pressed at the moment */
uint8_t Key_getPress(uint8_t keyMask);

/* Returns 1 on the specific bit position, if key is long-pressed every time the repeat interval overflows */
uint8_t Key_getRpt(uint8_t keyMask);

/* Returns 1 on the specific bit position, if key has been pressed short */
uint8_t Key_getShort(uint8_t keyMask);

/* Returns 1 on the specific bit position, if key has been pressed long */
uint8_t Key_getLong(uint8_t keyMask);

#endif /* BUTTONS_H_ */
