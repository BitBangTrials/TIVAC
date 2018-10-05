#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

int main(void)
{
    uint32_t ui32Period;
    //Creating a variable to hold the period.
    SysCtlClockSet (SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
    //40MHz total system clock.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    //Enable the GPIOF peripheral.
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
    //Set GPIOF pins 1, 2, 3 as output.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    //Enable Timer 0 peripheral.
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    //Set Timer 0 in periodic configuration.

    ui32Period = (SysCtlClockGet() / 10) / 2;
    //Set period to be equal to 1/20 the clock frequency, for a total blink rate of 10Hz, 50% duty cycle.
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);
    //Load previously-calculated period into timer.

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
    }

    else
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 4);
    }
}
