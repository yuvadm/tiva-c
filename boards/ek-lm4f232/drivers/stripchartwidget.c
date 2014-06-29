//*****************************************************************************
//
// stripchartwidget.c - A simple strip chart widget.
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
#include "driverlib/debug.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "stripchartwidget.h"

//*****************************************************************************
//
//! \addtogroup stripchartwidget_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// This is a custom widget for drawing a simple strip chart.  The strip
// chart can be configured with an X/Y grid, and data series can be added
// to and displayed on the strip chart.  The strip chart can be "advanced"
// so that the grid lines will move on the display.  Before advancing
// the chart, the application must update the series data in the buffers.
// The strip chart will only display whatever is in the series buffers, the
// application must scroll the data in the series data buffers.  By adjusting
// the data in the series data buffers, advancing the strip chart, and
// repainting, the strip chart can be made to scroll the data across the
// display.
//
//*****************************************************************************

//*****************************************************************************
//
//! Draws the strip chart into a drawing context, off-screen buffer.
//!
//! \param psChartWidget points at the StripsChartWidget being processed.
//! \param psContext points to the context where all drawing should be done.
//!
//! This function renders a strip chart into a drawing context.
//! It assumes that the drawing context is an off-screen buffer, and that
//! the entire buffer belongs to this widget.
//!
//! \return None.
//
//*****************************************************************************
void
StripChartDraw(tStripChartWidget *psChartWidget, tContext *psContext)
{
    tStripChartAxis *psAxisY;
    int32_t i32Y;
    int32_t i32Ygrid;
    int32_t i32X;
    int32_t i32GridRange;
    int32_t i32DispRange;
    int32_t i32GridMin;
    int32_t i32DispMax;
    tStripChartSeries *psSeries;

    //
    // Check the parameters
    //
    ASSERT(psChartWidget);
    ASSERT(psContext);
    ASSERT(psChartWidget->psAxisY);

    //
    // Get handy pointer to Y axis
    //
    psAxisY = psChartWidget->psAxisY;

    //
    // Find the range of Y axis in Y axis units
    //
    i32GridRange = psAxisY->i32Max - psAxisY->i32Min;

    //
    // Find the range of the Y axis in display units (pixels)
    //
    i32DispRange = (psContext->sClipRegion.i16YMax -
                    psContext->sClipRegion.i16YMin);

    //
    // Find the minimum Y units value to be shown, and the maximum of the
    // clipping region.
    //
    i32GridMin = psAxisY->i32Min;
    i32DispMax = psContext->sClipRegion.i16YMax;

    //
    // Set the fg color for the rectangle fill to match what we want as the
    // chart background.
    //
    GrContextForegroundSet(psContext, psChartWidget->ui32BackgroundColor);
    GrRectFill(psContext, &psContext->sClipRegion);

    //
    // Draw vertical grid lines
    //
    GrContextForegroundSet(psContext, psChartWidget->ui32GridColor);
    for(i32X = psChartWidget->i32GridX; i32X < psContext->sClipRegion.i16XMax;
        i32X += psChartWidget->psAxisX->i32GridInterval)
    {
        GrLineDrawV(psContext, psContext->sClipRegion.i16XMax - i32X,
                    psContext->sClipRegion.i16YMin,
                    psContext->sClipRegion.i16YMax);
    }

    //
    // Draw horizontal grid lines
    //
    for(i32Ygrid = psAxisY->i32Min; i32Ygrid < psAxisY->i32Max;
        i32Ygrid += psAxisY->i32GridInterval)
    {
        i32Y = ((i32Ygrid - i32GridMin) * i32DispRange) / i32GridRange;
        i32Y = i32DispMax - i32Y;
        GrLineDrawH(psContext, psContext->sClipRegion.i16XMin,
                    psContext->sClipRegion.i16XMax, i32Y);
    }

    //
    // Compute location of Y=0 line, and draw it
    //
    i32Y = ((-i32GridMin) * i32DispRange) / i32GridRange;
    i32Y = i32DispMax - i32Y;
    GrLineDrawH(psContext, psContext->sClipRegion.i16XMin,
                psContext->sClipRegion.i16XMax, i32Y);

    //
    // Iterate through each series to draw it
    //
    psSeries = psChartWidget->psSeries;
    while(psSeries)
    {
        int idx = 0;

        //
        // Find the starting X position on the display for this series.
        // If the series has less data points than can fit on the display
        // then starting X can be somewhere in the middle of the screen.
        //
        i32X = 1 + psContext->sClipRegion.i16XMax - psSeries->ui16NumItems;

        //
        // If the starting X is off the left side of the screen, then the
        // staring index (idx) for reading data needs to be adjusted to the
        // first value in the series that will be visible on the screen
        //
        if(i32X < psContext->sClipRegion.i16XMin)
        {
            idx = psContext->sClipRegion.i16XMin - i32X;
            i32X = psContext->sClipRegion.i16XMin;
        }

        //
        // Set the drawing color for this series
        //
        GrContextForegroundSet(psContext, psSeries->ui32Color);

        //
        // Scan through all possible X values, find the Y value, and draw the
        // pixel.
        //
        for(; i32X <= psContext->sClipRegion.i16XMax; i32X++)
        {
            //
            // Find the Y value at each position in the data series.  Take into
            // account the data size and the stride
            //
            if(psSeries->ui8DataTypeSize == 1)
            {
                i32Y =
                    ((int8_t *)psSeries->pvData)[idx * psSeries->ui8Stride];
            }
            else if(psSeries->ui8DataTypeSize == 2)
            {
                i32Y =
                    ((int16_t *)psSeries->pvData)[idx * psSeries->ui8Stride];
            }
            else if(psSeries->ui8DataTypeSize == 4)
            {
                i32Y =
                    ((int32_t *)psSeries->pvData)[idx * psSeries->ui8Stride];
            }
            else
            {
                //
                // If there is an invalid data size, then just force Y value
                // to be off the display
                //
                i32Y = i32DispMax + 1;
                break;
            }

            //
            // Advance to the next position in the data series.
            //
            idx++;

            //
            // Now scale the Y value according to the axis scaling
            //
            i32Y = ((i32Y - i32GridMin) * i32DispRange) / i32GridRange;
            i32Y = i32DispMax - i32Y;

            //
            // Draw the pixel on the display
            //
            GrPixelDraw(psContext, i32X, i32Y);
        }

        //
        // Advance to the next series until there are no more.
        //
        psSeries = psSeries->psNextSeries;
    }

    //
    // Draw a frame around the entire chart.
    //
    GrContextForegroundSet(psContext, psChartWidget->ui32Y0Color);
    GrRectDraw(psContext, &psContext->sClipRegion);

    //
    // Draw titles
    //
    GrContextForegroundSet(psContext, psChartWidget->ui32TextColor);
    GrContextFontSet(psContext, psChartWidget->psFont);

    //
    // Draw the chart title, if there is one
    //
    if(psChartWidget->pcTitle)
    {
        GrStringDrawCentered(psContext, psChartWidget->pcTitle, -1,
                             psContext->sClipRegion.i16XMax / 2,
                             GrFontHeightGet(psChartWidget->psFont), 0);
    }

    //
    // Draw the Y axis max label, if there is one
    //
    if(psChartWidget->psAxisY->pcMaxLabel)
    {
        GrStringDraw(psContext, psChartWidget->psAxisY->pcMaxLabel, -1,
                     psContext->sClipRegion.i16XMin +
                     GrFontMaxWidthGet(psChartWidget->psFont) / 2,
                     GrFontHeightGet(psChartWidget->psFont) / 2, 0);
    }

    //
    // Draw the Y axis min label, if there is one
    //
    if(psChartWidget->psAxisY->pcMinLabel)
    {
        GrStringDraw(psContext, psChartWidget->psAxisY->pcMinLabel, -1,
                     psContext->sClipRegion.i16XMin +
                     GrFontMaxWidthGet(psChartWidget->psFont) / 2,
                     psContext->sClipRegion.i16YMax -
                     (GrFontHeightGet(psChartWidget->psFont) +
                      (GrFontHeightGet(psChartWidget->psFont) / 2)),
                     0);
    }

    //
    // Draw a label for the name of the Y axis, if there is one
    //
    if(psChartWidget->psAxisY->pcName)
    {
        GrStringDraw(psContext, psChartWidget->psAxisY->pcName, -1,
                     psContext->sClipRegion.i16XMin + 1,
                     (psContext->sClipRegion.i16YMax / 2) -
                     (GrFontHeightGet(psChartWidget->psFont) / 2),
                     1);
    }
}

//*****************************************************************************
//
//! Paints the strip chart on the display.
//!
//! \param psWidget is a pointer to the strip chart widget to be drawn.
//!
//! This function draws the contents of a strip chart on the display.  This is
//! called in response to a \b WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
StripChartPaint(tWidget *psWidget)
{
    tStripChartWidget *psChartWidget;
    tContext sContext;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);
    ASSERT(psWidget->psDisplay);

    //
    // Convert the generic widget pointer into a strip chart widget pointer.
    //
    psChartWidget = (tStripChartWidget *)psWidget;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    ASSERT(psChartWidget->psOffscreenDisplay);
    GrContextInit(&sContext, psChartWidget->psOffscreenDisplay);

    //
    // Render the strip chart into the off-screen buffer
    //
    StripChartDraw(psChartWidget, &sContext);

    //
    // Initialize a drawing context for the display where the widget is to be
    // drawn.  This is the physical display, not an off-screen buffer.
    //
    GrContextInit(&sContext, psWidget->psDisplay);

    //
    // Initialize the clipping region on the physical display, based on the
    // extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(psWidget->sPosition));

    //
    // Now copy the rendered strip chart into the physical display
    //
    GrImageDraw(&sContext, psChartWidget->psOffscreenDisplay->pvDisplayData,
                psWidget->sPosition.i16XMin, psWidget->sPosition.i16YMin);
}

//*****************************************************************************
//
//! Advances the strip chart X grid by a certain number of pixels.
//!
//! \param psChartWidget is a pointer to the strip chart widget to be advanced.
//! \param i32Count is the number of positions to advance the grid.
//!
//! This function advances the X grid of the strip chart by the specified
//! number of positions.  By using this function to advance the grid in
//! combination with updating the data in the series data buffers, the strip
//! chart can be made to appear to scroll across the display.
//!
//! \return None.
//
//*****************************************************************************
void
StripChartAdvance(tStripChartWidget *psChartWidget, int32_t i32Count)
{
    //
    // Adjust the starting point of the X-grid
    //
    psChartWidget->i32GridX += i32Count;
    psChartWidget->i32GridX %= psChartWidget->psAxisX->i32GridInterval;
}

//*****************************************************************************
//
//! Adds a data series to the strip chart.
//!
//! \param psWidget is a pointer to the strip chart widget to be modified.
//! \param psNewSeries is a strip chart data series to be added to the strip
//! chart.
//!
//! This function will add a data series to the strip chart.  This function
//! just links the series into the strip chart.  It is up to the application
//! to make sure that the data series is initialized correctly.
//!
//! \return None.
//
//*****************************************************************************
void
StripChartSeriesAdd(tStripChartWidget *psWidget,
                      tStripChartSeries *psNewSeries)
{
    //
    // If there is already at least one series in this chart, then link
    // in to the existing chain.
    //
    if(psWidget->psSeries)
    {
        tStripChartSeries *psSeries = psWidget->psSeries;
        while(psSeries->psNextSeries)
        {
            psSeries = psSeries->psNextSeries;
        }
        psSeries->psNextSeries = psNewSeries;
    }

    //
    // Otherwise, there is not already a series in this chart, so set this
    // new series as the first series for the chart.
    //
    else
    {
        psWidget->psSeries = psNewSeries;
    }
    psNewSeries->psNextSeries = 0;
}

//*****************************************************************************
//
//! Removes a data series from the strip chart.
//!
//! \param psWidget is a pointer to the strip chart widget to be modified.
//! \param psOldSeries is a strip chart data series that is to be removed
//! from the strip chart.
//!
//! This function will remove an existing data series from a strip chart.  It
//! will search the list of data series for the specified series, and if
//! found it will be unlinked from the chain of data series for this strip
//! chart.
//!
//! \return None.
//
//*****************************************************************************
void
StripChartSeriesRemove(tStripChartWidget *psWidget,
                         tStripChartSeries *psOldSeries)
{
    //
    // If the series to be removed is the first one, then find the next
    // series in the chain and set it to be first.
    //
    if(psWidget->psSeries == psOldSeries)
    {
        psWidget->psSeries = psOldSeries->psNextSeries;
    }

    //
    // Otherwise, scan through the chain to find the old series
    //
    else
    {
        tStripChartSeries *psSeries = psWidget->psSeries;
        while(psSeries->psNextSeries)
        {
            //
            // If the old series is found, unlink it from the chain
            //
            if(psSeries->psNextSeries == psOldSeries)
            {
                psSeries->psNextSeries = psOldSeries->psNextSeries;
                break;
            }
            else
            {
                psSeries = psSeries->psNextSeries;
            }
        }
    }

    //
    // Finally, set the "next" pointer of the old series to null so that
    // there will not be any confusing chain fragments if this series is
    // reused.
    //
    psOldSeries->psNextSeries = 0;
}

//*****************************************************************************
//
//! Handles messages for a strip chart widget.
//!
//! \param psWidget is a pointer to the strip chart widget.
//! \param ui32Msg is the message.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function receives messages intended for this strip chart widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
int32_t
StripChartMsgProc(tWidget *psWidget, uint32_t ui32Msg, uint32_t ui32Param1,
                  uint32_t ui32Param2)
{
    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Determine which message is being sent.
    //
    switch(ui32Msg)
    {
        //
        // The widget paint request has been sent.
        //
        case WIDGET_MSG_PAINT:
        {
            //
            // Handle the widget paint request.
            //
            StripChartPaint(psWidget);

            //
            // Return one to indicate that the message was successfully
            // processed.
            //
            return(1);
        }

        //
        // Deliberately ignore all button press messages.  They may be handled
        // by another widget.
        //
        case WIDGET_MSG_KEY_SELECT:
        case WIDGET_MSG_KEY_UP:
        case WIDGET_MSG_KEY_DOWN:
        case WIDGET_MSG_KEY_LEFT:
        case WIDGET_MSG_KEY_RIGHT:
        {
            return(0);
        }

        //
        // An unknown request has been sent.
        //
        default:
        {
            //
            // Let the default message handler process this message.
            //
            return(WidgetDefaultMsgProc(psWidget, ui32Msg, ui32Param1,
                                        ui32Param2));
        }
    }
}

//*****************************************************************************
//
//! Initializes a strip chart widget.
//!
//! \param psWidget is a pointer to the strip chart widget to initialize.
//! \param psDisplay is a pointer to the display on which to draw the chart.
//! \param i32X is the X coordinate of the upper left corner of the canvas.
//! \param i32Y is the Y coordinate of the upper left corner of the canvas.
//! \param i32Width is the width of the canvas.
//! \param i32Height is the height of the canvas.
//! \param pcTitle is the label text for the strip chart
//! \param psFont is the font to use for drawing text on the chart.
//! \param ui32BackgroundColor is the colr of the background for the chart.
//! \param ui32TextColor is the color used for drawing text.
//! \param ui32Y0Color is the color used for drawing the Y=0 line and the frame
//! around the chart.
//! \param ui32GridColor is the color of the X/Y grid
//! \param psAxisX is a pointer to the X-axis object
//! \param psAxisY is a pointer to the Y-axis object
//! \param psOffscreenDisplay is a pointer to an offscreen display that will
//! be used for rendering the strip chart prior to drawing it on the
//! physical display.
//!
//! This function initializes the caller provided strip chart widget.
//!
//! \return None.
//
//*****************************************************************************
void
StripChartInit(tStripChartWidget *psWidget, const tDisplay *psDisplay,
              int32_t i32X, int32_t i32Y, int32_t i32Width, int32_t i32Height,
              char * pcTitle, tFont *psFont,
              uint32_t ui32BackgroundColor,
              uint32_t ui32TextColor,
              uint32_t ui32Y0Color,
              uint32_t ui32GridColor,
              tStripChartAxis *psAxisX, tStripChartAxis *psAxisY,
              tDisplay *psOffscreenDisplay)
{
    uint32_t ui32Idx;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);
    ASSERT(psDisplay);
    ASSERT(psAxisX);
    ASSERT(psAxisY);
    ASSERT(psOffscreenDisplay);

    //
    // Clear out the widget structure.
    //
    for(ui32Idx = 0; ui32Idx < sizeof(tStripChartWidget); ui32Idx += 4)
    {
        ((uint32_t *)psWidget)[ui32Idx / 4] = 0;
    }

    //
    // Set the size of the widget structure.
    //
    psWidget->sBase.i32Size = sizeof(tStripChartWidget);

    //
    // Mark this widget as fully disconnected.
    //
    psWidget->sBase.psParent = 0;
    psWidget->sBase.psNext = 0;
    psWidget->sBase.psChild = 0;

    //
    // Save the display pointer.
    //
    psWidget->sBase.psDisplay = psDisplay;

    //
    // Set the extents of the display area.
    //
    psWidget->sBase.sPosition.i16XMin = i32X;
    psWidget->sBase.sPosition.i16YMin = i32Y;
    psWidget->sBase.sPosition.i16XMax = i32X + i32Width - 1;
    psWidget->sBase.sPosition.i16YMax = i32Y + i32Height - 1;

    //
    // Initialize the widget fields
    //
    psWidget->pcTitle = pcTitle;
    psWidget->psFont = psFont;
    psWidget->ui32BackgroundColor = ui32BackgroundColor;
    psWidget->ui32TextColor = ui32TextColor;
    psWidget->ui32Y0Color = ui32Y0Color;
    psWidget->ui32GridColor = ui32GridColor;
    psWidget->psAxisX = psAxisX;
    psWidget->psAxisY = psAxisY;
    psWidget->psOffscreenDisplay = psOffscreenDisplay;

    //
    // Use the strip chart message handler to process messages to this widget.
    //
    psWidget->sBase.pfnMsgProc = StripChartMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

