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
// This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
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
//! to press the USR_SW1 button.  Once the button is pressed, control is passed
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
// Global variable used to store the frequency of the system clock.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

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
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, g_ui32SysClock);
}

//*****************************************************************************
//
// Demonstrate the use of the USB stick update example.
//
//*****************************************************************************
int
main(void)
{
    uint_fast32_t ui32Count;

    //
    // Run from the PLL at 50 MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 50000000);

    //
    // Configure the device pins.
    //
    PinoutSet(false, false);

    //
    // Initialize the UART.
    //
    ConfigureUART();

    //
    // Clear the terminal and print banner.
    //
    UARTprintf("\033[2J\033[H");
    UARTprintf("usb-stick-demo!\n");

    //
    // Indicate what is happening.
    //
    UARTprintf("Press\n");
    UARTprintf("USR_SW1 to\n");
    UARTprintf("start the USB\n");
    UARTprintf("stick updater.\n");

    //
    // Enable the GPIO module which the select button is attached to.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    //
    // Enable the GPIO pin to read the user button.
    //
    ROM_GPIODirModeSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_DIR_MODE_IN);
    MAP_GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA,
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
        if(ROM_GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) == 0)
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
        if(ROM_GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) != 0)
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
        UARTprintf("The USB stick\n");
        UARTprintf("updater is now\n");
        UARTprintf("waiting for a\n");
        UARTprintf("USB stick.\n");

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
