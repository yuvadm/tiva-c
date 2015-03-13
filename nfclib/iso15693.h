//*****************************************************************************
//
// iso15693.h - The top level API used to communicate with MIFARE cards.
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
//*****************************************************************************

#ifndef __ISO15693_H__
#define __ISO15693_H__

//*****************************************************************************
//
// The maximum size in bytes of a card's UID in ASCII.
//
//*****************************************************************************
#define UID_SIZE                8


//*****************************************************************************
//
// The size in card label string for the card's UID.  Two characters per byte
// and a spot for a null terminator.
//
//*****************************************************************************
#define CARD_LABEL_SIZE         (6 + (UID_SIZE * 2) + 1)

extern void ISO15693Init(void);
extern int ISO15693InventoryAFI(unsigned char ucSubCarrier, unsigned char ucDataRate,
            	  unsigned char ucAfi, unsigned char ucNbSlots,
            	  unsigned char *pucMask, unsigned char ucMaskLen);
extern int ISO15693Inventory(unsigned char ucSubCarrier, unsigned char ucDataRate,
            	  unsigned char ucNbSlots, unsigned char *pucMask, unsigned char ucMaskLen);
extern int ISO15693Anticollision16Slots(unsigned char ucSubCarrier, unsigned char ucDataRate,
            	  	  	  	 unsigned char *pucMask, unsigned char ucMaskLen);

extern int BlockReadSingleUID(unsigned char  *pucUID, unsigned int uiBlock, unsigned char *pucBuf);
extern int BlockWriteSingleUID(unsigned char  *pucUID, unsigned int uiBlock, unsigned char ucValueLen, unsigned char *pucBuf);
extern int BlockLockSingleUID(unsigned char  *pucUID, unsigned int uiBlock, unsigned char *pucResponse);

extern int BlockReadSingle(unsigned int uiBlock, unsigned char *pucBuf);
extern int BlockWriteSingle(unsigned int uiBlock, unsigned char ucValueLen, unsigned char *pucBuf);
extern int BlockLockSingle(unsigned int uiBlock, unsigned char *pucResponse);

extern int ISO15693StayQuiet(unsigned char  *pucUID);

#endif

