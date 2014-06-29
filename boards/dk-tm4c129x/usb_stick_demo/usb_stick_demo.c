//*****************************************************************************
//
// usb_stick_demo.c - USB stick update demo.
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
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Stick Update Demo (usb_stick_demo)</h1>
//!
//! An example to demonstrate the use of the flash-based USB stick update
//! program.  This example is meant to be loaded into flash memory from a USB
//! memory stick, using the USB stick update program (usb_stick_update),
//! running on the microcontroller.
//!
//! After this program is built, the binary file (usb_stick_demo.bin), should
//! be renamed to the filename expected by usb_stick_update ("FIRMWARE.BIN" by
//! default) and copied to the root directory of a USB memory stick.  Then,
//! when the memory stick is plugged into the eval board that is running the
//! usb_stick_update program, this example program will be loaded into flash
//! and then run on the microcontroller.
//!
//! This program simply displays a message on the screen and prompts the user
//! to press the select button.  Once the button is pressed, control is passed
//! back to the usb_stick_update program which is still is flash, and it will
//! attempt to load another program from the memory stick.  This shows how
//! a user application can force a new firmware update from the memory stick.
//
//*****************************************************************************

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
// Demonstrate the use of the USB stick update example.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32Count;
    uint32_t ui32SysClock;
    tContext i16Context;
    tRectangle i16Rect;

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
    GrContextInit(&i16Context, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&i16Context, "usb-stick-demo");

    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    i16Rect.i16XMin = 0;
    i16Rect.i16YMin = 0;
    i16Rect.i16XMax = GrContextDpyWidthGet(&i16Context) - 1;
    i16Rect.i16YMax = 23;
    GrContextForegroundSet(&i16Context, ClrDarkBlue);
    GrRectFill(&i16Context, &i16Rect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&i16Context, ClrWhite);
    GrRectDraw(&i16Context, &i16Rect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&i16Context, g_psFontCm20);
    GrStringDrawCentered(&i16Context, "usb-stick-demo", -1,
                         GrContextDpyWidthGet(&i16Context) / 2, 10, 0);

    //
    // Indicate what is happening.
    //
    GrContextFontSet(&i16Context, g_psFontCm24);
    GrStringDrawCentered(&i16Context, "Press the SEL button to", -1,
                         GrContextDpyWidthGet(&i16Context) / 2, 60, 0);
    GrStringDrawCentered(&i16Context, "start the USB stick updater", -1,
                         GrContextDpyWidthGet(&i16Context) / 2, 84, 0);

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&i16Context);

    //
    // Wait for the pullup to take effect or the next loop will exist too soon.
    //
    ROM_SysCtlDelay(1000);

    //
    // Wait until the select button has been pressed for ~40ms (in order to
    // debounce the press).
    //
    ui32Count = 0;
    while(1)
    {
        //
        // See if the button is pressed.
        //
        if(ROM_GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_1) == 0)
        {
            //
            // Increment the count since the button is pressed.
            //
            ui32Count++;

            //
            // If the count has reached 4, then the button has been debounced
            // as being pressed.
            //
            if(ui32Count == 4)
            {
                break;
            }
        }
        else
        {
            //
            // Reset the count since the button is not pressed.
            //
            ui32Count = 0;
        }

        //
        // Delay for approximately 10ms.
        //
        ROM_SysCtlDelay(120000000 / (3 * 100));
    }

    //
    // Wait until the select button has been released for ~40ms (in order to
    // debounce the release).
    //
    ui32Count = 0;
    while(1)
    {
        //
        // See if the button is pressed.
        //
        if(ROM_GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_1) != 0)
        {
            //
            // Increment the count since the button is released.
            //
            ui32Count++;

            //
            // If the count has reached 4, then the button has been debounced
            // as being released.
            //
            if(ui32Count == 4)
            {
                break;
            }
        }
        else
        {
            //
            // Reset the count since the button is pressed.
            //
            ui32Count = 0;
        }

        //
        // Delay for approximately 10ms.
        //
        ROM_SysCtlDelay(120000000 / (3 * 100));
    }

    //
    // Indicate that the updater is being called.
    //
    GrStringDrawCentered(&i16Context, "The USB stick updater is now", -1,
                         GrContextDpyWidthGet(&i16Context) / 2, 140, 0);
    GrStringDrawCentered(&i16Context, "waiting for a USB stick", -1,
                         GrContextDpyWidthGet(&i16Context) / 2, 164, 0);

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&i16Context);

    //
    // Call the updater so that it will search for an update on a memory stick.
    //
    (*((void (*)(void))(*(uint32_t *)0x2c)))();

    //
    // The updater should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }
}
