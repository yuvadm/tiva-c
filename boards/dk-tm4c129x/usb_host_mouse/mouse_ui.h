//*****************************************************************************
//
// mouse_ui.h - Header for the DK-TM4C129X USB keyboard application UI.
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

#ifndef _MOUSE_UI_H__
#define _MOUSE_UI_H__

//*****************************************************************************
//
// The extents of the drawing area.
//
//*****************************************************************************
#define MOUSE_MIN_X             8
#define MOUSE_MAX_X             312
#define MOUSE_MIN_Y             25
#define MOUSE_MAX_Y             210

//*****************************************************************************
//
// The mouse colors and size.
//
//*****************************************************************************
#define DISPLAY_MOUSE_BG        ClrBlack
#define DISPLAY_MOUSE_FG        ClrWhite
#define DISPLAY_MOUSE_SIZE      2

//*****************************************************************************
//
// Status of the device.
//
//*****************************************************************************
typedef struct
{
    //
    // Holds if there is a device connected to this port.
    //
    bool bConnected;

    //
    // Holds if the mouse state has been updated.
    //
    bool bUpdate;

    //
    // The instance data for the device if bConnected is true.
    //
    uint32_t ui32Instance;

    //
    // The mouse button state.
    //
    uint32_t ui32Buttons;

    //
    // The mouse X position.
    //
    uint32_t ui32XPos;

    //
    // The mouse Y position.
    //
    uint32_t ui32YPos;
}
tMouseStatus;

//
// Global status information for this application.
//
extern tMouseStatus g_sStatus;

void UIInit(uint32_t ui32SysClock);
void UIUpdateStatus(void);

#endif
