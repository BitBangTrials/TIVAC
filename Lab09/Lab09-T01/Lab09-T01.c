#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
//Include all referenced files.

//If M_PI is not defined, define it as the below value.
#ifndef M_PI
#define M_PI                    3.14159265358979323846
#endif

//Define SERIES_LENGTH, substituting the given value where it occurs.
#define SERIES_LENGTH 100

//Declare gSeriesData as a float array of size SERIES_LENGTH, defined above.
float gSeriesData[SERIES_LENGTH];

//Create variable to count up from 0 to SERIES_LENGTH
int32_t i32DataCount = 0;

int main(void)
{
    //Float variable to hold how large each step in the calculation is, in radians.
    float fRadians;
    //Enable lazy stacking and the FPU using the ROM functions.
    ROM_FPULazyStackingEnable();
    ROM_FPUEnable();
    //Use ROM function to set system clock, with these settings clock is at 50MHz.
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
    //Calculate Fradians as 2pi/length, to calculate each point for one full cycle.
    fRadians = ((2 * M_PI) / SERIES_LENGTH);

    //Calculate each point, one full cycle divided into SERIES_LENGTH number of segments.
    while(i32DataCount < SERIES_LENGTH)
    {
        //Calculate SIN function from 0 to 2pi.
        gSeriesData[i32DataCount] = sinf(fRadians * i32DataCount);
        //Increment step
        i32DataCount++;
    }
    //Once calculation is complete, hold in an endless loop.
    while(1)
    {
    }
}
