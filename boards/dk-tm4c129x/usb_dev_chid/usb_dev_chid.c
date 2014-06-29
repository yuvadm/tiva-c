 //*****************************************************************************
//
// usb_dev_chid.c - Main routines for the composite keyboard mouse example.
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
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcomp.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "usblib/device/usbdhidkeyb.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"
#include "usb_structs.h"
#include "ui.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Composite HID Keyboard Mouse Device (usb_dev_chid)</h1>
//!
//! This example application turns the evaluation board into a composite USB
//! keyboard and mouse example using the Human Interface Device class.  The
//! color LCD displays a blank area which acts as a mouse touchpad.  The button
//! on the bottom of the screen acts as a toggle between keyboard and mouse
//! mode.  Pressing it toggles the screen to keyboard mode and allows keys
//! to be sent to the USB host.  The board status LED is used to indicate the
//! current Caps Lock state and is updated in response to pressing the ``Caps''
//! key on the virtual keyboard or any other keyboard attached to the same USB
//! host system.
//
//*****************************************************************************

//*****************************************************************************
//
// The system tick timer period.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 100

//*****************************************************************************
//
// The current system tick count.
//
//*****************************************************************************
volatile uint32_t g_ui32SysTickCount;

//*****************************************************************************
//
// This is the interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    g_ui32SysTickCount++;
}

//*****************************************************************************
//
// Handles all of the generic USB events.
//
//*****************************************************************************
uint32_t
USBEventHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam,
                void *pvMsgData)
{
    //
    // Infor the UI code of the state change.
    //
    if(ui32Event == USB_EVENT_CONNECTED)
    {
        UIMode(UI_CONNECTED);
    }
    else if(ui32Event == USB_EVENT_DISCONNECTED)
    {
        UIMode(UI_NOT_CONNECTED);
    }
    else if(ui32Event == USB_EVENT_SUSPEND)
    {
        UIMode(UI_SUSPENDED);
    }
    else if(ui32Event == USB_EVENT_RESUME)
    {
        UIMode(UI_CONNECTED);
    }
    return(0);
}

//*****************************************************************************
//
// This is the main loop that runs the application.
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
    // Initialize the touch screen driver.
    //
    TouchScreenInit(ui32SysClock);

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(UITouchCallback);

    //
    // Set the system tick to fire 100 times per second.
    //
    ROM_SysTickPeriodSet(ui32SysClock / SYSTICKS_PER_SECOND);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //
    // Initialize the USB stack for device mode.
    //
    USBStackModeSet(0, eUSBModeDevice, 0);

    //
    // Initialize the USB keyboard interface.
    //
    USBKeyboardInit();

    //
    // Initialize the USB mouse interface.
    //
    USBMouseInit();

    //
    // Call the composite device initialization for both the mouse and
    // keyboard.
    //
    USBDHIDMouseCompositeInit(0, &g_sMouseDevice, &g_psCompDevices[0]);
    USBDHIDKeyboardCompositeInit(0, &g_sKeyboardDevice, &g_psCompDevices[1]);

    //
    // Pass the device information to the USB library and place the device
    // on the bus.
    //
    USBDCompositeInit(0, &g_sCompDevice, DESCRIPTOR_DATA_SIZE,
                      g_pui8DescriptorData);

    //
    // Initialize the user interface.
    //
    UIInit();

    while(1)
    {
        //
        // Run the main loop for the user interface.
        //
        UIMain();
    }
}
