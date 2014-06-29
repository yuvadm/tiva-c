//*****************************************************************************
//
// usb_otg_mouse.c - USB OTG Mouse (combined host and device mouse).
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
#include "inc/hw_types.h"
#include "inc/hw_usb.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/device/usbdevice.h"
#include "usblib/host/usbhost.h"
#include "utils/uartstdio.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "usb_otg_mouse.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB OTG HID Mouse Example (usb_otg_mouse)</h1>
//!
//! This example application demonstrates the use of USB On-The-Go (OTG) to
//! offer both USB host and device operation.  When the DK board is connected
//! to a USB host, it acts as a BIOS-compatible USB mouse.  The select button
//! on the board (on the bottom right corner) acts as mouse button 1 and
//! the mouse pointer may be moved by dragging your finger or a stylus across
//! the touchscreen in the desired direction.
//!
//! If a USB mouse is connected to the USB OTG port, the board operates as a
//! USB host and draws dots on the display to track the mouse movement.  The
//! states of up to three mouse buttons are shown at the bottom right of the
//! display.
//!
//*****************************************************************************

//*****************************************************************************
//
// The current state of the USB in the system based on the detected mode.
//
//*****************************************************************************
volatile tUSBMode g_iCurrentMode = eUSBModeNone;

//*****************************************************************************
//
// The size of the host controller's memory pool in bytes.
//
//*****************************************************************************
#define HCD_MEMORY_SIZE         128

//*****************************************************************************
//
// The memory pool to provide to the Host controller driver.
//
//*****************************************************************************
uint8_t g_pui8HCDPool[HCD_MEMORY_SIZE];

//*****************************************************************************
//
// This global is used to indicate to the main that a mode change has
// occurred.
//
//*****************************************************************************
uint32_t g_ui32NewState;

//*****************************************************************************
//
// The system clock frequency in Hz.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

//*****************************************************************************
//
// The graphics context for the screen.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// A function which returns the number of milliseconds since it was last
// called.  This can be found in usb_dev_mouse.c.
//
//*****************************************************************************
extern uint32_t GetTickms(void);

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
// Callback function for mode changes.
//
//*****************************************************************************
void
ModeCallback(uint32_t ui32Index, tUSBMode iMode)
{
    //
    // Save the new mode.
    //
    g_iCurrentMode = iMode;

    switch(iMode)
    {
        case eUSBModeHost:
        {
            break;
        }
        case eUSBModeDevice:
        {
            break;
        }
        case eUSBModeNone:
        {
            break;
        }
        default:
        {
            break;
        }
    }
    g_ui32NewState = 1;
}

//*****************************************************************************
//
// Initialize the USB for OTG mode on the platform.
//
//*****************************************************************************
void
USBOTGInit(uint32_t ui32ClockRate, tUSBModeCallback pfnModeCallback)
{
    uint32_t ui32PLLRate;

    //
    // Enable USB controller.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);

    //
    // Set the current processor speed to provide more accurate timing
    // information to the USB library.
    //
    ui32PLLRate = 480000000;
    USBOTGFeatureSet(0, USBLIB_FEATURE_CPUCLK, &ui32ClockRate);
    USBOTGFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

    //
    // Initialize the USB OTG mode and pass in a mode callback.
    //
    USBStackModeSet(0, eUSBModeOTG, pfnModeCallback);
}

//*****************************************************************************
//
// main routine.
//
//*****************************************************************************
int
main(void)
{
    tLPMFeature sLPMFeature;

    //
    // Run from the PLL at 120 MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet();

    //
    // Configure the UART.
    //
    UARTStdioConfig(0, 115200, g_ui32SysClock);

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(g_ui32SysClock);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&g_sContext, "usb-otg-mouse");

    //
    // Configure USB for OTG operation.
    //
    USBOTGInit(g_ui32SysClock, ModeCallback);

    sLPMFeature.ui32HIRD = 500;
    sLPMFeature.ui32Features = USBLIB_FEATURE_LPM_EN |
                               USBLIB_FEATURE_LPM_RMT_WAKE;
    USBHCDFeatureSet(0, USBLIB_FEATURE_LPM, &sLPMFeature);

    //
    // Initialize the host stack.
    //
    HostInit();

    //
    // Initialize the device stack.
    //
    DeviceInit();

    //
    // Initialize the USB controller for dual mode operation with a 2ms polling
    // rate.
    //
    USBOTGModeInit(0, 2000, g_pui8HCDPool, HCD_MEMORY_SIZE);

    //
    // Set the new state so that the screen updates on the first
    // pass.
    //
    g_ui32NewState = 1;

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Tell the OTG library code how much time has passed in milliseconds
        // since the last call.
        //
        USBOTGMain(GetTickms());

        //
        // Handle deferred state change.
        //
        if(g_ui32NewState)
        {
            g_ui32NewState =0;

            //
            // Update the status area of the screen.
            //
            ClearMainWindow();

            //
            // Update the status bar with the new mode.
            //
            switch(g_iCurrentMode)
            {
                case eUSBModeHost:
                {
                    UpdateStatus("Host Mode", 0, true);
                    break;
                }
                case eUSBModeDevice:
                {
                    UpdateStatus("Device Mode", 0, true);
                    break;
                }
                case eUSBModeNone:
                {
                    UpdateStatus("Idle Mode\n", 0, true);
                    break;
                }
                default:
                {
                    break;
                }
            }
        }

        if(g_iCurrentMode == eUSBModeDevice)
        {
            DeviceMain();
        }
        else if(g_iCurrentMode == eUSBModeHost)
        {
            HostMain();
        }
    }
}
