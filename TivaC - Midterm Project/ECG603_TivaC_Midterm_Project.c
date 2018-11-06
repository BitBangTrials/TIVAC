#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "driverlib/interrupt.h"
#include "driverlib/hibernate.h"
#include "TSL2561_mod.h"
#include "utils/ustdlib.h"

#define key "LGCG7W8V9A9OGSMX"
//Note: Was given TSL2561, not TSL2591.  2591 is high dynamic range, 2561 is not.
void ConfigureUART(void)
//Configures the UART to run at 119200 baud rate
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);	//enables UART module 1
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);	//enables GPIO port b

	GPIOPinConfigure(GPIO_PB1_U1TX);	//configures PB1 as TX pin
	GPIOPinConfigure(GPIO_PB0_U1RX);	//configures PB0 as RX pin
	GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);	//sets the UART pin type

	UARTClockSourceSet(UART1_BASE, UART_CLOCK_PIOSC);	//sets the clock source
	UARTStdioConfig(1, 119200, 16000000);	//enables UARTstdio baud rate, clock, and which UART to use
}

// lux equation approximation without floating point calculations per TSL2561 datasheet.
//////////////////////////////////////////////////////////////////////////////
// Routine: unsigned int CalculateLux(unsigned int ch0, unsigned int ch0, int iType)
//
// Description: Calculate the approximate illuminance (lux) given the raw
// channel values of the TSL2560. The equation if implemented
// as a piece−wise linear approximation.
//
// Arguments: unsigned int iGain − gain, where 0:1X, 1:16X
// unsigned int tInt − integration time, where 0:13.7mS, 1:100mS, 2:402mS,
// 3:Manual
// unsigned int ch0 − raw channel value from channel 0 of TSL2560
// unsigned int ch1 − raw channel value from channel 1 of TSL2560
// unsigned int iType − package type (T or CS)
//
// Return: unsigned int − the approximate illuminance (lux)
//
//////////////////////////////////////////////////////////////////////////////
unsigned int CalculateLux(unsigned int iGain, unsigned int tInt, unsigned int ch0,
unsigned int ch1, int iType)
{
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// first, scale the channel values depending on the gain and integration time
// 16X, 402mS is nominal.
// scale if integration time is NOT 402 msec
unsigned long chScale;
unsigned long channel1;
unsigned long channel0;
switch (tInt)
{
case 0: // 13.7 msec
chScale = CHSCALE_TINT0;
break;
case 1: // 101 msec
chScale = CHSCALE_TINT1;
break;
default: // assume no scaling
chScale = (1 << CH_SCALE);
break;
}
// scale if gain is NOT 16X
if (!iGain) chScale = chScale << 4; // scale 1X to 16X
// scale the channel values
channel0 = (ch0 * chScale) >> CH_SCALE;
channel1 = (ch1 * chScale) >> CH_SCALE;
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// find the ratio of the channel values (Channel1/Channel0)
// protect against divide by zero
unsigned long ratio1 = 0;
if (channel0 != 0) ratio1 = (channel1 << (RATIO_SCALE+1)) / channel0;
// round the ratio value
unsigned long ratio = (ratio1 + 1) >> 1;
// is ratio <= eachBreak ?
unsigned int b, m;

switch (iType)
{
case 0: // T, FN and CL package
if ((ratio >= 0) && (ratio <= K1T))
{b=B1T; m=M1T;}
else if (ratio <= K2T)
{b=B2T; m=M2T;}
else if (ratio <= K3T)
{b=B3T; m=M3T;}
else if (ratio <= K4T)
{b=B4T; m=M4T;}
else if (ratio <= K5T)
{b=B5T; m=M5T;}
else if (ratio <= K6T)
{b=B6T; m=M6T;}
else if (ratio <= K7T)
{b=B7T; m=M7T;}
else if (ratio > K8T)
{b=B8T; m=M8T;}
break;
case 1:// CS package
if ((ratio >= 0) && (ratio <= K1C))
{b=B1C; m=M1C;}
else if (ratio <= K2C)
{b=B2C; m=M2C;}
else if (ratio <= K3C)
{b=B3C; m=M3C;}
else if (ratio <= K4C)
{b=B4C; m=M4C;}
else if (ratio <= K5C)
{b=B5C; m=M5C;}
else if (ratio <= K6C)
{b=B6C; m=M6C;}
else if (ratio <= K7C)
{b=B7C; m=M7C;}
else if (ratio > K8C)
{b=B8C; m=M8C;}
break;
}

unsigned long temp;
temp = ((channel0 * b) - (channel1 * m));
// do not allow negative lux value
if (temp < 0) temp = 0;
// round lsb (2^(LUX_SCALE−1))
temp += (1 << (LUX_SCALE - 1));
// strip off fractional portion
unsigned long lux = temp >> LUX_SCALE;
return(lux);
}

void I2C0_Init ()
//Configure/initialize the I2C0 
{
	SysCtlPeripheralEnable (SYSCTL_PERIPH_I2C0);	//enables I2C0
	SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOB);	//enable PORTB as peripheral
	GPIOPinTypeI2C (GPIO_PORTB_BASE, GPIO_PIN_3);	//set I2C PB3 as SDA
	GPIOPinConfigure (GPIO_PB3_I2C0SDA);

	GPIOPinTypeI2CSCL (GPIO_PORTB_BASE, GPIO_PIN_2);	//set I2C PB2 as SCLK
	GPIOPinConfigure (GPIO_PB2_I2C0SCL);

	I2CMasterInitExpClk (I2C0_BASE, SysCtlClockGet(), false);	//Set the clock of the I2C to ensure proper connection
	while (I2CMasterBusy (I2C0_BASE));	//wait while the master SDA is busy
}

void I2C0_Write (uint8_t addr, uint8_t N, ...)
//Writes data from master to slave
//Takes the address of the device, the number of arguments, and a variable amount of register addresses to write to
{
	I2CMasterSlaveAddrSet (I2C0_BASE, addr, false);	//Find the device based on the address given
	while (I2CMasterBusy (I2C0_BASE));

	va_list vargs;	//variable list to hold the register addresses passed

	va_start (vargs, N);	//initialize the variable list with the number of arguments

	I2CMasterDataPut (I2C0_BASE, va_arg(vargs, uint8_t));	//put the first argument in the list in to the I2C bus
	while (I2CMasterBusy (I2C0_BASE));
	if (N == 1)	//if only 1 argument is passed, send that register command then stop
	{
		I2CMasterControl (I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);
		while (I2CMasterBusy (I2C0_BASE));
		va_end (vargs);
	}
	else
	//if more than 1, loop through all the commands until they are all sent
	{
		I2CMasterControl (I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
		while (I2CMasterBusy (I2C0_BASE));
		uint8_t i;
		for (i = 1; i < N - 1; i++)
		{
			I2CMasterDataPut (I2C0_BASE, va_arg(vargs, uint8_t));	//send the next register address to the bus
			while (I2CMasterBusy (I2C0_BASE));

			I2CMasterControl (I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);	//burst send, keeps receiving until the stop signal is received
			while (I2CMasterBusy (I2C0_BASE));
		}

		I2CMasterDataPut (I2C0_BASE, va_arg(vargs, uint8_t));	//puts the last argument on the SDA bus
		while (I2CMasterBusy (I2C0_BASE));

		I2CMasterControl (I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);	//send the finish signal to stop transmission
		while (I2CMasterBusy (I2C0_BASE));

		va_end (vargs);
	}

}

uint32_t I2C0_Read (uint8_t addr, uint8_t reg)
//Read data from slave to master
//Takes in the address of the device and the register to read from
{
	I2CMasterSlaveAddrSet (I2C0_BASE, addr, false);	//find the device based on the address given
	while (I2CMasterBusy (I2C0_BASE));

	I2CMasterDataPut (I2C0_BASE, reg);	//send the register to be read on to the I2C bus
	while (I2CMasterBusy (I2C0_BASE));

	I2CMasterControl (I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);	//send the send signal to send the register value
	while (I2CMasterBusy (I2C0_BASE));

	I2CMasterSlaveAddrSet (I2C0_BASE, addr, true);	//set the master to read from the device
	while (I2CMasterBusy (I2C0_BASE));

	I2CMasterControl (I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);	//send the receive signal to the device
	while (I2CMasterBusy (I2C0_BASE));

	return I2CMasterDataGet (I2C0_BASE);	//return the data read from the bus
}

void TSL2561_init ()
//Initializes the TSL2591 to have low gain, 100ms integration.
{
	uint32_t x;
	x = I2C0_Read (TSL2561_ADDR, (TSL2561_COMMAND_BIT | TSL2561_ID));	//read the device ID
	if (x == 0x50)
	{
		//UARTprintf ("GOT IT! %i\n", x);	//used during debugging to make sure correct ID is received
	}
	else
	{
	    while (1){};		//loop here if the dev ID is not correct
	}

	I2C0_Write (TSL2561_ADDR, 2, (TSL2561_COMMAND_BIT | TSL2561_CONFIG), 0x10);	//configures the TSL2591 to have low gain and integration time of 100ms
	I2C0_Write (TSL2561_ADDR, 2, (TSL2561_COMMAND_BIT | TSL2561_ENABLE), (TSL2561_ENABLE_POWERON | TSL2561_ENABLE_AEN | TSL2561_ENABLE_AIEN | TSL2561_ENABLE_NPIEN));	//enables proper interrupts and power to work with TSL2591
}

uint32_t GetLuminosity ()
//This function will read the channels of the TSL and returns the calculated value to the caller
//Channel 0 is the broadband photodiode, over both visible and infrared.
//Channel 1 is the primarily infrared detecting photodiode, to be subtracted out for our visible approximation.
//This section could be compressed to save memory and use less variables, but is broken out
//for ease of use, as we are not currently memory-constrained.
{
    uint16_t ch0, ch1;  //variables to hold the channels of the TSL2561.
    uint32_t H0 = 0;
    uint32_t L0 = 0;
    uint32_t H1 = 0;
    uint32_t L1= 0;
    uint32_t lux = 0;

    //Read High and Low channel bytes into ch0 word.
    L0 = I2C0_Read (TSL2561_ADDR, (TSL2561_COMMAND_BIT | TSL2561_C0DATAL));
    H0 = I2C0_Read (TSL2561_ADDR, (TSL2561_COMMAND_BIT | TSL2561_C0DATAH));
    ch0 = (H0 * 256) + L0;
    //Read High and Low channel bytes into ch1 word.
    L1 = I2C0_Read (TSL2561_ADDR, (TSL2561_COMMAND_BIT | TSL2561_C1DATAL));
    H1 = I2C0_Read (TSL2561_ADDR, (TSL2561_COMMAND_BIT | TSL2561_C1DATAH));
    ch1 = (H1 * 256) + L1;

	lux = CalculateLux(0, 1, ch0, ch1, 0); //low gain, 100ms integration, channels 0 and 1, T-type package.

	return lux;
}

void main (void)
{
	char HTTP_POST[300];	//string buffer to hold the HTTP command
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);	//set the main clock to run at 40MHz
	uint32_t lux = 0, i;
	uint32_t luxAvg = 0;

	ConfigureUART ();	//configure the UART of Tiva C
	I2C0_Init ();		//initialize the I2C0 of Tiva C
	TSL2561_init ();	//initialize the TSL2591

	SysCtlPeripheralEnable (SYSCTL_PERIPH_HIBERNATE);	//enable button 2 to be used during hibernation
	HibernateEnableExpClk (SysCtlClockGet());	//Get the system clock to set to the hibernation clock
	HibernateGPIORetentionEnable ();	//Retain the pin function during hibernation
	HibernateRTCSet (0);	//Set RTC hibernation
	HibernateRTCEnable ();	//enable RTC hibernation
	HibernateRTCMatchSet (0, 30);	//hibernate for 30 seconds
	HibernateWakeSet (HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_RTC);	//allow hibernation wake up from RTC time or button 2

	for (i = 0; i < 20; i++)
	//finds the average of the lux channel to send through UART.
	{
		lux = GetLuminosity ();
		luxAvg += lux;
	}
	luxAvg = luxAvg/20;

	UARTprintf ("AT+RST\r\n");	//reset the esp8266 before pushing data
	SysCtlDelay (100000000);
	UARTprintf ("AT+CIPMUX=1\r\n");	//enable multiple send ability
	SysCtlDelay (20000000);
	UARTprintf ("AT+CIPSTART=4,\"TCP\",\"184.106.153.149\",80\r\n");	//Establish a connection with the thingspeak servers
	SysCtlDelay (50000000);

	//The following lines of code puts the TEXT with the data from the lux in to a string to be sent through UART

	usprintf (HTTP_POST, "GET https://api.thingspeak.com/update?api_key=%s&field1=%d&headers=falseHTTP/1.1\nHostapi.thingspeak.com\nConnection:close\Accept*\*\r\n\r\n", key, luxAvg);
	UARTprintf ("AT+CIPSEND=4,%d\r\n", strlen(HTTP_POST));	//command the ESP8266 to allow sending of information
	SysCtlDelay (50000000);
	UARTprintf (HTTP_POST);	//send the string of the HTTP GET to the ESP8266
	SysCtlDelay (50000000);

	HibernateRequest ();	//Hibernate
	while (1)
	{};
}
