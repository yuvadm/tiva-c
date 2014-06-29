//*****************************************************************************
//
// bitband.c - Bit-band manipulation example.
//
// Copyright (c) 2011-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "drivers/cfal96x64x16.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Bit-Banding (bitband)</h1>
//!
//! This example application demonstrates the use of the bit-banding
//! capabilities of the Cortex-M3 microprocessor.  All of SRAM and all of the
//! peripherals reside within bit-band regions, meaning that bit-banding
//! operations can be applied to any of them.  In this example, a variable in
//! SRAM is set to a particular value one bit at a time using bit-banding
//! operations (it would be more efficient to do a single non-bit-banded write;
//! this simply demonstrates the operation of bit-banding).
//
//*****************************************************************************

//*****************************************************************************
//
// A map of hex nibbles to ASCII characters.
//
//*****************************************************************************
static const char * const pcHex = "0123456789ABCDEF";

//*****************************************************************************
//
// The value that is to be modified via bit-banding.
//
//*****************************************************************************
static volatile uint32_t g_ui32Value;

//*****************************************************************************
//
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
tContext g_sContext;

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
// Delay for the specified number of seconds.  Depending upon the current
// SysTick value, the delay will be between N-1 and N seconds (i.e. N-1 full
// seconds are guaranteed, aint32_t with the remainder of the current second).
//
//*****************************************************************************
void
Delay(uint32_t ui32Seconds)
{
    //
    // Loop while there are more seconds to wait.
    //
    while(ui32Seconds--)
    {
        //
        // Wait until the SysTick value is less than 1000.
        //
        while(ROM_SysTickValueGet() > 1000)
        {
        }

        //
        // Wait until the SysTick value is greater than 1000.
        //
        while(ROM_SysTickValueGet() < 1000)
        {
        }
    }
}

//*****************************************************************************
//
// Print the given value as a hexadecimal string on the CSTN.
//
//*****************************************************************************
void
PrintValue(uint32_t ui32Value)
{
    char pcBuffer[9];

    pcBuffer[0] = pcHex[(ui32Value >> 28) & 15];
    pcBuffer[1] = pcHex[(ui32Value >> 24) & 15];
    pcBuffer[2] = pcHex[(ui32Value >> 20) & 15];
    pcBuffer[3] = pcHex[(ui32Value >> 16) & 15];
    pcBuffer[4] = pcHex[(ui32Value >> 12) & 15];
    pcBuffer[5] = pcHex[(ui32Value >> 8) & 15];
    pcBuffer[6] = pcHex[(ui32Value >> 4) & 15];
    pcBuffer[7] = pcHex[(ui32Value >> 0) & 15];
    pcBuffer[8] = '\0';

    GrStringDrawCentered(&g_sContext, pcBuffer, -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 28, 1);
}

//*****************************************************************************
//
// This example demonstrates the use of bit-banding to set individual bits
// within a word of SRAM.
//
//*****************************************************************************
int
main(void)
{
    tRectangle sRect;
    uint32_t ui32Errors, ui32Idx;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Initialize the display driver.
    //
    CFAL96x64x16Init();

    //
    // Initialize the graphics context and find the middle X coordinate.
    //
    GrContextInit(&g_sContext, &g_sCFAL96x64x16);

    //
    // Fill the top part of the screen with blue to create the banner.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMax = 9;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
    GrRectFill(&g_sContext, &sRect);

    //
    // Change foreground for white text.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);
    GrStringDrawCentered(&g_sContext, "bitband", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 4, 0);
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);

    //
    // Set up and enable the SysTick timer.  It will be used as a reference
    // for delay loops.  The SysTick timer period will be set up for one
    // second.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet());
    ROM_SysTickEnable();

    //
    // Set the value and error count to zero.
    //
    g_ui32Value = 0;
    ui32Errors = 0;

    //
    // Print the initial value to the CSTN.
    //
    PrintValue(g_ui32Value);

    //
    // Delay for 1 second.
    //
    Delay(1);

    //
    // Set the value to 0xdecafbad using bit band accesses to each individual
    // bit.
    //
    for(ui32Idx = 0; ui32Idx < 32; ui32Idx++)
    {
        //
        // Set this bit.
        //
        HWREGBITW(&g_ui32Value, 31 - ui32Idx) = 
                                            (0xdecafbad >> (31 - ui32Idx)) & 1;

        //
        // Print the current value to the CSTN.
        //
        PrintValue(g_ui32Value);

        //
        // Delay for 1 second.
        //
        Delay(1);
    }

    //
    // Make sure that the value is 0xdecafbad.
    //
    if(g_ui32Value != 0xdecafbad)
    {
        ui32Errors++;
    }

    //
    // Make sure that the individual bits read back correctly.
    //
    for(ui32Idx = 0; ui32Idx < 32; ui32Idx++)
    {
        if(HWREGBITW(&g_ui32Value, ui32Idx) != ((0xdecafbad >> ui32Idx) & 1))
        {
            ui32Errors++;
        }
    }

    //
    // Delay for 2 seconds.
    //
    Delay(2);

    //
    // Print out the result.
    //
    if(ui32Errors)
    {
        GrStringDrawCentered(&g_sContext, "Errors!", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2, 48, 0);
    }
    else
    {
        GrStringDrawCentered(&g_sContext, "Success!", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2, 48, 0);
    }

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
