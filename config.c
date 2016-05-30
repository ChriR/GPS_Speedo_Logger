/*
 * config.c
 *
 *  Created on: 28.03.2016
 *      Author: Christoph Ringl
 */


#include "config.h"
#include "sdcard.h"
#include "conversion.h"

#include "fatfs/ff.h"
#include "fatfs/diskio.h"
#include "fatfs/integer.h"

#include <string.h>


config_t conf;

/* array of pointers to strings */
const char * confstrs[] = {
	"demo",
	"logDebug",
	"logIntvl",
	"logAutoStart",
	"gpsUartBaud",
	"gpsAltThreshold",
	"gpsDopThreshold",
	"gpsDistThreshold",
	"dispDimTime",
	"dispOffTime"
};


#define CFG_SAMPLE			"demo=0\r\nlogDebug=0\r\nlogIntvl=5\r\nlogAutoStart=0\r\ngpsUartBaud=115200\r\ngpsAltThreshold=1.5\r\ngpsDopThreshold=5.0\r\ngpsDistThreshold=2.5\r\ndispDimTime=30.0\r\ndispOffTime=300.0\r\n"
#define CFG_SAMPLE_L		169


extern FATFS FatFs[_VOLUMES];		/* File system object for each logical drive */
extern FIL File[2];					/* File object */

/* Initialises the configuration module. */
void conf_init(void)
{
	conf.demo = DEF_DEMO;
	
	conf.logDebug = DEF_LOGDBG;
	conf.logIntvl = DEF_LOGINTVL;
	conf.logAutoStart = DEF_LOGASTART;
	
	conf.gpsUartBaud = DEF_GPSBAUD;
	conf.gpsAltThreshold = DEF_GPSALTTH;
	conf.gpsDopThreshold = DEF_GPSDOPTH;
	conf.gpsDistThreshold = DEF_GPSDISTTH;
	
	conf.dispDimTime = DEF_DISPDIMT;
	conf.dispOffTime = DEF_DISPOFFT;
}

/* Precesses read characters. Data length must be CONF_TRANBUFFSZ characters. */
void conf_processLine(char * str)
{
	static char wrk_buf[CONF_PROCBUFFSZ+1];
	char tmp_buf[CONF_PROCBUFFSZ+1];
	uint8_t valId;
	uint8_t i;
	char val[CONF_MAXVALLEN];
	static int16_t len = 0;
	char * chrptr;
	char * chrptrCFGSTR;
	char * chrptrEOL;
	char * chrptrEQ;

	wrk_buf[CONF_PROCBUFFSZ] = 0;

	/* add new data to work buffer */
	if(len > (CONF_PROCBUFFSZ-CONF_TRANBUFFSZ))
	{
		strncpy(tmp_buf, wrk_buf+CONF_TRANBUFFSZ, CONF_PROCBUFFSZ-CONF_TRANBUFFSZ);
		strncpy(wrk_buf, tmp_buf, CONF_PROCBUFFSZ);
	}
	strncpy(&wrk_buf[len], str, CONF_TRANBUFFSZ);
	len += CONF_TRANBUFFSZ;

	while(1)
	{
		/* check if line ending is inluded in buffer */
		chrptrEOL = strchr(wrk_buf, '\r');	/* end of line */
		if(chrptrEOL==NULL) return;		

		/* check if equal sign is included in buffer */
		chrptrEQ = strchr(wrk_buf, '=');
		if(chrptrEQ==NULL || ((chrptrEQ+1)>=chrptrEOL)) chrptrEQ=NULL;	/* proceed buff to next line ending!! */
		
		/* check if config string values are included in buffer */
		chrptrCFGSTR=NULL;
		for(i=0; i<CFG_CNT; i++)
		{
			chrptr = strstr(wrk_buf, confstrs[i]);
			if(chrptr!=NULL && chrptr<chrptrEOL)
			{
				chrptrCFGSTR = chrptr;
				valId = i;
				break;
			}
		}
		if(chrptrCFGSTR==NULL || chrptrEQ==NULL) 
		{
			/* delete invalid parts of buffer */
			len -= (chrptrEOL - wrk_buf + 1);
			strncpy(tmp_buf, chrptrEOL, CONF_PROCBUFFSZ);
			strncpy(wrk_buf, tmp_buf, CONF_PROCBUFFSZ);
			continue;
		}

		/* delete leading whitespaces */
		chrptrEQ++;
		while((chrptrEOL-chrptrEQ)>0 && (*chrptrEQ==' ')) chrptrEQ++;
		/* delete trailing whitespaces */
		chrptr = strchr(chrptrEQ, ' ');
		if(chrptr!=NULL && chrptr<chrptrEOL) chrptrEOL=chrptr;
		
		/* copy value string to val */
		if((chrptrEOL-chrptrEQ)>0)
			strncpy(val, chrptrEQ, chrptrEOL-chrptrEQ);
			
		val[chrptrEOL-chrptrEQ] = 0;
		
		/* evaluate config string values */
		switch(valId)
		{
		case CFG_DEMO:
			conf.demo = aToBool(val, DEF_DEMO);
			break;
		case CFG_LOGDBG:
			conf.logDebug = aToBool(val, DEF_LOGDBG);
			break;
		case CFG_LOGINTVL:
			conf.logIntvl = (uint8_t)(axp1ToUi32(val, DEF_LOGINTVL*10) / 1U);
			if(conf.logIntvl>255 || conf.logIntvl<5) conf.logIntvl = DEF_LOGINTVL;
			break;
		case CFG_LOGASTART:
			conf.logAutoStart = aToBool(val, DEF_LOGASTART);
			break;
		case CFG_GPSBAUD:
			conf.gpsUartBaud = (axp1ToUi32(val, DEF_GPSBAUD*10) / 10U);
			if(conf.gpsUartBaud>921600U || conf.gpsUartBaud<2400U) conf.gpsUartBaud = DEF_GPSBAUD;
			break;
		case CFG_GPSALTTH:
			conf.gpsAltThreshold = (int32_t)(axp1ToUi32(val, DEF_GPSALTTH));
			if(conf.gpsAltThreshold>500U || conf.gpsAltThreshold<1U) conf.gpsAltThreshold = DEF_GPSALTTH;
			break;
		case CFG_GPSDOPTH:
			conf.gpsDopThreshold = (uint8_t)(axp1ToUi32(val, DEF_GPSDOPTH));
			if(conf.gpsDopThreshold>300U || conf.gpsDopThreshold<1U) conf.gpsDopThreshold = DEF_GPSDOPTH;
			break;
		case CFG_GPSDISTTH:
			conf.gpsDistThreshold = (uint32_t)(axp1ToUi32(val, DEF_GPSDISTTH));
			if(conf.gpsDistThreshold>5000U || conf.gpsDistThreshold<1U) conf.gpsDistThreshold = DEF_GPSDISTTH;
			break;
		case CFG_DISPDIMT:
			conf.dispDimTime = (uint32_t)(axp1ToUi32(val, DEF_DISPDIMT));
			if(conf.dispDimTime>(12*60*60*10)) conf.dispDimTime = DEF_DISPDIMT;
			break;
		case CFG_DISPOFFT:
			conf.dispOffTime = (uint32_t)(axp1ToUi32(val, DEF_DISPOFFT));
			if(conf.dispOffTime>(12*60*60*10)) conf.dispOffTime = DEF_DISPOFFT;
			break;
		}

		/* delete evaluated parts of buffer */
		chrptrEOL++;
		len -= (chrptrEOL - wrk_buf - 0);
		strncpy(tmp_buf, chrptrEOL, CONF_PROCBUFFSZ);
		strncpy(wrk_buf, tmp_buf, CONF_PROCBUFFSZ);
	}
}

/* Increases the selected configuration value */
bool conf_inc(int8_t selection)
{
	switch(selection)
	{
	case CFG_LOGDBG:
		conf.logDebug = !conf.logDebug;
		break;
	case CFG_LOGINTVL:
		if(conf.logIntvl < 255) conf.logIntvl += 5;
		break;
	case CFG_LOGASTART:
#ifndef CONF_READONLY
		conf.logAutoStart = !conf.logAutoStart;
#endif
		break;
	case CFG_GPSBAUD:
#ifndef CONF_READONLY
		switch(conf.gpsUartBaud){
		case 115200: break;
		case 57600: conf.gpsUartBaud=115200; break;
		case 38400: conf.gpsUartBaud=57600; break;
		case 19200: conf.gpsUartBaud=38400; break;
		case 9600: conf.gpsUartBaud=19200; break;
		case 4800: conf.gpsUartBaud=9600; break;
		}
#endif
		break;
	case CFG_GPSALTTH:
		if(conf.gpsAltThreshold < 500U) conf.gpsAltThreshold += 2;
		break;
	case CFG_GPSDOPTH:
		if(conf.gpsDopThreshold < 300U) conf.gpsDopThreshold += 2;
		break;
	case CFG_GPSDISTTH:
		if(conf.gpsDistThreshold < 5000U) conf.gpsDistThreshold += 2;
		break;
	case CFG_DISPDIMT:
		if(conf.dispDimTime < 500) conf.dispDimTime += 5;
		else if(conf.dispDimTime < 1000) conf.dispDimTime += 50;
		else if(conf.dispDimTime < (12*60*60*10)) conf.dispDimTime += 500;
		break;
	case CFG_DISPOFFT:
		if(conf.dispOffTime < 500) conf.dispOffTime += 5;
		else if(conf.dispOffTime < 1000) conf.dispOffTime += 50;
		else if(conf.dispOffTime < (12*60*60*10)) conf.dispOffTime += 500;
		break;
#ifndef CONF_READONLY
	case CFG_EDITSAVE:
		if(conf_write()!=0) return false;
		break;
#endif
	}

	return true;
}

/* Decreases the selected configuration value */
bool conf_dec(int8_t selection)
{
	switch(selection)
	{
	case CFG_LOGDBG:
		conf.logDebug = !conf.logDebug;
		break;
	case CFG_LOGINTVL:
		if(conf.logIntvl > 5) conf.logIntvl -= 5;
		break;
	case CFG_LOGASTART:
#ifndef CONF_READONLY
		conf.logAutoStart = !conf.logAutoStart;
#endif
		break;
	case CFG_GPSBAUD:
#ifndef CONF_READONLY
		switch(conf.gpsUartBaud){
		case 115200: conf.gpsUartBaud=57600; break;
		case 57600: conf.gpsUartBaud=38400; break;
		case 38400: conf.gpsUartBaud=19200; break;
		case 19200: conf.gpsUartBaud=9600; break;
		case 9600: conf.gpsUartBaud=4800; break;
		case 4800: break;
		}
#endif
		break;
	case CFG_GPSALTTH:
		if(conf.gpsAltThreshold > 1) conf.gpsAltThreshold -= 2;
		break;
	case CFG_GPSDOPTH:
		if(conf.gpsDopThreshold > 1) conf.gpsDopThreshold -= 2;
		break;
	case CFG_GPSDISTTH:
		if(conf.gpsDistThreshold > 1) conf.gpsDistThreshold -= 2;
		break;
	case CFG_DISPDIMT:
		if(conf.dispDimTime > 1000) conf.dispDimTime -= 500;
		else if(conf.dispDimTime > 500) conf.dispDimTime -= 50;
		else if(conf.dispDimTime > 0) conf.dispDimTime -= 5;
		break;
	case CFG_DISPOFFT:
		if(conf.dispOffTime > 1000) conf.dispOffTime -= 500;
		else if(conf.dispOffTime > 500) conf.dispOffTime -= 50;
		else if(conf.dispOffTime > 0) conf.dispOffTime -= 5;
		break;
	}

	return true;
}

/* Reads the configuration file from the SD card. 
 * String function are excessively used throughout this function,
 * but conf_read() should not be called periodically.
 */
uint8_t conf_read(void)
{
#ifndef SDCARD_OFF
	char str_buf[32];
	//uint8_t i=0;
	uint32_t cnt;
	BYTE b1;

	if(!sd_initialised())
	{
		// init SD Card
		b1 = sd_initCard();
		if ( !(b1==FR_OK) ) return b1;
	}

	b1 = f_mount(&FatFs[0], "0:", 1);
	if ( !(b1==FR_OK) ) return b1;

	b1 = f_chdir(CONF_PATH);
	if(b1==FR_NO_PATH)
	{
		b1 = f_mkdir(CONF_PATH);
		if ( !(b1==FR_OK) ) return b1;
		b1 = f_chdir(CONF_PATH);
	}
	if ( !(b1==FR_OK) ) return b1;

	b1 = f_open(&File[1], CONF_FILENAME, FA_OPEN_EXISTING | FA_READ);
	if ( b1==FR_NO_FILE ) conf_write();
	else if ( !(b1==FR_OK) ) return b1;

	/* read config file */
	do {
		b1 = f_read(&File[1], str_buf, 32, &cnt);
		if(b1!=FR_OK) return b1;
		if(cnt<32) { str_buf[cnt] = 0; }
		conf_processLine(str_buf);
	} while (cnt==32);

	b1 = f_close(&File[1]);
	if ( !(b1==FR_OK) ) return b1;

	return FR_OK;
#else
	return FR_OK;
#endif	/* SDCARD_OFF */
}


void shiftDecLeft (char * str, uint8_t Len)
{
	str[Len+1] = str[Len+0];
	str[Len+0] = str[Len-1];
	str[Len-1] = '.';
}

/* write a sample config file to SD card. */
uint8_t conf_write(void)
{
#ifndef SDCARD_OFF
#ifndef CONF_READONLY
	char str_buf[250];
	uint16_t len;
	BYTE b1;
	UINT cnt;

	/* check if card is initialised, initialise if necessary */
	if(!sd_initialised())
	{
		// init SD Card
		b1 = sd_initCard();
		if ( !(b1==FR_OK) ) return b1;
	}

	/* mount file system */
	b1 = f_mount(&FatFs[0], "0:", 1);
	if ( !(b1==FR_OK) ) return b1;

	/* change to configuration directory */
	b1 = f_chdir(CONF_PATH);
	if(b1==FR_NO_PATH)
	{
		b1 = f_mkdir(CONF_PATH);
		if ( !(b1==FR_OK) ) return b1;
		b1 = f_chdir(CONF_PATH);
	}
	if ( !(b1==FR_OK) ) return b1;

	// open file for write
	b1 = f_open(&File[0], CONF_FILENAME, FA_WRITE | FA_CREATE_ALWAYS);
	if ( !(b1==FR_OK) ) return b1;

	/* Write configuration to buffer */
	/*"demo=0\r\n*/
	len = 0;
	strncpy((char *)(&str_buf[len]), "demo=0", 6);
	len += 6;
	/* logDebug=0\r\n*/
	strncpy((char *)(&str_buf[len]), "\r\nlogDebug=", 11);
	len += 11;
	if(conf.logDebug) str_buf[len] = '1'; else str_buf[len] = '0';
	len += 1;
	/* logIntvl=5\r\n*/
	strncpy((char *)(&str_buf[len]), "\r\nlogIntvl=", 11);
	len += 11;
	ui8ToA(conf.logIntvl, &str_buf[len], 3);
	shiftDecLeft(&str_buf[len], 3);
	len += 4;
	/* logAutoStart=0\r\n*/
	strncpy((char *)(&str_buf[len]), "\r\nlogAutoStart=", 15);
	len += 15;
	if(conf.logAutoStart) str_buf[len] = '1'; else str_buf[len] = '0';
	len += 1;
	/* gpsUartBaud=115200\r\n*/
	strncpy((char *)(&str_buf[len]), "\r\ngpsUartBaud=", 14);
	len += 14;
	ui32ToA(conf.gpsUartBaud, &str_buf[len], 6);
	len += 6;
	/* gpsAltThreshold=1.5\r\n 500 */
	strncpy((char *)(&str_buf[len]), "\r\ngpsAltThreshold=", 18);
	len += 18;
	ui16ToA(conf.gpsAltThreshold, &str_buf[len], 3, false);
	shiftDecLeft(&str_buf[len], 3);
	len += 4;
	/* gpsDopThreshold=5.0\r\n 5000*/
	strncpy((char *)(&str_buf[len]), "\r\ngpsDopThreshold=", 18);
	len += 18;
	ui16ToA(conf.gpsDopThreshold, &str_buf[len], 4, false);
	shiftDecLeft(&str_buf[len], 4);
	len += 5;
	/* gpsDistThreshold=2.5\r\n*/
	strncpy((char *)(&str_buf[len]), "\r\ngpsDistThreshold=", 19);
	len += 19;
	ui16ToA(conf.gpsDistThreshold, &str_buf[len], 3, false);
	shiftDecLeft(&str_buf[len], 3);
	len += 4;
	/* dispDimTime=30.0\r\n*/
	strncpy((char *)(&str_buf[len]), "\r\ndispDimTime=", 14);
	len += 14;
	ui32ToA(conf.dispDimTime, &str_buf[len], 6);
	shiftDecLeft(&str_buf[len], 6);
	len += 7;
	/* dispOffTime=300.0\r\n"*/
	strncpy((char *)(&str_buf[len]), "\r\ndispOffTime=", 14);
	len += 14;
	ui32ToA(conf.dispOffTime, &str_buf[len], 6);
	shiftDecLeft(&str_buf[len], 6);
	len += 7;

	strncpy((char *)(&str_buf[len]), "\r\n\0", 3);
	len += 2;

	/* write buffer to file */
	b1 = f_write(&File[0], str_buf, len, &cnt);
	if ( !(b1==FR_OK) ) return b1;

	// close file
	b1 = f_close(&File[0]);
	if ( !(b1==FR_OK) ) return b1;
	
	return FR_OK;
#else
	return FR_OK;
#endif
#else
	return FR_OK;
#endif	/* SDCARD_OFF */
}
