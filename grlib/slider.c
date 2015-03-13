//*****************************************************************************
//
// slider.c - A simple slider widget class.
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
#include "grlib/slider.h"

//*****************************************************************************
//
//! \addtogroup slider_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Make sure min and max are defined.
//
//*****************************************************************************
#ifndef min
#define min(a, b)               (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b)               (((a) < (b)) ? (b) : (a))
#endif

//*****************************************************************************
//
// Converts a slider value to a position on the display.
//
// \param pSlider is a pointer to the slider widget for which the conversion is
// being requested.
// \param i32Value is the slider value that is to be converted to a position.
//
// Converts a value within the range represented by a slider into a position
// along the slider control.  The function converts taking into account the
// slider position and style as well as its range.
//
// \return Returns the screen position (x coordinate for horizontal sliders or
// y coordinate for vertical ones) that represents the value passed.
//
//*****************************************************************************
static int16_t
SliderValueToPosition(tSliderWidget *pSlider, int32_t i32Value)
{
    uint16_t ui16Size;
    int32_t i32Range;
    int32_t i32Pos;

    //
    // First look for the trivial cases.  To ensure correct display and remove
    // artifacts caused by rounding errors, we specifically catch the cases
    // where the value provided is at either end of the slider range.  In these
    // cases we return values that are outside the actual widget rectangle.
    // This is detected while drawing so that the relevant bars fill the full
    // area and don't leave single pixel lines at either end even when the
    // slider is at full scale.  These cases also catch out-of-range values
    // and peg them at one end of the range or the other.
    //
    // First check for values at the top of the range.
    //
    if(i32Value >= pSlider->i32Max)
    {
        //
        // Is this a vertical slider?
        //
        if(pSlider->ui32Style & SL_STYLE_VERTICAL)
        {
            //
            // Vertical slider case.  Return the top position.
            //
            i32Pos = pSlider->sBase.sPosition.i16YMin - 1;

            //
            // Adjust by 1 to move past the border if this widget has one.
            //
            if(pSlider->ui32Style & SL_STYLE_OUTLINE)
            {
                i32Pos++;
            }
        }
        else
        {
            //
            // Horizontal slider case.  Return the rightmost position.
            //
            i32Pos = pSlider->sBase.sPosition.i16XMax + 1;

            //
            // Adjust by 1 to move past the border if this widget has one.
            //
            if(pSlider->ui32Style & SL_STYLE_OUTLINE)
            {
                i32Pos--;
            }
        }
        return((int16_t)i32Pos);
    }

    //
    // Now look at the bottom end of the range.
    //
    if(i32Value <= pSlider->i32Min)
    {
        //
        // Is this a vertical slider?
        //
        if(pSlider->ui32Style & SL_STYLE_VERTICAL)
        {
            //
            // Vertical slider case.  Return the bottom position.
            //
            i32Pos = pSlider->sBase.sPosition.i16YMax + 1;

            //
            // Adjust by 1 to move past the border if this widget has one.
            //
            if(pSlider->ui32Style & SL_STYLE_OUTLINE)
            {
                i32Pos--;
            }
        }
        else
        {
            //
            // Horizontal slider case.  Return the leftmost position.
            //
            i32Pos = pSlider->sBase.sPosition.i16XMin - 1;

            //
            // Adjust by 1 to move past the border if this widget has one.
            //
            if(pSlider->ui32Style & SL_STYLE_OUTLINE)
            {
                i32Pos++;
            }
        }
        return((int16_t)i32Pos);
    }

    //
    // What is the length of the whole slider?
    //
    if(pSlider->ui32Style & SL_STYLE_VERTICAL)
    {
        //
        // Vertical slider case.
        //
        ui16Size = (pSlider->sBase.sPosition.i16YMax -
                  pSlider->sBase.sPosition.i16YMin) + 1;
    }
    else
    {
        //
        // Horizontal slider case.
        //
        ui16Size = (pSlider->sBase.sPosition.i16XMax -
                  pSlider->sBase.sPosition.i16XMin) + 1;
    }

    //
    // Adjust the range if the slider has an outline (which removes 2 pixels).
    //
    if(pSlider->ui32Style & SL_STYLE_OUTLINE)
    {
        ui16Size -= 2;
    }

    //
    // Determine the range of the slider (the number of individual integers
    // represented by the widget).
    //
    i32Range = (pSlider->i32Max - pSlider->i32Min) + 1;

    //
    // Now we can determine the relevant position relative to the start of the
    // slider.
    //
    i32Pos = (((i32Value - pSlider->i32Min) * (int32_t)ui16Size) / i32Range);

    //
    // Clip the calculated position to the valid range based on the slider
    // size.
    //
    i32Pos = max(i32Pos, 0);
    i32Pos = min(i32Pos, (int32_t)ui16Size - 1);

    //
    // Adjust for the position of the widget relative to the screen origin.
    //
    if(pSlider->ui32Style & SL_STYLE_VERTICAL)
    {
        //
        // Vertical case - adjust the Y coordinate.
        //
        i32Pos = pSlider->sBase.sPosition.i16YMax - i32Pos;
    }
    else
    {
        //
        // Horizontal case - adjust the X coordinate.
        //
        i32Pos += pSlider->sBase.sPosition.i16XMin;
    }

    //
    // If the widget has an outline, make sure to adjust for this too.
    //
    i32Pos += ((pSlider->ui32Style & SL_STYLE_OUTLINE) ? 1 : 0);

    //
    // Convert to the expected return type and hand the caller the final
    // value.
    //
    return((int16_t)i32Pos);
}

//*****************************************************************************
//
// Converts a slider position to a value within its range.
//
// \param pSlider is a pointer to the slider widget for which the conversion is
// being requested.
// \param ui16Pos is a position within the slider.  This is an x coordinate for
// a horizontal slider or a y coordinate for a vertical one.  In both cases,
// the position is relative to the display origin.
//
// Converts a screen position into a value within the current range of the
// slider.  The function converts taking into account the slider position and
// style as well as its range.
//
// \return Returns the slider value represented by the position passed.
//
//*****************************************************************************
static int32_t
SliderPositionToValue(tSliderWidget *pSlider, int16_t i16Pos)
{
    int16_t i16Max;
    int16_t i16Min;
    int32_t i32Value;

    //
    // Determine the bounds of the control on the display.
    //
    if(pSlider->ui32Style & SL_STYLE_VERTICAL)
    {
        i16Max = pSlider->sBase.sPosition.i16YMax;
        i16Min = pSlider->sBase.sPosition.i16YMin;
    }
    else
    {
        i16Max = pSlider->sBase.sPosition.i16XMax;
        i16Min = pSlider->sBase.sPosition.i16XMin;
    }

    //
    // Adjust for the outline if present.
    //
    if(pSlider->ui32Style & SL_STYLE_OUTLINE)
    {
        i16Max--;
        i16Min--;
    }

    //
    // If the control is too narrow, this is a bug but handle it gracefully
    // rather than throwing a divide by zero later.
    //
    ASSERT(i16Max > i16Min);
    if(i16Max <= i16Min)
    {
        return(pSlider->i32Min);
    }

    //
    // Clip the supplied position to the extent of the widget.
    //
    i16Pos = min(i16Max, i16Pos);
    i16Pos = max(i16Min, i16Pos);

    //
    // Adjust the position to make it relative to the start of the slider.
    //
    if(pSlider->ui32Style & SL_STYLE_VERTICAL)
    {
        i16Pos = i16Max - i16Pos;
    }

    else
    {
        i16Pos -= i16Min;
    }

    //
    // Calculate the value represented by this position.
    //
    i32Value = ((int32_t)i16Pos * ((pSlider->i32Max - pSlider->i32Min) + 1)) /
             (int32_t)((i16Max - i16Min) + 1);

    //
    // Adjust for the bottom of the value range.
    //
    i32Value += pSlider->i32Min;

    //
    // Hand the conversion result back to the caller.
    //
    return(i32Value);
}

//*****************************************************************************
//
//! Draws a slider.
//!
//! \param psWidget is a pointer to the slider widget to be drawn.
//! \param psDirty is the subrectangle of the widget which is to be redrawn.
//! This is expressed in screen coordinates.
//!
//! This function draws a slider on the display.  This is called in response to
//! a \b #WIDGET_MSG_PAINT message or when the slider position changes.
//!
//! \return None.
//
//*****************************************************************************
static void
SliderPaint(tWidget *psWidget, tRectangle *psDirty)
{
    tRectangle sClipRect, sValueRect, sEmptyRect, sActiveClip;
    tSliderWidget *pSlider;
    tContext sCtx;
    int32_t i32X, i32Y;
    bool bIntersect;
    int16_t i16Pos;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a slider widget pointer.
    //
    pSlider = (tSliderWidget *)psWidget;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sCtx, psWidget->psDisplay);

    //
    // Initialize the clipping region based on the update rectangle passed.
    //
    bIntersect = GrRectIntersectGet(psDirty, &(pSlider->sBase.sPosition),
                                    &sClipRect);
    GrContextClipRegionSet(&sCtx, &sClipRect);

    //
    // Draw the control outline if necessary.
    //
    if(pSlider->ui32Style & SL_STYLE_OUTLINE)
    {
        //
        // Outline the slider with the outline color.
        //
        GrContextForegroundSet(&sCtx, pSlider->ui32OutlineColor);
        GrRectDraw(&sCtx, &(psWidget->sPosition));

        //
        // Adjust the clipping rectangle to prevent the outline from being
        // corrupted later.
        //
        if(sClipRect.i16XMin == psWidget->sPosition.i16XMin)
        {
            sClipRect.i16XMin++;
        }

        if(sClipRect.i16YMin == psWidget->sPosition.i16YMin)
        {
            sClipRect.i16YMin++;
        }

        if(sClipRect.i16XMax == psWidget->sPosition.i16XMax)
        {
            sClipRect.i16XMax--;
        }

        if(sClipRect.i16YMax == psWidget->sPosition.i16YMax)
        {
            sClipRect.i16YMax--;
        }
    }

    //
    // Determine the position associated with the current slider value.
    //
    i16Pos = SliderValueToPosition(pSlider, pSlider->i32Value);

    //
    // Remember this so that the dirty rectangle code in the click handler
    // draws the correct thing the first time it is called.
    //
    pSlider->i16Pos = i16Pos;

    //
    // Determine the rectangles for the active and empty portions of the
    // widget.
    //
    if(pSlider->ui32Style & SL_STYLE_VERTICAL)
    {
        //
        // Determine the rectangle corresponding to the bottom (value) portion
        // of the slider.
        //
        sValueRect.i16XMin = psWidget->sPosition.i16XMin;
        sValueRect.i16XMax = psWidget->sPosition.i16XMax;
        sValueRect.i16YMin = i16Pos;
        sValueRect.i16YMax = psWidget->sPosition.i16YMax;

        //
        // Determine the rectangle corresponding to the top (empty) portion
        // of the slider.
        //
        sEmptyRect.i16XMin = psWidget->sPosition.i16XMin;
        sEmptyRect.i16XMax = psWidget->sPosition.i16XMax;
        sEmptyRect.i16YMin = psWidget->sPosition.i16YMin;
        sEmptyRect.i16YMax = max(sEmptyRect.i16YMin, sValueRect.i16YMin - 1);
    }
    else
    {
        //
        // Determine the rectangle corresponding to the bottom (value) portion
        // of the slider.
        //
        sValueRect.i16YMin = psWidget->sPosition.i16YMin;
        sValueRect.i16YMax = psWidget->sPosition.i16YMax;
        sValueRect.i16XMin = psWidget->sPosition.i16XMin;
        sValueRect.i16XMax = i16Pos;

        //
        // Determine the rectangle corresponding to the top (empty) portion
        // of the slider.
        //
        sEmptyRect.i16YMin = psWidget->sPosition.i16YMin;
        sEmptyRect.i16YMax = psWidget->sPosition.i16YMax;
        sEmptyRect.i16XMax = psWidget->sPosition.i16XMax;
        sEmptyRect.i16XMin = min(sEmptyRect.i16XMax, sValueRect.i16XMax + 1);
    }

    //
    // Compute the center of the slider.  This will be needed later if drawing
    // text or an image.
    //
    i32X = (psWidget->sPosition.i16XMin +
          ((psWidget->sPosition.i16XMax -
            psWidget->sPosition.i16XMin + 1) / 2));
    i32Y = (psWidget->sPosition.i16YMin +
          ((psWidget->sPosition.i16YMax -
            psWidget->sPosition.i16YMin + 1) / 2));

    //
    // Get the required clipping rectangle for the active/value part of
    // the slider.
    //
    bIntersect = GrRectIntersectGet(&sClipRect, &sValueRect, &sActiveClip);

    //
    // Does any part of the value rectangle intersect with the region we are
    // supposed to be redrawing?
    //
    if(bIntersect)
    {
        //
        // Yes - we have something to draw.
        //

        //
        // Set the new clipping rectangle.
        //
        GrContextClipRegionSet(&sCtx, &sActiveClip);

        //
        // Do we need to fill the active area with a color?
        //
        if(pSlider->ui32Style & SL_STYLE_FILL)
        {
            GrContextForegroundSet(&sCtx, pSlider->ui32FillColor);
            GrRectFill(&sCtx, &sValueRect);
        }

        //
        // Do we need to draw an image in the active area?
        //
        if(pSlider->ui32Style & SL_STYLE_IMG)
        {
            GrContextForegroundSet(&sCtx, pSlider->ui32TextColor);
            GrContextBackgroundSet(&sCtx, pSlider->ui32FillColor);
            GrImageDraw(&sCtx, pSlider->pui8Image,
                        i32X - (GrImageWidthGet(pSlider->pui8Image) / 2),
                        i32Y - (GrImageHeightGet(pSlider->pui8Image) / 2));
        }

        //
        // Do we need to render a text string over the top of the active area?
        //
        if(pSlider->ui32Style & SL_STYLE_TEXT)
        {
            GrContextFontSet(&sCtx, pSlider->psFont);
            GrContextForegroundSet(&sCtx, pSlider->ui32TextColor);
            GrContextBackgroundSet(&sCtx, pSlider->ui32FillColor);
            GrStringDrawCentered(&sCtx, pSlider->pcText, -1, i32X, i32Y,
                                 pSlider->ui32Style & SL_STYLE_TEXT_OPAQUE);
        }
    }

    //
    // Now get the required clipping rectangle for the background portion of
    // the slider.
    //
    bIntersect = GrRectIntersectGet(&sClipRect, &sEmptyRect, &sActiveClip);

    //
    // Does any part of the background rectangle intersect with the region we
    // are supposed to be redrawing?
    //
    if(bIntersect)
    {
        //
        // Yes - we have something to draw.
        //

        //
        // Set the new clipping rectangle.
        //
        GrContextClipRegionSet(&sCtx, &sActiveClip);

        //
        // Do we need to fill the active area with a color?
        //
        if(pSlider->ui32Style & SL_STYLE_BACKG_FILL)
        {
            GrContextForegroundSet(&sCtx, pSlider->ui32BackgroundFillColor);
            GrRectFill(&sCtx, &sEmptyRect);
        }

        //
        // Do we need to draw an image in the active area?
        //
        if(pSlider->ui32Style & SL_STYLE_BACKG_IMG)
        {
            GrContextForegroundSet(&sCtx, pSlider->ui32BackgroundTextColor);
            GrContextBackgroundSet(&sCtx, pSlider->ui32BackgroundFillColor);
            GrImageDraw(&sCtx, pSlider->pui8BackgroundImage,
                  i32X - (GrImageWidthGet(pSlider->pui8BackgroundImage) / 2),
                  i32Y - (GrImageHeightGet(pSlider->pui8BackgroundImage) / 2));
        }

        //
        // Do we need to render a text string over the top of the active area?
        //
        if(pSlider->ui32Style & SL_STYLE_BACKG_TEXT)
        {
            GrContextFontSet(&sCtx, pSlider->psFont);
            GrContextForegroundSet(&sCtx, pSlider->ui32BackgroundTextColor);
            GrContextBackgroundSet(&sCtx, pSlider->ui32BackgroundFillColor);
            GrStringDrawCentered(&sCtx, pSlider->pcText, -1, i32X, i32Y,
                              pSlider->ui32Style & SL_STYLE_BACKG_TEXT_OPAQUE);
        }
    }
}

//*****************************************************************************
//
//! Handles pointer events for slider.
//!
//! \param psWidget is a pointer to the slider widget.
//! \param ui32Msg is the pointer event message.
//! \param i32X is the X coordinate of the pointer event.
//! \param i32Y is the Y coordinate of the pointer event.
//!
//! This function processes pointer event messages for a slider.  This is
//! called in response to a \b #WIDGET_MSG_PTR_DOWN, \b #WIDGET_MSG_PTR_MOVE,
//! and \b #WIDGET_MSG_PTR_UP messages.
//!
//! If the message is \b #WIDGET_MSG_PTR_MOVE or is \b #WIDGET_MSG_PTR_DOWN and
//! the coordinates are within the bounds of the slider, the slider value is
//! updated and, if changed, the slider's OnChange callback function is called.
//!
//! \return Returns 1 if the message was consumed by the slider and 0
//! otherwise.
//
//*****************************************************************************
static int32_t
SliderClick(tWidget *psWidget, uint32_t ui32Msg, int32_t i32X, int32_t i32Y)
{
    tSliderWidget *pSlider;
    tRectangle sRedrawRect;
    int16_t i16Pos;
    int32_t i32NewVal;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a slider widget pointer.
    //
    pSlider = (tSliderWidget *)psWidget;

    //
    // If the slider is locked, ignore all pointer messages.
    //
    if(pSlider->ui32Style & SL_STYLE_LOCKED)
    {
        return(0);
    }

    //
    // See if the given coordinates are within the extents of the slider.
    //
    if((ui32Msg == WIDGET_MSG_PTR_MOVE) ||
       ((ui32Msg == WIDGET_MSG_PTR_DOWN) &&
        (i32X >= psWidget->sPosition.i16XMin) &&
        (i32X <= psWidget->sPosition.i16XMax) &&
        (i32Y >= psWidget->sPosition.i16YMin) &&
        (i32Y <= psWidget->sPosition.i16YMax)))
    {
        //
        // Map the pointer position to a slider value.
        //
        i32NewVal = SliderPositionToValue(pSlider,
                             (pSlider->ui32Style & SL_STYLE_VERTICAL) ?
                             i32Y : i32X);

        //
        // Convert back to ensure that the dirty rectangle we calculate here
        // uses the same values as will be used when the widget is next
        // painted.
        //
        i16Pos = SliderValueToPosition(pSlider, i32NewVal);

        //
        // Did the value change?
        //
        if(i32NewVal != pSlider->i32Value)
        {
            //
            // Yes - the value changed so report it to the app and redraw the
            // slider.
            //
            if(pSlider->pfnOnChange)
            {
                (pSlider->pfnOnChange)(psWidget, i32NewVal);
            }

            //
            // Determine the rectangle that we need to redraw to update the
            // slider to the new position.
            //
            if(pSlider->ui32Style & SL_STYLE_VERTICAL)
            {
                //
                // Vertical slider case.
                //
                sRedrawRect.i16YMin = min(pSlider->i16Pos, i16Pos);
                sRedrawRect.i16YMax = max(pSlider->i16Pos, i16Pos);
                sRedrawRect.i16XMin = psWidget->sPosition.i16XMin;
                sRedrawRect.i16XMax = psWidget->sPosition.i16XMax;
            }
            else
            {
                //
                // Horizontal slider case.
                //
                sRedrawRect.i16XMin = min(pSlider->i16Pos, i16Pos);
                sRedrawRect.i16XMax = max(pSlider->i16Pos, i16Pos);
                sRedrawRect.i16YMin = psWidget->sPosition.i16YMin;
                sRedrawRect.i16YMax = psWidget->sPosition.i16YMax;
            }

            //
            // Update the widget value and position.
            //
            pSlider->i32Value = i32NewVal;
            pSlider->i16Pos = i16Pos;

            //
            // Redraw the area of the control that has changed.
            //
            SliderPaint(psWidget, &sRedrawRect);
        }

        //
        // These coordinates are within the extents of the slider widget.
        //
        return(1);
    }

    //
    // These coordinates are not within the extents of the slider widget.
    //
    return(0);
}

//*****************************************************************************
//
//! Handles messages for a slider widget.
//!
//! \param psWidget is a pointer to the slider widget.
//! \param ui32Msg is the message.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function receives messages intended for this slider widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
int32_t
SliderMsgProc(tWidget *psWidget, uint32_t ui32Msg, uint32_t ui32Param1,
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
            SliderPaint(psWidget, &(psWidget->sPosition));

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
            return(SliderClick(psWidget, ui32Msg, ui32Param1, ui32Param2));
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
//! Initializes a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to initialize.
//! \param psDisplay is a pointer to the display on which to draw the slider.
//! \param i32X is the X coordinate of the upper left corner of the slider.
//! \param i32Y is the Y coordinate of the upper left corner of the slider.
//! \param i32Width is the width of the slider.
//! \param i32Height is the height of the slider.
//!
//! This function initializes the provided slider widget.
//!
//! \return None.
//
//*****************************************************************************
void
SliderInit(tSliderWidget *psWidget, const tDisplay *psDisplay, int32_t i32X,
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
    for(ui32Idx = 0; ui32Idx < sizeof(tSliderWidget); ui32Idx += 4)
    {
        ((uint32_t *)psWidget)[ui32Idx / 4] = 0;
    }

    //
    // Set the size of the slider widget structure.
    //
    psWidget->sBase.i32Size = sizeof(tSliderWidget);

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
    // Set the extents of this slider.
    //
    psWidget->sBase.sPosition.i16XMin = i32X;
    psWidget->sBase.sPosition.i16YMin = i32Y;
    psWidget->sBase.sPosition.i16XMax = i32X + i32Width - 1;
    psWidget->sBase.sPosition.i16YMax = i32Y + i32Height - 1;

    //
    // Use the slider message handler to process messages to this widget.
    //
    psWidget->sBase.pfnMsgProc = SliderMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
