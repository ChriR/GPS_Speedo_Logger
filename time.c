/*
 * time.c
 *
 *  Created on: 19.03.2016
 *      Author: Christoph Ringl
 */

#include <stdbool.h>
#include <stdint.h>

#include "time.h"

uint8_t hr=0, min=0, sec=0, ms=0;					/* clock */
uint8_t da=1, mo=1, ye=0;							/* date, year=ye+2000 */		
uint8_t stpwHr=0, stpwMin=0, stpwSec=0, stpwMsec=0;	/* Stopwatch variables */
bool stpwRun=false;

bool sync = false;

uint8_t daInMo[] = {31,28,31,30,31,30,30,31,30,31,30,31};


time_t tmpT, tempStpw;
date_t tmpD;

/* Inrease day, month and year (private) */
inline void tick_day()
{
	da++;
	if(mo==2 && ye%4) daInMo[1]=29; else daInMo[1]=28;	/* consider leap year */
	if(da==daInMo[mo-1]+1)
	{
		da=1;
		mo++;
		if(mo==13)
		{
			mo=1;
			ye++;
		}
	}
}

/*
 * 100ms tick function must be called every 100ms to ensure correct time and stopwatch function.
 */
void time_tick100Ms(void)
{
	sync=true;

	// TIME
	ms++;
	if(ms==10)
	{
		ms=0;
		sec++;
		if(sec==60)
		{
			sec=0;
			min++;
			if(min==60)
			{
				min=0;
				hr++;
				if(hr==24)
				{
					hr=0;
					tick_day();
				}
			}
		}
	}

	// STOPWATCH
	if(stpwRun)
	{
		stpwMsec++;
		if(stpwMsec==10)
		{
			stpwMsec=0;
			stpwSec++;
			if(stpwSec==60)
			{
				stpwSec=0;
				stpwMin++;
				if(stpwMin==60)
				{
					stpwMin=0;
					stpwHr++;
					if(stpwHr==99)
					{
						stpwHr=0;
					}
				}
			}
		}
	}
}

/*
 * Returns the current time. Must be called prior to date() if date should also be retreived.
 */
time_t time(void)
{
	do
	{
		sync = false;

		tmpT.hr = hr;
		tmpT.min = min;
		tmpT.sec = sec;
		tmpT.ms = ms;
		
		tmpD.y = ye;		/* syncronize also date */
		tmpD.m = mo;
		tmpD.d = da;
	} while(sync==true); /* copy values again when interrupted by a 100ms tick event */

	return tmpT;
}

/* Returns the current date. */
date_t date(void)
{
	return tmpD;	
}

/*
 * Syncronises the time with a specific value (e.g. GPS time).
 */
void time_sync(uint8_t Ye, uint8_t Mo, uint8_t Da, uint8_t Hr, uint8_t Min, uint8_t Sec)
{
	do
	{
		sync = false;
		
		ye = Ye;
		mo = Mo;
		da = Da;

		hr = Hr;
		min = Min;
		sec = Sec;
		ms = 0;
	} while(sync==true); /* copy values again when interrupted by a 100ms tick event */
}

/*
 * Returns the current stopwatch value.
 */
time_t stpw(void)
{
	do
	{
		sync = false;

		tempStpw.hr = stpwHr;
		tempStpw.min = stpwMin;
		tempStpw.sec = stpwSec;
		tempStpw.ms = stpwMsec;
	} while(sync==true); /* copy values again when interrupted by a 100ms tick event */

	return tempStpw;
}

/*
 * Starts or resumes the stopwatch.
 */
void stpw_start(void)
{
	stpwRun = true;
}
/*
 * Stops or pauses the stopwatch.
 */
void stpw_stop(void)
{
	stpwRun = false;
}
/*
 * Resets the stopwatch to zero.
 */
void stpw_reset(void)
{
	do
	{
		sync = false;

		stpwHr = 0;
		stpwMin = 0;
		stpwSec = 0;
		stpwMsec = 0;
	} while(sync==true); /* copy values again when interrupted by a 100ms tick event */

	sync = false;
}
