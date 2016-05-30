/*
 * display.c
 *
 *  Created on: 19.03.2016
 *      Author: Christoph Ringl
 */

#include <stdint.h>
#include <stdbool.h>

#include "display.h"
#include "oled_ssd1351.h"
#include "conversion.h"
#include "config.h"

extern config_t conf;

/* ################### internal variables ################### */

uint16_t 	colors[8];										/* array with predefined colors. Can be accessed via colors[red] */
enum 		ecolor {black, white, red, green, blue, orange, yellow, gray};	/* names of the predefined colors */
uint16_t 	syscolors[4];									/* array with system colors. Can be accessed via syscolors[back] */
enum 		esyscolor {back, text, textstat, selection};	/* names of the system colors */
/* The color scheme can be changed by changing values in the syscolors array */

char 		buffer[10];			/* buffer for integer-string conversion */

uint8_t 	sats = 1;			/* internal buffered number of sats */
uint8_t 	gpsfix = 0;			/* GPS fix information: 1=no fix, 2=2D fix, 3=3D fix, others=no GPS module*/
uint8_t		dop = 99;			/* GPS DOP value for accuracy indication */
bool 		satblink = false;	/* true, if satellite icon should blink */
uint8_t 	bat = 1;			/* internal buffered remaining battery capacity [%] */
bool 		batblink = false;	/* true, if battery icon should blink */
uint8_t		sdCap = 1;			/* remaining SD card capacity */

uint8_t 	page = 0;			/* the selected page to be displayed */

/* ################### icons for drawing ################### */

const uint32_t icon_sd[] = {	/* SD card icon */
		0b0111110000,			/* .XXXXX.... */
		0b0101011000,			/* .X.X.XX... */
		0b0101010100,			/* .X.X.X.X.. */
		0b0100000010,			/* .X......X. */
		0b0100000010,			/* .X......X. */
		0b0100000010,			/* .X......X. */
		0b0100000010,			/* .X......X. */
		0b0100000010,			/* .X......X. */
		0b0100000010,			/* .X......X. */
		0b0111111110			/* .XXXXXXXX. */
};
const uint32_t icon_gps[] = {	/* GPS satellite icon */
		0b0100000000,
		0b1110000000,
		0b0111000000,
		0b0011011000,
		0b0000111000,
		0b0001110000,
		0b0001101100,
		0b0000001110,
		0b0000000111,
		0b0000000010
};
const uint32_t icon_batt[] = {	/* battery icon */
		0b11111111111100,
		0b10000000000100,
		0b10000000000100,
		0b10000000000111,
		0b10000000000111,
		0b10000000000111,
		0b10000000000111,
		0b10000000000100,
		0b10000000000100,
		0b11111111111100
};
const uint32_t icon_rec[] = {	/* Record icon (Circle) */
		0b0000110000,
		0b0011111100,
		0b0111111110,
		0b0111111110,
		0b1111111111,
		0b1111111111,
		0b0111111110,
		0b0111111110,
		0b0011111100,
		0b0000110000
};

/* ################### private function prototypes ################### */

/* Draw the static (non-changing) text and grahpics for the selected page. */
void display_drawStaticText(void);

/* Draws the battery icon. */
void draw_Battery(bool forceshow);
/* Hides the battery icon. */
void draw_BatteryHide(void);

/* Draws the GPS satellite icon. */
void draw_GPS(bool forceshow);
/* Hides the GPS satellite icon. */
void draw_GPSHide(void);


/* ################### hardware dependent function definitions ################### */

/*
 * Initialises display functions, includes oled and SPI initialisation.
 */
void display_init(uint32_t g_ui32SysClock)
{
	oled_init(g_ui32SysClock);
	
	colors[black] = Color565(0, 0, 0);
	colors[white] = Color565(0xFF, 0xFF, 0xFF);
	colors[red] = Color565(0xFF, 0, 0);
	colors[green] = Color565(0, 0xFF, 0);
	colors[blue] = Color565(0, 0, 0xFF);
	colors[orange] = Color565(0xFF, 110, 0);
	colors[yellow] = Color565(0xFF, 0xFF, 0);
	colors[gray] = Color565(0x8F, 0x8F, 0x8F);

	syscolors[back] = colors[black];
	syscolors[text] = colors[white];
	syscolors[textstat] = colors[gray];
	syscolors[selection] = colors[blue];
	
	page = 0;
}

/*
 * Initialises the display content after system start and fill with content if selected page.
 */
void display_initContent(void)
{
	oled_fillScreen(syscolors[back]);
	display_GPS(0, 0, 99);
	display_SDMissing();
	display_Battery(0);

	display_drawStaticText();
}

/* 
 * Sets the page to be displayed and perform static page drawing.
 * Page		the page to be selected and drawn.
 */
void display_setPage(uint8_t Page)
{
	if(Page>GP_LASTPAGE) Page=GP_LASTPAGE;

	page = Page;
	display_drawStaticText();
}

/*
 * Prints the time on the display.
 * hr, min, sec		the hour, minute and second to be printed.
 */
void display_Time(uint8_t hr, uint8_t min, uint8_t sec)
{
	oled_drawtext(ui8ToA(hr, buffer, 2), syscolors[text], syscolors[back], GP_TIMEX, GP_TIMEY);
	oled_drawtext(":", syscolors[text], syscolors[back], GP_TIMEX+12, GP_TIMEY);
	oled_drawtext(ui8ToA(min, buffer, 2), syscolors[text], syscolors[back], GP_TIMEX+18, GP_TIMEY);
	oled_drawtext(":", syscolors[text], syscolors[back], GP_TIMEX+30, GP_TIMEY);
	oled_drawtext(ui8ToA(sec, buffer, 2), syscolors[text], syscolors[back], GP_TIMEX+36, GP_TIMEY);
}

/*
 * Draw the static (non-changing) text and grahpics for the selected page.
 */
void display_drawStaticText(void)
{
	oled_fillRect(0, 13, SSD1351WIDTH, SSD1351HEIGHT-13, syscolors[back]);

	if(page==0)
	{
		/* Stopwatch */
		oled_drawtext("Stopwatch", syscolors[textstat], syscolors[back], GP_STPWX, GP_STPWY);
		oled_drawtext_big(":", syscolors[text], syscolors[back], GP_STPWX+22, GP_STPWY+9);
		oled_drawtext_big(":", syscolors[text], syscolors[back], GP_STPWX+55, GP_STPWY+9);
		oled_drawtext_big(".", syscolors[text], syscolors[back], GP_STPWX+88, GP_STPWY+9);
		/* Altitude */
		oled_drawtext("Altitude", syscolors[textstat], syscolors[back], GP_ALTX, GP_ALTY);
		oled_drawtext("m", syscolors[textstat], syscolors[back], GP_ALTX+45, GP_ALTY+15);
		oled_drawtext("Up", syscolors[textstat], syscolors[back], GP_ALTX+55, GP_ALTY+6);
		oled_drawtext("Dwn", syscolors[textstat], syscolors[back], GP_ALTX+55, GP_ALTY+15);
		oled_drawtext("m", syscolors[textstat], syscolors[back], GP_ALTX+104, GP_ALTY+6);
		oled_drawtext("m", syscolors[textstat], syscolors[back], GP_ALTX+104, GP_ALTY+15);
		/* Speed */
		oled_drawtext("Speed", syscolors[textstat], syscolors[back], GP_SPDX, GP_SPDY);
		oled_drawtext("km", syscolors[textstat], syscolors[back], GP_SPDX+34, GP_SPDY+6);
		oled_drawHLine(GP_SPDX+34, GP_SPDY+14, 11, syscolors[textstat]);
		oled_drawtext("h", syscolors[textstat], syscolors[back], GP_SPDX+36, GP_SPDY+15);
		oled_drawtext("Avg", syscolors[textstat], syscolors[back], GP_SPDX+55, GP_SPDY+6);
		oled_drawtext("Max", syscolors[textstat], syscolors[back], GP_SPDX+55, GP_SPDY+15);
		oled_drawtext("km/h", syscolors[textstat], syscolors[back], GP_SPDX+98, GP_SPDY+6);
		oled_drawtext("km/h", syscolors[textstat], syscolors[back], GP_SPDX+98, GP_SPDY+15);
		/* Distance */
		oled_drawtext("Distance", syscolors[textstat], syscolors[back], GP_DISTX, GP_DISTY);
		oled_drawtext("km", syscolors[textstat], syscolors[back], GP_DISTX+34, GP_DISTY+15);
		oled_drawtext("m", syscolors[textstat], syscolors[back], GP_DISTX+81, GP_DISTY+15);
	}
	else if(page==1)
	{
		/* Time to first fix */
		oled_drawtext("Time To First Fix", syscolors[textstat], syscolors[back], GP_TTFFX, GP_TTFFY);
		oled_drawtext("m", syscolors[textstat], syscolors[back], GP_TTFFX+18, GP_TTFFY+9);
		oled_drawtext("s", syscolors[textstat], syscolors[back], GP_TTFFX+42, GP_TTFFY+9);
		
		/* Time Since Reset */
		oled_drawtext("Time Since Reset", syscolors[textstat], syscolors[back], GP_TSRX, GP_TSRY);
		oled_drawtext("d", syscolors[text], syscolors[back], GP_TSRX+24, GP_TSRY+9);
		oled_drawtext(":", syscolors[text], syscolors[back], GP_TSRX+48, GP_TSRY+9);
		oled_drawtext(":", syscolors[text], syscolors[back], GP_TSRX+66, GP_TSRY+9);

		/* Satellite Info */
		oled_drawtext("Vw/Fx PDOP/HDOP/VDOP", syscolors[textstat], syscolors[back], GP_SATX, GP_SATY);
		oled_drawtext("/", syscolors[textstat], syscolors[back], GP_SATX+12, GP_SATY+9);
		oled_drawtext("/", syscolors[textstat], syscolors[back], GP_SATX+60, GP_SATY+9);
		oled_drawtext("/", syscolors[textstat], syscolors[back], GP_SATX+90, GP_SATY+9);

		/* Fix Info */
		oled_drawtext("Fix Type/Quality", syscolors[textstat], syscolors[back], GP_FIXX, GP_FIXY);
		oled_drawtext("fix", syscolors[text], syscolors[back], GP_FIXX+18, GP_FIXY+9);


		/* Lat/Lon Position */
		oled_drawtext("Latitude", syscolors[textstat], syscolors[back], GP_LLX, GP_LLY);
		oled_drawtext("Longitude", syscolors[textstat], syscolors[back], GP_LLX, GP_LLY+18);
		oled_drawtext(".", syscolors[text], syscolors[back], GP_LLX+18, GP_LLY+9);
		oled_drawtext(".", syscolors[text], syscolors[back], GP_LLX+18, GP_LLY+27);
		oled_drawtext("`", syscolors[text], syscolors[back], GP_LLX+54, GP_LLY+9);	/* '`' is substitude symbol for '°' */
		oled_drawtext("`", syscolors[text], syscolors[back], GP_LLX+54, GP_LLY+27);
	}
	else if(page==2)
	{
		/* Satellites Overview */
		oled_drawtext("Satellites Overview", syscolors[textstat], syscolors[back], GP_SATOVX, GP_SATOVY);
		oled_drawHLine(GP_SATOVX, GP_SATOVY+9, 126, syscolors[textstat]);		/* draw grid */
		oled_drawHLine(GP_SATOVX, GP_SATOVY+29, 126, syscolors[textstat]);
		oled_drawHLine(GP_SATOVX, GP_SATOVY+49, 126, syscolors[textstat]);
		oled_drawHLine(GP_SATOVX, GP_SATOVY+69, 126, syscolors[textstat]);
		oled_drawHLine(GP_SATOVX, GP_SATOVY+89, 126, syscolors[textstat]);
		oled_drawVLine(GP_SATOVX+0, GP_SATOVY+9, 80, syscolors[textstat]);
		oled_drawVLine(GP_SATOVX+14, GP_SATOVY+9, 80, syscolors[textstat]);
		oled_drawVLine(GP_SATOVX+28, GP_SATOVY+9, 80, syscolors[textstat]);
		oled_drawVLine(GP_SATOVX+42, GP_SATOVY+9, 80, syscolors[textstat]);
		oled_drawVLine(GP_SATOVX+56, GP_SATOVY+9, 80, syscolors[textstat]);
		oled_drawVLine(GP_SATOVX+70, GP_SATOVY+9, 80, syscolors[textstat]);
		oled_drawVLine(GP_SATOVX+84, GP_SATOVY+9, 80, syscolors[textstat]);
		oled_drawVLine(GP_SATOVX+98, GP_SATOVY+9, 80, syscolors[textstat]);
		oled_drawVLine(GP_SATOVX+112, GP_SATOVY+9, 80, syscolors[textstat]);
		oled_drawVLine(GP_SATOVX+126, GP_SATOVY+9, 80, syscolors[textstat]);
	}
	else if(page==3)
	{
		display_conf(-1, sSelection);
	}
}


/*
 * Draws the stopwatch text on the display (sMs = tenth of seconds).
 * sHr, sMin, sSec, sMs		the Hour, Minute, Second and tenth of seconds of the stopwatch.
 */
void display_Stpw(uint8_t sHr, uint8_t sMin, uint8_t sSec, uint8_t sMs)
{
	if(!((1<<page) & GP_STPWPAGE)) return;

	oled_drawtext_big(ui8ToA(sHr, buffer, 2), syscolors[text], syscolors[back], GP_STPWX, GP_STPWY+9);
	oled_drawtext_big(ui8ToA(sMin, buffer, 2), syscolors[text], syscolors[back], GP_STPWX+33, GP_STPWY+9);
	oled_drawtext_big(ui8ToA(sSec, buffer, 2), syscolors[text], syscolors[back], GP_STPWX+66, GP_STPWY+9);
	oled_drawtext_big(ui8ToA(sMs, buffer, 1), syscolors[text], syscolors[back], GP_STPWX+99, GP_STPWY+9);
}

/*
 * Draws the altitude and the altitude upwards/downwards made good on the display.
 * Alt		the altitude in m
 * AltUp	altitude made good upwards in m
 * AltDwn	altitude made good downwards in m
 */
void display_Alt(int32_t Alt, uint32_t AltUp, uint32_t AltDwn)
{
	if(!((1<<page) & GP_ALTPAGE)) return;

	oled_drawtext_big(ui16ToA(Alt, buffer, 4, true), syscolors[text], syscolors[back], GP_ALTX, GP_ALTY+9);

	oled_drawtext(ui16ToA(AltUp, buffer, 4, true), syscolors[text], syscolors[back], GP_ALTX+79, GP_ALTY+6);
	oled_drawtext(ui16ToA(AltDwn, buffer, 4, true), syscolors[text], syscolors[back], GP_ALTX+79, GP_ALTY+15);

}

/*
 * Draws the current, average and maximum speed in km/h on the display.
 * Speed	Current speed in km/h
 * Avg		Average speed in km/h
 * Max		Maximum speed in km/h
 */
void display_Speed(uint16_t Speed, uint16_t Avg, uint16_t Max)
{
	if(!((1<<page) & GP_SPDPAGE)) return;

	oled_drawtext_big(ui16ToA(Speed, buffer, 3, true), syscolors[text], syscolors[back], GP_SPDX, GP_SPDY+9);

	oled_drawtext(ui16ToA(Avg, buffer, 3, true), syscolors[text], syscolors[back], GP_SPDX+79, GP_SPDY+6);
	oled_drawtext(ui16ToA(Max, buffer, 3, true), syscolors[text], syscolors[back], GP_SPDX+79, GP_SPDY+15);
}

/*
 * Draws the distance made good on the display.
 * Dist		distance made good in m.
 */
void display_Dist(int32_t Dist)
{
	uint16_t m, km;

	if(!((1<<page) & GP_DISTPAGE)) return;

	m = Dist % 1000;
	km = Dist / 1000;

	oled_drawtext_big(ui16ToA(km, buffer, 3, true), syscolors[text], syscolors[back], GP_DISTX, GP_DISTY+9);
	oled_drawtext_big(ui16ToA(m, buffer, 3, true), syscolors[text], syscolors[back], GP_DISTX+47, GP_DISTY+9);
}

/* 
 * Draws the Time since reset on the display. 
 * d, h, m, s		days, hours, minutes and seconds since reset.
 */
void display_Tsr(uint32_t d, uint8_t h, uint8_t m, uint8_t s)
{
	if(!((1<<page) & GP_TSRPAGE)) return;

	oled_drawtext(ui16ToA(d, buffer, 4, false), syscolors[text], syscolors[back], GP_TSRX, GP_TSRY+9);

	oled_drawtext(ui8ToA(h, buffer, 2), syscolors[text], syscolors[back], GP_TSRX+36, GP_TSRY+9);
	oled_drawtext(ui8ToA(m, buffer, 2), syscolors[text], syscolors[back], GP_TSRX+54, GP_TSRY+9);
	oled_drawtext(ui8ToA(s, buffer, 2), syscolors[text], syscolors[back], GP_TSRX+72, GP_TSRY+9);
}

/* 
 * Draws general satellite and fix information on the display.
 * satView		number of satellites in view
 * satFix		number of satellites used in fix
 * PDOP			Position dilution of precision in 0.1
 * HDOP			Horizontal dilution of precision in 0.1
 * VDOP			Vertival dilution of precision in 0.1
 */
void display_Satinfo(uint8_t satView, uint8_t satFix, uint8_t PDOP, uint8_t HDOP, uint8_t VDOP)
{
	if(!((1<<page) & GP_SATPAGE)) return;

	oled_drawtext(ui8ToA(satView, buffer, 2), syscolors[text], syscolors[back], GP_SATX, GP_SATY+9);
	oled_drawtext(ui8ToA(satFix,  buffer, 2), syscolors[text], syscolors[back], GP_SATX+18, GP_SATY+9);

	ui8ToA(PDOP, buffer, 3);
	buffer[3] = buffer[2];
	buffer[4] = 0;
	buffer[2] = '.';
	oled_drawtext(buffer, syscolors[text], syscolors[back], GP_SATX+36, GP_SATY+9);
	ui8ToA(HDOP, buffer, 3);
	buffer[3] = buffer[2];
	buffer[4] = 0;
	buffer[2] = '.';
	oled_drawtext(buffer, syscolors[text], syscolors[back], GP_SATX+66, GP_SATY+9);
	ui8ToA(VDOP, buffer, 3);
	buffer[3] = buffer[2];
	buffer[4] = 0;
	buffer[2] = '.';
	oled_drawtext(buffer, syscolors[text], syscolors[back], GP_SATX+96, GP_SATY+9);
}

/*
 * Draws GPS fix information on the display.
 * GPSFixType		the GPS fix type (1=nofix, 2=2Dfix, 3=3Dfix)
 * GPSFixQuality	the fix quality/source (0-8, see code)
 */
void display_Fixinfo(uint8_t GPSFixType, uint8_t GPSFixQuality)
{
	char q[4];
	if(!((1<<page) & GP_FIXPAGE)) return;

	switch(GPSFixType)
	{
	case 1: q[0]='n'; q[1]='o'; break;	/* no fix */
	case 2: q[0]='2'; q[1]='D'; break;	/* 2D fix */
	case 3: q[0]='3'; q[1]='D'; break;	/* 3D fix */
	default:q[0]='-'; q[1]='-'; break;	/* unknown */
	}
	q[2] = 0;

	oled_drawtext(q, syscolors[text], syscolors[back], GP_FIXX, GP_FIXY+9);

	switch(GPSFixQuality)
	{
	case 0: q[0]='I'; q[1]='n'; q[2]='v'; break;	/* invalid */
	case 1: q[0]='G'; q[1]='P'; q[2]='S'; break;	/* GPS */
	case 2: q[0]='D'; q[1]='G'; q[2]='P'; break;	/* DGPS */
	case 3: q[0]='P'; q[1]='P'; q[2]='S'; break;	/* PPS */
	case 4: q[0]='R'; q[1]='T'; q[2]='K'; break;	/* RTK */
	case 5: q[0]='F'; q[1]='R'; q[2]='T'; break;	/* Float RTK */
	case 6: q[0]='E'; q[1]='s'; q[2]='t'; break;	/* Estimated */
	case 7: q[0]='M'; q[1]='a'; q[2]='n'; break;	/* Manual */
	case 8: q[0]='S'; q[1]='i'; q[2]='m'; break;	/* Simulation */
	}
	q[3] = 0;

	oled_drawtext(q, syscolors[text], syscolors[back], GP_FIXX+54, GP_FIXY+9);
}

/* 
 * Draws the latitude and longitude position. 
 * LatI/LonI	integer part of coordinate 
 * LatF/LonF	fractional part of coordinate
 * NS/EW		North-South/East-West char. 
 */
void display_LatLon(uint8_t LatI, uint32_t LatF, char NS, uint8_t LonI, uint32_t LonF, char EW)
{
	char buff[2];
	if(!((1<<page) & GP_LLPAGE)) return;

	buff[1] = 0;
	
	buff[0] = NS;
	oled_drawtext(ui8ToA(LatI, buffer, 2) , syscolors[text], syscolors[back], GP_LLX+6 , GP_LLY+9);
	oled_drawtext(ui32ToA(LatF, buffer, 5), syscolors[text], syscolors[back], GP_LLX+24, GP_LLY+9);
	oled_drawtext(buff                    , syscolors[text], syscolors[back], GP_LLX+66, GP_LLY+9);

	buff[0] = EW;
	oled_drawtext(ui8ToA(LonI, buffer, 3) , syscolors[text], syscolors[back], GP_LLX   , GP_LLY+27);
	oled_drawtext(ui32ToA(LonF, buffer, 5), syscolors[text], syscolors[back], GP_LLX+24, GP_LLY+27);
	oled_drawtext(buff                    , syscolors[text], syscolors[back], GP_LLX+66, GP_LLY+27);
}

/* 
 * Draws the time to first fix.
 * sec		Seconds to first fix
 */
void display_TTFF(uint32_t sec)
{
	if(!((1<<page) & GP_TTFFPAGE)) return;

	uint32_t min = sec / 60;
	sec -= min * 60;

	oled_drawtext(ui32ToA(min, buffer, 3),          syscolors[text], syscolors[back], GP_TTFFX, GP_TTFFY+9);
	oled_drawtext(ui8ToA((uint8_t)sec, buffer, 2) , syscolors[text], syscolors[back], GP_TTFFX+30 , GP_TTFFY+9);

}

/* 
 * Draws a table with an overview of satellite IDs, IDs used in fix and SNR data. 
 * satViewNum		number of satellites in view
 * satFixNum		number of satellites used in fix
 * satsView			pointer to an 32 index array containing satellite IDs in view.
 * satsViewSNR		pointer to an 32 index array containing satellite SNR.
 * satsFix			pointer to an 32 index array containing satellite IDs used in fix.
 */
void display_Satov(uint8_t satViewNum, uint8_t satFixNum, uint8_t * satsView, uint8_t * satsViewSNR, uint8_t * satsFix)
{
	uint8_t i, r, c;
	bool inview, infix;
	uint8_t satid, satsnr;
	
	if(!((1<<page) & GP_SATOVPAGE)) return;
	
	/* check maximum number of sats in view and in fix */
	if(satViewNum>32) satViewNum=32;
	if(satFixNum>12) satFixNum=12;
	
	/* loop through row (r) and column (c) */
	for(r=0; r<4; r++)
	{
		for(c=0; c<9; c++)
		{
			if(r==3 && c>4) break;
			satid = c+r*9+1;
			
			/* check if satellite ID is currently in view, 
			   if true, get SNR */
			inview = false;
			for(i=0; i<satViewNum; i++)
			{
				if(satid == satsView[i])
				{ inview = true; satsnr=satsViewSNR[i]; break; }
			}
			
			/* check if satellite ID is currently present in fix array */
			infix = false;
			for(i=0; i<satFixNum; i++)
			{
				if(satid == satsFix[i])
				{
					infix = true;
					break;
				}
			}
			
			/* draw ID and SNR according to whether sat is used in fix, sat is in view or neither. */
			if(infix)
			{
				oled_drawtext(ui8ToA(satid,  buffer, 2), colors[green], syscolors[back], GP_SATOVX+2+(c*14), GP_SATOVY+11+(r*20));
				oled_drawtext(ui8ToA(satsnr, buffer, 2), colors[green], syscolors[back], GP_SATOVX+2+(c*14), GP_SATOVY+20+(r*20));
			}
			else if(inview)
			{
				oled_drawtext(ui8ToA(satid,  buffer, 2), syscolors[text], syscolors[back], GP_SATOVX+2+(c*14), GP_SATOVY+11+(r*20));			
				oled_drawtext(ui8ToA(satsnr, buffer, 2), syscolors[text], syscolors[back], GP_SATOVX+2+(c*14), GP_SATOVY+20+(r*20));
			}
			else
			{
				oled_drawtext(ui8ToA(satid, buffer, 2), syscolors[textstat], syscolors[back], GP_SATOVX+2+(c*14), GP_SATOVY+11+(r*20));
				oled_drawtext("--",                     syscolors[textstat], syscolors[back], GP_SATOVX+2+(c*14), GP_SATOVY+20+(r*20));
			}
		}
	}

	
	
}

/* ================================================= */

/* 
 * Displays the SD card icon, remaining SD capacity and the record state.
 * rec			true, if record is in progress
 * capacity		remaining SD capacity in percent (0-100)
 */
void display_SD(bool rec, uint8_t capacity)
{
    oled_drawIcon(icon_sd, colors[green], syscolors[back], GP_SDX, GP_SDY, 10, 10);
    oled_drawPixel(GP_SDX+3, GP_SDY+6, colors[green]);
    oled_drawPixel(GP_SDX+4, GP_SDY+7, colors[green]);
    oled_drawPixel(GP_SDX+5, GP_SDY+6, colors[green]);
    oled_drawPixel(GP_SDX+6, GP_SDY+5, colors[green]);

    if (rec)
    {
        oled_drawIcon(icon_rec, colors[red], syscolors[back], GP_SDX+12, GP_SDY, 10, 10);
    }
    else
    {
    	oled_fillRect(GP_SDX+12, GP_SDY, 3, 10, colors[blue]);
    	oled_fillRect(GP_SDX+15, GP_SDY, 4, 10, syscolors[back]);
    	oled_fillRect(GP_SDX+19, GP_SDY, 3, 10, colors[blue]);
    }
}

/* 
 * Draw the icon for indicating, that no SD card is present. 
 */
void display_SDMissing(void)
{
    oled_drawIcon(icon_sd, colors[red], syscolors[back], GP_SDX, GP_SDY, 10, 10);
    oled_drawPixel(GP_SDX+2, GP_SDY+4, colors[red]);
    oled_drawPixel(GP_SDX+3, GP_SDY+5, colors[red]);
    oled_drawPixel(GP_SDX+5, GP_SDY+5, colors[red]);
    oled_drawPixel(GP_SDX+6, GP_SDY+4, colors[red]);
    oled_drawPixel(GP_SDX+4, GP_SDY+6, colors[red]);
    oled_drawPixel(GP_SDX+3, GP_SDY+7, colors[red]);
    oled_drawPixel(GP_SDX+2, GP_SDY+8, colors[red]);
    oled_drawPixel(GP_SDX+5, GP_SDY+7, colors[red]);
    oled_drawPixel(GP_SDX+6, GP_SDY+8, colors[red]);

    oled_fillRect(GP_SDX+12, GP_SDY, 10, 10, syscolors[back]);
}

/* 
 * Prints the GPS satellite symbol and the number of satellites on the display.
 * satellites	the number of satellites to be displayed.
 * fix 			the GPS fix type (1=nofix, 2=2Dfix, 3=3Dfix, others=no GPS module)
 */
void display_GPS(uint8_t satellites, uint8_t fix, uint8_t DOP)
{

	if(satellites>99) satellites=99;
	if(sats==satellites && gpsfix==fix && dop==DOP) return;
	sats = satellites;
	gpsfix=fix;
	dop=DOP;

	if(gpsfix!=2 && gpsfix!=3)	/* blink if no 2D or 3D fix */
	{
		satblink = true;
	}
	else
	{
		satblink = false;
		if(satblink == true) draw_GPS(true); else draw_GPS(false);
	}
}

/* 
 * Draws the GPS satellite icon. 
 */
void draw_GPS(bool forceshow)
{
	static uint8_t tmpsats = 0;
	static uint8_t tmpfix = 1;
	static uint8_t tmpdop = 1;
	uint8_t i;

	if(tmpsats==sats && tmpfix==gpsfix && tmpdop==dop && forceshow==false) return;
	tmpsats=sats;
	tmpfix=gpsfix;
	tmpdop = dop;

	if(!(tmpfix==3 || tmpfix==2 || tmpfix==1)) // no fix information provided
	{
		oled_drawIcon(icon_gps, colors[white], syscolors[back], GP_GPSX, GP_GPSY, 10, 10);

		for(i=0;i<10;i++)
		{
			oled_drawPixel(GP_GPSX+9-i, GP_GPSY+i, colors[red]);
		}
	}
	else if(tmpdop>GP_DOPTHRESHR)
	{
	    oled_drawIcon(icon_gps, colors[red], syscolors[back], GP_GPSX, GP_GPSY, 10, 10);
	}
	else if(tmpdop>GP_DOPTHRESHY)
	{
	    oled_drawIcon(icon_gps, colors[yellow], syscolors[back], GP_GPSX, GP_GPSY, 10, 10);
	}
	else
	{
	    oled_drawIcon(icon_gps, colors[green], syscolors[back], GP_GPSX, GP_GPSY, 10, 10);
	}

	oled_drawtext(ui8ToA(tmpsats, buffer, 2), syscolors[text], syscolors[back], GP_GPSX+11, GP_GPSY+1);
}
/* 
 * Hides the GPS satellite icon.
 */
void draw_GPSHide()
{
	oled_fillRect(GP_GPSX, GP_GPSY, 10, 10, syscolors[back]);
}

/*
 * Display remaining battery capacity.
 * capacity		the battery capacity to be drawn in precent (0-100)
 */
void display_Battery(uint8_t capacity)
{
	if(capacity>100) capacity=100;
	if(bat==capacity) return;
	bat = capacity;

	if(bat<=20)
	{
		batblink = true;
	}
	else // bat>20
	{
		batblink = false;
		if(batblink == true) draw_Battery(true); else draw_Battery(false);

	}
}

/* 
 * Draws the battery icon. 
 */
void draw_Battery(bool forceshow)
{
	uint16_t col;
	static uint8_t tmpbat = 0;
	uint8_t tmp;

	if(bat==tmpbat && forceshow==false) return;
	tmpbat = bat;

    oled_drawIcon(icon_batt, syscolors[text], syscolors[back], GP_BATTX, GP_BATTY, 14, 10);
	tmp = tmpbat / 10;

	if(tmp<=2) col = colors[red];
	else if(tmp<=4) col = colors[orange];
	else if(tmp<=6) col = colors[yellow];
	else col = colors[green];

	oled_fillRect(GP_BATTX+1, GP_BATTY+1, tmp, 8, col);


	//oled_drawtext(ui8ToA(tmpbat, buffer, 3), colors[white], syscolors[back], GP_BATTX+15, GP_BATTY+1);
}
/* 
 * Hides the battery icon.
 */
void draw_BatteryHide()
{
	oled_fillRect(GP_BATTX, GP_BATTY, 14, 10, syscolors[back]);
}

/* 
 *Must be called in the desired slow blink frequency. 
 */
void display_blink_slow(void)
{

}
/* 
 * Must be called in the desired fast blink frequency. 
 */
void display_blink_fast(void)
{
	static bool show = false;

	show = !show;

	if(show)
	{
		if(batblink) draw_Battery(true);
		if(satblink) draw_GPS(true);
	}
	else
	{
		if(batblink) draw_BatteryHide();
		if(satblink) draw_GPSHide();
	}
}

/* ================================================= */
/* Configuration */

/*
 * Displays he current configuration
 */
void display_conf(int8_t selected, eselcolor selColor)
{
	uint16_t backcol;
	uint16_t selectioncol;

	switch(selColor)
	{
	case sBack:
		selectioncol = syscolors[back];
		break;
	case sSelection:
		selectioncol = syscolors[selection];
		break;
	case sRed:
		selectioncol = colors[red];
		break;
	case sGreen:
		selectioncol = colors[green];
		break;
	}


	//			  "----.----.----.----."
	if(selected==CFG_LOGDBG) backcol = selectioncol; else backcol = syscolors[back];
	oled_drawtext("logDebug=", syscolors[textstat], backcol, GP_LOGSETX, GP_LOGSETY);
	if(conf.logDebug) buffer[0] = '1'; else buffer[0] = '0';
	buffer[1]='\0';
	oled_drawtext(buffer, syscolors[text], backcol, GP_LOGSETX+(9*6), GP_LOGSETY);

	//			  "----.----.----.----."
	if(selected==CFG_LOGINTVL) backcol = selectioncol; else backcol = syscolors[back];
	oled_drawtext("logIntvl=", syscolors[textstat], backcol, GP_LOGSETX, GP_LOGSETY+9);
	ui8ToA(conf.logIntvl, buffer, 3);
	buffer[4] = buffer[3]; buffer[3] = buffer[2]; buffer[2] = '.';
	oled_drawtext(buffer, syscolors[text], backcol, GP_LOGSETX+(9*6), GP_LOGSETY+9);

	//			  "----.----.----.----."
	if(selected==CFG_LOGASTART) backcol = selectioncol; else backcol = syscolors[back];
	oled_drawtext("logAutoStart=", syscolors[textstat], backcol, GP_LOGSETX, GP_LOGSETY+18);
	if(conf.logAutoStart) buffer[0] = '1'; else buffer[0] = '0';
	buffer[1]='\0';
	oled_drawtext(buffer, syscolors[text], backcol, GP_LOGSETX+(13*6), GP_LOGSETY+18);


	//			  "----.----.----.----." gpsUartBaud
	if(selected==CFG_GPSBAUD) backcol = selectioncol; else backcol = syscolors[back];
	oled_drawtext("gpsUartBaud=", syscolors[textstat], backcol, GP_GPSSETX, GP_GPSSETY);
	ui32ToA(conf.gpsUartBaud, buffer, 6);
	//buffer[7] = buffer[6]; buffer[6] = buffer[5]; buffer[5] = '.';
	oled_drawtext(buffer, syscolors[text], backcol, GP_GPSSETX+(12*6), GP_GPSSETY);

	//			  "----.----.----.----." gpsAltThreshold
	if(selected==CFG_GPSALTTH) backcol = selectioncol; else backcol = syscolors[back];
	oled_drawtext("gpsAltThres=", syscolors[textstat], backcol, GP_GPSSETX, GP_GPSSETY+9);
	ui32ToA(conf.gpsAltThreshold, buffer, 6);
	buffer[7] = buffer[6]; buffer[6] = buffer[5]; buffer[5] = '.';
	oled_drawtext(buffer, syscolors[text], backcol, GP_GPSSETX+(12*6), GP_GPSSETY+9);

	//			  "----.----.----.----." gpsDopThreshold
	if(selected==CFG_GPSDOPTH) backcol = selectioncol; else backcol = syscolors[back];
	oled_drawtext("gpsDopThres=", syscolors[textstat], backcol, GP_GPSSETX, GP_GPSSETY+18);
	ui8ToA(conf.gpsDopThreshold, buffer, 3);
	buffer[4] = buffer[3]; buffer[3] = buffer[2]; buffer[2] = '.';
	oled_drawtext(buffer, syscolors[text], backcol, GP_GPSSETX+(12*6), GP_GPSSETY+18);

	//			  "----.----.----.----." gpsDistThreshold
	if(selected==CFG_GPSDISTTH) backcol = selectioncol; else backcol = syscolors[back];
	oled_drawtext("gpsDistThres=", syscolors[textstat], backcol, GP_GPSSETX, GP_GPSSETY+27);
	ui32ToA(conf.gpsDistThreshold, buffer, 6);
	buffer[7] = buffer[6]; buffer[6] = buffer[5]; buffer[5] = '.';
	oled_drawtext(buffer, syscolors[text], backcol, GP_GPSSETX+(13*6), GP_GPSSETY+27);

	//			  "----.----.----.----."
	if(selected==CFG_DISPDIMT) backcol = selectioncol; else backcol = syscolors[back];
	oled_drawtext("dispDimTime=", syscolors[textstat], backcol, GP_DISPSETX, GP_DISPSETY);
	ui32ToA(conf.dispDimTime, buffer, 6);
	buffer[7] = buffer[6]; buffer[6] = buffer[5]; buffer[5] = '.';
	oled_drawtext(buffer, syscolors[text], backcol, GP_DISPSETX+(12*6), GP_DISPSETY);

	//			  "----.----.----.----."
	if(selected==CFG_DISPOFFT) backcol = selectioncol; else backcol = syscolors[back];
	oled_drawtext("dispOffTime=", syscolors[textstat], backcol, GP_DISPSETX, GP_DISPSETY+9);
	ui32ToA(conf.dispOffTime, buffer, 6);
	buffer[7] = buffer[6]; buffer[6] = buffer[5]; buffer[5] = '.';
	oled_drawtext(buffer, syscolors[text], backcol, GP_DISPSETX+(12*6), GP_DISPSETY+9);

#ifndef CONF_READONLY
	//			  "----.----.----.----."
	if(selected==CFG_EDITSAVE) backcol = selectioncol; else backcol = syscolors[back];
	oled_drawtext("save configuration", syscolors[textstat], backcol, GP_DISPCONFX, GP_DISPCONFY);
#endif
}
