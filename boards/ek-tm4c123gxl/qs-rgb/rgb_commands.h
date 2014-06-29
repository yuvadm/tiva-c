//*****************************************************************************
//
// rgb_commands.h - Prototypes for the evaluation board command line utils.
//
// Copyright (c) 2012-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#ifndef __RGB_COMMANDS_H__
#define __RGB_COMMANDS_H__

//*****************************************************************************
//
// Defines for the command line argument parser provided as a standard part of
// TivaWare.  qs-rgb application uses the command line parser to extend
// functionality to the serial port.
//
//*****************************************************************************

#define CMDLINE_MAX_ARGS 3

//*****************************************************************************
//
// Declaration for the callback functions that will implement the command line
// functionality.  These functions get called by the command line interpreter
// when the corresponding command is typed into the command line.
//
//*****************************************************************************
extern int CMD_help (int argc, char **argv);
extern int CMD_hib (int argc, char **argv);
extern int CMD_rand (int argc, char **argv);
extern int CMD_intensity (int argc, char **argv);
extern int CMD_rgb (int argc, char **argv);

#endif //__RGB_COMMANDS_H__
