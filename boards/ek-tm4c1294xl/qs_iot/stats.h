//*****************************************************************************
//
// stats.h - Header file for saving cloud-connected statistics
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

#ifndef __STATS_H__
#define __STATS_H__

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
// Types used for recording general statistics
//
//*****************************************************************************
typedef struct
{
    //
    // Name of the item
    //
    char *pcName;

    //
    // Numerical value
    //
    void *pvValue;

    //
    // Exosite alias of the item
    //
    char *pcCloudAlias;

    //
    // Write/Read status of the item
    //
    enum
    {
        STRING,
        INT,
        HEX
    }
    eValueType;

    //
    // Write/Read status of the item
    //
    enum
    {
        READ_ONLY,
        WRITE_ONLY,
        READ_WRITE,
        NONE
    }
    eReadWriteType;
}
tStat;

//*****************************************************************************
//
// Useful macros for manipulating cloud variables.
//
//*****************************************************************************
#define StatIntVal(sStat)       (*((uint32_t *)((sStat).pvValue)))
#define StatStringVal(sStat)    ((char *)((sStat).pvValue))

#define MAX_STAT_STRING         32

//*****************************************************************************
//
// External function protos
//
//*****************************************************************************
extern void StatSetVal(tStat *psStat, char *pcInputValue);
extern void StatRequestFormat(tStat *psStat, char *pcRequestBuffer);
extern void StatPrintValue(tStat *psStat, char *pcValueString);



//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __STATS_H__

