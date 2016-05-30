/*
 * config.h
 *
 *  Created on: 28.03.2016
 *      Author: Christoph Ringl
 *
 *      Extern modules must use >> extern config_t conf; <<
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define CONF_PATH		"logs"			/* The path to the config file. Without drive letter and without leading and trailing slashes. */
#define CONF_FILENAME	"logger.cfg"	/* The name plus extension of the configuration file. */

//#define CONF_READONLY

#define DEF_DEMO			false
#define DEF_LOGDBG			true
#define DEF_LOGINTVL		10			/* 1/10 sec */
#define DEF_LOGASTART		true
#define DEF_GPSBAUD			115200		/* baud */
#define DEF_GPSALTTH		15			/* 1/10 m */
#define DEF_GPSDOPTH		50			/* 1/10 */
#define DEF_GPSDISTTH		25			/* 1/10 m */
#define DEF_DISPDIMT		300			/* 1/10 sec, default=30s */
#define DEF_DISPOFFT		(60*60*10)	/* 1/10 sec, default=1h */

#define CFG_DEMO			0
#define CFG_LOGDBG			1
#define CFG_LOGINTVL		2
#define CFG_LOGASTART		3
#define CFG_GPSBAUD			4
#define CFG_GPSALTTH		5
#define CFG_GPSDOPTH		6
#define CFG_GPSDISTTH		7
#define CFG_DISPDIMT		8
#define CFG_DISPOFFT		9

#define CFG_FIRSTEDIT		1

#ifndef CONF_READONLY
#define CFG_EDITSAVE		10
#define CFG_LASTEDIT		10
#else
#define CFG_LASTEDIT		9
#endif

#define CFG_CNT				10



#define CONF_PROCBUFFSZ	128				/* process buffer size(16,32,64,128) */
#define CONF_TRANBUFFSZ	32				/* transfer buffer size (16,32,64,128) */
#define CONF_MAXVALLEN	32				/* maximum length of config values(16,32,64,128) */

typedef struct {
	bool 		demo;				/* True, to show static demo data on display. */
	
	bool 		logDebug;			/* True, if debug data should be logged. */
	uint8_t 	logIntvl;			/* The log interval in 1/10 sec. */
	bool 		logAutoStart;		/* True, if log should be started automatically after startup and first fix. */
	
	uint32_t 	gpsUartBaud;		/* UART baud rate for GPS receiver. */
	int32_t 	gpsAltThreshold;	/* Threshold that determines when altitude up and down is summed up. */
	uint8_t 	gpsDopThreshold;	/* Threshold that determines when altitude or distance is summed up. */
	uint32_t 	gpsDistThreshold;	/* Threshold that determines when distance is summed up. */
	
	uint32_t	dispDimTime;		/* Time in 1/10 seconds until display dims. */
	uint32_t	dispOffTime;		/* Time in 1/10 seconds until display turns off. */
} config_t;

/* Initialises the configuration module. */
void conf_init(void);

/* Reads the configuration file from the SD card. */
uint8_t conf_read(void);

/* write a sample config file to SD card. */
uint8_t conf_write(void);

/* Increases the selected configuration value */
bool conf_inc(int8_t selection);
/* Decreases the selected configuration value */
bool conf_dec(int8_t selection);

#endif /* CONFIG_H_ */
