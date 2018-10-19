#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"
#include "driverlib/rom.h"
//Include all needed files for peripherals and system operations.

#define PWM_FREQUENCY 5000 //PWM frequency of 5 kHz, which is a period of 20us.

int main(void)
{
    volatile uint32_t ui32Load;
    volatile uint32_t ui32PWMClock;
    volatile uint8_t ui8Adjust;
    volatile uint8_t r;
    volatile uint8_t b;
    volatile uint8_t g;
    ui8Adjust = 90;
    //Create our variables used by our PWM to control the LED.  Begin at 90%.

    ROM_SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
    ROM_SysCtlPWMClockSet(SYSCTL_PWMDIV_64);
    //Set system clock 40 MHz, PWM clock divided by 64 for 625 kHz.

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    //Enable clock to PWM5, GPIO ports D and F.

    ROM_GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
    ROM_GPIOPinConfigure(GPIO_PF1_M1PWM5);
    ROM_GPIOPinConfigure(GPIO_PF2_M1PWM6);
    ROM_GPIOPinConfigure(GPIO_PF3_M1PWM7);
    //Enable PWM to output to port F pins 1,2,3.

    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
    ROM_GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_DIR_MODE_IN);
    ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    //Unlock PF0 for button use, configure PF0 and PF4 as inputs, and configure pull-up resistors for these pins.

    ui32PWMClock = SysCtlClockGet() / 64;
    ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;
    ROM_PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
    ROM_PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
    ROM_PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, ui32Load);
    ROM_PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, ui32Load);
    //Calculate our PWM clock then divide by PWM frequency to load into timer for PWM generation.
    //PWM1 configured as down-counter, calculated load count loaded into PWM.

    ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8Adjust * ui32Load / 100);
    ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8Adjust * ui32Load / 100);
    ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8Adjust * ui32Load / 100);
    ROM_PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);
    ROM_PWMOutputState(PWM1_BASE, PWM_OUT_6_BIT, true);
    ROM_PWMOutputState(PWM1_BASE, PWM_OUT_7_BIT, true);
    ROM_PWMGenEnable(PWM1_BASE, PWM_GEN_2);
    ROM_PWMGenEnable(PWM1_BASE, PWM_GEN_3);
    //Pull PWM load value, divide by 100 to give us our minimum resolution (constrained by variable size).
    //With a period of 20us, each step will be 200ns.
    //Configure PWM1 generators 2,3 then enable them.
    while(1)
    {
        //If switch 1 is pressed, decrement our duty cycle to adjust LED.
        if(ROM_GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00)
        {
            for(r=90; r>=10; r--)
            {
                for(b=90; b>=10; b--)
                {
                    for(g=90; g>=10; g--)
                    {
                        ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, r * ui32Load / 100);
                        ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, b * ui32Load / 100);
                        ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, g * ui32Load / 100);
                    }
                }
            }
        }
        //If switch 2 is pressed, increment our duty cycle to adjust LED.
        if(ROM_GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0)==0x00)
        {
            for(r=10; r<=90; r++)
            {
                for(b=10; b<=90; b++)
                {
                    for(g=10; g<=90; g++)
                    {
                        ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, r * ui32Load / 100);
                        ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, b * ui32Load / 100);
                        ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, g * ui32Load / 100);
                    }
                }
            }
        }
    }
}
