//*****************************************************************************
//
// random.c - Random number generator utilizing MD4 hash function of
//            environmental noise captured as the seed and a linear congruence
//            generator for the random numbers.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C129X Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "random.h"

//*****************************************************************************
//
// The pool of entropy that has been collected.
//
//*****************************************************************************
static uint32_t g_pui32RandomEntropy[16];

//*****************************************************************************
//
// The index of the next byte to be added to the entropy pool.
//
//*****************************************************************************
static uint32_t g_ui32RandomIndex = 0;

//*****************************************************************************
//
// The random number seed, which corresponds to the most recently returned
// random number.  This is set based on the entropy-generated random number
// by RandomSeed().
//
//*****************************************************************************
static uint32_t g_ui32RandomSeed = 0;

//*****************************************************************************
//
// Add entropy to the pool.
//
//*****************************************************************************
void
RandomAddEntropy(uint32_t ui32Entropy)
{
    //
    // Add this byte to the entropy pool.
    //
    ((uint8_t *)g_pui32RandomEntropy)[g_ui32RandomIndex] = ui32Entropy & 0xff;

    //
    // Increment to the next byte of the entropy pool.
    //
    g_ui32RandomIndex = (g_ui32RandomIndex + 1) & 63;
}

//*****************************************************************************
//
// Seed the random number generator by running a MD4 hash on the entropy pool.
// Note that the entropy pool may change from beneath us, but for the purposes
// of generating random numbers that is not a concern.  Also, the MD4 hash was
// broken long ago, but since it is being used to generate random numbers
// instead of providing security this is not a concern.
//
//*****************************************************************************
void
RandomSeed(void)
{
    uint32_t ui32A, ui32B, ui32C, ui32D, ui32Temp, ui32Idx;

    //
    // Initialize the digest.
    //
    ui32A = 0x67452301;
    ui32B = 0xefcdab89;
    ui32C = 0x98badcfe;
    ui32D = 0x10325476;

    //
    // Perform the first round of operations.
    //
#define F(a, b, c, d, k, s)                                                  \
    {                                                                        \
        ui32Temp = a + (d ^ (b & (c ^ d))) + g_pui32RandomEntropy[k];        \
        a = (ui32Temp << s) | (ui32Temp >> (32 - s));                        \
    }
    for(ui32Idx = 0; ui32Idx < 16; ui32Idx += 4)
    {
        F(ui32A, ui32B, ui32C, ui32D, ui32Idx + 0, 3);
        F(ui32D, ui32A, ui32B, ui32C, ui32Idx + 1, 7);
        F(ui32C, ui32D, ui32A, ui32B, ui32Idx + 2, 11);
        F(ui32B, ui32C, ui32D, ui32A, ui32Idx + 3, 19);
    }

    //
    // Perform the second round of operations.
    //
#define G(a, b, c, d, k, s)                                                  \
    {                                                                        \
        ui32Temp = a + ((b & c) | (b & d) | (c & d)) +                       \
                   g_pui32RandomEntropy[k] +   0x5a827999;                   \
        a = (ui32Temp << s) | (ui32Temp >> (32 - s));                        \
    }
    for(ui32Idx = 0; ui32Idx < 4; ui32Idx++)
    {
        G(ui32A, ui32B, ui32C, ui32D, ui32Idx + 0, 3);
        G(ui32D, ui32A, ui32B, ui32C, ui32Idx + 4, 5);
        G(ui32C, ui32D, ui32A, ui32B, ui32Idx + 8, 9);
        G(ui32B, ui32C, ui32D, ui32A, ui32Idx + 12, 13);
    }

    //
    // Perform the third round of operations.
    //
#define H(a, b, c, d, k, s)                                                  \
    {                                                                        \
        ui32Temp = a + (b ^ c ^ d) + g_pui32RandomEntropy[k] + 0x6ed9eba1;   \
        a = (ui32Temp << s) | (ui32Temp >> (32 - s));                        \
    }
    for(ui32Idx = 0; ui32Idx < 4; ui32Idx += 2)
    {
        H(ui32A, ui32B, ui32C, ui32D, ui32Idx + 0, 3);
        H(ui32D, ui32A, ui32B, ui32C, ui32Idx + 8, 9);
        H(ui32C, ui32D, ui32A, ui32B, ui32Idx + 4, 11);
        H(ui32B, ui32C, ui32D, ui32A, ui32Idx + 12, 15);

        if(ui32Idx == 2)
        {
            ui32Idx -= 3;
        }
    }

    //
    // Use the first word of the resulting digest as the random number seed.
    //
    g_ui32RandomSeed = ui32A + 0x67452301;
}

//*****************************************************************************
//
// Generate a new random number.  The number returned would more accruately be
// described as a pseudo-random number since a linear congruence generator is
// being used.
//
//*****************************************************************************
uint32_t
RandomNumber(void)
{
    //
    // Generate a new pseudo-random number with a linear congruence random
    // number generator.  This new random number becomes the seed for the next
    // random number.
    //
    g_ui32RandomSeed = (g_ui32RandomSeed * 1664525) + 1013904223;

    //
    // Return the new random number.
    //
    return(g_ui32RandomSeed);
}
