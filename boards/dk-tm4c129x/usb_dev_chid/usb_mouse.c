//*****************************************************************************
//
// usb_dev_mouse.c - Main routines for the mouse portion of the composite
// device.
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
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcomp.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "usblib/device/usbdhidkeyb.h"
#include "usb_structs.h"

//*****************************************************************************
//
// The global state of the mouse.
//
//*****************************************************************************
static volatile struct
{
    int32_t i32X;
    int32_t i32Y;
    uint8_t ui8Buttons;

    enum
    {
        //
        // No pending or reports being sent.
        //
        MOUSE_IDLE,

        //
        // Sending a report with none pending.
        //
        MOUSE_SENDING,

        //
        // Sending a report and have one pending.
        //
        MOUSE_SENDING_PEND,

        //
        // Pending report but none currently sending.
        //
        MOUSE_PENDING,

        //
        // Disconnect occurred.
        //
        MOUSE_DISCONNECT
    }
    eState;
}
sMouseState;

//*****************************************************************************
//
// Initialize the global state of the mouse.
//
//*****************************************************************************
void
USBMouseInit(void)
{
    //
    // Initialize the mouse state..
    //
    sMouseState.i32X = 0;
    sMouseState.i32Y = 0;
    sMouseState.eState = MOUSE_IDLE;
    sMouseState.ui8Buttons = 0;
}

//*****************************************************************************
//
// This function is called by the UI to update the mouse movement and buttons.
//
// \param i32X is the delta in X movement for the mouse.
// \param i32Y is the delta in Y movement for the mouse.
// \param ui8Buttons is the bit mapped value for the buttons.
//
// \return None.
//
//*****************************************************************************
void
USBMouseUpdate(int32_t i32X, int32_t i32Y, uint8_t ui8Buttons)
{
    switch(sMouseState.eState)
    {
        case MOUSE_SENDING:
        {
            //
            // Go to the pending while sending state because there is a
            // transmit already being sent.  This
            //
            sMouseState.eState = MOUSE_SENDING_PEND;
        }
        case MOUSE_SENDING_PEND:
        case MOUSE_PENDING:
        {
            //
            // Accumulate changes in mouse position if there is already a
            // report being sent.
            //
            sMouseState.i32X += i32X;
            if(sMouseState.i32X > 127)
            {
                sMouseState.i32X = 127;
            }
            else if(sMouseState.i32X < -128)
            {
                sMouseState.i32X = -128;
            }

            sMouseState.i32Y += i32Y;
            if(sMouseState.i32X > 127)
            {
                sMouseState.i32X = 127;
            }
            else if(sMouseState.i32X < -128)
            {
                sMouseState.i32X = -128;
            }
            sMouseState.ui8Buttons |= ui8Buttons;

            break;
        }
        case MOUSE_IDLE:
        default:
        {
            //
            // If idle then just send the update.
            //
            sMouseState.i32X = i32X;
            sMouseState.i32Y = i32Y;

            sMouseState.ui8Buttons |= ui8Buttons;

            sMouseState.eState = MOUSE_PENDING;

            break;
        }
    }
}

//*****************************************************************************
//
// Event handler for the USB HID mouse callbacks.  This was passed into the
// USB library as the callback for USB HID mouse events.
//
//*****************************************************************************
uint32_t
USBMouseHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam,
                void *pvMsgData)
{
    //
    // The only event that is monitored is the transmit complete to know
    // that it safe to send another report.
    //
    if(ui32Event == USB_EVENT_TX_COMPLETE)
    {
        switch(sMouseState.eState)
        {
            //
            // Done sending return to idle.
            //
            case MOUSE_SENDING:
            {
                //
                // Clear the previous X,Y accumulated values.
                //
                sMouseState.i32X = 0;
                sMouseState.i32Y = 0;

                //
                // If done with sending then always send out a button update
                // if buttons were pressed.
                //
                if(sMouseState.ui8Buttons)
                {
                    sMouseState.ui8Buttons = 0;
                    sMouseState.eState = MOUSE_PENDING;
                }
                else
                {
                    sMouseState.eState = MOUSE_IDLE;
                }
                break;
            }

            //
            // While sending more data was ready to be sent so switch to the
            // pending state again.
            //
            case MOUSE_SENDING_PEND:
            {
                sMouseState.eState = MOUSE_PENDING;
                break;
            }

            //
            // Should not get here, but included for completeness.
            //
            case MOUSE_PENDING:
            case MOUSE_IDLE:
            default:
            {
                break;
            }
        }
    }
    else if(ui32Event == USB_EVENT_DISCONNECTED)
    {
        //
        // Stay in the disconnected state.
        //
        sMouseState.eState = MOUSE_DISCONNECT;
    }
    else if(ui32Event == USB_EVENT_CONNECTED)
    {
        //
        // This received even if the mouse is not active, but reset the state
        // in all cases.
        //
        sMouseState.i32X = 0;
        sMouseState.i32Y = 0;
        sMouseState.ui8Buttons = 0;
        sMouseState.eState = MOUSE_IDLE;
    }
    return(0);
}

//*****************************************************************************
//
// Main routine for the mouse.
//
//*****************************************************************************
void
USBMouseMain(void)
{
    //
    // Disable interrupts while changing the variables below.
    //
    IntMasterDisable();

    switch(sMouseState.eState)
    {
        case MOUSE_PENDING:
        {
            //
            // Send the report since there is one pending.
            //
            USBDHIDMouseStateChange((void *)&g_sMouseDevice,
                                    (uint8_t)sMouseState.i32X,
                                    (uint8_t)sMouseState.i32Y,
                                    sMouseState.ui8Buttons);
            //
            // Clear out the report data so that pending data does not
            // continually accumulate.
            //
            sMouseState.i32X = 0;
            sMouseState.i32Y = 0;

            if(sMouseState.ui8Buttons)
            {
                //
                // Need to always send out that all buttons are released.
                //
                sMouseState.ui8Buttons = 0;
                sMouseState.eState = MOUSE_SENDING_PEND;
            }
            else
            {
                //
                // Switch to sending state and wait for the transmit to be
                // complete.
                //
                sMouseState.eState = MOUSE_SENDING;
            }

            break;
        }
        case MOUSE_DISCONNECT:
        case MOUSE_SENDING:
        case MOUSE_SENDING_PEND:
        case MOUSE_IDLE:
        default:
        {
            break;
        }
    }

    //
    // Enable interrupts now that the main loop is complete.
    //
    IntMasterEnable();
}
