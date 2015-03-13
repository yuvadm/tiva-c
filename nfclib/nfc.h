//*****************************************************************************
//
// nfc.h - NFC implementation.
//
// Copyright (c) 2014 Texas Instruments Incorporated.  All rights reserved.
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

#ifndef __NFC_H__
#define __NFC_H__

//*****************************************************************************
//
// General NFC Function Prototypes
//
//*****************************************************************************
extern unsigned char g_ucNFCID[4];

extern void NfcTagType4BSetupRegisters(void);
extern void NfcTagType4ASetupRegisters(void);
extern int  ISO14443BATQB(unsigned char *pucPUPI,unsigned char ucAFI,
                unsigned char ucBitRate,unsigned char ucMaxFrameSize,
                unsigned char ucProtocolType,unsigned char ucFWI,
                unsigned char ucADC, unsigned char ucFO);

#endif //__NFC_H__
