/*
 * oled_ssd1351.h
 *
 *  Created on: 17.03.2016
 *      Author: Christoph Ringl
 *
 *       Brief: oled_ssd1351.h provides a hardware abstraction and interface to SSD1351 controlled OLEDs.
 *  			Basic graphic functions are included like drawing horizontal and vertical lines,
 *  			filling rectangles and drawing text in two different font sizes. Also, symbols can be drawn
 *  			from a 32-bit integer array, thus symbols with up to 32 pixel width can be drawn.
 */

#ifndef OLED_SSD1351_H_
#define OLED_SSD1351_H_

#include <stdint.h>
#include <stdbool.h>

//#define OLED_OFF

#define SSD1351WIDTH 					128
#define SSD1351HEIGHT 					128

// Timing Delays
#define SSD1351_DELAYS_HWFILL	    	(3)
#define SSD1351_DELAYS_HWLINE       	(1)

// SSD1351 Commands
#define SSD1351_CMD_SETCOLUMN 			0x15
#define SSD1351_CMD_SETROW    			0x75
#define SSD1351_CMD_WRITERAM   			0x5C
#define SSD1351_CMD_READRAM   			0x5D
#define SSD1351_CMD_SETREMAP 			0xA0
#define SSD1351_CMD_STARTLINE 			0xA1
#define SSD1351_CMD_DISPLAYOFFSET 		0xA2
#define SSD1351_CMD_DISPLAYALLOFF 		0xA4
#define SSD1351_CMD_DISPLAYALLON  		0xA5
#define SSD1351_CMD_NORMALDISPLAY 		0xA6
#define SSD1351_CMD_INVERTDISPLAY 		0xA7
#define SSD1351_CMD_FUNCTIONSELECT 		0xAB
#define SSD1351_CMD_DISPLAYOFF 			0xAE
#define SSD1351_CMD_DISPLAYON     		0xAF
#define SSD1351_CMD_PRECHARGE 			0xB1
#define SSD1351_CMD_DISPLAYENHANCE		0xB2
#define SSD1351_CMD_CLOCKDIV 			0xB3
#define SSD1351_CMD_SETVSL 				0xB4
#define SSD1351_CMD_SETGPIO 			0xB5
#define SSD1351_CMD_PRECHARGE2 			0xB6
#define SSD1351_CMD_SETGRAY 			0xB8
#define SSD1351_CMD_USELUT				0xB9
#define SSD1351_CMD_PRECHARGELEVEL 		0xBB
#define SSD1351_CMD_VCOMH 				0xBE
#define SSD1351_CMD_CONTRASTABC			0xC1
#define SSD1351_CMD_CONTRASTMASTER		0xC7
#define SSD1351_CMD_MUXRATIO            0xCA
#define SSD1351_CMD_COMMANDLOCK         0xFD
#define SSD1351_CMD_HORIZSCROLL         0x96
#define SSD1351_CMD_STOPSCROLL          0x9E
#define SSD1351_CMD_STARTSCROLL         0x9F

#define setOLEDRST()		GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0, (1<<0))
#define clearOLEDRST()		GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0, 0)
#define setOLEDCS()			GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_4, (1<<4))
#define clearOLEDCS()		GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_4, 0)
#define setOLEDDC()			GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_7, (1<<7))	// data=1, command=0
#define clearOLEDDC()		GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_7, 0)


/*
 * Initialise OLED and underlaying SPI
 * _g_ui32SysClock is the system clock frequency in Hz.
 */
void oled_init(uint32_t _g_ui32SysClock);

void oled_setContrast(uint8_t contrast);
void oled_sleep(void);
void oled_wake(void);

/*
 * Reset the display
 * TODO : not implemented yet!
 */
void reset(void);

/*
 * Convert 8-bit RGB color to OLED color
 */
uint16_t Color565(uint8_t r, uint8_t g, uint8_t b);

/* =================  D R A W I N G  =================== */
/*
 * Draw a single pixel
 */
void oled_drawPixel(uint16_t x, uint16_t y, uint16_t color);

/*
 * Draw a filled rectange
 */
void oled_fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/*
 * Draw a horizontal line
 */
void  oled_drawHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color);

/*
 * Draw a vertical line
 */
void  oled_drawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color);

/*
 * Fill the entire screen with a given color
 */
void  oled_fillScreen(uint16_t fillcolor);

/*
 * Write text with small font to oled.
 * Text must end with \0
 * forecolor is color of text.
 * backcolor is color of text background and space between text.
 * x and y are postion of upper left corner of text.
 */
void oled_drawtext(char * data, uint16_t forecolor, uint16_t backcolor, uint8_t x, uint8_t y);

/*
 * Write text with big font to oled.
 * Text must end with \0
 * forecolor is color of text.
 * backcolor is color of text background and space between text.
 * x and y are postion of upper left corner of text.
 */
void oled_drawtext_big(char * data, uint16_t forecolor, uint16_t backcolor, uint8_t x, uint8_t y);

void oled_drawIcon(const uint32_t * data, uint16_t forecolor, uint16_t backcolor, uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/*
 * Invert the display in terms of color
 */
void oled_invert(bool v);



#endif /* OLED_SSD1351_H_ */
