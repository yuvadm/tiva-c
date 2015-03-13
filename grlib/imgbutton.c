//*****************************************************************************
//
// imgbutton.c - An image-based button widget.
//
// Copyright (c) 2009-2014 Texas Instruments Incorporated.  All rights reserved.
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
#include "grlib/imgbutton.h"

//*****************************************************************************
//
//! \addtogroup imgbutton_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Draws an image button.
//!
//! \param psWidget is a pointer to the image button widget to be drawn.
//!
//! This function draws a rectangular image button on the display.  This is
//! called in response to a \b #WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
ImageButtonPaint(tWidget *psWidget)
{
    const uint8_t *pui8Image;
    tImageButtonWidget *psPush;
    tContext sCtx;
    int32_t i32X, i32Y;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a image button widget pointer.
    //
    psPush = (tImageButtonWidget *)psWidget;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sCtx, psWidget->psDisplay);

    //
    // Initialize the clipping region based on the extents of this rectangular
    // image button.
    //
    GrContextClipRegionSet(&sCtx, &(psWidget->sPosition));

    //
    // Compute the center of the image button.
    //
    i32X = (psWidget->sPosition.i16XMin +
          ((psWidget->sPosition.i16XMax -
            psWidget->sPosition.i16XMin + 1) / 2));
    i32Y = (psWidget->sPosition.i16YMin +
          ((psWidget->sPosition.i16YMax -
            psWidget->sPosition.i16YMin + 1) / 2));

    //
    // Do we need to fill the widget background with a color?
    //
    if(psPush->ui32Style & IB_STYLE_FILL)
    {
        //
        // Yes. Set the appropriate color depending upon whether or not
        // the widget is currently pressed.
        //
        GrContextForegroundSet(&sCtx,
                               ((psPush->ui32Style & IB_STYLE_PRESSED) ?
                                psPush->ui32PressedColor :
                                psPush->ui32BackgroundColor));
        GrRectFill(&sCtx, &(psWidget->sPosition));
    }

    //
    // Set the foreground and background colors to use for 1 BPP
    // images and text
    //
    GrContextForegroundSet(&sCtx, psPush->ui32ForegroundColor);
    GrContextBackgroundSet(&sCtx,
                           ((psPush->ui32Style & IB_STYLE_PRESSED) ?
                            psPush->ui32PressedColor :
                            psPush->ui32BackgroundColor));

    //
    // Do we need to draw the background image?
    //
    if(!(psPush->ui32Style & IB_STYLE_IMAGE_OFF))
    {
        //
        // Get the background image to be drawn.
        //
        pui8Image = ((psPush->ui32Style & IB_STYLE_PRESSED) ?
                    psPush->pui8PressImage : psPush->pui8Image);

        //
        // Draw the image centered in the image button.
        //
        GrImageDraw(&sCtx, pui8Image, i32X - (GrImageWidthGet(pui8Image) / 2),
                    i32Y - (GrImageHeightGet(pui8Image) / 2));
    }

    //
    // Adjust the drawing position if the button is pressed.
    //
    i32X += ((psPush->ui32Style & IB_STYLE_PRESSED) ? psPush->i16XOffset : 0);
    i32Y += ((psPush->ui32Style & IB_STYLE_PRESSED) ? psPush->i16YOffset : 0);

    //
    // If there is a keycap image and it is not disabled, center this on the
    // top of the button, applying any offset defined if the button is
    // currently pressed.
    //
    if(psPush->pui8KeycapImage && !(psPush->ui32Style & IB_STYLE_KEYCAP_OFF))
    {
        //
        // Draw the keycap image.
        //
        GrImageDraw(&sCtx, psPush->pui8KeycapImage,
                    i32X - (GrImageWidthGet(psPush->pui8KeycapImage) / 2),
                    i32Y - (GrImageHeightGet(psPush->pui8KeycapImage) / 2));
    }

    //
    // See if the button text style is selected.
    //
    if(psPush->ui32Style & IB_STYLE_TEXT)
    {
        //
        // Draw the text centered in the middle of the button with offset
        // applied if the button is currently pressed.
        //
        GrContextFontSet(&sCtx, psPush->psFont);
        GrStringDrawCentered(&sCtx, psPush->pcText, -1, i32X, i32Y, 0);
    }
}

//*****************************************************************************
//
//! Handles pointer events for a rectangular image button.
//!
//! \param psWidget is a pointer to the image button widget.
//! \param ui32Msg is the pointer event message.
//! \param i32X is the X coordinate of the pointer event.
//! \param i32Y is the Y coordinate of the pointer event.
//!
//! This function processes pointer event messages for a rectangular push
//! button.  This is called in response to a \b #WIDGET_MSG_PTR_DOWN,
//! \b #WIDGET_MSG_PTR_MOVE, and \b #WIDGET_MSG_PTR_UP messages.
//!
//! If the \b #WIDGET_MSG_PTR_UP message is received with a position within the
//! extents of the image button, the image button's OnClick callback function is
//! called.
//!
//! \return Returns 1 if the coordinates are within the extents of the push
//! button and 0 otherwise.
//
//*****************************************************************************
static int32_t
ImageButtonClick(tWidget *psWidget, uint32_t ui32Msg, int32_t i32X,
                 int32_t i32Y)
{
    tImageButtonWidget *psPush;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a image button widget pointer.
    //
    psPush = (tImageButtonWidget *)psWidget;

    //
    // See if this is a pointer up message.
    //
    if(ui32Msg == WIDGET_MSG_PTR_UP)
    {
        //
        // Indicate that this image button is no longer pressed.
        //
        psPush->ui32Style &= ~(IB_STYLE_PRESSED);

        //
        // Redraw the button in the released state.
        //
        ImageButtonPaint(psWidget);

        //
        // If the pointer is still within the button bounds, and it is a
        // release notify button, call the notification function here.
        //
        if(GrRectContainsPoint(&psWidget->sPosition, i32X, i32Y) &&
           (psPush->ui32Style & IB_STYLE_RELEASE_NOTIFY) && psPush->pfnOnClick)
        {
            psPush->pfnOnClick(psWidget);
        }
    }

    //
    // See if the given coordinates are within the extents of the image button.
    //
    if(GrRectContainsPoint(&psWidget->sPosition, i32X, i32Y))
    {
        //
        // See if this is a pointer down message.
        //
        if(ui32Msg == WIDGET_MSG_PTR_DOWN)
        {
            //
            // Indicate that this image button is pressed.
            //
            psPush->ui32Style |= IB_STYLE_PRESSED;

            //
            // Draw the button in the pressed state.
            //
            ImageButtonPaint(psWidget);
        }

        //
        // See if there is an OnClick callback for this widget.
        //
        if(psPush->pfnOnClick)
        {
            //
            // If the pointer was just pressed then call the callback.
            //
            if((ui32Msg == WIDGET_MSG_PTR_DOWN) &&
               !(psPush->ui32Style & IB_STYLE_RELEASE_NOTIFY))
            {
                psPush->pfnOnClick(psWidget);
            }

            //
            // See if auto-repeat is enabled for this widget.
            //
            if(psPush->ui32Style & IB_STYLE_AUTO_REPEAT)
            {
                //
                // If the pointer was just pressed, reset the auto-repeat
                // count.
                //
                if(ui32Msg == WIDGET_MSG_PTR_DOWN)
                {
                    psPush->ui32AutoRepeatCount = 0;
                }

                //
                // See if the pointer was moved.
                //
                else if(ui32Msg == WIDGET_MSG_PTR_MOVE)
                {
                    //
                    // Increment the auto-repeat count.
                    //
                    psPush->ui32AutoRepeatCount++;

                    //
                    // If the auto-repeat count exceeds the auto-repeat delay,
                    // and it is a multiple of the auto-repeat rate, then
                    // call the callback.
                    //
                    if((psPush->ui32AutoRepeatCount >=
                        psPush->ui16AutoRepeatDelay) &&
                       (((psPush->ui32AutoRepeatCount -
                          psPush->ui16AutoRepeatDelay) %
                         psPush->ui16AutoRepeatRate) == 0))
                    {
                        psPush->pfnOnClick(psWidget);
                    }
                }
            }
        }

        //
        // These coordinates are within the extents of the image button widget.
        //
        return(1);
    }

    //
    // These coordinates are not within the extents of the image button widget.
    //
    return(0);
}

//*****************************************************************************
//
//! Handles messages for an image button widget.
//!
//! \param psWidget is a pointer to the image button widget.
//! \param ui32Msg is the message.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function receives messages intended for this image button widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
int32_t
ImageButtonMsgProc(tWidget *psWidget, uint32_t ui32Msg,
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
            ImageButtonPaint(psWidget);

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
            return(ImageButtonClick(psWidget, ui32Msg, ui32Param1,
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
//! Initializes an image button widget.
//!
//! \param psWidget is a pointer to the image button widget to initialize.
//! \param psDisplay is a pointer to the display on which to draw the push
//! button.
//! \param i32X is the X coordinate of the upper left corner of the image
//! button.
//! \param i32Y is the Y coordinate of the upper left corner of the image
//! button.
//! \param i32Width is the width of the image button.
//! \param i32Height is the height of the image button.
//!
//! This function initializes the provided image button widget.
//!
//! \return None.
//
//*****************************************************************************
void
ImageButtonInit(tImageButtonWidget *psWidget, const tDisplay *psDisplay,
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
    for(ui32Idx = 0; ui32Idx < sizeof(tImageButtonWidget); ui32Idx += 4)
    {
        ((uint32_t *)psWidget)[ui32Idx / 4] = 0;
    }

    //
    // Set the size of the image button widget structure.
    //
    psWidget->sBase.i32Size = sizeof(tImageButtonWidget);

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
    // Set the extents of this rectangular image button.
    //
    psWidget->sBase.sPosition.i16XMin = i32X;
    psWidget->sBase.sPosition.i16YMin = i32Y;
    psWidget->sBase.sPosition.i16XMax = i32X + i32Width - 1;
    psWidget->sBase.sPosition.i16YMax = i32Y + i32Height - 1;

    //
    // Use the rectangular image button message handler to process messages to
    // this image button.
    //
    psWidget->sBase.pfnMsgProc = ImageButtonMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
