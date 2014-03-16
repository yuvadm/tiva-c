//*****************************************************************************
//
// stats.c - Implements structures and functions for cloud-connected statistics
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

#include "driverlib/rom.h"
#include "utils/ustdlib.h"
#include "stats.h"

//*****************************************************************************
//
// Sets the value of a tStat structure based on a formatted string input.
//
//*****************************************************************************
void
StatSetVal(tStat *psStat, char *pcInputValue)
{
    if((psStat->eValueType) == STRING)
    {
        ustrncpy(StatStringVal(*psStat), pcInputValue, MAX_STAT_STRING);
    }
    else if((psStat->eValueType == INT) ||
            (psStat->eValueType == HEX))
    {
        StatIntVal(*psStat) = ustrtoul(pcInputValue, NULL, 0);
    }
}

//*****************************************************************************
//
// Given a destination buffer and a tStat pointer, this function will produce a
// formatted "request string" that can be used in the Exosite_Write function.
//
//*****************************************************************************
void
StatRequestFormat(tStat *psStat, char *pcRequestBuffer)
{
    if((psStat->eValueType) == STRING)
    {
        //
        // Disable interrupts to avoid changes to the string during the copy
        // operation.
        //
        ROM_IntMasterDisable();

        usprintf(pcRequestBuffer, "%s=%s",
                 psStat->pcCloudAlias,
                 StatStringVal(*psStat));

        ROM_IntMasterEnable();
    }
    else if((psStat->eValueType) == INT)
    {
        usprintf(pcRequestBuffer, "%s=%d",
                 psStat->pcCloudAlias,
                 StatIntVal(*psStat));
    }
    else if((psStat->eValueType) == HEX)
    {
        usprintf(pcRequestBuffer, "%s=0x%x",
                 psStat->pcCloudAlias,
                 StatIntVal(*psStat));
    }
}

//*****************************************************************************
//
// Given a pointer to a tStat and a destination string, this function will
// print the value of the tStat to the string.
//
//*****************************************************************************
void
StatPrintValue(tStat *psStat, char *pcValueString)
{
    char *pcSourceString;
    char pcPercentString[3];
    uint32_t ui32CharValue;
    uint8_t ui8Index;
    //
    // Check the type of data that is stored in this psStat variable.
    //
    if((psStat->eValueType) == STRING)
    {
        //
        // If this is a string, perform a little processing to remove
        // percent-encoded characters. First, copy the original string pointer
        // for ease of use.
        //
        pcSourceString = StatStringVal(*psStat);


        //
        // Loop through the characters in the string one by one.
        //
        ui8Index = 0;
        while(*pcSourceString != 0)
        {
            //
            // If the character is a percent sign, calculate the correct
            // percent-encoding substitution.
            //
            if(*pcSourceString == '%')
            {
                //
                // The next two characters should be a hexadecimal
                // representation of the actual character that should be
                // printed. Convert them to an integer here.
                //
                ustrncpy(pcPercentString, (pcSourceString + 1), 2);

                pcPercentString[2] = 0;

                ui32CharValue = ustrtoul((pcPercentString), NULL, 16);

                //
                // Increment the source pointer past the escaped characters.
                //
                pcSourceString = pcSourceString + 3;

                //
                // Copy the converted value into the destination string.
                //
                pcValueString[ui8Index] = ui32CharValue;

            }
            else
            {
                //
                // If the character was not a percent sign, it can be copied
                // directly.
                //
                pcValueString[ui8Index] = *pcSourceString;
                pcSourceString++;
            }

            //
            // Increment the index.
            //
            ui8Index++;
        }
        
        pcValueString[ui8Index] = 0;
    }
    else if((psStat->eValueType) == INT)
    {
        //
        // If this is an integer value, just print the value as text into the
        // destination string.
        //
        usprintf(pcValueString, "%d", StatIntVal(*psStat));
    }
}
