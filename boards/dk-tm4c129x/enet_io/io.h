//*****************************************************************************
//
// io.h - Prototypes for I/O routines for the enet_io example.
//
// Copyright (c) 2009-2014 Texas Instruments Incorporated.  All rights reserved.
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

#ifndef __IO_H__
#define __IO_H__

#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Exported global variables.
//
//*****************************************************************************
extern volatile unsigned long g_ulAnimSpeed;

//*****************************************************************************
//
// Exported function prototypes.
//
//*****************************************************************************
void io_init(void);
void io_set_led(bool bOn);
void io_get_ledstate(char *pcBuf, int iBufLen);
void io_set_animation_speed_string(char *pcBuf);
void io_get_animation_speed_string(char *pcBuf, int iBufLen);
void io_set_animation_speed(unsigned long ulSpeedPercent);
unsigned long io_get_animation_speed(void);
int io_is_led_on(void);

#ifdef __cplusplus
}
#endif

#endif // __IO_H__
