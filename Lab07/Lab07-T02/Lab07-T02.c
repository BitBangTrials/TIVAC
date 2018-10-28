#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/uart.h"
#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "utils/uartstdio.h"
//Include all referenced functions.

#define UART_BUFFERED
#define baudrate 115200
//#define baudrate 9600

#ifdef DEBUG //Record file name and line number of any errors from a library API with incorrect parameters.
void __error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//Declaring our temperature variables as global for reference by other tools.
uint32_t ui32ADC0Value[8];
volatile uint32_t ui32TempAvg;
volatile uint32_t ui32TempValueC;
volatile uint32_t ui32TempValueF;

//Creating a temperature calculation function to be called on-demand.
//As we are dealing with slow signals, no noticeable delay is perceived.
//We only take the ADC value of the temperature when we need it.
void TemperatureCalculation(void)
    {
        ADCProcessorTrigger(ADC0_BASE, 0); //Trigger ADC conversion with the processor.
        while(!ADCIntStatus(ADC0_BASE, 0, false)) //Wait for ADC conversion to complete.
        {
        }
        ADCSequenceDataGet(ADC0_BASE, 0, ui32ADC0Value); //After conversion is complete, pull the data into the array.
        ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + ui32ADC0Value[4] + ui32ADC0Value[5] + ui32ADC0Value[6] + ui32ADC0Value[7] + 4) /8;
        //Average the eight array temperatures, adding +4 to compensate for rounding due to integer math.
        ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10; //Calculate degrees C.
        ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5; //Calculate degrees F.
    }

//This is our interrupt handler for UART, triggered on character receipt (or FIFO level reached if enabled)
//or on timeout if another character isn't received in a 32-bit period.
void UARTIntHandler(void)
{
    uint32_t ui32Status;
    uint8_t ReadCharacter;

    ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status

    UARTIntClear(UART0_BASE, ui32Status); //Clear the asserted interrupts

    while(UARTCharsAvail(UART0_BASE)) //loop while there are chars
    {
        ReadCharacter = UARTCharGetNonBlocking(UART0_BASE); //Read our input character from UART.

        UARTprintf("\f");
        UARTprintf("Enter the command: R: Red LED, G: Green LED, B: Blue LED, T: Temperature \n");
        UARTprintf("Upper-case turns LED on, lower-case turns LED off.  Enter selection: \n");


        //Check if the input character matches any of the instructions and perform the desired action.
        if(ReadCharacter == 'R')
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1); //turn on LED
            UARTprintf("Red LED on. \n");
        }
        else if(ReadCharacter == 'r')
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0); //turn off LED
            UARTprintf("Red LED off. \n");
        }
        else if(ReadCharacter == 'G')
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3); //turn on LED
            UARTprintf("Green LED on. \n");
        }
        else if(ReadCharacter == 'g')
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0); //turn off LED
            UARTprintf("Green LED off. \n");
        }
        else if(ReadCharacter == 'B')
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2); //turn on LED
            UARTprintf("Blue LED on. \n");
        }
        else if(ReadCharacter == 'b')
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0); //turn off LED
            UARTprintf("Blue LED off. \n");
        }
        else if(ReadCharacter == 't'|| ReadCharacter == 'T')
        {
            TemperatureCalculation(); //Call our ADC function to check temperature then display.
            UARTprintf("\n");
            UARTprintf("Current Device Temperature in Celsius: %i\n", ui32TempValueC);
            UARTprintf("Current Device Temperature in Fahrenheit: %i\n", ui32TempValueF);
        }
        else
        {
            UARTprintf("\n Invalid Selection.\n"); //No matching character sent, error detection.
        }
    }
}

int main(void)
{
    //Set up our system clock, 40MHz.
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    //Enable UART0 and GPIOA peripherals.  GPIO A is where the UART TX/RX are located.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    //Configure pins A0 and A1 for UART.
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    //Enable GPIOF, for on-board LED usage.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

    //Configure our UART clock based on system clock and desired baud rate, defined above.
    UARTStdioConfig(0, baudrate, SysCtlClockGet());
    //Enable all interrupts.
    IntMasterEnable();
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    //Register our UART interrupt handler so it can be used.
    UARTIntRegister(UART0_BASE, UARTIntHandler);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); //Enable clock to ADC0.

    ADCHardwareOversampleConfigure(ADC0_BASE, 32);
    //Configure hardware averaging of ADC0, 32 samples.
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    //ADC0, Sample Sequencer 0, triggered by processor, highest priority (0).

    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 3, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 5, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 6, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 7, ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 0);
    //Configure the eight steps of our sequencer.  All using ADC0 and sequencer 0, measuring the internal temperature
    //sensor, then triggering the interrupt flag when finished and signal the last conversion on sequencer 0.
    //Enable ADC after all eight steps are set.
    IntMasterEnable();

    //Print our initial instructions.
    UARTprintf("\f");
    UARTprintf("Enter the command: R: Red LED, G: Green LED, B: Blue LED, T: Temperature \n");
    UARTprintf("Upper-case turns LED on, lower-case turns LED off.  Enter selection:");

    while(1) //Wait here until UART has input.
    {

    }

}
