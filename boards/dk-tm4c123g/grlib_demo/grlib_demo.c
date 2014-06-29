//*****************************************************************************
//
// grlib_demo.c - Demonstration of the TivaWare Graphics Library.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C123G Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "drivers/cfal96x64x16.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Graphics Library Demonstration (grlib_demo)</h1>
//!
//! This application provides a demonstration of the capabilities of the
//! TivaWare Graphics Library.  The display will be configured to demonstrate
//! the available drawing primitives: lines, circles, rectangles, strings, and
//! images.
//
//*****************************************************************************

//*****************************************************************************
//
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// The image of the TI logo.
//
//*****************************************************************************
const uint8_t g_pui8Logo[] =
{
    IMAGE_FMT_4BPP_COMP,
    30, 0,
    30, 0,

    15,
    0x00, 0x00, 0x00,
    0x03, 0x02, 0x12,
    0x06, 0x05, 0x2b,
    0x0a, 0x08, 0x43,
    0x0d, 0x0a, 0x57,
    0x10, 0x0d, 0x69,
    0x12, 0x0e, 0x76,
    0x14, 0x10, 0x87,
    0x17, 0x12, 0x96,
    0x19, 0x14, 0xa6,
    0x1b, 0x15, 0xb1,
    0x1d, 0x17, 0xbe,
    0x1e, 0x18, 0xc8,
    0x21, 0x19, 0xd7,
    0x23, 0x1b, 0xe4,
    0x24, 0x1c, 0xed,

    0x84, 0x02, 0x79, 0x88, 0x8a, 0x50, 0x07, 0x00, 0x00, 0x08, 0xdf, 0xff,
    0xff, 0x80, 0x07, 0x00, 0x00, 0xbf, 0x90, 0x8a, 0x35, 0x30, 0x8f, 0xff,
    0xff, 0x70, 0x01, 0x31, 0xef, 0xa0, 0x8f, 0x89, 0x03, 0xff, 0x60, 0x17,
    0x90, 0x12, 0x33, 0x10, 0x17, 0xff, 0xff, 0xca, 0x13, 0x04, 0x98, 0x16,
    0xa9, 0x9a, 0x60, 0x16, 0xff, 0x18, 0x04, 0xfd, 0x1d, 0xff, 0xff, 0x90,
    0x16, 0xfc, 0x0b, 0x04, 0xf7, 0x2f, 0xff, 0xff, 0x80, 0x15, 0xfd, 0x84,
    0x08, 0x1e, 0xf5, 0x28, 0xbf, 0x8f, 0xf7, 0x00, 0x4f, 0x00, 0xf4, 0x00,
    0x6f, 0xff, 0x90, 0x00, 0x67, 0x66, 0x0a, 0x66, 0x66, 0xdf, 0xff, 0xa1,
    0xf2, 0x51, 0xe2, 0x00, 0x00, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf6,
    0x00, 0x30, 0x9f, 0xb0, 0x34, 0xef, 0xff, 0xfc, 0x20, 0x42, 0x0b, 0x8b,
    0xff, 0xd0, 0xbf, 0x71, 0x42, 0x80, 0x22, 0x01, 0xbf, 0x0b, 0x82, 0xef,
    0x42, 0x42, 0x70, 0x22, 0x00, 0x1b, 0x0b, 0x42, 0xff, 0x35, 0x8c, 0x02,
    0x89, 0x13, 0x25, 0xff, 0x1a, 0x14, 0x00, 0xaf, 0x09, 0x04, 0xfe, 0x24,
    0x86, 0x04, 0x8f, 0x09, 0x60, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xc5, 0x8f,
    0xfb, 0x00, 0x00, 0x00, 0x00, 0x2f, 0xff, 0xfd, 0x73, 0x10, 0x00, 0x00,
    0x04, 0x07, 0xfc, 0x10, 0x09, 0xfc, 0x89, 0x5f, 0xfe, 0x40, 0x51, 0x59,
    0x00, 0x00, 0x21, 0x00, 0x01, 0xef, 0x06, 0x72, 0x22, 0x21, 0x9f, 0x92,
    0x93, 0x6a, 0x7f, 0x08, 0xff, 0xee, 0xee, 0xfa, 0x97, 0x00, 0x2f, 0xff,
    0x12, 0xff, 0xff, 0xd1, 0x8f, 0x00, 0x08, 0x89, 0x50, 0x94, 0x17, 0x00,
    0x02, 0x11, 0x20, 0x17, 0x00, 0x00, 0x61, 0x4f, 0x8f, 0x03, 0x05, 0xff,
    0xff, 0x50, 0x17, 0x8c, 0x01, 0x3a, 0xdd, 0x60, 0x8f, 0x01, 0x04, 0x88,
    0x70, 0x40, 0x17, 0x47, 0x77
};

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
// A simple demonstration of the features of the TivaWare Graphics Library.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32Idx;
    tRectangle sRect;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();

    //
    // Set the clocking to run from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    //
    // Initialize the display driver.
    //
    CFAL96x64x16Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sCFAL96x64x16);

    //
    // Fill the top 12 rows of the screen with blue to create the banner.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMax = 11;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);
    GrStringDrawCentered(&g_sContext, "grlib_demo", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 5, 0);

    //
    // Draw a vertical sweep of lines from red to green.
    //
    for(ui32Idx = 0; ui32Idx <= 8; ui32Idx++)
    {
        GrContextForegroundSet(&g_sContext,
                               (((((10 - ui32Idx) * 255) / 8) << ClrRedShift) |
                                (((ui32Idx * 255) / 8) << ClrGreenShift)));
        GrLineDraw(&g_sContext, 60, 60, 0, 60 - (5 * ui32Idx));
    }

    //
    // Draw a horizontal sweep of lines from green to blue.
    //
    for(ui32Idx = 1; ui32Idx <= 11; ui32Idx++)
    {
        GrContextForegroundSet(&g_sContext,
                               (((((11 - ui32Idx) * 255) / 11) <<
                                 ClrGreenShift) |
                                (((ui32Idx * 255) / 11) << ClrBlueShift)));
        GrLineDraw(&g_sContext, 60, 60, (ui32Idx * 5), 20);
    }

    //
    // Draw a filled circle with an overlapping circle.
    //
    GrContextForegroundSet(&g_sContext, ClrBlue);
    GrCircleFill(&g_sContext, 80, 30, 15);
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrCircleDraw(&g_sContext, 80, 30, 15);

    //
    // Draw a filled rectangle with an overlapping rectangle.
    //
    GrContextForegroundSet(&g_sContext, ClrGray);
    sRect.i16XMin = 8;
    sRect.i16YMin = 45;
    sRect.i16XMax = 46;
    sRect.i16YMax = 51;
    GrRectFill(&g_sContext, &sRect);
    GrContextForegroundSet(&g_sContext, ClrWhite);
    sRect.i16XMin += 4;
    sRect.i16YMin += 4;
    sRect.i16XMax += 4;
    sRect.i16YMax += 4;
    GrRectDraw(&g_sContext, &sRect);

    //
    // Draw a piece of text in fonts of increasing size.
    //
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrStringDraw(&g_sContext, "Strings", -1, 6, 16, 0);
    GrContextForegroundSet(&g_sContext, ClrSilver);
    GrStringDraw(&g_sContext, "Strings", -1, 7, 17, 0);

    //
    // Draw an image.
    //
    GrTransparentImageDraw(&g_sContext, g_pui8Logo, 64, 34, ClrBlack);
#if 0
    GrImageDraw(&g_sContext, g_pui8Logo, 64, 34);
#endif

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&g_sContext);

    //
    // Loop forever.
    //
    while(1)
    {
    }
}
