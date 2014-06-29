//*****************************************************************************
//
// usb_stick_demo.c - USB stick update demo.
//
// Copyright (c) 2012-2014 Texas Instruments Incorporated.  All rights reserved.
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
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/gpio.h"
#include "grlib/grlib.h"
#include "drivers/cfal96x64x16.h"

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
    uint_fast32_t ui32Count;
    tContext sContext;
    tRectangle sRect;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();

    //
    // Set the system clock to run at 50MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Initialize the display driver.
    //
    CFAL96x64x16Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sCFAL96x64x16);

    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.i16YMax = 9;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&sContext, ClrWhite);
    GrRectDraw(&sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&sContext, g_psFontFixed6x8);
    GrStringDrawCentered(&sContext, "usb-stick-demo", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 4, 0);

    //
    // Indicate what is happening.
    //
    GrStringDrawCentered(&sContext, "Press the", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 20, 0);
    GrStringDrawCentered(&sContext, "select button to", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 30, 0);
    GrStringDrawCentered(&sContext, "start the USB", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 40, 0);
    GrStringDrawCentered(&sContext, "stick updater.", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 50, 0);

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&sContext);

    //
    // Enable the GPIO module which the select button is attached to.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);

    //
    // Enable the GPIO pin to read the user button.
    //
    ROM_GPIODirModeSet(GPIO_PORTM_BASE, GPIO_PIN_4, GPIO_DIR_MODE_IN);
    MAP_GPIOPadConfigSet(GPIO_PORTM_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);

    //
    // Wait for the pullup to take effect or the next loop will exist too soon.
    //
    SysCtlDelay(1000);

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
        if(ROM_GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_4) == 0)
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
        SysCtlDelay(16000000 / (3 * 100));
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
        if(ROM_GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_4) != 0)
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
        SysCtlDelay(16000000 / (3 * 100));
    }

    //
    // Indicate that the updater is being called.
    //
    GrStringDrawCentered(&sContext, "The USB stick", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 20, true);
    GrStringDrawCentered(&sContext, "updater is now", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 30, true);
    GrStringDrawCentered(&sContext, "waiting for a", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 40, true);
    GrStringDrawCentered(&sContext, "USB stick.", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 50, true);

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&sContext);

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
