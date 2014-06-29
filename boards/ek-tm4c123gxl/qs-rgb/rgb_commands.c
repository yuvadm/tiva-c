//*****************************************************************************
//
// rgb_commands.c - Command line functionality implementation
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

#include <stdint.h>
#include <stdbool.h>
#include "qs-rgb.h"
#include "drivers/rgb.h"
#include "inc/hw_types.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "utils/cmdline.h"
#include "rgb_commands.h"

//*****************************************************************************
//
// Table of valid command strings, callback functions and help messages.  This
// is used by the cmdline module.
//
//*****************************************************************************
tCmdLineEntry g_psCmdTable[] =
{
    {"help",     CMD_help,      " : Display list of commands" },
    {"hib",      CMD_hib,       " : Place system into hibernate mode"},
    {"rand",     CMD_rand,      " : Start automatic color sequencing"},
    {"intensity",CMD_intensity, " : Adjust brightness 0 to 100 percent"},
    {"rgb",      CMD_rgb,       " : Adjust color 000000-FFFFFF HTML notation"},
    { 0, 0, 0 }
};

//*****************************************************************************
//
// Command: help
//
// Print the help strings for all commands.
//
//*****************************************************************************
int
CMD_help(int argc, char **argv)
{
    int32_t i32Index;

    (void)argc;
    (void)argv;

    //
    // Start at the beginning of the command table
    //
    i32Index = 0;

    //
    // Get to the start of a clean line on the serial output.
    //
    UARTprintf("\nAvailable Commands\n------------------\n\n");

    //
    // Display strings until we run out of them.
    //
    while(g_psCmdTable[i32Index].pcCmd)
    {
      UARTprintf("%17s %s\n", g_psCmdTable[i32Index].pcCmd,
                 g_psCmdTable[i32Index].pcHelp);
      i32Index++;
    }

    //
    // Leave a blank line after the help strings.
    //
    UARTprintf("\n");

    return (0);
}

//*****************************************************************************
//
// Command: hib
//
// Force the device into hibernate mode now.
//
//*****************************************************************************
int
CMD_hib(int argc, char **argv)
{
    //
    // Keep the compiler happy.
    //
    (void)argc;
    (void)argv;

    //
    // Enter hibernate state.
    //
    AppHibernateEnter();

    return (0);
}

//*****************************************************************************
//
// Command: rand
//
// Starts the automatic light sequence immediately.
//
//*****************************************************************************
int
CMD_rand(int argc, char **argv)
{
    //
    // Keep the compiler happy.
    //
    (void)argc;
    (void)argv;

    //
    // Turn on automatic mode.
    //
    g_sAppState.ui32Mode = APP_MODE_AUTO;

    return (0);
}

//*****************************************************************************
//
// Command: intensity
//
// Takes a single argument that is between zero and one hundred. The argument
// must be an integer.  This is interpreted as the percentage of maximum
// brightness with which to display the current color.
//
//*****************************************************************************
int
CMD_intensity(int argc, char **argv)
{
    uint32_t ui32Intensity;

    //
    // This command requires one parameter.
    //
    if(argc == 2)
    {
        //
        // Extract the intensity from the command line parameter.
        //
        ui32Intensity = ustrtoul(argv[1], 0, 10);

        //
        // Convert the value to a fractional floating point value.
        //
        g_sAppState.fIntensity = ((float) ui32Intensity) / 100.0f;

        //
        // Set the intensity of the RGB LED.
        //
        RGBIntensitySet(g_sAppState.fIntensity);
    }

    return(0);

}

//*****************************************************************************
//
// Command: rgb
//
// Takes a single argument that is a string between 000000 and FFFFFF.
// This is the HTML color code that should be used to set the RGB LED color.
//
// http://www.w3schools.com/html/html_colors.asp
//
//*****************************************************************************
int
CMD_rgb(int argc, char **argv)
{
    uint32_t ui32HTMLColor;

    //
    // This command requires one parameter.
    //
    if(argc == 2)
    {
        //
        // Extract the required color from the command line parameter.
        //
        ui32HTMLColor = ustrtoul(argv[1], 0, 16);

        //
        // Decompose teh color into red, green and blue components.
        //
        g_sAppState.ui32Colors[RED] = (ui32HTMLColor & 0xFF0000) >> 8;
        g_sAppState.ui32Colors[GREEN] = (ui32HTMLColor & 0x00FF00);
        g_sAppState.ui32Colors[BLUE] = (ui32HTMLColor & 0x0000FF) << 8;

        //
        // Turn off automatic mode and set the desired LED color.
        //
        g_sAppState.ui32Mode = APP_MODE_REMOTE;
        g_sAppState.ui32ModeTimer = 0;
        RGBColorSet(g_sAppState.ui32Colors);
    }

    return (0);

}
