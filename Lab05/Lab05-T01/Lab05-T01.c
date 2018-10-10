#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
//Include all required files and drivers for the lab.

#define TARGET_IS_BLIZZARD_RB1
#include "driverlib/rom.h"
//Enable calling the Peripheral Driver Library from ROM, saving flash space.

#ifdef DEBUG //Record file name and line number of any errors from a library API with incorrect parameters.
void_error_(char *pcFilename, uint32_t ui32Line)
{
}
#endif

uint32_t ui32ADC0Value[8];
volatile uint32_t ui32TempAvg;
volatile uint32_t ui32TempValueC;
volatile uint32_t ui32TempValueF;
//Set up an array to store our ADC output, must be greater or equal to our FIFO size to capture everything (SS0 size 8).
//Set up variables to store the calculated average, degrees C, and degrees F of our temperature.
const uint8_t ui8LED = 4; //Store value to turn on LED at Port F pin 2 (Blue).

int main(void)
{
    const uint8_t ui8LED = 4; //Store value to turn on LED at Port F pin 2 (Blue).

    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ); //40MHz clock total.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); //Enable clock to ADC0.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
    //Enable GPIO Port F and set Pin 2 as output.

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
    //Configure the four steps of our sequencer.  All using ADC0 and sequencer 0, measuring the internal temperature
    //sensor, then triggering the interrupt flag when finished and signal the last conversion on sequencer 0.
    //Enable ADC after all eight steps are set.

    while(1)
    {
        ADCIntClear(ADC0_BASE, 0); //Clear ADC interrupt flag.
        ADCProcessorTrigger(ADC0_BASE, 0); //Trigger ADC conversion with the processor.

        while(!ADCIntStatus(ADC0_BASE, 0, false)) //Wait for ADC conversion to complete.
        {
        }
        ADCSequenceDataGet(ADC0_BASE, 0, ui32ADC0Value); //After conversion is complete, pull the data into the array.

        ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + ui32ADC0Value[4] + ui32ADC0Value[5] + ui32ADC0Value[6] + ui32ADC0Value[7] + 4) /8;
        //Average the eight array temperatures, adding +4 to compensate for rounding due to integer math.

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
}
