/*
 * gps.h
 *
 * Created: 08.09.2015 20:02:53
 *  Author: Christoph Ringl
 *
 *   Brief: gps.h provides GPS NMEA decoding and interpreting. Data are input characterwise, supported
 * 			NMEA sentences are GGA, GSA, GSV, RMC and VTG. Also, functionality is provided to determine
 * 			maximum and average speed, distance covered and altitude upwards and downwards covered.
 */ 


#ifndef GPS_H_
#define GPS_H_

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>



#define GPS_UART_BUF_LEN	83U		/* max 80 visible text chars + carriage return + line feed + spare (zero) */
//#define GPS_UART_BAUD		115200	/* initial UART baud rate */
//#define GPS_DEBUG_SENTENCE		/* uncomment, to print Sentence identifers on debug_interface */

#define GPS_SENTENCE_GGA	"GGA"	/* Fix information */
#define GPS_SENTENCE_GSA	"GSA"	/* Overall Satellite data */
#define GPS_SENTENCE_GSV	"GSV"	/* Detailed Satellite data */
#define GPS_SENTENCE_RMC	"RMC"	/* recommended minimum data for gps */
#define GPS_SENTENCE_VTG	"VTG"	/* Vector track an Speed over the Ground */

/* Thresholds unit: 1e-10 m (e.g.: 30 means threshold 3 m) */
#define GPS_ALT_THRESHOLD	15U		/* Threshold for altitude up/down calculation. */
#define GPS_DIST_THRESHOLD	25U		/* Threshold for distance calculation. */
#define GPS_HDOP_THRESHOLD	50U		/* a HDOP mimimum of x satellites is required in order to calc. distance and altitude */

#define GPS_CALC_C			(1<<0)
#define GPS_CALC_D			(1<<1)
#define GPS_CALC_AU			(1<<2)
#define GPS_CALC_AD			(1<<3)


#define GPS_DIST_AVGFACT	4		/* multiple of 2 */

typedef struct {
	uint8_t		h;		/* Hour */
	uint8_t		m;		/* Minute */
	uint8_t		s;		/* Second */
	uint16_t	ms;		/* Millisecond */
	uint32_t	day;	/* the day starting from date 1.1.0001 representing the date */
} gps_time_t;

typedef struct {
	uint8_t		d;		/* Day */
	uint8_t		m;		/* Month */
	uint8_t		y;		/* Year starting from 2000 */
} gps_date_t;

typedef struct {
	char		NSEW;			/* North/South - East/West indicator */
	uint8_t		coord_int;		/* integer part of degree coordinates */
	uint32_t	coord_fract;	/* fractions part of degree coordinate */
} gps_coordinate_t;

/* Fix Quality Enumeration */
typedef enum gps_fix{
	invalid = 0,
	GPSFix,
	DGPSFix,
	PPS,		/* Precise Positioning Service */
	RTK,		/* Real Time Kinematic */
	FloatRTK,
	estimated,	/* dead reckoning */
	manualInput,
	Simulation
} gps_fix_e;

typedef struct {
	uint8_t				ID[32];			/* the satellite IDs in view */
	uint8_t				SNR[32];		/* signal noise ration in dB */
} gps_satdata_t;

typedef struct {
	gps_date_t			Date;
	gps_time_t			Time;
	gps_coordinate_t	Lat;			/* N-S (Breite, 0-90) */
	gps_coordinate_t	Lon;			/* E-W (Länge, 0-180) */
	gps_fix_e			GPSFixQuality;
	uint8_t				GPSFixType;		/* GPS fix type, 1=nofix, 2=2Dfix, 3=3Dfix */
	uint8_t				NumSatView;		/* Satellites in view */
	uint8_t				NumSatFix;		/* Satellites used in fix */
	uint8_t				SatsInFix[12];	/* the satellite IDs used in fix */
	gps_satdata_t *		SatsInView;		/* the satellite in view data */
	uint8_t				PDOP;			/* 1*e-10 */
	uint8_t				HDOP;			/* 1*e-10 */
	uint8_t				VDOP;			/* 1*e-10 */
	int32_t				Alt;			/* 1*e-10 Altitude above MSL */
	int32_t				Height;			/* 1*e-10 height of MSL above WGS84 */
	uint16_t			GSpeed;			/* 1*e-10 ground speed (km/h) */
} gps_nmea_data_t;

typedef struct {
	/* Natural values */
	uint16_t			spd;
	int32_t				alt;
	gps_coordinate_t	lat;
	gps_coordinate_t	lon;
	gps_time_t			time;
	/* Calculated values, can be resetted */
	uint32_t			dist;			/* distance made good */
	uint32_t			altUp;			/* altitude made good upwards */
	uint32_t			altDwn;			/* altitude made good downwards */
	uint16_t			spdMax;			/* maximum speed */
	uint16_t			spdAvg;			/* average speed */
	gps_time_t			tsr;			/* time since reset */
} gps_data_t;


/* ################### Function Prototypes ################### */

/* Initialises GPS Module functions, includes UART initialisation. */
void gps_init(uint32_t g_ui32SysClock);

/* Reset all variables to their default/initial/zero values. */
void gps_resetValues(void);

/* Reset variables that are computed and are intended to be reset by the user. */
void gps_resetComputedValues(void);

/* Calculates all data in the gps_data struct. Must be called prior to reading gps_data struct.
 * Distance must be the latest value by calling gps_computeDist(). */
uint8_t gps_computeData(void);

/* Calculates the average Speed. Must be called prior to reading gps_data struct.
 * This function is also called by gps_computeData(). */
void gps_computeAvgSpd(void);

/* Calculates the distance between the current and the previous GPS locations and
 * adds that value to the distance variable in the gps_data struct. */
void gps_computeDist(void);

/* Changes UART Baud Rate to Slow (9600 Baud). gps_init must be called in advance. */
void gps_setBaudSlow(void);

/* Changes UART Baud Rate to Slow (115200 Baud). gps_init must be called in advance. */
void gps_setBaudFast(void);

/* Must be called periodically to check the UART receiver buffer. */
uint8_t gps_checkUart(void);

/* Returns the raw nmea outputted data. */
inline gps_nmea_data_t gps_getRawData(void);

/* Returns computed navigation/position data. */
inline gps_data_t gps_getData(void);

/* Determines the distance between 2 coordinates.
 * Returns	the distance in 0.1 m
 * p1Lat	the latitude position of point 1
 * p1Lon	the longitude position of point 1
 * p2Lat	the latitude position of point 2
 * p2Lon	the longitude position of point 2 */
uint32_t gps_calcDist(gps_coordinate_t p1Lat, gps_coordinate_t p1Lon,
						gps_coordinate_t p2Lat, gps_coordinate_t p2Lon);


#endif /* GPS_H_ */
