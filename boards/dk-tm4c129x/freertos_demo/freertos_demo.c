//*****************************************************************************
//
// freertos_demo.c - Simple FreeRTOS example.
//
// Copyright (c) 2009-2014 Texas Instruments Incorporated.  All rights reserved.
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

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"
#include "display_task.h"
#include "idle_task.h"
#include "led_task.h"
#include "lwip_task.h"
#include "spider_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>FreeRTOS Example (freertos_demo)</h1>
//!
//! This application utilizes FreeRTOS to perform a variety of tasks in a
//! concurrent fashion.  The following tasks are created:
//!
//! * An lwIP task, which serves up web pages via the Ethernet interface.  This
//!   is actually two tasks, one which runs the lwIP stack and one which
//!   manages the Ethernet interface (sending and receiving raw packets).
//!
//! * An LED task, which simply blinks the on-board LED at a user-controllable
//!   rate (changed via the web interface).
//!
//! * A set of spider tasks, each of which controls a spider that crawls around
//!   the LCD.  The speed at which the spiders move is controllable via the web
//!   interface.  Up to thirty-two spider tasks can be run concurrently (an
//!   application-imposed limit).
//!
//! * A spider control task, which manages presses on the touch screen and
//!   determines if a spider task should be terminated (if the spider is
//!   ``squished'') or if a new spider task should be created (if no spider is
//!   ``squished'').
//!
//! * There is an automatically created idle task, which monitors changes in
//!   the board's IP address and sends those changes to the user via a UART
//!   message.
//!
//! Across the bottom of the LCD, several status items are displayed:  the
//! amount of time the application has been running, the number of tasks that
//! are running, the IP address of the board, the number of Ethernet packets
//! that have been transmitted, and the number of Ethernet packets that have
//! been received.
//!
//! The finder application (in tools/finder) can also be used to discover the
//! IP address of the board.  The finder application will search the network
//! for all boards that respond to its requests and display information about
//! them.
//!
//! The web site served by lwIP includes the ability to adjust the toggle rate
//! of the LED task and the update speed of the spiders (all spiders move at
//! the same speed).
//!
//! For additional details on FreeRTOS, refer to the FreeRTOS web page at:
//! http://www.freertos.org/
//!
//! For additional details on lwIP, refer to the lwIP web page at:
//! http://savannah.nongnu.org/projects/lwip/
//
//*****************************************************************************

//*****************************************************************************
//
// System clock rate in Hz.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

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
// This hook is called by FreeRTOS when an stack overflow error is detected.
//
//*****************************************************************************
void
vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName)
{
    tContext sContext;

    //
    // A fatal FreeRTOS error was detected, so display an error message.
    //
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);
    GrContextForegroundSet(&sContext, ClrRed);
    GrContextBackgroundSet(&sContext, ClrBlack);
    GrContextFontSet(&sContext, g_psFontCm20);
    GrStringDrawCentered(&sContext, "Fatal FreeRTOS error!", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         (((GrContextDpyHeightGet(&sContext) - 24) / 2) +
                          24), 1);

    //
    // This function can not return, so loop forever.  Interrupts are disabled
    // on entry to this function, so no processor interrupts will interrupt
    // this loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// Initialize FreeRTOS and start the initial set of tasks.
//
//*****************************************************************************
int
main(void)
{
    tContext sContext;

    //
    // Run from the PLL at 120 MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480),
                                            configCPU_CLOCK_HZ);

    //
    // Initialize the device pinout appropriately for this board.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(g_ui32SysClock);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&sContext, "freertos-demo");

    //
    // Make sure the main oscillator is enabled because this is required by
    // the PHY.  The system must have a 25MHz crystal attached to the OSC
    // pins.  The SYSCTL_MOSC_HIGHFREQ parameter is used when the crystal
    // frequency is 10MHz or higher.
    //
    SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);

    //
    // Create the display task.
    //
    if(DisplayTaskInit() != 0)
    {
        GrContextForegroundSet(&sContext, ClrRed);
        GrStringDrawCentered(&sContext, "Failed to create display task!", -1,
                             GrContextDpyWidthGet(&sContext) / 2,
                             (((GrContextDpyHeightGet(&sContext) - 24) / 2) +
                              24), 0);
        while(1)
        {
        }
    }

    //
    // Create the spider task.
    //
    if(SpiderTaskInit() != 0)
    {
        GrContextForegroundSet(&sContext, ClrRed);
        GrStringDrawCentered(&sContext, "Failed to create spider task!", -1,
                             GrContextDpyWidthGet(&sContext) / 2,
                             (((GrContextDpyHeightGet(&sContext) - 24) / 2) +
                              24), 0);
        while(1)
        {
        }
    }

    //
    // Create the LED task.
    //
    if(LEDTaskInit() != 0)
    {
        GrContextForegroundSet(&sContext, ClrRed);
        GrStringDrawCentered(&sContext, "Failed to create LED task!", -1,
                             GrContextDpyWidthGet(&sContext) / 2,
                             (((GrContextDpyHeightGet(&sContext) - 24) / 2) +
                              24), 0);
        while(1)
        {
        }
    }

    //
    // Create the lwIP tasks.
    //
    if(lwIPTaskInit() != 0)
    {
        GrContextForegroundSet(&sContext, ClrRed);
        GrStringDrawCentered(&sContext, "Failed to create lwIP tasks!", -1,
                             GrContextDpyWidthGet(&sContext) / 2,
                             (((GrContextDpyHeightGet(&sContext) - 24) / 2) +
                              24), 0);
        while(1)
        {
        }
    }

    //
    // Start the scheduler.  This should not return.
    //
    vTaskStartScheduler();

    //
    // In case the scheduler returns for some reason, print an error and loop
    // forever.
    //
    GrContextForegroundSet(&sContext, ClrRed);
    GrStringDrawCentered(&sContext, "Failed to start scheduler!", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         (((GrContextDpyHeightGet(&sContext) - 24) / 2) + 24),
                         0);
    while(1)
    {
    }
}
