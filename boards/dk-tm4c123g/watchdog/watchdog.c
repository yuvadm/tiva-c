//*****************************************************************************
//
// watchdog.c - Watchdog timer example.
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
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/watchdog.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "drivers/cfal96x64x16.h"
#include "drivers/buttons.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Watchdog (watchdog)</h1>
//!
//! This example application demonstrates the use of the watchdog as a simple
//! heartbeat for the system.  If the watchdog is not periodically fed, it will
//! reset the system.  Each time the watchdog is fed, the LED is inverted so
//! that it is easy to see that it is being fed, which occurs once every
//! second.  To stop the watchdog being fed and, hence, cause a system reset,
//! press the select button.
//
//*****************************************************************************

//*****************************************************************************
//
// Graphics context used to show text on the display.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// Flag to tell the watchdog interrupt handler whether or not to clear the
// interrupt (feed the watchdog).
//
//*****************************************************************************
volatile bool g_bFeedWatchdog = true;

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
// The interrupt handler for the watchdog.  This feeds the dog (so that the
// processor does not get reset) and winks the LED connected to GPIO B3.
//
//*****************************************************************************
void
WatchdogIntHandler(void)
{
    //
    // If we have been told to stop feeding the watchdog, return immediately
    // without clearing the interrupt.  This will cause the system to reset
    // next time the watchdog interrupt fires.
    //
    if(!g_bFeedWatchdog)
    {
        return;
    }

    //
    // Clear the watchdog interrupt.
    //
    ROM_WatchdogIntClear(WATCHDOG0_BASE);

    //
    // Invert the GPIO PF3 value.
    //
    ROM_GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_2,
                     (ROM_GPIOPinRead(GPIO_PORTG_BASE, GPIO_PIN_2) ^
                                     GPIO_PIN_2));
}

//*****************************************************************************
//
// This function is called when the select button is pressed.
//
//*****************************************************************************
static int32_t
SelectButtonPressed(void)
{
    //
    // Find the X center of the display
    //
    int32_t i32CenterX = GrContextDpyWidthGet(&g_sContext) / 2;

    //
    // Let the user know that the button has been pressed and that the
    // watchdog is being starved.
    //
    GrStringDrawCentered(&g_sContext, "Starving", -1, i32CenterX, 14, 1);
    GrStringDrawCentered(&g_sContext, "Watchdog", -1, i32CenterX, 24, 1);
    GrStringDrawCentered(&g_sContext, "System", -1, i32CenterX, 36, 1);
    GrStringDrawCentered(&g_sContext, "   will   ", -1, i32CenterX, 46, 1);
    GrStringDrawCentered(&g_sContext, "reset ...", -1, i32CenterX, 56, 1);

    //
    // Set the flag that tells the interrupt handler not to clear the
    // watchdog interrupt.
    //
    g_bFeedWatchdog = false;

    return(0);
}

//*****************************************************************************
//
// This example demonstrates the use of the watchdog timer.
//
//*****************************************************************************
int
main(void)
{
    int_fast32_t i32CenterX;
    tRectangle sRect;

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
    // Initialize the display and  buttons drivers.
    //
    CFAL96x64x16Init();
    ButtonsInit();

    //
    // Initialize the graphics context and find the middle X coordinate.
    //
    GrContextInit(&g_sContext, &g_sCFAL96x64x16);
    i32CenterX = GrContextDpyWidthGet(&g_sContext) / 2;

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
    GrStringDrawCentered(&g_sContext, "watchdog", -1, i32CenterX, 4, 0);

    //
    // Show the state and offer some instructions to the user.
    //
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);
    GrStringDrawCentered(&g_sContext, "Feeding", -1, i32CenterX, 14, 1);
    GrStringDrawCentered(&g_sContext, "Watchdog", -1, i32CenterX, 24, 1);
    GrStringDrawCentered(&g_sContext, "Press", -1, i32CenterX, 36, 1);
    GrStringDrawCentered(&g_sContext, "Select", -1, i32CenterX, 46, 1);
    GrStringDrawCentered(&g_sContext, "to stop", -1, i32CenterX, 56, 1);

    //
    // Enable the peripherals used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);

    //
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();

    //
    // Set GPIO PG2 as an output.  This drives an LED on the board that will
    // toggle when a watchdog interrupt is processed.
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_2);
    ROM_GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_2, 0);

    //
    // Enable the watchdog interrupt.
    //
    ROM_IntEnable(INT_WATCHDOG);

    //
    // Set the period of the watchdog timer.
    //
    ROM_WatchdogReloadSet(WATCHDOG0_BASE, ROM_SysCtlClockGet());

    //
    // Enable reset generation from the watchdog timer.
    //
    ROM_WatchdogResetEnable(WATCHDOG0_BASE);

    //
    // Enable the watchdog timer.
    //
    ROM_WatchdogEnable(WATCHDOG0_BASE);

    //
    // Loop forever while the LED winks as watchdog interrupts are handled.
    //
    while(1)
    {
        //
        // Poll for the select button pressed
        //
        uint8_t ui8Buttons = ButtonsPoll(0, 0);
        if(ui8Buttons & SELECT_BUTTON)
        {
            SelectButtonPressed();
            while(1)
            {
            }
        }
    }
}
