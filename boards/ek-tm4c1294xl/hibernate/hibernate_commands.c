//*****************************************************************************
//
// hibernate_commands.c - Command line functionality for Hibernate Example.
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

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "utils/cmdline.h"
#include "hibernate_commands.h"

//*****************************************************************************
//
// Hibernate request flag.  Indicates that the user wishes to enter hibernate.
//
//*****************************************************************************
extern bool g_bHibernate;

//*****************************************************************************
//
// Variables that keep track of the date and time.
//
//*****************************************************************************
extern uint32_t g_ui32MonthIdx, g_ui32DayIdx, g_ui32YearIdx;
extern int32_t g_ui32HourIdx, g_ui32MinIdx;

//*****************************************************************************
//
// Variables that keep terminal display position and status.
//
//*****************************************************************************
extern bool g_bFirstUpdate;
extern uint8_t g_ui8FirstLine;

//*****************************************************************************
//
// Function prototype for setting the system date/time.
//
//*****************************************************************************
extern void DateTimeSet(void);

//*****************************************************************************
//
// Table of valid command strings, callback functions and help messages.  This
// is used by the cmdline module.
//
//*****************************************************************************
tCmdLineEntry g_psCmdTable[] =
{
    {"help",   CMD_help,   " : Display list of commands." },
    {"hib",    CMD_hib,    " : Place system into hibernate mode."},
    {"date",   CMD_date,   " : Set Date \"DD/MM/YYYY\"."},
    {"time12", CMD_time12, " : Set Time 12HR style \"HH:MM:XX\" "
                           "XX = AM or PM"},
    {"time24", CMD_time24, " : Set Time 24HR style \"HH:MM\"."},
    {"cls",    CMD_cls,    " : Clear the terminal screen"},
    { 0, 0, 0 }
};

//*****************************************************************************
//
// Command: cls
//
// Clear the terminal screen.
//
//*****************************************************************************
int
CMD_cls(int argc, char **argv)
{
    UARTprintf("\033[2J\033[H");
    g_bFirstUpdate = true;
    g_ui8FirstLine = 1;
    return (0);
}

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

    //
    // Keep the compiler happy.
    //
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
// Request the device enter hibernate mode now.
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
    // Request Application to enter hibernate state.
    //
    g_bHibernate = true;

    return (0);
}

//*****************************************************************************
//
// Command: date
//
// Set the current system date.  Use format "DD/MM/YYYY"
//
//*****************************************************************************
int
CMD_date(int argc, char **argv)
{
    const char *pcNext;

    //
    // Check the argument count and return errors for too many or too few.
    //
    if(argc == 1)
    {
        return(CMDLINE_TOO_FEW_ARGS);
    }
    if(argc > 2)
    {
        return(CMDLINE_TOO_MANY_ARGS);
    }

    //
    // Convert the date to unsigned long
    //
    g_ui32DayIdx = ustrtoul(argv[1], &pcNext, 10);
    g_ui32MonthIdx = ustrtoul(pcNext+1, &pcNext, 10) - 1;
    g_ui32YearIdx = (ustrtoul(pcNext+1, NULL, 10) - 2000);

    //
    // Perform the conversions to a time struct and store in the hibernate
    // module after doing a minimal amount of validation.
    //
    if((g_ui32DayIdx > 31) || (g_ui32MonthIdx > 11))
    {
        return(CMDLINE_INVALID_ARG);
    }

    DateTimeSet();

    return(0);
}

//*****************************************************************************
//
//Command: time12
//
// Set the current system time.  Use format "HH:MM:SS:X" Where X is 'A' or 'P'
// for AM or PM. HH is hours. MM is minutes. SS is seconds.
//
//*****************************************************************************
int
CMD_time12(int argc, char **argv)
{
    const char *pcNext;

    //
    // Check the argument count and return errors for too many or too few.
    //
    if(argc == 1)
    {
        return(CMDLINE_TOO_FEW_ARGS);
    }
    if(argc > 2)
    {
        return(CMDLINE_TOO_MANY_ARGS);
    }

    //
    // Convert the user string to unsigned long hours and minutes.
    //
    g_ui32HourIdx = ustrtoul(argv[1], &pcNext, 10);
    g_ui32MinIdx = ustrtoul(pcNext + 1, &pcNext, 10);

    //
    // Accomodate the PM vs AM modification.  All times are stored internally
    // as 24 hour format.
    //
    if(ustrncmp(pcNext + 1, "PM", 2) == 0)
    {
        if(g_ui32HourIdx < 12)
        {
            g_ui32HourIdx += 12;
        }
    }
    else
    {
        if(g_ui32HourIdx > 11)
        {
            g_ui32HourIdx -= 12;
        }
    }

    //
    // Perform the conversions to a time struct and store in the hibernate
    // module.  Also do some minimal checking on the input data.
    //
    if((g_ui32HourIdx < 24) && (g_ui32MinIdx < 60))
    {
        DateTimeSet();
    }

    return(0);
}

//*****************************************************************************
//
//Command: time24
//
// Set the current system time. Use format "HH:MM:SS"
//
//*****************************************************************************
int
CMD_time24(int argc, char **argv)
{
    const char *pcNext;

    //
    // Check the argument count and return errors for too many or too few.
    //
    if(argc == 1)
    {
        return(CMDLINE_TOO_FEW_ARGS);
    }
    if(argc > 2)
    {
        return(CMDLINE_TOO_MANY_ARGS);
    }

    //
    // Convert the user string to unsigned long hours and minutes.
    //
    g_ui32HourIdx = ustrtoul(argv[1], &pcNext, 10);
    g_ui32MinIdx = ustrtoul(pcNext + 1, NULL, 10);

    //
    // Perform the conversions to a time struct and store in the hibernate
    // module.  Also do some minimal checking on the input data.
    //
    if((g_ui32HourIdx < 24) && (g_ui32MinIdx < 60))
    {
        DateTimeSet();
    }

    return(0);
}
