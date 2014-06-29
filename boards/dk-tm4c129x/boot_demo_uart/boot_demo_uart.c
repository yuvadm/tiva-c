//*****************************************************************************
//
// boot_demo_uart.c - UART boot loader example.
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
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Boot Loader UART Demo (boot_demo_uart)</h1>
//!
//! This example demonstrates the use of the ROM-based UART boot loader.  When
//! the application runs, it will wait for the screen to be pressed (to
//! simulate an application waiting for a trigger to begin a firmware update)
//! and then branch to the UART boot loader in the ROM.  The UART is configured
//! for a baud rate of 115,200 and does not require the use of auto-bauding.
//
//*****************************************************************************

//*****************************************************************************
//
// A global we use to keep track of when the user presses the screen.
//
//*****************************************************************************
volatile bool g_bFirmwareUpdate = false;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// This function is called in interrupt context by the touch screen driver when
// there is a pointer event.
//
//*****************************************************************************
int32_t
TSHandler(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    //
    // See if this is a pointer up message.
    //
    if(ui32Message == WIDGET_MSG_PTR_UP)
    {
        //
        // The screen has been pressed and released, so begin the firmware
        // update.
        //
        g_bFirmwareUpdate = true;
    }

    //
    // Success.
    //
    return(0);
}

//*****************************************************************************
//
// A simple application demonstrating use of the boot loader,
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32SysClock;
    tContext sContext;
    tRectangle sRect;

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
    FrameDraw(&sContext, "boot-demo-uart");

    //
    // Print instructions on the screen.
    //
    GrStringDrawCentered(&sContext, "Press the screen to start", -1, 160, 108,
                         false);
    GrStringDrawCentered(&sContext, "the update process", -1, 160, 128, false);

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(ui32SysClock);

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(TSHandler);

    //
    // Enable the UART that will be used for the firmware update.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure the UART for 115200, 8-N-1.
    //
    ROM_UARTConfigSetExpClk(UART0_BASE, ui32SysClock, 115200,
                            (UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_WLEN_8));

    //
    // Enable the UART operation.
    //
    ROM_UARTEnable(UART0_BASE);

    //
    // Wait until the screen has been pressed, indicating that the firwmare
    // update should begin.
    //
    while(!g_bFirmwareUpdate)
    {
    }

    //
    // Clear the screen.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = 319;
    sRect.i16YMax = 239;
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &sRect);

    //
    // Indicate that the firmware update is about to start.
    //
    GrContextForegroundSet(&sContext, ClrWhite);
    GrStringDrawCentered(&sContext, "Update process started...", -1, 160, 98,
                         false);
    GrStringDrawCentered(&sContext, "Using UART0 with", -1, 160, 138, false);
    GrStringDrawCentered(&sContext, "115,200 baud, 8-N-1.", -1, 160, 158,
                         false);

    //
    // Disable all processor interrupts.  Instead of disabling them one at a
    // time, a direct write to NVIC is done to disable all peripheral
    // interrupts.
    //
    HWREG(NVIC_DIS0) = 0xffffffff;
    HWREG(NVIC_DIS1) = 0xffffffff;
    HWREG(NVIC_DIS2) = 0xffffffff;
    HWREG(NVIC_DIS3) = 0xffffffff;
    HWREG(NVIC_DIS4) = 0xffffffff;

    //
    // Call the ROM UART boot loader.
    //
    ROM_UpdateUART();

    //
    // The boot loader should not return.  In the off chance that it does,
    // enter a dead loop.
    //
    while(1)
    {
    }
}
