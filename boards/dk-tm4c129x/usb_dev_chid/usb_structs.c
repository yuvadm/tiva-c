//*****************************************************************************
//
// usb_structs.c - Data structures defining the USB keyboard and mouse device.
//
// Copyright (c) 2009-2014 Texas Instruments Incorporated.  All rights reserved.
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

#include <stdint.h>
#include <stdbool.h>
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcomp.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "usblib/device/usbdhidkeyb.h"
#include "usb_structs.h"

//*****************************************************************************
//
// Flags for Keyboard and Mouse shared states.
//
//*****************************************************************************
volatile uint32_t g_ui32USBFlags;

//*****************************************************************************
//
// The languages supported by this device.
//
//*****************************************************************************
const uint8_t g_pui8LangDescriptor[] =
{
    4,
    USB_DTYPE_STRING,
    USBShort(USB_LANG_EN_US)
};

//*****************************************************************************
//
// The manufacturer string.
//
//*****************************************************************************
const uint8_t g_pui8ManufacturerString[] =
{
    (17 + 1) * 2,
    USB_DTYPE_STRING,
    'T', 0, 'e', 0, 'x', 0, 'a', 0, 's', 0, ' ', 0, 'I', 0, 'n', 0, 's', 0,
    't', 0, 'r', 0, 'u', 0, 'm', 0, 'e', 0, 'n', 0, 't', 0, 's', 0,
};

//*****************************************************************************
//
// The product string.
//
//*****************************************************************************
const uint8_t g_pui8ProductString[] =
{
    (13 + 1) * 2,
    USB_DTYPE_STRING,
    'M', 0, 'o', 0, 'u', 0, 's', 0, 'e', 0, ' ', 0, 'E', 0, 'x', 0, 'a', 0,
    'm', 0, 'p', 0, 'l', 0, 'e', 0
};

//*****************************************************************************
//
// The serial number string.
//
//*****************************************************************************
const uint8_t g_pui8SerialNumberString[] =
{
    (8 + 1) * 2,
    USB_DTYPE_STRING,
    '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0, '8', 0
};

//*****************************************************************************
//
// The descriptor string table.
//
//*****************************************************************************
const uint8_t * const g_pui8StringDescriptors[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductString,
    g_pui8SerialNumberString,
};

#define NUM_STRING_DESCRIPTORS (sizeof(g_pui8StringDescriptors) /             \
                                sizeof(uint8_t *))

//****************************************************************************
//
// The HID mouse device initialization and customization structures.
//
//****************************************************************************
tUSBDHIDMouseDevice g_sMouseDevice =
{
    //
    // Tiva VID.
    //
    USB_VID_TI_1CBE,

    //
    // Tiva HID Mouse PID.
    //
    USB_PID_MOUSE,

    //
    // This is in 2mA increments so 500mA.
    //
    250,

    //
    // Bus powered device.
    //
    USB_CONF_ATTR_BUS_PWR,

    //
    // The Mouse handler function.
    //
    USBMouseHandler,

    //
    // Point to the mouse device structure.
    //
    (void *)&g_sMouseDevice,

    //
    // The composite device does not use the strings from the class.
    //
    0,
    0,
};

//*****************************************************************************
//
// The HID keyboard device initialization and customization structures.
//
//*****************************************************************************
tUSBDHIDKeyboardDevice g_sKeyboardDevice =
{
    //
    // Tiva VID.
    //
    USB_VID_TI_1CBE,

    //
    // Tiva HID Mouse PID.
    //
    USB_PID_KEYBOARD,

    //
    // This is in 2mA increments so 500mA.
    //
    250,

    //
    // Bus powered device.
    //
    USB_CONF_ATTR_BUS_PWR,

    //
    // The Mouse handler function.
    //
    USBKeyboardHandler,

    //
    // Point to the mouse device structure.
    //
    (void *)&g_sKeyboardDevice,

    //
    // The composite device does not use the strings from the class.
    //
    0,
    0,
};

//****************************************************************************
//
// The array of devices supported by this composite device.
//
//****************************************************************************
tCompositeEntry g_psCompDevices[NUM_DEVICES];

//****************************************************************************
//
// The memory allocation for the composite USB device descriptors.
//
//****************************************************************************
uint8_t g_pui8DescriptorData[DESCRIPTOR_DATA_SIZE];

//****************************************************************************
//
// Allocate the Device Data for the top level composite device class.
//
//****************************************************************************
tUSBDCompositeDevice g_sCompDevice =
{
    //
    // Tiva VID.
    //
    USB_VID_TI_1CBE,

    //
    // Tiva PID for composite HID and HID.
    //
    USB_PID_COMP_HID_HID,

    //
    // This is in 2mA increments so 500mA.
    //
    250,

    //
    // Bus powered device.
    //
    USB_CONF_ATTR_BUS_PWR,

    //
    // There is no need for a default composite event handler.
    //
    USBEventHandler,

    //
    // The string table.
    //
    g_pui8StringDescriptors,
    NUM_STRING_DESCRIPTORS,

    //
    // The Composite device array.
    //
    NUM_DEVICES,
    g_psCompDevices,
};
