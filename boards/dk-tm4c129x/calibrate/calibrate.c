//*****************************************************************************
//
// calibrate.c - Calibration routine for the touch screen driver.
//
// Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.0.12573 of the DK-TM4C129X Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "utils/ustdlib.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Calibration for the Touch Screen (calibrate)</h1>
//!
//! The raw sample interface of the touch screen driver is used to compute the
//! calibration matrix required to convert raw samples into screen X/Y
//! positions.  The produced calibration matrix can be inserted into the touch
//! screen driver to map the raw samples into screen coordinates.
//!
//! The touch screen calibration is performed according to the algorithm
//! described by Carlos E. Videles in the June 2002 issue of Embedded Systems
//! Design.  It can be found online at
//! <a href="http://www.embedded.com/story/OEG20020529S0046">
//! http://www.embedded.com/story/OEG20020529S0046</a>.
//
//*****************************************************************************

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Performs calibration of the touch screen.
//
//*****************************************************************************
int
main(void)
{
    int32_t i32Idx, i32X1, i32Y1, i32X2, i32Y2, i32Count, ppi32Points[3][4];
    uint32_t ui32SysClock;
    char pcBuffer[32];
    tContext sContext;
    tRectangle sRect;

    //
    // Run from the PLL at 120 MHz.
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(ui32SysClock);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&sContext, "calibrate");

    //
    // Print the instructions across the middle of the screen in white with a
    // 20 point small-caps font.
    //
    GrContextForegroundSet(&sContext, ClrWhite);
    GrContextFontSet(&sContext, g_psFontCmsc20);
    GrStringDrawCentered(&sContext, "Touch the box", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         (GrContextDpyHeightGet(&sContext) / 2) - 10, 0);

    //
    // Set the points used for calibration based on the size of the screen.
    //
    ppi32Points[0][0] = GrContextDpyWidthGet(&sContext) / 10;
    ppi32Points[0][1] = (GrContextDpyHeightGet(&sContext) * 2) / 10;
    ppi32Points[1][0] = GrContextDpyWidthGet(&sContext) / 2;
    ppi32Points[1][1] = (GrContextDpyHeightGet(&sContext) * 9) / 10;
    ppi32Points[2][0] = (GrContextDpyWidthGet(&sContext) * 9) / 10;
    ppi32Points[2][1] = GrContextDpyHeightGet(&sContext) / 2;

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(ui32SysClock);

    //
    // Loop through the calibration points.
    //
    for(i32Idx = 0; i32Idx < 3; i32Idx++)
    {
        //
        // Fill a white box around the calibration point.
        //
        GrContextForegroundSet(&sContext, ClrWhite);
        sRect.i16XMin = ppi32Points[i32Idx][0] - 5;
        sRect.i16YMin = ppi32Points[i32Idx][1] - 5;
        sRect.i16XMax = ppi32Points[i32Idx][0] + 5;
        sRect.i16YMax = ppi32Points[i32Idx][1] + 5;
        GrRectFill(&sContext, &sRect);

        //
        // Flush any cached drawing operations.
        //
        GrFlush(&sContext);

        //
        // Initialize the raw sample accumulators and the sample count.
        //
        i32X1 = 0;
        i32Y1 = 0;
        i32Count = -5;

        //
        // Loop forever.  This loop is explicitly broken out of when the pen is
        // lifted.
        //
        while(1)
        {
            //
            // Grab the current raw touch screen position.
            //
            i32X2 = g_i16TouchX;
            i32Y2 = g_i16TouchY;

            //
            // See if the pen is up or down.
            //
            if((i32X2 < g_i16TouchMin) || (i32Y2 < g_i16TouchMin))
            {
                //
                // The pen is up, so see if any samples have been accumulated.
                //
                if(i32Count > 0)
                {
                    //
                    // The pen has just been lifted from the screen, so break
                    // out of the controlling while loop.
                    //
                    break;
                }

                //
                // Reset the accumulators and sample count.
                //
                i32X1 = 0;
                i32Y1 = 0;
                i32Count = -5;

                //
                // Grab the next sample.
                //
                continue;
            }

            //
            // Increment the count of samples.
            //
            i32Count++;

            //
            // If the sample count is greater than zero, add this sample to the
            // accumulators.
            //
            if(i32Count > 0)
            {
                i32X1 += i32X2;
                i32Y1 += i32Y2;
            }
        }

        //
        // Save the averaged raw ADC reading for this calibration point.
        //
        ppi32Points[i32Idx][2] = i32X1 / i32Count;
        ppi32Points[i32Idx][3] = i32Y1 / i32Count;

        //
        // Erase the box around this calibration point.
        //
        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &sRect);
    }

    //
    // Clear the screen.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.i16YMax = GrContextDpyHeightGet(&sContext) - 1;
    GrRectFill(&sContext, &sRect);

    //
    // Indicate that the calibration data is being displayed.
    //
    GrContextForegroundSet(&sContext, ClrWhite);
    GrStringDraw(&sContext, "Calibration data:", -1, 16, 32, 0);

    //
    // Compute and display the M0 calibration value.
    //
    usprintf(pcBuffer, "M0 = %d",
             (((ppi32Points[0][0] - ppi32Points[2][0]) *
               (ppi32Points[1][3] - ppi32Points[2][3])) -
              ((ppi32Points[1][0] - ppi32Points[2][0]) *
               (ppi32Points[0][3] - ppi32Points[2][3]))));
    GrStringDraw(&sContext, pcBuffer, -1, 16, 72, 0);

    //
    // Compute and display the M1 calibration value.
    //
    usprintf(pcBuffer, "M1 = %d",
             (((ppi32Points[0][2] - ppi32Points[2][2]) *
               (ppi32Points[1][0] - ppi32Points[2][0])) -
              ((ppi32Points[0][0] - ppi32Points[2][0]) *
               (ppi32Points[1][2] - ppi32Points[2][2]))));
    GrStringDraw(&sContext, pcBuffer, -1, 16, 92, 0);

    //
    // Compute and display the M2 calibration value.
    //
    usprintf(pcBuffer, "M2 = %d",
             ((((ppi32Points[2][2] * ppi32Points[1][0]) -
                (ppi32Points[1][2] * ppi32Points[2][0])) * ppi32Points[0][3]) +
              (((ppi32Points[0][2] * ppi32Points[2][0]) -
                (ppi32Points[2][2] * ppi32Points[0][0])) * ppi32Points[1][3]) +
              (((ppi32Points[1][2] * ppi32Points[0][0]) -
                (ppi32Points[0][2] * ppi32Points[1][0])) * ppi32Points[2][3])));
    GrStringDraw(&sContext, pcBuffer, -1, 16, 112, 0);

    //
    // Compute and display the M3 calibration value.
    //
    usprintf(pcBuffer, "M3 = %d",
             (((ppi32Points[0][1] - ppi32Points[2][1]) *
               (ppi32Points[1][3] - ppi32Points[2][3])) -
              ((ppi32Points[1][1] - ppi32Points[2][1]) *
               (ppi32Points[0][3] - ppi32Points[2][3]))));
    GrStringDraw(&sContext, pcBuffer, -1, 16, 132, 0);

    //
    // Compute and display the M4 calibration value.
    //
    usprintf(pcBuffer, "M4 = %d",
             (((ppi32Points[0][2] - ppi32Points[2][2]) *
               (ppi32Points[1][1] - ppi32Points[2][1])) -
              ((ppi32Points[0][1] - ppi32Points[2][1]) *
               (ppi32Points[1][2] - ppi32Points[2][2]))));
    GrStringDraw(&sContext, pcBuffer, -1, 16, 152, 0);

    //
    // Compute and display the M5 calibration value.
    //
    usprintf(pcBuffer, "M5 = %d",
             ((((ppi32Points[2][2] * ppi32Points[1][1]) -
                (ppi32Points[1][2] * ppi32Points[2][1])) * ppi32Points[0][3]) +
              (((ppi32Points[0][2] * ppi32Points[2][1]) -
                (ppi32Points[2][2] * ppi32Points[0][1])) * ppi32Points[1][3]) +
              (((ppi32Points[1][2] * ppi32Points[0][1]) -
                (ppi32Points[0][2] * ppi32Points[1][1])) * ppi32Points[2][3])));
    GrStringDraw(&sContext, pcBuffer, -1, 16, 172, 0);

    //
    // Compute and display the M6 calibration value.
    //
    usprintf(pcBuffer, "M6 = %d",
             (((ppi32Points[0][2] - ppi32Points[2][2]) *
               (ppi32Points[1][3] - ppi32Points[2][3])) -
              ((ppi32Points[1][2] - ppi32Points[2][2]) *
               (ppi32Points[0][3] - ppi32Points[2][3]))));
    GrStringDraw(&sContext, pcBuffer, -1, 16, 192, 0);

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&sContext);

    //
    // The calibration is complete.  Sit around and wait for a reset.
    //
    while(1)
    {
    }
}
