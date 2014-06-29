//*****************************************************************************
//
// usb_host_keyboard.c - The USB keyboard handling routines.
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
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhhid.h"
#include "usblib/host/usbhhidkeyboard.h"
#include "usb_host_hub.h"

//*****************************************************************************
//
// The size of the keyboard device interface's memory pool in bytes.
//
//*****************************************************************************
#define KEYBOARD_MEMORY_SIZE    128

//*****************************************************************************
//
// The memory pool to provide to the keyboard device.
//
//*****************************************************************************
uint8_t g_pui8Buffer[KEYBOARD_MEMORY_SIZE];

//*****************************************************************************
//
// The global value used to store the keyboard instance value.
//
//*****************************************************************************
static tUSBHKeyboard * g_psKeyboardInstance;

extern const tHIDKeyboardUsageTable g_sUSKeyboardMap;

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
// This variable holds the current status of the modifiers keys.
//
//*****************************************************************************
uint32_t g_ui32Modifiers;

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
            // Change the state so that the main loop knows that the keyboard
            // is no longer present.
            //
            g_iKeyboardState = eStateNoDevice;

            break;
        }

        //
        // New Key press detected.
        //
        case USBH_EVENT_HID_KB_PRESS:
        {
            //
            // If this was a Caps Lock key then update the Caps Lock state.
            //
            if(ui32MsgParam == HID_KEYB_USAGE_CAPSLOCK)
            {
                //
                // The main loop needs to update the keyboard's Caps Lock
                // state.
                //
                g_iKeyboardState = eStateKeyboardUpdate;

                //
                // Toggle the current Caps Lock state.
                //
                g_ui32Modifiers ^= HID_KEYB_CAPS_LOCK;
            }
            else if(ui32MsgParam == HID_KEYB_USAGE_SCROLLOCK)
            {
                //
                // The main loop needs to update the keyboard's Scroll Lock
                // state.
                //
                g_iKeyboardState = eStateKeyboardUpdate;

                //
                // Toggle the current Scroll Lock state.
                //
                g_ui32Modifiers ^= HID_KEYB_SCROLL_LOCK;
            }
            else if(ui32MsgParam == HID_KEYB_USAGE_NUMLOCK)
            {
                //
                // The main loop needs to update the keyboard's Scroll Lock
                // state.
                //
                g_iKeyboardState = eStateKeyboardUpdate;

                //
                // Toggle the current Num Lock state.
                //
                g_ui32Modifiers ^= HID_KEYB_NUM_LOCK;
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
                    cChar = (char)
                        USBHKeyboardUsageToChar(g_psKeyboardInstance,
                                                &g_sUSKeyboardMap,
                                                (uint8_t)ui32MsgParam);
                }

                //
                // A zero value indicates there was no textual mapping of this
                // usage code.
                //
                if(cChar != 0)
                {
                    PrintChar(cChar);
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
            USBHKeyboardInit(g_psKeyboardInstance);

            //
            // Proceed to the keyboard connected state.
            //
            g_iKeyboardState = eStateKeyboardConnected;

            //
            // Set the current state of the modifiers.
            //
            USBHKeyboardModifierSet(g_psKeyboardInstance, g_ui32Modifiers);

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

            USBHKeyboardModifierSet(g_psKeyboardInstance, g_ui32Modifiers);

            //
            // Set the USER LED based on the Caps Lock.
            //
            if(g_ui32Modifiers & HID_KEYB_CAPS_LOCK)
            {
//                ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
            }
            else
            {
//                ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);
            }

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
// This is the main loop that runs the application.
//
//*****************************************************************************
void
KeyboardOpen(void)
{
    //
    // Open an instance of the keyboard driver.  The keyboard does not need
    // to be present at this time, this just save a place for it and allows
    // the applications to be notified when a keyboard is present.
    //
    g_psKeyboardInstance = USBHKeyboardOpen(KeyboardCallback, g_pui8Buffer,
                                            KEYBOARD_MEMORY_SIZE);

//    //
//    // Enable the peripheral used by the USER LED.
//    //
//    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    g_ui32Modifiers = 0;

    //
    // Set GPIO F3 to and output so that the LED can be controlled with GPIO
    // Port F pin 3.
    //
//    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);
//    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);
}

