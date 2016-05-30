/*
 * main.c -- GPS Tacho
 *
 *      Author: Christoph Ringl
 *
 * Pin Map:
 * 			OEM Name	Tiva Pin	IO	Info
 * 		OLED / SD (SSI3)
 * 			MOSI		Q2			O	SSI3XDAT0
 * 			MISO		Q3			I	SSI3XDAT1
 * 			SCK			Q0			O	Serial Clock
 * 			OLEDCS		K4			O	OLED Chip Select
 * 			SDCS		K5			O	SD Chip Select
 * 			CD			K6			I	SD Card Detect
 * 			DC			K7			O	OLED Data/Command
 * 			RESET		K0			O	OLED Reset
 * 		GPS (UART6)
 * 			TX			P0	RX		I	GPS Out - mC In
 * 			RX			P1	TX		O	mC Out - GPS In
 * 		Keys
 * 			L0-L3
 */


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/tm4c1294ncpdt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/systick.h"
#include "driverlib/adc.h"
#include "inc/hw_memmap.h"

#include "debug.h"
#include "sdcard.h"
#include "spi_wrapper.h"
#include "buttons.h"
#include "display.h"
#include "oled_ssd1351.h"
#include "gps.h"
#include "time.h"
#include "conversion.h"
#include "config.h"


#define SPISPEED	(10e6)

/* ------------------------------
 * G L O B A L S
 * ------------------------------
 */

extern config_t conf;				/* system configuration data */

uint32_t g_ui32SysClock;			/* system clock frequency */
uint8_t uart_a;						/* debug uart handler */


volatile uint16_t _100msFlag = 0;	/* timing flags */
volatile uint16_t _500msFlag = 0;
volatile uint16_t _5sFlag = 0;

volatile uint32_t ticks = 0;		/* 100ms ticks since system start */
uint32_t lastGPStick = 99999;
uint32_t ticksToFF = 0; 			/* 500 ms ticks to first fix */

bool rec = false;					/* true, if currently recording */

char mainbuffer[40];


/* ------------------------------
 * P R O T O T Y P E S
 * ------------------------------
 */

void GPIO_init(void);
void Timer_init(void);

void handleUARTData(char data);

void Timer0AIntHandler(void);

void Demo(void);

/* ------------------------------
 * F U N C T I O N S
 * ------------------------------
 */
void GPIO_init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1|GPIO_PIN_0);
}

void Timer_init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

	// Timer 0 konfigurieren
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock/1000);

	// Timer Interrupts
	TimerIntRegister(TIMER0_BASE, TIMER_A, Timer0AIntHandler);

	IntEnable(INT_TIMER0A);
	IntPrioritySet(INT_TIMER0A, 0x20);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	// Timer einschalten
	TimerEnable(TIMER0_BASE, TIMER_A);
}


/* ------------------------------
 * I N T E R R U P T S
 * ------------------------------
 */

/*1 ms ticks*/
void Timer0AIntHandler(void)
{
	// Interrupt Flag löschen
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	//------------------------------------------------
	// 100 ms ticks
	static int cnt=55;
	cnt++;
	if(cnt>=100){
		ticks++;
		_100msFlag++;
		time_tick100Ms();

		if (_100msFlag > 600)
		{
			_100msFlag = 0; // reset after 10sec
		}
		cnt = 0;
	}

	//------------------------------------------------
	// 500 ms ticks
	static int cnt3 = 227;
	static int ot=0;
	cnt3++;
	if (cnt3==500)
	{
		_500msFlag = 1;
		cnt3 = 0;
		ot ^= 1;
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, ot);
	}

	//------------------------------------------------
	// 10 ms ticks
	static int cnt2 = 3;
	cnt2++;
	if (cnt2>=10){
		key_debounce();		/* key debouncing */
		sd_tick10Ms();		/* sd periodic system tick */

		cnt2=0;
	}
	//------------------------------------------------
	// 5s ticks
	static int cnt4 = 0;
	cnt4++;
	if (cnt4==5000)
	{
		_5sFlag = 1;
		cnt4 = 0;
	}



}


/* ------------------------------
 * M A I N
 * ------------------------------
 */

int main(void) {
	bool stpwRun = false;			/* true, if stopwatch is running */
	time_t tmpTime, tmpStpw;		/* current time and stopwatch */
	date_t tmpDate;
	gps_nmea_data_t tmpNmea;		/* temporary raw nmea data */
	gps_data_t tmpGps;				/* temporary processed gps data */
	uint8_t selPage = 0;			/* selected display page */
	uint32_t debugCnt;
	uint8_t retval;
	uint8_t sdintvl = 0;
	uint8_t sdcalc = 0;
	bool recset = rec;
	uint32_t tmplogdist;
	uint8_t displaystate = 1;		/* 0=off, 1=on, 2=dim */
	uint32_t displaytimer = 0;
	bool config_menu = false;
	int8_t config_selected;

	/* Interrupts global on */
    IntMasterDisable();

	/* Set system frequency to 120MHz */
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);


    /* Initialise GPIOs and turn all LEDs off */
    GPIO_init();
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1|GPIO_PIN_0, 0);

    /* Initialise Debugging interface */
    debug_init(g_ui32SysClock);
    debug_RXHook(handleUARTData);

    /* Initialise Key/Button/switch debouncing functions */
    Key_init();

    /* Initialise 1ms periodic Timer (TCNT0) */
    Timer_init();
	
	/* Initialise SPI */
	SPI_init(g_ui32SysClock, SPISPEED, 8);

	/* Initialise SD card */
	sd_init(g_ui32SysClock);

    /* Initialise Config functions */
    conf_init();

    /* Read config file */
    conf_read();

	/* Interrupts global on */
    IntMasterEnable();

    /* Initialise OLED */
    display_init(g_ui32SysClock);

    /* Initialise GPS */
    gps_init(g_ui32SysClock);


    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 2);

    /* Startup Debug Message */
    debug_print((char *)"\x1B[2J\x1B[f");	/* Set cursor to home position */
    debug_print((char *)"GPS Tacho Debug Output\r\n");
    debug_print((char *)"------------------\r\n");


	/* Fill display with initial content */
    display_initContent();

    /* Apply config settings */
	if(conf.logAutoStart) recset = true;
	if(conf.demo) Demo();

    /* ---===### Infinite Loop ###===--- */
    while(1)
    {	
		/* Check if keys are pressed short or long */
    	if(Key_getState(0x0F))
    	{
    		displaytimer = 0;
    		if(displaystate==0)
    			{
    				oled_wake();
    				Key_clear(0x0F);
    			}
    		if(displaystate==0||displaystate==2)
    			{ oled_setContrast(0x0F); }
    		displaystate = 1;
    	}

    	if(config_menu)	/* currently within configuration menu, configuration button actions */
    	{
    		if(Key_getShort(1<<0))
    		{
    			if(config_selected==CFG_EDITSAVE)
    			{
    				if(conf_inc(config_selected))
    					display_conf(config_selected, sGreen);
    				else
    					display_conf(config_selected, sRed);
    			}
    			else
    			{
    				conf_inc(config_selected);
    				display_conf(config_selected, sSelection);
    			}
    		}
			if(Key_getShort(1<<1))
			{
				if(++config_selected==(1+CFG_LASTEDIT))
					config_selected=CFG_FIRSTEDIT;
					display_conf(config_selected, sSelection);
			}
    		if(Key_getLong(1<<1))
    		{
    			config_menu = false;
    			display_setPage(selPage);
    			//display_conf(-1, sSelection);
    		}
    		if(Key_getShort(1<<2))
    		{
    			conf_dec(config_selected);
    			display_conf(config_selected, sSelection);
    		}
    	}
    	else	/* NOT currently within configuration menu, standard button actions */
    	{
			if(Key_getShort(1<<0))
			{
				if(stpwRun)	stpw_stop(); else stpw_start();
				stpwRun = !stpwRun;
			}
			if(Key_getLong(1<<0))
			{
				stpw_reset();
			}
			if(Key_getLong(1<<1))
			{
				config_menu = true;
				display_setPage(3);
				config_selected = CFG_FIRSTEDIT;
				display_conf(config_selected, sSelection);
			}
			if(Key_getShort(1<<2))
			{
				if(sd_initialised() && rec)
					retval = logDataSet(tmpDate, tmpTime, tmpGps.lat, tmpGps.lon, tmpGps.alt, tmpNmea.Height,
							tmpGps.spd, tmpGps.dist - tmplogdist, tmpNmea.NumSatFix, tmpNmea.PDOP,
							((uint8_t)(tmpNmea.GPSFixType)<<4)|tmpNmea.GPSFixQuality, 0, true);
			}
			if(Key_getLong(1<<2))
			{
				recset = !recset;
			}
			if(Key_getShort(1<<3))
			{
				if(++selPage==GP_LASTLOOPPAGE+1) selPage=0;
				display_setPage(selPage);
			}
			if(Key_getLong(1<<3))
			{
				gps_resetComputedValues();
				tmplogdist = 0;
			}
    	}

		/* Perform some actions every 0.5 seconds */
    	if(_500msFlag)
    	{
    		_500msFlag = 0;
    		display_blink_fast();

    		if(lastGPStick+50 < ticks)
    		{
    			gps_resetValues();
    			display_GPS(0, 0, 99);
    		}

    		debug_startMeas();
			
			display_Time(tmpTime.hr, tmpTime.min, tmpTime.sec);
			display_GPS(tmpNmea.NumSatFix, tmpNmea.GPSFixType, tmpNmea.HDOP);

    		tmpNmea = gps_getRawData();
    		tmpGps = gps_getData();

    		/* detect first 3D fix since startup */
			if(ticksToFF==0 && (tmpNmea.GPSFixType==3) && (tmpNmea.GPSFixQuality!=0))
			{ 
				ticksToFF = ticks;
				gps_resetComputedValues();
				sdintvl=0;
			}
			
    		display_Alt((tmpGps.alt+5)/10, (tmpGps.altUp+5)/10, (tmpGps.altDwn+5)/10); /* 5s */
    		display_Speed((tmpGps.spd+5)/10, (tmpGps.spdAvg+5)/10, (tmpGps.spdMax+5)/10);
			display_Dist(tmpGps.dist/10); /* 5s */
    		
    		display_Tsr(tmpGps.tsr.day, tmpGps.tsr.h, tmpGps.tsr.m, tmpGps.tsr.s);
    		display_Satinfo(tmpNmea.NumSatView,tmpNmea.NumSatFix, tmpNmea.PDOP, tmpNmea.HDOP, tmpNmea.VDOP);
    		display_Fixinfo(tmpNmea.GPSFixType, (uint8_t)(tmpNmea.GPSFixQuality));
			display_LatLon(tmpGps.lat.coord_int, tmpGps.lat.coord_fract, tmpGps.lat.NSEW,
					tmpGps.lon.coord_int, tmpGps.lon.coord_fract, tmpGps.lon.NSEW);
			
			display_Satov(tmpNmea.NumSatView, tmpNmea.NumSatFix, tmpNmea.SatsInView->ID, tmpNmea.SatsInView->SNR, 
					tmpNmea.SatsInFix);

    		debugCnt = debug_getMeas();
    		//debug_print("\r\nD: ");
    		//debug_printMeas(debugCnt);

    		if(sd_inserted())
    		{
    			if(!sd_initialised())
    			{
    				sd_initCard();
    			}
    			display_SD(rec, 100);
    		}
    		else
    		{
    			display_SDMissing();
    		}

		}
		
		/* Perform some actions every 5 seconds */
    	if(_5sFlag)
    	{
    		_5sFlag = 0;
    		display_blink_slow();



    		display_Battery(100);

    		display_TTFF(ticksToFF/10);
    	}
		
		/* Perform some actions every 100 milliseconds */
    	if(_100msFlag)
    	{
    		_100msFlag = 0;
    		tmpStpw = stpw();

    		sdintvl++;

    		display_Stpw(tmpStpw.hr, tmpStpw.min, tmpStpw.sec, tmpStpw.ms);

    		retval = gps_checkUart();
    		if(retval)
    		{
    			lastGPStick = ticks;
				tmpNmea = gps_getRawData();
				if(retval==1)
					time_sync(tmpNmea.Date.y, tmpNmea.Date.m, tmpNmea.Date.d, tmpNmea.Time.h, tmpNmea.Time.m, tmpNmea.Time.s);

    		debug_startMeas();

    			sdcalc = gps_computeData();
    			if(!conf.logDebug) sdcalc = 0;

    		debugCnt = debug_getMeas();
    		//debug_print("\r\nG: ");
    		//debug_printMeas(debugCnt);

    			tmpGps = gps_getData();
    		}
	
			tmpTime = time();
			tmpDate = date();

    		if(displaystate!=0) displaytimer++;
    		if(displaytimer==conf.dispDimTime && conf.dispDimTime!=0 && displaystate==1 &&
    				(conf.dispOffTime>conf.dispDimTime || conf.dispOffTime==0))
    			{
    				oled_setContrast(0x05);
    				displaystate=2;
    			}
    		if(displaytimer==conf.dispOffTime && conf.dispOffTime!=0 && displaystate!=0)
    			{
    				oled_sleep();
    				displaystate=0;
    			}
		}

    	/* Check if data has been received on debug interface (USB-UART) */
    	debug_checkRX();

    	if((sdintvl>=conf.logIntvl) || sdcalc)
    	{
    		sdintvl = 0;

    		debug_startMeas();

    		if(sd_inserted())
    		{
    			if (recset && !rec && ticksToFF!=0)	/* record can be started after first fix */
    			{
    				rec=recset;
					{
						char fnmlog[50];
						char fnmevent[50];
						log_getNextID(fnmlog, tmpTime, tmpDate, false);
						log_getNextID(fnmevent, tmpTime, tmpDate, true);
						retval = log_Start(fnmlog, fnmevent);
					}
					
    				tmplogdist = tmpGps.dist;
    			}
    			if(rec)
    			{
    				retval = logDataSet(tmpDate, tmpTime, tmpGps.lat, tmpGps.lon, tmpGps.alt, tmpNmea.Height,
    						tmpGps.spd, tmpGps.dist - tmplogdist, tmpNmea.NumSatFix, tmpNmea.PDOP,
    						((uint8_t)(tmpNmea.GPSFixType)<<4)|tmpNmea.GPSFixQuality, sdcalc, false);
        			tmplogdist = tmpGps.dist;
        			sdcalc = 0;
    			}
    			if(!recset && rec)
    			{
    				rec=recset;
    				retval = log_Stop();
    			}
    		}

    		debugCnt = debug_getMeas();
    		//debug_print("\r\nS: ");
    		//debug_printMeas(debugCnt);
    	}
    }
}

/* Handle data received on debug interface (USB-UART) */
void handleUARTData(char data)
{
	//static uint8_t pin = 0;

	if (data=='s')			/* 's' starts stopwatch */
	{
		stpw_start();
	}
	else if (data=='p')		/* 'p' stops stopwatch */
	{
		stpw_stop();
	}
	else if (data=='r')		/* 'r' resets stopwatch */
	{
		stpw_reset();
	}
	else if (data=='R')		/* 'R' resets computed values (Distance, Alt made good, ...) */
	{
		gps_resetComputedValues();
	}
	else if (data=='1')		/* '1' shows page 1 */
	{
		display_setPage(0);
	}
	else if (data=='2')		/* '2' shows page 2 */
	{
		display_setPage(1);
	}
	else if (data=='3')		/* '3' shows page 3 */
	{
		display_setPage(2);
	}
	else if (data=='+')		/* '+' starts recording */
	{
		if(sd_inserted()) rec = true;
	}
	else if (data=='-')		/* '-' pauses recording */
	{
		rec = false;
	}	
	
	
}


void Demo(void)
{
	display_Alt(278, 65, 22);
	display_Speed(78, 45, 98);
	display_Dist(7239);

	display_Stpw(0, 23, 15, 4);
	display_Time(15, 12, 00);

	display_GPS(07, 3, 15);
	display_Battery(100);
	display_SD(true, 100);

	display_blink_fast();

	while(1)
	{

	}
}


