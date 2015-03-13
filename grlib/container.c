//*****************************************************************************
//
// container.c - Generic container widget.
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
#include "grlib/container.h"

//*****************************************************************************
//
//! \addtogroup container_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Draws a container widget.
//!
//! \param psWidget is a pointer to the container widget to be drawn.
//!
//! This function draws a container widget on the display.  This is called in
//! response to a \b #WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
ContainerPaint(tWidget *psWidget)
{
    tContainerWidget *pContainer;
    int32_t i32X1, i32X2, i32Y;
    tContext sCtx;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a container widget pointer.
    //
    pContainer = (tContainerWidget *)psWidget;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sCtx, psWidget->psDisplay);

    //
    // Initialize the clipping region based on the extents of this container.
    //
    GrContextClipRegionSet(&sCtx, &(psWidget->sPosition));

    //
    // See if the container fill style is selected.
    //
    if(pContainer->ui32Style & CTR_STYLE_FILL)
    {
        //
        // Fill the container with the fill color.
        //
        GrContextForegroundSet(&sCtx, pContainer->ui32FillColor);
        GrRectFill(&sCtx, &(psWidget->sPosition));
    }

    //
    // See if the container text style is selected.
    //
    if(pContainer->ui32Style & CTR_STYLE_TEXT)
    {
        //
        // Set the font and colors used to draw the container text.
        //
        GrContextFontSet(&sCtx, pContainer->psFont);
        GrContextForegroundSet(&sCtx, pContainer->ui32TextColor);
        GrContextBackgroundSet(&sCtx, pContainer->ui32FillColor);

        //
        // Get the width of the container text.
        //
        i32X2 = GrStringWidthGet(&sCtx, pContainer->pcText, -1);

        //
        // Determine the position of the text.  The position depends on the
        // the width of the string and if centering is enabled.
        //
        if(pContainer->ui32Style & CTR_STYLE_TEXT_CENTER)
        {
            i32X1 = (psWidget->sPosition.i16XMin +
                   ((psWidget->sPosition.i16XMax -
                     psWidget->sPosition.i16XMin + 1 - i32X2 - 8) / 2));
        }
        else
        {
            i32X1 = psWidget->sPosition.i16XMin + 4;
        }

        //
        // Draw the container text.
        //
        GrStringDraw(&sCtx, pContainer->pcText, -1, i32X1 + 4,
                     psWidget->sPosition.i16YMin,
                     pContainer->ui32Style & CTR_STYLE_TEXT_OPAQUE);

        //
        // See if the container outline style is selected.
        //
        if(pContainer->ui32Style & CTR_STYLE_OUTLINE)
        {
            //
            // Get the position of the right side of the string.
            //
            i32X2 = i32X1 + i32X2 + 8;

            //
            // Get the position of the vertical center of the text.
            //
            i32Y = (psWidget->sPosition.i16YMin +
                  (GrFontBaselineGet(pContainer->psFont) / 2));

            //
            // Set the color to draw the outline.
            //
            GrContextForegroundSet(&sCtx, pContainer->ui32OutlineColor);

            //
            // Draw the outline around the container widget, leaving a gap
            // where the text reside across the top of the widget.
            //
            GrLineDraw(&sCtx, i32X1, i32Y, psWidget->sPosition.i16XMin, i32Y);
            GrLineDraw(&sCtx, psWidget->sPosition.i16XMin, i32Y,
                       psWidget->sPosition.i16XMin,
                       psWidget->sPosition.i16YMax);
            GrLineDraw(&sCtx, psWidget->sPosition.i16XMin,
                       psWidget->sPosition.i16YMax,
                       psWidget->sPosition.i16XMax,
                       psWidget->sPosition.i16YMax);
            GrLineDraw(&sCtx, psWidget->sPosition.i16XMax,
                       psWidget->sPosition.i16YMax,
                       psWidget->sPosition.i16XMax, i32Y);
            GrLineDraw(&sCtx, psWidget->sPosition.i16XMax, i32Y, i32X2, i32Y);
        }
    }

    //
    // Otherwise, see if the container outline style is selected.
    //
    else if(pContainer->ui32Style & CTR_STYLE_OUTLINE)
    {
        //
        // Outline the container with the outline color.
        //
        GrContextForegroundSet(&sCtx, pContainer->ui32OutlineColor);
        GrRectDraw(&sCtx, &(psWidget->sPosition));
    }
}

//*****************************************************************************
//
//! Handles messages for a container widget.
//!
//! \param psWidget is a pointer to the container widget.
//! \param ui32Msg is the message.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function receives messages intended for this container widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
int32_t
ContainerMsgProc(tWidget *psWidget, uint32_t ui32Msg, uint32_t ui32Param1,
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
            ContainerPaint(psWidget);

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
//! Initializes a container widget.
//!
//! \param psWidget is a pointer to the container widget to initialize.
//! \param psDisplay is a pointer to the display on which to draw the container
//! widget.
//! \param i32X is the X coordinate of the upper left corner of the container
//! widget.
//! \param i32Y is the Y coordinate of the upper left corner of the container
//! widget.
//! \param i32Width is the width of the container widget.
//! \param i32Height is the height of the container widget.
//!
//! This function initializes a container widget, preparing it for placement
//! into the widget tree.
//!
//! \return none.
//
//*****************************************************************************
void
ContainerInit(tContainerWidget *psWidget, const tDisplay *psDisplay,
              int32_t i32X, int32_t i32Y, int32_t i32Width, int32_t i32Height)
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
    for(ui32Idx = 0; ui32Idx < sizeof(tContainerWidget); ui32Idx += 4)
    {
        ((uint32_t *)psWidget)[ui32Idx / 4] = 0;
    }

    //
    // Set the size of the container widget structure.
    //
    psWidget->sBase.i32Size = sizeof(tContainerWidget);

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
    // Set the extents of this container widget.
    //
    psWidget->sBase.sPosition.i16XMin = i32X;
    psWidget->sBase.sPosition.i16YMin = i32Y;
    psWidget->sBase.sPosition.i16XMax = i32X + i32Width - 1;
    psWidget->sBase.sPosition.i16YMax = i32Y + i32Height - 1;

    //
    // Use the container widget message handler to process messages to this
    // container widget.
    //
    psWidget->sBase.pfnMsgProc = ContainerMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
