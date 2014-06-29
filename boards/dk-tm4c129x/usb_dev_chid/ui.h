//*****************************************************************************
//
// ui.c - User interface code for the USB composite HID keyboard/mouse example.
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

#ifndef __UI_H__
#define __UI_H__

//*****************************************************************************
//
// The three special lock keys.
//
//*****************************************************************************
#define UI_CAPS_LOCK            0x00000001
#define UI_SCROLL_LOCK          0x00000002
#define UI_NUM_LOCK             0x00000003

//*****************************************************************************
//
// The state of the UI in terms of USB.
//
//*****************************************************************************
typedef enum
{
    UI_CONNECTED,
    UI_NOT_CONNECTED,
    UI_SUSPENDED
}
tUIState;

extern volatile uint32_t g_ui32SysTickCount;

//*****************************************************************************
//
// UI function prototypes.
//
//*****************************************************************************
void UIKeyEvent(tWidget *psWidget, uint32_t ui32Key, uint32_t ui32Event);
void UIInit(void);
void UIMain(void);
void UICapsLock(bool bEnable);
void UIScrollLock(bool bEnable);
void UINumLock(bool bEnable);
void UIMode(tUIState eState);
int32_t UITouchCallback(uint32_t ui32Message, int32_t i32X, int32_t i32Y);

//*****************************************************************************
//
// USB mouse function prototypes.
//
//*****************************************************************************
void USBMouseInit(void);
void USBMouseMain(void);
void USBMouseUpdate(int32_t i32X, int32_t i32Y, uint8_t ui8Buttons);

//*****************************************************************************
//
// USB keyboard prototypes.
//
//*****************************************************************************
void USBKeyboardInit(void);
void USBKeyboardMain(void);
void USBKeyboardUpdate(uint8_t ui8Modifiers, uint8_t ui8UsageCode,
                       bool bPressed);

#endif
