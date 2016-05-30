/*
 * display.h
 *
 *  Created on: 19.03.2016
 *      Author: Christoph Ringl
 *
 *       Brief: display.h provides an abstraction layer for oled_ssd1351.h to display GPS specific
 *  			data. Content is organized in pages
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_


//#include "oled_ssd1351.h"
#include <stdint.h>
#include <stdbool.h>

/* Bitmasks for Page 1 - Page 3 */
#define GP_P1_bm		(1<<0)
#define GP_P2_bm		(1<<1)
#define GP_P3_bm		(1<<2)
#define GP_P4_bm		(1<<3)

/* Status Line */
#define GP_SDX			106
#define GP_SDY			0
#define GP_SDPAGE		GP_P1_bm|GP_P2_bm|GP_P3_bm|GP_P4_bm

#define GP_GPSX			78
#define GP_GPSY			0
#define GP_GPSPAGE		GP_P1_bm|GP_P2_bm|GP_P3_bm|GP_P4_bm
#define GP_DOPTHRESHR	60
#define GP_DOPTHRESHY	25

#define GP_BATTX		56
#define GP_BATTY		0
#define GP_BATTPAGE		GP_P1_bm|GP_P2_bm|GP_P3_bm|GP_P4_bm

#define GP_TIMEX		0
#define GP_TIMEY		1
#define GP_TIMEPAGE		GP_P1_bm|GP_P2_bm|GP_P3_bm|GP_P4_bm

/* PAGE 0 */
#define GP_STPWX		2
#define GP_STPWY		13
#define GP_STPWPAGE		GP_P1_bm

#define GP_ALTX			2
#define GP_ALTY			(GP_STPWY+27)
#define GP_ALTPAGE		GP_P1_bm

#define GP_SPDX			2
#define GP_SPDY			(GP_ALTY+27)
#define GP_SPDPAGE		GP_P1_bm

#define GP_DISTX		2
#define GP_DISTY		(GP_SPDY+27)
#define GP_DISTPAGE		GP_P1_bm

/* PAGE 1 */
#define GP_TTFFX		2				/* Time to first fix */
#define GP_TTFFY		13
#define GP_TTFFPAGE		GP_P2_bm

#define GP_TSRX			2				/* Time since reset */
#define GP_TSRY			(GP_TTFFY+19)
#define GP_TSRPAGE		GP_P2_bm

#define GP_SATX			2				/* Basic Satellite information */
#define GP_SATY			(GP_TSRY+19)
#define GP_SATPAGE		GP_P2_bm

#define GP_FIXX			2				/* Basic Satellite information */
#define GP_FIXY			(GP_SATY+19)
#define GP_FIXPAGE		GP_P2_bm

#define GP_LLX			2				/* Latitude/Longitude Position information */
#define GP_LLY			(GP_FIXY+19)
#define GP_LLPAGE		GP_P2_bm

/* PAGE 2 */
#define GP_SATOVX		0				/* Satellites overview */
#define GP_SATOVY		13
#define GP_SATOVPAGE	GP_P3_bm

/* PAGE 3 */

#define GP_LOGSETX		2				/* log settings */
#define GP_LOGSETY		13
#define GP_GPSSETX		2				/* uart settings */
#define GP_GPSSETY		(GP_LOGSETY+27)
#define GP_DISPSETX		2				/* display settings */
#define GP_DISPSETY		(GP_GPSSETY+36)
#define GP_DISPCONFX	2
#define GP_DISPCONFY	(GP_DISPSETY+18)

#define GP_LASTLOOPPAGE	2
#define GP_LASTPAGE		3

typedef enum {sBack, sSelection, sRed, sGreen} eselcolor;	/* names of the predefined selection colors */


/* Initialises display functions, includes oled and SPI initialisation. */
void display_init(uint32_t _g_ui32SysClock);

/* Initialises the display content after system start and fill with content if selected page. */
void display_initContent(void);

/* Sets the page to be displayed and perform static page drawing. */
void display_setPage(uint8_t Page);

/* Must be called in the desired slow blink frequency. */
void display_blink_slow(void);

/* Must be called in the desired fast blink frequency. */
void display_blink_fast(void);


/* ---=== PAGE 0 ===--- */
/* Draws the stopwatch text on the display (sMs = tenth of seconds). */
void display_Stpw(uint8_t sHr, uint8_t sMin, uint8_t sSec, uint8_t sMs);
/* Draws the altitude and the altitude upwards/downwards made good on the display. */
void display_Alt(int32_t Alt, uint32_t AltUp, uint32_t AltDwn);
/* Draws the current, average and maximum speed in km/h on the display. */
void display_Speed(uint16_t Speed, uint16_t Avg, uint16_t Max);
/* Draws the distance made good on the display. */
void display_Dist(int32_t Dist);

/* ---=== PAGE 1 ===--- */
/* Draws the Time since reset on the display. */
void display_Tsr(uint32_t d, uint8_t h, uint8_t m, uint8_t s);
/* Draws general satellite and fix information on the display. */
void display_Satinfo(uint8_t satView, uint8_t satFix, uint8_t PDOP, uint8_t HDOP, uint8_t VDOP);
/* Draws GPS fix information on the display. */
void display_Fixinfo(uint8_t GPSFixType, uint8_t GPSFixQuality);
/* Draws the latitude and longitude position. LxxI:integer part of coord., LxxF:fractional part, NS/EW:North-South/East-West char. */
void display_LatLon(uint8_t LatI, uint32_t LatF, char NS, uint8_t LonI, uint32_t LonF, char EW);
/* Draws the time to first fix. */
void display_TTFF(uint32_t seconds);

/* ---=== PAGE 2 ===--- */
/* Draws a table with an overview of satellite IDs, IDs used in fix and SNR data. */
void display_Satov(uint8_t satViewNum, uint8_t satFixNum, uint8_t * satsView, uint8_t * satsViewSNR, uint8_t * satsFix);

/* ---=== Status Line information ===--- */
/* Prints the time on the display. */ 
void display_Time(uint8_t hr, uint8_t min, uint8_t sec);

/* Prints the GPS satellite symbol and the number of satellites on the display. */
void display_GPS(uint8_t satellites, uint8_t fix, uint8_t DOP);
/* Display remaining battery capacity. */
void display_Battery(uint8_t capacity);
/* Displays the SD card icon, remaining SD capacity and the record state. */
void display_SD(bool rec, uint8_t capacity);
/* Draw the icon for indicating, that no SD card is present. */
void display_SDMissing(void);

/* Displays he current configuration */
void display_conf(int8_t selected, eselcolor selColor);


#endif /* DISPLAY_H_ */
