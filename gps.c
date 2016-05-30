/*
 * gps.c
 *
 * Created: 08.09.2015 20:02:45
 *  Author: Christoph Ringl
 */ 

#include "gps.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <math.h>

#include "inc/hw_types.h"

#include "debug.h"
#include "uart.h"
#include "config.h"



extern config_t conf;

/* ################### internal variables ################### */

uint8_t uart_gps;

char gps_uart_buffer[GPS_UART_BUF_LEN];
uint8_t gps_uart_ptr = 0;

gps_nmea_data_t 	nmea_data;			/* raw nmea data */
gps_data_t 			gps_data;			/* computed gps data */
gps_coordinate_t 	prevLat, prevLon;	/* previous coordinates for distance calculation */
gps_coordinate_t 	sumLat, sumLon;		/*  */
int32_t 			prevAlt;			/* previoua altitude for alt up/down calculation */
gps_time_t 			avgStartTime;		/* start time for avg speed calculation */

gps_satdata_t 		sats1[32], sats2[32];	/*  */
gps_satdata_t *		psatsRead;
gps_satdata_t *		psatsWrite;

/* ################### private function prototypes ################### */

/* Set Time Since Reset (TSR) */
void setTsr(void);

/* Adds chars to a message string queue until a NMEA message is present.
 * The message is decoded and its content stored in the raw nmea_data struct. */
uint8_t processUartData(char c);

/* Converts a 1 or 2 digit unsigned string to a unsigned 8-bit integer. */
uint8_t strToSat(char * str, uint8_t len);
/* Converts a 1-4 signed string with 0 or 1 decimal places to a 32-bit signed integer. */
int32_t strToDec(char * str, uint8_t len);
/* Converts a 6 digit string with 3 optional decimal places into a time struct. */
gps_time_t * strToTime(char * str, uint8_t len, gps_time_t * time);
/* Converts a 4 or 5 digit string with 4 decimal places into a coordinate struct. */
gps_coordinate_t * strToCoo(char * str, uint8_t len, gps_coordinate_t * coo, uint8_t isLon);


/* ################### hardware dependent function definitions ################### */
/* 
 * Initialises GPS Module functions, includes UART initialisation. 
 */
void gps_init(uint32_t g_ui32SysClock)
{
	/* Reset values */
	gps_resetValues();
	psatsRead = sats2;
	psatsWrite = sats1;
	nmea_data.SatsInView = psatsWrite;

	/* UART initialisation */
	uart_gps = UART_init(6, g_ui32SysClock, conf.gpsUartBaud, (UART_CONFIG_WLEN_8|UART_CONFIG_STOP_ONE|UART_CONFIG_PAR_NONE));
}

/* Changes UART Baud Rate to Slow (9600 Baud). gps_init must be called in advance. */
void gps_setBaudSlow(void)
{
	
}

/* Changes UART Baud Rate to Slow (115200 Baud). gps_init must be called in advance. */
void gps_setBaudFast(void)
{
	
}

/* ################### hardware independent function definitions ################### */
/* 
 * Reset all variables to their default/initial/zero values.
 */
void gps_resetValues(void)
{
	gps_data.alt = 0;
	gps_data.lat.NSEW = '-';
	gps_data.lat.coord_fract = 0;
	gps_data.lat.coord_int = 0;
	gps_data.lon.NSEW = '-';
	gps_data.lon.coord_fract = 0;
	gps_data.lon.coord_int = 0;
	gps_data.spd = 0;
	gps_data.time.h = 0;
	gps_data.time.m = 0;
	gps_data.time.s = 0;
	gps_data.time.ms = 0;
	gps_data.time.day = 0;

	nmea_data.Alt = 0;
	nmea_data.GPSFixQuality = invalid;
	nmea_data.GSpeed = 0;
	nmea_data.PDOP = 255;
	nmea_data.HDOP = 255;
	nmea_data.VDOP = 255;
	nmea_data.Height = 0;
	nmea_data.NumSatView = 0;
	nmea_data.NumSatFix = 0;
	nmea_data.Lat.NSEW = '-';
	nmea_data.Lat.coord_fract = 0;
	nmea_data.Lat.coord_int = 0;
	nmea_data.Lon.NSEW = '-';
	nmea_data.Lon.coord_fract = 0;
	nmea_data.Lon.coord_int = 0;
	nmea_data.Time.h = 0;
	nmea_data.Time.m = 0;
	nmea_data.Time.s = 0;
	nmea_data.Time.ms = 0;
	nmea_data.Time.day = 0;

	gps_resetComputedValues();
}

/* 
 * Reset variables that are computed and are intended to be reset by the user.
 */
void gps_resetComputedValues(void)
{
	prevAlt = gps_data.alt;
	prevLat = gps_data.lat;
	prevLon = gps_data.lon;
	sumLat = prevLat;
	sumLon = prevLon;
	avgStartTime = gps_data.time;
	gps_data.altDwn = 0;
	gps_data.altUp = 0;
	gps_data.dist = 0;
	gps_data.spdAvg = 0;
	gps_data.spdMax = 0;
	setTsr();
}

/* 
 * Set Time Since Reset (TSR)
 */
void setTsr(void)
{
	uint32_t days;
	int8_t h, m, s;

	s = (int8_t)gps_data.time.s - (int8_t)avgStartTime.s;
	m = (int8_t)gps_data.time.m - (int8_t)avgStartTime.m;
	h = (int8_t)gps_data.time.h - (int8_t)avgStartTime.h;
	days = gps_data.time.day - avgStartTime.day;

	if(s < 0)
	{
		m -= 1;
		s += 60;
	}
	if(m < 0)
	{
		h -= 1;
		m += 60;
	}
	if(h < 0)
	{
		days -= 1;
		h += 24;
	}

	gps_data.tsr.day = days;
	gps_data.tsr.h = h;
	gps_data.tsr.m = m;
	gps_data.tsr.s = s;
	gps_data.tsr.ms = 0;
}

/*
 * Calculates all data in the gps_data struct. Must be called prior to reading gps_data struct.
 * Distance must be the latest value by calling gps_computeDist().
 */
uint8_t gps_computeData(void)
{
	uint32_t tmpdist;
	uint8_t retval = 0;

	/* Coordinates */
	if(nmea_data.GPSFixType == 3 && /* update coordinates only on 3D fix */
		(gps_data.lat.coord_fract != nmea_data.Lat.coord_fract || gps_data.lon.coord_fract != nmea_data.Lon.coord_fract ||
		gps_data.lat.coord_int != nmea_data.Lat.coord_int || gps_data.lon.coord_int != nmea_data.Lon.coord_int ||
		gps_data.lat.NSEW !=  nmea_data.Lat.NSEW || gps_data.lon.NSEW !=  nmea_data.Lon.NSEW))
	{
		retval |= GPS_CALC_C;
		gps_data.lat = nmea_data.Lat;
		gps_data.lon = nmea_data.Lon;

		tmpdist = gps_calcDist(gps_data.lat, gps_data.lon, prevLat, prevLon);
		if(nmea_data.PDOP<=conf.gpsDopThreshold && tmpdist>=conf.gpsDistThreshold)
		{
			retval |= GPS_CALC_D;
			prevLat = gps_data.lat;
			prevLon = gps_data.lon;
			gps_data.dist += tmpdist;
		}
	}
	
	/* Time */
	gps_data.time = nmea_data.Time;

	/* Speed */
	gps_data.spd = nmea_data.GSpeed;
	if(gps_data.spd > gps_data.spdMax) gps_data.spdMax = gps_data.spd;
	
	/* Altitude */
	gps_data.alt = nmea_data.Alt;
	if(nmea_data.PDOP<=conf.gpsDopThreshold && gps_data.alt>(prevAlt+conf.gpsAltThreshold))		//nmea_data.HDOP<=GPS_HDOP_THRESHOLD &&
	{
		retval |= GPS_CALC_AU;
		gps_data.altUp += gps_data.alt - prevAlt;
		prevAlt = gps_data.alt;
	}
	if(nmea_data.PDOP<=conf.gpsDopThreshold && gps_data.alt<(prevAlt-conf.gpsAltThreshold))
	{
		retval |= GPS_CALC_AD;
		gps_data.altDwn += prevAlt - gps_data.alt;
		prevAlt = gps_data.alt;
	}

	setTsr();

	gps_computeDist();
	gps_computeAvgSpd();

	return retval;
}

/*
 * Calculates the distance between the current and the previous GPS locations and
 * adds that value to the distance variable in the gps_data struct.
 */
void gps_computeDist(void)
{
	//gps_data.dist += gps_calcDist(gps_data.lat, gps_data.lon, prevLat, prevLon);
	//gps_data.dist = 5000;
}

/*
 * Calculates the average Speed. Must be called prior to reading gps_data struct.
 * This function is also called by gps_computeData().
 */
void gps_computeAvgSpd(void)
{
	uint32_t dist, tsr;

	dist = (uint32_t)(gps_data.dist*36U);
	tsr = (gps_data.tsr.h*60U+gps_data.tsr.m)*60U+gps_data.tsr.s;

	gps_data.spdAvg = dist / tsr / 10U;
}

/* 
 * Must be called periodically to check the UART receiver buffer. 
 */
uint8_t gps_checkUart(void)
{
	char buffer[2];
	uint8_t getCnt;
	uint8_t retVal;

	buffer[1] = 0;

	/* GPS UART data available */
	while(UARTDataAvailable(uart_gps))
	{
		getCnt = UARTGet(uart_gps, buffer, 1);
		if(getCnt==1)
		{
			retVal = processUartData(buffer[0]);
			if(retVal>=1 && retVal<=5) return retVal;
		}
	}
	
	return 0;
}

/* 
 * Adds chars to a message string queue until a NMEA message is present.
 * The message is decoded and its content stored in the raw nmea_data struct.
 * c 		the character to add to the message string queue
 * Returns	An index representing the decoded NMEA message, or 
 * 			0 if not a GPS message was detected or 
 * 			255 if the message is still incomplete.
 */
uint8_t processUartData(char c)
{
	// VOLATILE NUR ZUM DEBUGGEN !!
	uint8_t ptr_start = 0, ptr_end = 0;
	uint8_t i, j, tmp, tmp2;
	static uint8_t gsvmc=0; 	/* GSV message count */
	static uint8_t gsvmn=0;		/* GSV message number */
	gps_satdata_t * tmpptr;
	
	if(c=='$') // start of new NMEA sentence, assess received sentence
	{
		if(!(gps_uart_buffer[0]=='G' && gps_uart_buffer[1]=='P')) // check first two chars, always "GP", '$' not included in buffer!
		{
			gps_uart_ptr = 0;
			return 0;
		}
		
		if(gps_uart_buffer[2]==GPS_SENTENCE_GGA[0] && gps_uart_buffer[3]==GPS_SENTENCE_GGA[1] && gps_uart_buffer[4]==GPS_SENTENCE_GGA[2])
		{
			i = 0;
			
			while(gps_uart_buffer[i]!=',') {i++;}
			
			// Time
			ptr_start = ++i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			strToTime(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1, &(nmea_data.Time));
			
			// Lat
			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			strToCoo(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1, &(nmea_data.Lat), 0);
			nmea_data.Lat.NSEW = gps_uart_buffer[i];
			i += 2;
			
			// Lon
			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			strToCoo(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1, &(nmea_data.Lon), 1);
			nmea_data.Lon.NSEW = gps_uart_buffer[i];
			i += 2;
			
			// Fix Quality
			nmea_data.GPSFixQuality = (gps_fix_e)(gps_uart_buffer[i] - '0');
			i += 2;
			
			// Number of satellites in view
			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			//nmea_data.NumSatView = strToSat(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1);
			
			// HDOP
			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			if(nmea_data.GPSFixType==2 || nmea_data.GPSFixType==3)
				{ nmea_data.HDOP = strToDec(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1); }
			else
				{ nmea_data.HDOP = 255; }

			// Altitude
			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			nmea_data.Alt = strToDec(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1);
			i += 2;	// skip field 'M'
			
			// Height
			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			nmea_data.Height = strToDec(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1);
			i += 2;
			
			gps_uart_ptr = 0;
#ifdef GPS_DEBUG_SENTENCE
			debug_print((char *)"GGA\r\n");
#endif
			return 1;
		}
		else if(gps_uart_buffer[2]==GPS_SENTENCE_GSA[0] && gps_uart_buffer[3]==GPS_SENTENCE_GSA[1] && gps_uart_buffer[4]==GPS_SENTENCE_GSA[2])
		{
			i = 0;
			j = 0;		// sat index

			while(gps_uart_buffer[i]!=',') {i++;}
			i++;

			// 1: 2D/3D Mode A(utomatic), M(anual)
			while(gps_uart_buffer[i]!=',') {i++;}
			i++;

			// 2: mode: 1=no fix, 2=2D fix, 3=3D fix
			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			nmea_data.GPSFixType = strToSat(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1);

			nmea_data.NumSatFix = 0;
			for(j=0;j<12;j++)
			{
				ptr_start = i;							// 3-14: sat ID
				while(gps_uart_buffer[i]!=',') {i++;}
				ptr_end = i++ - 1;
				if(ptr_end>=ptr_start)
				{
					nmea_data.SatsInFix[j] = strToSat(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1);
					nmea_data.NumSatFix++;
				}
			}

			// PDOP
			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			if(nmea_data.GPSFixType==2 || nmea_data.GPSFixType==3)
				{ nmea_data.PDOP = strToDec(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1); }
			else
				{ nmea_data.PDOP = 255; }

			// HDOP
			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			if(nmea_data.GPSFixType==2 || nmea_data.GPSFixType==3)
				{ nmea_data.HDOP = strToDec(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1); }
			else
				{ nmea_data.HDOP = 255; }

			// VDOP
			ptr_start = i;
			while(gps_uart_buffer[i]!='*') {i++;}
			ptr_end = i++ - 1;
			if(nmea_data.GPSFixType==2 || nmea_data.GPSFixType==3)
				{ nmea_data.VDOP = strToDec(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1); }
			else
				{ nmea_data.VDOP = 255; }

			gps_uart_ptr = 0;
#ifdef GPS_DEBUG_SENTENCE
			debug_print((char *)"GSA\r\n");
#endif
			return 2;	
		}
		else if(gps_uart_buffer[2]==GPS_SENTENCE_GSV[0] && gps_uart_buffer[3]==GPS_SENTENCE_GSV[1] && gps_uart_buffer[4]==GPS_SENTENCE_GSV[2])
		{
			/*
			1    = Total number of messages of this type in this cycle
			2    = Message number
			3    = Total number of SVs in view
			4    = SV PRN number
			5    = Elevation in degrees, 90 maximum
			6    = Azimuth, degrees from true north, 000 to 359
			7    = SNR, 00-99 dB (null when not tracking)
			8-11 = Information about second SV, same as field 4-7
			12-15= Information about third SV, same as field 4-7
			16-19= Information about fourth SV, same as field 4-7
			*/
			i = 0;

			while(gps_uart_buffer[i]!=',') {i++;}
			i++;
			
			// Message Count
			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			tmp = strToSat(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1);
			
			// Message Number
			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			tmp2 = strToSat(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1);
			
			/* Check if message count changes only when current message number is 1 and
			   if message numbers are consecutive */
			if(tmp!=gsvmc && tmp2 != 1) {gps_uart_ptr = 0; return 0;}
			gsvmc = tmp;
			if(tmp2!=1 && tmp2!=gsvmn+1) {gps_uart_ptr = 0; return 0;}
			gsvmn = tmp2;
			
			// Sats in view
			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			tmp = strToSat(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1);
			
			tmp2 = tmp - (gsvmn-1)*4;		/* Get the number of sats in the current message (4 sats/message) */
			if(tmp2>4) tmp2=4;
			
			for(j=0; j<tmp2; j++)
			{
				// Sat ID
				ptr_start = i;
				while(gps_uart_buffer[i]!=',') {i++;}
				ptr_end = i++ - 1;
				psatsWrite->ID[(gsvmn-1)*4+j] = strToSat(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1);

				// skip Elevation/Azimuth
				while(gps_uart_buffer[i]!=',') {i++;} // 1
				i++;
				while(gps_uart_buffer[i]!=',') {i++;} // 2
				i++;
			
				// SatSNR
				ptr_start = i;
				while(gps_uart_buffer[i]!=(j==3?'*':',')) {i++;}
				ptr_end = i++ - 1;
				psatsWrite->SNR[(gsvmn-1)*4+j] = strToSat(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1);
			}
		
			
			if(gsvmn==gsvmc)
			{
				nmea_data.NumSatView = tmp;
				tmpptr = psatsRead;
				psatsRead = psatsWrite;
				psatsWrite = tmpptr;
			}
			
			gps_uart_ptr = 0;
#ifdef GPS_DEBUG_SENTENCE
			debug_print((char *)"GSV\r\n");
#endif
			return 3;		
		}
		else if(gps_uart_buffer[2]==GPS_SENTENCE_RMC[0] && gps_uart_buffer[3]==GPS_SENTENCE_RMC[1] && gps_uart_buffer[4]==GPS_SENTENCE_RMC[2])
		{
			i = 0;

			while(gps_uart_buffer[i]!=',') {i++;}	// 1
			i++;
			while(gps_uart_buffer[i]!=',') {i++;}	// 2
			i++;
			while(gps_uart_buffer[i]!=',') {i++;}	// 3
			i++;
			while(gps_uart_buffer[i]!=',') {i++;}	// 4
			i++;
			while(gps_uart_buffer[i]!=',') {i++;}	// 5
			i++;
			while(gps_uart_buffer[i]!=',') {i++;}	// 6
			i++;
			while(gps_uart_buffer[i]!=',') {i++;}	// 7
			i++;
			while(gps_uart_buffer[i]!=',') {i++;}	// 8
			i++;
			while(gps_uart_buffer[i]!=',') {i++;}	// 9
			i++;

			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			if((ptr_end - ptr_start + 1)==6)
			{
				nmea_data.Date.d = strToSat(&gps_uart_buffer[ptr_start], 2);
				nmea_data.Date.m = strToSat(&gps_uart_buffer[ptr_start+2], 2);
				nmea_data.Date.y = strToSat(&gps_uart_buffer[ptr_start+4], 2);
			}

			gps_uart_ptr = 0;
#ifdef GPS_DEBUG_SENTENCE
			debug_print((char *)"RMC\r\n");
#endif
			return 4;		
		}
		else if(gps_uart_buffer[2]==GPS_SENTENCE_VTG[0] && gps_uart_buffer[3]==GPS_SENTENCE_VTG[1] && gps_uart_buffer[4]==GPS_SENTENCE_VTG[2])
		{
			// $GPVTG,054.7,T,034.4,M,,N,010.2,K*48
			i = 0;
			
			while(gps_uart_buffer[i]!=',') {i++;} // 1
			i++;
			while(gps_uart_buffer[i]!=',') {i++;} // 2
			i++;
			while(gps_uart_buffer[i]!=',') {i++;} // 3
			i++;
			while(gps_uart_buffer[i]!=',') {i++;} // 4
			i++;
			while(gps_uart_buffer[i]!=',') {i++;} // 5
			i++;
			while(gps_uart_buffer[i]!=',') {i++;} // 6
			i++;
			while(gps_uart_buffer[i]!=',') {i++;} // 7
			i++;
				
			ptr_start = i;
			while(gps_uart_buffer[i]!=',') {i++;}
			ptr_end = i++ - 1;
			nmea_data.GSpeed = strToDec(&gps_uart_buffer[ptr_start], ptr_end - ptr_start + 1);

			gps_uart_ptr = 0;
#ifdef GPS_DEBUG_SENTENCE
			debug_print((char *)"VTG\r\n");
#endif
			return 5;
		}
		else // unknown NMEA sentence
		{
			
		}
		gps_uart_ptr = 0;
	}
	else // write received char to buffer
	{
		gps_uart_buffer[gps_uart_ptr] = c;
		if(gps_uart_ptr < GPS_UART_BUF_LEN) gps_uart_ptr++;
		return 255;
	}
	
	return 0;
}

/* 
 * Returns the raw nmea outputted data. 
 */
inline gps_nmea_data_t gps_getRawData(void)
{
	return nmea_data;
}

/* 
 * Returns computed navigation/position data. 
 */
inline gps_data_t gps_getData(void)
{
	return gps_data;
}

/* 
 * Converts a 6 digit string with 3 optional decimal places into a time struct.
 * Format: HHMMSS.mmm
 */
gps_time_t * strToTime(char * str, uint8_t len, gps_time_t * time)
{
	if(len < 6) return time;	// ensure minimum length
	
	time->h = (str[0] - '0') * 10 + str[1] - '0';
	time->m = (str[2] - '0') * 10 + str[3] - '0';
	time->s = (str[4] - '0') * 10 + str[5] - '0';
	
	if(str[6] == '.' && len >= 10)
	{
		time->ms = (str[7] - '0') * 100 + (str[8] - '0') * 10 + str[9] - '0';
	}
	else
	{
		time->ms = 0;
	}
	
	return time;
}

/* 
 * Converts a 4 or 5 digit string with 4 decimal places into a coordinate struct.
 * Format isLon=0:  DDMM.MMMM
 *        isLon=1: DDDMM.MMMM
 * typical clock cycles (isLon=0): 387
 * typical clock cycles (isLon=1): 398
 */
gps_coordinate_t * strToCoo(char * str, uint8_t len, gps_coordinate_t * coo, uint8_t isLon)
{
	uint32_t tmp;
	uint8_t ptr = isLon;
	
	if(ptr > 0) ptr = 1;
	if(len < (ptr+9))
	{
		return coo;
	}
	
	/* Convert Int part for Lat or Lon coordinates */
	if(isLon)
	{
		coo->coord_int = (str[0] - '0') * 100 + (str[1] - '0') * 10 + (str[2] - '0');
	}
	else
	{
		coo->coord_int = (str[0] - '0') * 10 + (str[1] - '0');
	}
	
	ptr += 2;
	
	/* Convert Fract part */
	tmp = (str[ptr++] - '0') * 10;
	tmp += str[ptr++] - '0';
	coo->coord_fract = (tmp * 426666 + 128) / 256;		// see Excel sheet for details
	
	ptr++;
	tmp = (str[ptr++] - '0') * 1000;
	tmp += (str[ptr++] - '0') * 100;
	tmp += (str[ptr++] - '0') * 10;
	tmp += str[ptr++] - '0';
	
	coo->coord_fract += (tmp * 10923 + 32768) / 65536;	// see Excel sheet for details
	
	return coo;
}

/* 
 * Converts a 1 or 2 digit unsigned string to a unsigned 8-bit integer.
 * Format: NN
 */
uint8_t strToSat(char * str, uint8_t len)
{
	if(len==2)
	return (str[0] - '0') * 10 + (str[1] - '0');
	else if (len==1)
	return (str[0] - '0');
	else
	return 0;
}

/* 
 * Converts a 1-4 signed string with 0 or 1 decimal places to a 32-bit signed integer.
 * Format:  ###0.0
 *         -###0.0
 *		    ###0
 *		   -###0		0.0
 */
int32_t strToDec(char * str, uint8_t len)
{
	int32_t num = 0;
	uint8_t ptr;
	int8_t dec = -1;
	
	for(ptr=0; ptr<len; ptr++)
	{
		if(str[ptr]=='.') dec = ptr;
	}
	if(dec==-1) dec=len; 
	
	if(str[0]=='-') ptr = 1; else ptr = 0;
	for( ;ptr<dec; ptr++)
	{
		num *= 10;
		num += str[ptr] - '0';
	}
	
	if(dec<len)
	{
		num *= 10;
		num += str[dec+1] - '0';
	}
	
	if(str[0]=='-')
	{
		return -num;
	}
	else
	{
		return num;
	}
}

/* 
 * Determines the distance between 2 coordinates.
 * Returns	the distance in 0.1 m
 * p1Lat	the latitude position of point 1
 * p1Lon	the longitude position of point 1
 * p2Lat	the latitude position of point 2
 * p2Lon	the longitude position of point 2 
 */
#define		D_EARTH		12742001		/* meters */
#define		M_PI		3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342

uint32_t gps_calcDist(gps_coordinate_t p1Lat, gps_coordinate_t p1Lon,
						gps_coordinate_t p2Lat, gps_coordinate_t p2Lon)
{
	//debug_startMeas();

	uint32_t dist = 0;
	double lat1, lat2, lon1, lon2, a;
	
	lat1 = (double)p1Lat.coord_int + (double)p1Lat.coord_fract/100000;
	lat2 = (double)p2Lat.coord_int + (double)p2Lat.coord_fract/100000;
	lon1 = (double)p1Lon.coord_int + (double)p1Lon.coord_fract/100000;
	lon2 = (double)p1Lon.coord_int + (double)p2Lon.coord_fract/100000;
	
    lat1 = lat1 * M_PI / 180;
    lon1 = lon1 * M_PI / 180;
    lat2 = lat2 * M_PI / 180;
    lon2 = lon2 * M_PI / 180;

	a = ((lat2-lat1)*(lat2-lat1) + cos(lat1)*cos(lat2) * (lon2-lon1)*(lon2-lon1)) / 4;
	
	dist = (uint32_t)((atan(sqrt(a)) * D_EARTH * 10) + 0.5);
	
	//debug_printMeas();

	return dist;
}
