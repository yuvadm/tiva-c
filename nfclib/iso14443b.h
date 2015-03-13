//*****************************************************************************
//
// iso14443b.h - ISO 14443B implementation.
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

#ifndef __ISO14443B_H__
#define __ISO14443B_H__

//
// REQB command, which wakes cards from IDLE state and puts them into READY
// state. One of the two possible parameters for \e ucCmd in
// ISO14443BSelectFirst() and ISO14443BSelectNext().
//
#define ISO14443B_REQB          0x00
//
// WUPB command, which wakes cards from IDLE or HALT state and puts them
// into READY or READY*  state. One of the two possible parameters for
// \e ucCmd in ISO14443BSelectFirst() and ISO14443BSelectNext().
//
#define ISO14443B_WUPB          0x08

extern void ISO14443BSetupRegisters(void);
extern void ISO14443BPowerOn(void);
extern void ISO14443BPowerOff(void);
extern int ISO14443BHalt(unsigned char *pucPUPI);
extern int ISO14443BREQB(unsigned char ucCmd, unsigned char ucAFI, unsigned char ucN,
		                 unsigned char *pucATQB, unsigned int *puiATQBSize );
extern int ISO14443BATTRIB(unsigned char *pucPUPI, unsigned char ucTR0, unsigned char ucTR1, unsigned char ucEOF_SOF,
				unsigned char ucMaxFrameSize, unsigned char ucBitRateD2C, unsigned char ucBitRateC2D,
				unsigned char ucProtocolType, unsigned char ucCID, unsigned char *A2ATTRIB);

#endif
