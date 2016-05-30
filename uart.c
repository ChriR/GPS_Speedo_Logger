/*
 * uart.c
 *
 *  Created on: 23.04.2015
 *      Author: Christoph Ringl
 */
#include "inc/hw_types.h"

#include "uart.h"

/* Put Data into Read Buffer (private) */
void UARTPutRead(uint8_t UART_handler, const char *pui8Buffer);

/* Check internal, if unread data are available in the RX buffer (private) */
void CheckUARTData(uint8_t UART_handler);

/* Common Interrupt handler, processes incoming UART interrupts (private) */
void UARTIntHandler(uint8_t UARTNo, uint32_t intFlags, bool NoIsHandler);

 /* ISR for UART0 */
void UART0IntHandler(void);
void UART1IntHandler(void);
void UART6IntHandler(void);


static uint32_t g_ui32SysClock;
static uart_t uarts[8];

/* Initialises a specified UART Module including GPIO and interrupt init */
uint8_t UART_init(uint8_t UARTNo, uint32_t _g_ui32SysClock, uint32_t _ui32Baud, uint32_t _ui32Config)
{
	g_ui32SysClock = _g_ui32SysClock;
	uint8_t i = 0;
	while (uarts[i].initialized) {
		i++;
	}

	char * pBuff;

	uarts[i].UARTNo = UARTNo;
	uint32_t offset = (UARTNo<<12);
	uarts[i].ui32Base = UART0_BASE + offset;
	
	pBuff = (char*) calloc(UART_BUFF_LEN, sizeof(char));
	if (pBuff==NULL) return 255;
	uarts[i].w_start = 0;
	uarts[i].w_end = 0;
	uarts[i].w_buffer = pBuff;

	pBuff = (char*) calloc(UART_BUFF_LEN, sizeof(char));
	if (pBuff==NULL) return 255;
	uarts[i].r_start = 0;
	uarts[i].r_end = 0;
	uarts[i].r_buffer = pBuff;

	// UART specific part
	if(UARTNo==0) 
	{
		//Pins A0 & A1
		SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
		SysCtlPeripheralReset(SYSCTL_PERIPH_UART0);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

		GPIOPinConfigure(GPIO_PA0_U0RX);
		GPIOPinConfigure(GPIO_PA1_U0TX);

		GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

		UARTIntUnregister(uarts[i].ui32Base);
		UARTIntRegister(uarts[i].ui32Base, UART0IntHandler);
		IntEnable(INT_UART0);
	}
	else if(UARTNo==6)
	{
		//Pins AP & P1
		SysCtlPeripheralEnable(SYSCTL_PERIPH_UART6);
		SysCtlPeripheralReset(SYSCTL_PERIPH_UART6);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);

		GPIOPinConfigure(GPIO_PP0_U6RX);
		GPIOPinConfigure(GPIO_PP1_U6TX);

		GPIOPinTypeUART(GPIO_PORTP_BASE, GPIO_PIN_0 | GPIO_PIN_1);

		UARTIntUnregister(uarts[i].ui32Base);
		UARTIntRegister(uarts[i].ui32Base, UART6IntHandler);
		IntEnable(INT_UART6);
	}

	//UARTFIFODisable(uarts[i].ui32Base);
	UARTFIFOEnable(uarts[i].ui32Base);
	UARTFIFOLevelSet(uarts[i].ui32Base, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
	UARTTxIntModeSet(uarts[i].ui32Base, UART_TXINT_MODE_EOT); //UART_TXINT_MODE_FIFO, UART_TXINT_MODE_EOT
	UARTConfigSetExpClk(uarts[i].ui32Base, g_ui32SysClock, _ui32Baud, _ui32Config);

    UARTIntEnable(uarts[i].ui32Base, UART_INT_RX | UART_INT_RT | UART_INT_TX);

	uarts[i].initialized = true;
    return i;
}

void UARTPut(uint8_t UART_handler, const char *pui8Buffer)
{
	if (uarts[UART_handler].initialized)
	{
		IntMasterDisable();
		while(*pui8Buffer)
		{
			uarts[UART_handler].w_buffer[uarts[UART_handler].w_end] = *pui8Buffer++;
			uarts[UART_handler].w_end++;
			if (uarts[UART_handler].w_end == UART_BUFF_LEN) uarts[UART_handler].w_end = 0;
		}
		IntMasterEnable();

		UARTSend(UART_handler);
	}
}

bool UARTSend(uint8_t UART_handler)
{

	bool success = true;
	if (!uarts[UART_handler].initialized)
	{
		return true;
	}

	while(uarts[UART_handler].w_start != uarts[UART_handler].w_end)
	{

		success = UARTCharPutNonBlocking(uarts[UART_handler].ui32Base, uarts[UART_handler].w_buffer[uarts[UART_handler].w_start]);

		if (!success) break;

		/*if (!success) {
			while(UARTBusy(uarts[UART_handler].ui32Base));
			success = UARTCharPutNonBlocking(uarts[UART_handler].ui32Base, uarts[UART_handler].buffer[uarts[UART_handler].start]);
			if (!success) break;
		}*/
		uarts[UART_handler].w_start++;
		if (uarts[UART_handler].w_start >= UART_BUFF_LEN) uarts[UART_handler].w_start = 0;
	}

	return success;
}


/* UART Read functions ------------------------------------------------------ */


/* Put Data into Read Buffer (private) */
void UARTPutRead(uint8_t UART_handler, const char *pui8Buffer)
{
	if (uarts[UART_handler].initialized)
	{
		/*IntMasterDisable(); */
		while(*pui8Buffer)
		{
			uarts[UART_handler].r_buffer[uarts[UART_handler].r_end] = *pui8Buffer++;
			uarts[UART_handler].r_end++;
			if (uarts[UART_handler].r_end == UART_BUFF_LEN) uarts[UART_handler].r_end = 0;
		}
		/*IntMasterEnable(); */

		/*UARTSend(UART_handler); */
	}
}


/* Check, if unread data are have been received */
bool UARTDataAvailable(uint8_t UART_handler)
{
	CheckUARTData(UART_handler);

	return uarts[UART_handler].r_start != uarts[UART_handler].r_end;
}

/* returns number of read bytes */
uint8_t UARTGet(uint8_t UART_handler, char * buffer, uint8_t numToRead)
{
	uint16_t cnt = 0;

	if (!uarts[UART_handler].initialized || numToRead==0)
	{
		return 0;
	}

	while( (uarts[UART_handler].r_start != uarts[UART_handler].r_end)  && (cnt<numToRead))
	{
		IntMasterDisable();
		buffer[cnt] = uarts[UART_handler].r_buffer[uarts[UART_handler].r_start];
		uarts[UART_handler].r_start++;
		if (uarts[UART_handler].r_start >= UART_BUFF_LEN) uarts[UART_handler].r_start = 0;
		IntMasterEnable();
		cnt++;
	}

	return cnt;
}

/* Common Interrupt handler ----------------------------------------------- */
/* Common Interrupt handler, processes incoming UART interrupts (private) */
void UARTIntHandler(uint8_t UARTNo, uint32_t intFlags, bool NoIsHandler)
{

	uint8_t UART_handler = 255;
	if(NoIsHandler)
	{
		UART_handler = UARTNo;
	}
	else
	{
		/* Get the UART_handler from the UART Number */
		int i;
		for(i=0; i<=UART_MAX; i++)
		{
			if(uarts[i].UARTNo == UARTNo)
			{
				UART_handler = i;
				break;
			}
		}
		if (UART_handler == 255) return;
	}

	
	if(intFlags & UART_INT_RX)
	{
		char rchar[2];
		rchar[1] = 0;

		/* Loop while there are characters in the receive FIFO. */
		while(UARTCharsAvail(uarts[UART_handler].ui32Base))
		{
			rchar[0] = UARTCharGetNonBlocking(uarts[UART_handler].ui32Base);

			UARTPutRead(UART_handler, rchar);
		}
	}
	/* Send if TX Data available */
	if(intFlags & UART_INT_TX)
	{
		UARTSend(UART_handler);
	}
}

/* Check internal, if unread data are available in the RX buffer (private) */
void CheckUARTData(uint8_t UART_handler)
{
	UARTIntHandler(UART_handler, UART_INT_RX, true);
}


/* ----------------------
 * -------- ISRs --------
 */
 /* ISR for UART0 */
void UART0IntHandler(void)
{
	uint32_t ui32Status;

	/* Get the interrrupt status. */
	ui32Status = UARTIntStatus(UART0_BASE, true);

	/* Clear the asserted interrupts. */
	UARTIntClear(UART0_BASE, ui32Status);

	UARTIntHandler(0, ui32Status, false);
}

void UART1IntHandler(void)
{
	uint32_t ui32Status;

	/* Get the interrrupt status. */
	ui32Status = UARTIntStatus(UART1_BASE, true);

	/* Clear the asserted interrupts. */
	UARTIntClear(UART1_BASE, ui32Status);

	UARTIntHandler(1, ui32Status, false);
}

void UART6IntHandler(void)
{
	uint32_t ui32Status;

	/* Get the interrrupt status. */
	ui32Status = UARTIntStatus(UART6_BASE, true);

	/* Clear the asserted interrupts. */
	UARTIntClear(UART6_BASE, ui32Status);

	UARTIntHandler(6, ui32Status, false);
}

/* Convert a 16 bit unsigned integer to an ascii character array (string) */
char * int16ToA(uint16_t int16, char * buffer)
{
	uint16_t tmp, tmp2;
	uint8_t i;

	buffer[5] = 0;

	for(i=0;i<5;i++)
	{
		tmp = (uint16_t)(int16 / 10) * 10;
		tmp2 = int16 - tmp;
		buffer[4-i] = (char)tmp2 + '0';
		int16 /= 10;
	}

	return buffer;
}

/* Convert a 8 bit unsigned integer to an ascii character array (string) */
char * int8ToA(uint8_t int8, char * buffer)
{
	uint8_t tmp, tmp2;
	uint8_t i;

	buffer[3] = 0;

	for(i=0;i<3;i++)
	{
		tmp = (uint8_t)(int8 / 10) * 10;
		tmp2 = int8 - tmp;
		buffer[2-i] = (char)tmp2 + '0';
		int8 /= 10;
	}

	return buffer;
}
