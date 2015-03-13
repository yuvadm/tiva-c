//*****************************************************************************
//
// iso14443a.h - ISO 14443A implementation.
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

#ifndef __ISO14443A_H__
#define __ISO14443A_H__

//
// REQA command, which wakes cards from IDLE state and puts them into READY
// state. One of the two possible parameters for \e ucCmd in
// ISO14443ASelectFirst() and ISO14443ASelectNext().
//
#define ISO14443A_REQA          0x26
//
// WUPA command, which wakes cards from IDLE or HALT state and puts them
// into READY or READY*  state. One of the two possible parameters for
// \e ucCmd in ISO14443ASelectFirst() and ISO14443ASelectNext().
//
#define ISO14443A_WUPA          0x52

extern void ISO14443ASetupRegisters(void);
extern void ISO14443APowerOn(void);
extern void ISO14443APowerOff(void);
extern void ISO14443AHalt(void);
extern int ISO14443AREQA(unsigned char ucCmd, int *piATQA);
extern int ISO14443ASelect(unsigned char const *pucUID,
                           unsigned int uiUIDLength, unsigned char *pucSAK);
extern int ISO14443ASelectFirst(unsigned char ucCmd, unsigned char *pucUID,
                                unsigned int *puiUIDLength,
                                unsigned char *pucSAK);
extern int ISO14443ASelectNext(unsigned char ucCmd, unsigned char *pucUID,
                               unsigned int *puiUIDLength,
                               unsigned char *pucSAK);
extern int ISO14443ACheckParity(const unsigned short * const pusData,
                                const long lLen);
extern void ISO14443ACalculateParity(unsigned short * const pusData,
                                     const long lLen);
extern int ISO14443ACheckCRC(const unsigned short * const pusData,
                             const long lLen);
extern long ISO14443ACalculateCRC(unsigned short * const pusData, const long lSize);

extern int ISO14443RATS(unsigned char ucFSDI, unsigned char ucCID, unsigned char *pucATS);
extern int ISO14443PPS(unsigned char ucCID, unsigned char ucDRI, unsigned char ucDSI);
extern int ISO14443DESELECT(unsigned char ucCID);
#endif
