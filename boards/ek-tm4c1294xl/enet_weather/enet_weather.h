//*****************************************************************************
//
// commands.h - Header file for commandline functions in qs-cloud
//
// Copyright (c) 2014 Texas Instruments Incorporated.  All rights reserved.
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

#ifndef __ENET_WEATER_H__
#define __ENET_WEATER_H__

#include <stdint.h>
#include <stdbool.h>

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
// Defines for setting up the system tick clock.
//
//*****************************************************************************
#define SYSTEM_TICK_MS          10
#define SYSTEM_TICK_S           100

//*****************************************************************************
//
// Define for the cycle speed of the cities.
//
//*****************************************************************************
#define CYCLE_DELAY          3000000

//*****************************************************************************
//
// Define for the number of times the full city list cycles before another
// update.
//
//*****************************************************************************
#define UPDATE_CYCLES          10

//*****************************************************************************
//
// Define the size of the flash program blocks for saving configuration data.
//
//*****************************************************************************
#define FLASH_PB_START          0x40000
#define FLASH_PB_END            FLASH_PB_START + 0x4000

//*****************************************************************************
//
// Input command line buffer size.
//
//*****************************************************************************
#define APP_INPUT_BUF_SIZE                  1024

//*****************************************************************************
//
// The flash parameter block structure.
//
//*****************************************************************************
typedef struct
{
    //
    // Reserved space used by the flash program block code.
    //
    uint32_t ui32PBReserved;

    //
    // The custom city name.
    //
    char pcCustomCity[60];

    //
    // The web proxy name.
    //
    char pcProxy[80];

    //
    // The current temperature unit setting.
    //
    bool bCelsius;

    //
    // Current enable/disable setting for the custom city.
    //
    bool bCustomEnabled;

    //
    // Current enable/disable setting for the proxy.
    //
    bool bProxyEnabled;

    //
    // Indicates whether the current settings have been saved.
    //
    bool bSave;
}
sParameters;

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __ENET_WEATER_H__
