//*****************************************************************************
//
// requests.h - Header for requests.c file. Provides definitions for formatting
//              requests for Exosite.
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
// This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#ifndef __REQUESTS_H__
#define __REQUESTS_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Determines how much memory to reserve for sending requests to Exosite.
//
//*****************************************************************************
#define REQUEST_BUFFER_SIZE     255

//*****************************************************************************
//
// Buffers for holding request data before sending them to exosite.
//
//*****************************************************************************
extern char g_pcWriteRequest[REQUEST_BUFFER_SIZE];
extern char g_pcReadRequest[REQUEST_BUFFER_SIZE];

//*****************************************************************************
//
// Buffer for holding responses from Exosite's servers until they can be
// parsed.
//
//*****************************************************************************
extern char g_pcResponse[255];

//*****************************************************************************
//
// Function prototypes.
//
//*****************************************************************************
extern bool AddSyncRequest(tStat *psStat);
extern bool AddRequest(char *pcNewRequest, char *pcRequestBuffer,
                       uint32_t ui32Size);
extern bool ExtractValueByAlias(char *pcAlias, char *pcBuffer,
                                char *pcDestString, uint32_t ui32MaxSize);
extern bool SyncWithExosite(tStat **psStats);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __REQUESTS_H__
