//*****************************************************************************
//
// clocksetwidget.c - A widget for setting clock date/time.
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
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "utils/uartstdio.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "utils/ustdlib.h"
#include "clocksetwidget.h"

//*****************************************************************************
//
//! \addtogroup clocksetwidget_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// This is a custom widget for setting the date/time of a clock.  The widget
// will display the year, month, day, hour and minute on the display.  The
// user can highlight the fields with the left/right keys, and can change the
// value of each with the up/down keys.  When finished the user highlights
// the OK field on the screen and presses the select button.
//
//*****************************************************************************

//*****************************************************************************
//
// Define indices for each of the fields used for date and time.
//
//*****************************************************************************
#define FIELD_YEAR              0
#define FIELD_MONTH             1
#define FIELD_DAY               2
#define FIELD_HOUR              3
#define FIELD_MINUTE            4
#define FIELD_OK                5
#define FIELD_CANCEL            6
#define FIELD_LAST              6
#define NUM_FIELDS              7

//*****************************************************************************
//
//! Paints the clock set widget on the display.
//!
//! \param psWidget is a pointer to the clock setting widget to be drawn.
//!
//! This function draws the date and time fields of the clock setting widget
//! onto the display.  One of the fields can be highlighted.  This is
//! called in response to a \b WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
ClockSetPaint(tWidget *psWidget)
{
    tClockSetWidget *psClockWidget;
    tContext sContext;
    tRectangle sRect, sRectSel;
    struct tm *psTime;
    char pcBuf[8];
    int32_t i32X, i32Y, i32Width, i32Height;
    uint32_t ui32Idx, ui32FontHeight, ui32FontWidth, ui32SelWidth;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);
    ASSERT(psWidget->psDisplay);

    //
    // Convert the generic widget pointer into a clock set widget pointer.
    //
    psClockWidget = (tClockSetWidget *)psWidget;
    ASSERT(psClockWidget->psTime);

    //
    // Get pointer to the time structure
    //
    psTime = psClockWidget->psTime;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sContext, psWidget->psDisplay);

    //
    // Initialize the clipping region based on the extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(psWidget->sPosition));

    //
    // Set the font for the context, and get font height and width - they
    // are used a lot later.
    //
    GrContextFontSet(&sContext, psClockWidget->psFont);
    ui32FontHeight = GrFontHeightGet(psClockWidget->psFont);
    ui32FontWidth = GrFontMaxWidthGet(psClockWidget->psFont);

    //
    // Fill the widget with the background color.
    //
    GrContextForegroundSet(&sContext, psClockWidget->ui32BackgroundColor);
    GrRectFill(&sContext, &sContext.sClipRegion);

    //
    // Draw a border around the widget
    //
    GrContextForegroundSet(&sContext, psClockWidget->ui32ForegroundColor);
    GrContextBackgroundSet(&sContext, psClockWidget->ui32BackgroundColor);
    GrRectDraw(&sContext, &sContext.sClipRegion);

    //
    // Compute a rectangle for the screen title.  Put it at the top of
    // the widget display, and sized to be the height of the font, plus
    // a few pixels of space.
    //
    sRect.i16XMin = sContext.sClipRegion.i16XMin;
    sRect.i16XMax = sContext.sClipRegion.i16XMax;
    sRect.i16YMin = sContext.sClipRegion.i16YMin;
    sRect.i16YMax = ui32FontHeight * 2;
    GrRectDraw(&sContext, &sRect);

    //
    // Print a title for the widget
    //
    GrContextFontSet(&sContext, psClockWidget->psFont);
    GrStringDrawCentered(&sContext, "CLOCK SET", -1,
                         (1 + sRect.i16XMax - sRect.i16XMin) / 2,
                         (1 + sRect.i16YMax - sRect.i16YMin) / 2, 1);

    //
    // Reset the rectangle to cover the non-title area of the display
    //
    sRect.i16YMin = sRect.i16YMax + 1;
    sRect.i16YMax = sContext.sClipRegion.i16YMax;

    //
    // Compute the width and height of the area remaining for showing the
    // clock fields.
    //
    i32Width = 1 + (sRect.i16XMax - sRect.i16XMin);
    i32Height = 1 + (sRect.i16YMax - sRect.i16YMin);

    //
    // Compute the X and Y starting point for the row that will show the
    // date.
    //
    i32X = sRect.i16XMin + (i32Width - (ui32FontWidth * 10)) / 2;
    i32Y = sRect.i16YMin + ((i32Height * 1) / 6) - (ui32FontHeight / 2);

    //
    // Draw the date field separators on the date row.
    //
    GrStringDraw(&sContext, "/", -1, i32X + (ui32FontWidth * 4), i32Y, 0);
    GrStringDraw(&sContext, "/", -1, i32X + (ui32FontWidth * 7), i32Y, 0);

    //
    // Compute the X and Y starting point for the row that will show the
    // time.
    //
    i32X = sRect.i16XMin + (i32Width - (ui32FontWidth * 5)) / 2;
    i32Y = sRect.i16YMin + ((i32Height * 3) / 6) - (ui32FontHeight / 2);

    //
    // Draw the time field separators on the time row.
    //
    GrStringDraw(&sContext, ":", -1, i32X + (ui32FontWidth * 2), i32Y, 0);

    //
    // Process each of the fields to be shown on the widget
    //
    for(ui32Idx = 0; ui32Idx < NUM_FIELDS; ui32Idx++)
    {
        //
        // Compute the X and Y for the text for each field, and print the
        // text into a buffer.
        //
        switch(ui32Idx)
        {
            //
            // Year
            //
            case FIELD_YEAR:
            {
                usnprintf(pcBuf, sizeof(pcBuf), "%4u", psTime->tm_year+1900);
                i32X = sRect.i16XMin + (i32Width - (ui32FontWidth * 10)) / 2;
                i32Y = sRect.i16YMin + ((i32Height * 1) / 6) -
                       (ui32FontHeight / 2);
                ui32SelWidth = 4;
                break;
            }

            //
            // Month
            //
            case FIELD_MONTH:
            {
                usnprintf(pcBuf, sizeof(pcBuf), "%02u", psTime->tm_mon + 1);
                i32X += ui32FontWidth * 5;
                ui32SelWidth = 2;
                break;
            }

            //
            // Day
            //
            case FIELD_DAY:
            {
                usnprintf(pcBuf, sizeof(pcBuf), "%02u", psTime->tm_mday);
                i32X += ui32FontWidth * 3;
                ui32SelWidth = 2;
                break;
            }

            //
            // Hour
            //
            case FIELD_HOUR:
            {
                usnprintf(pcBuf, sizeof(pcBuf), "%02u", psTime->tm_hour);
                i32X = sRect.i16XMin + (i32Width - (ui32FontWidth * 5)) / 2;
                i32Y = sRect.i16YMin + ((i32Height * 3) / 6) -
                       (ui32FontHeight / 2);
                ui32SelWidth = 2;
                break;
            }

            //
            // Minute
            //
            case FIELD_MINUTE:
            {
                usnprintf(pcBuf, sizeof(pcBuf), "%02u", psTime->tm_min);
                i32X += ui32FontWidth * 3;
                ui32SelWidth = 2;
                break;
            }

            //
            // OK
            //
            case FIELD_OK:
            {
                usnprintf(pcBuf, sizeof(pcBuf), "OK");
                i32X = (i32Width - (ui32FontWidth * 9)) / 2;
                i32X += sRect.i16XMin;
                i32Y = ((i32Height * 5) / 6) - (ui32FontHeight / 2);
                i32Y += sRect.i16YMin;
                ui32SelWidth = 2;
                break;
            }

            //
            // CANCEL (default case is purely to keep the compiler from
            // issuing a warning that ui32SelWidth may be used ininitialized).
            //
            case FIELD_CANCEL:
            default:
            {
                usnprintf(pcBuf, sizeof(pcBuf), "CANCEL");
                i32X += ui32FontWidth * 3;
                ui32SelWidth = 6;
                break;
            }
        }


        //
        // If the current field index is the highlighted field, then this
        // text field will be drawn with highlighting.
        //
        if(ui32Idx == psClockWidget->ui32Highlight)
        {
            //
            // Compute a rectangle for the highlight area.
            //
            sRectSel.i16XMin = i32X;
            sRectSel.i16XMax = (ui32SelWidth * ui32FontWidth) + i32X;
            sRectSel.i16YMin = i32Y - 2;
            sRectSel.i16YMax = ui32FontHeight + i32Y + 2;

            //
            // Set the foreground color to the text color, and then fill the
            // highlight rectangle.  The text field will be highlighted by
            // inverting the normal colors.
            // Then draw the highlighting rectangle.
            //
            GrContextForegroundSet(&sContext,
                                   psClockWidget->ui32ForegroundColor);
            GrRectFill(&sContext, &sRectSel);

            //
            // Change the foreground color to the normal background color.
            // This will be used for drawing the text for the highlighted
            // field, which has the colors inverted (FG <--> BG)
            //
            GrContextForegroundSet(&sContext,
                                   psClockWidget->ui32BackgroundColor);
        }
        else
        {
            //
            // This text field is not highlighted so just set the normal
            // foreground color.
            //
            GrContextForegroundSet(&sContext,
                                   psClockWidget->ui32ForegroundColor);
        }

        //
        // Print the text from the buffer to the display at the computed
        // location.
        //
        GrStringDraw(&sContext, pcBuf, -1, i32X, i32Y, 0);
    }
}

//*****************************************************************************
//
//! Determine the number of days in a month.
//!
//! \param tm_mon is the month number to use for determining the number of
//! days. The month begins with 0 meaning January and 11 meaning December.
//!
//! This function returns the highest day number for the specified month.
//! It does not account for leap year, so February always returns 28 days.
//!
//! \return Returns the highest day number for the month.
//
//*****************************************************************************
static uint8_t
MaxDayOfMonth(uint8_t tm_mon)
{
    //
    // Process the month
    //
    switch(tm_mon)
    {
        //
        // February returns 28 days
        //
        case 1:
        {
            return(28);
        }

        //
        // April, June, September and November return 30
        //
        case 3:
        case 5:
        case 8:
        case 10:
        {
            return(30);
        }

        //
        // Remaining months have 31 days.
        //
        default:
        {
            return(31);
        }
    }
}

//*****************************************************************************
//
//! Handle the UP button event.
//!
//! \param psWidget is a pointer to the clock setting widget on which to
//! operate.
//!
//! This function handles the event when the user has pressed the up button.
//! It will increment the currently highlighted date/time field if it is not
//! already at the maximum value.  If the month or day of the month is being
//! changed then it enforces the maximum number of days for the month.
//!
//! \return Returns non-zero if the button event was handled, and 0 if the
//! button event was not handled.
//
//*****************************************************************************
static int32_t
ClockSetKeyUp(tClockSetWidget *psWidget)
{
    //
    // Get pointer to the time structure to be modified.
    //
    struct tm *psTime = psWidget->psTime;

    //
    // Determine which field is highlighted.
    //
    switch(psWidget->ui32Highlight)
    {
        //
        // Increment the year.  Cap it at 2037 to keep things simple.
        //
        case FIELD_YEAR:
        {
            if(psTime->tm_year+1900 < 2037)
            {
                psTime->tm_year++;
            }
            break;
        }

        //
        // Increment the month.  Adjust the day of the month if needed.
        //
        case FIELD_MONTH:
        {
            if(psTime->tm_mon < 11)
            {
                psTime->tm_mon++;
            }
            if(psTime->tm_mday > MaxDayOfMonth(psTime->tm_mon))
            {
                psTime->tm_mday = MaxDayOfMonth(psTime->tm_mon);
            }
            break;
        }

        //
        // Increment the day.  Cap it at the max number of days for the
        // current value of month.
        //
        case FIELD_DAY:
        {
            if(psTime->tm_mday < MaxDayOfMonth(psTime->tm_mon))
            {
                psTime->tm_mday++;
            }
            break;
        }

        //
        // Increment the hour.
        //
        case FIELD_HOUR:
        {
            if(psTime->tm_hour < 23)
            {
                psTime->tm_hour++;
            }
            break;
        }

        //
        // Increment the minute.
        //
        case FIELD_MINUTE:
        {
            if(psTime->tm_min < 59)
            {
                psTime->tm_min++;
            }
            break;
        }

        //
        // Bad value for field index - ignore.
        //
        default:
        {
            break;
        }
    }

    //
    // Since something may have been changed in the clock value, request
    // a repaint of the widget.
    //
    WidgetPaint(&psWidget->sBase);

    //
    // Return indication that the button event was handled.
    //
    return(1);
}

//*****************************************************************************
//
//! Handle the DOWN button event.
//!
//! \param psWidget is a pointer to the clock setting widget on which to
//! operate.
//!
//! This function handles the event when the user has pressed the down button.
//! It will decrement the currently highlighted date/time field if it is not
//! already at the minimum value.  If the month is being changed then it
//! enforces the maximum number of days for the month.
//!
//! \return Returns non-zero if the button event was handled, and 0 if the
//! button event was not handled.
//
//*****************************************************************************
static int32_t
ClockSetKeyDown(tClockSetWidget *psWidget)
{
    //
    // Get pointer to the time structure to be modified.
    //
    struct tm *psTime = psWidget->psTime;

    //
    // Determine which field is highlighted.
    //
    switch(psWidget->ui32Highlight)
    {
        //
        // Decrement the year.  Minimum year is 1970.
        //
        case FIELD_YEAR:
        {
            if(psTime->tm_year+1900 > 1970)
            {
                psTime->tm_year--;
            }
            break;
        }

        //
        // Decrement the month.  If the month has changed, check that the
        // day is valid for this month, and enforce the maximum day number
        // for this month.
        //
        case FIELD_MONTH:
        {
            if(psTime->tm_mon > 0)
            {
                psTime->tm_mon--;
            }
            if(psTime->tm_mday > MaxDayOfMonth(psTime->tm_mon))
            {
                psTime->tm_mday = MaxDayOfMonth(psTime->tm_mon);
            }
            break;
        }

        //
        // Decrement the day
        //
        case FIELD_DAY:
        {
            if(psTime->tm_mday > 1)
            {
                psTime->tm_mday--;
            }
            break;
        }

        //
        // Decrement the hour
        //
        case FIELD_HOUR:
        {
            if(psTime->tm_hour > 0)
            {
                psTime->tm_hour--;
            }
            break;
        }

        //
        // Decrement the minute
        //
        case FIELD_MINUTE:
        {
            if(psTime->tm_min > 0)
            {
                psTime->tm_min--;
            }
            break;
        }

        //
        // Bad value for field index - ignore.
        //
        default:
        {
            break;
        }
    }

    //
    // Since something may have been changed in the clock value, request
    // a repaint of the widget.
    //
    WidgetPaint(&psWidget->sBase);

    //
    // Return indication that the button event was handled.
    //
    return(1);
}

//*****************************************************************************
//
//! Handle the LEFT button event.
//!
//! \param psWidget is a pointer to the clock setting widget on which to
//! operate.
//!
//! This function handles the event when the user has pressed the left button.
//! It will change the highlighted field to the previous field.  If it is
//! at the first field in the display, it will wrap around to the last.
//!
//! \return Returns non-zero if the button event was handled, and 0 if the
//! button event was not handled.
//
//*****************************************************************************
static int32_t
ClockSetKeyLeft(tClockSetWidget *psWidget)
{
    //
    // If not already at the minimum, decrement the highlighted field index.
    //
    if(psWidget->ui32Highlight)
    {
        psWidget->ui32Highlight--;
    }
    else
    {
        //
        // Already at the first field, so reset to the last field.
        //
        psWidget->ui32Highlight = FIELD_LAST;
    }

    //
    // The highlighted field changed, so request a repaint of the widget.
    //
    WidgetPaint(&psWidget->sBase);

    //
    // Return indication that the button event was handled.
    //
    return(1);
}

//*****************************************************************************
//
//! Handle the RIGHT button event.
//!
//! \param psWidget is a pointer to the clock setting widget on which to
//! operate.
//!
//! This function handles the event when the user has pressed the right button.
//! It will change the highlighted field to the next field.  If it is already
//! at the last field in the display, it will wrap around to the first.
//!
//! \return Returns non-zero if the button event was handled, and 0 if the
//! button event was not handled.
//
//*****************************************************************************
static int32_t
ClockSetKeyRight(tClockSetWidget *psWidget)
{
    //
    // If not already at the last field, increment the highlighted field index.
    //
    if(psWidget->ui32Highlight < FIELD_LAST)
    {
        psWidget->ui32Highlight++;
    }
    else
    {
        //
        // Already at the last field, so reset to the first field.
        //
        psWidget->ui32Highlight = 0;
    }

    //
    // The highlighted field changed, so request a repaint of the widget.
    //
    WidgetPaint(&psWidget->sBase);

    //
    // Return indication that the button event was handled.
    //
    return(1);
}

//*****************************************************************************
//
//! Handle the select button event.
//!
//! \param psWidget is a pointer to the clock setting widget on which to
//! operate.
//!
//! This function handles the event when the user has pressed the select
//! button.  If either the OK or CANCEL fields is highlighted, then the
//! function will call the callback function to notify the application that
//! an action has been taken and the widget should be dismissed.
//!
//! \return Returns non-zero if the button event was handled, and 0 if the
//! button event was not handled.
//
//*****************************************************************************
static int32_t
ClockSetKeySelect(tClockSetWidget *psWidget)
{
    bool bOk;

    //
    // Determine if the OK text field is highlighted and set a flag.
    //
    bOk = (psWidget->ui32Highlight == FIELD_OK) ? true : false;

    //
    // If there is a callback function installed, and either the OK or CANCEL
    // fields is highlighted, then take action.
    //
    if(psWidget->pfnOnOkClick && ((psWidget->ui32Highlight == FIELD_OK) ||
                                  (psWidget->ui32Highlight == FIELD_CANCEL)))
    {
        //
        // Call the callback function and pass the flag to indicate if OK
        // was selected (otherwise it was CANCEL).
        //
        psWidget->pfnOnOkClick(&psWidget->sBase, bOk);

        //
        // Set the default highlighted field.  This is the field that will
        // be highlighted the next time this widget is activated.
        //
        psWidget->ui32Highlight = FIELD_CANCEL;

        //
        // Return to caller, indicating the button event was handled.
        //
        return(1);
    }
    else
    {
        //
        // There is no callback function, or neither the OK or CANCEL fields is
        // highlighted.  In this case ingore the button event.
        //
        return(0);
    }
}

//*****************************************************************************
//
//! Dispatch button events destined for this widget.
//!
//! \param psWidget is a pointer to the clock setting widget on which to
//! operate.
//! \param ui32Msg is the widget message to process.
//!
//! This function receives button/key event messages that are meant for
//! this widget.  It then calls the appropriate function to handle the
//! button event.
//!
//! \return Returns non-zero if the button event was handled, and 0 if the
//! button event was not handled.
//
//*****************************************************************************
static int32_t
ClockSetKeyHandler(tWidget *psWidget, uint32_t ui32Msg)
{
    tClockSetWidget *psClockWidget;

    ASSERT(psWidget);

    //
    // Get pointer to the clock setting widget.
    //
    psClockWidget = (tClockSetWidget *)psWidget;

    //
    // Process the key event
    //
    switch(ui32Msg)
    {
        //
        // Select key
        //
        case WIDGET_MSG_KEY_SELECT:
        {
            return(ClockSetKeySelect(psClockWidget));
        }

        //
        // Up button
        //
        case WIDGET_MSG_KEY_UP:
        {
            return(ClockSetKeyUp(psClockWidget));
        }

        //
        // Down button
        //
        case WIDGET_MSG_KEY_DOWN:
        {
            return(ClockSetKeyDown(psClockWidget));
        }

        //
        // Left button
        //
        case WIDGET_MSG_KEY_LEFT:
        {
            return(ClockSetKeyLeft(psClockWidget));
        }

        //
        // Right button
        //
        case WIDGET_MSG_KEY_RIGHT:
        {
            return(ClockSetKeyRight(psClockWidget));
        }

        //
        // This is an unexpected event.  Return an indication that the event
        // was not handled.
        //
        default:
        {
            return(0);
        }
    }
}

//*****************************************************************************
//
//! Handles messages for a clock setting  widget.
//!
//! \param psWidget is a pointer to the clock set widget.
//! \param ui32Msg is the message.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function receives messages intended for this clock set widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
int32_t
ClockSetMsgProc(tWidget *psWidget, uint32_t ui32Msg, uint32_t ui32Param1,
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
            ClockSetPaint(psWidget);

            //
            // Return one to indicate that the message was successfully
            // processed.
            //
            return(1);
        }

        //
        // Process any button/key event messages.
        //
        case WIDGET_MSG_KEY_SELECT:
        case WIDGET_MSG_KEY_UP:
        case WIDGET_MSG_KEY_DOWN:
        case WIDGET_MSG_KEY_LEFT:
        case WIDGET_MSG_KEY_RIGHT:
        {
            //
            // If the key event is for this widget, then process the key event
            //
            if((tWidget *)ui32Param1 == psWidget)
            {
                return(ClockSetKeyHandler(psWidget, ui32Msg));
            }
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
//! Initializes a clock setting widget.
//!
//! \param psWidget is a pointer to the clock set widget to initialize.
//! \param psDisplay is a pointer to the display on which to draw the widget.
//! \param i32X is the X coordinate of the upper left corner of the widget.
//! \param i32Y is the Y coordinate of the upper left corner of the widget.
//! \param i32Width is the width of the widget.
//! \param i32Height is the height of the widget.
//! \param psFont is the font to use for drawing text on the widget.
//! \param ui32ForegroundColor is the color of the text and lines on the
//! widget.
//! \param ui32BackgroundColor is the color of the widget background.
//! \param psTime is a pointer to the time structure to use for clock fields.
//! \param pfnOnOkClick is a callback function that is called when the user
//! selects the OK field on the display.
//!
//! This function initializes the caller provided clock setting widget.
//!
//! \return None.
//
//*****************************************************************************
void
ClockSetInit(tClockSetWidget *psWidget, const tDisplay *psDisplay,
             int32_t i32X, int32_t i32Y, int32_t i32Width, int32_t i32Height,
             tFont *psFont, uint32_t ui32ForegroundColor,
             uint32_t ui32BackgroundColor, struct tm *psTime,
             void (*pfnOnOkClick)(tWidget *psWidget, bool bOk))
{
    uint32_t ui32Idx;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);
    ASSERT(psDisplay);

    //
    // Clear out the widget structure.
    //
    for(ui32Idx = 0; ui32Idx < sizeof(tClockSetWidget); ui32Idx += 4)
    {
        ((uint32_t *)psWidget)[ui32Idx / 4] = 0;
    }

    //
    // Set the size of the widget structure.
    //
    psWidget->sBase.i32Size = sizeof(tClockSetWidget);

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
    psWidget->psFont = psFont;
    psWidget->ui32ForegroundColor = ui32ForegroundColor;
    psWidget->ui32BackgroundColor = ui32BackgroundColor;
    psWidget->psTime = psTime;
    psWidget->pfnOnOkClick = pfnOnOkClick;

    //
    // Use the clock set message handler to process messages to this widget.
    //
    psWidget->sBase.pfnMsgProc = ClockSetMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

