//*****************************************************************************
//
// listbox.c - A listbox widget.
//
// Copyright (c) 2008-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the Tiva Graphics Library.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/debug.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/listbox.h"

//*****************************************************************************
//
//! \addtogroup listbox_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Make sure that the abs() macro is defined.
//
//*****************************************************************************
#ifndef abs
#define abs(a) (((a) >= 0) ? (a) : (-(a)))
#endif

//*****************************************************************************
//
// Make sure min and max are defined.
//
//*****************************************************************************
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) < (b)) ? (b) : (a))
#endif

//*****************************************************************************
//
//! Draws the contents of a listbox.
//!
//! \param psWidget is a pointer to the listbox widget to be drawn.
//!
//! This function draws the contents of a listbox on the display.  This is
//! called in response to a \b #WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
ListBoxPaint(tWidget *psWidget)
{
    tListBoxWidget *pListBox;
    tContext sCtx;
    tRectangle sWidgetRect, sLineRect;
    int16_t i16Height;
    int32_t i32Width;
    uint16_t ui16String;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a listbox widget pointer.
    //
    pListBox = (tListBoxWidget *)psWidget;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sCtx, psWidget->psDisplay);
    GrContextFontSet(&sCtx, pListBox->psFont);

    //
    // Initialize the clipping region based on the extents of this listbox.
    //
    sWidgetRect = psWidget->sPosition;
    GrContextClipRegionSet(&sCtx, &sWidgetRect);

    //
    // See if the listbox outline style is selected.
    //
    if(pListBox->ui32Style & LISTBOX_STYLE_OUTLINE)
    {
        //
        // Outline the listbox with the outline color.
        //
        GrContextForegroundSet(&sCtx, pListBox->ui32OutlineColor);
        GrRectDraw(&sCtx, &(psWidget->sPosition));

        //
        // Shrink the widget region by one pixel on each side and draw another
        // rectangle, this time in the background color.  This ensures that the
        // text will not interfere with the colored border.
        //
        sWidgetRect.i16XMin++;
        sWidgetRect.i16YMin++;
        sWidgetRect.i16XMax--;
        sWidgetRect.i16YMax--;
        GrContextForegroundSet(&sCtx, pListBox->ui32BackgroundColor);
        GrRectDraw(&sCtx, &sWidgetRect);

        //
        // Reduce the size of the rectangle by another pixel to get the final
        // area into which we will put the text.
        //
        sWidgetRect.i16XMin++;
        sWidgetRect.i16YMin++;
        sWidgetRect.i16XMax--;
        sWidgetRect.i16YMax--;
        GrContextClipRegionSet(&sCtx, &sWidgetRect);
    }

    //
    // Start drawing at the top of the widget.
    //
    sLineRect = sWidgetRect;
    ui16String = pListBox->ui16StartEntry;
    i16Height = GrFontHeightGet(pListBox->psFont);

    //
    // Keep drawing until we reach the bottom of the listbox or run out of
    // strings to draw.
    //
    while((sLineRect.i16YMin < sWidgetRect.i16YMax) &&
          (ui16String < pListBox->ui16Populated))
    {
        //
        // Calculate the rectangle that will enclose this line of text.
        //
        sLineRect.i16YMax = sLineRect.i16YMin + i16Height - 1;

        //
        // Set foreground and background colors appropriately.
        //
        GrContextBackgroundSet(&sCtx, ((ui16String == pListBox->i16Selected) ?
                               pListBox->ui32SelectedBackgroundColor :
                               pListBox->ui32BackgroundColor));
        GrContextForegroundSet(&sCtx, ((ui16String == pListBox->i16Selected) ?
                               pListBox->ui32SelectedTextColor :
                               pListBox->ui32TextColor));

        //
        // Draw the text.
        //
        GrStringDraw(&sCtx, pListBox->ppcText[ui16String], -1,
                     sLineRect.i16XMin, sLineRect.i16YMin, 1);

        //
        // Determine the width of the string we just rendered.
        //
        i32Width = GrStringWidthGet(&sCtx, pListBox->ppcText[ui16String], -1);

        //
        // Do we need to clear the area to the right of the string?
        //
        if(i32Width < (sLineRect.i16XMax - sLineRect.i16XMin + 1))
        {
            //
            // Yes - we need to fill the right side of this string with
            // background color.
            //
            GrContextForegroundSet(&sCtx,
                                   ((ui16String == pListBox->i16Selected) ?
                                    pListBox->ui32SelectedBackgroundColor :
                                    pListBox->ui32BackgroundColor));
            sLineRect.i16XMin += i32Width;
            GrRectFill(&sCtx, &sLineRect);
            sLineRect.i16XMin = sWidgetRect.i16XMin;
        }

        //
        // Move on to the next string, wrapping if necessary.
        //
        ui16String++;
        if(ui16String == pListBox->ui16MaxEntries)
        {
            ui16String = 0;
        }
        sLineRect.i16YMin += i16Height;

        //
        // If we are wrapping and got back at the oldest entry, we drop out.
        //
        if(ui16String == pListBox->ui16OldestEntry)
        {
            break;
        }
    }

    //
    // Fill the remainder of the listbox area with the background color.
    //
    if(sLineRect.i16YMin < sWidgetRect.i16YMax)
    {
        //
        // Determine the rectangle to be filled.
        //
        sLineRect.i16YMax = sWidgetRect.i16YMax;

        //
        // Fill the rectangle with the background color.
        //
        GrContextForegroundSet(&sCtx, pListBox->ui32BackgroundColor);
        GrRectFill(&sCtx, &sLineRect);
    }
}

//*****************************************************************************
//
// Handles pointer messages for a listbox widget.
//
// \param pListBox is a pointer to the listbox widget.
// \param ui32Msg is the message.
// \param i32X is the X coordinate of the pointer.
// \param i32Y is the Y coordinate of the pointer.
//
// This function receives pointer messages intended for this listbox widget
// and processes them accordingly.
//
// \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
static int32_t
ListBoxPointer(tListBoxWidget *pListBox, uint32_t ui32Msg, int32_t i32X,
               int32_t i32Y)
{
    int32_t i32LineNum, i32Entry, i32Visible, i32MaxUp, i32MaxDown, i32Scroll;

    switch(ui32Msg)
    {
        //
        // The touchscreen has been pressed.
        //
        case WIDGET_MSG_PTR_DOWN:
        {
            //
            // Is the pointer press within the bounds of this widget?
            //
            if(!GrRectContainsPoint(&(pListBox->sBase.sPosition), i32X, i32Y))
            {
                //
                // This is not a message for us so return 0 to indicate that
                // we did not process it.
                //
                return(0);
            }
            else
            {
                //
                // The pointer was pressed within this control.  Remember the Y
                // coordinate and reset or scrolling flag.
                //
                pListBox->ui16Scrolled = 0;
                pListBox->i32PointerY = i32Y;

                //
                // Return 1 to indicate to the widget manager that we processed
                // the message.  This widget will now receive all pointer move
                // messages until the pointer is released.
                //
                return(1);
            }
        }

        //
        // The touchscreen has been released.
        //
        case WIDGET_MSG_PTR_UP:
        {
            //
            // If the pointer is still within the bounds of the control and
            // we have not scrolled the contents since the last time the
            // pointer was pressed, we assume that this is a tap rather than
            // a drag and select the element that falls beneath the current
            // pointer position.  If the pointer is outside our control, if
            // we have scrolled already or if the control is locked, don't
            // change the selection.
            //
            if((pListBox->ui16Scrolled == 0) &&
               !(pListBox->ui32Style & LISTBOX_STYLE_LOCKED) &&
                GrRectContainsPoint(&(pListBox->sBase.sPosition), i32X, i32Y))
            {
                //
                // It seems we need to change the selected element. What is
                // the display line number that has been clicked on?
                //
                i32LineNum = (i32Y -
                              (int32_t)pListBox->sBase.sPosition.i16YMin) /
                               GrFontHeightGet(pListBox->psFont);

                //
                // We now know the location of the click as a number of text
                // lines from the top of the list box.  Now determine what
                // entry is shown there, remembering that the index may wrap.
                //
                i32Entry = ((int32_t)pListBox->ui16StartEntry + i32LineNum) %
                            pListBox->ui16MaxEntries;

                //
                // If this is an unpopulated entry or the current selection,
                // clear the selection.
                //
                if((i32Entry >= (int32_t)pListBox->ui16Populated) ||
                   (i32Entry == (int32_t)pListBox->i16Selected))
                {
                    //
                    // Yes - update the selection and force a repaint.
                    //
                    pListBox->i16Selected = (int16_t)0xFFFF;
                }
                else
                {
                    //
                    // The pointer was tapped on a valid entry other than the
                    // current selection so change the selection.
                    //
                    pListBox->i16Selected = (int16_t)i32Entry;
                }

                //
                // Force a repaint of the widget.
                //
                WidgetPaint((tWidget *)pListBox);

                //
                // Tell the client that the selection changed.
                //
                if(pListBox->pfnOnChange)
                {
                    (pListBox->pfnOnChange)((tWidget *)pListBox,
                                            pListBox->i16Selected);
                }
            }

            //
            // We process all pointer up messages so return 1 to tell the
            // widget manager this.
            //
            return(1);
        }

        //
        // The pointer is moving while pressed.
        //
        case WIDGET_MSG_PTR_MOVE:
        {
            //
            // How far has the pointer moved vertically from the point where it
            // was pressed or where we last registered a scroll?  i32LineNum
            // will be negative for downward scrolling.
            //
            i32LineNum = pListBox->i32PointerY - i32Y;

            //
            // If this distance is greater than or equal to the height of a
            // line of text, we need to check to see if we need to scroll the
            // list box contents.
            //
            if(abs(i32LineNum) >= GrFontHeightGet(pListBox->psFont))
            {
                //
                // We have to scroll if this is possible.  How many lines can
                // be visible on the display?
                //
                i32Visible = (pListBox->sBase.sPosition.i16YMax -
                              pListBox->sBase.sPosition.i16YMin) /
                             (int32_t)GrFontHeightGet(pListBox->psFont);

                //
                // If we have fewer strings in the listbox than there are lines
                // on the display, scrolling is not possible so give up now.
                //
                if(i32Visible > (int32_t)pListBox->ui16Populated)
                {
                    return(1);
                }

                //
                // How many lines of scrolling does the latest pointer position
                // indicate?  A negative value implies downward scrolling (i.e.
                // showing earlier strings).
                //
                i32Scroll = i32LineNum /
                            (int32_t)GrFontHeightGet(pListBox->psFont);

                //
                // What is the farthest we could scroll downwards (i.e. moving
                // the pointer towards the bottom of the screen)?  Note - this
                // will be negative or 0.
                //
                i32MaxDown =
                    (pListBox->ui16StartEntry >= pListBox->ui16OldestEntry) ?
                    (pListBox->ui16OldestEntry - pListBox->ui16StartEntry ) :
                    ((pListBox->ui16OldestEntry - pListBox->ui16StartEntry) -
                     pListBox->ui16MaxEntries);

                //
                // What is the farthest we could scroll upwards?  Note - this
                // will be a positive number.
                //
                i32MaxUp = ((int32_t)pListBox->ui16Populated - i32Visible) +
                           i32MaxDown;

                //
                // Determine the actual scroll distance given the maximum
                // distances calculated.
                //
                i32Scroll = min(i32Scroll, i32MaxUp);
                i32Scroll = max(i32Scroll, i32MaxDown);

                if(i32Scroll)
                {
                    int32_t i32Temp;

                    //
                    // Adjust the start entry appropriately, taking care to
                    // handle the wrap case.  The use of a temporary variable
                    // here is required to work around a compiler bug which
                    // resulted in an invalid value of pListBox->ui16StartEntry
                    // following the calculation.
                    //
                    i32Temp = pListBox->ui16StartEntry;
                    i32Temp += i32Scroll;
                    i32Temp %= (int32_t)pListBox->ui16MaxEntries;
                    pListBox->ui16StartEntry = (uint16_t)i32Temp;

                    //
                    // Remember that we scrolled.
                    //
                    pListBox->ui16Scrolled = 1;

                    //
                    // Adjust the pointer position we record to take into account
                    // the amount we just scrolled.
                    //
                    pListBox->i32PointerY -= (i32Scroll *
                                            GrFontHeightGet(pListBox->psFont));

                    //
                    // Repaint the contents of the widget.
                    //
                    WidgetPaint((tWidget *)pListBox);
                }
            }

            return(1);
        }
    }

    //
    // We don't handle any other messages so return 0 if we get these.
    //
    return(0);
}

//*****************************************************************************
//
//! Handles messages for a listbox widget.
//!
//! \param psWidget is a pointer to the listbox widget.
//! \param ui32Msg is the message.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function receives messages intended for this listbox widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
int32_t
ListBoxMsgProc(tWidget *psWidget, uint32_t ui32Msg, uint32_t ui32Param1,
              uint32_t ui32Param2)
{
    tListBoxWidget *pListBox;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic pointer to a list box pointer.
    //
    pListBox = (tListBoxWidget *)psWidget;

    //
    // Determine which message is being sent.
    //
    switch(ui32Msg)
    {
        //
        // A pointer message has been received.
        //
        case WIDGET_MSG_PTR_DOWN:
        case WIDGET_MSG_PTR_UP:
        case WIDGET_MSG_PTR_MOVE:
            return(ListBoxPointer(pListBox, ui32Msg, (int32_t)ui32Param1,
                                  (int32_t)ui32Param2));

        //
        // The widget paint request has been sent.
        //
        case WIDGET_MSG_PAINT:
        {
            //
            // Handle the widget paint request.
            //
            ListBoxPaint(psWidget);

            //
            // Return one to indicate that the message was successfully
            // processed.
            //
            return(1);
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
//! Initializes a listbox widget.
//!
//! \param psWidget is a pointer to the listbox widget to initialize.
//! \param psDisplay is a pointer to the display on which to draw the listbox.
//! \param ppcText is a pointer to an array of character pointers which will
//! hold the strings that the listbox displays.
//! \param ui16MaxEntries provides the total number of entries in the
//! \e ppcText array.
//! \param ui16PopulatedEntries provides the number of entries in the
//! \e ppcText array which are populated.
//! \param i32X is the X coordinate of the upper left corner of the listbox.
//! \param i32Y is the Y coordinate of the upper left corner of the listbox.
//! \param i32Width is the width of the listbox.
//! \param i32Height is the height of the listbox.
//!
//! This function initializes the provided listbox widget.
//!
//! \return None.
//
//*****************************************************************************
void
ListBoxInit(tListBoxWidget *psWidget, const tDisplay *psDisplay,
            const char **ppcText, uint16_t ui16MaxEntries,
            uint16_t ui16PopulatedEntries, int32_t i32X, int32_t i32Y,
            int32_t i32Width, int32_t i32Height)
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
    for(ui32Idx = 0; ui32Idx < sizeof(tListBoxWidget); ui32Idx += 4)
    {
        ((uint32_t *)psWidget)[ui32Idx / 4] = 0;
    }

    //
    // Set the size of the listbox widget structure.
    //
    psWidget->sBase.i32Size = sizeof(tListBoxWidget);

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
    // Set the extents of this listbox.
    //
    psWidget->sBase.sPosition.i16XMin = i32X;
    psWidget->sBase.sPosition.i16YMin = i32Y;
    psWidget->sBase.sPosition.i16XMax = i32X + i32Width - 1;
    psWidget->sBase.sPosition.i16YMax = i32Y + i32Height - 1;

    //
    // Use the listbox message handler to process messages to this listbox.
    //
    psWidget->sBase.pfnMsgProc = ListBoxMsgProc;

    //
    // Initialize some of the widget fields that are not accessible via
    // macros.
    //
    psWidget->ppcText = ppcText;
    psWidget->ui16MaxEntries = ui16MaxEntries;
    psWidget->ui16Populated = ui16PopulatedEntries;
    psWidget->i16Selected = (int16_t)0xFFFF;
}

//*****************************************************************************
//
//! Adds a line of text to a listbox.
//!
//! \param pListBox is a pointer to the listbox widget that is to receive the
//! new text string.
//! \param pcTxt is a pointer to the string that is to be added to the listbox.
//!
//! This function adds a new string to the listbox.  If the listbox has
//! style \b #LISTBOX_STYLE_WRAP and the current string table is full, this
//! function will discard the oldest string and replace it with the one passed
//! here.  If this style flag is absent, the function will return -1 if no
//! empty entries exist in the string table for the widget.
//!
//! The display is not automatically updated as a result of this function call.
//! An application must call WidgetPaint() to update the display after adding
//! a new string to the listbox.
//!
//! \note To replace the string associated with a particular, existing element
//! in the listbox, use ListBoxTextSet().
//!
//! \return Returns the string table index into which the new string has been
//! placed if successful or -1 if the string table is full and
//! \b #LISTBOX_STYLE_WRAP is not set.
//
//*****************************************************************************
int32_t ListBoxTextAdd(tListBoxWidget *pListBox, const char *pcTxt)
{
    uint32_t ui32Index;

    //
    // Is the list box full?
    //
    if(pListBox->ui16Populated == pListBox->ui16MaxEntries)
    {
        //
        // The box is already full.  If the wrap style is not set, fail
        // the call.
        //
        if(!(pListBox->ui32Style & LISTBOX_STYLE_WRAP))
        {
            //
            // The listbox is full and it is not configured to wrap so we can't
            // add another string to it.
            //
            return(-1);
        }
        else
        {
            //
            // We are wrapping so replace the oldest entry in the box.
            //
            ui32Index = pListBox->ui16OldestEntry;

            //
            // Check to see if we are displaying the oldest entry and, if so,
            // move the start entry on by one to keep the display order
            // correct.
            //
            if(pListBox->ui16OldestEntry == pListBox->ui16StartEntry)
            {
                pListBox->ui16StartEntry++;
                if(pListBox->ui16StartEntry == pListBox->ui16MaxEntries)
                {
                    pListBox->ui16StartEntry = 0;
                }
            }

            //
            // The new oldest entry is the next one.  Update the index and
            // take care to wrap if we reach the end of the string table.
            //
            pListBox->ui16OldestEntry++;
            if(pListBox->ui16OldestEntry == pListBox->ui16MaxEntries)
            {
                pListBox->ui16OldestEntry = 0;
            }
        }
    }
    else
    {
        //
        // The listbox is not full so add the new string to the first free
        // slot in the string table.
        //
        ui32Index = pListBox->ui16Populated;
        pListBox->ui16Populated++;
    }

    //
    // Save the new string in the appropriate string table entry.
    //
    pListBox->ppcText[ui32Index] = pcTxt;

    //
    // Tell the caller which string table entry was added.
    //
    return((int32_t)ui32Index);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
