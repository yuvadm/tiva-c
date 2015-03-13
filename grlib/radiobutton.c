//*****************************************************************************
//
// radiobutton.c - Radio button widget.
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
#include "grlib/radiobutton.h"

//*****************************************************************************
//
//! \addtogroup radiobutton_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Draws a radio button widget.
//!
//! \param psWidget is a pointer to the radio button widget to be drawn.
//! \param bClick is a boolean that is \b true if the paint request is a result
//! of a pointer click and \b false if not.
//!
//! This function draws a radio button widget on the display.  This is called
//! in response to a \b #WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
RadioButtonPaint(tWidget *psWidget, uint32_t bClick)
{
    tRadioButtonWidget *pRadio;
    tContext sCtx;
    int32_t i32X, i32Y;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a radio button widget pointer.
    //
    pRadio = (tRadioButtonWidget *)psWidget;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sCtx, psWidget->psDisplay);

    //
    // Initialize the clipping region based on the extents of this radio
    // button.
    //
    GrContextClipRegionSet(&sCtx, &(psWidget->sPosition));

    //
    // See if the radio button fill style is selected.
    //
    if((pRadio->ui16Style & RB_STYLE_FILL) && !bClick)
    {
        //
        // Fill the radio button with the fill color.
        //
        GrContextForegroundSet(&sCtx, pRadio->ui32FillColor);
        GrRectFill(&sCtx, &(psWidget->sPosition));
    }

    //
    // See if the radio button outline style is selected.
    //
    if((pRadio->ui16Style & RB_STYLE_OUTLINE) && !bClick)
    {
        //
        // Outline the radio button with the outline color.
        //
        GrContextForegroundSet(&sCtx, pRadio->ui32OutlineColor);
        GrRectDraw(&sCtx, &(psWidget->sPosition));
    }

    //
    // Draw the radio button.
    //
    i32X = psWidget->sPosition.i16XMin + (pRadio->ui16CircleSize / 2) + 2;
    i32Y = (psWidget->sPosition.i16YMin +
          ((psWidget->sPosition.i16YMax - psWidget->sPosition.i16YMin) / 2));
    if(!bClick)
    {
        GrContextForegroundSet(&sCtx, pRadio->ui32OutlineColor);
        GrCircleDraw(&sCtx, i32X, i32Y, pRadio->ui16CircleSize / 2);
    }

    //
    // Select the foreground color based on whether or not the radio button is
    // selected.
    //
    if(pRadio->ui16Style & RB_STYLE_SELECTED)
    {
        GrContextForegroundSet(&sCtx, pRadio->ui32OutlineColor);
    }
    else
    {
        GrContextForegroundSet(&sCtx, pRadio->ui32FillColor);
    }

    //
    // Fill in the radio button.
    //
    GrCircleFill(&sCtx, i32X, i32Y, (pRadio->ui16CircleSize / 2) - 2);

    //
    // See if the radio button text or image style is selected.
    //
    if((pRadio->ui16Style & (RB_STYLE_TEXT | RB_STYLE_IMG)) && !bClick)
    {
        //
        // Shrink the clipping region by the size of the radio button so that
        // it is not overwritten by further "decorative" portions of the
        // widget.
        //
        sCtx.sClipRegion.i16XMin += pRadio->ui16CircleSize + 4;

        //
        // If the radio button outline style is selected then shrink the
        // clipping region by one pixel on each side so that the outline is not
        // overwritten by the text or image.
        //
        if(pRadio->ui16Style & RB_STYLE_OUTLINE)
        {
            sCtx.sClipRegion.i16YMin++;
            sCtx.sClipRegion.i16XMax--;
            sCtx.sClipRegion.i16YMax--;
        }

        //
        // See if the radio button image style is selected.
        //
        if(pRadio->ui16Style & RB_STYLE_IMG)
        {
            //
            // Determine where along the Y extent of the widget to draw the
            // image.  It is drawn at the top if it takes all (or more than
            // all) of the Y extent of the widget, and it is drawn centered if
            // it takes less than the Y extent.
            //
            if(GrImageHeightGet(pRadio->pui8Image) >
               (sCtx.sClipRegion.i16YMax - sCtx.sClipRegion.i16YMin))
            {
                i32Y = sCtx.sClipRegion.i16YMin;
            }
            else
            {
                i32Y = (sCtx.sClipRegion.i16YMin +
                      ((sCtx.sClipRegion.i16YMax - sCtx.sClipRegion.i16YMin -
                        GrImageHeightGet(pRadio->pui8Image) + 1) / 2));
            }

            //
            // Set the foreground and background colors to use for 1 BPP
            // images.
            //
            GrContextForegroundSet(&sCtx, pRadio->ui32TextColor);
            GrContextBackgroundSet(&sCtx, pRadio->ui32FillColor);

            //
            // Draw the image next to the radio button.
            //
            GrImageDraw(&sCtx, pRadio->pui8Image, sCtx.sClipRegion.i16XMin,
                        i32Y);
        }

        //
        // See if the radio button text style is selected.
        //
        if(pRadio->ui16Style & RB_STYLE_TEXT)
        {
            //
            // Determine where along the Y extent of the widget to draw the
            // string.  It is drawn at the top if it takes all (or more than
            // all) of the Y extent of the widget, and it is drawn centered if
            // it takes less than the Y extent.
            //
            if(GrFontHeightGet(pRadio->psFont) >
               (sCtx.sClipRegion.i16YMax - sCtx.sClipRegion.i16YMin))
            {
                i32Y = sCtx.sClipRegion.i16YMin;
            }
            else
            {
                i32Y = (sCtx.sClipRegion.i16YMin +
                      ((sCtx.sClipRegion.i16YMax - sCtx.sClipRegion.i16YMin -
                        GrFontHeightGet(pRadio->psFont) + 1) / 2));
            }

            //
            // Draw the text next to the radio button.
            //
            GrContextFontSet(&sCtx, pRadio->psFont);
            GrContextForegroundSet(&sCtx, pRadio->ui32TextColor);
            GrContextBackgroundSet(&sCtx, pRadio->ui32FillColor);
            GrStringDraw(&sCtx, pRadio->pcText, -1, sCtx.sClipRegion.i16XMin,
                         i32Y, pRadio->ui16Style & RB_STYLE_TEXT_OPAQUE);
        }
    }
}

//*****************************************************************************
//
//! Handles pointer events for a radio button.
//!
//! \param psWidget is a pointer to the radio button widget.
//! \param ui32Msg is the pointer event message.
//! \param i32X is the X coordinate of the pointer event.
//! \param i32Y is the Y coordiante of the pointer event.
//!
//! This function processes pointer event messages for a radio button.  This is
//! called in response to a \b #WIDGET_MSG_PTR_DOWN, \b #WIDGET_MSG_PTR_MOVE,
//! and \b #WIDGET_MSG_PTR_UP messages.
//!
//! If the \b #WIDGET_MSG_PTR_UP message is received with a position within the
//! extents of the radio button, the radio button's selected state will be
//! unchanged if it is already selected.  If it is not selected, it will be
//! selected, its OnChange function will be called, and the peer radio button
//! widget that is selected will be unselected, causing its OnChange to be
//! called as well.
//!
//! \return Returns 1 if the coordinates are within the extents of the radio
//! button and 0 otherwise.
//
//*****************************************************************************
static int32_t
RadioButtonClick(tWidget *psWidget, uint32_t ui32Msg, int32_t i32X,
                 int32_t i32Y)
{
    tRadioButtonWidget *pRadio, *pRadio2;
    tWidget *pSibling;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a radio button widget pointer.
    //
    pRadio = (tRadioButtonWidget *)psWidget;

    //
    // See if the given coordinates are within the extents of the radio button.
    //
    if((i32X >= psWidget->sPosition.i16XMin) &&
       (i32X <= psWidget->sPosition.i16XMax) &&
       (i32Y >= psWidget->sPosition.i16YMin) &&
       (i32Y <= psWidget->sPosition.i16YMax))
    {
        //
        // See if the pointer was just raised and this radio button is not
        // selected.
        //
        if((ui32Msg == WIDGET_MSG_PTR_UP) &&
           !(pRadio->ui16Style & RB_STYLE_SELECTED))
        {
            //
            // Loop through the siblings of this radio button widget.
            //
            for(pSibling = psWidget->psParent->psChild; pSibling;
                pSibling = pSibling->psNext)
            {
                //
                // Skip this widget if it is not a radio button widget, or if
                // it is the original radio button widget.
                //
                if((pSibling == psWidget) ||
                   (pSibling->pfnMsgProc != RadioButtonMsgProc))
                {
                    continue;
                }

                //
                // Convert the generic widget pointer into a radio button
                // widget pointer.
                //
                pRadio2 = (tRadioButtonWidget *)pSibling;

                //
                // See if the sibling radio button is selected.
                //
                if(pRadio2->ui16Style & RB_STYLE_SELECTED)
                {
                    //
                    // Clear the selected state of the sibling radio button.
                    //
                    pRadio2->ui16Style &= ~(RB_STYLE_SELECTED);

                    //
                    // Redraw the sibling radio button.
                    //
                    RadioButtonPaint(pSibling, 1);

                    //
                    // If there is an OnChange callback for the sibling radio
                    // button then call the callback.
                    //
                    if(pRadio2->pfnOnChange)
                    {
                        pRadio2->pfnOnChange(pSibling, 0);
                    }
                }
            }

            //
            // Set the selected state of this radio button.
            //
            pRadio->ui16Style |= RB_STYLE_SELECTED;

            //
            // Redraw the radio button.
            //
            RadioButtonPaint(psWidget, 1);

            //
            // If there is an OnChange callback for this widget then call the
            // callback.
            //
            if(pRadio->pfnOnChange)
            {
                pRadio->pfnOnChange(psWidget, 1);
            }
        }

        //
        // These coordinates are within the extents of the radio button widget.
        //
        return(1);
    }

    //
    // These coordinates are not within the extents of the radio button widget.
    //
    return(0);
}

//*****************************************************************************
//
//! Handles messages for a radio button widget.
//!
//! \param psWidget is a pointer to the radio button widget.
//! \param ui32Msg is the message.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function receives messages intended for this radio button widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
int32_t
RadioButtonMsgProc(tWidget *psWidget, uint32_t ui32Msg,
                   uint32_t ui32Param1, uint32_t ui32Param2)
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
            RadioButtonPaint(psWidget, 0);

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
            return(RadioButtonClick(psWidget, ui32Msg, ui32Param1,
                                    ui32Param2));
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
//! Initializes a radio button widget.
//!
//! \param psWidget is a pointer to the radio button widget to initialize.
//! \param psDisplay is a pointer to the display on which to draw the push
//! button.
//! \param i32X is the X coordinate of the upper left corner of the radio
//! button.
//! \param i32Y is the Y coordinate of the upper left corner of the radio
//! button.
//! \param i32Width is the width of the radio button.
//! \param i32Height is the height of the radio button.
//!
//! This function initializes the provided radio button widget.
//!
//! \return None.
//
//*****************************************************************************
void
RadioButtonInit(tRadioButtonWidget *psWidget, const tDisplay *psDisplay,
                int32_t i32X, int32_t i32Y, int32_t i32Width,
                int32_t i32Height)
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
    for(ui32Idx = 0; ui32Idx < sizeof(tRadioButtonWidget); ui32Idx += 4)
    {
        ((uint32_t *)psWidget)[ui32Idx / 4] = 0;
    }

    //
    // Set the size of the radio button widget structure.
    //
    psWidget->sBase.i32Size = sizeof(tRadioButtonWidget);

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
    // Set the extents of this radio button.
    //
    psWidget->sBase.sPosition.i16XMin = i32X;
    psWidget->sBase.sPosition.i16YMin = i32Y;
    psWidget->sBase.sPosition.i16XMax = i32X + i32Width - 1;
    psWidget->sBase.sPosition.i16YMax = i32Y + i32Height - 1;

    //
    // Use the radio button message handler to processage messages to this
    // radio button.
    //
    psWidget->sBase.pfnMsgProc = RadioButtonMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
