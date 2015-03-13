//*****************************************************************************
//
// ssitrf79x0.h - Header file for the TI TRF79x0 SSI driver for the
// dk-lm3s9b96 boards.
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

#ifndef __SSITRF79X0_H__
#define __SSITRF79X0_H__

//*****************************************************************************
//
// Exported function prototypes.
//
//*****************************************************************************
extern void SSITRF79x0Init(void);
extern void SSITRF79x0WriteRegister(unsigned char ucAddress,
                                    unsigned char ucData);
extern void SSITRF79x0WriteContinuousStart(unsigned char ucAddress);
extern void SSITRF79x0WriteContinuousData(unsigned char const *pucBuffer,
                                          unsigned int uiLength);
extern void SSITRF79x0WriteContinuousStop(void);
extern unsigned char SSITRF79x0ReadRegister(unsigned char ucAddress);
extern void SSITRF79x0ReadContinuousStart(unsigned char ucAddress);
extern void SSITRF79x0ReadContinuousData(unsigned char *pucBuffer,
                                         unsigned int uiLength);
extern void SSITRF79x0ReadContinuousStop(void);
extern unsigned char SSITRF79x0ReadIRQStatus(void);
extern void SSITRF79x0WriteDirectCommand(unsigned char ucCommand);
extern void SSITRF79x0WriteDirectContinuousStart(void);
extern void SSITRF79x0WriteResetFifoDirectCommand(unsigned char ucCommand);
extern void SSITRF79x0DummyWrite(unsigned char const *pucBuffer,
                                 unsigned int uiLength);
extern void SSITRF79x0WriteDirectCommandWithDummy(unsigned char ucCommand);
extern void SSITRF79x0WritePacket(uint8_t *pui8Buffer, uint8_t ui8CRCBit,
                                  uint8_t ui8TotalLength, uint8_t ui8PayloadLength,
                                  bool eHeaderEnable);

extern void SSITRF79x0ChipSelectAssert(void);
extern void SSITRF79x0GenericWrite(unsigned char const *pucBuffer, unsigned int uiLength);
extern void SSITRF79x0ChipSelectDeAssert(void);


#endif // __SSITRF79X0_H__
