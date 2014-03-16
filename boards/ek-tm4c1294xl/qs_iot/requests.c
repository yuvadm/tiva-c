//*****************************************************************************
//
// requests.c - Functions for formatting requests to sync data with Exosite
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
#include "exosite.h"
#include "stats.h"
#include "qs_iot.h"
#include "requests.h"

//*****************************************************************************
//
// Buffers for writing and reading exosite data.
//
//*****************************************************************************
char g_pcWriteRequest[REQUEST_BUFFER_SIZE];
char g_pcReadRequest[REQUEST_BUFFER_SIZE];
char g_pcResponse[255];

//*****************************************************************************
//
// Syncs an individual stat with Exosite based on its read/write settings.
//
//*****************************************************************************
bool
AddSyncRequest(tStat *psStat)
{
    char pcFormattedRequest[100];
    char *pcStatAlias;
    int eStatRWType;

    pcStatAlias = psStat->pcCloudAlias;
    eStatRWType = psStat->eReadWriteType;

    //
    // Only interact with the server if the stat has an alias
    //
    if(pcStatAlias == 0)
    {
        return 1;
    }

    //
    // Check to see if we write this stat to the server.
    //
    if((eStatRWType == WRITE_ONLY) || (eStatRWType == READ_WRITE))
    {
        //
        // Format a request to write the current value of this stat.
        //
        StatRequestFormat(psStat, pcFormattedRequest);

        //
        // If the request didn't fit, report failure.
        //
        if(!AddRequest(pcFormattedRequest, g_pcWriteRequest,
                       ustrlen(pcFormattedRequest)))
        {
            return 0;
        }

    }
    else if(eStatRWType == READ_ONLY)
    {
        //
        // If the request didn't fit, report failure.
        //
        if(!AddRequest(pcStatAlias, g_pcReadRequest,
                       ustrlen(pcStatAlias)))
        {
            return 0;
        }

    }

    //
    // Shouldn't get here...
    //
    return 1;
}

bool
AddRequest(char *pcNewRequest, char *pcRequestBuffer, uint32_t ui32Size)
{
    uint32_t ui32BufferOffset;

    //
    // Set the buffer offset to the location of the first null character in the
    // string.
    //
    ui32BufferOffset = ustrlen(pcRequestBuffer);


    //
    // Check to make sure that the buffer is not full.
    //
    if(ui32BufferOffset >= REQUEST_BUFFER_SIZE)
    {
        //
        // If the buffer was already full, return a zero to indicate failure.
        //
        pcRequestBuffer[REQUEST_BUFFER_SIZE - 1] = 0;
        return 0;
    }

    //
    // Check to make sure that the new request is small enough to fit in the
    // buffer, even if we have to add an ampersand and a null terminator.
    //
    if(ui32Size < (REQUEST_BUFFER_SIZE - ui32BufferOffset - 2))
    {
        if(ui32BufferOffset != 0)
        {
            //
            // If the buffer has any data in it, add an ampersand to separate
            // this request from any previous requests.
            //
            pcRequestBuffer[ui32BufferOffset++] = '&';
        }

        //
        // Append the data from the new request to the request buffer, and make
        // sure to put a terminator after it.
        //
        ustrncpy(&pcRequestBuffer[ui32BufferOffset], pcNewRequest, ui32Size);
        pcRequestBuffer[ui32BufferOffset + ui32Size] = 0;
        return 1;
    }
    else
    {
        //
        // If the input string is too long, return a zero.
        //
        return 0;
    }
}

bool
ExtractValueByAlias(char *pcAlias, char *pcBuffer, char *pcDestString,
                    uint32_t ui32MaxSize)
{
    char pcSearchString[100];
    char *pcValueStart;
    uint32_t ui32Idx;

    //
    // Set the search string to be the desired alias with an equals-sign
    // appended. This should help us distinguish between a real alias and a
    // string value made up of the same letters.
    //
    usprintf(pcSearchString, "%s=", pcAlias);

    //
    // Find the desired alias in the buffer.
    //
    pcValueStart = ustrstr(pcBuffer, pcAlias);

    //
    // If we couldn't find it, return a zero. Otherwise, continue extracting
    // the value.
    //
    if(!pcValueStart)
    {
        return false;
    }

    //
    // Find the equals-sign, which should be just before the start of the
    // value.
    //
    pcValueStart = ustrstr(pcValueStart, "=");

    if(!pcValueStart)
    {
        return false;
    }

    //
    // Advance to the first character of the value.
    //
    pcValueStart++;

    //
    // Loop through the input value from the buffer, and copy characters to the
    // destination string.
    //
    ui32Idx = 0;
    while(ui32Idx < ui32MaxSize)
    {
        //
        // Check for the end of the value string.
        //
        if((pcValueStart[ui32Idx] == '&') ||
           (pcValueStart[ui32Idx] == 0))
        {
            //
            // If we have reached the end of the value, null-terminate the
            // destination string, and return.
            //
            pcDestString[ui32Idx] = 0;
            return 1;
        }
        else
        {
            pcDestString[ui32Idx] = pcValueStart[ui32Idx];
        }

        ui32Idx++;
    }

    pcDestString[ui32MaxSize - 1] = 0;
    return 1;
}

//*****************************************************************************
//
// Given a list of statistics, sync each of them with Exosite's server.
//
//*****************************************************************************
bool
SyncWithExosite(tStat **psStats)
{
    int eStatRWType[NUM_STATS];
    char pcServerValue[100];
    uint32_t ui32Index;
    tStat *psStat;

    //
    // Clear the request buffers
    //
    g_pcWriteRequest[0] = 0;
    g_pcReadRequest[0] = 0;

    //
    // Loop over all statistics in the list, and add them to the request
    // buffer.
    //
    for(ui32Index = 0; psStats[ui32Index] != NULL; ui32Index++)
    {
        //
        // Record the read/write behavior of each stat before sending the
        // request. If a particular stat is set to "READ_WRITE", we need to
        // know that now.
        //
        eStatRWType[ui32Index] = psStats[ui32Index]->eReadWriteType;
        if(AddSyncRequest(psStats[ui32Index]) == 0)
        {
            return false;
        }
    }

    //
    // Perform the write, and wait for a response from the server. If exosite
    // doesn't respond, return "false", and assume that no data got through.
    //
    if(ustrlen(g_pcWriteRequest))
    {
        if(!Exosite_Write(g_pcWriteRequest, ustrlen(g_pcWriteRequest)))
        {
            return false;
        }
    }

    //
    // Perform the read, and wait for a response from the server. If exosite
    // doesn't respond, return "false", and assume that no data got through.
    //
    if(ustrlen(g_pcReadRequest))
    {
        if(!Exosite_Read(g_pcReadRequest, g_pcResponse, 255))
        {
            return false;
        }
    }

    //
    // If execution reaches this point, we can assume that the server has
    // accepted the data just sent. Update the information for each stat
    // accordingly.
    //
    for(ui32Index = 0; psStats[ui32Index] != NULL; ui32Index++)
    {
        psStat = psStats[ui32Index];

        //
        // Disable interrupts to avoid data disturbances in other interupt
        // contexts.
        //
        ROM_IntMasterDisable();

        //
        // Check the read/write status of this stat.
        //
        if(psStat->eReadWriteType == READ_ONLY)
        {
            //
            // If a stat is CURRENTLY marked as READ_ONLY, then update its
            // value to match the new value from the server.
            //
            ExtractValueByAlias(psStat->pcCloudAlias, g_pcResponse,
                                pcServerValue, 100);
            StatSetVal(psStat, pcServerValue);
        }
        else if(eStatRWType[ui32Index] == READ_WRITE)
        {
            //
            // If a stat was PREVIOUSLY marked as READ_WRITE at the time that
            // the most recent request was sent, then update its status to
            // READ_ONLY.
            //
            psStat->eReadWriteType = READ_ONLY;
        }

        //
        // Re-enable interrupts.
        //
        ROM_IntMasterEnable();
    }

    //
    // Successfully updated all stats.
    //
    return true;
}

