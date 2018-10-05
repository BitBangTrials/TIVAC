#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "inc/hw_gpio.h"

    uint32_t ui32PeriodOn;
    uint32_t ui32PeriodOff;
    uint32_t ui32PeriodButton;
    //Variables of convenience to store periods for LED-on, LED-off, and Button-pressed LED-on.
int main(void)
{


    SysCtlClockSet (SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
    //40MHz clock.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //Enable GPIOF peripheral.
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
    //Set GPIOF pins 1, 2, 3 as outputs

    HWREG(GPIO_PORTF_BASE+GPIO_O_LOCK)=GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE+GPIO_O_CR)|=GPIO_PIN_0;
    HWREG(GPIO_PORTF_BASE+GPIO_O_LOCK)=0;
    //Write to registers to unlock GPIO Port F Pin 0, normally reserved for debugging.

    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0); //Set Port F pin 0 as input.
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_0); //Enable Pin F0 as input.
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_INT_PIN_0, GPIO_RISING_EDGE); //Set rising edge detected as input for pin F0.
    IntEnable(INT_GPIOF); //Enable interrupt for GPIO Port F.


    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0); //Enable Timer 0 peripheral.
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC); //Configure timer 0 to be periodic.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1); //Enable Timer 1 peripheral.
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC); //Configure timer 1 to be periodic.

    uint32_t ui32Clock = SysCtlClockGet(); //Retrieve system clock and store in variable.
    ui32PeriodOn = (ui32Clock / 2) * 75 / 100; //Calculate period of on-cycle.
    ui32PeriodOff = (ui32Clock / 2) * 25 / 100; //Calculate period of off-cycle.
    ui32PeriodButton = (ui32Clock * 1.5); //Calculate period of LED on when button pressed.

    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32PeriodOn -1); //Load on-period into timer 0.
    IntEnable(INT_TIMER0A); //Enable vector associated with Timer 0A.

    TimerLoadSet(TIMER1_BASE, TIMER_A, ui32PeriodButton -1); //Load button on-period into timer 1.
    IntEnable(INT_TIMER1A); //Enable vector associated with Timer 1A.


    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT); //Enable interrupt from Timer 0A.
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT); //Enable interrupt from Timer 1A.
    IntMasterEnable();

    TimerEnable(TIMER0_BASE, TIMER_A); //Enable and start timer 0A.

    while(1)
    {
    }
}

void Timer0IntHandler(void)

{
// Clear the timer interrupt
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // Read the current state of the GPIO pin and
    // write back the opposite state

    if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2))
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
        TimerLoadSet(TIMER0_BASE, TIMER_A, ui32PeriodOff -1);
        //If LED is on, turn it off and begin the off-period timer.
    }

    else
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 4);
        TimerLoadSet(TIMER0_BASE, TIMER_A, ui32PeriodOn -1);
        //If LED is off, turn it on and begin the on-period timer.
    }
}

void Timer1IntHandler(void)

{
    // Clear the timer interrupt
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    TimerDisable(TIMER1_BASE, TIMER_A); //Disable Timer 1A.
    //When Timer 1A is triggered, turn blue LED off.
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32PeriodOff -1); //Load off-cycle period into Timer 0A.
    TimerEnable(TIMER0_BASE, TIMER_A); //Enable Timer 0A.

}

void PortFPin0IntHandler(void)
{
    GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_0); //Clear the timer interrupt.
    TimerDisable(TIMER0_BASE, TIMER_A); //Disable Timer 0A.
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 4); //Turn on Blue LED.
    TimerEnable(TIMER1_BASE, TIMER_A); //Enable Timer 1A.
}
