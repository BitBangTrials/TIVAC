#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "inc/hw_gpio.h"

//Include all required files and drivers for the lab.

#define TARGET_IS_BLIZZARD_RB1
#include "driverlib/rom.h"
//Enable calling the Peripheral Driver Library from ROM, saving flash space.

#ifdef DEBUG //Record file name and line number of any errors from a library API with incorrect parameters.
void_error_(char *pcFilename, uint32_t ui32Line)
{
}
#endif

uint8_t ui8LED = 4; //Store value to turn on LED at Port F pin 2 (Blue).
uint32_t ui32ADC0Value[8];
volatile uint32_t ui32TempAvg;
volatile uint32_t ui32TempValueC;
volatile uint32_t ui32TempValueF;
int main(void)
{
    //uint32_t ui32ADC0Value[8];
    //Set up an array to store our ADC output, must be greater or equal to our FIFO size to capture everything (SS0 size 8).
    //volatile uint32_t ui32TempAvg;
    //volatile uint16_t ui16TempAvg1;
    //volatile uint16_t ui16TempAvg2;
    //volatile uint32_t ui32TempValueC;
    //volatile uint32_t ui32TempValueF;
    //Set up variables to store the calculated average, degrees C, and degrees F of our temperature.
    //Two averaging variables required, total numbers being operated on exceeds 32-bit capacity.

    uint32_t ui32TimerDelay; //Set timer delay for use in ADC triggering.

    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ); //40MHz clock total.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); //Enable clock to ADC0.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1); //Enable Timer 1 peripheral.
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC); //Configure timer 1 to be periodic.

    uint32_t ui32Clock = SysCtlClockGet(); //Retrieve system clock and store in variable.
    ui32TimerDelay = (ui32Clock / 2); //Trigger at 2Hz, 0.5 second period.

    TimerLoadSet(TIMER1_BASE, TIMER_A, ui32TimerDelay -1); //Load button on-period into timer 1.
    IntEnable(INT_TIMER1A); //Enable vector associated with Timer 1A.

    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT); //Enable interrupt from Timer 1A.
    IntMasterEnable();

    TimerEnable(TIMER1_BASE, TIMER_A); //Enable and start timer 1A.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
    //Enable GPIO Port F and set Pin 2 as output.

    ADCHardwareOversampleConfigure(ADC0_BASE, 32); //Configure hardware averaging of ADC0, 32 samples.

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

    //TimerControlTrigger(TIMER1_BASE, TIMER_A, 1); //Enables the ADC trigger output, support to be added at a later time.

    while(1)
    {

        if(ui32TempValueF > 72)
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, ui8LED); //If Temperature is greater than 72F, turn on LED.
        }
        else
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0); //If temperature is not greater than 72F, turn off LED.
        }

    }
}

void Timer1IntHandler(void)
{
    // Clear the timer interrupt
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    ADCIntClear(ADC0_BASE, 0); //Clear ADC interrupt flag.
    ADCProcessorTrigger(ADC0_BASE, 0); //Trigger ADC conversion with the processor.

    while(!ADCIntStatus(ADC0_BASE, 0, false)) //Wait for ADC conversion to complete.
    {
    }
    ADCSequenceDataGet(ADC0_BASE, 0, ui32ADC0Value); //After conversion is complete, pull the data into the array.
    ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + ui32ADC0Value[4] + ui32ADC0Value[5] + ui32ADC0Value[6] + ui32ADC0Value[7] + 4) /8;
    //ui16TempAvg1 = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + 2) /4;
    //ui16TempAvg2 = (ui32ADC0Value[4] + ui32ADC0Value[5] + ui32ADC0Value[6] + ui32ADC0Value[7] + 2) /4;
    //Average the eight array temperatures, adding +4 to compensate for rounding due to integer math.
    //Two operations required due to 32-bit limitations.  May result in slight rounding errors.
    //ui32TempAvg = (ui16TempAvg1 + ui16TempAvg2 + 1) /2;
    //Calculating total average.

    ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10; //Calculate degrees C.
    ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5; //Calculate degrees F.

    if(ui32TempValueF > 72)
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, ui8LED); //If Temperature is greater than 72F, turn on LED.
    }
    else
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0); //If temperature is not greater than 72F, turn off LED.
    }

}
