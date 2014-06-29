//*****************************************************************************
//
// usb_gamepad_structs.h - Data structures defining the gamepad USB device.
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

#ifndef _USB_GAMEPAD_STRUCTS_H_
#define _USB_GAMEPAD_STRUCTS_H_

//*****************************************************************************
//
//! The custom HID report that is sent back to the host.
//
//*****************************************************************************
typedef struct
{
    //
    //! The current 8-bit signed X position.
    //
    int8_t i8XPos;

    //
    //! The current 8-bit signed Y position.
    //
    int8_t i8YPos;

    //
    //! The current button state, only the bits 0-2 are valid.
    //
    uint8_t ui8Buttons;
} tCustomReport;

extern tUSBDHIDGamepadDevice g_sGamepadDevice;

extern uint32_t GamepadHandler(void *pvCBData, uint32_t ui32Event,
                               uint32_t ui32MsgData, void *pvMsgData);

#endif
