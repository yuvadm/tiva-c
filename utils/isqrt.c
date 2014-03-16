//*****************************************************************************
//
// isqrt.c - Integer square root.
//
// Copyright (c) 2005-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the Tiva Utility Library.
//
//*****************************************************************************

#include <stdint.h>
#include "utils/isqrt.h"

//*****************************************************************************
//
//! \addtogroup isqrt_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Compute the integer square root of an integer.
//!
//! \param ui32Value is the value whose square root is desired.
//!
//! This function will compute the integer square root of the given input
//! value.  Since the value returned is also an integer, it is actually better
//! defined as the largest integer whose square is less than or equal to the
//! input value.
//!
//! \return Returns the square root of the input value.
//
//*****************************************************************************
uint32_t
isqrt(uint32_t ui32Value)
{
    uint32_t ui32Rem, ui32Root, ui32Idx;

    //
    // Initialize the remainder and root to zero.
    //
    ui32Rem = 0;
    ui32Root = 0;

    //
    // Loop over the sixteen bits in the root.
    //
    for(ui32Idx = 0; ui32Idx < 16; ui32Idx++)
    {
        //
        // Shift the root up by a bit to make room for the new bit that is
        // about to be computed.
        //
        ui32Root <<= 1;

        //
        // Get two more bits from the input into the remainder.
        //
        ui32Rem = ((ui32Rem << 2) + (ui32Value >> 30));
        ui32Value <<= 2;

        //
        // Make the test root be 2n + 1.
        //
        ui32Root++;

        //
        // See if the root is greater than the remainder.
        //
        if(ui32Root <= ui32Rem)
        {
            //
            // Subtract the test root from the remainder.
            //
            ui32Rem -= ui32Root;

            //
            // Increment the root, setting the second LSB.
            //
            ui32Root++;
        }
        else
        {
            //
            // The root is greater than the remainder, so the new bit of the
            // root is actually zero.
            //
            ui32Root--;
        }
    }

    //
    // Return the computed root.
    //
    return(ui32Root >> 1);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
