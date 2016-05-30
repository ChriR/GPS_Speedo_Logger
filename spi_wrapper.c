/*
 * spi_wrapper.c
 *
 *  Created on: 06.05.2015
 *      Author: Christoph Ringl
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c1294ncpdt.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"
#include "driverlib/ssi.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"

#include "spi_wrapper.h"


uint32_t spi_g_ui32SysClock;
uint32_t spi_freq;
uint8_t spi_data_length;


void SSIIntHandler(uint32_t ui32Base, uint32_t ui32Status);
void SSI3IntHandler(void);

inline void Deassert(void);
inline void Assert(uint8_t process);

void (* pfnCSHandler[SPI_MAXPROC*2])(void);	/* stores handler to functions to assert and deassert cs lines */
uint8_t procCount = 0;						/* number of stored processes */
volatile uint8_t trmProc = 0;				/* 0, if no transmission is in progress; otherwise the transmission (process) ID */


/* Initialise SPI (SSI3) with given clock frequency and data bit length. */
void SPI_init(uint32_t g_ui32SysClock, uint32_t freq, uint8_t data_length)
{
	spi_g_ui32SysClock = g_ui32SysClock;
	spi_freq = freq;
	spi_data_length = data_length;

	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
	
	GPIOPinConfigure(GPIO_PQ0_SSI3CLK);
	GPIOPinConfigure(GPIO_PQ2_SSI3XDAT0);
	GPIOPinConfigure(GPIO_PQ3_SSI3XDAT1);

	GPIOPinTypeSSI(GPIO_PORTQ_BASE, GPIO_PIN_0|GPIO_PIN_2|GPIO_PIN_3);

	//SSIIntClear(SSI3_BASE, SSI_TXEOT);

	SSIConfigSetExpClk(SSI3_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, freq, data_length);

	/* Enable interrupts */
	//SSIIntRegister(SSI3_BASE, SSI3IntHandler);
	//SSIIntEnable(SSI3_BASE, SSI_TXEOT);
	
	SSIEnable(SSI3_BASE);
}

/* Sets SPI bus speed to value set in SPI_init(). */
void SPI_setSlow(void)
{
	while(SSIBusy(SSI3_BASE));
	SSIDisable(SSI3_BASE);
	SSIConfigSetExpClk(SSI3_BASE, spi_g_ui32SysClock, SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, SPI_CLKSLOW, spi_data_length);
	SSIEnable(SSI3_BASE);
}
/* Sets SPI bus speed to value set in SPI_CLKSLOW. */
void SPI_setFast(void)
{
	while(SSIBusy(SSI3_BASE));
	SSIDisable(SSI3_BASE);
	SSIConfigSetExpClk(SSI3_BASE, spi_g_ui32SysClock, SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, spi_freq, spi_data_length);
	SSIEnable(SSI3_BASE);
}

/* Reserves SPI ressource so that access through this module is inhibitat for other processes. */
void SPI_reserve(uint8_t process)
{
	if(process>procCount) return;

	trmProc = process;
}

/* Releases SPI ressource so that SPI can be used by other processes again. */
void SPI_release(uint8_t process)
{
	if(process>procCount) return;
	//if(process!=trmProc) return;

	trmProc = 0;
}

/* 
 * Registers a process with CS assert and deassert functions and returns the assigned process number.
 */
uint8_t SPI_registerProc(void (* pfnAssertCS)(void), void (* pfnDeAssertCS)(void))
{
	if(procCount>=SPI_MAXPROC) return 255;
	
	/* store CS assert/deassert functions and increase and return process counter */
	procCount++;	
	
	pfnCSHandler[procCount*2] = pfnAssertCS;		/* first element is 1 so that element 0 stays empty */
	pfnCSHandler[procCount*2+1] = pfnDeAssertCS;
	
	return procCount;
}

/* 
 * Put n bytes of data in the transmit buffer and start transmission.
 */
void SPI_Put(uint8_t process, uint8_t * data, uint8_t cnt)
{
	uint8_t i;
	
	if(process>procCount) return;

	while(trmProc);
	
	/* assert CS line */
	Assert(process);

	for(i=0; i<cnt; i++)
	{SSIDataPut(SSI3_BASE, data[i]); }
	while(SSIBusy(SSI3_BASE));
	
	/* deassert CS line */
	Deassert();
}

/* 
 * Put 1 byte of data in the transmit buffer and start transmission.
 */
void SPI_Put8(uint8_t process, uint8_t data)
{
	if(process>procCount) return;

	while(trmProc);
	
	/* assert CS line */
	Assert(process);

	SSIDataPut(SSI3_BASE, data);
	while(SSIBusy(SSI3_BASE));

	/* deassert CS line */
	Deassert();
}

/* 
 * Put 2 byte of data in the transmit buffer and start transmission.
 */
void SPI_Put16(uint8_t process, uint8_t data1, uint8_t data2)
{
	if(process>procCount) return;

	while(trmProc);
	
	/* assert CS line */
	Assert(process);

	SSIDataPut(SSI3_BASE, data1);
	SSIDataPut(SSI3_BASE, data2);
	while(SSIBusy(SSI3_BASE));

	/* deassert CS line */
	Deassert();
}

/* 
 * Put 3 bytes of data in the transmit buffer and start transmission.
 */
void SPI_Put24(uint8_t process, uint8_t data1, uint8_t data2, uint8_t data3)
{
	if(process>procCount) return;

	while(trmProc);
	
	/* assert CS line */
	Assert(process);

	SSIDataPut(SSI3_BASE, data1);
	SSIDataPut(SSI3_BASE, data2);
	SSIDataPut(SSI3_BASE, data3);
	while(SSIBusy(SSI3_BASE));

	/* deassert CS line */
	Deassert();
}

/* Asserts the Chip Select line and reserves the SPI ressource. */
void SPI_SelectCS(uint8_t process)
{
	/* Check that process ID is valid and that CS wasn't selected already */
	if(process>procCount) return;
	if(process==trmProc) return;

	while(trmProc);
	
	/* assert CS line */
	Assert(process);
}

/* Deasserts the Chip Select line and releases the SPI ressource. */
void SPI_DeselectCS(uint8_t process)
{
	/* Check if current active SPI process is calling the deselect function */
	if(process!=trmProc) return;

	/* deassert CS line */
	Deassert();
}

/* Transmits and receives 1 byte of data. */
uint8_t SPI_xchg(uint8_t process, uint8_t data)
{
	if(process>procCount) return 0;
	if(process!=trmProc) return 0;
	uint8_t i;
	uint32_t ret;
	
	for(i=0; i<8; i++)	/* read 8 times to clear Rcv FIFO */
	{ SSIDataGetNonBlocking(SSI3_BASE, &ret); }
	
	SSIDataPut(SSI3_BASE, data);
	while(SSIBusy(SSI3_BASE));
	SSIDataGet(SSI3_BASE, &ret);
	//SSIDataGetNonBlocking(SSI3_BASE, &ret);
	
	return ret;
}

/* Transmits an arbitrary amount of data bytes through SPI. */
void SPI_xmit(uint8_t process, uint32_t cnt, const uint8_t * buffXmit)
{
	if(process>procCount) return;
	if(process!=trmProc) return;
	uint32_t i;
	uint32_t ret;
	uint32_t tmp;
	
	for(i=0; i<cnt; i++)
	{
		tmp = *buffXmit++;
		SSIDataPut(SSI3_BASE, tmp);
		SSIDataGet(SSI3_BASE, &ret);
	}
}

/* Receives an arbitrary amount of data bytes through SPI. */
void SPI_rcv(uint8_t process, uint32_t cnt, uint8_t * buffRcv)
{
	if(process>procCount) return;
	if(process!=trmProc) return;
	uint32_t i;
	uint32_t tmp;
	
	for(i=0; i<cnt; i++)
	{
		SSIDataPut(SSI3_BASE, 0xFF);
		SSIDataGet(SSI3_BASE, &tmp);
		buffRcv[i] = tmp;
	}
}

/* ---===  P R I V A T E  ===--- */

/* Assert CS line */
inline void Deassert(void)
{
	pfnCSHandler[trmProc*2+1]();
	trmProc = 0;
}

/* Assert CS line */
inline void Assert(uint8_t process)
{
	trmProc = process;
	pfnCSHandler[trmProc*2]();
}

/*
 * The generic SPI interrupt handler. (private)
 */
void SSIIntHandler(uint32_t ui32Base, uint32_t ui32Status)
{
	if(ui32Status & SSI_TXEOT)
	{
		/* deassert CS line */
		pfnCSHandler[trmProc*2+1]();

		trmProc = 0;
	}
}

/*
 * The SPI interrupt handler for SSI3. (private)
 */
void SSI3IntHandler(void)
{
	uint32_t ui32Status;

	/* Get the interrrupt status. */
	ui32Status = SSIIntStatus(SSI3_BASE, true);

	/* Clear the asserted interrupts. */
	SSIIntClear(SSI3_BASE, ui32Status);

	/* Call the generic SSI interrupt handler */
	SSIIntHandler(SSI3_BASE, ui32Status);
}


