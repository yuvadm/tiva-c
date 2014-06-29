//*****************************************************************************
//
// usb_host_mouse.c - main application code for the host mouse example.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C123G Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "grlib/grlib.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhhid.h"
#include "usblib/host/usbhhidmouse.h"
#include "utils/ustdlib.h"
#include "drivers/cfal96x64x16.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB HID Mouse Host (usb_host_mouse)</h1>
//!
//! This application demonstrates the handling of a USB mouse attached to the
//! evaluation kit.  Once attached, the position of the mouse pointer and the
//! state of the mouse buttons are output to the display.
//
//*****************************************************************************

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100
#define MS_PER_SYSTICK (1000 / TICKS_PER_SECOND)

//*****************************************************************************
//
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// Our running system tick counter and a global used to determine the time
// elapsed since last call to GetTickms().
//
//*****************************************************************************
uint32_t g_ui32SysTickCount;
uint32_t g_ui32LastTick;

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
// The size of the mouse device interface's memory pool in bytes.
//
//*****************************************************************************
#define MOUSE_MEMORY_SIZE       128

//*****************************************************************************
//
// These defines are used to define the screen constraints to the application.
//
//*****************************************************************************
#define DISPLAY_BANNER_HEIGHT   10
#define DISPLAY_BANNER_BG       ClrDarkBlue
#define DISPLAY_TEXT_BORDER     2
#define DISPLAY_TEXT_FG         ClrWhite
#define DISPLAY_TEXT_BG         ClrBlack

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
static int32_t g_i32CursorX;
static int32_t g_i32CursorY;

//*****************************************************************************
//
// The current USB operating mode - Host, Device or unknown.
//
//*****************************************************************************
tUSBMode g_eCurrentUSBMode;

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
    STATE_NO_DEVICE,

    //
    // Mouse has been detected and needs to be initialized in the main
    // loop.
    //
    STATE_MOUSE_INIT,

    //
    // Mouse is connected and waiting for events.
    //
    STATE_MOUSE_CONNECTED,

    //
    // An unsupported device has been attached.
    //
    STATE_UNKNOWN_DEVICE,

    //
    // A power fault has occured.
    //
    STATE_POWER_FAULT
}
g_eUSBState;

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
// This is the generic callback from host stack.
//
// pvData is actually a pointer to a tEventInfo structure.
//
// This function will be called to inform the application when a USB event has
// occurred that is outside those related to the mouse device.  At this
// point this is used to detect unsupported devices being inserted and removed.
// It is also used to inform the application when a power fault has occurred.
// This function is required when the g_USBGenericEventDriver is included in
// the host controller driver array that is passed in to the
// USBHCDRegisterDrivers() function.
//
//*****************************************************************************
void
USBHCDEvents(void *pvData)
{
    tEventInfo *pEventInfo;
    tRectangle sRect;

    //
    // Fill the bottom rows of the screen with blue to create the status area.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = GrContextDpyHeightGet(&g_sContext) -
                  DISPLAY_BANNER_HEIGHT - 1;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMax = sRect.i16YMin + DISPLAY_BANNER_HEIGHT;

    GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_BG);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Set the font for the status message
    //
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);

    //
    // Cast this pointer to its actual type.
    //
    pEventInfo = (tEventInfo *)pvData;

    switch(pEventInfo->ui32Event)
    {
        //
        // New mouse detected.
        //
        case USB_EVENT_CONNECTED:
        {
            //
            // See if this is a HID Mouse.
            //
            if((USBHCDDevClass(pEventInfo->ui32Instance, 0) == USB_CLASS_HID) &&
               (USBHCDDevProtocol(pEventInfo->ui32Instance, 0) ==
                USB_HID_PROTOCOL_MOUSE))
            {
                //
                // Indicate that the mouse has been detected.
                //
                GrStringDrawCentered(&g_sContext, "Mouse Connected", -1,
                                     GrContextDpyWidthGet(&g_sContext) / 2,
                                     sRect.i16YMin + 5, 0);

                //
                // Set initial mouse information.
                //
                GrStringDrawCentered(&g_sContext, "0,0", -1,
                                     GrContextDpyWidthGet(&g_sContext) / 2, 26, 1);
                GrStringDrawCentered(&g_sContext, "000", -1,
                                     GrContextDpyWidthGet(&g_sContext) / 2, 46, 1);

                //
                // Proceed to the STATE_MOUSE_INIT state so that the main loop
                // can finish initialized the mouse since USBHMouseInit()
                // cannot be called from within a callback.
                //
                g_eUSBState = STATE_MOUSE_INIT;
            }

            break;
        }
        //
        // Unsupported device detected.
        //
        case USB_EVENT_UNKNOWN_CONNECTED:
        {
            GrStringDrawCentered(&g_sContext, "Unknown Device", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2,
                                 sRect.i16YMin + 5, 0);
            //
            // An unknown device was detected.
            //
            g_eUSBState = STATE_UNKNOWN_DEVICE;

            break;
        }
        //
        // Device has been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // Indicate that the device has been disconnected.
            //
            GrStringDrawCentered(&g_sContext, "No Device", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2,
                                 sRect.i16YMin + 5, 0);

            //
            // Reset the mouse information.
            //
            GrStringDrawCentered(&g_sContext, "   -,-   ", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2, 26, 1);
            GrStringDrawCentered(&g_sContext, "---", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2, 46, 1);

            //
            // Change the state so that the main loop knows that the device is
            // no longer present.
            //
            g_eUSBState = STATE_NO_DEVICE;

            //
            // Reset the button state.
            //
            g_ui32Buttons = 0;

            break;
        }
        //
        // Power Fault has occurred.
        //
        case USB_EVENT_POWER_FAULT:
        {
            //
            // Indicate that there has been a power fault.
            //
            GrStringDrawCentered(&g_sContext, "Power Fault", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2,
                                 sRect.i16YMin + 5, 0);

            //
            // Reset the mouse information.
            //
            GrStringDrawCentered(&g_sContext, "   -,-   ", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2, 26, 1);
            GrStringDrawCentered(&g_sContext, "---", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2, 46, 1);

            //
            // No power means no device is present.
            //
            g_eUSBState = STATE_POWER_FAULT;

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
// This is the handler for this SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Update our tick counter.
    //
    g_ui32SysTickCount++;
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
    uint32_t ui32RetVal;
    uint32_t ui32Saved;

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

//*****************************************************************************
//
// USB Mode callback
//
// \param ui32Index is the zero-based index of the USB controller making the
//        callback.
// \param eMode indicates the new operating mode.
//
// This function is called by the USB library whenever an OTG mode change
// occurs and, if a connection has been made, informs us of whether we are to
// operate as a host or device.
//
// \return None.
//
//*****************************************************************************
void
ModeCallback(uint32_t ui32Index, tUSBMode eMode)
{
    //
    // Save the new mode.
    //
    g_eCurrentUSBMode = eMode;
}

//*****************************************************************************
//
// This is the callback from the USB HID mouse handler.
//
// \param psMsInstance is ignored by this function.
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
    int32_t i32DoUpdate;
    char pcBuffer[20];

    //
    // Do an update unless there is no reason to.
    //
    i32DoUpdate = 1;

    switch(ui32Event)
    {
        //
        // Mouse button press detected.
        //
        case USBH_EVENT_HID_MS_PRESS:
        {
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
            //
            // Update the cursor X position.
            //
            g_i32CursorX += (int8_t)ui32MsgParam;

            //
            // Cap the value to not cause an overflow.
            //
            if(g_i32CursorX > 9999)
            {
                g_i32CursorX = 9999;
            }

            if(g_i32CursorX < -9999)
            {
                g_i32CursorX = -9999;
            }

            break;
        }

        //
        // Mouse Y movement detected.
        //
        case USBH_EVENT_HID_MS_Y:
        {
            //
            // Update the cursor Y position.
            //
            g_i32CursorY += (int8_t)ui32MsgParam;

            //
            // Cap the value to not cause an overflow.
            //
            if(g_i32CursorY > 9999)
            {
                g_i32CursorY = 9999;
            }

            if(g_i32CursorY < -9999)
            {
                g_i32CursorY = -9999;
            }

            break;
        }
        default:
        {
            //
            // No reason to update.
            //
            i32DoUpdate = 0;

            break;
        }
    }

    //
    // Display the current mouse position and button state if there was an
    // update.
    //
    if(i32DoUpdate)
    {
        GrStringDrawCentered(&g_sContext, "Position:", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2, 16, 0);
        usprintf(pcBuffer, "   %d,%d   ", g_i32CursorX, g_i32CursorY);
        GrStringDrawCentered(&g_sContext, pcBuffer, -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2, 26, 1);
        GrStringDrawCentered(&g_sContext, "Buttons:", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2, 36, 0);
        usprintf(pcBuffer, "%d%d%d", g_ui32Buttons & 1, (g_ui32Buttons & 2) >> 1,
                 (g_ui32Buttons & 4) >> 2);
        GrStringDrawCentered(&g_sContext, pcBuffer, -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2, 46, 1);
    }
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int
main(void)
{
    tRectangle sRect;

    //
    // Set the clocking to run from the PLL at 50MHz.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Initialize the display driver.
    //
    CFAL96x64x16Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sCFAL96x64x16);

    //
    // Fill the top part of the screen with blue to create the banner.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMax = (DISPLAY_BANNER_HEIGHT) - 1;
    GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_BG);
    GrRectFill(&g_sContext, &sRect);

    //
    // Change foreground for white text.
    //
    GrContextForegroundSet(&g_sContext, DISPLAY_TEXT_FG);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);
    GrStringDrawCentered(&g_sContext, "usb-host-mouse", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 4, 0);

    //
    // Display default information about the mouse
    //
    GrStringDrawCentered(&g_sContext, "Position:", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2, 16, 0);
    GrStringDrawCentered(&g_sContext, "-,-", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2, 26, 1);
    GrStringDrawCentered(&g_sContext, "Buttons:", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2, 36, 0);
    GrStringDrawCentered(&g_sContext, "---", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2, 46, 1);

    //
    // Fill the bottom rows of the screen with blue to create the status area.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = GrContextDpyHeightGet(&g_sContext) -
                  DISPLAY_BANNER_HEIGHT - 1;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMax = sRect.i16YMin + DISPLAY_BANNER_HEIGHT;

    GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_BG);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Print a no device message a placeholder for the message printed
    // during an event.
    //
    GrStringDrawCentered(&g_sContext, "No Device", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2,
                         sRect.i16YMin + 5, 0);

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable Clocking to the USB controller.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);

    //
    // Configure the required pins for USB operation.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    ROM_GPIOPinConfigure(GPIO_PG4_USB0EPEN);
    ROM_GPIOPinTypeUSBDigital(GPIO_PORTG_BASE, GPIO_PIN_4);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initially wait for device connection.
    //
    g_eUSBState = STATE_NO_DEVICE;

    //
    // Initialize the USB stack mode and pass in a mode callback.
    //
    USBStackModeSet(0, eUSBModeOTG, ModeCallback);

    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ui32NumHostClassDrivers);

    //
    // Initialized the cursor.
    //
    g_ui32Buttons = 0;
    g_i32CursorX = 0;
    g_i32CursorY = 0;

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
    // Initialize the USB controller for OTG operation with a 2ms polling
    // rate.
    //
    USBOTGModeInit(0, 2000, g_pui8HCDPool, HCD_MEMORY_SIZE);

    //
    // The main loop for the application.
    //
    while(1)
    {
        //
        // Tell the OTG state machine how much time has passed in
        // milliseconds since the last call.
        //
        USBOTGMain(GetTickms());

        switch(g_eUSBState)
        {
            //
            // This state is entered when the mouse is first detected.
            //
            case STATE_MOUSE_INIT:
            {
                //
                // Initialize the newly connected mouse.
                //
                USBHMouseInit(g_psMouseInstance);

                //
                // Proceed to the mouse connected state.
                //
                g_eUSBState = STATE_MOUSE_CONNECTED;

                break;
            }

            case STATE_MOUSE_CONNECTED:
            {
                //
                // Nothing is currently done in the main loop when the mouse
                // is connected.
                //
                break;
            }

            case STATE_NO_DEVICE:
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
    }
}
