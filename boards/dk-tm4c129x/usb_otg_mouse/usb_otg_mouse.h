//*****************************************************************************
//
// usb_otg_mouse.h - The otg example application common definitions.
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

#ifndef __USB_OTG_MOUSE_H__
#define __USB_OTG_MOUSE_H__

void HostInit(void);
void HostMain(void);
void DeviceInit(void);
void DeviceMain(void);
void UpdateStatus(char *pcString, uint32_t ui32Buttons, bool bClrGBg);
void ClearMainWindow(void);

//*****************************************************************************
//
// Graphics context used to show text on the display.
//
//*****************************************************************************
extern tContext g_sContext;

//*****************************************************************************
//
// The system clock frequency in Hz.
//
//*****************************************************************************
extern uint32_t g_ui32SysClock;

#endif
