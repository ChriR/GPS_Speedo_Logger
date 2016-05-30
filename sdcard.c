/*
 * sdcard.c
 *
 *  Created on: 22.03.2016
 *      Author: Christoph Ringl
 */


#include "sdcard.h"
#include "spi_wrapper.h"
#include "time.h"
#include "conversion.h"
#include "gps.h"
#include "config.h"
#include "fatfs/ff.h"
#include "fatfs/diskio.h"
#include "fatfs/integer.h"

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"

#include <string.h>

extern config_t conf;

uint8_t SDSPIProcess;


// -------------- FatFs Definitions -------------------
DWORD AccSize;				/* Work register for fs command */
WORD AccFiles, AccDirs;
FILINFO Finfo;
#if _USE_LFN
char Lfname[_MAX_LFN+1];
#endif

BYTE buff[512];				/* Working buffer */

FATFS FatFs[_VOLUMES];		/* File system object for each logical drive */
FIL File[2];				/* File object */
//DIR Dir;					/* Directory object */


//BYTE RtcOk;					/* RTC is available */

//volatile UINT Timer;		/* Performance timer (100Hz increment) */
// ----------------------------------------------------

uint8_t logFlag = 0;


/* ---===###  S D   S Y S T E M   F U N C T I O N S  ###===--- */

void SDCS_assert(void)
{
	clearSDCS();
}

void SDCS_deassert(void)
{
	setSDCS();
}

inline uint8_t getSDSPIProcess(void)
{
	return SDSPIProcess;
}

/* Initialises SD card functions. */
void sd_init(uint32_t sd_g_ui32SysClock)
{
#ifndef SDCARD_OFF
	SDSPIProcess = SPI_registerProc(SDCS_assert, SDCS_deassert);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);

    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_5);											/* SD Chip Select */
	GPIOPinTypeGPIOInput(GPIO_PORTK_BASE, GPIO_PIN_6);											/* SD Card Detect */

	GPIOPadConfigSet(GPIO_PORTK_BASE, GPIO_PIN_5, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);		/* SD Chip Select */
	GPIOPadConfigSet(GPIO_PORTK_BASE, GPIO_PIN_6, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);	/* SD Card Detect */

	setSDCS();
#endif
}

void sd_tick10Ms(void)
{
#ifndef SDCARD_OFF
	disk_timerproc();
#endif
}

/*
 * SD Init
 */
uint8_t sd_initCard(void)
{
	uint8_t p1 = 255;
	p1 = (uint8_t)disk_initialize((BYTE)0);
	return p1;
}

/* 
 * Returns true, if a SD card is in the socket.
 */
bool sd_inserted(void)
{
	return !(disk_status(0) & (STA_NODISK));
}

/*
 * Returns true, if a SD card is initialised.
 */
bool sd_initialised(void)
{
	return !(disk_status(0) & (STA_NODISK|STA_NOINIT));
}

/* ---===###  L O G   F U N C T I O N S  ###===--- */
/*
 * Returns the next unused Log File Name.
 */
char * log_getNextID(char * buffer, time_t time, date_t date, bool event)
{
	uint8_t i = 0;
	strncpy(&buffer[i], event ? EVENT_FILENAME : LOG_FILENAME, 20);
	while(buffer[i]) i++;

	buffer[i++] = '_';

	ui8ToA(date.y, &buffer[i], 2);
	i+=2;
	ui8ToA(date.m, &buffer[i], 2);
	i+=2;
	ui8ToA(date.d, &buffer[i], 2);
	i+=2;

	buffer[i++] = '_';

	ui8ToA(time.hr, &buffer[i], 2);
	i+=2;
	ui8ToA(time.min, &buffer[i], 2);
	i+=2;
	ui8ToA(time.sec, &buffer[i], 2);
	i+=2;

	strncpy(&buffer[i], LOG_EXT, 4+1);
	i+=4;
	return buffer;
}

/*
 * Opens file and starts logging.
 */
uint8_t log_Start(char * logFilename, char * eventFilename)
{
#ifndef SDCARD_OFF
	BYTE b1;
	UINT cnt;

	if(!sd_initialised())
	{
		// init SD Card
		b1 = sd_initCard();
		if ( !(b1==FR_OK) ) return b1;
	}

	b1 = f_mount(&FatFs[0], "0:", 1);
	if ( !(b1==FR_OK) ) return b1;

	b1 = f_chdir(LOG_DIR);
	if(b1==FR_NO_PATH)
	{
		b1 = f_mkdir(LOG_DIR);
		if ( !(b1==FR_OK) ) return b1;
		b1 = f_chdir(LOG_DIR);
	}
	if ( !(b1==FR_OK) ) return b1;

	
	strncpy((char *)buff, LOG_LEADIN, LOG_LILENGTH+1);
	
	/* open log file for write */
	b1 = f_open(&File[0], logFilename, FA_WRITE | FA_CREATE_ALWAYS);
	if ( !(b1==FR_OK) ) return b1;

	/* write lead-in to log file */
	b1 = f_write(&File[0], buff, LOG_LILENGTH, &cnt);
	if ( !(b1==FR_OK) ) return b1;
	
	
	/* open event file for write */
	b1 = f_open(&File[1], eventFilename, FA_WRITE | FA_CREATE_ALWAYS);
	if ( !(b1==FR_OK) ) return b1;
	
	/* write lead-in to event file */
	b1 = f_write(&File[1], buff, LOG_LILENGTH, &cnt);
	if ( !(b1==FR_OK) ) return b1;
	
	logFlag = 1;
#endif
	return 0;
}

/*
 * Stops logging and closes file.
 */
uint8_t log_Stop(void)
{
#ifndef SDCARD_OFF
	BYTE b1;
	UINT cnt;

	strncpy((char *)buff,LOG_LEADOUT, LOG_LOLENGTH+1); 
	
	/* write lead-out to log file */
	b1 = f_write(&File[0], buff, LOG_LOLENGTH, &cnt);
	if ( !(b1==FR_OK) ) return b1;

	/* close log file */
	b1 = f_close(&File[0]);
	if ( !(b1==FR_OK) ) return b1;

	
	/* write lead-out to event file */
	b1 = f_write(&File[1], buff, LOG_LOLENGTH, &cnt);
	if ( !(b1==FR_OK) ) return b1;

	/* close event file */
	b1 = f_close(&File[1]);
	if ( !(b1==FR_OK) ) return b1;	
	
	logFlag = 0;
#endif
	return FR_OK;
}

/*
 * Writes data to the Log file.
 */
uint8_t logDataSet(	date_t Date, time_t Time, gps_coordinate_t Lat, gps_coordinate_t Lon, int32_t alt, int32_t height,
					uint16_t speed, uint32_t dist, uint8_t satsInFix, uint8_t DOP, uint8_t fix, uint32_t debug, bool event)
{
#ifndef SDCARD_OFF
#if LOGFILETYPE == 1
	static char str_buf[100];
	uint8_t i=0;
	BYTE b1;
	UINT cnt;
	if(logFlag==0) return 1;

	// [hh:mm:ss.cc]	13
	str_buf[i++] = '[';
	ui8ToA(Time.hr, &str_buf[i], 2);
	i+=2;
	str_buf[++i] = ':';
	ui8ToA(Time.min, &str_buf[i], 2);
	i+=2;
	str_buf[++i] = ':';
	ui8ToA(Time.sec, &str_buf[i], 2);
	i+=2;
	str_buf[++i] = '.';
	ui8ToA(Time.ms, &str_buf[i], 2);
	i+=2;
	str_buf[++i] = ']';
	i++;
	
	//	_lat="ll.lllll"		15
	str_buf[i++] = ' ';
	str_buf[i++] = 'l';
	str_buf[i++] = 'a';
	str_buf[i++] = 't';
	str_buf[i++] = '=';
	str_buf[i++] = '"';	
	ui16ToA(Lat.coord_int, &str_buf[i], 2, false);
	i+=2;
	str_buf[i++] = '.';
	ui32ToA(Lat.coord_fract/100000, &str_buf[i], 5);
	i+=5;
	str_buf[i++] = '"';

	//	_lon="lll.lllll"		17
	str_buf[i++] = ' ';
	str_buf[i++] = 'l';
	str_buf[i++] = 'o';
	str_buf[i++] = 'n';
	str_buf[i++] = '=';
	str_buf[i++] = '"';	
	ui16ToA(Lon.coord_int, &str_buf[i], 3, false);
	i+=2;
	str_buf[i++] = '.';
	ui32ToA(Lon.coord_fract/100000, &str_buf[i], 5);
	i+=5;
	str_buf[i++] = '"';

	str_buf[i++] = '\r';
	str_buf[i++] = '\n';

	// write record to file
	b1 = f_write(&File[0], str_buf, i, &cnt);
	if ( !(b1==FR_OK) ) return b1;
	
#elif LOGFILETYPE == 2

	static char str_buf[100];
	uint8_t i=0;
	BYTE b1;
	UINT cnt;
	if(logFlag==0) return 1;

	// yyyy/mm/dd;	11
	str_buf[i++] = '2';
	str_buf[i++] = '0';
	ui8ToA(Date.y, &str_buf[i], 2);
	i+=2;
	str_buf[i++] = '/';
	ui8ToA(Date.m, &str_buf[i], 2);
	i+=2;
	str_buf[i++] = '/';
	ui8ToA(Date.d, &str_buf[i], 2);
	i+=2;
	str_buf[i++] = CSV_DELIMITER;
	
	// hh:mm:ss.c;	11
	ui8ToA(Time.hr, &str_buf[i], 2);
	i+=2;
	str_buf[i++] = ':';
	ui8ToA(Time.min, &str_buf[i], 2);
	i+=2;
	str_buf[i++] = ':';
	ui8ToA(Time.sec, &str_buf[i], 2);
	i+=2;
	str_buf[i++] = '.';
	ui8ToA(Time.ms, &str_buf[i], 1);
	i+=1;
	str_buf[i++] = CSV_DELIMITER;
	
	//	Latitude: ll.lllll;		9
	ui16ToA(Lat.coord_int, &str_buf[i], 2, false);
	i+=2;
	str_buf[i++] = '.';
	ui32ToA(Lat.coord_fract, &str_buf[i], 5);
	i+=5;
	str_buf[i++] = CSV_DELIMITER;
	str_buf[i++] = Lat.NSEW;
	str_buf[i++] = CSV_DELIMITER;


	//	Longitude: lll.lllll;		10
	ui16ToA(Lon.coord_int, &str_buf[i], 3, false);
	i+=3;
	str_buf[i++] = '.';
	ui32ToA(Lon.coord_fract, &str_buf[i], 5);
	i+=5;
	str_buf[i++] = CSV_DELIMITER;
	str_buf[i++] = Lon.NSEW;
	str_buf[i++] = CSV_DELIMITER;
	
	//	Altitude: -aaaa.a;		7-8
	if(alt<0) { str_buf[i++] = '-'; alt*= -1; }
	ui32ToA(alt, &str_buf[i], 5);
	str_buf[i+5] = str_buf[i+4];
	str_buf[i+4] = '.';
	i+=6;
	str_buf[i++] = CSV_DELIMITER;

	//	Height: -hhhh.h;		7-8
	if(height<0) { str_buf[i++] = '-'; height*= -1; }
	ui32ToA(height, &str_buf[i], 5);
	str_buf[i+5] = str_buf[i+4];
	str_buf[i+4] = '.';
	i+=6;
	str_buf[i++] = CSV_DELIMITER;


	//	Speed: sss.s;		6
	ui16ToA(speed, &str_buf[i], 4, false);
	str_buf[i+4] = str_buf[i+3];
	str_buf[i+3] = '.';
	i+=5;
	str_buf[i++] = CSV_DELIMITER;

	//	Distance: dddd.d;		7
	ui32ToA(dist, &str_buf[i], 5);
	str_buf[i+5] = str_buf[i+4];
	str_buf[i+4] = '.';
	i+=6;
	str_buf[i++] = CSV_DELIMITER;

	// nn;				3
	ui8ToA(satsInFix, &str_buf[i], 2);
	i+=2;
	str_buf[i++] = CSV_DELIMITER;

	// hh,h;			5
	ui8ToA(DOP, &str_buf[i], 3);
	str_buf[i+3] = str_buf[i+2];
	str_buf[i+2] = '.';
	i+=4;
	str_buf[i++] = CSV_DELIMITER;

	// f/f;			4
	ui8ToA(fix & 0x0F, &str_buf[i++], 1);
	str_buf[i++] = '/';
	ui8ToA((fix & 0xF0)>>4, &str_buf[i++], 1);
	str_buf[i++] = CSV_DELIMITER;

	if(conf.logDebug)
	{
		// 0xHH;			5	Debug
		str_buf[i++] = '[';
		//str_buf[i++] = 'x';
		//ui32toX(debug, &str_buf[i], 2);
		//i+=2;
		if(debug & GPS_CALC_C) str_buf[i++] = 'C';
		if(debug & GPS_CALC_D) str_buf[i++] = 'D';
		if(debug & GPS_CALC_AU) str_buf[i++] = '+';
		if(debug & GPS_CALC_AD) str_buf[i++] = '-';
		str_buf[i++] = ']';
		str_buf[i++] = CSV_DELIMITER;
	}
	//	<cr><lf>		2
	str_buf[i++] = '\r';
	str_buf[i++] = '\n';

	// write line to file
	b1 = f_write(&File[event?1:0], str_buf, i, &cnt);
	if ( !(b1==FR_OK) ) return b1;
#endif	/* LOGFILETYPE */
#endif	/* SDCARD_OFF */
	return 0;
}


/* passes the current time to the fatfs library */
inline uint32_t sd_fattime(void)
{
	time_t tmpT = time();
	date_t tmpD = date();
	
		return  ((tmpD.y+2000-1980) << 25) |	// Year = 2007
            (tmpD.m << 21) |					// Month = June
            (tmpD.d << 16) |					// Day = 5
            (tmpT.hr << 11) |					// Hour = 11
            (tmpT.min << 5) |					// Min = 38
            (tmpT.sec >> 1);					// Sec = 0      
}
