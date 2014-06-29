//*****************************************************************************
//
// stripchartwidget.h - Prototypes for a strip chart widget.
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

#ifndef __STRIPCHARTWIDGET_H__
#define __STRIPCHARTWIDGET_H__

//*****************************************************************************
//
//! \addtogroup stripchartwidget_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
//! A structure that represents a data series to be shown on the strip chart.
//
//*****************************************************************************
typedef struct _StripChartSeries
{
    //
    //! A pointer to the next series in the chart.
    //
    struct _StripChartSeries *psNextSeries;

    //
    //! A pointer to the brief name of the data set
    //
    char *pcName;

    //
    //! The color of the data series.
    //
    uint32_t ui32Color;

    //
    //! The number of bytes of the data type (1, 2, or 4)
    //
    uint8_t ui8DataTypeSize;

    //
    //! The stride of the data.  This can be used when this data set is
    //! part of a larger set of samples that appear in a large array
    //! interleaved at a regular interval.  Use a value of 1 if the data set
    //! is not interleaved.
    //
    uint8_t ui8Stride;

    //
    //! The number of items in the data set
    //
    uint16_t ui16NumItems;

    //
    //! A pointer to the first data item.
    //
    void *pvData;
}
tStripChartSeries;

//*****************************************************************************
//
//! A structure that represents an axis of the strip chart.
//
//*****************************************************************************
typedef struct _StripChartAxis
{
    //
    //! A brief name for the axis.  Leave null for no name to be shown.
    //
    char *pcName;

    //
    //! Label for the minimum extent of the axis.  Leave null for no label.
    //
    char *pcMinLabel;

    //
    //! Label for the max extent of the axis. Leave null for no label.
    //
    char *pcMaxLabel;

    //
    //! The minimum units value for the axis.
    //
    int32_t i32Min;

    //
    //! The maximum units value for the axis
    //
    int32_t i32Max;

    //
    //! The grid interval for the axis.  Use 0 for no grid.
    //
    int32_t i32GridInterval;
} tStripChartAxis;

//*****************************************************************************
//
//! A structure that represents a strip chart widget.
//
//*****************************************************************************
typedef struct _StripChartWidget
{
    //
    //! The generic widget information.
    //
    tWidget sBase;

    //
    //! The title for the strip chart.  Leave null for no title.
    //
    char *pcTitle;

    //
    //! The font to use for drawing text on the chart.
    //
    const tFont *psFont;

    //
    //! The background color of the chart.
    //
    uint32_t ui32BackgroundColor;

    //
    //! The color for text that is drawn on the chart (titles, etc).
    //
    uint32_t ui32TextColor;

    //
    //! The color of the Y-axis 0-crossing line.
    //
    uint32_t ui32Y0Color;

    //
    //! The color of the grid lines.
    //
    uint32_t ui32GridColor;

    //
    //! The X axis
    //
    tStripChartAxis *psAxisX;

    //
    //! The Y axis
    //
    tStripChartAxis *psAxisY;

    //
    //! A pointer to the first data series for the strip chart.
    //
    tStripChartSeries *psSeries;

    //
    //! A pointer to an off-screen display to be used for rendering the chart.
    //
    const tDisplay *psOffscreenDisplay;

    //
    //! The current X-grid alignment.  This value changes in order to give the
    //! appearance of the grid moving as the strip chart advances.
    //
    int32_t i32GridX;
} tStripChartWidget;

//*****************************************************************************
//
//! Declares an initialized strip chart widget data structure.
//!
//! \param psParent is a pointer to the parent widget.
//! \param psNext is a pointer to the sibling widget.
//! \param psChild is a pointer to the first child widget.
//! \param psDisplay is a pointer to the off-screen display on which to draw.
//! \param i32X is the X coordinate of the upper left corner of the canvas.
//! \param i32Y is the Y coordinate of the upper left corner of the canvas.
//! \param i32Width is the width of the canvas.
//! \param i32Height is the height of the canvas.
//! \param pcTitle is a string for the chart title, NULL for no title.
//! \param psFont is the font used for rendering text on the chart.
//! \param ui32BackgroundColor is the background color for the chart.
//! \param ui32TextColor is the color of text (titles, labels, etc.)
//! \param ui32Y0Color is the color of the Y-axis gridline at Y=0
//! \param ui32GridColor is the color of grid lines.
//! \param psAxisX is a pointer to the axis structure for the X-axis.
//! \param psAxisY is a pointer to the axis structure for the Y-axis.
//! \param psOffscreenDisplay is a buffer for rendering the chart before
//! showing on the physical display.  The dimensions of the off-screen display
//! should match the drawing area of psDisplay.
//!
//! This macro provides an initialized strip chart widget data structure, which
//! can be used to construct the widget tree at compile time in global
//! variables (as opposed to run-time via function calls).  This must be
//! assigned to a variable, such as:
//!
//! \verbatim
//!     tStripChartWidget g_sStripChart = StripChartStruct(...);
//! \endverbatim
//!
//! Or, in an array of variables:
//!
//! \verbatim
//!     tStripChartWidget g_psStripChart[] =
//!     {
//!         StripChartStruct(...),
//!         StripChartStruct(...)
//!     };
//! \endverbatim
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define StripChartStruct(psParent, psNext, psChild, psDisplay,                \
                         i32X, i32Y, i32Width, i32Height,                     \
                         pcTitle, psFont, ui32BackgroundColor, ui32TextColor, \
                         ui32Y0Color, ui32GridColor, psAxisX, psAxisY,        \
                         psOffscreenDisplay)                                  \
        {                                                                     \
            {                                                                 \
                sizeof(tStripChartWidget),                                    \
                (tWidget *)(psParent),                                        \
                (tWidget *)(psNext),                                          \
                (tWidget *)(psChild),                                         \
                psDisplay,                                                    \
                {                                                             \
                    i32X,                                                     \
                    i32Y,                                                     \
                    (i32X) + (i32Width) - 1,                                  \
                    (i32Y) + (i32Height) - 1                                  \
                },                                                            \
                StripChartMsgProc                                             \
            },                                                                \
            pcTitle, psFont, ui32BackgroundColor, ui32TextColor, ui32Y0Color, \
            ui32GridColor, psAxisX, psAxisY, 0, psOffscreenDisplay, 0         \
        }

//*****************************************************************************
//
//! Declares an initialized variable containing a strip chart widget data
//! structure.
//!
//! \param sName is the name of the variable to be declared.
//! \param psParent is a pointer to the parent widget.
//! \param psNext is a pointer to the sibling widget.
//! \param psChild is a pointer to the first child widget.
//! \param psDisplay is a pointer to the off-screen display on which to draw.
//! \param i32X is the X coordinate of the upper left corner of the canvas.
//! \param i32Y is the Y coordinate of the upper left corner of the canvas.
//! \param i32Width is the width of the canvas.
//! \param i32Height is the height of the canvas.
//! \param pcTitle is a string for the chart title, NULL for no title.
//! \param psFont is the font used for rendering text on the chart.
//! \param ui32BackgroundColor is the background color for the chart.
//! \param ui32TextColor is the color of text (titles, labels, etc.)
//! \param ui32Y0Color is the color of the Y-axis gridline at Y=0
//! \param ui32GridColor is the color of grid lines.
//! \param psAxisX is a pointer to the axis structure for the X-axis.
//! \param psAxisY is a pointer to the axis structure for the Y-axis.
//! \param psOffscreenDisplay is a buffer for rendering the chart before
//! showing on the physical display.  The dimensions of the off-screen display
//! should match the drawing area of psDisplay.
//!
//! This macro declares a variable containing an initialized strip chart widget
//! data structure, which can be used to construct the widget tree at compile
//! time in global variables (as opposed to run-time via function calls).
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define StripChart(sName, psParent, psNext, psChild, psDisplay,               \
                   i32X, i32Y, i32Width, i32Height,                           \
                   pcTitle, psFont, ui32BackgroundColor, ui32TextColor,       \
                   ui32Y0Color, ui32GridColor, psAxisX, psAxisY,              \
                   psOffscreenDisplay)                                        \
        tStripChartWidget sName =                                             \
            StripChartStruct(psParent, psNext, psChild, psDisplay,            \
                             i32X, i32Y, i32Width,  i32Height,                \
                             pcTitle, psFont, ui32BackgroundColor,            \
                             ui32TextColor, ui32Y0Color, ui32GridColor,       \
                             psAxisX, psAxisY, psOffscreenDisplay)

//*****************************************************************************
//
//! Sets the X-axis of the strip chart.
//!
//! \param psStripChartWidget is a pointer to the strip chart widget to modify.
//! \param psAxis is the new X-axis structure for the strip chart.
//!
//! This function sets the X-axis for the widget.
//!
//! \return None.
//
//*****************************************************************************
#define StripChartXAxisSet(psStripChartWidget, psAxis)                        \
    do                                                                        \
    {                                                                         \
        (psStripChartWidget)->psAxisX = psAxis;                               \
    } while(0)

//*****************************************************************************
//
//! Sets the Y-axis of the strip chart.
//!
//! \param psStripChartWidget is a pointer to the strip chart widget to modify.
//! \param psAxis is the new Y-axis structure for the strip chart.
//!
//! This function sets the Y-axis for the widget.
//!
//! \return None.
//
//*****************************************************************************
#define StripChartYAxisSet(psStripChartWidget, psAxis)                        \
    do                                                                        \
    {                                                                         \
        (psStripChartWidget)->psAxisY = psAxis;                               \
    } while(0)

//*****************************************************************************
//
// Prototypes for the strip chart widget APIs.
//
//*****************************************************************************
extern int32_t StripChartMsgProc(tWidget *psWidget, uint32_t ui32Msg,
                                 uint32_t ui32Param1, uint32_t ui32Param2);
extern void StripChartInit(tStripChartWidget *psWidget,
                           const tDisplay *psDisplay,
                           int32_t i32X, int32_t i32Y,
                           int32_t i32Width, int32_t i32Height,
                           char * pcTitle, tFont *psFont,
                           uint32_t ui32BackgroundColor,
                           uint32_t ui32TextColor,
                           uint32_t ui32Y0Color,
                           uint32_t ui32GridColor,
                           tStripChartAxis *psAxisX, tStripChartAxis *psAxisY,
                           tDisplay *psOffscreenDisplay);
extern void StripChartSeriesAdd(tStripChartWidget *psWidget,
                                tStripChartSeries *psSeries);
extern void StripChartSeriesRemove(tStripChartWidget *psWidget,
                                   tStripChartSeries *psSeries);
extern void StripChartAdvance(tStripChartWidget *psChartWidget,
                              int32_t i32Count);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif // __STRIPCHARTWIDGET_H__
