/*
 * conversion.c
 *
 * Created: 22.08.2015 21:23:52
 *  Author: Christoph RIngl
 */ 

#include <stdbool.h>
#include <stdint.h>

#include "conversion.h"

/* Convert signed, 8 bit integer + 8 bit fractal to ascii. */
char * s8p8ToA(int16_t number, char * buffer)
{
	char tmp[3];
	if(number<0)
	{
		buffer[0] = '-';
		number = -number;
	} else
	buffer[0] = '+';
	
	ui8ToA(uint16Integer(number), tmp, 3);
	buffer[1] = tmp[0];
	buffer[2] = tmp[1];
	buffer[3] = tmp[2];
	buffer[4] = '.';
	ui8ToA(uint16Fractal(number), tmp, 2);
	buffer[5] = tmp[0];
	buffer[6] = tmp[1];
	buffer[7] = 0;
	
	return buffer;
}

/* Convert 8 bit unsigned to ascii. */
char * ui8ToA(uint8_t number, char * buffer, uint8_t digits)
{
	int8_t i;
	
	for (i=(digits-1); i>=0; i--)
	{
		buffer[i] = (number % 10) + '0';
		number /= 10;
	}
	buffer[digits] = 0;
	
	return buffer;
}

/* Convert 16 bit unsigned to ascii. */
char * ui16ToA(uint16_t number, char * buffer, uint8_t digits, bool zero2white)
{
	int8_t i;
	if(digits==0)
	{
		buffer[0] = '0';
		buffer[1] = 0;
		return buffer;
	}
	if(digits>5) digits = 5;
	
	for (i=(digits-1); i>=0; i--)
	{
		buffer[i] = (number % 10) + '0';
		number /= 10;
	}
	buffer[digits] = 0;
	
	if(zero2white){
		for(i=0; i<(digits-1); i++)
		{
			if(buffer[i]=='0') buffer[i]=' ';
			else break;
		}
	}

	return buffer;
}

/* Convert 32 bit unsigned to ascii. */
char * ui32ToA(uint32_t number, char * buffer, uint8_t digits)
{
	int8_t i;
	if(digits==0)
	{
		buffer[0] = '0';
		buffer[1] = 0;
		return buffer;
	}
	if(digits>9) digits = 9;
	
	for (i=(digits-1); i>=0; i--)
	{
		buffer[i] = (number % 10) + '0';
		number /= 10;
	}
	buffer[digits] = 0;
	
	/*if(zero2white){
		for(i=0; i<(digits-1); i++)
		{
			if(buffer[i]=='0') buffer[i]=' ';
			else break;
		}
	}*/

	return buffer;
}

/* Convert 32 bit unsigned to hex ascii. */
char * ui32ToX(uint32_t value, char * buffer, uint8_t digits)
{
	int8_t i;
	if(digits==0)
	{
		buffer[0] = '0';
		buffer[1] = 0;
		return buffer;
	}
	if(digits>8) digits = 8;

	for (i=(digits-1); i>=0; i--)
	{
		buffer[i] = (value & 0x0F) + '0';
		if(buffer[i]>'9') buffer[i] += 'A'-'9';
		value >>= 4;
	}

	buffer[digits] = 0;

	return buffer;
}


/* 
 * Converts a 1 - 10 digit unsigned string to a unsigned 32-bit integer.
 * Returns number*10.
 * Format:	##0.#
 */
uint32_t axp1ToUi32(char * str, uint32_t def)
{
	uint32_t retval=0;
	
	/* skip whitespaces */
	while(*str==' ' || *str=='\t') str++;
	
	/* check fist digit */
	if(*str<'0' || *str>'9') return def;
	
	do
	{
		retval *= 10;
		retval += (*str - '0');
		str++;
	} while (*str>='0' && *str<='9');
	
	retval *= 10;
	
	if(*str=='.')
	{
		str++;
		if(*str>='0' || *str<='9')
		{
			retval += (*str - '0');
			str++;
		}
		else return def;
	}
	
	if(*str==' ' || *str=='\t' || *str=='\r' || *str=='n' || *str==0) return retval;
	else return def;
}

/* Convert string to bool. */
bool aToBool(char * str, bool def)
{
	/* skip whitespaces */
	while(*str==' ' || *str=='\t') str++;
	
	/* check fist digit */
	if(*str=='0' || *str=='f' || *str=='F') return false;
	if((*str>='1' && *str<='9') || *str=='t' || *str=='T') return true;

	return def;
}
