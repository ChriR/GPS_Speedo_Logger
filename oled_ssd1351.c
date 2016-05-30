/*
 * oled_ssd1351.c
 *
 *  Created on: 17.03.2016
 *      Author: Christoph Ringl
 */


#include "oled_ssd1351.h"
#include "spi_wrapper.h"
#include "oled_fontTables.h"

#include <stdint.h>
#include <stdbool.h>

#include "inc/tm4c1294ncpdt.h"
#include "driverlib/gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
//#include "inc/hw_ssi.h"


uint32_t oled_g_ui32SysClock;
uint8_t oledSPIProcess;


/* Initialisation sequence for OLED */
void oled_initSequence(void);

//Set memory area(address) to write the following display data to
void oled_setRAM(uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2);

/* SPI wrapper access functions */
inline void spiWrite(uint8_t data);
inline void spiWrite16(uint16_t data);
inline void spiWrite24(uint32_t data);

/* functions for writing commands and data to oled */
void writeData(uint8_t d);
void writeData16(uint16_t d);
void writeCommand(uint8_t c);
void writeCommand8(uint8_t command, uint8_t data1);
void writeCommand16(uint8_t command, uint8_t data1, uint8_t data2);
void writeCommand24(uint8_t command, uint8_t data1, uint8_t data2, uint8_t data3);


void oledCS_assert(void)
{
	clearOLEDCS();
}

void oledCS_deassert(void)
{
	setOLEDCS();
}

/*
 * Initialise OLED and underlaying SPI
 * _g_ui32SysClock is the system clock frequency in Hz.
 */
void oled_init(uint32_t _g_ui32SysClock)
{
	oled_g_ui32SysClock = _g_ui32SysClock;

#ifndef OLED_OFF
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);

    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_4|GPIO_PIN_7);

	GPIOPadConfigSet(GPIO_PORTK_BASE, GPIO_PIN_0, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);
    GPIOPadConfigSet(GPIO_PORTK_BASE, GPIO_PIN_4, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);
	GPIOPadConfigSet(GPIO_PORTK_BASE, GPIO_PIN_7, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);

	setOLEDRST();
	clearOLEDCS();
	setOLEDDC();

	oledSPIProcess = SPI_registerProc(oledCS_assert, oledCS_deassert);
	
	oled_initSequence();
#endif
}

/*
 * Initialisation sequence for OLED
 */
void oled_initSequence(void)
{

	setOLEDRST();
	SysCtlDelay(oled_g_ui32SysClock / 3 / 10 * 5);		/* 500 ms */
	clearOLEDRST();
	SysCtlDelay(oled_g_ui32SysClock / 3 / 10 * 5);		/* 500 ms */
	setOLEDRST();
	SysCtlDelay(oled_g_ui32SysClock / 3 / 10 * 5);		/* 500 ms */

	/* Initialization Sequence */
	
	writeCommand8(SSD1351_CMD_COMMANDLOCK, 0x12);	/* set command lock */
	writeCommand8(SSD1351_CMD_COMMANDLOCK, 0xB1);
	
	writeCommand(SSD1351_CMD_DISPLAYOFF);
	
	
	writeCommand8(SSD1351_CMD_CLOCKDIV, 0xF1);		/* 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16) */

	writeCommand8(SSD1351_CMD_MUXRATIO, 127);
		
	writeCommand8(SSD1351_CMD_SETREMAP, 0x74);
			
	writeCommand16(SSD1351_CMD_SETCOLUMN, 0x00, 0x7F);
	writeCommand16(SSD1351_CMD_SETROW, 0x00, 0x7F);

	if (SSD1351HEIGHT == 96) {
		writeCommand8(SSD1351_CMD_STARTLINE, 96);
	} else {
		writeCommand8(SSD1351_CMD_STARTLINE, 0);
	}

	writeCommand8(SSD1351_CMD_DISPLAYOFFSET, 0x00);

	writeCommand8(SSD1351_CMD_SETGPIO, 0x00);

	writeCommand8(SSD1351_CMD_FUNCTIONSELECT, 0x01);/* internal (diode drop) */

	writeCommand8(SSD1351_CMD_PRECHARGE, 0x32);

	writeCommand8(SSD1351_CMD_VCOMH, 0x05);

	writeCommand(SSD1351_CMD_NORMALDISPLAY);

	writeCommand24(SSD1351_CMD_CONTRASTABC, 0xC8, 0x80, 0xC8);

	writeCommand8(SSD1351_CMD_CONTRASTMASTER, 0x0F);

	writeCommand24(SSD1351_CMD_SETVSL, 0xA0, 0xB5, 0x55);

	writeCommand8(SSD1351_CMD_PRECHARGE2, 0x01);

	writeCommand(SSD1351_CMD_DISPLAYON);			/* turn on oled panel */
}

void oled_setContrast(uint8_t contrast)
{
	contrast &= 0x0F;

	writeCommand8(SSD1351_CMD_CONTRASTMASTER, contrast);
}
void oled_sleep(void)
{
	writeCommand(SSD1351_CMD_DISPLAYOFF);
}
void oled_wake(void)
{
	writeCommand(SSD1351_CMD_DISPLAYON);
}


/*
 * Convert 8-bit RGB color to OLED color
 * rrrrrggg gggbbbbb
 */
uint16_t Color565(uint8_t r, uint8_t g, uint8_t b)
{
	uint16_t c;
	c = r >> 3;
	c <<= 6;
	c |= g >> 2;
	c <<= 5;
	c |= b >> 3;

	return c;
}


/*
 * Set memory area(address) to write the following display data to
 */
void oled_setRAM(uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2)
{
	writeCommand16(SSD1351_CMD_SETCOLUMN, x1, x2);
	writeCommand16(SSD1351_CMD_SETROW, y1, y2);
	
	writeCommand(SSD1351_CMD_WRITERAM);
}


/* =================  D R A W I N G  =================== */
/*
 * Draw a single pixel
 */
void oled_drawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	/* Bounds check */
	if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT)) return;

	oled_setRAM(x,x,y,y);

	writeData16(color);
}

/*
 * Draw a filled rectange
 */
void oled_fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	uint16_t i;
	/* Bounds check */
	if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT))
	return;

	/* Y bounds check */
	if (y+h > SSD1351HEIGHT)
	{
		h = SSD1351HEIGHT - y - 1;
	}

	/* X bounds check */
	if (x+w > SSD1351WIDTH)
	{
		w = SSD1351WIDTH - x - 1;
	}

	/* set location */
	oled_setRAM(x, x+w-1, y, y+h-1);

	for (i=0; i < w*h; i++) {
		writeData16(color);
	}
}

/*
 * Draw a horizontal line
 */
void oled_drawHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color)
{
	uint16_t i;

	/* Bounds check */
	if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT))
		return;

	/* X bounds check */
	if (x+w > SSD1351WIDTH)
	{
		w = SSD1351WIDTH - x - 1;
	}

	/* set location */
	oled_setRAM(x, x+w-1, y, y);

	for (i=0; i < w; i++) {
		writeData16(color);
	}
}

/*
 * Draw a vertical line
 */
void oled_drawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color)
{
	uint16_t i;
	/* Bounds check */
	if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT))
		return;

	/* Y bounds check */
	if (y+h > SSD1351HEIGHT)
	{
		h = SSD1351HEIGHT - y - 1;
	}

	/* set location */
	oled_setRAM(x, x, y, y+h-1);

	for (i=0; i < h; i++) {
		writeData16(color);
	}
}

/*
 * Fill the entire screen with a given color
 */
void oled_fillScreen(uint16_t fillcolor)
{
	oled_fillRect(0, 0, SSD1351WIDTH, SSD1351HEIGHT, fillcolor);
}

/*
 * Invert the display in terms of color
 */
void oled_invert(bool v)
{
	if (v) {
		writeCommand(SSD1351_CMD_INVERTDISPLAY);
	} else {
		writeCommand(SSD1351_CMD_NORMALDISPLAY);
	}
}

/* =================  T E X T  =================== */

/*
 * Write text with small font to oled.
 * Text must end with \0
 * forecolor is color of text.
 * backcolor is color of text background and space between text.
 * x and y are postion of upper left corner of text.
 */
void oled_drawtext(char * data, uint16_t forecolor, uint16_t backcolor, uint8_t x, uint8_t y)
{
	uint8_t cnt=0;
	uint8_t c;
	uint8_t z, s;
	uint8_t rowmask;

 	while(*(data+cnt)!=0)
	{cnt++;}

    oled_setRAM(x,x+(cnt*6-1), y, y+8-1); /* for left alignment */
    /*oled_setRAM(SSD1351WIDTH-x-(cnt*6-1),SSD1351WIDTH-x, y, y+8-1); */ /* for right alignment */

	for(z=0;z<8;z++)						/* row (zeile, z) */
	{
		rowmask = (1<<z);
		for(c=0;c<cnt;c++)					/* character count (c) */
		{
			for(s=0;s<6;s++)				/* column in char (spalte, s) */
			{
				if (s==5)
				{
					writeData16(backcolor);	/* Space between characters */
				}
				else if (font7x5[data[c]-' '][s] & rowmask)
				{
					writeData16(forecolor);	/* character */
				}
				else
				{
					writeData16(backcolor);	/* character background */
				}
			}
		}
    }

}

/*
 * Write text with big font to oled.
 * Text must end with \0
 * forecolor is color of text.
 * backcolor is color of text background and space between text.
 * x and y are postion of upper left corner of text.
 */
void oled_drawtext_big(char * data, uint16_t forecolor, uint16_t backcolor, uint8_t x, uint8_t y)
{
	uint8_t cnt=0;
	uint8_t c;
	uint8_t z, s;
	uint16_t rowmask;

 	while(*(data+cnt)!=0)
	{cnt++;}

    oled_setRAM(x,x+(cnt*11-1), y, y+16-1); /* for left alignment */
    /*oled_setRAM(SSD1351WIDTH-x-(cnt*11-1),SSD1351WIDTH-x, y, y+16-1); */ /* for right alignment */

	for(z=0;z<16;z++)						/* row (zeile, z) */
	{
		rowmask = (1<<z);
		for(c=0;c<cnt;c++)					/* character count (c) */
		{
			for(s=0;s<11;s++)				/* column in char (spalte, s) */
			{
				if (s==10)
				{
					writeData16(backcolor);	/* Space between characters */
				}
				else if (font16x10[data[c]-' '][s] & rowmask)
				{
					writeData16(forecolor);	/* character */
				}
				else
				{
					writeData16(backcolor);	/* character background */
				}
			}
		}
    }

}

/* ===============  I C O N  ==================== */

void oled_drawIcon(const uint32_t * data, uint16_t forecolor, uint16_t backcolor, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	int8_t z, s;
	uint32_t tmp;

    oled_setRAM(x,x+w-1, y, y+h-1); 	/* for left alignment */

    for(z=0;z<h;z++)
    {
    	tmp = *(data+z);
    	for(s=w-1;s>=0;s--)
    	{
    		if(tmp & (uint32_t)(1<<s))
    		{
    			writeData16(forecolor);	/* symbol */
			}
			else
			{
				writeData16(backcolor);	/* symbol background */
			}
    	}
    }

}

/* =================================================== */

void reset(void)
{

}

/*
 * Write 8 bit of data to the oled
 */
void writeData(uint8_t d)
{
#ifndef OLED_OFF
	setOLEDDC();
	spiWrite(d);
#endif
}

/*
 * Write 16 bit of data to the oled
 */
void writeData16(uint16_t d)
{
#ifndef OLED_OFF
	setOLEDDC();
	spiWrite16(d);
#endif
}

/*
 * Write a single 8-bit command to the oled
 */
void writeCommand(uint8_t c)
{
#ifndef OLED_OFF
	clearOLEDDC();
	spiWrite(c);
#endif
}

/*
 * Write a 8-bit command plus 8-bit data to the oled.
 */
void writeCommand8(uint8_t command, uint8_t data)
{
#ifndef OLED_OFF
	clearOLEDDC();
	spiWrite(command);
	setOLEDDC();
	spiWrite(data);
#endif
}

/*
 * Write a 8-bit command plus 2 x 8-bit data to the oled.
 */
void writeCommand16(uint8_t command, uint8_t data1, uint8_t data2)
{
#ifndef OLED_OFF
	clearOLEDDC();
	spiWrite(command);
	setOLEDDC();
	spiWrite16((uint16_t)data1<<8|data2);
#endif
}

/*
 * Write a 8-bit command plus 3 x 8-bit data to the oled.
 */
void writeCommand24(uint8_t command, uint8_t data1, uint8_t data2, uint8_t data3)
{
#ifndef OLED_OFF
	clearOLEDDC();
	spiWrite(command);
	setOLEDDC();
	spiWrite24((uint32_t)data1<<16|data2<<8|data3);
#endif
}

/*  SPI wrapper access functions */
inline void spiWrite(uint8_t data)
{
	SPI_Put8(oledSPIProcess, data);
}
inline void spiWrite16(uint16_t data)
{
	SPI_Put16(oledSPIProcess, (uint8_t)(data>>8), (uint8_t)data);
}
inline void spiWrite24(uint32_t data)
{
	SPI_Put24(oledSPIProcess, (uint8_t)(data>>16), (uint8_t)(data>>8), (uint8_t)data);
}
