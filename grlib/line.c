//*****************************************************************************
//
// line.c - Routines for drawing lines.
//
// Copyright (c) 2007-2014 Texas Instruments Incorporated.  All rights reserved.
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

//*****************************************************************************
//
//! \addtogroup primitives_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Draws a horizontal line.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param i32X1 is the X coordinate of one end of the line.
//! \param i32X2 is the X coordinate of the other end of the line.
//! \param i32Y is the Y coordinate of the line.
//!
//! This function draws a horizontal line, taking advantage of the fact that
//! the line is horizontal to draw it more efficiently.  The clipping of the
//! horizontal line to the clipping rectangle is performed within this routine;
//! the display driver's horizontal line routine is used to perform the actual
//! line drawing.
//!
//! \return None.
//
//*****************************************************************************
void
GrLineDrawH(const tContext *pContext, int32_t i32X1, int32_t i32X2,
            int32_t i32Y)
{
    int32_t i32Temp;

    //
    // Check the arguments.
    //
    ASSERT(pContext);

    //
    // If the Y coordinate of this line is not in the clipping region, then
    // there is nothing to be done.
    //
    if((i32Y < pContext->sClipRegion.i16YMin) ||
       (i32Y > pContext->sClipRegion.i16YMax))
    {
        return;
    }

    //
    // Swap the X coordinates if the first is larger than the second.
    //
    if(i32X1 > i32X2)
    {
        i32Temp = i32X1;
        i32X1 = i32X2;
        i32X2 = i32Temp;
    }

    //
    // If the entire line is outside the clipping region, then there is nothing
    // to be done.
    //
    if((i32X1 > pContext->sClipRegion.i16XMax) ||
       (i32X2 < pContext->sClipRegion.i16XMin))
    {
        return;
    }

    //
    // Clip the starting coordinate to the left side of the clipping region if
    // required.
    //
    if(i32X1 < pContext->sClipRegion.i16XMin)
    {
        i32X1 = pContext->sClipRegion.i16XMin;
    }

    //
    // Clip the ending coordinate to the right side of the clipping region if
    // required.
    //
    if(i32X2 > pContext->sClipRegion.i16XMax)
    {
        i32X2 = pContext->sClipRegion.i16XMax;
    }

    //
    // Call the low level horizontal line drawing routine.
    //
    DpyLineDrawH(pContext->psDisplay, i32X1, i32X2, i32Y,
                 pContext->ui32Foreground);
}

//*****************************************************************************
//
//! Draws a vertical line.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param i32X is the X coordinate of the line.
//! \param i32Y1 is the Y coordinate of one end of the line.
//! \param i32Y2 is the Y coordinate of the other end of the line.
//!
//! This function draws a vertical line, taking advantage of the fact that the
//! line is vertical to draw it more efficiently.  The clipping of the vertical
//! line to the clipping rectangle is performed within this routine; the
//! display driver's vertical line routine is used to perform the actual line
//! drawing.
//!
//! \return None.
//
//*****************************************************************************
void
GrLineDrawV(const tContext *pContext, int32_t i32X, int32_t i32Y1,
            int32_t i32Y2)
{
    int32_t i32Temp;

    //
    // Check the arguments.
    //
    ASSERT(pContext);

    //
    // If the X coordinate of this line is not within the clipping region, then
    // there is nothing to be done.
    //
    if((i32X < pContext->sClipRegion.i16XMin) ||
       (i32X > pContext->sClipRegion.i16XMax))
    {
        return;
    }

    //
    // Swap the Y coordinates if the first is larger than the second.
    //
    if(i32Y1 > i32Y2)
    {
        i32Temp = i32Y1;
        i32Y1 = i32Y2;
        i32Y2 = i32Temp;
    }

    //
    // If the entire line is out of the clipping region, then there is nothing
    // to be done.
    //
    if((i32Y1 > pContext->sClipRegion.i16YMax) ||
       (i32Y2 < pContext->sClipRegion.i16YMin))
    {
        return;
    }

    //
    // Clip the starting coordinate to the top side of the clipping region if
    // required.
    //
    if(i32Y1 < pContext->sClipRegion.i16YMin)
    {
        i32Y1 = pContext->sClipRegion.i16YMin;
    }

    //
    // Clip the ending coordinate to the bottom side of the clipping region if
    // required.
    //
    if(i32Y2 > pContext->sClipRegion.i16YMax)
    {
        i32Y2 = pContext->sClipRegion.i16YMax;
    }

    //
    // Call the low level vertical line drawing routine.
    //
    DpyLineDrawV(pContext->psDisplay, i32X, i32Y1, i32Y2,
                 pContext->ui32Foreground);
}

//*****************************************************************************
//
//! Computes the clipping code used by the Cohen-Sutherland clipping algorithm.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param i32X is the X coordinate of the point.
//! \param i32Y is the Y coordinate of the point.
//!
//! This function computes the clipping code used by the Cohen-Sutherland
//! clipping algorithm.  Clipping is performed by classifying the endpoints of
//! the line based on their relation to the clipping region; this determines
//! those relationships.
//!
//! \return Returns the clipping code.
//
//*****************************************************************************
static int32_t
GrClipCodeGet(const tContext *pContext, int32_t i32X, int32_t i32Y)
{
    int32_t i32Code;

    //
    // Initialize the clipping code to zero.
    //
    i32Code = 0;

    //
    // Set bit zero of the clipping code if the Y coordinate is above the
    // clipping region.
    //
    if(i32Y < pContext->sClipRegion.i16YMin)
    {
        i32Code |= 1;
    }

    //
    // Set bit one of the clipping code if the Y coordinate is below the
    // clipping region.
    //
    if(i32Y > pContext->sClipRegion.i16YMax)
    {
        i32Code |= 2;
    }

    //
    // Set bit two of the clipping code if the X coordinate is to the left of
    // the clipping region.
    //
    if(i32X < pContext->sClipRegion.i16XMin)
    {
        i32Code |= 4;
    }

    //
    // Set bit three of the clipping code if the X coordinate is to the right
    // of the clipping region.
    //
    if(i32X > pContext->sClipRegion.i16XMax)
    {
        i32Code |= 8;
    }

    //
    // Return the clipping code.
    //
    return(i32Code);
}

//*****************************************************************************
//
//! Clips a line to the clipping region.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param pi32X1 is the X coordinate of the start of the line.
//! \param pi32Y1 is the Y coordinate of the start of the line.
//! \param pi32X2 is the X coordinate of the end of the line.
//! \param pi32Y2 is the Y coordinate of the end of the line.
//!
//! This function clips a line to the extents of the clipping region using the
//! Cohen-Sutherland clipping algorithm.  The ends of the line are classified
//! based on their relation to the clipping region, and the codes are used to
//! either trivially accept a line (both end points within the clipping
//! region), trivially reject a line (both end points to one side of the
//! clipping region), or to adjust an endpoint one axis at a time to the edge
//! of the clipping region until the line can either be trivially accepted or
//! trivially rejected.
//!
//! The provided coordinates are modified such that they reside within the
//! extents of the clipping region if the line is not rejected.  If it is
//! rejected, the coordinates may be modified during the process of attempting
//! to clip them.
//!
//! \return Returns one if the clipped line lies within the extent of the
//! clipping region and zero if it does not.
//
//*****************************************************************************
static int32_t
GrLineClip(const tContext *pContext, int32_t *pi32X1, int32_t *pi32Y1,
           int32_t *pi32X2, int32_t *pi32Y2)
{
    int32_t i32Code, i32Code1, i32Code2, i32X, i32Y;

    //
    // Compute the clipping codes for the two endpoints of the line.
    //
    i32Code1 = GrClipCodeGet(pContext, *pi32X1, *pi32Y1);
    i32Code2 = GrClipCodeGet(pContext, *pi32X2, *pi32Y2);

    //
    // Loop forever.  This loop will be explicitly broken out of when the line
    // is either trivially accepted or trivially rejected.
    //
    while(1)
    {
        //
        // If both codes are zero, then both points lie within the extent of
        // the clipping region.  In this case, trivally accept the line.
        //
        if((i32Code1 == 0) && (i32Code2 == 0))
        {
            return(1);
        }

        //
        // If the intersection of the codes is non-zero, then the line lies
        // entirely off one edge of the clipping region.  In this case,
        // trivally reject the line.
        //
        if((i32Code1 & i32Code2) != 0)
        {
            return(0);
        }

        //
        // Determine the end of the line to move.  The first end of the line is
        // moved until it is within the clipping region, and then the second
        // end of the line is moved until it is also within the clipping
        // region.
        //
        if(i32Code1)
        {
            i32Code = i32Code1;
        }
        else
        {
            i32Code = i32Code2;
        }

        //
        // See if this end of the line lies above the clipping region.
        //
        if(i32Code & 1)
        {
            //
            // Move this end of the line to the intersection of the line and
            // the top of the clipping region.
            //
            i32X = (*pi32X1 + (((*pi32X2 - *pi32X1) *
                            (pContext->sClipRegion.i16YMin - *pi32Y1)) /
                           (*pi32Y2 - *pi32Y1)));
            i32Y = pContext->sClipRegion.i16YMin;
        }

        //
        // Otherwise, see if this end of the line lies below the clipping
        // region.
        //
        else if(i32Code & 2)
        {
            //
            // Move this end of the line to the intersection of the line and
            // the bottom of the clipping region.
            //
            i32X = (*pi32X1 + (((*pi32X2 - *pi32X1) *
                            (pContext->sClipRegion.i16YMax - *pi32Y1)) /
                           (*pi32Y2 - *pi32Y1)));
            i32Y = pContext->sClipRegion.i16YMax;
        }

        //
        // Otherwise, see if this end of the line lies to the left of the
        // clipping region.
        //
        else if(i32Code & 4)
        {
            //
            // Move this end of the line to the intersection of the line and
            // the left side of the clipping region.
            //
            i32X = pContext->sClipRegion.i16XMin;
            i32Y = (*pi32Y1 + (((*pi32Y2 - *pi32Y1) *
                            (pContext->sClipRegion.i16XMin - *pi32X1)) /
                           (*pi32X2 - *pi32X1)));
        }

        //
        // Otherwise, this end of the line lies to the right of the clipping
        // region.
        //
        else
        {
            //
            // Move this end of the line to the intersection of the line and
            // the right side of the clipping region.
            //
            i32X = pContext->sClipRegion.i16XMax;
            i32Y = (*pi32Y1 + (((*pi32Y2 - *pi32Y1) *
                            (pContext->sClipRegion.i16XMax - *pi32X1)) /
                           (*pi32X2 - *pi32X1)));
        }

        //
        // See which end of the line just moved.
        //
        if(i32Code1)
        {
            //
            // Save the new coordinates for the start of the line.
            //
            *pi32X1 = i32X;
            *pi32Y1 = i32Y;

            //
            // Recompute the clipping code for the start of the line.
            //
            i32Code1 = GrClipCodeGet(pContext, i32X, i32Y);
        }
        else
        {
            //
            // Save the new coordinates for the end of the line.
            //
            *pi32X2 = i32X;
            *pi32Y2 = i32Y;

            //
            // Recompute the clipping code for the end of the line.
            //
            i32Code2 = GrClipCodeGet(pContext, i32X, i32Y);
        }
    }
}

//*****************************************************************************
//
//! Draws a line.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param i32X1 is the X coordinate of the start of the line.
//! \param i32Y1 is the Y coordinate of the start of the line.
//! \param i32X2 is the X coordinate of the end of the line.
//! \param i32Y2 is the Y coordinate of the end of the line.
//!
//! This function draws a line, utilizing GrLineDrawH() and GrLineDrawV() to
//! draw the line as efficiently as possible.  The line is clipped to the
//! clippping rectangle using the Cohen-Sutherland clipping algorithm, and then
//! scan converted using Bresenham's line drawing algorithm.
//!
//! \return None.
//
//*****************************************************************************
void
GrLineDraw(const tContext *pContext, int32_t i32X1, int32_t i32Y1,
           int32_t i32X2, int32_t i32Y2)
{
    int32_t i32Error, i32DeltaX, i32DeltaY, i32YStep, bSteep;

    //
    // Check the arguments.
    //
    ASSERT(pContext);

    //
    // See if this is a vertical line.
    //
    if(i32X1 == i32X2)
    {
        //
        // It is more efficient to avoid Bresenham's algorithm when drawing a
        // vertical line, so use the vertical line routine to draw this line.
        //
        GrLineDrawV(pContext, i32X1, i32Y1, i32Y2);

        //
        // The line has ben drawn, so return.
        //
        return;
    }

    //
    // See if this is a horizontal line.
    //
    if(i32Y1 == i32Y2)
    {
        //
        // It is more efficient to avoid Bresenham's algorithm when drawing a
        // horizontal line, so use the horizontal line routien to draw this
        // line.
        //
        GrLineDrawH(pContext, i32X1, i32X2, i32Y1);

        //
        // The line has ben drawn, so return.
        //
        return;
    }

    //
    // Clip this line if necessary, and return without drawing anything if the
    // line does not cross the clipping region.
    //
    if(GrLineClip(pContext, &i32X1, &i32Y1, &i32X2, &i32Y2) == 0)
    {
        return;
    }

    //
    // Determine if the line is steep.  A steep line has more motion in the Y
    // direction than the X direction.
    //
    if(((i32Y2 > i32Y1) ? (i32Y2 - i32Y1) : (i32Y1 - i32Y2)) >
       ((i32X2 > i32X1) ? (i32X2 - i32X1) : (i32X1 - i32X2)))
    {
        bSteep = 1;
    }
    else
    {
        bSteep = 0;
    }

    //
    // If the line is steep, then swap the X and Y coordinates.
    //
    if(bSteep)
    {
        i32Error = i32X1;
        i32X1 = i32Y1;
        i32Y1 = i32Error;
        i32Error = i32X2;
        i32X2 = i32Y2;
        i32Y2 = i32Error;
    }

    //
    // If the starting X coordinate is larger than the ending X coordinate,
    // then swap the start and end coordinates.
    //
    if(i32X1 > i32X2)
    {
        i32Error = i32X1;
        i32X1 = i32X2;
        i32X2 = i32Error;
        i32Error = i32Y1;
        i32Y1 = i32Y2;
        i32Y2 = i32Error;
    }

    //
    // Compute the difference between the start and end coordinates in each
    // axis.
    //
    i32DeltaX = i32X2 - i32X1;
    i32DeltaY = (i32Y2 > i32Y1) ? (i32Y2 - i32Y1) : (i32Y1 - i32Y2);

    //
    // Initialize the error term to negative half the X delta.
    //
    i32Error = -i32DeltaX / 2;

    //
    // Determine the direction to step in the Y axis when required.
    //
    if(i32Y1 < i32Y2)
    {
        i32YStep = 1;
    }
    else
    {
        i32YStep = -1;
    }

    //
    // Loop through all the points along the X axis of the line.
    //
    for(; i32X1 <= i32X2; i32X1++)
    {
        //
        // See if this is a steep line.
        //
        if(bSteep)
        {
            //
            // Plot this point of the line, swapping the X and Y coordinates.
            //
            DpyPixelDraw(pContext->psDisplay, i32Y1, i32X1,
                           pContext->ui32Foreground);
        }
        else
        {
            //
            // Plot this point of the line, using the coordinates as is.
            //
            DpyPixelDraw(pContext->psDisplay, i32X1, i32Y1,
                         pContext->ui32Foreground);
        }

        //
        // Increment the error term by the Y delta.
        //
        i32Error += i32DeltaY;

        //
        // See if the error term is now greater than zero.
        //
        if(i32Error > 0)
        {
            //
            // Take a step in the Y axis.
            //
            i32Y1 += i32YStep;

            //
            // Decrement the error term by the X delta.
            //
            i32Error -= i32DeltaX;
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
