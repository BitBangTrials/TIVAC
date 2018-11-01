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
#include "Nokia5110.h"
#include "driverlib/fpu.h"
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
volatile float TempValueC;
volatile float TempValueF;
char CharF[5];
char CharC[5];

//Function to convert a float character to a character array.
//Credit to "aBUGSworstnightmare"
void ftoa(float f,char *buf)
{
    int pos=0,ix,dp,num;
    if (f<0)
    {
        buf[pos++]='-';
        f = -f;
    }
    dp=0;
    while (f>=10.0)
    {
        f=f/10.0;
        dp++;
    }
    for (ix=1;ix<8;ix++)
    {
            num = (int)f;
            f=f-num;
            if (num>9)
                buf[pos++]='#';
            else
                buf[pos++]='0'+num;
            if (dp==0) buf[pos++]='.';
            f=f*10.0;
            dp--;
    }
}

int main(void)
{
    uint32_t ui32TimerDelay; //Set timer delay for use in ADC triggering.

    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ); //40MHz clock total.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); //Enable clock to ADC0.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1); //Enable Timer 1 peripheral.
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC); //Configure timer 1 to be periodic.

    uint32_t ui32Clock = SysCtlClockGet(); //Retrieve system clock and store in variable.
    ui32TimerDelay = (ui32Clock); //Trigger at 1Hz, 1 second period.

    TimerLoadSet(TIMER1_BASE, TIMER_A, ui32TimerDelay -1); //Load on-period into timer 1.
    IntEnable(INT_TIMER1A); //Enable vector associated with Timer 1A.

    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT); //Enable interrupt from Timer 1A.
    IntMasterEnable();

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

    //Enable lazy stacking and the FPU using the ROM functions.
    FPULazyStackingEnable();
    FPUEnable();

    startSSI0(); //Enable SSI0 and allow some time to process.
    initialize_screen(BACKLIGHT_ON,SSI0); //Initialize LCD screen connected with SSI0.
    clear_screen(SSI0); //Clear previous screen contents.

    screen_write("ECG\n603!\nTemp\nSensor",ALIGN_CENTRE_CENTRE,SSI0); //Write enabling message.

    SysCtlDelay(ui32Clock * 2); //Pause for 2 seconds.
    TimerEnable(TIMER1_BASE, TIMER_A); //Enable and start timer 1A.

    //Endless loop to wait in while timer is running.
    while(1)
    {
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

    TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10; //Calculate degrees C.
    TempValueF = ((TempValueC * 9) + 160) / 5; //Calculate degrees F.

    //Convert float values into character arrays.
    ftoa(TempValueF, CharF);
    ftoa(TempValueC, CharC);

    clear_screen(SSI0); //Clear previous screen contents.
    screen_write("Temperature:\n",ALIGN_LEFT_CENTRE,SSI0); //Write temperature string, then character translations of characters.
    char_write(CharF[0],SSI0);
    char_write(CharF[1],SSI0);
    char_write(CharF[2],SSI0);
    char_write(CharF[3],SSI0);
    char_write(CharF[4],SSI0);
    char_write('F',SSI0);
    char_write(',',SSI0);
    char_write(' ',SSI0);
    char_write(CharC[0],SSI0);
    char_write(CharC[1],SSI0);
    char_write(CharC[2],SSI0);
    char_write(CharC[3],SSI0);
    char_write(CharC[4],SSI0);
    char_write('C',SSI0);

}
