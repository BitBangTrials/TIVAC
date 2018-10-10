#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
//Include all required files and drivers for the lab.

#define TARGET_IS_BLIZZARD_RB1
#include "driverlib/rom.h"
//Enable calling the Peripheral Driver Library from ROM, saving flash space.

#ifdef DEBUG //Record file name and line number of any errors from a library API with incorrect parameters.
void_error_(char *pcFilename, uint32_t ui32Line)
{
}
#endif

int main(void)
{
    uint32_t ui32ADC0Value[4];
    volatile uint32_t ui32TempAvg;
    volatile uint32_t ui32TempValueC;
    volatile uint32_t ui32TempValueF;
    //Set up an array to store our ADC output, must be greater or equal to our FIFO size to capture everything.
    //Set up variables to store the calculated average, degrees C, and degrees F of our temperature.

    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ); //40MHz clock total.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); //Enable clock to ADC0.

    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
    //ADC0, Sample Sequencer 1, triggered by processor, highest priority (0).

    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 3, ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 1);
    //Configure the four steps of our sequencer.  All using ADC0 and sequencer 1, measuring the internal temperature
    //sensor, then triggering the interrupt flag when finished and signal the last conversion on sequencer 1.
    //Enable ADC after all four steps are set.

    while(1)
    {
        ADCIntClear(ADC0_BASE, 1); //Clear ADC interrupt flag.
        ADCProcessorTrigger(ADC0_BASE, 1); //Trigger ADC conversion with the processor.

        while(!ADCIntStatus(ADC0_BASE, 1, false)) //Wait for ADC conversion to complete.
        {
        }
        ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value); //After conversion is complete, pull the data into the array.
        ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + 2) /4 ;
        //Average the four array temperatures, adding +2 to compensate for rounding due to integer math.

        ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10; //Calculate degrees C.
        ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5; //Calculate degrees F.
    }
}
