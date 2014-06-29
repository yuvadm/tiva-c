//*****************************************************************************
//
// usb_host_hub.h - The common definitions for the usb_host_hub application.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C129X Firmware Package.
//
//*****************************************************************************

#ifndef __USB_HOST_HUB_H__
#define __USB_HOST_HUB_H__

//*****************************************************************************
//
// The ASCII code for a backspace character.
//
//*****************************************************************************
#define ASCII_BACKSPACE         0x08

void KeyboardOpen(void);
void KeyboardMain(void);
void MSCOpen(uint32_t ui32Clock);
void MSCMain(void);
void UpdateStatus(int32_t i32Port);
void PrintChar(const char cChar);
void WriteString(const char *pcString);

bool FileInit(void);

int Cmd_ls(int argc, char *argv[]);
int Cmd_cd(int argc, char *argv[]);
int Cmd_pwd(int argc, char *argv[]);
int Cmd_cat(int argc, char *argv[]);

#endif
