#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

    uint32_t ui32PeriodOn;
    uint32_t ui32PeriodOff;
    //Variables of convenience to store periods for LED-on and LED-off.
int main(void)
{


    SysCtlClockSet (SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
    //40MHz total clock.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    //Enable GPIOF peripheral.
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
    //Set GPIOF pins 1, 2, 3 as outputs.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    //Enable Timer 0 peripheral.
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    //Configure timer 0 to be periodic.

    uint32_t ui32Clock = SysCtlClockGet(); //Retrieve system clock and store in variable.
    ui32PeriodOn = (ui32Clock / 2) * 75 / 100; //Calculate period of on-cycle.
    ui32PeriodOff = (ui32Clock / 2) * 25 / 100; //Calculate period of off-cycle.
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32PeriodOn -1); //Load on-period into timer 0.

    IntEnable(INT_TIMER0A); //Enable vector associated with Timer 0A.
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT); //Enable interrupt from Timer 0A.
    IntMasterEnable(); //Enable interrupt controller.

    TimerEnable(TIMER0_BASE, TIMER_A); //Enable and start the timer.

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
