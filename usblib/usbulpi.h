//*****************************************************************************
//
// usbulpi.h - Header file for ULPI access functions.
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
// This is part of revision 2.1.0.12573 of the Tiva USB Library.
//
//*****************************************************************************

#ifndef __ULPI_H__
#define __ULPI_H__

#define ULPI_CFG_HS             0x00000000
#define ULPI_CFG_FS             0x00000001
#define ULPI_CFG_LS             0x00000002
#define ULPI_CFG_AUTORESUME     0x00001000
#define ULPI_CFG_INVVBUSIND     0x00002000
#define ULPI_CFG_PASSTHRUIND    0x00004000
#define ULPI_CFG_EXTVBUSDRV     0x00400000
#define ULPI_CFG_EXTVBUSIND     0x00800000

extern void ULPIConfigSet(uint32_t ui32Base, uint32_t ui32Config);
extern void ULPIPowerTransceiver(uint32_t ui32Base, bool bEnable);

#endif
