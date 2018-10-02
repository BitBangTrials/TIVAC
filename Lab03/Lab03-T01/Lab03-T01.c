#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
//All of our included files are above, including our linked driver libraries.

uint8_t ui8PinData=2; //Setting the first blink variable as 2, or 0010 (red)
int clock_speed = 0;//Sensing clock speed variable.
int delay = 442708;
//The total delay with our clock divider at 64 (*2=128) and 3 clock cycles per delay will be:
//((1/3.125MHz)*3) * 442,708 = 424.99ms ~ 425ms.

int main(void)

{
    SysCtlClockSet(SYSCTL_SYSDIV_64|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
//Main oscillator, external 16MHz source, 400MHz PLL enabled, dividing by 64, total frequency 400MHz/(64*2)=3.125MHz.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);//Enabling GPIO Port F.
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);//Port F Pins 1,2,3 (LED pins) are output.
    clock_speed = SysCtlClockGet();//Sensing clock speed.
    while(1)
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, ui8PinData);//Write variable data
        SysCtlDelay(delay);//Delay 442,708 (3 clock cycles each)
        if(ui8PinData==8) {ui8PinData=2;} else {ui8PinData=ui8PinData*2;}
        //In effect, steps through pattern 2,4,8,2,4,8 for 0010,0100,1000,0010,0100,1000, or R,B,G,R,B,G.
    }

}
