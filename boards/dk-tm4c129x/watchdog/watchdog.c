//*****************************************************************************
//
// watchdog.c - Watchdog timer example.
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
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/watchdog.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"
#include "utils/ustdlib.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Watchdog (watchdog)</h1>
//!
//! This example application demonstrates the use of the watchdog as a simple
//! heartbeat for the system.  If a watchdog is not periodically fed, it will
//! reset the system.  The GREEN LED will blink once every second to show
//! every time watchdog 0 is being fed, the Amber LED will blink once every
//! second to indicate watchdog 1 is being fed. To stop the watchdog being fed
//! and, hence, cause a system reset, tap the left screen to starve the
//! watchdog 0, and right screen to starve the watchdog 1.
//!
//! The counters on the screen show the number of interrupts that each watchdog
//! served; the count wraps at 255.  Since the two watchdogs run in different
//! clock domains, the counters will get out of sync over time.
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
volatile bool g_bFeedWatchdog0 = true;
volatile bool g_bFeedWatchdog1 = true;

//*****************************************************************************
//
// counter of the watchdog interrupt being served.
//
//*****************************************************************************
volatile uint8_t g_ui8CounterWatchdog0 = 0;
volatile uint8_t g_ui8CounterWatchdog1 = 0;

//*****************************************************************************
//
// LED pin definations.  We use GREEN LED and AMBER LED
//
//*****************************************************************************
#define LED_GREEN_GPIO_PORTBASE GPIO_PORTQ_BASE
#define LED_GREEN_GPIO_PIN      GPIO_PIN_7
#define LED_AMBER_GPIO_PORTBASE GPIO_PORTF_BASE
#define LED_AMBER_GPIO_PIN      GPIO_PIN_1

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
// processor does not get reset) and winks the corresponding LED.
//
//*****************************************************************************
void
WatchdogIntHandler(void)
{
    char pcBuf[8];

    //
    // See if watchdog 0 generated an interrupt.
    //
    if(ROM_WatchdogIntStatus(WATCHDOG0_BASE, true))
    {
        //
        // If we have been told to stop feeding the watchdog, return immediately
        // without clearing the interrupt.  This will cause the system to reset
        // next time the watchdog interrupt fires.
        //
        if(g_bFeedWatchdog0)
        {
            //
            // Clear the watchdog interrupt.
            //
            ROM_WatchdogIntClear(WATCHDOG0_BASE);

            //
            // Increment the counter and display it on the screen.
            //
            g_ui8CounterWatchdog0++;
            GrContextFontSet(&g_sContext, g_psFontCmss20);
            usprintf(pcBuf, " %03d ", g_ui8CounterWatchdog0);
            GrStringDrawCentered(&g_sContext, pcBuf, -1, 80, 100, 1);

            //
            // Invert the GREEN LED value.
            //
            ROM_GPIOPinWrite(LED_GREEN_GPIO_PORTBASE, LED_GREEN_GPIO_PIN,
                             (ROM_GPIOPinRead(LED_GREEN_GPIO_PORTBASE,
                                              LED_GREEN_GPIO_PIN) ^
                                              LED_GREEN_GPIO_PIN));
        }
    }

    //
    // See if watchdog 1 generated an interrupt.
    //
    if(ROM_WatchdogIntStatus(WATCHDOG1_BASE, true))
    {
        //
        // If we have been told to stop feeding the watchdog, return immediately
        // without clearing the interrupt.  This will cause the system to reset
        // next time the watchdog interrupt fires.
        //
        if(g_bFeedWatchdog1)
        {
            //
            // Clear the watchdog interrupt.
            //
            ROM_WatchdogIntClear(WATCHDOG1_BASE);

            //
            // Increment the counter and display it on the screen.
            //
            g_ui8CounterWatchdog1++;
            GrContextFontSet(&g_sContext, g_psFontCmss20);
            usprintf(pcBuf, " %03d ", g_ui8CounterWatchdog1);
            GrStringDrawCentered(&g_sContext, pcBuf, -1, 240, 100, 1);

            //
            // Invert the AMBER LED value.
            //
            ROM_GPIOPinWrite(LED_AMBER_GPIO_PORTBASE, LED_AMBER_GPIO_PIN,
                             (ROM_GPIOPinRead(LED_AMBER_GPIO_PORTBASE,
                                              LED_AMBER_GPIO_PIN) ^
                                              LED_AMBER_GPIO_PIN));
        }
    }
}

//*****************************************************************************
//
// The touch screen driver calls this function to report all state changes.
//
//*****************************************************************************
static int32_t
WatchdogTouchCallback(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    //
    // If the screen is tapped, we will receive a PTR_DOWN then a PTR_UP
    // message.  Use PTR_UP as the trigger to stop feeding the watchdog.
    //
    if(ui32Message == WIDGET_MSG_PTR_UP)
    {
        //
        // See if the left or right side of the screen was touched.
        //
        if(i32X <= (GrContextDpyWidthGet(&g_sContext) / 2))
        {
            //
            // Let the user know that the tap has been registered and that the
            // watchdog0 is being starved.
            //
            GrContextFontSet(&g_sContext, g_psFontCmss20);
            GrContextForegroundSet(&g_sContext, ClrRed);
            GrStringDrawCentered(&g_sContext,
                                 "Watchdog 0 starved, reset shortly", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2 ,
                                 (GrContextDpyHeightGet(&g_sContext) / 2) + 40,
                                 1);
            GrContextForegroundSet(&g_sContext, ClrWhite);

            //
            // Set the flag that tells the interrupt handler not to clear the
            // watchdog0 interrupt.
            //
            g_bFeedWatchdog0 = false;
        }
        else
        {
            //
            // Let the user know that the tap has been registered and that the
            // watchdog1 is being starved.
            //
            GrContextFontSet(&g_sContext, g_psFontCmss20);
            GrContextForegroundSet(&g_sContext, ClrRed);
            GrStringDrawCentered(&g_sContext,
                                 "Watchdog 1 starved, reset shortly", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2 ,
                                 (GrContextDpyHeightGet(&g_sContext) / 2) + 60,
                                 1);
            GrContextForegroundSet(&g_sContext, ClrWhite);

            //
            // Set the flag that tells the interrupt handler not to clear the
            // watchdog1 interrupt.
            //
            g_bFeedWatchdog1 = false;
        }
    }

    return(0);
}

//*****************************************************************************
//
// This example demonstrates the use of both watchdog timers.
//
//*****************************************************************************
int
main(void)
{
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
    FrameDraw(&g_sContext, "watchdog");

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(ui32SysClock);
    TouchScreenCallbackSet(WatchdogTouchCallback);

    //
    // Reconfigure PF1 as a GPIO output so that it can be directly driven
    // (instead of being an Ethernet LED).
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);

    //
    // Show the state and offer some instructions to the user.
    //
    GrContextFontSet(&g_sContext, g_psFontCmss20);
    GrStringDrawCentered(&g_sContext, "Watchdog 0:", -1, 80, 80, 0);
    GrStringDrawCentered(&g_sContext, "Watchdog 1:", -1, 240, 80, 0);
    GrContextFontSet(&g_sContext, g_psFontCmss14);
    GrStringDrawCentered(&g_sContext,
                         "Tap the left screen to starve the watchdog 0",
                         -1, GrContextDpyWidthGet(&g_sContext) / 2 ,
                         (GrContextDpyHeightGet(&g_sContext) / 2) + 40, 1);
    GrStringDrawCentered(&g_sContext,
                         "Tap the right screen to starve the watchdog 1",
                         -1, GrContextDpyWidthGet(&g_sContext) / 2 ,
                         (GrContextDpyHeightGet(&g_sContext) / 2) + 60, 1);

    //
    // Enable the peripherals used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG1);

    //
    // Enable the watchdog interrupt.
    //
    ROM_IntEnable(INT_WATCHDOG);

    //
    // Set the period of the watchdog timer.
    //
    ROM_WatchdogReloadSet(WATCHDOG0_BASE, ui32SysClock);
    ROM_WatchdogReloadSet(WATCHDOG1_BASE, 16000000);

    //
    // Enable reset generation from the watchdog timer.
    //
    ROM_WatchdogResetEnable(WATCHDOG0_BASE);
    ROM_WatchdogResetEnable(WATCHDOG1_BASE);

    //
    // Enable the watchdog timer.
    //
    ROM_WatchdogEnable(WATCHDOG0_BASE);
    ROM_WatchdogEnable(WATCHDOG1_BASE);

    //
    // Loop forever while the LED winks as watchdog interrupts are handled.
    //
    while(1)
    {
    }
}
