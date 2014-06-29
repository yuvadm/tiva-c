//*****************************************************************************
//
// sine_demo.c - Computes a sine wave and displays it on the screen.
//
// Copyright (c) 2011-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C123G Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "drivers/cfal96x64x16.h"
#include "drivers/stripchartwidget.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Sine Demo (sine_demo)</h1>
//!
//! This example uses the floating point capabilities of the Tiva C Series
//! processor to compute a sine wave and show it on the display.
//
//*****************************************************************************

//*****************************************************************************
//
// Provide a definition for M_PI, if it was not provided by math.h.
//
//*****************************************************************************
#ifndef M_PI
#define M_PI                    3.14159265358979323846
#endif

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND        20
#define FSECONDS_PER_TICK       (1.0 / (float)TICKS_PER_SECOND)

//*****************************************************************************
//
// A counter for system clock ticks, used for tracking time.
//
//*****************************************************************************
static volatile uint32_t g_ui32TickCount;

//*****************************************************************************
//
// Define an off-screen buffer and display structure.  This is used by the
// strip chart widget for drawing a scrolling display.
//
//*****************************************************************************
#define OFFSCREEN_BUF_SIZE      GrOffScreen4BPPSize(96, 64)
uint8_t g_pui8OffscreenBuf[OFFSCREEN_BUF_SIZE];
tDisplay g_sOffscreenDisplay;

//*****************************************************************************
//
// Create a palette for the off-screen buffer that is used by the strip chart.
//
//*****************************************************************************
uint32_t g_pui32Palette[] =
{
    ClrBlack,
    ClrWhite,
    ClrRed,
    ClrDarkGreen,
};
#define NUM_PALETTE_ENTRIES     (sizeof(g_pui32Palette) / sizeof(uint32_t))

//*****************************************************************************
//
// Define the series for the strip chart.  The SERIES_LENGTH is the maximum
// number of points that will be shown on the chart.
//
//*****************************************************************************
#define SERIES_LENGTH 96
static tStripChartSeries g_sSeries =
{
    0, "SINE", ClrRed, 1, 1, 0, 0
};

//*****************************************************************************
//
// Defines the X-axis for the strip chart
//
//*****************************************************************************
static tStripChartAxis i16Axis16X =
{
    "TIME",                                 // title of axis
    0,                                      // label for minimum of axis
    0,                                      // label for maximum of axis
    0,                                      // minimum value for the axis
    95,                                     // maximum value for the axis
    TICKS_PER_SECOND                        // grid interval for the axis
};

//*****************************************************************************
//
// Defines the Y-axis for the strip chart.
//
//*****************************************************************************
static tStripChartAxis i16Axis16Y =
{
    "SIN(2pi*t/4)*0.5",                     // title of the axis
    "-1",                                   // label for minimum of axis
    "+1",                                   // label for maximum of axis
    -32,                                    // minimum value for the axis
    31,                                     // maximum value for the axis
    16                                      // grid interval for the axis
};

//*****************************************************************************
//
// Defines the strip chart widget.  This structure requires additional
// run-time initialization.
//
//*****************************************************************************
StripChart(g_sStripChart, WIDGET_ROOT, 0, 0, &g_sCFAL96x64x16, 0, 0, 96, 64,
           0, g_psFontFixed6x8, ClrBlack, ClrWhite, ClrWhite, ClrDarkGreen,
           &i16Axis16X, &i16Axis16Y, &g_sOffscreenDisplay);

//*****************************************************************************
//
// Creates a buffer for holding the values of the data series.  It must be
// large enough to hold the maximum number of data points in the series that
// will be shown on the strip chart.
//
//*****************************************************************************
static int8_t g_i8SeriesData[SERIES_LENGTH];

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
// This is the handler for this SysTick interrupt.  It simply increments a
// counter that is used for timing.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Update our tick counter.
    //
    g_ui32TickCount++;
}

//*****************************************************************************
//
// Compute and display a sine wave.
//
//*****************************************************************************
int
main(void)
{
    uint_fast16_t ui16ItemCount = 0;
    uint32_t ui32LastTickCount = 0;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();

    //
    // Set the clocking to run directly at 50 MHz.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    //
    // Configure SysTick to generate a periodic time tick interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Initialize the display driver.
    //
    CFAL96x64x16Init();

    //
    // Initialize an offscreen display and assign the palette.  This offscreen
    // buffer is needed by the strip chart widget.
    //
    GrOffScreen4BPPInit(&g_sOffscreenDisplay, g_pui8OffscreenBuf, 96, 64);
    GrOffScreen4BPPPaletteSet(&g_sOffscreenDisplay, g_pui32Palette, 0,
                              NUM_PALETTE_ENTRIES);

    //
    // Set the data series buffer pointer to point at the storage where the
    // series data points will be stored.
    //
    g_sSeries.pvData = g_i8SeriesData;

    //
    // Add the series to the strip chart
    //
    StripChartSeriesAdd(&g_sStripChart, &g_sSeries);

    //
    // Add the strip chart to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, &g_sStripChart.sBase);

    //
    // Enter a loop to continuously calculate a sine wave.
    //
    while(1)
    {
        float fElapsedTime;
        float fRadians;
        float fSine;

        //
        // Wait for the next timer tick.
        //
        while(ui32LastTickCount == g_ui32TickCount)
        {
        }
        ui32LastTickCount = g_ui32TickCount;

        //
        // Preparing to add a new data point to the strip chart ...
        // If the number count of items in the strip chart has reached the
        // maximum value, then the data points need to "slide down" in the
        // buffer so new data can be added at the end.
        //
        if(ui16ItemCount == SERIES_LENGTH)
        {
            memmove(&g_i8SeriesData[0], &g_i8SeriesData[1], SERIES_LENGTH - 1);
        }

        //
        // Otherwise, the series data buffer is less than full so just
        // increment the count of data points.
        //
        else
        {
            //
            // Increment the number of items that have been added to the strip
            // chart series data buffer.
            //
            ui16ItemCount++;

            //
            // Since the count of data items has changed, it must be updated in
            // the data series.
            //
            g_sSeries.ui16NumItems = ui16ItemCount;
        }

        //
        // Compute the elapsed time in decimal seconds, in floating point
        // format.
        //
        fElapsedTime = (float)g_ui32TickCount * FSECONDS_PER_TICK;

        //
        // Convert the time to radians.
        //
        fRadians = fElapsedTime * 2.0 * M_PI;

        //
        // Adjust the period of the wave.  This will give us a wave period
        // of 4 seconds, or 0.25 Hz.  This number was chosen arbitrarily to
        // provide a nice looking wave on the display.
        //
        fRadians /= 4.0;

        //
        // Compute the sine.  Multiply by 0.5 to reduce the amplitude.
        //
        fSine = sinf(fRadians) * 0.5;

        //
        // Finally, save the sine value into the last location in the series
        // data point buffer.  Convert the sine amplitude to display pixels.
        // (Amplitude 1 = 32 pixels)
        //
        g_i8SeriesData[ui16ItemCount - 1] = (int8_t)(fSine * 32.0);

        //
        // Now that a new data point has been added to the series, advance
        // the strip chart.
        //
        StripChartAdvance(&g_sStripChart, 1);

        //
        // Request a repaint and run the widget processing queue.
        //
        WidgetPaint(WIDGET_ROOT);
        WidgetMessageQueueProcess();
    }
}
