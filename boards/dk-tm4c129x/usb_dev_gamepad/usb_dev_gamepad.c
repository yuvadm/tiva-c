//*****************************************************************************
//
// usb_dev_gamepad.c - Main routines for the gamepad example.
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
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidgamepad.h"
#include "usb_gamepad_structs.h"
#include "drivers/buttons.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB HID Gamepad Device (usb_dev_gamepad)</h1>
//!
//! This example application turns the evaluation board into USB game pad
//! device using the Human Interface Device gamepad class.  The buttons on the
//! board are reported as buttons 1, 2, and 3.  The X and Y coordinates are
//! reported using the touch screen input.  This example also demonstrates how
//! to use a custom HID report descriptor which is specified in the
//! usb_game_structs.c in the g_pui8GameReportDescriptor structure.
//
//*****************************************************************************

//*****************************************************************************
//
// Graphics context used to show text on the color LCD display.
//
//*****************************************************************************
static tContext g_sContext;
#define TEXT_FONT               g_psFontCmss18b

//*****************************************************************************
//
// The HID custom gamepad report that is returned to the host.
//
//*****************************************************************************
static tCustomReport g_sReport;

//*****************************************************************************
//
// Global holding if there is an update to send to the host.
//
//*****************************************************************************
static volatile bool g_bUpdate;

//*****************************************************************************
//
// This enumeration holds the various states that the gamepad can be in during
// normal operation.
//
//*****************************************************************************
volatile enum
{
    //
    // Not yet configured.
    //
    eStateNotConfigured,

    //
    // Connected and not waiting on data to be sent.
    //
    eStateIdle,

    //
    // Suspended.
    //
    eStateSuspend,

    //
    // Connected and waiting on data to be sent out.
    //
    eStateSending
}
g_iGamepadState;

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
// Shows the status string on the display.
//
// \param psContext is a pointer to the graphics context representing the
// display.
// \param pcStatus is a pointer to the string to be shown.
//
//*****************************************************************************
void
DisplayStatus(tContext *psContext, char *pcStatus)
{
    tRectangle sRectLine;
    int32_t i32Y;

    //
    // Calculate the Y coordinate of the top left of the character cell
    // for our line of text.
    //
    i32Y = GrContextDpyHeightGet(psContext) - GrFontHeightGet(TEXT_FONT) - 10;

    //
    // Determine the bounding rectangle for this line of text. We add 4 pixels
    // to the height just to ensure that we clear a couple of pixels above and
    // below the line of text.
    //
    sRectLine.i16XMin = 0;
    sRectLine.i16XMax = GrContextDpyWidthGet(psContext) - 1;
    sRectLine.i16YMin = i32Y - GrFontHeightGet(TEXT_FONT);
    sRectLine.i16YMax = i32Y + GrFontHeightGet(TEXT_FONT) + 3;

    //
    // Clear the line with black.
    //
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(psContext, &sRectLine);

    //
    // Draw the new status string
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrStringDrawCentered(psContext, pcStatus, -1,
                         GrContextDpyWidthGet(psContext) / 2,
                         i32Y, false);
}

//*****************************************************************************
//
// The interrupt-context handler for touch screen events from the touch screen
// driver.  This function constantly overwrites the current report until the
// main loop can handle the new data.
//
//*****************************************************************************
int32_t
TSHandler(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    //
    // See which event is being sent from the touch screen driver.
    //
    switch(ui32Message)
    {
        //
        // The pen has just been placed down.
        //
        case WIDGET_MSG_PTR_DOWN:
        case WIDGET_MSG_PTR_MOVE:
        {
            //
            // Let the main loop know that there is an update.
            //
            g_bUpdate = true;

            //
            // Scale and save the current position.
            //
            g_sReport.i8XPos = (int8_t)(((i32X - 160) * 255)/320);
            g_sReport.i8YPos = (int8_t)(((i32Y - 120) * 255)/240);

            //
            // This event has been handled.
            //
            break;
        }

        //
        // The pen has just been picked up.
        //
        case WIDGET_MSG_PTR_UP:
        {
            //
            // Let the main loop know that there is an update.
            //
            g_bUpdate = true;

            //
            // Reset to the center position.
            //
            g_sReport.i8XPos = 0;
            g_sReport.i8YPos = 0;

            //
            // This event has been handled.
            //
            break;
        }
    }


    //
    // Tell the touch handler that everything is fine.
    //
    return(1);
}

//*****************************************************************************
//
// Handles asynchronous events from the HID gamepad driver.
//
// \param pvCBData is the event callback pointer provided during
// USBDHIDGamepadInit().  This is a pointer to our gamepad device structure
// (&g_sGamepadDevice).
// \param ui32Event identifies the event we are being called back for.
// \param ui32MsgData is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the HID gamepad driver to inform the application
// of particular asynchronous events related to operation of the gamepad HID
// device.
//
// \return Returns 0 in all cases.
//
//*****************************************************************************
uint32_t
GamepadHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgData,
               void *pvMsgData)
{
    switch (ui32Event)
    {
        //
        // The host has connected to us and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            g_iGamepadState = eStateIdle;

            DisplayStatus(&g_sContext, "Connected");
            break;
        }

        //
        // The host has disconnected from us.
        //
        case USB_EVENT_DISCONNECTED:
        {
            g_iGamepadState = eStateNotConfigured;

            DisplayStatus(&g_sContext, "Disconnected");
            break;
        }

        //
        // This event occurs every time the host acknowledges transmission
        // of a report.  It is to return to the idle state so that a new report
        // can be sent to the host.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            //
            // Enter the idle state since we finished sending something.
            //
            g_iGamepadState = eStateIdle;

            break;
        }

        //
        // This event indicates that the host has suspended the USB bus.
        //
        case USB_EVENT_SUSPEND:
        {
            //
            // Go to the suspended state.
            //
            g_iGamepadState = eStateSuspend;

            DisplayStatus(&g_sContext, "Suspended");
            break;
        }

        //
        // This event signals that the host has resumed signaling on the bus.
        //
        case USB_EVENT_RESUME:
        {
            //
            // Go back to the idle state.
            //
            g_iGamepadState = eStateIdle;

            DisplayStatus(&g_sContext, "Connected");
            break;
        }

        //
        // Return the pointer to the current report.  This call is
        // rarely if ever made, but is required by the USB HID
        // specification.
        //
        case USBD_HID_EVENT_GET_REPORT:
        {
            *(void **)pvMsgData = (void *)&g_sReport;

            break;
        }

        //
        // We ignore all other events.
        //
        default:
        {
            break;
        }
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
    uint8_t ui8ButtonsChanged, ui8Buttons;
    uint32_t ui32SysClock;

    //
    // Set the clocking to run from the PLL at 120MHz
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                                           SYSCTL_XTAL_25MHZ |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet();

    //
    // Configure the buttons driver.
    //
    ButtonsInit(ALL_BUTTONS);

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
    FrameDraw(&g_sContext, "usb-dev-gamepad");

    //
    // Default status is disconnected.
    //
    DisplayStatus(&g_sContext, "Disconnected");

    //
    // Not configured initially.
    //
    g_iGamepadState = eStateNotConfigured;

    //
    // Initialize the USB stack for device mode.
    //
    USBStackModeSet(0, eUSBModeDevice, 0);

    //
    // Pass the device information to the USB library and place the device
    // on the bus.
    //
    USBDHIDGamepadInit(0, &g_sGamepadDevice);

    //
    // Zero out the initial report.
    //
    g_sReport.ui8Buttons = 0;
    g_sReport.i8XPos = 0;
    g_sReport.i8YPos = 0;

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(ui32SysClock);

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(TSHandler);

    //
    // The main loop starts here.  We begin by waiting for a host connection
    // then drop into the main gamepad handling section.  If the host
    // disconnects, we return to the top and wait for a new connection.
    //
    while(1)
    {
        //
        // Wait here until USB device is connected to a host.
        //
        if(g_iGamepadState == eStateIdle)
        {
            //
            // See if the buttons updated.
            //
            ButtonsPoll(&ui8ButtonsChanged, &ui8Buttons);

            g_sReport.ui8Buttons = 0;

            //
            // Set button 1 if up button pressed.
            //
            if(ui8Buttons & UP_BUTTON)
            {
                g_sReport.ui8Buttons |= 0x01;
            }

            //
            // Set button 2 if down button pressed.
            //
            if(ui8Buttons & DOWN_BUTTON)
            {
                g_sReport.ui8Buttons |= 0x02;
            }

            //
            // Set button 3 if select button pressed.
            //
            if(ui8Buttons & SELECT_BUTTON)
            {
                g_sReport.ui8Buttons |= 0x04;
            }

            if(ui8ButtonsChanged)
            {
                g_bUpdate = true;
            }

            //
            // Send the report if there was an update.
            //
            if(g_bUpdate)
            {
                g_bUpdate = false;

                USBDHIDGamepadSendReport(&g_sGamepadDevice, &g_sReport,
                                         sizeof(g_sReport));

                //
                // Now sending data but protect this from an interrupt since
                // it can change in interrupt context as well.
                //
                IntMasterDisable();
                g_iGamepadState = eStateSending;
                IntMasterEnable();
            }
        }
    }
}
