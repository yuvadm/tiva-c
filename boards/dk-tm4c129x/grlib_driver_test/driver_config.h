//*****************************************************************************
//
// driver_config.h - Modify this header file to tailor grlib_driver_test for
//                   operation with your board and graphics display driver.
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

#ifndef __DRIVER_CONFIG_H__
#define __DRIVER_CONFIG_H__

//*****************************************************************************
//
// TODO: Include the header file for your display driver here.  The default
//       driver is for the QVGA display on the DK-TM4C129X board.  If your
//       driver is dependent upon any other headers, include them here too.
//       This may be the case if, for example, you are using a raster mode
//       display and you have a header containing the timing parameters for
//       the screen.
//
//*****************************************************************************
#include "drivers/kentec320x240x16_ssd2119.h"

//*****************************************************************************
//
// These two headers are required by the default implementation and can likely
// be removed or reworked when you update the application to run on a custom
// board.
//
//*****************************************************************************
#include "drivers/frame.h"
#include "drivers/pinout.h"

//*****************************************************************************
//
// TODO: Set DRIVER_NAME to the name of the tDisplay structure exported by
//       your display driver.
//
//*****************************************************************************
#define DRIVER_NAME g_sKentec320x240x16_SSD2119

//*****************************************************************************
//
// TODO: Set DRIVER_SYS_CLOCK_CONFIG to the value you require for your system.
//       This is the first parameter that is passed to the SysCtlClockFreqSet
//       function.
//
//*****************************************************************************
#define DRIVER_SYS_CLOCK_CONFIG    (SYSCTL_XTAL_25MHZ |                       \
                                    SYSCTL_OSC_MAIN |                         \
                                    SYSCTL_USE_PLL |                          \
                                    SYSCTL_CFG_VCO_480)

//*****************************************************************************
//
// TODO: Set DRIVER_SYS_CLOCK_FREQUENCY to the value you require for your
//       system.  This is the second parameter that is passed to the
//       SysCtlClockFreqSet function and represents the desired system clock
//       frequency.
//
//*****************************************************************************
#define DRIVER_SYS_CLOCK_FREQUENCY 120000000

//*****************************************************************************
//
// TODO: Define the color depth of the display driver's frame buffer.  If this
//       value is 16 or greater, any operations involving the driver color
//       palette will be compiled out of the test.  If it is 8 or lower,  make
//       sure you also define the macro DRIVER_PALETTE_SET to allow the
//       application to set colors in the driver's color lookup table.
//
//*****************************************************************************
#define DRIVER_BPP 16

#if (DRIVER_BPP < 16)
//*****************************************************************************
//
// TODO: This macro must be defined if your display driver uses a palettized
//       pixel format and a global color lookup table. Parameter pui32Color
//       points to an array of 32-bit values each containing and RGB888 color
//       definition.  ui32Index is the starting palette index in the driver's
//       color table that will be changed and ui32NumEntries indicates both the
//       number of colors in the array pointed to by pui32Color and the number
//       of driver color palette locations that are to be written.
//
//*****************************************************************************
#define DRIVER_PALETTE_SET(pui32Color, ui32Index, ui32NumEntries)            \
    YourDriverPaletteSet((pui32Color), (ui32Index), (ui32NumEntries));

#endif

//*****************************************************************************
//
// TODO: This macro must be defined to perform all necessary hardware and
//       software initialization for your display driver.  This will typically
//       involve configuring the LCD controller into the relevant mode for
//       the driver, setting peripheral pin muxing, configuring the EPI and
//       SDRAM if your driver uses raster mode and requires a frame buffer in
//       EPI-connected memory and setting a default color palette if needed.
//
//       The ui32SysClock parameter provides the configured system clock rate
//       in Hz.
//
//*****************************************************************************
#define DRIVER_INIT(ui32SysClock)                                            \
{                                                                            \
    PinoutSet();                                                             \
    Kentec320x240x16_SSD2119Init(ui32SysClock);                              \
}

//*****************************************************************************
//
// We allow the testcase to be built such that it issues no drawing orders
// other than those requested via the command line.  This can be helpful
// when bringing up a new display driver.
//
//*****************************************************************************
//#define NO_GRLIB_CALLS_ON_STARTUP

#endif
