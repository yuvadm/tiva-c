//*****************************************************************************
//
// bitband.c - Bit-band manipulation example.
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
// This is part of revision 2.1.0.12573 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/fpu.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/rom.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Bit-Banding (bitband)</h1>
//!
//! This example application demonstrates the use of the bit-banding
//! capabilities of the Cortex-M4F microprocessor.  All of SRAM and all of the
//! peripherals reside within bit-band regions, meaning that bit-banding
//! operations can be applied to any of them.  In this example, a variable in
//! SRAM is set to a particular value one bit at a time using bit-banding
//! operations (it would be more efficient to do a single non-bit-banded write;
//! this simply demonstrates the operation of bit-banding).
//
//*****************************************************************************


//*****************************************************************************
//
// The value that is to be modified via bit-banding.
//
//*****************************************************************************
static volatile uint32_t g_ui32Value;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
    while(1)
    {
        //
        // Hang on runtime error.
        //
    }
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
    // Initialize the UART interface.
    //
    ConfigureUART();

    UARTprintf("\033[2JBit banding...\n");

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
    // Print the initial value to the UART.
    //
    UARTprintf("\r%08x", g_ui32Value);

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
        HWREGBITW(&g_ui32Value, 31 - ui32Idx) = (0xdecafbad >>
                                                 (31 - ui32Idx)) & 1;

        //
        // Print the current value to the UART.
        //
        UARTprintf("\r%08x", g_ui32Value);

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
    // Print out the result.
    //
    if(ui32Errors)
    {
        UARTprintf("\nErrors!\n");
    }
    else
    {
        UARTprintf("\nSuccess!\n");
    }

    //
    // Loop forever.
    //
    while(1)
    {
    }
}
