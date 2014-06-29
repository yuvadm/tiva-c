//*****************************************************************************
//
// stripchartmanager.c - Manages a strip chart widget for the data logger.
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
// This is part of revision 2.1.0.12573 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_types.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "drivers/cfal96x64x16.h"
#include "drivers/slidemenuwidget.h"
#include "drivers/stripchartwidget.h"
#include "stripchartmanager.h"
#include "clocksetwidget.h"
#include "qs-logger.h"
#include "menus.h"
#include "utils/ustdlib.h"

//*****************************************************************************
//
// This module manages a strip chart widget for the data logger application.
// It provides functions to make it easy to configure a strip chart for the
// user-selected data series, and to add new data to the strip chart.  The
// functions in this module maintain buffers that hold the data for each data
// series that is selected for display on the strip chart.
//
//*****************************************************************************

//*****************************************************************************
//
// Define a scaling range for each data series.  Since multiple kinds of data
// will be shown on the strip chart, no one particular set of units can be
// selected.  Instead the strip chart Y axis will just be maintained in units
// of pixels, and the table below maps the Y axis range to min and max values
// for each data series.
//
//*****************************************************************************
typedef struct
{
    int16_t i16Min;
    int16_t i16Max;
} tDisplayScaling;

tDisplayScaling g_psScaling[] =
{
    { 0, 20000 },                             // analog channel inputs,
    { 0, 20000 },                             // 0-20V (20000 mV)
    { 0, 20000 },
    { 0, 20000 },
    { -200, 200 },                            // accelerometer axes, -2 - 2g
    { -200, 200 },                            // (units of 1/100g)
    { -200, 200 },
    { 0, 500 },                               // temperature, 0 - 50C (units of
    { 0, 500 },                               // 1/10C)
    { 0, 400 },                               // current, 0 - 40mA (units of
                                              // 100uA)
};

//*****************************************************************************
//
// Defines the maximum number of items that are stored in a data series.  This
// matches the width of the strip chart in pixels.
//
//*****************************************************************************
#define SERIES_LENGTH           96

//*****************************************************************************
//
// Create an array of strip chart data series, one for each channel of data
// that the data logger can acquire.  Fields that are unchanging, such as
// the name of each series, are pre-populated here, while other fields that
// may change are updated by functions.  These are the data series that get
// added to the strip chart for each item that is selected for logging.
//
//*****************************************************************************
static tStripChartSeries g_psSeries[] =
{
    { 0, "CH0", 0x000040, 1, 1, 0, 0 },
    { 0, "CH1", ClrLime, 1, 1, 0, 0 },
    { 0, "CH2", ClrAqua, 1, 1, 0, 0 },
    { 0, "CH3", ClrRed, 1, 1, 0, 0 },
    { 0, "ACCELX", ClrBlue, 1, 1, 0, 0 },
    { 0, "ACCELY", 0x00A000, 1, 1, 0, 0 },
    { 0, "ACCELZ", ClrFuchsia, 1, 1, 0, 0 },
    { 0, "CURRENT", ClrYellow, 1, 1, 0, 0 },
    { 0, "EXT TEMP", 0xC00040, 1, 1, 0, 0 },
    { 0, "INT TEMP", 0x60E080, 1, 1, 0, 0 },
};
#define MAX_NUM_SERIES          (sizeof(g_psSeries) /                         \
                                 sizeof(tStripChartSeries))

//*****************************************************************************
//
// Defines the X-axis for the strip chart.
//
//*****************************************************************************
static tStripChartAxis g_sAxisX =
{
    "X-AXIS",   // title of axis
    0,          // label for minimum of axis
    0,          // label for maximum of axis
    0,          // minimum value for the axis
    95,         // maximum value for the axis
    10          // grid interval for the axis
};

//*****************************************************************************
//
// Defines the Y-axis for the strip chart.
//
//*****************************************************************************
static tStripChartAxis g_sAxisY =
{
    0,          // title of the axis
    0,          // label for minimum of axis
    0,          // label for maximum of axis
    0,          // minimum value for the axis
    63,         // maximum value for the axis
    16          // grid interval for the axis
};

//*****************************************************************************
//
// Defines the strip chart widget.  This structure must be fully initialized
// by calling the function StripChartMgrInit().
//
//*****************************************************************************
StripChart(g_sStripChart, 0, 0, 0, 0, 0, 0, 96, 64, 0, g_psFontFixed6x8,
           ClrBlack, ClrWhite, ClrWhite, ClrDarkGreen, &g_sAxisX,
           &g_sAxisY, 0);

//*****************************************************************************
//
// Creates a buffer space for the values in the data series.  The buffer
// must be large enough to hold all of the data for the maximum possible
// number of data items that are selected.  If less than the maximum number
// are selected then some of the buffer space will be unused.
//
//*****************************************************************************
static uint8_t g_pui8SeriesData[MAX_NUM_SERIES * SERIES_LENGTH];

//*****************************************************************************
//
// The count of data series that are selected for showing on the strip chart.
// This value is set when the client calls the function StripChartConfigure().
//
//*****************************************************************************
static uint32_t g_ui32SelectedCount;

//*****************************************************************************
//
// The number of items (per series) that have been added to the strip chart.
//
//*****************************************************************************
static uint32_t g_ui32ItemCount;

//*****************************************************************************
//
// A bit mask of the specific data items that have been selected for logging.
//
//*****************************************************************************
static uint32_t g_ui32SelectedMask;

//*****************************************************************************
//
// Configure the strip chart for a selected set of data series.  The selected
// series is passed in as a bit mask.  Each bit that is set in the bit mask
// represents a selected series.  This function will go through the possible
// set of data series and for each that is selected it will be initialized
// and added to the strip chart.
//
//*****************************************************************************
void
StripChartMgrConfigure(uint32_t ui32SelectedMask)
{
    uint32_t ui32Idx, ui32ItemIdx;
    tStripChartSeries *psSeries;

    //
    // Save the channel mask for later use
    //
    g_ui32SelectedMask = ui32SelectedMask;

    //
    // Determine how many series are to appear in the strip chart.
    //
    g_ui32SelectedCount = 0;
    ui32Idx = ui32SelectedMask;
    while(ui32Idx)
    {
        if(ui32Idx & 1)
        {
            g_ui32SelectedCount++;
        }
        ui32Idx >>= 1;
    }

    //
    // Reset the number of items that have been stored in the series buffers.
    //
    g_ui32ItemCount = 0;

    //
    // Remove any series that were already added to the strip chart.
    //
    g_sStripChart.psSeries = 0;

    //
    // Loop through all series, and configure the selected series and add
    // them to the strip chart.
    //
    ui32ItemIdx = 0;
    for(ui32Idx = 0; ui32Idx < MAX_NUM_SERIES; ui32Idx++)
    {
        //
        // Check to see if this series is selected
        //
        if((1 << ui32Idx) & ui32SelectedMask)
        {
            //
            // Get a pointer to this series.
            //
            psSeries = &g_psSeries[ui32Idx];

            //
            // Set the stride for this series.  It will be the same as the
            // number of enabled data items.
            //
            psSeries->ui8Stride = g_ui32SelectedCount;

            //
            // Set the series data pointer to start at the first location
            // in the series buffer where this data item will appear.
            //
            psSeries->pvData = &g_pui8SeriesData[ui32ItemIdx];

            //
            // Add the series to the strip chart.
            //
            StripChartSeriesAdd(&g_sStripChart, psSeries);

            //
            // Increment the index of selected series.
            //
            ui32ItemIdx++;
        }
    }
}

//*****************************************************************************
//
// Scales the input data value to a Y pixel range according to the scaling
// table at the top of this file.
//
//*****************************************************************************
static uint8_t
ScaleDataToPixelY(int16_t i16Data, int16_t i16Min, int16_t i16Max)
{
    int32_t i32Y;
    int16_t i16Range;

    //
    // Adjust the input value so that the min will be the bottom of display.
    //
    i16Data -= i16Min;

    //
    // Compute the range of the input that will appear on the display
    //
    i16Range = i16Max - i16Min;

    //
    // Scale the input to the Y pixel range of the display
    //
    i32Y = (int32_t)i16Data * 63L;

    //
    // Add in half of divisor to get proper rounding
    //
    i32Y += (int32_t)i16Range / 2L;

    //
    // Apply final divisor to get Y pixel value on display
    //
    i32Y /= (int32_t)i16Range;

    //
    // If the Y coordinate is out of the range of the display, force the
    // value to be just off the display, in order to avoid aliasing to a
    // bogus Y pixel value when the return value is converted to a smaller
    // data type.
    //
    if((i32Y < 0) || (i32Y > 63))
    {
        i32Y = 64;
    }

    //
    // Return the Y pixel value
    //
    return((uint8_t)i32Y);
}

//*****************************************************************************
//
// Add data items to the strip chart and advance the strip chart position.
// This function will add the items pointed to by the parameter to the data
// series buffer and the strip chart will be updated to reflect the newly
// added data items.
// This function will assume that the number of items passed by the pointer
// is the same as was selected by the function StripChartMgrConfigure().
// up to the caller to ensure that the amount of data passed matches the
// number of items that were selected when the strip chart was configured.
//
//*****************************************************************************
void
StripChartMgrAddItems(int16_t *pi16DataItems)
{
    uint32_t ui32Idx, ui32SelectedMask = g_ui32SelectedMask;
    uint8_t *pui8NewData;

    //
    // Check for valid input data pointer
    //
    if(!pi16DataItems)
    {
        return;
    }

    //
    // If the number count of items in the strip chart is at the maximum, then
    // the items need to "slide down" and new data added to the end of the
    // buffer.
    //
    if(g_ui32ItemCount == SERIES_LENGTH)
    {
        memmove(&g_pui8SeriesData[0], &g_pui8SeriesData[g_ui32SelectedCount],
                SERIES_LENGTH * g_ui32SelectedCount);

        //
        // Set the pointer for newly added data to be the end of the series
        // buffer.
        //
        pui8NewData = &g_pui8SeriesData[(SERIES_LENGTH - 1) *
                      g_ui32SelectedCount];
    }
    else
    {

        //
        // Otherwise, the series data buffer is less than full so compute the
        // correct location in the buffer for the new data to be added.
        //
        pui8NewData = &g_pui8SeriesData[g_ui32ItemCount *
                                          g_ui32SelectedCount];

        //
        // Increment the number of items that have been added to the strip
        // chart series data buffer.
        //
        g_ui32ItemCount++;

        //
        // Since the count of data items has changed, it must be updated for
        // each series.
        //
        for(ui32Idx = 0; ui32Idx < MAX_NUM_SERIES; ui32Idx++)
        {
            g_psSeries[ui32Idx].ui16NumItems = g_ui32ItemCount;
        }
    }

    //
    // Convert each of the input data items being added to the strip chart
    // to a scaled Y pixel value.
    //
    ui32Idx = 0;
    while(ui32SelectedMask)
    {
        //
        // Is this data item being added to the chart?
        //
        if(ui32SelectedMask & 1)
        {
            //
            // Scale the data item and add it to the series buffer
            //
            *pui8NewData = ScaleDataToPixelY(*pi16DataItems,
                                               g_psScaling[ui32Idx].i16Min,
                                               g_psScaling[ui32Idx].i16Max);

            //
            // Increment the from/to pointers
            //
            pui8NewData++;
            pi16DataItems++;
        }
        ui32Idx++;
        ui32SelectedMask >>= 1;
    }

    //
    // Now that data has been added to the strip chart series buffers, either
    // at the end or in the middle, advance the strip chart position by 1.
    // Add a request for painting the strip chart widget.
    //
    StripChartAdvance(&g_sStripChart, 1);
    WidgetPaint(WIDGET_ROOT);
}

//*****************************************************************************
//
// Initializes the strip chart manager.  The strip chart needs an on-screen
// and off-screen display for drawing.  These are passed using the init
// function.
//
//*****************************************************************************
void
StripChartMgrInit(void)
{
    g_sStripChart.sBase.psDisplay = &g_sCFAL96x64x16;
    g_sStripChart.psOffscreenDisplay = &g_sOffscreenDisplayA;
}
