//*****************************************************************************
//
// rectangle.c - Routines for drawing and filling rectangles.
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
//! Draws a rectangle.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param pRect is a pointer to the structure containing the extents of the
//! rectangle.
//!
//! This function draws a rectangle.  The rectangle will extend from \e i32XMin
//! to \e i32XMax and \e i32YMin to \e i32YMax, inclusive.
//!
//! \return None.
//
//*****************************************************************************
void
GrRectDraw(const tContext *pContext, const tRectangle *pRect)
{
    //
    // Check the arguments.
    //
    ASSERT(pContext);
    ASSERT(pRect);

    //
    // Draw a line across the top of the rectangle.
    //
    GrLineDrawH(pContext, pRect->i16XMin, pRect->i16XMax, pRect->i16YMin);

    //
    // Return if the rectangle is one pixel tall.
    //
    if(pRect->i16YMin == pRect->i16YMax)
    {
        return;
    }

    //
    // Draw a line down the right side of the rectangle.
    //
    GrLineDrawV(pContext, pRect->i16XMax, pRect->i16YMin + 1, pRect->i16YMax);

    //
    // Return if the rectangle is one pixel wide.
    //
    if(pRect->i16XMin == pRect->i16XMax)
    {
        return;
    }

    //
    // Draw a line across the bottom of the rectangle.
    //
    GrLineDrawH(pContext, pRect->i16XMax - 1, pRect->i16XMin, pRect->i16YMax);

    //
    // Return if the rectangle is two pixels tall.
    //
    if((pRect->i16YMin + 1) == pRect->i16YMax)
    {
        return;
    }

    //
    // Draw a line up the left side of the rectangle.
    //
    GrLineDrawV(pContext, pRect->i16XMin, pRect->i16YMax - 1,
                pRect->i16YMin + 1);
}

//*****************************************************************************
//
//! Draws a filled rectangle.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param pRect is a pointer to the structure containing the extents of the
//! rectangle.
//!
//! This function draws a filled rectangle.  The rectangle will extend from
//! \e i32XMin to \e i32XMax and \e i32YMin to \e i32YMax, inclusive.  The
//! clipping of the rectangle to the clipping rectangle is performed within
//! this routine; the display driver's rectangle fill routine is used to
//! perform the actual rectangle fill.
//!
//! \return None.
//
//*****************************************************************************
void
GrRectFill(const tContext *pContext, const tRectangle *pRect)
{
    tRectangle sTemp;

    //
    // Check the arguments.
    //
    ASSERT(pContext);
    ASSERT(pRect);

    //
    // Swap the X coordinates if i16XMin is greater than i16XMax.
    //
    if(pRect->i16XMin > pRect->i16XMax)
    {
        sTemp.i16XMin = pRect->i16XMax;
        sTemp.i16XMax = pRect->i16XMin;
    }
    else
    {
        sTemp.i16XMin = pRect->i16XMin;
        sTemp.i16XMax = pRect->i16XMax;
    }

    //
    // Swap the Y coordinates if i16YMin is greater than i16YMax.
    //
    if(pRect->i16YMin > pRect->i16YMax)
    {
        sTemp.i16YMin = pRect->i16YMax;
        sTemp.i16YMax = pRect->i16YMin;
    }
    else
    {
        sTemp.i16YMin = pRect->i16YMin;
        sTemp.i16YMax = pRect->i16YMax;
    }

    //
    // Now that the coordinates are ordered, return without drawing anything if
    // the entire rectangle is out of the clipping region.
    //
    if((sTemp.i16XMin > pContext->sClipRegion.i16XMax) ||
       (sTemp.i16XMax < pContext->sClipRegion.i16XMin) ||
       (sTemp.i16YMin > pContext->sClipRegion.i16YMax) ||
       (sTemp.i16YMax < pContext->sClipRegion.i16YMin))
    {
        return;
    }

    //
    // Clip the X coordinates to the edges of the clipping region if necessary.
    //
    if(sTemp.i16XMin < pContext->sClipRegion.i16XMin)
    {
        sTemp.i16XMin = pContext->sClipRegion.i16XMin;
    }
    if(sTemp.i16XMax > pContext->sClipRegion.i16XMax)
    {
        sTemp.i16XMax = pContext->sClipRegion.i16XMax;
    }

    //
    // Clip the Y coordinates to the edges of the clipping region if necessary.
    //
    if(sTemp.i16YMin < pContext->sClipRegion.i16YMin)
    {
        sTemp.i16YMin = pContext->sClipRegion.i16YMin;
    }
    if(sTemp.i16YMax > pContext->sClipRegion.i16YMax)
    {
        sTemp.i16YMax = pContext->sClipRegion.i16YMax;
    }

    //
    // Call the low level rectangle fill routine.
    //
    DpyRectFill(pContext->psDisplay, &sTemp, pContext->ui32Foreground);
}

//*****************************************************************************
//
//! Determines if two rectangles overlap.
//!
//! \param psRect1 is a pointer to the first rectangle.
//! \param psRect2 is a pointer to the second rectangle.
//!
//! This function determines whether two rectangles overlap.  It assumes that
//! rectangles \e psRect1 and \e psRect2 are valid with \e i16XMin <
//! \e i16XMax and \e i16YMin < \e i16YMax.
//!
//! \return Returns 1 if there is an overlap or 0 if not.
//
//*****************************************************************************
int32_t
GrRectOverlapCheck(tRectangle *psRect1, tRectangle *psRect2)
{
    if((psRect1->i16XMax < psRect2->i16XMin) ||
       (psRect2->i16XMax < psRect1->i16XMin) ||
       (psRect1->i16YMax < psRect2->i16YMin) ||
       (psRect2->i16YMax < psRect1->i16YMin))
    {
        return(0);
    }
    else
    {
        return(1);
    }
}

//*****************************************************************************
//
//! Determines the intersection of two rectangles.
//!
//! \param psRect1 is a pointer to the first rectangle.
//! \param psRect2 is a pointer to the second rectangle.
//! \param psIntersect is a pointer to a rectangle which will be written with
//! the intersection of \e psRect1 and \e psRect2.
//!
//! This function determines if two rectangles overlap and, if they do,
//! calculates the rectangle representing their intersection.  If the rectangles
//! do not overlap, 0 is returned and \e psIntersect is not written.
//!
//! \return Returns 1 if there is an overlap or 0 if not.
//
//*****************************************************************************
int32_t
GrRectIntersectGet(tRectangle *psRect1, tRectangle *psRect2,
                   tRectangle *psIntersect)
{
    //
    // Make sure we were passed valid rectangles.
    //
    if((psRect1->i16XMax <= psRect1->i16XMin) ||
       (psRect1->i16YMax <= psRect1->i16YMin) ||
       (psRect2->i16XMax <= psRect2->i16XMin) ||
       (psRect2->i16YMax <= psRect2->i16YMin))
    {
        return(0);
    }

    //
    // Make sure that there is an intersection between the two rectangles.
    //
    if(!GrRectOverlapCheck(psRect1, psRect2))
    {
        return(0);
    }

    //
    // The rectangles do intersect so determine the rectangle of the
    // intersection.
    //
    psIntersect->i16XMin = max(psRect1->i16XMin, psRect2->i16XMin);
    psIntersect->i16XMax = min(psRect1->i16XMax, psRect2->i16XMax);
    psIntersect->i16YMin = max(psRect1->i16YMin, psRect2->i16YMin);
    psIntersect->i16YMax = min(psRect1->i16YMax, psRect2->i16YMax);

    return(1);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
