//*****************************************************************************
//
// usb_host_keyboard.c - An example using that supports a keyboard.
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
#include <string.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhhid.h"
#include "usblib/host/usbhhidkeyboard.h"
#include "drivers/pinout.h"
#include "keyboard_ui.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Host keyboard example(usb_host_keyboard)</h1>
//!
//! This example application demonstrates how to support a USB keyboard using
//! the DK-TM4C129X development board.  This application supports only a
//! standard keyboard HID device, but can report on the types of other devices
//! that are connected without having the ability to access them.  Key presses
//! are shown on the display as well as the caps-lock, scroll-lock, and
//! num-lock states of the keyboard.  The bottom left status bar reports the
//! type of device attached.  The user interface for the application is
//! handled in the keyboard_ui.c file while the usb_host_keyboard.c file
//! handles start up and the USB interface.
//!
//! The application can be recompiled to run using and external USB phy to
//! implement a high speed host using an external USB phy.  To use the external
//! phy the application must be built with \b USE_ULPI defined.  This disables
//! the internal phy and the connector on the DK-TM4C129X board and enables the
//! connections to the external ULPI phy pins on the DK-TM4C129X board.
//
//*****************************************************************************

//*****************************************************************************
//
// The size of the host controller's memory pool in bytes.
//
//*****************************************************************************
#define HCD_MEMORY_SIZE         128

//*****************************************************************************
//
// The global keyboard status structure.
//
//*****************************************************************************
tKeyboardStatus g_sStatus;

//*****************************************************************************
//
// The memory pool to provide to the Host controller driver.
//
//*****************************************************************************
uint8_t g_pui8HCDPool[HCD_MEMORY_SIZE * MAX_USB_DEVICES];

//*****************************************************************************
//
// Declare the USB Events driver interface.
//
//*****************************************************************************
DECLARE_EVENT_DRIVER(g_sUSBEventDriver, 0, 0, USBHCDEvents);

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the Keyboard class is loaded.
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
// The global value used to store the keyboard instance value.
//
//*****************************************************************************
static tUSBHKeyboard *g_psKeyboard;

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
// This enumerated type is used to hold the states of the keyboard.
//
//*****************************************************************************
enum
{
    //
    // No device is present.
    //
    eStateNoDevice,

    //
    // Keyboard has been detected and needs to be initialized in the main
    // loop.
    //
    eStateKeyboardInit,

    //
    // Keyboard is connected and waiting for events.
    //
    eStateKeyboardConnected,

    //
    // Keyboard has received a key press that requires updating the keyboard
    // in the main loop.
    //
    eStateKeyboardUpdate,
}
g_iKeyboardState;

//*****************************************************************************
//
// This is the callback from the USB HID keyboard handler.
//
// pvCBData is ignored by this function.
// ui32Event is one of the valid events for a keyboard device.
// ui32MsgParam is defined by the event that occurs.
// pvMsgData is a pointer to data that is defined by the event that
// occurs.
//
// This function will be called to inform the application when a keyboard has
// been plugged in or removed and any time a key is pressed or released.
//
// This function will return 0.
//
//*****************************************************************************
void
KeyboardCallback(tUSBHKeyboard *psKbInstance, uint32_t ui32Event,
                 uint32_t ui32MsgParam, void *pvMsgData)
{
    char cChar;

    switch(ui32Event)
    {
        //
        // New keyboard detected.
        //
        case USB_EVENT_CONNECTED:
        {
            //
            // Proceed to the STATE_KEYBOARD_INIT state so that the main loop
            // can finish initialized the mouse since USBHKeyboardInit() cannot
            // be called from within a callback.
            //
            g_iKeyboardState = eStateKeyboardInit;

            break;
        }

        //
        // Keyboard has been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // Change the state so that the main loop knows that a device is no
            // longer present.
            //
            g_iKeyboardState = eStateNoDevice;

            break;
        }

        //
        // New Key press detected.
        //
        case USBH_EVENT_HID_KB_PRESS:
        {
            if(ui32MsgParam == HID_KEYB_USAGE_CAPSLOCK)
            {
                //
                // The main loop needs to update the device state.
                //
                g_iKeyboardState = eStateKeyboardUpdate;

                //
                // Toggle the current Caps Lock state.
                //
                g_sStatus.ui32Modifiers ^= HID_KEYB_CAPS_LOCK;
            }
            else if(ui32MsgParam == HID_KEYB_USAGE_SCROLLOCK)
            {
                //
                // The main loop needs to update the device state.
                //
                g_iKeyboardState = eStateKeyboardUpdate;

                //
                // Toggle the current Scroll Lock state.
                //
                g_sStatus.ui32Modifiers ^= HID_KEYB_SCROLL_LOCK;
            }
            else if(ui32MsgParam == HID_KEYB_USAGE_NUMLOCK)
            {
                //
                // The main loop needs to update the device state.
                //
                g_iKeyboardState = eStateKeyboardUpdate;

                //
                // Toggle the current Num Lock state.
                //
                g_sStatus.ui32Modifiers ^= HID_KEYB_NUM_LOCK;
            }
            else
            {
                //
                // Was this the backspace key?
                //
                if((uint8_t)ui32MsgParam == HID_KEYB_USAGE_BACKSPACE)
                {
                    //
                    // Yes - set the ASCII code for a backspace key.  This is
                    // not returned by USBHKeyboardUsageToChar since this only
                    // returns printable characters.
                    //
                    cChar = ASCII_BACKSPACE;
                }
                else
                {
                    //
                    // This is not backspace so try to map the usage code to a
                    // printable ASCII character.
                    //
                    cChar = (char)USBHKeyboardUsageToChar(g_psKeyboard,
                                                        &g_sUSKeyboardMap,
                                                        (uint8_t)ui32MsgParam);
                }

                //
                // A zero value indicates there was no textual mapping of this
                // usage code.
                //
                if(cChar != 0)
                {
                    UIPrintChar(cChar);
                }
            }
            break;
        }
        case USBH_EVENT_HID_KB_MOD:
        {
            //
            // This application ignores the state of the shift or control
            // and other special keys.
            //
            break;
        }
        case USBH_EVENT_HID_KB_REL:
        {
            //
            // This applications ignores the release of keys as well.
            //
            break;
        }
    }
}

//*****************************************************************************
//
// The main routine for handling the USB keyboard.
//
//*****************************************************************************
void
KeyboardMain(void)
{
    switch(g_iKeyboardState)
    {
        //
        // This state is entered when they keyboard is first detected.
        //
        case eStateKeyboardInit:
        {
            //
            // Initialized the newly connected keyboard.
            //
            USBHKeyboardInit(g_psKeyboard);

            //
            // Proceed to the keyboard connected state.
            //
            g_iKeyboardState = eStateKeyboardConnected;

            //
            // Set the current state of the modifiers.
            //
            USBHKeyboardModifierSet(g_psKeyboard, g_sStatus.ui32Modifiers);

            break;
        }
        case eStateKeyboardUpdate:
        {
            //
            // If the application detected a change that required an
            // update to be sent to the keyboard to change the modifier
            // state then call it and return to the connected state.
            //
            g_iKeyboardState = eStateKeyboardConnected;

            USBHKeyboardModifierSet(g_psKeyboard, g_sStatus.ui32Modifiers);

            //
            // Update the modifier status.
            //
            UIUpdateStatus();

            break;
        }
        case eStateKeyboardConnected:
        default:
        {
            break;
        }
    }
}

//*****************************************************************************
//
// This is the generic callback from host stack.
//
// pvData is actually a pointer to a tEventInfo structure.
//
// This function will be called to inform the application when a USB event has
// occurred that is outside those related to the keyboard device.  At this
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

    //
    // Cast this pointer to its actual type.
    //
    pEventInfo = (tEventInfo *)pvData;

    switch(pEventInfo->ui32Event)
    {
        case USB_EVENT_UNKNOWN_CONNECTED:
        case USB_EVENT_CONNECTED:
        {
            //
            // Save the device instance data.
            //
            g_sStatus.ui32Instance = pEventInfo->ui32Instance;
            g_sStatus.bConnected = true;

            //
            // Update the port status for the new device.
            //
            UIUpdateStatus();

            break;
        }
        //
        // A device has been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // Device is no longer connected.
            //
            g_sStatus.bConnected = false;

            //
            // Update the port status for the new device.
            //
            UIUpdateStatus();

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
// The main application loop.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32SysClock, ui32PLLRate;
#ifdef USE_ULPI
    uint32_t ui32Setting;
#endif

    //
    // Set the application to run at 120 MHz with a PLL frequency of 480 MHz.
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Set the part pin out appropriately for this device.
    //
    PinoutSet();

#ifdef USE_ULPI
    //
    // Switch the USB ULPI Pins over.
    //
    USBULPIPinoutSet();

    //
    // Enable USB ULPI with high speed support.
    //
    ui32Setting = USBLIB_FEATURE_ULPI_HS;
    USBOTGFeatureSet(0, USBLIB_FEATURE_USBULPI, &ui32Setting);

    //
    // Setting the PLL frequency to zero tells the USB library to use the
    // external USB clock.
    //
    ui32PLLRate = 0;
#else
    //
    // Save the PLL rate used by this application.
    //
    ui32PLLRate = 480000000;
#endif

    //
    // Initialize the connection status.
    //
    g_sStatus.bConnected = false;

    //
    // Initially there are no modifiers set.
    //
    g_sStatus.ui32Modifiers = 0;

    //
    // Enable Clocking to the USB controller.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

    //
    // Initialize the USB stack mode and pass in a mode callback.
    //
    USBStackModeSet(0, eUSBModeHost, 0);

    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ui32NumHostClassDrivers);

    //
    // Open an instance of the keyboard driver.  The keyboard does not need
    // to be present at this time, this just save a place for it and allows
    // the applications to be notified when a keyboard is present.
    //
    g_psKeyboard = USBHKeyboardOpen(KeyboardCallback, 0, 0);

    //
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);

    //
    // Tell the USB library the CPU clock and the PLL frequency.  This is a
    // new requirement for TM4C129 devices.
    //
    USBHCDFeatureSet(0, USBLIB_FEATURE_CPUCLK, &ui32SysClock);
    USBHCDFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

    //
    // Initialize the USB controller for Host mode.
    //
    USBHCDInit(0, g_pui8HCDPool, sizeof(g_pui8HCDPool));

    //
    // Initialize the GUI elements.
    //
    UIInit(ui32SysClock);

    //
    // The main loop for the application.
    //
    while(1)
    {
        //
        // Call the USB library to let non-interrupt code run.
        //
        USBHCDMain();

        //
        // Call the keyboard and mass storage main routines.
        //
        KeyboardMain();
    }
}
