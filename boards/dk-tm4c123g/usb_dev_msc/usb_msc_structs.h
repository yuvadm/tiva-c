//*****************************************************************************
//
// usb_msc_structs.h - Data structures defining the mass storage USB device.
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

#ifndef __USB_MSC_STRUCTS_H__
#define __USB_MSC_STRUCTS_H__

//*****************************************************************************
//
// The mass storage class device structure.
//
//*****************************************************************************
extern tUSBDMSCDevice g_sMSCDevice;

//*****************************************************************************
//
// The externally provided mass storage class event call back function.
//
//*****************************************************************************
extern uint32_t USBDMSCEventCallback(void *pvCBData, uint32_t ui32Event,
                                     uint32_t ui32MsgParam, void *pvMsgData);

#endif
