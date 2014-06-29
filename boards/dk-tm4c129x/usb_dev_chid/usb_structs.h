//*****************************************************************************
//
// usb_structs.h - Data structures defining the composite mouse and keyboard
// USB device.
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

#ifndef __USB_STRUCTS_H__
#define __USB_STRUCTS_H__

//****************************************************************************
//
// Globals used by both classes.
//
//****************************************************************************
extern volatile uint32_t g_ui32USBFlags;
extern volatile uint32_t g_ui32SysTickCount;
extern volatile uint8_t g_ui8Buttons;

//****************************************************************************
//
// The number of individual device class instances comprising this composite
// device.
//
//****************************************************************************
#define NUM_DEVICES 2

//*****************************************************************************
//
// Keyboard Device Instance.
//
//*****************************************************************************
extern tUSBDHIDKeyboardDevice g_sKeyboardDevice;

//*****************************************************************************
//
// Mouse Device instance
//
//*****************************************************************************
extern tUSBDHIDMouseDevice g_sMouseDevice;

//*****************************************************************************
//
// Array of all composite devices.
//
//*****************************************************************************
extern tCompositeEntry g_psCompDevices[NUM_DEVICES];

//*****************************************************************************
//
// the root composite device.
//
//*****************************************************************************
extern tUSBDCompositeDevice g_sCompDevice;

//*****************************************************************************
//
// The memory allocated to hold the composite descriptor that is created by
// the call to USBDCompositeInit().
//
//*****************************************************************************
#define DESCRIPTOR_DATA_SIZE    (COMPOSITE_DHID_SIZE + COMPOSITE_DHID_SIZE)
extern uint8_t g_pui8DescriptorData[DESCRIPTOR_DATA_SIZE];

//****************************************************************************
//
// The flags used by this application for the g_ulFlags value.
//
//****************************************************************************
#define FLAG_MOVE_UPDATE       0
#define FLAG_CONNECTED         1
#define FLAG_LED_ACTIVITY      2
#define FLAG_MOVE_MOUSE        3
#define FLAG_COMMAND_RECEIVED  4
#define FLAG_SUSPENDED         5

//****************************************************************************
//
// The size of the transmit and receive buffers used for the redirected UART.
// This number should be a power of 2 for best performance.  256 is chosen
// pretty much at random though the buffer should be at least twice the size
// of a maximum-sized USB packet.
//
//****************************************************************************

extern void USBMouseMoveHandler(void);
extern void USBKeyboardMain(void);

//****************************************************************************
//
// CDC device callback function prototypes.
//
//****************************************************************************
extern uint32_t USBEventHandler(void *pvCBData, uint32_t ui32Event,
                                uint32_t ui32MsgValue, void *pvMsgData);
extern uint32_t USBMouseHandler(void *pvCBData, uint32_t ui32Event,
                                uint32_t ui32MsgData, void *pvMsgData);
extern uint32_t USBKeyboardHandler(void *pvCBData, uint32_t ui32Event,
                                   uint32_t ui32MsgData, void *pvMsgData);
#endif
