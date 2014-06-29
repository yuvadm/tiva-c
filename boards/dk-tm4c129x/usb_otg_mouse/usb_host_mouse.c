//*****************************************************************************
//
// usb_host_mouse.c - main application code for the host mouse example.
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
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhhid.h"
#include "usblib/host/usbhhidmouse.h"
#include "utils/uartstdio.h"
#include "usb_otg_mouse.h"

//*****************************************************************************
//
// The size of the mouse device interface's memory pool in bytes.
//
//*****************************************************************************
#define MOUSE_MEMORY_SIZE       128

//*****************************************************************************
//
// The memory pool to provide to the mouse device.
//
//*****************************************************************************
uint8_t g_pui8Buffer[MOUSE_MEMORY_SIZE];

//*****************************************************************************
//
// Declare the USB Events driver interface.
//
//*****************************************************************************
DECLARE_EVENT_DRIVER(g_sUSBEventDriver, 0, 0, USBHCDEvents);

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the Mouse class is loaded.
//
//*****************************************************************************
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
    &g_sUSBHIDClassDriver,
    &g_sUSBEventDriver
};

//*****************************************************************************
//
// This global holds the number of class drivers in the g_ppHostClassDrivers
// list.
//
//*****************************************************************************
static const uint32_t g_ui32NumHostClassDrivers =
    sizeof(g_ppHostClassDrivers) / sizeof(tUSBHostClassDriver *);

//*****************************************************************************
//
// The global value used to store the mouse instance value.
//
//*****************************************************************************
static tUSBHMouse *g_psMouseInstance;

//*****************************************************************************
//
// The global values used to store the mouse state.
//
//*****************************************************************************
static uint32_t g_ui32Buttons;
static tRectangle g_sCursor;

//*****************************************************************************
//
// This enumerated type is used to hold the states of the mouse.
//
//*****************************************************************************
enum
{
    //
    // No device is present.
    //
    eStateNoDevice,

    //
    // Mouse has been detected and needs to be initialized in the main
    // loop.
    //
    eStateMouseInit,

    //
    // Mouse is connected and waiting for events.
    //
    eStateMouseConnected,

    //
    // An unsupported device has been attached.
    //
    eStateUnknownDevice,

    //
    // A power fault has occurred.
    //
    eStatePowerFault
}
iUSBState;

//*****************************************************************************
//
// These defines are used to define the screen constraints to the application.
//
//*****************************************************************************
#define DISPLAY_BANNER_HEIGHT   20
#define DISPLAY_BANNER_BG       ClrDarkBlue
#define DISPLAY_BANNER_FG       ClrWhite
#define DISPLAY_MOUSE_BG        ClrBlack
#define DISPLAY_MOUSE_FG        ClrWhite
#define DISPLAY_MOUSE_SIZE      2

//*****************************************************************************
//
// This function clears the main application screen area.
//
//*****************************************************************************
void
ClearMainWindow(void)
{
    tRectangle sRect;

    //
    // Initialize the button indicator.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = DISPLAY_BANNER_HEIGHT + 1;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMax = GrContextDpyHeightGet(&g_sContext) - DISPLAY_BANNER_HEIGHT;

    GrContextForegroundSet(&g_sContext, DISPLAY_MOUSE_BG);
    GrRectFill(&g_sContext, &sRect);
    GrContextForegroundSet(&g_sContext, DISPLAY_MOUSE_FG);
}

//*****************************************************************************
//
// This function updates the cursor position based on deltas received from
// the mouse device.
//
// \param i32XDelta is the signed movement in the X direction.
// \param i32YDelta is the signed movement in the Y direction.
//
// This function is called by the mouse handler code when it detects a change
// in the position of the mouse.  It will take the inputs and force them
// to be constrained to the display area of the screen.  If the left mouse
// button is pressed then the mouse will draw on the screen and if it is not
// it will move around normally.  A side effect of not being able to read the
// current state of the screen is that the cursor will erase anything it moves
// over while the left mouse button is not pressed.
//
// \return None.
//
//*****************************************************************************
void
UpdateCursor(int32_t i32XDelta, int32_t i32YDelta)
{
    int32_t i32Temp;

    //
    // If the left button is not pressed then erase the previous cursor
    // position.
    //
    if((g_ui32Buttons & 1) == 0)
    {
        //
        // Erase the previous cursor.
        //
        GrContextForegroundSet(&g_sContext, DISPLAY_MOUSE_BG);
        GrRectFill(&g_sContext, &g_sCursor);
    }

    //
    // Need to do signed math so use the temporary signed value.
    //
    i32Temp = g_sCursor.i16XMin;

    //
    // Update the X position without going off the screen.
    //
    if(((int)g_sCursor.i16XMin + i32XDelta + DISPLAY_MOUSE_SIZE) <
       GrContextDpyWidthGet(&g_sContext))
    {
        //
        // Update the X cursor position.
        //
        i32Temp += i32XDelta;

        //
        // Don't let the cursor go off the left of the screen either.
        //
        if(i32Temp < 0)
        {
            i32Temp = 0;
        }
    }

    //
    // Update the X position.
    //
    g_sCursor.i16XMin = i32Temp;
    g_sCursor.i16XMax = i32Temp + DISPLAY_MOUSE_SIZE;

    //
    // Need to do signed math so use the temporary signed value.
    //
    i32Temp = g_sCursor.i16YMin;

    //
    // Update the Y position without going off the screen.
    //
    if(((int)g_sCursor.i16YMin + i32YDelta) <
       (GrContextDpyHeightGet(&g_sContext) -
        DISPLAY_BANNER_HEIGHT - DISPLAY_MOUSE_SIZE - 1))
    {
        //
        // Update the Y cursor position.
        //
        i32Temp += i32YDelta;

        //
        // Don't let the cursor overwrite the status area of the screen.
        //
        if(i32Temp < DISPLAY_BANNER_HEIGHT + 1)
        {
            i32Temp = DISPLAY_BANNER_HEIGHT + 1;
        }
    }

    //
    // Update the Y position.
    //
    g_sCursor.i16YMin = i32Temp;
    g_sCursor.i16YMax = i32Temp + DISPLAY_MOUSE_SIZE;

    //
    // Draw the new cursor.
    //
    GrContextForegroundSet(&g_sContext, DISPLAY_MOUSE_FG);
    GrRectFill(&g_sContext, &g_sCursor);
}

//*****************************************************************************
//
// This function will update the small mouse button indicators in the status
// bar area of the screen.  This can be called on its own or it will be called
// whenever UpdateStatus() is called as well.
//
//*****************************************************************************
void
UpdateButtons(void)
{
    tRectangle sRect, sRectInner;
    int iButton;

    //
    // Initialize the button indicator position.
    //
    sRect.i16XMin = GrContextDpyWidthGet(&g_sContext) - 36;
    sRect.i16YMin = GrContextDpyHeightGet(&g_sContext) - 18;
    sRect.i16XMax = sRect.i16XMin + 6;
    sRect.i16YMax = sRect.i16YMin + 8;
    sRectInner.i16XMin = sRect.i16XMin + 1;
    sRectInner.i16YMin = sRect.i16YMin + 1;
    sRectInner.i16XMax = sRect.i16XMax - 1;
    sRectInner.i16YMax = sRect.i16YMax - 1;

    //
    // Check all three buttons.
    //
    for(iButton = 0; iButton < 3; iButton++)
    {
        //
        // Draw the button indicator red if pressed and black if not pressed.
        //
        if(g_ui32Buttons & (1 << iButton))
        {
            GrContextForegroundSet(&g_sContext, ClrRed);
        }
        else
        {
            GrContextForegroundSet(&g_sContext, ClrBlack);
        }

        //
        // Draw the back of the  button indicator.
        //
        GrRectFill(&g_sContext, &sRectInner);

        //
        // Draw the border on the button indicator.
        //
        GrContextForegroundSet(&g_sContext, ClrWhite);
        GrRectDraw(&g_sContext, &sRect);

        //
        // Move to the next button indicator position.
        //
        sRect.i16XMin += 8;
        sRect.i16XMax += 8;
        sRectInner.i16XMin += 8;
        sRectInner.i16XMax += 8;
    }
}

//*****************************************************************************
//
// This function updates the status area of the screen.  It uses the current
// state of the application to print the status bar.
//
//*****************************************************************************
void
UpdateStatus(char *pcString, uint32_t ui32Buttons, bool bClrBackground)
{
    tRectangle sRect;

    //
    // Fill the bottom rows of the screen with blue to create the status area.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = GrContextDpyHeightGet(&g_sContext) -
                  DISPLAY_BANNER_HEIGHT - 1;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMax = sRect.i16YMin + DISPLAY_BANNER_HEIGHT;

    //
    // Were we asked to clear the background of the status area?
    //
    GrContextBackgroundSet(&g_sContext, DISPLAY_BANNER_BG);

    if(bClrBackground)
    {
        //
        // Draw the background of the banner.
        //
        GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_BG);
        GrRectFill(&g_sContext, &sRect);

        //
        // Put a white box around the banner.
        //
        GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_FG);
        GrRectDraw(&g_sContext, &sRect);
    }

    //
    // Write the current state to the left of the status area.
    //
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);

    //
    // Update the status on the screen.
    //
    if(pcString != 0)
    {
        UARTprintf(pcString);
        UARTprintf("\n");
        GrStringDraw(&g_sContext, pcString, -1, 10, sRect.i16YMin + 4, 1);

        g_ui32Buttons = ui32Buttons;
    }
    else if(iUSBState == eStateNoDevice)
    {
        //
        // Mouse is currently disconnected.
        //
        UARTprintf("no device\n");
        GrStringDraw(&g_sContext, "no device     ", -1, 10, sRect.i16YMin + 4,
                     1);
    }
    else if(iUSBState == eStateMouseConnected)
    {
        //
        // Mouse is connected.
        //
        UARTprintf("connected\n");
        GrStringDraw(&g_sContext, "connected     ", -1, 10, sRect.i16YMin + 4,
                     1);
    }
    else if(iUSBState == eStateUnknownDevice)
    {
        //
        // Some other (unknown) device is connected.
        //
        UARTprintf("unknown device\n");
        GrStringDraw(&g_sContext, "unknown device", -1, 10, sRect.i16YMin + 4,
                     1);
    }
    else if(iUSBState == eStatePowerFault)
    {
        //
        // Power fault.
        //
        UARTprintf("power fault\n");
        GrStringDraw(&g_sContext, "power fault   ", -1, 10, sRect.i16YMin + 4,
                     1);
    }

    UpdateButtons();
}

//*****************************************************************************
//
// This is the generic callback from host stack.
//
// \param pvData is actually a pointer to a tEventInfo structure.
//
// This function will be called to inform the application when a USB event has
// occurred that is outside those related to the mouse device.  At this
// point this is used to detect unsupported devices being inserted and removed.
// It is also used to inform the application when a power fault has occurred.
// This function is required when the g_USBGenericEventDriver is included in
// the host controller driver array that is passed in to the
// USBHCDRegisterDrivers() function.
//
// \return None.
//
//*****************************************************************************
void
USBHCDEvents(void *pvData)
{
    tEventInfo *psEventInfo;

    //
    // Cast this pointer to its actual type.
    //
    psEventInfo = (tEventInfo *)pvData;

    switch(psEventInfo->ui32Event)
    {
        //
        // New mouse detected.
        //
        case USB_EVENT_CONNECTED:
        {
            //
            // See if this is a HID Keyboard.
            //
            if((USBHCDDevClass(psEventInfo->ui32Instance, 0) ==
                USB_CLASS_HID) &&
               (USBHCDDevProtocol(psEventInfo->ui32Instance, 0) ==
                USB_HID_PROTOCOL_MOUSE))
            {
                //
                // Indicate that the mouse has been detected.
                //
                UARTprintf("Mouse Connected\n");

                //
                // Proceed to the eStateMouseInit state so that the main loop
                // can finish initialized the mouse since USBHMouseInit()
                // cannot be called from within a callback.
                //
                iUSBState = eStateMouseInit;
            }

            break;
        }
        //
        // Unsupported device detected.
        //
        case USB_EVENT_UNKNOWN_CONNECTED:
        {
            UARTprintf("Unsupported Device Connected\n");

            //
            // An unknown device was detected.
            //
            iUSBState = eStateUnknownDevice;

            break;
        }
        //
        // Device has been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // Indicate that the mouse has been disconnected.
            //
            UARTprintf("Device Disconnected\n");

            //
            // Change the state so that the main loop knows that the mouse is
            // no longer present.
            //
            iUSBState = eStateNoDevice;

            //
            // Reset the button state.
            //
            g_ui32Buttons = 0;

            break;
        }
        //
        // Power Fault.
        //
        case USB_EVENT_POWER_FAULT:
        {
            UARTprintf("Power Fault\n");

            //
            // No power means no device is present.
            //
            iUSBState = eStatePowerFault;

            break;
        }
        default:
        {
            break;
        }
    }
}

//*****************************************************************************
//
// This is the callback from the USB HID mouse handler.
//
// \param pvCBData is ignored by this function.
// \param ui32Event is one of the valid events for a mouse device.
// \param ui32MsgParam is defined by the event that occurs.
// \param pvMsgData is a pointer to data that is defined by the event that
// occurs.
//
// This function will be called to inform the application when a mouse has
// been plugged in or removed and any time mouse movement or button pressed
// is detected.
//
// \return None.
//
//*****************************************************************************
void
MouseCallback(tUSBHMouse *psMsInstance, uint32_t ui32Event,
              uint32_t ui32MsgParam, void *pvMsgData)
{
    switch(ui32Event)
    {
        //
        // Mouse button press detected.
        //
        case USBH_EVENT_HID_MS_PRESS:
        {
            UARTprintf("Button Pressed %02x\n", ui32MsgParam);

            //
            // Save the new button that was pressed.
            //
            g_ui32Buttons |= ui32MsgParam;

            break;
        }

        //
        // Mouse button release detected.
        //
        case USBH_EVENT_HID_MS_REL:
        {
            UARTprintf("Button Released %02x\n", ui32MsgParam);

            //
            // Remove the button from the pressed state.
            //
            g_ui32Buttons &= ~ui32MsgParam;

            break;
        }

        //
        // Mouse X movement detected.
        //
        case USBH_EVENT_HID_MS_X:
        {
            UARTprintf("X:%02d.\n", (signed char)ui32MsgParam);

            //
            // Update the cursor on the screen.
            //
            UpdateCursor((signed char)ui32MsgParam, 0);

            break;
        }

        //
        // Mouse Y movement detected.
        //
        case USBH_EVENT_HID_MS_Y:
        {
            UARTprintf("Y:%02d.\n", (signed char)ui32MsgParam);

            //
            // Update the cursor on the screen.
            //
            UpdateCursor(0, (signed char)ui32MsgParam);

            break;
        }
    }

    //
    // Update the status area of the screen.
    //
    UpdateStatus(0, 0, false);
}

//*****************************************************************************
//
// Initialize the host mode stack.
//
//*****************************************************************************
void
HostInit(void)
{
    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ui32NumHostClassDrivers);

    //
    // Initialize the button states.
    //
    g_ui32Buttons = 0;

    //
    // Update the status on the screen.
    //
    UpdateStatus(0, 0, true);

    //
    // Open an instance of the mouse driver.  The mouse does not need
    // to be present at this time, this just saves a place for it and allows
    // the applications to be notified when a mouse is present.
    //
    g_psMouseInstance =
        USBHMouseOpen(MouseCallback, g_pui8Buffer, MOUSE_MEMORY_SIZE);

    //
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);

    //
    // Call the main loop for the Host controller driver.
    //
    iUSBState = eStateNoDevice;
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
void
HostMain(void)
{
    switch(iUSBState)
    {
        //
        // This state is entered when the mouse is first detected.
        //
        case eStateMouseInit:
        {
            //
            // Initialize the newly connected mouse.
            //
            USBHMouseInit(g_psMouseInstance);

            //
            // Proceed to the mouse connected state.
            //
            iUSBState = eStateMouseConnected;

            //
            // Update the status on the screen.
            //
            UpdateStatus(0, 0, true);

            //
            // Update the cursor on the screen.
            //
            UpdateCursor(GrContextDpyWidthGet(&g_sContext) / 2,
                         GrContextDpyHeightGet(&g_sContext)/ 2);

            break;
        }
        case eStateMouseConnected:
        {
            //
            // Nothing is currently done in the main loop when the mouse
            // is connected.
            //
            break;
        }
        case eStateNoDevice:
        {
            //
            // The mouse is not connected so nothing needs to be done here.
            //
            break;
        }
        default:
        {
            break;
        }
    }

    //
    // Periodically call the main loop for the Host controller driver.
    //
    USBHCDMain();
}
