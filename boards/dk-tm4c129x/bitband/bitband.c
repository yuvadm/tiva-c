//*****************************************************************************
//
// bitband.c - Bit-band manipulation example.
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
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "grlib/grlib.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Bit-Banding (bitband)</h1>
//!
//! This example application demonstrates the use of the bit-banding
//! capabilities of the Cortex-M4 microprocessor.  All of SRAM and all of the
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
// Graphics context used to show text on the display.
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
// seconds are guaranteed, along with the remainder of the current second).
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
// Print the given value as a hexadecimal string on the display.
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
                         GrContextDpyWidthGet(&g_sContext) / 2, 120, 1);
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
    uint32_t ui32SysClock;
    uint32_t ui32Errors, ui32Idx;

    //
    // Set the system clock to run at 16MHz from the PLL.
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 16000000);

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
    GrContextInit(&g_sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&g_sContext, "bitband");

    //
    // Set up and enable the SysTick timer.  It will be used as a reference
    // for delay loops.  The SysTick timer period will be set up for one
    // second.
    //
    ROM_SysTickPeriodSet(ui32SysClock);
    ROM_SysTickEnable();

    //
    // Set the value and error count to zero.
    //
    g_ui32Value = 0;
    ui32Errors = 0;

    //
    // Print the initial value to the display.
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
        // Print the current value to the display.
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
                             GrContextDpyWidthGet(&g_sContext) / 2, 180, 0);
    }
    else
    {
        GrStringDrawCentered(&g_sContext, "Success!", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2, 180, 0);
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
