//*****************************************************************************
//
// hibernate.c - Hibernation example.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C123G Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/hibernate.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/rom.h"
#include "driverlib/uart.h"
#include "grlib/grlib.h"
#include "utils/ustdlib.h"
#include "drivers/cfal96x64x16.h"
#include "drivers/buttons.h"
#include "inc/hw_hibernate.h"
#include "driverlib/pin_map.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Hibernate Example (hibernate)</h1>
//!
//! An example to demonstrate the use of the Hibernation module.  The user
//! can put the microcontroller in hibernation by pressing the select
//! button.  The microcontroller will then wake on its own after 5 seconds,
//! or immediately if the user presses the select button again.  The program
//! keeps a count of the number of times it has entered hibernation.  The
//! value of the counter is stored in the battery backed memory of the
//! Hibernation module so that it can be retrieved when the microcontroller
//! wakes.
//
//*****************************************************************************

//*****************************************************************************
//
// Macros to convert character based display rows and columns into the pixel
// based X and Y coordinates needed for display drawing functions.
//
//*****************************************************************************
#define Col(c)                  ((c) * 6)
#define Row(r)                  ((r) * 8)

//*****************************************************************************
//
// A counter that counts the number of ticks of the SysTick interrupt.
//
//*****************************************************************************
volatile uint32_t g_ui32SysTickCount = 0;

//*****************************************************************************
//
// A buffer to hold character strings that will be printed to the display.
//
//*****************************************************************************
static char g_pcBuf[40];

//*****************************************************************************
//
// Text that will be displayed if there is an error.
//
//*****************************************************************************
static char *g_pcErrorText[] =
{
    "The controller",
    "did not enter",
    "hibernate mode.",
    "---------------------",
    "   PRESS BUTTON",
    "    TO RESTART",
    0
};

//*****************************************************************************
//
// A flag to indicate the select button was pressed.
//
//*****************************************************************************
static volatile bool bSelectPressed = 0;

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
// Delay a certain number of SysTick timer ticks.
//
//*****************************************************************************
void
SysTickWait(uint32_t ui32Ticks)
{
    ui32Ticks += g_ui32SysTickCount;
    while(g_ui32SysTickCount <= ui32Ticks)
    {
    }
}

//*****************************************************************************
//
// The SysTick handler.  Increments a tick counter and debounces the push
// button.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    uint8_t ui8Data, ui8Delta;

    //
    // Increment the tick counter.
    //
    g_ui32SysTickCount++;

    //
    // Get buttons status using button debouncer driver
    //
    ui8Data = ButtonsPoll(&ui8Delta, 0);

    //
    // See if the select button was just pressed.
    //
    if(BUTTON_PRESSED(SELECT_BUTTON, ui8Data, ui8Delta))
    {
        //
        // Set a flag to indicate that the select button was just pressed.
        //
        bSelectPressed = 1;
    }

    //
    // Else, see if the select button was just released.
    //
    if(BUTTON_RELEASED(SELECT_BUTTON, ui8Data, ui8Delta))
    {
        //
        // Clear the button pressed flag.
        //
        bSelectPressed = 0;
    }
}

//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************
void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, 16000000);
    //
    UARTStdioConfig(0, 115200, 16000000);
}

//*****************************************************************************
//
// Run the hibernate example.  Use a loop to put the microcontroller into
// hibernate mode, and to wake up based on time. Also allow the user to cause
// it to hibernate and/or wake up based on button presses.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32Idx;
    uint32_t ui32Status = 0;
    uint32_t ui32HibernateCount = 0;
    tContext sContext;
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
    // Initialize the UART.
    //
    ConfigureUART();

    //
    // Initialize the OLED display
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
    // Change foreground for white text.
    //
    GrContextForegroundSet(&sContext, ClrWhite);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&sContext, g_psFontFixed6x8);
    GrStringDrawCentered(&sContext, "hibernate", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 4, 0);

    //
    // Initialize the buttons driver
    //
    ButtonsInit();

    //
    // Set up systick to generate interrupts at 100 Hz.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 100);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //
    // Enable the Hibernation module.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);

    //
    // Print wake cause message on display.
    //
    GrStringDrawCentered(&sContext, "Wake due to:", -1,
                         GrContextDpyWidthGet(&sContext) / 2, Row(2) + 4,
                         true);

    //
    // Check to see if Hibernation module is already active, which could mean
    // that the processor is waking from a hibernation.
    //
    if(HibernateIsActive())
    {
        //
        // Read the status bits to see what caused the wake.
        //
        ui32Status = HibernateIntStatus(0);
        HibernateIntClear(ui32Status);

        //
        // Wake was due to the push button.
        //
        if(ui32Status & HIBERNATE_INT_PIN_WAKE)
        {
            GrStringDrawCentered(&sContext, "BUTTON", -1,
                                 GrContextDpyWidthGet(&sContext) / 2,
                                 Row(3) + 4, true);
        }

        //
        // Wake was due to RTC match
        //
        else if(ui32Status & HIBERNATE_INT_RTC_MATCH_0)
        {
            GrStringDrawCentered(&sContext, "TIMEOUT", -1,
                                 GrContextDpyWidthGet(&sContext) / 2,
                                 Row(3) + 4, true);
        }

        //
        // Wake is due to neither button nor RTC, so it must have been a hard
        // reset.
        //
        else
        {
            GrStringDrawCentered(&sContext, "RESET", -1,
                                 GrContextDpyWidthGet(&sContext) / 2,
                                 Row(3) + 4, true);
        }

        //
        // If the wake is due to button or RTC, then read the first location
        // from the battery backed memory, as the hibernation count.
        //
        if(ui32Status & (HIBERNATE_INT_PIN_WAKE | HIBERNATE_INT_RTC_MATCH_0))
        {
            HibernateDataGet(&ui32HibernateCount, 1);
        }
    }

    //
    // Enable the Hibernation module.  This should always be called, even if
    // the module was already enabled, because this function also initializes
    // some timing parameters.
    //
    HibernateEnableExpClk(ROM_SysCtlClockGet());

    //
    // If the wake was not due to button or RTC match, then it was a reset.
    //
    if(!(ui32Status & (HIBERNATE_INT_PIN_WAKE | HIBERNATE_INT_RTC_MATCH_0)))
    {
        //
        // Configure the module clock source.
        //
        HibernateClockConfig(HIBERNATE_OSC_LOWDRIVE);

        //
        // Finish the wake cause message.
        //
        GrStringDrawCentered(&sContext, "RESET", -1,
                             GrContextDpyWidthGet(&sContext) / 2,
                             Row(3) + 4, true);

        //
        // Wait a couple of seconds in case we need to break in with the
        // debugger.
        //
        SysTickWait(3 * 100);

        //
        // Allow time for the crystal to power up.  This line is separated from
        // the above to make it clear this is still needed, even if the above
        // delay is removed.
        //
        SysTickWait(15);
    }

    //
    // Print the count of times that hibernate has occurred.
    //
    usnprintf(g_pcBuf, sizeof(g_pcBuf), "Hib count=%4u", ui32HibernateCount);
    GrStringDrawCentered(&sContext, g_pcBuf, -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         Row(1) + 4, true);

    //
    // Print messages on the screen about hibernation.
    //
    GrStringDrawCentered(&sContext, "Select to Hib", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         Row(4) + 4, true);
    GrStringDrawCentered(&sContext, "Wake in 5 s,", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         Row(5) + 4, true);
    GrStringDrawCentered(&sContext, "or press Select", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         Row(6) + 4, true);
    GrStringDrawCentered(&sContext, "for immed. wake.", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         Row(7) + 4, true);

    //
    // Clear the button pressed flag, in case it was held down at the
    // beginning.
    //
    bSelectPressed = 0;

    //
    // Wait for user to press the button.
    //
    while(!bSelectPressed)
    {
        //
        // Wait a bit before looping again.
        //
        SysTickWait(10);
    }

    //
    // Tell user to release the button.
    //
    GrStringDrawCentered(&sContext, "                ", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         Row(4) + 4, true);
    GrStringDrawCentered(&sContext, "   Release the  ", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         Row(5) + 4, true);
    GrStringDrawCentered(&sContext, "     button.    ", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         Row(6) + 4, true);
    GrStringDrawCentered(&sContext, "                ", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         Row(7) + 4, true);

    //
    // Wait for user to release the button.
    //
    while(bSelectPressed)
    {
    }

    //
    // If hibernation count is very large, it may be that there was already
    // a value in the hibernate memory, so reset the count.
    //
    ui32HibernateCount = (ui32HibernateCount > 10000) ? 0 : ui32HibernateCount;

    //
    // Increment the hibernation count, and store it in the battery backed
    // memory.
    //
    ui32HibernateCount++;
    HibernateDataSet(&ui32HibernateCount, 1);

    //
    // Clear and enable the RTC and set the match registers to 5 seconds in the
    // future. Set both to same, though they could be set differently, the
    // first to match will cause a wake.
    //
    HibernateRTCSet(0);
    HibernateRTCEnable();
    HibernateRTCMatchSet(0, 5);

    //
    // Set wake condition on pin or RTC match.  Board will wake when 5 seconds
    // elapses, or when the button is pressed.
    //
    HibernateWakeSet(HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_RTC);

    //
    // Request hibernation.
    //
    HibernateRequest();

    //
    // Give it time to activate, it should never get past this wait.
    //
    SysTickWait(100);

    //
    // Should not have got here, something is wrong.  Print an error message to
    // the user.
    //
    sRect.i16XMin = 0;
    sRect.i16XMax = 95;
    sRect.i16YMin = 0;
    sRect.i16YMax = 63;
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &sRect);
    GrContextForegroundSet(&sContext, ClrWhite);
    ui32Idx = 0;
    while(g_pcErrorText[ui32Idx])
    {
        GrStringDraw(&sContext, g_pcErrorText[ui32Idx], -1, Col(0),
                     Row(ui32Idx), true);
        ui32Idx++;
    }

    //
    // Wait for the user to press the button, then restart the app.
    //
    bSelectPressed = 0;
    while(!bSelectPressed)
    {
    }

    //
    // Reset the processor.
    //
    ROM_SysCtlReset();

    //
    // Finished.
    //
    while(1)
    {
    }
}
