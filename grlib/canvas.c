//*****************************************************************************
//
// canvas.c - A drawing canvas widget.
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
#include "grlib/canvas.h"

//*****************************************************************************
//
//! \addtogroup canvas_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Draws the contents of a canvas.
//!
//! \param psWidget is a pointer to the canvas widget to be drawn.
//!
//! This function draws the contents of a canvas on the display.  This is
//! called in response to a \b #WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
CanvasPaint(tWidget *psWidget)
{
    tCanvasWidget *psCanvas;
    tContext sCtx;
    int32_t i32X, i32Y, i32Size;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a canvas widget pointer.
    //
    psCanvas = (tCanvasWidget *)psWidget;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sCtx, psWidget->psDisplay);

    //
    // Initialize the clipping region based on the extents of this canvas.
    //
    GrContextClipRegionSet(&sCtx, &(psWidget->sPosition));

    //
    // See if the canvas fill style is selected.
    //
    if(psCanvas->ui32Style & CANVAS_STYLE_FILL)
    {
        //
        // Fill the canvas with the fill color.
        //
        GrContextForegroundSet(&sCtx, psCanvas->ui32FillColor);
        GrRectFill(&sCtx, &(psWidget->sPosition));
    }

    //
    // See if the canvas outline style is selected.
    //
    if(psCanvas->ui32Style & CANVAS_STYLE_OUTLINE)
    {
        //
        // Outline the canvas with the outline color.
        //
        GrContextForegroundSet(&sCtx, psCanvas->ui32OutlineColor);
        GrRectDraw(&sCtx, &(psWidget->sPosition));
    }

    //
    // See if the canvas text or image style is selected.
    //
    if(psCanvas->ui32Style & (CANVAS_STYLE_TEXT | CANVAS_STYLE_IMG))
    {
        //
        // Compute the center of the canvas.
        //
        i32X = (psWidget->sPosition.i16XMin +
              ((psWidget->sPosition.i16XMax -
                psWidget->sPosition.i16XMin + 1) / 2));
        i32Y = (psWidget->sPosition.i16YMin +
              ((psWidget->sPosition.i16YMax -
                psWidget->sPosition.i16YMin + 1) / 2));

        //
        // If the canvas outline style is selected then shrink the clipping
        // region by one pixel on each side so that the outline is not
        // overwritten by the text or image.
        //
        if(psCanvas->ui32Style & CANVAS_STYLE_OUTLINE)
        {
            sCtx.sClipRegion.i16XMin++;
            sCtx.sClipRegion.i16YMin++;
            sCtx.sClipRegion.i16XMax--;
            sCtx.sClipRegion.i16YMax--;
        }

        //
        // See if the canvas image style is selected.
        //
        if(psCanvas->ui32Style & CANVAS_STYLE_IMG)
        {
            //
            // Set the foreground and background colors to use for 1 BPP
            // images.
            //
            GrContextForegroundSet(&sCtx, psCanvas->ui32TextColor);
            GrContextBackgroundSet(&sCtx, psCanvas->ui32FillColor);

            //
            // Draw the image centered in the canvas.
            //
            GrImageDraw(&sCtx, psCanvas->pui8Image,
                        i32X - (GrImageWidthGet(psCanvas->pui8Image) / 2),
                        i32Y - (GrImageHeightGet(psCanvas->pui8Image) / 2));
        }

        //
        // See if the canvas text style is selected.
        //
        if(psCanvas->ui32Style & CANVAS_STYLE_TEXT)
        {
            //
            // Set the relevant font and colors.
            //
            GrContextFontSet(&sCtx, psCanvas->psFont);
            GrContextForegroundSet(&sCtx, psCanvas->ui32TextColor);
            GrContextBackgroundSet(&sCtx, psCanvas->ui32FillColor);

            //
            // Determine the drawing position for the string based on the
            // text alignment style.  First consider the horizontal case.  We
            // enter this section with i32X set to the center of the widget.
            //

            //
            // How wide is the string?
            //
            i32Size = GrStringWidthGet(&sCtx, psCanvas->pcText, -1);

            if(psCanvas->ui32Style & CANVAS_STYLE_TEXT_LEFT)
            {
                //
                // The string is to be aligned with the left edge of
                // the widget.  Use the clipping rectangle as reference
                // since this will ensure that the string doesn't
                // encroach on any border that is set.
                //
                i32X = sCtx.sClipRegion.i16XMin;
            }
            else
            {
                if(psCanvas->ui32Style & CANVAS_STYLE_TEXT_RIGHT)
                {
                    //
                    // The string is to be aligned with the right edge of
                    // the widget.  Use the clipping rectangle as reference
                    // since this will ensure that the string doesn't
                    // encroach on any border that is set.
                    //
                    i32X = sCtx.sClipRegion.i16XMax - i32Size;
                }
                else
                {
                    //
                    // We are centering the string horizontally so adjust
                    // the position accordingly to take into account the
                    // width of the string.
                    //
                    i32X -= (i32Size / 2);
                }
            }

            //
            // Now consider the horizontal case.  We enter this section with
            // i32Y set to the center of the widget.
            //
            // How tall is the string?
            //
            i32Size = GrStringHeightGet(&sCtx);

            if(psCanvas->ui32Style & CANVAS_STYLE_TEXT_TOP)
            {
                //
                // The string is to be aligned with the top edge of
                // the widget.  Use the clipping rectangle as reference
                // since this will ensure that the string doesn't
                // encroach on any border that is set.
                //
                i32Y = sCtx.sClipRegion.i16YMin;
            }
            else
            {
                if(psCanvas->ui32Style & CANVAS_STYLE_TEXT_BOTTOM)
                {
                    //
                    // The string is to be aligned with the bottom edge of
                    // the widget.  Use the clipping rectangle as reference
                    // since this will ensure that the string doesn't
                    // encroach on any border that is set.
                    //
                    i32Y = sCtx.sClipRegion.i16YMax - i32Size;
                }
                else
                {
                    //
                    // We are centering the string vertically so adjust
                    // the position accordingly to take into account the
                    // height of the string.
                    //
                    i32Y -= (i32Size / 2);
                }
            }

            //
            // Now draw the string.
            //
            GrStringDraw(&sCtx, psCanvas->pcText, -1, i32X, i32Y,
                         psCanvas->ui32Style & CANVAS_STYLE_TEXT_OPAQUE);
        }
    }

    //
    // See if the application-drawn style is selected.
    //
    if(psCanvas->ui32Style & CANVAS_STYLE_APP_DRAWN)
    {
        //
        // Call the application-supplied function to draw the canvas.
        //
        psCanvas->pfnOnPaint(psWidget, &sCtx);
    }
}

//*****************************************************************************
//
//! Handles messages for a canvas widget.
//!
//! \param psWidget is a pointer to the canvas widget.
//! \param ui32Msg is the message.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function receives messages intended for this canvas widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
int32_t
CanvasMsgProc(tWidget *psWidget, uint32_t ui32Msg, uint32_t ui32Param1,
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
            CanvasPaint(psWidget);

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
//! Initializes a canvas widget.
//!
//! \param psWidget is a pointer to the canvas widget to initialize.
//! \param psDisplay is a pointer to the display on which to draw the canvas.
//! \param i32X is the X coordinate of the upper left corner of the canvas.
//! \param i32Y is the Y coordinate of the upper left corner of the canvas.
//! \param i32Width is the width of the canvas.
//! \param i32Height is the height of the canvas.
//!
//! This function initializes the provided canvas widget.
//!
//! \return None.
//
//*****************************************************************************
void
CanvasInit(tCanvasWidget *psWidget, const tDisplay *psDisplay, int32_t i32X,
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
    for(ui32Idx = 0; ui32Idx < sizeof(tCanvasWidget); ui32Idx += 4)
    {
        ((uint32_t *)psWidget)[ui32Idx / 4] = 0;
    }

    //
    // Set the size of the canvas widget structure.
    //
    psWidget->sBase.i32Size = sizeof(tCanvasWidget);

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
    // Set the extents of this canvas.
    //
    psWidget->sBase.sPosition.i16XMin = i32X;
    psWidget->sBase.sPosition.i16YMin = i32Y;
    psWidget->sBase.sPosition.i16XMax = i32X + i32Width - 1;
    psWidget->sBase.sPosition.i16YMax = i32Y + i32Height - 1;

    //
    // Use the canvas message handler to process messages to this canvas.
    //
    psWidget->sBase.pfnMsgProc = CanvasMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
