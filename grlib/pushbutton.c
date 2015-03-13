//*****************************************************************************
//
// pushbutton.c - Various types of push buttons.
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
#include "grlib/pushbutton.h"

//*****************************************************************************
//
//! \addtogroup pushbutton_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Draws a rectangular push button.
//!
//! \param psWidget is a pointer to the push button widget to be drawn.
//!
//! This function draws a rectangular push button on the display.  This is
//! called in response to a \b #WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
RectangularButtonPaint(tWidget *psWidget)
{
    const uint8_t *pui8Image;
    tPushButtonWidget *pPush;
    tContext sCtx;
    int32_t i32X, i32Y;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a push button widget pointer.
    //
    pPush = (tPushButtonWidget *)psWidget;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sCtx, psWidget->psDisplay);

    //
    // Initialize the clipping region based on the extents of this rectangular
    // push button.
    //
    GrContextClipRegionSet(&sCtx, &(psWidget->sPosition));

    //
    // See if the push button fill style is selected.
    //
    if(pPush->ui32Style & PB_STYLE_FILL)
    {
        //
        // Fill the push button with the fill color.
        //
        GrContextForegroundSet(&sCtx, ((pPush->ui32Style & PB_STYLE_PRESSED) ?
                                       pPush->ui32PressFillColor :
                                       pPush->ui32FillColor));
        GrRectFill(&sCtx, &(psWidget->sPosition));
    }

    //
    // See if the push button outline style is selected.
    //
    if(pPush->ui32Style & PB_STYLE_OUTLINE)
    {
        //
        // Outline the push button with the outline color.
        //
        GrContextForegroundSet(&sCtx, pPush->ui32OutlineColor);
        GrRectDraw(&sCtx, &(psWidget->sPosition));
    }

    //
    // See if the push button text or image style is selected.
    //
    if(pPush->ui32Style & (PB_STYLE_TEXT | PB_STYLE_IMG))
    {
        //
        // Compute the center of the push button.
        //
        i32X = (psWidget->sPosition.i16XMin +
              ((psWidget->sPosition.i16XMax -
                psWidget->sPosition.i16XMin + 1) / 2));
        i32Y = (psWidget->sPosition.i16YMin +
              ((psWidget->sPosition.i16YMax -
                psWidget->sPosition.i16YMin + 1) / 2));

        //
        // If the push button outline style is selected then shrink the
        // clipping region by one pixel on each side so that the outline is not
        // overwritten by the text or image.
        //
        if(pPush->ui32Style & PB_STYLE_OUTLINE)
        {
            sCtx.sClipRegion.i16XMin++;
            sCtx.sClipRegion.i16YMin++;
            sCtx.sClipRegion.i16XMax--;
            sCtx.sClipRegion.i16YMax--;
        }

        //
        // See if the push button image style is selected.
        //
        if(pPush->ui32Style & PB_STYLE_IMG)
        {
            //
            // Set the foreground and background colors to use for 1 BPP
            // images.
            //
            GrContextForegroundSet(&sCtx, pPush->ui32TextColor);
            GrContextBackgroundSet(&sCtx,
                                   ((pPush->ui32Style & PB_STYLE_PRESSED) ?
                                    pPush->ui32PressFillColor :
                                    pPush->ui32FillColor));

            //
            // Get the image to be drawn.
            //
            pui8Image = (((pPush->ui32Style & PB_STYLE_PRESSED) &&
                         pPush->pui8PressImage) ?
                        pPush->pui8PressImage : pPush->pui8Image);

            //
            // Draw the image centered in the push button.
            //
            GrImageDraw(&sCtx, pui8Image,
                        i32X - (GrImageWidthGet(pui8Image) / 2),
                        i32Y - (GrImageHeightGet(pui8Image) / 2));
        }

        //
        // See if the push button text style is selected.
        //
        if(pPush->ui32Style & PB_STYLE_TEXT)
        {
            //
            // Draw the text centered in the middle of the push button.
            //
            GrContextFontSet(&sCtx, pPush->psFont);
            GrContextForegroundSet(&sCtx, pPush->ui32TextColor);
            GrContextBackgroundSet(&sCtx,
                                   ((pPush->ui32Style & PB_STYLE_PRESSED) ?
                                    pPush->ui32PressFillColor :
                                    pPush->ui32FillColor));
            GrStringDrawCentered(&sCtx, pPush->pcText, -1, i32X, i32Y,
                                 pPush->ui32Style & PB_STYLE_TEXT_OPAQUE);
        }
    }
}

//*****************************************************************************
//
//! Handles pointer events for a rectangular push button.
//!
//! \param psWidget is a pointer to the push button widget.
//! \param ui32Msg is the pointer event message.
//! \param i32X is the X coordinate of the pointer event.
//! \param i32Y is the Y coordinate of the pointer event.
//!
//! This function processes pointer event messages for a rectangular push
//! button.  This is called in response to a \b #WIDGET_MSG_PTR_DOWN,
//! \b #WIDGET_MSG_PTR_MOVE, and \b #WIDGET_MSG_PTR_UP messages.
//!
//! If the \b #WIDGET_MSG_PTR_UP message is received with a position within the
//! extents of the push button, the push button's OnClick callback function is
//! called.
//!
//! \return Returns 1 if the coordinates are within the extents of the push
//! button and 0 otherwise.
//
//*****************************************************************************
static int32_t
RectangularButtonClick(tWidget *psWidget, uint32_t ui32Msg, int32_t i32X,
                       int32_t i32Y)
{
    tPushButtonWidget *pPush;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a push button widget pointer.
    //
    pPush = (tPushButtonWidget *)psWidget;

    //
    // See if this is a pointer up message.
    //
    if(ui32Msg == WIDGET_MSG_PTR_UP)
    {
        //
        // Indicate that this push button is no longer pressed.
        //
        pPush->ui32Style &= ~(PB_STYLE_PRESSED);

        //
        // If filling is enabled for this push button, or if an image is being
        // used and a pressed button image is provided, then redraw the push
        // button to show it in its non-pressed state.
        //
        if((pPush->ui32Style & PB_STYLE_FILL) ||
           ((pPush->ui32Style & PB_STYLE_IMG) && pPush->pui8PressImage))
        {
            RectangularButtonPaint(psWidget);
        }

        //
        // If the pointer is still within the button bounds, and it is a
        // release notify button, call the notification function here.
        //
        if(GrRectContainsPoint(&psWidget->sPosition, i32X, i32Y) &&
           (pPush->ui32Style & PB_STYLE_RELEASE_NOTIFY) && pPush->pfnOnClick)
        {
            pPush->pfnOnClick(psWidget);
        }
    }

    //
    // See if the given coordinates are within the extents of the push button.
    //
    if(GrRectContainsPoint(&psWidget->sPosition, i32X, i32Y))
    {
        //
        // See if this is a pointer down message.
        //
        if(ui32Msg == WIDGET_MSG_PTR_DOWN)
        {
            //
            // Indicate that this push button is pressed.
            //
            pPush->ui32Style |= PB_STYLE_PRESSED;

            //
            // If filling is enabled for this push button, or if an image is
            // being used and a pressed button image is provided, then redraw
            // the push button to show it in its pressed state.
            //
            if((pPush->ui32Style & PB_STYLE_FILL) ||
               ((pPush->ui32Style & PB_STYLE_IMG) && pPush->pui8PressImage))
            {
                RectangularButtonPaint(psWidget);
            }
        }

        //
        // See if there is an OnClick callback for this widget.
        //
        if(pPush->pfnOnClick)
        {
            //
            // If the pointer was just pressed then call the callback.
            //
            if((ui32Msg == WIDGET_MSG_PTR_DOWN) &&
               !(pPush->ui32Style & PB_STYLE_RELEASE_NOTIFY))
            {
                pPush->pfnOnClick(psWidget);
            }

            //
            // See if auto-repeat is enabled for this widget.
            //
            if(pPush->ui32Style & PB_STYLE_AUTO_REPEAT)
            {
                //
                // If the pointer was just pressed, reset the auto-repeat
                // count.
                //
                if(ui32Msg == WIDGET_MSG_PTR_DOWN)
                {
                    pPush->ui32AutoRepeatCount = 0;
                }

                //
                // See if the pointer was moved.
                //
                else if(ui32Msg == WIDGET_MSG_PTR_MOVE)
                {
                    //
                    // Increment the auto-repeat count.
                    //
                    pPush->ui32AutoRepeatCount++;

                    //
                    // If the auto-repeat count exceeds the auto-repeat delay,
                    // and it is a multiple of the auto-repeat rate, then
                    // call the callback.
                    //
                    if((pPush->ui32AutoRepeatCount >=
                        pPush->ui16AutoRepeatDelay) &&
                       (((pPush->ui32AutoRepeatCount -
                          pPush->ui16AutoRepeatDelay) %
                         pPush->ui16AutoRepeatRate) == 0))
                    {
                        pPush->pfnOnClick(psWidget);
                    }
                }
            }
        }

        //
        // These coordinates are within the extents of the push button widget.
        //
        return(1);
    }

    //
    // These coordinates are not within the extents of the push button widget.
    //
    return(0);
}

//*****************************************************************************
//
//! Handles messages for a rectangular push button widget.
//!
//! \param psWidget is a pointer to the push button widget.
//! \param ui32Msg is the message.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function receives messages intended for this push button widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
int32_t
RectangularButtonMsgProc(tWidget *psWidget, uint32_t ui32Msg,
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
            RectangularButtonPaint(psWidget);

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
            return(RectangularButtonClick(psWidget, ui32Msg, ui32Param1,
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
//! Initializes a rectangular push button widget.
//!
//! \param psWidget is a pointer to the push button widget to initialize.
//! \param psDisplay is a pointer to the display on which to draw the push
//! button.
//! \param i32X is the X coordinate of the upper left corner of the push
//! button.
//! \param i32Y is the Y coordinate of the upper left corner of the push
//! button.
//! \param i32Width is the width of the push button.
//! \param i32Height is the height of the push button.
//!
//! This function initializes the provided push button widget so that it will
//! be a rectangular push button.
//!
//! \return None.
//
//*****************************************************************************
void
RectangularButtonInit(tPushButtonWidget *psWidget, const tDisplay *psDisplay,
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
    for(ui32Idx = 0; ui32Idx < sizeof(tPushButtonWidget); ui32Idx += 4)
    {
        ((uint32_t *)psWidget)[ui32Idx / 4] = 0;
    }

    //
    // Set the size of the push button widget structure.
    //
    psWidget->sBase.i32Size = sizeof(tPushButtonWidget);

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
    // Set the extents of this rectangular push button.
    //
    psWidget->sBase.sPosition.i16XMin = i32X;
    psWidget->sBase.sPosition.i16YMin = i32Y;
    psWidget->sBase.sPosition.i16XMax = i32X + i32Width - 1;
    psWidget->sBase.sPosition.i16YMax = i32Y + i32Height - 1;

    //
    // Use the rectangular push button message handler to process messages to
    // this push button.
    //
    psWidget->sBase.pfnMsgProc = RectangularButtonMsgProc;
}

//*****************************************************************************
//
//! Draws a circular push button.
//!
//! \param psWidget is a pointer to the push button widget to be drawn.
//!
//! This function draws a circular push button on the display.  This is called
//! in response to a \b #WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
CircularButtonPaint(tWidget *psWidget)
{
    const uint8_t *pui8Image;
    tPushButtonWidget *pPush;
    tContext sCtx;
    int32_t i32X, i32Y, i32R;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a push button widget pointer.
    //
    pPush = (tPushButtonWidget *)psWidget;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sCtx, psWidget->psDisplay);

    //
    // Initialize the clipping region based on the extents of this circular
    // push button.
    //
    GrContextClipRegionSet(&sCtx, &(psWidget->sPosition));

    //
    // Get the radius of the circular push button, along with the X and Y
    // coordinates for its center.
    //
    i32R = (psWidget->sPosition.i16XMax - psWidget->sPosition.i16XMin + 1) / 2;
    i32X = psWidget->sPosition.i16XMin + i32R;
    i32Y = psWidget->sPosition.i16YMin + i32R;

    //
    // See if the push button fill style is selected.
    //
    if(pPush->ui32Style & PB_STYLE_FILL)
    {
        //
        // Fill the push button with the fill color.
        //
        GrContextForegroundSet(&sCtx, ((pPush->ui32Style & PB_STYLE_PRESSED) ?
                                       pPush->ui32PressFillColor :
                                       pPush->ui32FillColor));
        GrCircleFill(&sCtx, i32X, i32Y, i32R);
    }

    //
    // See if the push button outline style is selected.
    //
    if(pPush->ui32Style & PB_STYLE_OUTLINE)
    {
        //
        // Outline the push button with the outline color.
        //
        GrContextForegroundSet(&sCtx, pPush->ui32OutlineColor);
        GrCircleDraw(&sCtx, i32X, i32Y, i32R);
    }

    //
    // See if the push button text or image style is selected.
    //
    if(pPush->ui32Style & (PB_STYLE_TEXT | PB_STYLE_IMG))
    {
        //
        // If the push button outline style is selected then shrink the
        // clipping region by one pixel on each side so that the outline is not
        // overwritten by the text or image.
        //
        if(pPush->ui32Style & PB_STYLE_OUTLINE)
        {
            sCtx.sClipRegion.i16XMin++;
            sCtx.sClipRegion.i16YMin++;
            sCtx.sClipRegion.i16XMax--;
            sCtx.sClipRegion.i16YMax--;
        }

        //
        // See if the push button image style is selected.
        //
        if(pPush->ui32Style & PB_STYLE_IMG)
        {
            //
            // Set the foreground and background colors to use for 1 BPP
            // images.
            //
            GrContextForegroundSet(&sCtx, pPush->ui32TextColor);
            GrContextBackgroundSet(&sCtx,
                                   ((pPush->ui32Style & PB_STYLE_PRESSED) ?
                                    pPush->ui32PressFillColor :
                                    pPush->ui32FillColor));

            //
            // Get the image to be drawn.
            //
            pui8Image = (((pPush->ui32Style & PB_STYLE_PRESSED) &&
                         pPush->pui8PressImage) ?
                        pPush->pui8PressImage : pPush->pui8Image);

            //
            // Draw the image centered in the push button.
            //
            GrImageDraw(&sCtx, pui8Image, i32X - (GrImageWidthGet(pui8Image) / 2),
                        i32Y - (GrImageHeightGet(pui8Image) / 2));
        }

        //
        // See if the push button text style is selected.
        //
        if(pPush->ui32Style & PB_STYLE_TEXT)
        {
            //
            // Draw the text centered in the middle of the push button.
            //
            GrContextFontSet(&sCtx, pPush->psFont);
            GrContextForegroundSet(&sCtx, pPush->ui32TextColor);
            GrContextBackgroundSet(&sCtx,
                                   ((pPush->ui32Style & PB_STYLE_PRESSED) ?
                                    pPush->ui32PressFillColor :
                                    pPush->ui32FillColor));
            GrStringDrawCentered(&sCtx, pPush->pcText, -1, i32X, i32Y,
                                 pPush->ui32Style & PB_STYLE_TEXT_OPAQUE);
        }
    }
}

//*****************************************************************************
//
//! Handles pointer events for a circular push button.
//!
//! \param psWidget is a pointer to the push button widget.
//! \param ui32Msg is the pointer event message.
//! \param i32X is the X coordinate of the pointer event.
//! \param i32Y is the Y coordinate of the pointer event.
//!
//! This function processes pointer event messages for a circular push button.
//! This is called in response to a \b #WIDGET_MSG_PTR_DOWN,
//! \b #WIDGET_MSG_PTR_MOVE, and \b #WIDGET_MSG_PTR_UP messages.
//!
//! If the \b #WIDGET_MSG_PTR_UP message is received with a position within the
//! extents of the push button, the push button's OnClick callback function is
//! called.
//!
//! \return Returns 1 if the coordinates are within the extents of the push
//! button and 0 otherwise.
//
//*****************************************************************************
static int32_t
CircularButtonClick(tWidget *psWidget, uint32_t ui32Msg, int32_t i32X,
                    int32_t i32Y)
{
    tPushButtonWidget *pPush;
    int32_t i32Xc, i32Yc, i32R;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a push button widget pointer.
    //
    pPush = (tPushButtonWidget *)psWidget;

    //
    // See if this is a pointer up message.
    //
    if(ui32Msg == WIDGET_MSG_PTR_UP)
    {
        //
        // Indicate that this push button is no longer pressed.
        //
        pPush->ui32Style &= ~(PB_STYLE_PRESSED);

        //
        // If filling is enabled for this push button, or if an image is being
        // used and a pressed button image is provided, then redraw the push
        // button to show it in its non-pressed state.
        //
        if((pPush->ui32Style & PB_STYLE_FILL) ||
           ((pPush->ui32Style & PB_STYLE_IMG) && pPush->pui8PressImage))
        {
            CircularButtonPaint(psWidget);
        }
    }

    //
    // Get the radius of the circular push button, along with the X and Y
    // coordinates for its center.
    //
    i32R = (psWidget->sPosition.i16XMax - psWidget->sPosition.i16XMin + 1) / 2;
    i32Xc = psWidget->sPosition.i16XMin + i32R;
    i32Yc = psWidget->sPosition.i16YMin + i32R;

    //
    // See if the given coordinates are within the radius of the push button.
    //
    if((((i32X - i32Xc) * (i32X - i32Xc)) +
        ((i32Y - i32Yc) * (i32Y - i32Yc))) <= (i32R * i32R))
    {
        //
        // See if this is a pointer down message.
        //
        if(ui32Msg == WIDGET_MSG_PTR_DOWN)
        {
            //
            // Indicate that this push button is pressed.
            //
            pPush->ui32Style |= PB_STYLE_PRESSED;

            //
            // If filling is enabled for this push button, or if an image is
            // being used and a pressed button image is provided, then redraw
            // the push button to show it in its pressed state.
            //
            if((pPush->ui32Style & PB_STYLE_FILL) ||
               ((pPush->ui32Style & PB_STYLE_IMG) && pPush->pui8PressImage))
            {
                CircularButtonPaint(psWidget);
            }
        }

        //
        // See if there is an OnClick callback for this widget.
        //
        if(pPush->pfnOnClick)
        {
            //
            // If the pointer was just pressed or if the pointer was released
            // and this button is set for release notification then call the
            // callback.
            //
            if(((ui32Msg == WIDGET_MSG_PTR_DOWN) &&
               !(pPush->ui32Style & PB_STYLE_RELEASE_NOTIFY)) ||
               ((ui32Msg == WIDGET_MSG_PTR_UP) &&
                (pPush->ui32Style & PB_STYLE_RELEASE_NOTIFY)))
            {
                pPush->pfnOnClick(psWidget);
            }

            //
            // See if auto-repeat is enabled for this widget.
            //
            if(pPush->ui32Style & PB_STYLE_AUTO_REPEAT)
            {
                //
                // If the pointer was just pressed, reset the auto-repeat
                // count.
                //
                if(ui32Msg == WIDGET_MSG_PTR_DOWN)
                {
                    pPush->ui32AutoRepeatCount = 0;
                }

                //
                // See if the pointer was moved.
                //
                else if(ui32Msg == WIDGET_MSG_PTR_MOVE)
                {
                    //
                    // Increment the auto-repeat count.
                    //
                    pPush->ui32AutoRepeatCount++;

                    //
                    // If the auto-repeat count exceeds the auto-repeat delay,
                    // and it is a multiple of the auto-repeat rate, then
                    // call the callback.
                    //
                    if((pPush->ui32AutoRepeatCount >=
                        pPush->ui16AutoRepeatDelay) &&
                       (((pPush->ui32AutoRepeatCount -
                          pPush->ui16AutoRepeatDelay) %
                         pPush->ui16AutoRepeatRate) == 0))
                    {
                        pPush->pfnOnClick(psWidget);
                    }
                }
            }
        }

        //
        // These coordinates are within the extents of the push button widget.
        //
        return(1);
    }

    //
    // These coordinates are not within the extents of the push button widget.
    //
    return(0);
}

//*****************************************************************************
//
//! Handles messages for a circular push button widget.
//!
//! \param psWidget is a pointer to the push button widget.
//! \param ui32Msg is the message.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function receives messages intended for this push button widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
int32_t
CircularButtonMsgProc(tWidget *psWidget, uint32_t ui32Msg,
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
            CircularButtonPaint(psWidget);

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
            return(CircularButtonClick(psWidget, ui32Msg, ui32Param1,
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
//! Initializes a circular push button widget.
//!
//! \param psWidget is a pointer to the push button widget to initialize.
//! \param psDisplay is a pointer to the display on which to draw the push
//! button.
//! \param i32X is the X coordinate of the upper left corner of the push
//! button.
//! \param i32Y is the Y coordinate of the upper left corner of the push
//! button.
//! \param i32R is the radius of the push button.
//!
//! This function initializes the provided push button widget so that it will
//! be a circular push button.
//!
//! \return None.
//
//*****************************************************************************
void
CircularButtonInit(tPushButtonWidget *psWidget, const tDisplay *psDisplay,
                   int32_t i32X, int32_t i32Y, int32_t i32R)
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
    for(ui32Idx = 0; ui32Idx < sizeof(tPushButtonWidget); ui32Idx += 4)
    {
        ((uint32_t *)psWidget)[ui32Idx / 4] = 0;
    }

    //
    // Set the size of the push button widget structure.
    //
    psWidget->sBase.i32Size = sizeof(tPushButtonWidget);

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
    // Set the extents of this circular push button.
    //
    psWidget->sBase.sPosition.i16XMin = i32X - i32R;
    psWidget->sBase.sPosition.i16YMin = i32Y - i32R;
    psWidget->sBase.sPosition.i16XMax = i32X + i32R;
    psWidget->sBase.sPosition.i16YMax = i32Y + i32R;

    //
    // Use the circular push button message handler to processes messages to
    // this push button.
    //
    psWidget->sBase.pfnMsgProc = CircularButtonMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
