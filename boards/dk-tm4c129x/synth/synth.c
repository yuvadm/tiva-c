//*****************************************************************************
//
// synth.c - A single-octave synthesizer to demonstrate the use of the sound
//           driver.
//
// Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
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

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "utils/sine.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/sound.h"
#include "drivers/touch.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Synthesizer (synth)</h1>
//!
//! This application provides a single-octave synthesizer utilizing the touch
//! screen as a virtual piano keyboard.  The notes played on the virtual piano
//! are played out via the on-board speaker.
//
//*****************************************************************************

//*****************************************************************************
//
// The colors used to draw the white keys.
//
//*****************************************************************************
#define ClrWhiteKey             0xcfcfcf
#define ClrWhiteBright          0xffffff
#define ClrWhiteDim             0x9f9f9f

//*****************************************************************************
//
// The colors used to draw the black keys.
//
//*****************************************************************************
#define ClrBlackKey             0x000000
#define ClrBlackBright          0x606060
#define ClrBlackDim             0x303030

//*****************************************************************************
//
// The color used to draw a pressed key.
//
//*****************************************************************************
#define ClrPressed              0x3f3fbf

//*****************************************************************************
//
// The width and height of the white keys.  The width should be an even number.
//
//*****************************************************************************
#define WHITE_WIDTH             36
#define WHITE_HEIGHT            190

//*****************************************************************************
//
// The width and height of the black keys.  The width should be a multiple of
// four.
//
//*****************************************************************************
#define BLACK_WIDTH             26
#define BLACK_HEIGHT            110

//*****************************************************************************
//
// The screen offset of the upper left hand corner of the keyboard.
//
//*****************************************************************************
#define X_OFFSET                16
#define Y_OFFSET                32

//*****************************************************************************
//
// A structure that describes a key on the keyboard.
//
//*****************************************************************************
typedef struct
{
    //
    // The outline of the key.
    //
    tRectangle sOutline;

    //
    // The first/top fill for the key.
    //
    tRectangle sFill1;

    //
    // The second/bottom fill for the key (not used for black keys).
    //
    tRectangle sFill2;

    //
    // The frequency of the note produced by this key.
    //
    uint32_t ui32Freq;
}
tKey;

//*****************************************************************************
//
// The white keys on the keyboard.
//
//*****************************************************************************
static const tKey g_psWhiteKeys[] =
{
    //
    // C4
    //
    {
        //
        // Outline
        //
        {
            X_OFFSET,
            Y_OFFSET,
            X_OFFSET + WHITE_WIDTH - 1,
            Y_OFFSET + WHITE_HEIGHT - 1
        },

        //
        // Top fill
        //
        {
            X_OFFSET + 2,
            Y_OFFSET + 2,
            X_OFFSET + WHITE_WIDTH - ((BLACK_WIDTH * 3) / 4) - 1,
            Y_OFFSET + BLACK_HEIGHT - 1
        },

        //
        // Bottom fill
        //
        {
            X_OFFSET + 2,
            Y_OFFSET + BLACK_HEIGHT,
            X_OFFSET + WHITE_WIDTH - 3,
            Y_OFFSET + WHITE_HEIGHT - 3
        },

        //
        // Frequency
        //
        261
    },

    //
    // D4
    //
    {
        //
        // Outline
        //
        {
            X_OFFSET + WHITE_WIDTH,
            Y_OFFSET,
            X_OFFSET + (WHITE_WIDTH * 2) - 1,
            Y_OFFSET + WHITE_HEIGHT - 1
        },

        //
        // Top fill
        //
        {
            X_OFFSET + WHITE_WIDTH + (BLACK_WIDTH / 4),
            Y_OFFSET + 2,
            X_OFFSET + (WHITE_WIDTH * 2) - (BLACK_WIDTH / 4) - 1,
            Y_OFFSET + BLACK_HEIGHT - 1
        },

        //
        // Bottom fill
        //
        {
            X_OFFSET + WHITE_WIDTH + 2,
            Y_OFFSET + BLACK_HEIGHT,
            X_OFFSET + (WHITE_WIDTH * 2) - 3,
            Y_OFFSET + WHITE_HEIGHT - 3
        },

        //
        // Frequency
        //
        294
    },

    //
    // E4
    //
    {
        //
        // Outline
        //
        {
            X_OFFSET + (WHITE_WIDTH * 2),
            Y_OFFSET,
            X_OFFSET + (WHITE_WIDTH * 3) - 1,
            Y_OFFSET + WHITE_HEIGHT - 1
        },

        //
        // Top fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 2) + ((BLACK_WIDTH * 3) / 4),
            Y_OFFSET + 2,
            X_OFFSET + (WHITE_WIDTH * 3) - 3,
            Y_OFFSET + BLACK_HEIGHT - 1,
        },

        //
        // Bottom fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 2) + 2,
            Y_OFFSET + BLACK_HEIGHT,
            X_OFFSET + (WHITE_WIDTH * 3) - 3,
            Y_OFFSET + WHITE_HEIGHT - 3
        },

        //
        // Frequency
        //
        330
    },

    //
    // F4
    //
    {
        //
        // Outline
        //
        {
            X_OFFSET + (WHITE_WIDTH * 3),
            Y_OFFSET,
            X_OFFSET + (WHITE_WIDTH * 4) - 1,
            Y_OFFSET + WHITE_HEIGHT - 1
        },

        //
        // Top fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 3) + 2,
            Y_OFFSET + 2,
            X_OFFSET + (WHITE_WIDTH * 4) - ((BLACK_WIDTH * 3) / 4) - 1,
            Y_OFFSET + BLACK_HEIGHT - 1
        },

        //
        // Bottom fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 3) + 2,
            Y_OFFSET + BLACK_HEIGHT,
            X_OFFSET + (WHITE_WIDTH * 4) - 3,
            Y_OFFSET + WHITE_HEIGHT - 3
        },

        //
        // Frequency
        //
        349
    },

    //
    // G4
    //
    {
        //
        // Outline
        //
        {
            X_OFFSET + (WHITE_WIDTH * 4),
            Y_OFFSET,
            X_OFFSET + (WHITE_WIDTH * 5) - 1,
            Y_OFFSET + WHITE_HEIGHT - 1
        },

        //
        // Top fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 4) + (BLACK_WIDTH / 4),
            Y_OFFSET + 2,
            X_OFFSET + (WHITE_WIDTH * 5) - (BLACK_WIDTH / 2) - 1,
            Y_OFFSET + BLACK_HEIGHT - 1
        },

        //
        // Bottom fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 4) + 2,
            Y_OFFSET + BLACK_HEIGHT,
            X_OFFSET + (WHITE_WIDTH * 5) - 3,
            Y_OFFSET + WHITE_HEIGHT - 3
        },

        //
        // Frequency
        //
        392
    },

    //
    // A4
    //
    {
        //
        // Outline
        //
        {
            X_OFFSET + (WHITE_WIDTH * 5),
            Y_OFFSET,
            X_OFFSET + (WHITE_WIDTH * 6) - 1,
            Y_OFFSET + WHITE_HEIGHT - 1
        },

        //
        // Top fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 5) + (BLACK_WIDTH / 2),
            Y_OFFSET + 2,
            X_OFFSET + (WHITE_WIDTH * 6) - (BLACK_WIDTH / 4) - 1,
            Y_OFFSET + BLACK_HEIGHT - 1
        },

        //
        // Bottom fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 5) + 2,
            Y_OFFSET + BLACK_HEIGHT,
            X_OFFSET + (WHITE_WIDTH * 6) - 3,
            Y_OFFSET + WHITE_HEIGHT - 3
        },

        //
        // Frequency
        //
        440
    },

    //
    // B4
    //
    {
        //
        // Outline
        //
        {
            X_OFFSET + (WHITE_WIDTH * 6),
            Y_OFFSET,
            X_OFFSET + (WHITE_WIDTH * 7) - 1,
            Y_OFFSET + WHITE_HEIGHT - 1
        },

        //
        // Top fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 6) + ((BLACK_WIDTH * 3) / 4),
            Y_OFFSET + 2,
            X_OFFSET + (WHITE_WIDTH * 7) - 3,
            Y_OFFSET + BLACK_HEIGHT - 1
        },

        //
        // Bottom fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 6) + 2,
            Y_OFFSET + BLACK_HEIGHT,
            X_OFFSET + (WHITE_WIDTH * 7) - 3,
            Y_OFFSET + WHITE_HEIGHT - 3
        },

        //
        // Frequency
        //
        494
    },

    //
    // C5
    //
    {
        //
        // Outline
        //
        {
            X_OFFSET + (WHITE_WIDTH * 7),
            Y_OFFSET,
            X_OFFSET + (WHITE_WIDTH * 8) - 1,
            Y_OFFSET + WHITE_HEIGHT - 1
        },

        //
        // Top fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 7) + 2,
            Y_OFFSET + 2,
            X_OFFSET + (WHITE_WIDTH * 8) - 3,
            Y_OFFSET + BLACK_HEIGHT - 1
        },

        //
        // Bottom fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 7) + 2,
            Y_OFFSET + BLACK_HEIGHT,
            X_OFFSET + (WHITE_WIDTH * 8) - 3,
            Y_OFFSET + WHITE_HEIGHT - 3
        },

        //
        // Frequency
        //
        523
    }
};

//*****************************************************************************
//
// The number of white keys.
//
//*****************************************************************************
#define NUM_WHITE_KEYS          (sizeof(g_psWhiteKeys) /                      \
                                 sizeof(g_psWhiteKeys[0]))

//*****************************************************************************
//
// The black keys on the keyboard.
//
//*****************************************************************************
static const tKey g_psBlackKeys[] =
{
    //
    // C#4
    //
    {
        //
        // Outline
        //
        {
            X_OFFSET + WHITE_WIDTH - ((BLACK_WIDTH * 3) / 4),
            Y_OFFSET,
            X_OFFSET + WHITE_WIDTH + (BLACK_WIDTH / 4) - 1,
            Y_OFFSET + BLACK_HEIGHT - 1
        },

        //
        // Fill
        //
        {
            X_OFFSET + WHITE_WIDTH - ((BLACK_WIDTH * 3) / 4) + 2,
            Y_OFFSET + 2,
            X_OFFSET + WHITE_WIDTH + (BLACK_WIDTH / 4) - 3,
            Y_OFFSET + BLACK_HEIGHT - 3
        },

        //
        // Unused
        //
        {
            0
        },

        //
        // Frequency
        //
        277
    },

    //
    // D#4
    //
    {
        //
        // Outline
        //
        {
            X_OFFSET + (WHITE_WIDTH * 2) - (BLACK_WIDTH / 4),
            Y_OFFSET,
            X_OFFSET + (WHITE_WIDTH * 2) + ((BLACK_WIDTH * 3) / 4) - 1,
            Y_OFFSET + BLACK_HEIGHT - 1
        },

        //
        // Fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 2) - (BLACK_WIDTH / 4) + 2,
            Y_OFFSET + 2,
            X_OFFSET + (WHITE_WIDTH * 2) + ((BLACK_WIDTH * 3) / 4) - 3,
            Y_OFFSET + BLACK_HEIGHT - 3
        },

        //
        // Unused
        //
        {
            0
        },

        //
        // Frequency
        //
        311
    },

    //
    // F#4
    //
    {
        //
        // Outline
        //
        {
            X_OFFSET + (WHITE_WIDTH * 4) - ((BLACK_WIDTH * 3) / 4),
            Y_OFFSET,
            X_OFFSET + (WHITE_WIDTH * 4) + (BLACK_WIDTH / 4) - 1,
            Y_OFFSET + BLACK_HEIGHT - 1
        },

        //
        // Fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 4) - ((BLACK_WIDTH * 3) / 4) + 2,
            Y_OFFSET + 2,
            X_OFFSET + (WHITE_WIDTH * 4) + (BLACK_WIDTH / 4) - 3,
            Y_OFFSET + BLACK_HEIGHT - 3
        },

        //
        // Unused
        //
        {
            0
        },

        //
        // Frequency
        //
        370
    },

    //
    // G#4
    //
    {
        //
        // Outline
        //
        {
            X_OFFSET + (WHITE_WIDTH * 5) - (BLACK_WIDTH / 2),
            Y_OFFSET,
            X_OFFSET + (WHITE_WIDTH * 5) + (BLACK_WIDTH / 2) - 1,
            Y_OFFSET + BLACK_HEIGHT - 1
        },

        //
        // Fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 5) - (BLACK_WIDTH / 2) + 2,
            Y_OFFSET + 2,
            X_OFFSET + (WHITE_WIDTH * 5) + (BLACK_WIDTH / 2) - 3,
            Y_OFFSET + BLACK_HEIGHT - 3
        },

        //
        // Unused
        //
        {
            0
        },

        //
        // Frequency
        //
        415
    },

    //
    // A#4
    //
    {
        //
        // Outline
        //
        {
            X_OFFSET + (WHITE_WIDTH * 6) - (BLACK_WIDTH / 4),
            Y_OFFSET,
            X_OFFSET + (WHITE_WIDTH * 6) + ((BLACK_WIDTH * 3) / 4) - 1,
            Y_OFFSET + BLACK_HEIGHT - 1
        },

        //
        // Fill
        //
        {
            X_OFFSET + (WHITE_WIDTH * 6) - (BLACK_WIDTH / 4) + 2,
            Y_OFFSET + 2,
            X_OFFSET + (WHITE_WIDTH * 6) + ((BLACK_WIDTH * 3) / 4) - 3,
            Y_OFFSET + BLACK_HEIGHT - 3
        },

        //
        // Unused
        //
        {
            0
        },

        //
        // Frequency
        //
        466
    }
};

//*****************************************************************************
//
// The number of black keys.
//
//*****************************************************************************
#define NUM_BLACK_KEYS          (sizeof(g_psBlackKeys) /                      \
                                 sizeof(g_psBlackKeys[0]))

//*****************************************************************************
//
// The buffer used to store the synthesized waveform that is to be played.  The
// buffer size must be a power of 2 less than or equal to 2048.
//
//*****************************************************************************
#define AUDIO_SIZE              2048
static int16_t g_pi16AudioBuffer[AUDIO_SIZE];

//*****************************************************************************
//
// A set of flags that indicate the current state of the application.
//
//*****************************************************************************
static uint32_t g_ui32Flags;
#define FLAG_PING               0           // The "ping" half of the sound
                                            // buffer needs to be filled
#define FLAG_PONG               1           // The "pong" half of the sound
                                            // buffer needs to be filled

//*****************************************************************************
//
// The key that is currently pressed.
//
//*****************************************************************************
uint32_t g_ui32Key = NUM_WHITE_KEYS + NUM_BLACK_KEYS;

//*****************************************************************************
//
// The position within the waveform of the currently playing key.
//
//*****************************************************************************
uint32_t g_ui32AudioPos;

//*****************************************************************************
//
// The step rate of the waveform for the currently playing key.
//
//*****************************************************************************
uint32_t g_ui32AudioStep;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Fills in one of the white keys with the given color.
//
//*****************************************************************************
static inline void
FillWhiteKey(tContext *pContext, uint32_t ui32Key, uint32_t ui32Color)
{
    //
    // Select the specified color.
    //
    GrContextForegroundSet(pContext, ui32Color);

    //
    // Fill in the upper and lower portions of the white key.
    //
    GrRectFill(pContext, &(g_psWhiteKeys[ui32Key].sFill1));
    GrRectFill(pContext, &(g_psWhiteKeys[ui32Key].sFill2));
}

//*****************************************************************************
//
// Draws the white keys on the display.
//
//*****************************************************************************
static inline void
DrawWhiteKeys(tContext *pContext)
{
    uint32_t ui32Key;

    //
    // Loop through the white keys.
    //
    for(ui32Key = 0; ui32Key < NUM_WHITE_KEYS; ui32Key++)
    {
        //
        // Select the color for the top and left edges of the white key.
        //
        GrContextForegroundSet(pContext, ClrWhiteBright);

        //
        // Draw the top and left edges of the white key.
        //
        GrLineDraw(pContext,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMin,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMin,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMax,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMin);
        GrLineDraw(pContext,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMin + 1,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMin + 1,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMax - 1,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMin + 1);
        GrLineDraw(pContext,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMin,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMin + 1,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMin,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMax);
        GrLineDraw(pContext,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMin + 1,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMin + 2,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMin + 1,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMax - 1);

        //
        // Select the color for the bottom and right edges of the white key.
        //
        GrContextForegroundSet(pContext, ClrWhiteDim);

        //
        // Draw the bottom and right edges of the white key.
        //
        GrLineDraw(pContext,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMax,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMin + 1,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMax,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMax);
        GrLineDraw(pContext,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMax - 1,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMin + 2,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMax - 1,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMax - 1);
        GrLineDraw(pContext,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMin + 1,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMax,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMax - 1,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMax);
        GrLineDraw(pContext,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMin + 2,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMax - 1,
                   g_psWhiteKeys[ui32Key].sOutline.i16XMax - 2,
                   g_psWhiteKeys[ui32Key].sOutline.i16YMax - 1);

        //
        // Fill in the white key with the default color.
        //
        FillWhiteKey(pContext, ui32Key, ClrWhiteKey);
    }
}

//*****************************************************************************
//
// Fills in one of the black keys with the given color.
//
//*****************************************************************************
static inline void
FillBlackKey(tContext *pContext, uint32_t ui32Key, uint32_t ui32Color)
{
    //
    // Select the specified color.
    //
    GrContextForegroundSet(pContext, ui32Color);

    //
    // FIll in the black key.
    //
    GrRectFill(pContext, &(g_psBlackKeys[ui32Key].sFill1));
}

//*****************************************************************************
//
// Draws the black keys on the display.
//
//*****************************************************************************
static inline void
DrawBlackKeys(tContext *pContext)
{
    uint32_t ui32Key;

    //
    // Loop through the black keys.
    //
    for(ui32Key = 0; ui32Key < NUM_BLACK_KEYS; ui32Key++)
    {
        //
        // Select the color for the top and left edges of the black key.
        //
        GrContextForegroundSet(pContext, ClrBlackBright);

        //
        // Draw the top and left edges of the black key.
        //
        GrLineDraw(pContext,
                   g_psBlackKeys[ui32Key].sOutline.i16XMin,
                   g_psBlackKeys[ui32Key].sOutline.i16YMin,
                   g_psBlackKeys[ui32Key].sOutline.i16XMax,
                   g_psBlackKeys[ui32Key].sOutline.i16YMin);
        GrLineDraw(pContext,
                   g_psBlackKeys[ui32Key].sOutline.i16XMin + 1,
                   g_psBlackKeys[ui32Key].sOutline.i16YMin + 1,
                   g_psBlackKeys[ui32Key].sOutline.i16XMax - 1,
                   g_psBlackKeys[ui32Key].sOutline.i16YMin + 1);
        GrLineDraw(pContext,
                   g_psBlackKeys[ui32Key].sOutline.i16XMin,
                   g_psBlackKeys[ui32Key].sOutline.i16YMin + 1,
                   g_psBlackKeys[ui32Key].sOutline.i16XMin,
                   g_psBlackKeys[ui32Key].sOutline.i16YMax);
        GrLineDraw(pContext,
                   g_psBlackKeys[ui32Key].sOutline.i16XMin + 1,
                   g_psBlackKeys[ui32Key].sOutline.i16YMin + 2,
                   g_psBlackKeys[ui32Key].sOutline.i16XMin + 1,
                   g_psBlackKeys[ui32Key].sOutline.i16YMax - 1);

        //
        // Select the color for the bottom and right edges of the black key.
        //
        GrContextForegroundSet(pContext, ClrBlackDim);

        //
        // Draw the bottom and right edges of the black key.
        //
        GrLineDraw(pContext,
                   g_psBlackKeys[ui32Key].sOutline.i16XMax,
                   g_psBlackKeys[ui32Key].sOutline.i16YMin + 1,
                   g_psBlackKeys[ui32Key].sOutline.i16XMax,
                   g_psBlackKeys[ui32Key].sOutline.i16YMax);
        GrLineDraw(pContext,
                   g_psBlackKeys[ui32Key].sOutline.i16XMax - 1,
                   g_psBlackKeys[ui32Key].sOutline.i16YMin + 2,
                   g_psBlackKeys[ui32Key].sOutline.i16XMax - 1,
                   g_psBlackKeys[ui32Key].sOutline.i16YMax - 1);
        GrLineDraw(pContext,
                   g_psBlackKeys[ui32Key].sOutline.i16XMin + 1,
                   g_psBlackKeys[ui32Key].sOutline.i16YMax,
                   g_psBlackKeys[ui32Key].sOutline.i16XMax - 1,
                   g_psBlackKeys[ui32Key].sOutline.i16YMax);
        GrLineDraw(pContext,
                   g_psBlackKeys[ui32Key].sOutline.i16XMin + 2,
                   g_psBlackKeys[ui32Key].sOutline.i16YMax - 1,
                   g_psBlackKeys[ui32Key].sOutline.i16XMax - 2,
                   g_psBlackKeys[ui32Key].sOutline.i16YMax - 1);

        //
        // Fill in the black key with the default color.
        //
        FillBlackKey(pContext, ui32Key, ClrBlackKey);
    }
}

//*****************************************************************************
//
// The callback function that is called by the sound driver to indicate that
// half of the sound buffer has been played.
//
//*****************************************************************************
void
SoundCallback(uint32_t ui32Half)
{
    //
    // See which half of the sound buffer has been played.
    //
    if(ui32Half == 0)
    {
        //
        // The first half of the sound buffer needs to be filled.
        //
        HWREGBITW(&g_ui32Flags, FLAG_PING) = 1;
    }
    else
    {
        //
        // The second half of the sound buffer needs to be filled.
        //
        HWREGBITW(&g_ui32Flags, FLAG_PONG) = 1;
    }
}

//*****************************************************************************
//
// The callback function that is called by the touch screen driver to indicate
// activity on the touch screen.
//
//*****************************************************************************
int32_t
TouchCallback(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    uint32_t ui32Key;

    //
    // See if this touch event occurred on one of the black keys.
    //
    for(ui32Key = 0; ui32Key < NUM_BLACK_KEYS; ui32Key++)
    {
        if((i32X >= g_psBlackKeys[ui32Key].sOutline.i16XMin) &&
           (i32X <= g_psBlackKeys[ui32Key].sOutline.i16XMax) &&
           (i32Y >= g_psBlackKeys[ui32Key].sOutline.i16YMin) &&
           (i32Y <= g_psBlackKeys[ui32Key].sOutline.i16YMax))
        {
            break;
        }
    }

    //
    // See if a match was found.
    //
    if(ui32Key != NUM_BLACK_KEYS)
    {
        //
        // The touch event occurred on one of the black keys.  Increment the
        // index by the number of white keys since they are listed first.
        //
        ui32Key += NUM_WHITE_KEYS;
    }
    else
    {
        //
        // The touch event did not occur on one of the black keys.  Check the
        // white keys.
        //
        for(ui32Key = 0; ui32Key < NUM_WHITE_KEYS; ui32Key++)
        {
            if((i32X >= g_psWhiteKeys[ui32Key].sOutline.i16XMin) &&
               (i32X <= g_psWhiteKeys[ui32Key].sOutline.i16XMax) &&
               (i32Y >= g_psWhiteKeys[ui32Key].sOutline.i16YMin) &&
               (i32Y <= g_psWhiteKeys[ui32Key].sOutline.i16YMax))
            {
                break;
            }
        }

        //
        // If the touch event did not occur on one of the white keys, set the
        // key number to a non-existant key.
        //
        if(ui32Key == NUM_WHITE_KEYS)
        {
            ui32Key += NUM_BLACK_KEYS;
        }
    }

    //
    // Determine the message that is being sent.
    //
    switch(ui32Message)
    {
        //
        // The user has just touched the screen.
        //
        case WIDGET_MSG_PTR_DOWN:
        {
            //
            // Save this key as the currently pressed key.
            //
            g_ui32Key = ui32Key;

            //
            // Done handling this message.
            //
            break;
        }

        //
        // The user has moved the touch location on the screen.
        //
        case WIDGET_MSG_PTR_MOVE:
        {
            //
            // Save this key as the currently pressed key.
            //
            g_ui32Key = ui32Key;

            //
            // Done handling this message.
            //
            break;
        }

        //
        // The user is no longer touching the screen.
        //
        case WIDGET_MSG_PTR_UP:
        {
            //
            // Indicate that no key is being pressed.
            //
            g_ui32Key = NUM_WHITE_KEYS + NUM_BLACK_KEYS;

            //
            // Done handling this message.
            //
            break;
        }
    }

    //
    // Success.
    //
    return(0);
}

//*****************************************************************************
//
// Generates an additional section of the audio output based on the currently
// pressed key (if any).
//
//*****************************************************************************
uint32_t
GenerateAudio(int16_t *pi16Buffer, uint32_t ui32Count)
{
    int32_t i32Val, i32Vol, i32VolStep;
    uint32_t ui32Key, ui32NewStep;

    //
    // See if one of the push buttons is pressed.
    //
    ui32Key = (ROM_GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_3) |
               ROM_GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_5));
    if(ui32Key == GPIO_PIN_5)
    {
        //
        // The up botton is pressed and the down button is not pressed.
        // Therefore, turn up the volume.
        //
        SoundVolumeUp(1);
    }
    if(ui32Key == GPIO_PIN_3)
    {
        //
        // The up botton is not pressed and the down button is pressed.
        // Therefore, turn down the volume.
        //
        SoundVolumeDown(1);
    }

    //
    // Get the currently pressed piano key.
    //
    ui32Key = g_ui32Key;

    //
    // See if this key is one of the white keys.
    //
    if(ui32Key < NUM_WHITE_KEYS)
    {
        //
        // Compute the step value required to generate this white key's
        // frequency.
        //
        ui32NewStep =
            ((g_psWhiteKeys[ui32Key].ui32Freq * 65536) / 64000) * 65536;
    }

    //
    // See if this key is one of the black keys.
    //
    else if(ui32Key < (NUM_WHITE_KEYS + NUM_BLACK_KEYS))
    {
        //
        // Compute the step value required to generate this black key's
        // frequency.
        //
        ui32NewStep =
            (((g_psBlackKeys[ui32Key - NUM_WHITE_KEYS].ui32Freq * 65536) /
              64000) * 65536);
    }

    //
    // No key is being pressed.
    //
    else
    {
        //
        // Do not generate any waveform.
        //
        ui32NewStep = 0;
    }

    //
    // See if no key was previously pressed and no key is currently pressed.
    //
    if((g_ui32AudioStep == 0) && (ui32NewStep == 0))
    {
        //
        // Fill the buffer with silence.
        //
        while(ui32Count--)
        {
            *pi16Buffer++ = 0;
        }

        //
        // There is nothing further to do.
        //
        return(ui32Key);
    }

    //
    // See if the same key as last time is pressed.
    //
    if(g_ui32AudioStep == ui32NewStep)
    {
        //
        // Set the volume of the waveform generator to full volume.
        //
        i32Vol = 1024;
        i32VolStep = 0;
    }

    //
    // See if a key was previously pressed.
    //
    else if(g_ui32AudioStep == 0)
    {
        //
        // There was not a previously pressed key, so ramp the volume of the
        // first waveform generator to full volume.
        //
        i32Vol = 0;
        i32VolStep = 1024 / ui32Count;

        //
        // Start the new waveform at zero.
        //
        g_ui32AudioPos = 0;
    }

    //
    // Otherwise there is already a key playing.
    //
    else
    {
        //
        // Ramp the volume of the waveform generator to zero.
        //
        i32Vol = 1024;
        i32VolStep = -1024 / ui32Count;
    }

    //
    // Loop through the samples to be generated.
    //
    while(ui32Count--)
    {
        //
        // Compute the value of the waveform.
        //
        i32Val = sine(g_ui32AudioPos + (sine(g_ui32AudioPos * 3) * 10922));

        //
        // Increment the position of the waveform.
        //
        g_ui32AudioPos += g_ui32AudioStep;

        //
        // Scale the waveform value by the volume.
        //
        i32Val = (i32Val * i32Vol) / 1024;

        //
        // Increment the waveform volume by the step.
        //
        i32Vol += i32VolStep;
        if(i32Vol < 0)
        {
            i32Vol = 0;
        }
        if(i32Vol > 1024)
        {
            i32Vol = 1024;
        }

        //
        // Cilp the waveform to min/max if required.
        //
        i32Val /= 2;
        if(i32Val > 32767)
        {
            i32Val = 32767;
        }
        if(i32Val < -32768)
        {
            i32Val = -32768;
        }

        //
        // Add the new waveform value to the sample buffer.
        //
        *pi16Buffer++ = (int16_t)i32Val;
    }

    //
    // Save the new step value.
    //
    g_ui32AudioStep = ui32NewStep;

    //
    // Return the currently pressed key.
    //
    return(ui32Key);
}

//*****************************************************************************
//
// This application performs simple audio synthesis and playback based on the
// keys pressed on the touch screen virtual piano keyboard.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32SysClock, ui32OldKey, ui32NewKey;
    tContext sContext;

    //
    // Run from the PLL at 120 MHz.
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(ui32SysClock);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&sContext, "synth");

    //
    // Draw the keys on the virtual piano keyboard.
    //
    DrawWhiteKeys(&sContext);
    DrawBlackKeys(&sContext);

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(ui32SysClock);
    TouchScreenCallbackSet(TouchCallback);

    //
    // Initialize the sound driver.
    //
    SoundInit(ui32SysClock);
    SoundVolumeSet(128);
    SoundStart(g_pi16AudioBuffer, AUDIO_SIZE, 64000, SoundCallback);

    //
    // Default the old and new key to not pressed so that the first key press
    // will be properly drawn on the keyboard.
    //
    ui32OldKey = NUM_WHITE_KEYS + NUM_BLACK_KEYS;
    ui32NewKey = NUM_WHITE_KEYS + NUM_BLACK_KEYS;

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // See if the first half of the sound buffer needs to be filled.
        //
        if(HWREGBITW(&g_ui32Flags, FLAG_PING) == 1)
        {
            //
            // Synthesize new audio into the first half of the sound buffer.
            //
            ui32NewKey = GenerateAudio(g_pi16AudioBuffer, AUDIO_SIZE / 2);

            //
            // Clear the flag for the first half of the sound buffer.
            //
            HWREGBITW(&g_ui32Flags, FLAG_PING) = 0;
        }

        //
        // See if the second half of the sound buffer needs to be filled.
        //
        if(HWREGBITW(&g_ui32Flags, FLAG_PONG) == 1)
        {
            //
            // Synthesize new audio into the second half of the sound buffer.
            //
            ui32NewKey = GenerateAudio(g_pi16AudioBuffer + (AUDIO_SIZE / 2),
                                       AUDIO_SIZE / 2);

            //
            // Clear the flag for the second half of the sound buffer.
            //
            HWREGBITW(&g_ui32Flags, FLAG_PONG) = 0;
        }

        //
        // See if a different key has been pressed.
        //
        if(ui32OldKey != ui32NewKey)
        {
            //
            // See if the old key was a white key.
            //
            if(ui32OldKey < NUM_WHITE_KEYS)
            {
                //
                // Redraw the face of the white key so that it no longer shows
                // as being pressed.
                //
                FillWhiteKey(&sContext, ui32OldKey, ClrWhiteKey);
            }

            //
            // See if the old key was a black key.
            //
            else if(ui32OldKey < (NUM_WHITE_KEYS + NUM_BLACK_KEYS))
            {
                //
                // Redraw the face of the black key so that it no longer shows
                // as being pressed.
                //
                FillBlackKey(&sContext, ui32OldKey - NUM_WHITE_KEYS,
                             ClrBlackKey);
            }

            //
            // See if the new key is a white key.
            //
            if(ui32NewKey < NUM_WHITE_KEYS)
            {
                //
                // Redraw the face of the white key so that it is shown as
                // being pressed.
                //
                FillWhiteKey(&sContext, ui32NewKey, ClrPressed);
            }

            //
            // See if the new key is a black key.
            //
            else if(ui32NewKey < (NUM_WHITE_KEYS + NUM_BLACK_KEYS))
            {
                //
                // Redraw the face of the black key so that it is shown as
                // being pressed.
                //
                FillBlackKey(&sContext, ui32NewKey - NUM_WHITE_KEYS,
                             ClrPressed);
            }

            //
            // Save the new key as the old key.
            //
            ui32OldKey = ui32NewKey;
        }
    }
}
