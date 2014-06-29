//*****************************************************************************
//
// usb_dev_keyboard.c - Main routines for the keyboard portion of the composite
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
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/keyboard.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcomp.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "usblib/device/usbdhidkeyb.h"
#include "usb_structs.h"
#include "ui.h"

//****************************************************************************
//
// Global USB keyboard state.
//
//****************************************************************************
struct
{
    //
    // Holds a pending special key press for the Caps Lock, Scroll Lock, or
    // Num Lock keys.
    //
    uint8_t ui8Special;
}
g_sKeyboardState;

//****************************************************************************
//
// The look up table entries for usage codes.
//
//****************************************************************************
typedef struct
{
    char cChar;
    char cUsage;
}
sUsageEntry;

//****************************************************************************
//
// The un-shifted HID usage codes used by the graphical keyboard.
//
//****************************************************************************
static const sUsageEntry g_pcUsageCodes[] =
{
    {'q', HID_KEYB_USAGE_Q}, {'w', HID_KEYB_USAGE_W},
    {'e', HID_KEYB_USAGE_E}, {'r', HID_KEYB_USAGE_R},
    {'t', HID_KEYB_USAGE_T}, {'y', HID_KEYB_USAGE_Y},
    {'u', HID_KEYB_USAGE_U}, {'i', HID_KEYB_USAGE_I},
    {'o', HID_KEYB_USAGE_O}, {'p', HID_KEYB_USAGE_P},
    {'a', HID_KEYB_USAGE_A}, {'s', HID_KEYB_USAGE_S},
    {'d', HID_KEYB_USAGE_D}, {'f', HID_KEYB_USAGE_F},
    {'g', HID_KEYB_USAGE_G}, {'h', HID_KEYB_USAGE_H},
    {'j', HID_KEYB_USAGE_J}, {'k', HID_KEYB_USAGE_K},
    {'l', HID_KEYB_USAGE_L}, {'z', HID_KEYB_USAGE_Z},
    {'x', HID_KEYB_USAGE_X}, {'c', HID_KEYB_USAGE_C},
    {'v', HID_KEYB_USAGE_V}, {'b', HID_KEYB_USAGE_B},
    {'n', HID_KEYB_USAGE_N}, {'m', HID_KEYB_USAGE_M},
    {'0', HID_KEYB_USAGE_0}, {'1', HID_KEYB_USAGE_1},
    {'2', HID_KEYB_USAGE_2}, {'3', HID_KEYB_USAGE_3},
    {'4', HID_KEYB_USAGE_4}, {'5', HID_KEYB_USAGE_5},
    {'6', HID_KEYB_USAGE_6}, {'7', HID_KEYB_USAGE_7},
    {'8', HID_KEYB_USAGE_8}, {'9', HID_KEYB_USAGE_9},
    {'-', HID_KEYB_USAGE_MINUS}, {'=', HID_KEYB_USAGE_EQUAL},
    {'\'', HID_KEYB_USAGE_FQUOTE}, {'[', HID_KEYB_USAGE_LBRACKET},
    {']', HID_KEYB_USAGE_RBRACKET}, {';', HID_KEYB_USAGE_SEMICOLON},
    {' ', HID_KEYB_USAGE_SPACE}, {'/', HID_KEYB_USAGE_FSLASH},
    {'\\', HID_KEYB_USAGE_BSLASH}, {'.', HID_KEYB_USAGE_PERIOD},
    {',', HID_KEYB_USAGE_COMMA},
    {UI_CAPS_LOCK, HID_KEYB_USAGE_CAPSLOCK},
    {UI_SCROLL_LOCK, HID_KEYB_USAGE_SCROLLOCK},
    {UI_NUM_LOCK, HID_KEYB_USAGE_NUMLOCK},
    {UNICODE_BACKSPACE, HID_KEYB_USAGE_BACKSPACE},
    {UNICODE_RETURN, HID_KEYB_USAGE_ENTER},
};

static const uint32_t ui32NumUsageCodes =
                                    sizeof(g_pcUsageCodes)/sizeof(sUsageEntry);

//****************************************************************************
//
// The shifted HID usage codes that are used by the graphical keyboard.
//
//****************************************************************************
static const sUsageEntry g_pcUsageCodesShift[] =
{
    {')', HID_KEYB_USAGE_0}, {'!', HID_KEYB_USAGE_1},
    {'@', HID_KEYB_USAGE_2}, {'#', HID_KEYB_USAGE_3},
    {'$', HID_KEYB_USAGE_4}, {'%', HID_KEYB_USAGE_5},
    {'^', HID_KEYB_USAGE_6}, {'&', HID_KEYB_USAGE_7},
    {'*', HID_KEYB_USAGE_8}, {'(', HID_KEYB_USAGE_9},
    {'?', HID_KEYB_USAGE_FSLASH}, {'+', HID_KEYB_USAGE_EQUAL},
    {':', HID_KEYB_USAGE_SEMICOLON}, {'_', HID_KEYB_USAGE_MINUS},
    {'~', HID_KEYB_USAGE_BQUOTE}, {'|', HID_KEYB_USAGE_BSLASH},
    {'\"', HID_KEYB_USAGE_FQUOTE},
};

static const uint32_t ui32NumUsageCodesShift =
                                    sizeof(g_pcUsageCodes)/sizeof(sUsageEntry);

//****************************************************************************
//
// Handle basic initialization of the USB keyboard.
//
//****************************************************************************
void
USBKeyboardInit(void)
{
    //
    // Clear out the special key variable.
    //
    g_sKeyboardState.ui8Special = 0;
}

//****************************************************************************
//
// Returns the Usage code for a ASCII character.
//
// \param cKey is the ASCII character to look up.
// \param bShifted determines if the look up is for the shifted value or not.
//
// This function is used to look up the USB HID usage code for a given
// ASCII character.  The \e bShifted value indicates if the key should be
// resported as shifted.
//
// \return Returns the usage code for the ASCII character or 0 if none found.
//
//****************************************************************************
static uint8_t
GetUsageCode(char cKey, bool bShifted)
{
    int32_t i32Idx, i32Entries;
    const sUsageEntry *pUsageTable;

    if(bShifted)
    {
        i32Entries = ui32NumUsageCodesShift;
        pUsageTable = g_pcUsageCodesShift;
    }
    else
    {
        i32Entries = ui32NumUsageCodes;
        pUsageTable = g_pcUsageCodes;
    }

    for(i32Idx = 0; i32Idx < i32Entries; i32Idx++)
    {
        if(pUsageTable[i32Idx].cChar == cKey)
        {
            return(pUsageTable[i32Idx].cUsage);
        }
    }
    return(0);
}

//****************************************************************************
//
// Called by the UI interface to update the USB keyboard.
//
// \param ui8Modifiers is the set of key modifiers.
// \param ui8Key is ASCII character to look up.
// \param bPressed indicates if this is a press or release event.
//
// This function is used to update a key that has been pressed based on the
// ASCII character that is passed in the \e ui8Key parameter.   The \e bPressed
// parameter is \b true if the key was pressed and \b false if the key was
// released.
//
// \return None.
//
//****************************************************************************
void
USBKeyboardUpdate(uint8_t ui8Modifiers, uint8_t ui8Key, bool bPressed)
{
    uint8_t ui8Usage;

    //
    // Move these to a-z because USB HID does not recognize unshifted values,
    // it uses the SHIFT modifier to change the case.
    //
    if((ui8Key >= 'A') && (ui8Key <= 'Z'))
    {
        ui8Key |= 0x20;

        if(bPressed)
        {
            ui8Modifiers |= HID_KEYB_LEFT_SHIFT;
        }
    }

    //
    // Get the usage code for this character.
    //
    ui8Usage = GetUsageCode(ui8Key, false);

    //
    // Check if this was a "special" key because USB HID handles these
    // separately.
    //
    if((ui8Usage == HID_KEYB_USAGE_CAPSLOCK) ||
       (ui8Usage == HID_KEYB_USAGE_SCROLLOCK) ||
       (ui8Usage == HID_KEYB_USAGE_NUMLOCK))
    {
        //
        // If there was already a special key pressed, then force it to be
        // released.
        //
        if(g_sKeyboardState.ui8Special)
        {
            USBDHIDKeyboardKeyStateChange(&g_sKeyboardDevice, ui8Modifiers,
                                          g_sKeyboardState.ui8Special, false);
        }

        //
        // Save the new special key.
        //
        g_sKeyboardState.ui8Special = ui8Usage;
    }

    //
    // If there was not an unshifted value for this character then look for
    // a shifted version of the character.
    //
    if(ui8Usage == 0)
    {
        //
        // Get the shifted value and set the shift modifier.
        //
        ui8Usage = GetUsageCode(ui8Key, true);

        if(bPressed)
        {
            ui8Modifiers |= HID_KEYB_LEFT_SHIFT;
        }
    }

    //
    // If a valid usage code was found then pass the key along to the
    // USB library.
    //
    if(ui8Usage)
    {
        USBDHIDKeyboardKeyStateChange(&g_sKeyboardDevice, ui8Modifiers,
                                      ui8Usage, bPressed);
    }
}

//****************************************************************************
//
// Handle the callbacks from the USB library's HID keyboard layer.
//
//****************************************************************************
uint32_t
USBKeyboardHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam,
                   void *pvMsgData)
{
    //
    // Handle LED set requests.  These are the various lock key requests.
    //
    if(ui32Event == USBD_HID_KEYB_EVENT_SET_LEDS)
    {
        //
        // Set the state of the lock keys in the UI.
        //
        UICapsLock(ui32MsgParam & HID_KEYB_CAPS_LOCK);
        UIScrollLock(ui32MsgParam & HID_KEYB_SCROLL_LOCK);
        UINumLock(ui32MsgParam & HID_KEYB_NUM_LOCK);
    }
    else if(ui32Event == USB_EVENT_TX_COMPLETE)
    {
        //
        // Any time a report is sent and there is a pending special key
        // pressed send a key release.
        //
        if(g_sKeyboardState.ui8Special)
        {
            USBDHIDKeyboardKeyStateChange(&g_sKeyboardDevice, 0,
                                          g_sKeyboardState.ui8Special, false);
            g_sKeyboardState.ui8Special = 0;
        }

    }
    return(0);
}
