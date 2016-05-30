/*
 * time.h
 *
 *  Created on: 19.03.2016
 *      Author: Christoph Ringl
 *
 *       Brief: time.h provides function for managing a time consisting of hours, minutes and
 *       		seconds and date. Additionally, a stopwatch function is provided with a resolution 
 *				of tenth of seconds.
 */

#ifndef TIME_H_
#define TIME_H_

#include <stdbool.h>
#include <stdint.h>


typedef struct strTime
{
	uint8_t hr, min, sec, ms;
} time_t;

typedef struct strDate
{
	uint8_t d, m, y;		/* year = YY+2000 */
} date_t;

/* 100ms tick function must be called every 100ms to ensure correct time and stopwatch function. */
void time_tick100Ms(void);

 /* Returns the current time. Must be called prior to date() if date should also be retreived. */
time_t time(void);

/* Returns the current date. */
date_t date(void);

/* Syncronises the time with a specific value (e.g. GPS time). */
void time_sync(uint8_t Ye, uint8_t Mo, uint8_t Da, uint8_t Hr, uint8_t Min, uint8_t Sec);

/* Returns the current stopwatch value. */
time_t stpw(void);

/* Starts or resumes the stopwatch. */
void stpw_start(void);

/* Stops or pauses the stopwatch. */
void stpw_stop(void);

/* Resets the stopwatch to zero. */
void stpw_reset(void);

#endif /* TIME_H_ */
