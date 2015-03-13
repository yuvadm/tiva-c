//*****************************************************************************
//
// checkbox.c - Check box widget.
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
#include "grlib/checkbox.h"

//*****************************************************************************
//
//! \addtogroup checkbox_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Draws a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to be drawn.
//! \param bClick is a boolean that is \b true if the paint request is a result
//! of a pointer click and \b false if not.
//!
//! This function draws a check box widget on the display.  This is called in
//! response to a \b #WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
CheckBoxPaint(tWidget *psWidget, bool bClick)
{
    tCheckBoxWidget *pCheck;
    tRectangle i16Rect;
    tContext sCtx;
    int32_t i32Y;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a check box widget pointer.
    //
    pCheck = (tCheckBoxWidget *)psWidget;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sCtx, psWidget->psDisplay);

    //
    // Initialize the clipping region based on the extents of this check box.
    //
    GrContextClipRegionSet(&sCtx, &(psWidget->sPosition));

    //
    // See if the check box fill style is selected.
    //
    if((pCheck->ui16Style & CB_STYLE_FILL) && !bClick)
    {
        //
        // Fill the check box with the fill color.
        //
        GrContextForegroundSet(&sCtx, pCheck->ui32FillColor);
        GrRectFill(&sCtx, &(psWidget->sPosition));
    }

    //
    // See if the check box outline style is selected.
    //
    if((pCheck->ui16Style & CB_STYLE_OUTLINE) && !bClick)
    {
        //
        // Outline the check box with the outline color.
        //
        GrContextForegroundSet(&sCtx, pCheck->ui32OutlineColor);
        GrRectDraw(&sCtx, &(psWidget->sPosition));
    }

    //
    // Draw the check box.
    //
    i16Rect.i16XMin = psWidget->sPosition.i16XMin + 2;
    i16Rect.i16YMin = (psWidget->sPosition.i16YMin +
                   ((psWidget->sPosition.i16YMax - psWidget->sPosition.i16YMin -
                     pCheck->ui16BoxSize + 1) / 2));
    i16Rect.i16XMax = i16Rect.i16XMin + pCheck->ui16BoxSize - 1;
    i16Rect.i16YMax = i16Rect.i16YMin + pCheck->ui16BoxSize - 1;
    if(!bClick)
    {
        GrContextForegroundSet(&sCtx, pCheck->ui32OutlineColor);
        GrRectDraw(&sCtx, &i16Rect);
    }

    //
    // Select the foreground color based on whether or not the check box is
    // selected.
    //
    if(pCheck->ui16Style & CB_STYLE_SELECTED)
    {
        GrContextForegroundSet(&sCtx, pCheck->ui32OutlineColor);
    }
    else
    {
        GrContextForegroundSet(&sCtx, pCheck->ui32FillColor);
    }

    //
    // Draw an "X" in the check box.
    //
    GrLineDraw(&sCtx, i16Rect.i16XMin + 1, i16Rect.i16YMin + 1,
               i16Rect.i16XMax - 1, i16Rect.i16YMax - 1);
    GrLineDraw(&sCtx, i16Rect.i16XMin + 1, i16Rect.i16YMax - 1,
               i16Rect.i16XMax - 1, i16Rect.i16YMin + 1);

    //
    // See if the check box text or image style is selected.
    //
    if((pCheck->ui16Style & (CB_STYLE_TEXT | CB_STYLE_IMG)) && !bClick)
    {
        //
        // Shrink the clipping region by the size of the check box so that it
        // is not overwritten by further "decorative" portions of the widget.
        //
        sCtx.sClipRegion.i16XMin += pCheck->ui16BoxSize + 4;

        //
        // If the check box outline style is selected then shrink the clipping
        // region by one pixel on each side so that the outline is not
        // overwritten by the text or image.
        //
        if(pCheck->ui16Style & CB_STYLE_OUTLINE)
        {
            sCtx.sClipRegion.i16YMin++;
            sCtx.sClipRegion.i16XMax--;
            sCtx.sClipRegion.i16YMax--;
        }

        //
        // See if the check box image style is selected.
        //
        if(pCheck->ui16Style & CB_STYLE_IMG)
        {
            //
            // Determine where along the Y extent of the widget to draw the
            // image.  It is drawn at the top if it takes all (or more than
            // all) of the Y extent of the widget, and it is drawn centered if
            // it takes less than the Y extent.
            //
            if(GrImageHeightGet(pCheck->pui8Image) >
               (sCtx.sClipRegion.i16YMax - sCtx.sClipRegion.i16YMin))
            {
                i32Y = sCtx.sClipRegion.i16YMin;
            }
            else
            {
                i32Y = (sCtx.sClipRegion.i16YMin +
                      ((sCtx.sClipRegion.i16YMax - sCtx.sClipRegion.i16YMin -
                        GrImageHeightGet(pCheck->pui8Image) + 1) / 2));
            }

            //
            // Set the foreground and background colors to use for 1 BPP
            // images.
            //
            GrContextForegroundSet(&sCtx, pCheck->ui32TextColor);
            GrContextBackgroundSet(&sCtx, pCheck->ui32FillColor);

            //
            // Draw the image next to the check box.
            //
            GrImageDraw(&sCtx, pCheck->pui8Image, sCtx.sClipRegion.i16XMin,
                        i32Y);
        }

        //
        // See if the check box text style is selected.
        //
        if(pCheck->ui16Style & CB_STYLE_TEXT)
        {
            //
            // Determine where along the Y extent of the widget to draw the
            // string.  It is drawn at the top if it takes all (or more than
            // all) of the Y extent of the widget, and it is drawn centered if
            // it takes less than the Y extent.
            //
            if(GrFontHeightGet(pCheck->psFont) >
               (sCtx.sClipRegion.i16YMax - sCtx.sClipRegion.i16YMin))
            {
                i32Y = sCtx.sClipRegion.i16YMin;
            }
            else
            {
                i32Y = (sCtx.sClipRegion.i16YMin +
                      ((sCtx.sClipRegion.i16YMax - sCtx.sClipRegion.i16YMin -
                        GrFontHeightGet(pCheck->psFont) + 1) / 2));
            }

            //
            // Draw the text next to the check box.
            //
            GrContextFontSet(&sCtx, pCheck->psFont);
            GrContextForegroundSet(&sCtx, pCheck->ui32TextColor);
            GrContextBackgroundSet(&sCtx, pCheck->ui32FillColor);
            GrStringDraw(&sCtx, pCheck->pcText, -1, sCtx.sClipRegion.i16XMin,
                         i32Y, pCheck->ui16Style & CB_STYLE_TEXT_OPAQUE);
        }
    }
}

//*****************************************************************************
//
//! Handles pointer events for a check box.
//!
//! \param psWidget is a pointer to the check box widget.
//! \param ui32Msg is the pointer event message.
//! \param i32X is the X coordinate of the pointer event.
//! \param i32Y is the Y coordinate of the pointer event.
//!
//! This function processes pointer event messages for a check box.  This is
//! called in response to a \b #WIDGET_MSG_PTR_DOWN, \b #WIDGET_MSG_PTR_MOVE,
//! and \b #WIDGET_MSG_PTR_UP messages.
//!
//! If the \b #WIDGET_MSG_PTR_UP message is received with a position within the
//! extents of the check box, the check box's selected state will be toggled
//! and its OnChange function is called.
//!
//! \return Returns 1 if the coordinates are within the extents of the check
//! box and 0 otherwise.
//
//*****************************************************************************
static int32_t
CheckBoxClick(tWidget *psWidget, uint32_t ui32Msg, int32_t i32X, int32_t i32Y)
{
    tCheckBoxWidget *pCheck;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a check box widget pointer.
    //
    pCheck = (tCheckBoxWidget *)psWidget;

    //
    // See if the given coordinates are within the extents of the check box.
    //
    if((i32X >= psWidget->sPosition.i16XMin) &&
       (i32X <= psWidget->sPosition.i16XMax) &&
       (i32Y >= psWidget->sPosition.i16YMin) &&
       (i32Y <= psWidget->sPosition.i16YMax))
    {
        //
        // See if the pointer was just raised.
        //
        if(ui32Msg == WIDGET_MSG_PTR_UP)
        {
            //
            // Toggle the selected state of this check box.
            //
            pCheck->ui16Style ^= CB_STYLE_SELECTED;

            //
            // Redraw the check box based on the new selected state.
            //
            CheckBoxPaint(psWidget, 1);

            //
            // If there is an OnChange callback for this widget then call the
            // callback.
            //
            if(pCheck->pfnOnChange)
            {
                pCheck->pfnOnChange(psWidget,
                                    pCheck->ui16Style & CB_STYLE_SELECTED);
            }
        }

        //
        // These coordinates are within the extents of the check box widget.
        //
        return(1);
    }

    //
    // These coordinates are not within the extents of the check box widget.
    //
    return(0);
}

//*****************************************************************************
//
//! Handles messages for a check box widget.
//!
//! \param psWidget is a pointer to the check box widget.
//! \param ui32Msg is the message.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function receives messages intended for this check box widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
int32_t
CheckBoxMsgProc(tWidget *psWidget, uint32_t ui32Msg, uint32_t ui32Param1,
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
            CheckBoxPaint(psWidget, 0);

            //
            // Return one to indicate that the message was successfully
            // processed.
            //
            return(1);
        }

        //
        // One of the pointer requests has been sent.
        //
        case WIDGET_MSG_PTR_DOWN:
        case WIDGET_MSG_PTR_MOVE:
        case WIDGET_MSG_PTR_UP:
        {
            //
            // Handle the pointer request, returning the appropriate value.
            //
            return(CheckBoxClick(psWidget, ui32Msg, ui32Param1, ui32Param2));
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
//! Initializes a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to initialize.
//! \param psDisplay is a pointer to the display on which to draw the check box.
//! \param i32X is the X coordinate of the upper left corner of the check box.
//! \param i32Y is the Y coordinate of the upper left corner of the check box.
//! \param i32Width is the width of the check box.
//! \param i32Height is the height of the check box.
//!
//! This function initializes the provided check box widget.
//!
//! \return None.
//
//*****************************************************************************
void
CheckBoxInit(tCheckBoxWidget *psWidget, const tDisplay *psDisplay, int32_t i32X,
             int32_t i32Y, int32_t i32Width, int32_t i32Height)
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
    for(ui32Idx = 0; ui32Idx < sizeof(tCheckBoxWidget); ui32Idx += 4)
    {
        ((uint32_t *)psWidget)[ui32Idx / 4] = 0;
    }

    //
    // Set the size of the check box widget structure.
    //
    psWidget->sBase.i32Size = sizeof(tCheckBoxWidget);

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
    // Set the extents of this check box.
    //
    psWidget->sBase.sPosition.i16XMin = i32X;
    psWidget->sBase.sPosition.i16YMin = i32Y;
    psWidget->sBase.sPosition.i16XMax = i32X + i32Width - 1;
    psWidget->sBase.sPosition.i16YMax = i32Y + i32Height - 1;

    //
    // Use the check box message handler to processage messages to this check
    // box.
    //
    psWidget->sBase.pfnMsgProc = CheckBoxMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
