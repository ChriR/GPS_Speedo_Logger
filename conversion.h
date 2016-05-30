/*
 * conversion.h
 *
 * Created: 22.08.2015 21:24:02
 *  Author: Christoph Ringl
 *
 *   Brief:	conversion.h provides basic conversion mechanism to convert between integers and strings.
 */ 


#ifndef CONVERSION_H_
#define CONVERSION_H_

#include <stdbool.h>
#include <stdint.h>

#define uint16Integer(number)	(number/256)
#define uint16Fractal(number)	(((uint16_t)((uint8_t)(number))*100)/256)

/* Convert signed, 8 bit integer + 8 bit fractal to ascii. */
char * s8p8ToA(int16_t number, char * buffer);

/* Convert 8 bit unsigned to ascii. */
char * ui8ToA(uint8_t number, char * buffer, uint8_t digits);

/* Convert 16 bit unsigned to ascii. */
char * ui16ToA(uint16_t number, char * buffer, uint8_t digits, bool zero2white);

/* Convert 32 bit unsigned to ascii. */
char * ui32ToA(uint32_t number, char * buffer, uint8_t digits);

/* Convert 32 bit unsigned to hex ascii. */
char * ui32ToX(uint32_t value, char * buffer, uint8_t digits);

/* Converts a 1 - 10 digit unsigned string to a unsigned 32-bit integer. */
uint32_t axp1ToUi32(char * str, uint32_t def);

/* Convert string to bool. */
bool aToBool(char * str, bool def);

#endif /* CONVERSION_H_ */
