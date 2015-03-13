//*****************************************************************************
//
// directmode.h - Direct mode communications.
//
// Copyright (c) 2010-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the Tiva Firmware Development Package.
//
//*****************************************************************************

#ifndef __DIRECTMODE_H__
#define __DIRECTMODE_H__

//*****************************************************************************
//
// Define NULL, if not already defined.
//
//*****************************************************************************
#ifndef NULL
#define NULL                    ((void *)0)
#endif

//
// Input to DirectModeTransceive is an opaque stream of bits, grouped as 8
// bits into one byte.  LSBit should be sent first.
//
#define DIRECT_MODE_SEND_OPAQUE 0

//
// Input to DirectModeTransceive is an array of bytes with associated parity
// bit, stored as 16 bit words.  The lower 8 bits in each word are the byte
// (LSBit should be sent first), the lowest bit of the upper 8 bits is the
// parity bit.  The remaining 7 bits are ignored.
//
//
#define DIRECT_MODE_SEND_PARITY 1

//
// Output from DirectModeTransceive should be an opaque bit stream, grouped as
// 8 bits into one byte, LSBit was received first.
//
#define DIRECT_MODE_RECV_OPAQUE 0

//
// Output from DirectModeTransceive should be an array of bytes with
// associated parity bit.
//
#define DIRECT_MODE_RECV_PARITY 2

#define DIRECT_MODE_SEND_MASK   1
#define DIRECT_MODE_RECV_MASK   2

extern void DirectModeInit(void);
extern void DirectModeTransceive(int iMode, void const *pvSendBuf,
                                 unsigned int iSendBytes,
                                 unsigned int iSendBits, void *pvRecvBuf,
                                 unsigned int *piRecvBytes,
                                 unsigned int *piRecvBits);

extern void DirectModeEnable(unsigned int iMode);
extern void DirectModeDisable(void);
extern int DirectModeIsEnabled(void);

#endif
