//*****************************************************************************
//
// usb_dev_mouse.c - Main routines for the mouse device.
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
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "utils/uartstdio.h"
#include "usb_mouse_structs.h"
#include "drivers/touch.h"
#include "usb_otg_mouse.h"

//*****************************************************************************
//
// The GPIO pin which is connected to the select button.
//
//*****************************************************************************
#define SEL_BTN_PORT            GPIO_PORTP_BASE
#define SEL_BTN_PIN             GPIO_PIN_1

//*****************************************************************************
//
// The defines used with the g_ui32Commands variable.
//
//*****************************************************************************
#define UPDATE_TICK_EVENT       0x80000000

//*****************************************************************************
//
// The incremental update for the mouse.
//
//*****************************************************************************
#define MOUSE_MOVE_INC          ((char)4)
#define MOUSE_MOVE_DEC          ((char)-4)

//*****************************************************************************
//
// The HID mouse report offsets for this mouse application.
//
//*****************************************************************************
#define HID_REPORT_BUTTONS      0
#define HID_REPORT_X            1
#define HID_REPORT_Y            2

//*****************************************************************************
//
// Holds command bits used to signal the main loop to perform various tasks.
//
//*****************************************************************************
volatile uint32_t g_ui32Commands;

//*****************************************************************************
//
// Holds the current state of the touchscreen - pressed or not.
//
//*****************************************************************************
volatile bool g_bScreenPressed;

//*****************************************************************************
//
// Holds the current state of the user button - pressed or not.
//
//*****************************************************************************
volatile bool g_bButtonPressed;


//*****************************************************************************
//
// A flag used to indicate whether or not we are currently connected to the USB
// host.
//
//*****************************************************************************
volatile bool g_bConnected;

//*****************************************************************************
//
// Holds the previous press position for the touchscreen.
//
//*****************************************************************************
volatile int32_t g_i32ScreenStartX;
volatile int32_t g_i32ScreenStartY;

//*****************************************************************************
//
// Holds the current press position for the touchscreen.
//
//*****************************************************************************
volatile int32_t g_i32ScreenX;
volatile int32_t g_i32ScreenY;

//*****************************************************************************
//
// The system tick timer rate.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND     100
#define MS_PER_SYSTICK          (1000 / SYSTICKS_PER_SECOND)

//*****************************************************************************
//
// Global system tick counter holds elapsed time since the application started
// expressed in 100ths of a second.
//
//*****************************************************************************
volatile uint32_t g_ui32SysTickCount;

//*****************************************************************************
//
// Global system tick counter hold the g_ui32SysTickCount the last time
// GetTickms() was called.
//
//*****************************************************************************
static uint32_t g_ui32LastTick;

//*****************************************************************************
//
// The number of system ticks to wait for each USB packet to be sent before
// we assume the host has disconnected.  The value 50 equates to half a second.
//
//*****************************************************************************
#define MAX_SEND_DELAY          50

//*****************************************************************************
//
// This enumeration holds the various states that the mouse can be in during
// normal operation.
//
//*****************************************************************************
volatile enum
{
    //
    // Unconfigured.
    //
    eMouseStateUnconfigured,

    //
    // No keys to send and not waiting on data.
    //
    eMouseStateIdle,

    //
    // Waiting on data to be sent out.
    //
    eMouseStateSend
}
g_iMouseState = eMouseStateUnconfigured;


uint32_t
MouseHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgData,
             void *pvMsgData)
{
    switch(ui32Event)
    {
        //
        // The USB host has connected to and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            UARTprintf("Host connected.\n");
            g_iMouseState = eMouseStateIdle;
            g_bConnected = true;
            break;
        }

        //
        // The USB host has disconnected from the device.
        //
        case USB_EVENT_DISCONNECTED:
        {
            UARTprintf("Host disconnected.\n");
            g_bConnected = false;
            g_iMouseState = eMouseStateUnconfigured;
            break;
        }

        //
        // A report was sent to the host. We are now free to send another.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            g_iMouseState = eMouseStateIdle;
            break;
        }
    }
    return(0);
}

//***************************************************************************
//
// Wait for a period of time for the state to become idle or unconfigured.
//
// \param ui32TimeoutTick is the number of system ticks to wait before
// declaring a timeout and returning \b false.
//
// This function polls the current mouse state for ui32TimeoutTicks system
// ticks waiting for it to either become idle or unconfigured.  If the state
// becomes one of these, the function returns true.  If ui32TimeoutTicks
// occur prior to the state becoming idle or unconfigured, false is returned
// to indicate a timeout.
//
// \return Returns \b true on success or \b false on timeout.
//
//***************************************************************************
bool
WaitForSendIdle(uint32_t ui32TimeoutTicks)
{
    uint32_t ui32Start, ui32Now, ui32Elapsed;

    ui32Start = g_ui32SysTickCount;
    ui32Elapsed = 0;

    while(ui32Elapsed < ui32TimeoutTicks)
    {
        //
        // Is the mouse is idle or we have disconnected, return immediately.
        //
        if((g_iMouseState == eMouseStateIdle) ||
           (g_iMouseState == eMouseStateUnconfigured))
        {
            return(true);
        }

        //
        // Determine how much time has elapsed since we started waiting.  This
        // should be safe across a wrap of g_ui32SysTickCount.  I suspect you
        // won't likely leave the app running for the 497.1 days it will take
        // for this to occur but you never know...
        //
        ui32Now = g_ui32SysTickCount;
        ui32Elapsed = (ui32Start < ui32Now) ? (ui32Now - ui32Start) :
                           (((uint32_t)0xFFFFFFFF - ui32Start) + ui32Now + 1);
    }

    //
    // If we get here, we timed out so return a bad return code to let the
    // caller know.
    //
    return(false);
}

//*****************************************************************************
//
// This is the interrupt handler for the SysTick interrupt.  It is called
// periodically and updates a global tick counter then sets a flag to tell the
// main loop to check to see if a new HID report should be sent to the host.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    g_ui32SysTickCount++;
    g_ui32Commands |= UPDATE_TICK_EVENT;
}

//*****************************************************************************
//
// This function handles updates due to the touchscreen and buttons.
//
// This function is called from the main loop each time the touchscreen state
// needs to be checked.  If it detects an update it will schedule an transfer
// to the host.
//
// Returns Returns \b true on success or \b false if an error is detected.
//
//*****************************************************************************
bool
TouchEventHandler(void)
{
    uint32_t ui32Retcode;
    int32_t i32DeltaX, i32DeltaY;
    bool bSuccess;
    bool bBtnPressed;

    //
    // Assume all is well until we determine otherwise.
    //
    bSuccess = true;

    //
    // Get the current state of the select button.
    //
    bBtnPressed = (ROM_GPIOPinRead(SEL_BTN_PORT, SEL_BTN_PIN) & SEL_BTN_PIN) ?
                   false : true;

    //
    // Is someone pressing the screen or has the button changed state?  If so,
    // we determine how far they have dragged their finger/stylus and use this
    // to calculate mouse position changes to send to the host.
    //
    if(g_bScreenPressed || (bBtnPressed != g_bButtonPressed))
    {
        //
        // Calculate how far we moved since the last time we checked.  This
        // rather odd layout prevents a compiler warning about undefined order
        // of volatile accesses.
        //
        i32DeltaX = g_i32ScreenX;
        i32DeltaX -= g_i32ScreenStartX;
        i32DeltaY = g_i32ScreenY;
        i32DeltaY -= g_i32ScreenStartY;

        //
        // Reset our start position.
        //
        g_i32ScreenStartX = g_i32ScreenX;
        g_i32ScreenStartY = g_i32ScreenY;

        //
        // Was there any movement or change in button state?
        //
        if(i32DeltaX || i32DeltaY || (bBtnPressed != g_bButtonPressed))
        {
            //
            // Yes - send a report back to the host after clipping the deltas
            // to the maximum we can support.
            //
            i32DeltaX = (i32DeltaX > 127) ? 127 : i32DeltaX;
            i32DeltaX = (i32DeltaX < -128) ? -128 : i32DeltaX;
            i32DeltaY = (i32DeltaY > 127) ? 127 : i32DeltaY;
            i32DeltaY = (i32DeltaY < -128) ? -128 : i32DeltaY;

            //
            // Remember the current button state.
            //
            g_bButtonPressed = bBtnPressed;

            //
            // Send the report back to the host.
            //
            g_iMouseState = eMouseStateSend;
            ui32Retcode = USBDHIDMouseStateChange((void *)&g_sMouseDevice,
                                                (char)i32DeltaX, (char)i32DeltaY,
                                                (bBtnPressed ?
                                                 MOUSE_REPORT_BUTTON_1 : 0));

            //
            // Did we schedule the report for transmission?
            //
            if(ui32Retcode == MOUSE_SUCCESS)
            {
                //
                // Wait for the host to acknowledge the transmission if all went
                // well.
                //
                bSuccess = WaitForSendIdle(MAX_SEND_DELAY);

                //
                // Did we time out waiting for the packet to be sent?
                //
                if (!bSuccess)
                {
                    //
                    // Yes - assume the host disconnected and go back to
                    // waiting for a new connection.
                    //
                    UARTprintf("Send timed out!\n");
                    g_bConnected = 0;
                }
            }
            else
            {
                //
                // An error was reported when trying to send the report. This
                // may be due to host disconnection but could also be due to a
                // clash between our attempt to send a report and the driver
                // sending the last report in response to an idle timer timeout
                // so we don't jump to the conclusion that we were disconnected
                // in this case.
                //
                UARTprintf("Can't send report.\n");
                bSuccess = false;
            }
        }
    }

    return(bSuccess);
}

//*****************************************************************************
//
// This function is called by the touchscreen driver whenever there is a
// change in press state or position.
//
//*****************************************************************************
int32_t
DeviceMouseTouchCallback(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    switch(ui32Message)
    {
        //
        // The touchscreen has been pressed.  Remember where we are so that
        // we can determine how far the pointer moves later.
        //
        case WIDGET_MSG_PTR_DOWN:
            g_i32ScreenStartX = i32X;
            g_i32ScreenStartY = i32Y;
            g_i32ScreenX = i32X;
            g_i32ScreenY = i32Y;
            g_bScreenPressed = true;
            break;

        //
        // The touchscreen is no longer being pressed.
        //
        case WIDGET_MSG_PTR_UP:
            g_bScreenPressed = false;
            break;

        //
        // The user is dragging his/her finger/stylus over the touchscreen.
        //
        case WIDGET_MSG_PTR_MOVE:
            g_i32ScreenX = i32X;
            g_i32ScreenY = i32Y;
            break;
    }

    //
    // Tell the mouse driver we handled the message.
    //
    return(1);
}

//*****************************************************************************
//
// This function initializes the mouse in device mode.
//
//*****************************************************************************
void
DeviceInit(void)
{
    //
    // Initialize the touchscreen driver and install our event handler.
    //
    TouchScreenInit(g_ui32SysClock);
    TouchScreenCallbackSet(DeviceMouseTouchCallback);

    //
    // Set the system tick to fire 100 times per second.
    //
    ROM_SysTickPeriodSet(g_ui32SysClock / SYSTICKS_PER_SECOND);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //
    // Pass the USB library our device information, initialize the USB
    // controller and connect the device to the bus.
    //
    USBDHIDMouseInit(0, (tUSBDHIDMouseDevice *)&g_sMouseDevice);
}

//*****************************************************************************
//
// This is the main loop that runs the mouse device application.
//
//*****************************************************************************
void
DeviceMain(void)
{
    bool bRetcode;

    if(g_iMouseState == eMouseStateUnconfigured)
    {
        return;
    }

    //
    // If it is time to check the touchscreen state then do so.
    //
    if(g_ui32Commands & UPDATE_TICK_EVENT)
    {
        g_ui32Commands &= ~UPDATE_TICK_EVENT;
        TouchEventHandler();

        //
        // Wait for the last data to go out before sending more data.  Also
        // ensure that we drop out if the device is disconnected.
        //
        bRetcode = WaitForSendIdle(MAX_SEND_DELAY);

        //
        // If we timed out, assume the host disconnected and start looking
        // for a new connection.
        //
        if(!bRetcode)
        {
            g_iMouseState = eMouseStateUnconfigured;
        }
    }
}

//*****************************************************************************
//
// This function returns the number of ticks since the last time this function
// was called.
//
//*****************************************************************************
uint32_t
GetTickms(void)
{
    uint32_t ui32RetVal, ui32Saved;

    ui32RetVal = g_ui32SysTickCount;
    ui32Saved = ui32RetVal;

    if(ui32Saved > g_ui32LastTick)
    {
        ui32RetVal = ui32Saved - g_ui32LastTick;
    }
    else
    {
        ui32RetVal = g_ui32LastTick - ui32Saved;
    }

    //
    // This could miss a few milliseconds but the timings here are on a
    // much larger scale.
    //
    g_ui32LastTick = ui32Saved;

    //
    // Return the number of milliseconds since the last time this was called.
    //
    return(ui32RetVal * MS_PER_SYSTICK);
}
