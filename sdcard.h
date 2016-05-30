/*
 * sdcard.h
 *
 *  Created on: 22.03.2016
 *      Author: Christoph Ringl
 *
 * 		 Brief: provides SD card access and logging functions.
 */

#ifndef SDCARD_H_
#define SDCARD_H_

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "time.h"
#include "gps.h"

//#define SDCARD_OFF		/* uncomment to turn SD functionality off */

/* defines chip select and sd card detect pin actions */
#define getSDCD()			(GPIOPinRead(GPIO_PORTK_BASE, GPIO_PIN_6)==(1<<6))
#define setSDCS()			GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_5, (1<<5))
#define clearSDCS()			GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_5, 0)

#define LOG_DIR				"logs"
#define LOG_FILENAME		"track"
#define EVENT_FILENAME		"events"

/* defines the type of log file to be produced: CSV, GPX */
#define LOGFILETYPE			2	/* 1=GPX, 2=CSV */

/* set this to the intro to be written to the log file */
#define LOG_LEADINGPX		"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\r\n<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" version=\"1.1\" creator=\"GPS logger\"\r\n\txmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\r\n\txsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\r\n <metadata>\r\n  <name>Log_hh:mm:ss</name>\r\n  <desc>GPS Log</desc>\r\n  <author>\r\n   <name>GPS logger</name>\r\n  </author>\r\n </metadata>\r\n <trk>\r\n  <name>Track1</name>\r\n  <desc></desc>\r\n  <trkseg>\r\n"
/* this is the length of the above string */
#define LOG_LILENGTHGPX		487
#define LOG_LEADOUTGPX		"  </trkseg>\r\n </trk>\r\n <trk>\r\n</gpx>\r\n"
#define LOG_LOLENGTHGPX		38
#define LOG_EXTGPX			".gpx"

#define LOG_LEADINCSV		"DATE,TIME,LATITUDE,N/S,LONGITUDE,E/W,ALT,HEIGHT,SPEED,DISTANCE,SATS,PDOP,FIX,DEBUG\r\n"
#define LOG_LILENGTHCSV		84
#define LOG_LEADOUTCSV		"\r\n"
#define LOG_LOLENGTHCSV		2
#define LOG_EXTCSV			".csv"
#define CSV_DELIMITER		','

#if LOGFILETYPE==1
#define LOG_LEADIN		LOG_LEADINGPX
#define LOG_LILENGTH	LOG_LILENGTHGPX
#define LOG_LEADOUT		LOG_LEADOUTGPX
#define LOG_LOLENGTH	LOG_LOLENGTHGPX
#define LOG_EXT			LOG_EXTGPX
#elif LOGFILETYPE==2
#define LOG_LEADIN		LOG_LEADINCSV
#define LOG_LILENGTH	LOG_LILENGTHCSV
#define LOG_LEADOUT		LOG_LEADOUTCSV
#define LOG_LOLENGTH	LOG_LOLENGTHCSV
#define LOG_EXT			LOG_EXTCSV
#endif

/* Initialises SD card functions. */
void sd_init(uint32_t sd_g_ui32SysClock);

/* Returns the process ID to write on the SPI bus. */
inline uint8_t getSDSPIProcess(void);

/* 100ms tick function must be called every 10ms to ensure correct time and stopwatch function. */
void sd_tick10Ms(void);

/* SD Init */
uint8_t sd_initCard(void);

/* Returns the next unused Log File Name. */
char * log_getNextID(char * buffer, time_t time, date_t date, bool event);

/* Opens file and starts logging. */
uint8_t log_Start(char * logFilename, char * eventFilename);

/* Stops logging and closes file. */
uint8_t log_Stop(void);

/* Writes data to the Log file. */
uint8_t logDataSet(	date_t Date, time_t Time, gps_coordinate_t Lat, gps_coordinate_t Lon, int32_t alt, int32_t height,
					uint16_t speed, uint32_t dist, uint8_t satsInFix, uint8_t DOP, uint8_t fix, uint32_t debug, bool event);

/* Returns true, if a SD card is in the socket */
bool sd_inserted(void);

/* Returns true, if a SD card is initialised. */
bool sd_initialised(void);

/* passes the current time to the fatfs library */
inline uint32_t sd_fattime(void);

#endif /* SDCARD_H_ */

