//*****************************************************************************
//
// qs_iot.h - EK-TM4C1294XL Quick start application
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

#ifndef __QS_IOT_H_
#define __QS_IOT_H__

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
// Globally defined constants
//
//*****************************************************************************
#define APP_TICKS_PER_SEC                   100
#define APP_INPUT_BUF_SIZE                  1024
#define NUM_STATS                           12

bool SyncWithExosite(tStat **psStats);
void PrintAllData(void);
bool ProvisionCIK(void);

//*****************************************************************************
//
// Global structure to hold information about the application. If a variable
// corresponds to an Exosite dataport, the alias of that dataport is also
// stored here
//
//*****************************************************************************
extern uint32_t g_ui32SW1Presses;
extern uint32_t g_ui32SW2Presses;
extern uint32_t g_ui32InternalTempF;
extern uint32_t g_ui32InternalTempC;
extern uint32_t g_ui32TimerIntCount;
extern uint32_t g_ui32SecondsOnTime;
extern uint32_t g_ui32LEDD1;
extern uint32_t g_ui32LEDD2;
extern uint32_t g_ui32BoardState;

extern char g_pcLocation[50];
extern char g_pcContactEmail[100];
extern char g_pcAlert[140];

extern tStat *g_psDeviceStatistics[NUM_STATS];

extern tStat g_sLEDD1;
extern tStat g_sLEDD2;
extern tStat g_sLocation;
extern tStat g_sBoardState;
extern tStat g_sContactEmail;
extern tStat g_sAlert;

extern char g_cInput[APP_INPUT_BUF_SIZE];

extern bool g_bPrintingData;
extern bool g_bGameActive;
extern volatile bool g_bOnline;
extern uint32_t g_ui32LinkRetries;

extern uint32_t g_ui32SysClock;

extern void PrintMac(void);
extern void PrintStats(tStat **psStats);
extern void PrintConnectionHelp(void);
extern bool LocateValidCIK(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __QS_IOT_H__
