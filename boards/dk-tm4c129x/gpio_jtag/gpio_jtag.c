//*****************************************************************************
//
// gpio_jtag.c - Example to demonstrate recovering the JTAG interface.
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
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>GPIO JTAG Recovery (gpio_jtag)</h1>
//!
//! This example demonstrates changing the JTAG pins into GPIOs, along with a
//! mechanism to revert them to JTAG pins.  When first run, the pins remain in
//! JTAG mode.  Pressing the touchscreen will toggle the pins between JTAG
//! and GPIO modes.
//!
//! In this example, four pins (PC0, PC1, PC2, and PC3) are switched.
//
//*****************************************************************************

//*****************************************************************************
//
// The current mode of pins PC0, PC1, PC2, and PC3.  When zero, the pins
// are in JTAG mode; when non-zero, the pins are in GPIO mode.
//
//*****************************************************************************
volatile uint32_t g_ui32Mode;

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
// The touch screen driver calls this function to report all state changes.
//
//*****************************************************************************
static int32_t
GPIOJTAGTestCallback(uint32_t ui32Message, int32_t i32X,  int32_t i32Y)
{
    //
    // Determine what to do now.  The only message we act upon here is PTR_UP
    // which indicates that someone has just ended a touch on the screen.
    //
    if(ui32Message == WIDGET_MSG_PTR_UP)
    {

        //
        // Toggle the pin mode.
        //
        g_ui32Mode ^= 1;

        //
        // See if the pins should be in JTAG or GPIO mode.
        //
        if(g_ui32Mode == 0)
        {
            //
            // Change PC0-3 into hardware (i.e. JTAG) pins.  First open the
            // lock and select the bits we want to modify in the GPIO commit
            // register.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x0F;

            //
            // Now modify the configuration of the pins that we unlocked.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) |= 0x0F;

            //
            // Finally, clear the commit register and the lock to prevent
            // the pin configuration from being changed accidentally later.
            // Note that the lock is closed whenever we write to the GPIO_O_CR
            // register so we need to reopen it here.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x00;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = 0;
        }
        else
        {
            //
            // Change PC0-3 into GPIO inputs. First open the lock and select
            // the bits we want to modify in the GPIO commit register.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x0F;

            //
            // Now modifiy the configuration of the pins that we unlocked.
            // Note that the DriverLib GPIO functions may need to access
            // registers protected by the lock mechanism so these calls should
            // be made while the lock is open here.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) &= 0xf0;
            ROM_GPIOPinTypeGPIOInput(GPIO_PORTC_BASE,
                                     (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
                                      GPIO_PIN_3));

            //
            // Finally, clear the commit register and the lock to prevent
            // the pin configuration from being changed accidentally later.
            // Note that the lock is closed whenever we write to the GPIO_O_CR
            // register so we need to reopen it here.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x00;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = 0;
        }
    }

    return(0);
}

//*****************************************************************************
//
// Toggle the JTAG pins between JTAG and GPIO mode with touches on the
// touchscreen toggling between the two states.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32Mode;
    uint32_t ui32SysClock;

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
    GrContextInit(&g_sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&g_sContext, "gpio-jtag");

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(ui32SysClock);
    TouchScreenCallbackSet(GPIOJTAGTestCallback);

    //
    // Set the global and local indicator of pin mode to zero, meaning JTAG.
    //
    g_ui32Mode = 0;
    ui32Mode = 0;

    //
    // Tell the user what to do.
    //
    GrStringDrawCentered(&g_sContext, "Tap display to toggle pin mode.", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2 ,
                         (GrContextDpyHeightGet(&g_sContext) - 24), 0);

    //
    // Tell the user what state we are in.
    //
    GrContextFontSet(&g_sContext, g_psFontCmss22b);
    GrStringDrawCentered(&g_sContext, "PC0-3 are", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2 ,
                         GrContextDpyHeightGet(&g_sContext) / 2, 0);
    GrStringDrawCentered(&g_sContext, "JTAG", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2 ,
                         (GrContextDpyHeightGet(&g_sContext) / 2) + 26, 0);

    //
    // Loop forever.  This loop simply exists to display the current state of
    // PC0-3; the handling of changing the JTAG pins to and from GPIO mode is
    // done in GPIO Interrupt Handler.
    //
    while(1)
    {
        //
        // Wait until the pin mode changes.
        //
        while(g_ui32Mode == ui32Mode)
        {
        }

        //
        // Save the new mode locally so that a subsequent pin mode change can
        // be detected.
        //
        ui32Mode = g_ui32Mode;

        //
        // Indicate the current mode for the PC0-3 pins.
        //
        GrStringDrawCentered(&g_sContext, ui32Mode ? " GPIO " : " JTAG ", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2 ,
                             (GrContextDpyHeightGet(&g_sContext) / 2) + 26, 1);
    }
}
