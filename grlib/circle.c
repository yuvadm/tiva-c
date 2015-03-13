//*****************************************************************************
//
// circle.c - Routines for drawing circles.
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
//! Draws a circle.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param i32X is the X coordinate of the center of the circle.
//! \param i32Y is the Y coordinate of the center of the circle.
//! \param i32Radius is the radius of the circle.
//!
//! This function draws a circle, utilizing the Bresenham circle drawing
//! algorithm.  The extent of the circle is from \e i32X - \e i32Radius to
//! \e i32X + \e i32Radius and \e i32Y - \e i32Radius to \e i32Y +
//! \e i32Radius, inclusive.
//!
//! \return None.
//
//*****************************************************************************
void
GrCircleDraw(const tContext *pContext, int32_t i32X, int32_t i32Y,
             int32_t i32Radius)
{
    int_fast32_t i32A, i32B, i32D, i32X1, i32Y1;

    //
    // Check the arguments.
    //
    ASSERT(pContext);

    //
    // Initialize the variables that control the Bresenham circle drawing
    // algorithm.
    //
    i32A = 0;
    i32B = i32Radius;
    i32D = 3 - (2 * i32Radius);

    //
    // Loop until the A delta is greater than the B delta, meaning that the
    // entire circle has been drawn.
    //
    while(i32A <= i32B)
    {
        //
        // Determine the row when subtracting the A delta.
        //
        i32Y1 = i32Y - i32A;

        //
        // See if this row is within the clipping region.
        //
        if((i32Y1 >= pContext->sClipRegion.i16YMin) &&
           (i32Y1 <= pContext->sClipRegion.i16YMax))
        {
            //
            // Determine the column when subtracting the B delta.
            //
            i32X1 = i32X - i32B;

            //
            // If this column is within the clipping region, then draw a pixel
            // at that position.
            //
            if((i32X1 >= pContext->sClipRegion.i16XMin) &&
               (i32X1 <= pContext->sClipRegion.i16XMax))
            {
                GrPixelDraw(pContext, i32X1, i32Y1);
            }

            //
            // Determine the column when adding the B delta.
            //
            i32X1 = i32X + i32B;

            //
            // If this column is within the clipping region, then draw a pixel
            // at that position.
            //
            if((i32X1 >= pContext->sClipRegion.i16XMin) &&
               (i32X1 <= pContext->sClipRegion.i16XMax))
            {
                GrPixelDraw(pContext, i32X1, i32Y1);
            }
        }

        //
        // Determine the row when adding the A delta.
        //
        i32Y1 = i32Y + i32A;

        //
        // See if this row is within the clipping region, and the A delta is
        // not zero (otherwise, it will be the same row as when the A delta was
        // subtracted).
        //
        if((i32Y1 >= pContext->sClipRegion.i16YMin) &&
           (i32Y1 <= pContext->sClipRegion.i16YMax) &&
           (i32A != 0))
        {
            //
            // Determine the column when subtracting the B delta.
            //
            i32X1 = i32X - i32B;

            //
            // If this column is within the clipping region, then draw a pixel
            // at that position.
            //
            if((i32X1 >= pContext->sClipRegion.i16XMin) &&
               (i32X1 <= pContext->sClipRegion.i16XMax))
            {
                GrPixelDraw(pContext, i32X1, i32Y1);
            }

            //
            // Determine the column when adding the B delta.
            //
            i32X1 = i32X + i32B;

            //
            // If this column is within the clipping region, then draw a pixel
            // at that position.
            //
            if((i32X1 >= pContext->sClipRegion.i16XMin) &&
               (i32X1 <= pContext->sClipRegion.i16XMax))
            {
                GrPixelDraw(pContext, i32X1, i32Y1);
            }
        }

        //
        // Only draw the complementary pixels if the A and B deltas are
        // different (otherwise, they describe the same set of pixels).
        //
        if(i32A != i32B)
        {
            //
            // Determine the row when subtracting the B delta.
            //
            i32Y1 = i32Y - i32B;

            //
            // See if this row is within the clipping region.
            //
            if((i32Y1 >= pContext->sClipRegion.i16YMin) &&
               (i32Y1 <= pContext->sClipRegion.i16YMax))
            {
                //
                // Determine the column when subtracting the a delta.
                //
                i32X1 = i32X - i32A;

                //
                // If this column is within the clipping region, then draw a
                // pixel at that position.
                //
                if((i32X1 >= pContext->sClipRegion.i16XMin) &&
                   (i32X1 <= pContext->sClipRegion.i16XMax))
                {
                    GrPixelDraw(pContext, i32X1, i32Y1);
                }

                //
                // Only draw the mirrored pixel if the A delta is non-zero
                // (otherwise, it will be the same pixel).
                //
                if(i32A != 0)
                {
                    //
                    // Determine the column when adding the A delta.
                    //
                    i32X1 = i32X + i32A;

                    //
                    // If this column is within the clipping region, then draw
                    // a pixel at that position.
                    //
                    if((i32X1 >= pContext->sClipRegion.i16XMin) &&
                       (i32X1 <= pContext->sClipRegion.i16XMax))
                    {
                        GrPixelDraw(pContext, i32X1, i32Y1);
                    }
                }
            }

            //
            // Determine the row when adding the B delta.
            //
            i32Y1 = i32Y + i32B;

            //
            // See if this row is within the clipping region.
            //
            if((i32Y1 >= pContext->sClipRegion.i16YMin) &&
               (i32Y1 <= pContext->sClipRegion.i16YMax))
            {
                //
                // Determine the column when subtracting the A delta.
                //
                i32X1 = i32X - i32A;

                //
                // If this column is within the clipping region, then draw a
                // pixel at that position.
                //
                if((i32X1 >= pContext->sClipRegion.i16XMin) &&
                   (i32X1 <= pContext->sClipRegion.i16XMax))
                {
                    GrPixelDraw(pContext, i32X1, i32Y1);
                }

                //
                // Only draw the mirrored pixel if the A delta is non-zero
                // (otherwise, it will be the same pixel).
                //
                if(i32A != 0)
                {
                    //
                    // Determine the column when adding the A delta.
                    //
                    i32X1 = i32X + i32A;

                    //
                    // If this column is within the clipping region, then draw
                    // a pixel at that position.
                    //
                    if((i32X1 >= pContext->sClipRegion.i16XMin) &&
                       (i32X1 <= pContext->sClipRegion.i16XMax))
                    {
                        GrPixelDraw(pContext, i32X1, i32Y1);
                    }
                }
            }
        }

        //
        // See if the error term is negative.
        //
        if(i32D < 0)
        {
            //
            // Since the error term is negative, adjust it based on a move in
            // only the A delta.
            //
            i32D += (4 * i32A) + 6;
        }
        else
        {
            //
            // Since the error term is non-negative, adjust it based on a move
            // in both the A and B deltas.
            //
            i32D += (4 * (i32A - i32B)) + 10;

            //
            // Decrement the B delta.
            //
            i32B -= 1;
        }

        //
        // Increment the A delta.
        //
        i32A++;
    }
}

//*****************************************************************************
//
//! Draws a filled circle.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param i32X is the X coordinate of the center of the circle.
//! \param i32Y is the Y coordinate of the center of the circle.
//! \param i32Radius is the radius of the circle.
//!
//! This function draws a filled circle, utilizing the Bresenham circle drawing
//! algorithm.  The extent of the circle is from \e i32X - \e i32Radius to
//! \e i32X + \e i32Radius and \e i32Y - \e i32Radius to \e i32Y +
//! \e i32Radius, inclusive.
//!
//! \return None.
//
//*****************************************************************************
void
GrCircleFill(const tContext *pContext, int32_t i32X, int32_t i32Y,
             int32_t i32Radius)
{
    int_fast32_t i32A, i32B, i32D, i32X1, i32X2, i32Y1;

    //
    // Check the arguments.
    //
    ASSERT(pContext);

    //
    // Initialize the variables that control the Bresenham circle drawing
    // algorithm.
    //
    i32A = 0;
    i32B = i32Radius;
    i32D = 3 - (2 * i32Radius);

    //
    // Loop until the A delta is greater than the B delta, meaning that the
    // entire circle has been filled.
    //
    while(i32A <= i32B)
    {
        //
        // Determine the row when subtracting the A delta.
        //
        i32Y1 = i32Y - i32A;

        //
        // See if this row is within the clipping region.
        //
        if((i32Y1 >= pContext->sClipRegion.i16YMin) &&
           (i32Y1 <= pContext->sClipRegion.i16YMax))
        {
            //
            // Determine the column when subtracting the B delta, and move it
            // to the left edge of the clipping region if it is to the left of
            // the clipping region.
            //
            i32X1 = i32X - i32B;
            if(i32X1 < pContext->sClipRegion.i16XMin)
            {
                i32X1 = pContext->sClipRegion.i16XMin;
            }

            //
            // Determine the column when adding the B delta, and move it to the
            // right edge of the clipping region if it is to the right of the
            // clipping region.
            //
            i32X2 = i32X + i32B;
            if(i32X2 > pContext->sClipRegion.i16XMax)
            {
                i32X2 = pContext->sClipRegion.i16XMax;
            }

            //
            // Draw a horizontal line if this portion of the circle is within
            // the clipping region.
            //
            if(i32X1 <= i32X2)
            {
                GrLineDrawH(pContext, i32X1, i32X2, i32Y1);
            }
        }

        //
        // Determine the row when adding the A delta.
        //
        i32Y1 = i32Y + i32A;

        //
        // See if this row is within the clipping region, and the A delta is
        // not zero (otherwise, this describes the same row of the circle).
        //
        if((i32Y1 >= pContext->sClipRegion.i16YMin) &&
           (i32Y1 <= pContext->sClipRegion.i16YMax) &&
           (i32A != 0))
        {
            //
            // Determine the column when subtracting the B delta, and move it
            // to the left edge of the clipping region if it is to the left of
            // the clipping region.
            //
            i32X1 = i32X - i32B;
            if(i32X1 < pContext->sClipRegion.i16XMin)
            {
                i32X1 = pContext->sClipRegion.i16XMin;
            }

            //
            // Determine the column when adding the B delta, and move it to the
            // right edge of the clipping region if it is to the right of the
            // clipping region.
            //
            i32X2 = i32X + i32B;
            if(i32X2 > pContext->sClipRegion.i16XMax)
            {
                i32X2 = pContext->sClipRegion.i16XMax;
            }

            //
            // Draw a horizontal line if this portion of the circle is within
            // the clipping region.
            //
            if(i32X1 <= i32X2)
            {
                GrLineDrawH(pContext, i32X1, i32X2, i32Y1);
            }
        }

        //
        // Only draw the complementary lines if the B delta is about to change
        // and the A and B delta are different (otherwise, they describe the
        // same set of pixels).
        //
        if((i32D >= 0) && (i32A != i32B))
        {
            //
            // Determine the row when subtracting the B delta.
            //
            i32Y1 = i32Y - i32B;

            //
            // See if this row is within the clipping region.
            //
            if((i32Y1 >= pContext->sClipRegion.i16YMin) &&
               (i32Y1 <= pContext->sClipRegion.i16YMax))
            {
                //
                // Determine the column when subtracting the A delta, and move
                // it to the left edge of the clipping regino if it is to the
                // left of the clipping region.
                //
                i32X1 = i32X - i32A;
                if(i32X1 < pContext->sClipRegion.i16XMin)
                {
                    i32X1 = pContext->sClipRegion.i16XMin;
                }

                //
                // Determine the column when adding the A delta, and move it to
                // the right edge of the clipping region if it is to the right
                // of the clipping region.
                //
                i32X2 = i32X + i32A;
                if(i32X2 > pContext->sClipRegion.i16XMax)
                {
                    i32X2 = pContext->sClipRegion.i16XMax;
                }

                //
                // Draw a horizontal line if this portion of the circle is
                // within the clipping region.
                //
                if(i32X1 <= i32X2)
                {
                    GrLineDrawH(pContext, i32X1, i32X2, i32Y1);
                }
            }

            //
            // Determine the row when adding the B delta.
            //
            i32Y1 = i32Y + i32B;

            //
            // See if this row is within the clipping region.
            //
            if((i32Y1 >= pContext->sClipRegion.i16YMin) &&
               (i32Y1 <= pContext->sClipRegion.i16YMax))
            {
                //
                // Determine the column when subtracting the A delta, and move
                // it to the left edge of the clipping region if it is to the
                // left of the clipping region.
                //
                i32X1 = i32X - i32A;
                if(i32X1 < pContext->sClipRegion.i16XMin)
                {
                    i32X1 = pContext->sClipRegion.i16XMin;
                }

                //
                // Determine the column when adding the A delta, and move it to
                // the right edge of the clipping region if it is to the right
                // of the clipping region.
                //
                i32X2 = i32X + i32A;
                if(i32X2 > pContext->sClipRegion.i16XMax)
                {
                    i32X2 = pContext->sClipRegion.i16XMax;
                }

                //
                // Draw a horizontal line if this portion of the circle is
                // within the clipping region.
                //
                if(i32X1 <= i32X2)
                {
                    GrLineDrawH(pContext, i32X1, i32X2, i32Y1);
                }
            }
        }

        //
        // See if the error term is negative.
        //
        if(i32D < 0)
        {
            //
            // Since the error term is negative, adjust it based on a move in
            // only the A delta.
            //
            i32D += (4 * i32A) + 6;
        }
        else
        {
            //
            // Since the error term is non-negative, adjust it based on a move
            // in both the A and B deltas.
            //
            i32D += (4 * (i32A - i32B)) + 10;

            //
            // Decrement the B delta.
            //
            i32B -= 1;
        }

        //
        // Increment the A delta.
        //
        i32A++;
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
