#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
// All of our included files are above, including our linked driver libraries.

uint8_t ui8PinData=2; //Setting the first blink variable as 2, or 0010 (red)
int clock_speed = 0;//Sensing clock speed.
int delay = 442708;
//The total delay with our clock divider at 64 (*2=128) and 3 clock cycles per delay will be:
//((1/3.125MHz)*3) * 442,708 = 424.99ms ~ 425ms.

const uint8_t R = 2;
const uint8_t B = 4;
const uint8_t G = 8;
//Setting values for the three LEDs for ease of use later.

uint8_t i = 0; //Counting variable for LED color cycle.


int main(void)
{

    uint8_t LEDpatterns[24];
    LEDpatterns[0] = B;
    LEDpatterns[1] = G;
    LEDpatterns[2] = R;
    LEDpatterns[3] = B;
    LEDpatterns[4] = G;
    LEDpatterns[5] = R;
    LEDpatterns[6] = 0;
    LEDpatterns[7] = 0;
    LEDpatterns[8] = R;
    LEDpatterns[9] = G;
    LEDpatterns[10] = B;
    LEDpatterns[11] = R+G;
    LEDpatterns[12] = R+B;
    LEDpatterns[13] = G+B;
    LEDpatterns[14] = R+G+B;
    LEDpatterns[15] = R;
    LEDpatterns[16] = G;
    LEDpatterns[17] = B;
    LEDpatterns[18] = R+G;
    LEDpatterns[19] = R+B;
    LEDpatterns[20] = G+B;
    LEDpatterns[21] = R+G+B;
    LEDpatterns[22] = 0;
    LEDpatterns[23] = 0;
//The colors of LEDs can be obtained with simple addition.
//This method is less space-efficient but is easier for quick use.

    SysCtlClockSet(SYSCTL_SYSDIV_64|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
    //Main oscillator, external 16MHz source, 400MHz PLL enabled, dividing by 64, total frequency 400MHz/(64*2)=3.125MHz.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);//Enabling GPIO Port F.
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);//Port F Pins 1,2,3 (LED pins) are output.
    clock_speed = SysCtlClockGet();
    while(1)
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, LEDpatterns[i]);//Write variable data
        SysCtlDelay(delay);//Delay 442,708 (3 clock cycles each)
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x00);//Write 0s to pins (all off)
        SysCtlDelay(delay);//Delay 442,708 (3 clock cycles each)
        if(i == 23) {i=0;} else {i=i+1;}
        //Step through each portion of the array to create the pattern above.
    }

}
