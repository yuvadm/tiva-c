//*****************************************************************************
//
// menus.h - Prototypes and definitions for application menus.
//
// Copyright (c) 2011-2014 Texas Instruments Incorporated.  All rights reserved.
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

#ifndef __MENUS_H__
#define __MENUS_H__

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
// Declare off-screen displays, buffers and menus for access by the rest of the
// application.
//
//*****************************************************************************
extern uint8_t g_pui8OffscreenBufA[];
extern uint8_t g_pui8OffscreenBufB[];
extern tDisplay g_sOffscreenDisplayA;
extern tDisplay g_sOffscreenDisplayB;
extern tSlideMenuWidget g_sMenuWidget;
extern tStripChartWidget g_sStripChart;
extern tSlideMenu g_sChannelsMenu;
extern tSlideMenu g_sPeriodMenu;
extern tCanvasWidget g_sAINContainerCanvas;
extern tCanvasWidget g_sAccelContainerCanvas;
extern tCanvasWidget g_sTempContainerCanvas;
extern tCanvasWidget g_sCurrentContainerCanvas;
extern tCanvasWidget g_sGyroContainerCanvas;
extern tCanvasWidget g_sMagContainerCanvas;
extern tCanvasWidget g_sClockContainerCanvas;
extern tCanvasWidget g_sStatusContainerCanvas;
extern tClockSetWidget g_sClockSetter;
extern struct tm g_sTimeClock;

//*****************************************************************************
//
// Module function prototypes.
//
//*****************************************************************************
extern void MenuInit(void (*pfnActive)(tWidget *, tSlideMenuItem *, bool));
extern void MenuUpdateText(uint32_t ui32TextID, const char *pcText);
extern void MenuGetState(tConfigState *pState);
extern void MenuSetState(tConfigState *pState);
extern void MenuGetDefaultState(tConfigState *pState);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif


#endif
