//*****************************************************************************
//
// idle_task.c - The FreeRTOS idle task.
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

#include <stdint.h>
#include <stdbool.h>
#include "utils/lwiplib.h"
#include "lwip/stats.h"
#include "display_task.h"
#include "idle_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
//*****************************************************************************
//
// The stack for the idle task.
//
//*****************************************************************************
uint32_t g_pui32IdleTaskStack[128];

//*****************************************************************************
//
// The number of tasks that are running.
//
//*****************************************************************************
static uint32_t g_ui32Tasks;

//*****************************************************************************
//
// The number of tasks that existed the last time the display was updated (used
// to detect when the display should be updated again).
//
//*****************************************************************************
static uint32_t g_ui32PreviousTasks;

//*****************************************************************************
//
// The number of seconds that the application has been running.  This is
// initialized to -1 in order to get the initial display updated as soon as
// possible.
//
//*****************************************************************************
static uint32_t g_ui32Seconds = 0xffffffff;

//*****************************************************************************
//
// The current IP address.  This is initialized to -1 in order to get the
// initial display updated as soon as possible.
//
//*****************************************************************************
static uint32_t g_ui32IPAddress = 0xffffffff;

//*****************************************************************************
//
// The number of packets that have been transmitted.  This is initialized to -1
// in order to get the initial display updated as soon as possible.
//
//*****************************************************************************
static uint32_t g_ui32TXPackets = 0xffffffff;

//*****************************************************************************
//
// The number of packets that have been received.  This is initialized to -1 in
// order to get the initial display updated as soon as possible.
//
//*****************************************************************************
static uint32_t g_ui32RXPackets = 0xffffffff;

//*****************************************************************************
//
// A buffer to contain the string versions of the information displayed at the
// bottom of the display.
//
//*****************************************************************************
static char g_pcTimeString[12];
static char g_pcTaskString[4];
static char g_pcIPString[24];
static char g_pcTxString[8];
static char g_pcRxString[8];

//*****************************************************************************
//
// Displays the IP address in a human-readable format.
//
//*****************************************************************************
static void
DisplayIP(uint32_t ui32IP)
{
    uint32_t ui32Loop, ui32Idx, ui32Value;

    //
    // If there is no IP address, indicate that one is being acquired.
    //
    if(ui32IP == 0)
    {
        DisplayString(114, 231 - 10, "  Acquiring...  ");
        return;
    }

    //
    // Set the initial index into the string that is being constructed.
    //
    ui32Idx = 0;

    //
    // Start the string with four spaces.  Not all will necessarily be used,
    // depending upon the length of the IP address string.
    //
    for(ui32Loop = 0; ui32Loop < 4; ui32Loop++)
    {
        g_pcIPString[ui32Idx++] = ' ';
    }

    //
    // Loop through the four bytes of the IP address.
    //
    for(ui32Loop = 0; ui32Loop < 32; ui32Loop += 8)
    {
        //
        // Extract this byte from the IP address word.
        //
        ui32Value = (ui32IP >> ui32Loop) & 0xff;

        //
        // Convert this byte into ASCII, using only the characters required.
        //
        if(ui32Value > 99)
        {
            g_pcIPString[ui32Idx++] = '0' + (ui32Value / 100);
        }
        if(ui32Value > 9)
        {
            g_pcIPString[ui32Idx++] = '0' + ((ui32Value / 10) % 10);
        }
        g_pcIPString[ui32Idx++] = '0' + (ui32Value % 10);

        //
        // Add a dot to separate this byte from the next.
        //
        g_pcIPString[ui32Idx++] = '.';
    }

    //
    // Fill the remainder of the string buffer with spaces.
    //
    for(ui32Loop = ui32Idx - 1; ui32Loop < 20; ui32Loop++)
    {
        g_pcIPString[ui32Loop] = ' ';
    }

    //
    // Null terminate the string at the appropriate place, based on the length
    // of the string version of the IP address.  There may or may not be
    // trailing spaces that remain.
    //
    g_pcIPString[ui32Idx + 3 - ((ui32Idx - 12) / 2)] = '\0';

    //
    // Display the string.  The horizontal position and the number of leading
    // spaces utilized depend on the length of the string version of the IP
    // address.  The end result is the IP address centered in the provided
    // space with leading/trailing spaces as required to clear the remainder of
    // the space.
    //
    DisplayString(117 - ((ui32Idx & 1) * 3), 231 - 10,
                  g_pcIPString + ((ui32Idx - 12) / 2));
}

//*****************************************************************************
//
// Displays a value in a human-readable format.  This does not need to deal
// with leading/trailing spaces sicne the values displayed are monotonically
// increasing.
//
//*****************************************************************************
static void
DisplayValue(char *pcBuffer, uint32_t ui32Value, uint32_t ui32X,
             uint32_t ui32Y)
{
    //
    // See if the value is less than 10.
    //
    if(ui32Value < 10)
    {
        //
        // Display the value using only a single digit.
        //
        pcBuffer[0] = '0' + ui32Value;
        pcBuffer[1] = '\0';
        DisplayString(ui32X + 15, ui32Y, pcBuffer);
    }

    //
    // Otherwise, see if the value is less than 100.
    //
    else if(ui32Value < 100)
    {
        //
        // Display the value using two digits.
        //
        pcBuffer[0] = '0' + (ui32Value / 10);
        pcBuffer[1] = '0' + (ui32Value % 10);
        pcBuffer[2] = '\0';
        DisplayString(ui32X + 12, ui32Y, pcBuffer);
    }

    //
    // Otherwise, see if the value is less than 1,000.
    //
    else if(ui32Value < 1000)
    {
        //
        // Display the value using three digits.
        //
        pcBuffer[0] = '0' + (ui32Value / 100);
        pcBuffer[1] = '0' + ((ui32Value / 10) % 10);
        pcBuffer[2] = '0' + (ui32Value % 10);
        pcBuffer[3] = '\0';
        DisplayString(ui32X + 9, ui32Y, pcBuffer);
    }

    //
    // Otherwise, see if the value is less than 10,000.
    //
    else if(ui32Value < 10000)
    {
        //
        // Display the value using four digits.
        //
        pcBuffer[0] = '0' + (ui32Value / 1000);
        pcBuffer[1] = '0' + ((ui32Value / 100) % 10);
        pcBuffer[2] = '0' + ((ui32Value / 10) % 10);
        pcBuffer[3] = '0' + (ui32Value % 10);
        pcBuffer[4] = '\0';
        DisplayString(ui32X + 6, ui32Y, pcBuffer);
    }

    //
    // Otherwise, see if the value is less than 100,000.
    //
    else if(ui32Value < 100000)
    {
        //
        // Display the value using five digits.
        //
        pcBuffer[0] = '0' + (ui32Value / 10000);
        pcBuffer[1] = '0' + ((ui32Value / 1000) % 10);
        pcBuffer[2] = '0' + ((ui32Value / 100) % 10);
        pcBuffer[3] = '0' + ((ui32Value / 10) % 10);
        pcBuffer[4] = '0' + (ui32Value % 10);
        pcBuffer[5] = '\0';
        DisplayString(ui32X + 3, ui32Y, pcBuffer);
    }

    //
    // Otherwise, the value is between 100,000 and 999,999.
    //
    else
    {
        //
        // Display the value using six digits.
        //
        pcBuffer[0] = '0' + ((ui32Value / 100000) % 10);
        pcBuffer[1] = '0' + ((ui32Value / 10000) % 10);
        pcBuffer[2] = '0' + ((ui32Value / 1000) % 10);
        pcBuffer[3] = '0' + ((ui32Value / 100) % 10);
        pcBuffer[4] = '0' + ((ui32Value / 10) % 10);
        pcBuffer[5] = '0' + (ui32Value % 10);
        pcBuffer[6] = '\0';
        DisplayString(ui32X, ui32Y, pcBuffer);
    }
}

//*****************************************************************************
//
// This hook is called by the FreeRTOS idle task when no other tasks are
// runnable.
//
//*****************************************************************************
void
vApplicationIdleHook(void)
{
    uint32_t ui32Temp;

    //
    // See if this is the first time that the idle task has been called.
    //
    if(g_ui32Seconds == 0xffffffff)
    {
        //
        // Draw the boxes for the statistics that are displayed.
        //
        DisplayMove(8, 231 - 20);
        DisplayDraw(311, 231 - 20);
        DisplayDraw(311, 230);
        DisplayDraw(8, 230);
        DisplayDraw(8, 231 - 20);
        DisplayMove(69, 231 - 20);
        DisplayDraw(69, 230);
        DisplayMove(111, 231 - 20);
        DisplayDraw(111, 230);
        DisplayMove(213, 231 - 20);
        DisplayDraw(213, 230);
        DisplayMove(262, 231 - 20);
        DisplayDraw(262, 230);

        //
        // Place the statistics titles in the boxes.
        //
        DisplayString(21, 231 - 18, "Uptime");
        DisplayString(75, 231 - 18, "Tasks");
        DisplayString(133, 231 - 18, "IP Address");
        DisplayString(232, 231 - 18, "TX");
        DisplayString(280, 231 - 18, "RX");
    }

    //
    // Get the number of seconds that the application has been running.
    //
    ui32Temp = xTaskGetTickCount() / (1000 / portTICK_RATE_MS);

    //
    // See if the number of seconds has changed.
    //
    if(ui32Temp != g_ui32Seconds)
    {
        //
        // Update the local copy of the run time.
        //
        g_ui32Seconds = ui32Temp;

        //
        // Convert the number of seconds into a text string.
        //
        g_pcTimeString[0] = '0' + ((ui32Temp / 36000) % 10);
        g_pcTimeString[1] = '0' + ((ui32Temp / 3600) % 10);
        g_pcTimeString[2] = ':';
        g_pcTimeString[3] = '0' + ((ui32Temp / 600) % 6);
        g_pcTimeString[4] = '0' + ((ui32Temp / 60) % 10);
        g_pcTimeString[5] = ':';
        g_pcTimeString[6] = '0' + ((ui32Temp / 10) % 6);
        g_pcTimeString[7] = '0' + (ui32Temp % 10);
        g_pcTimeString[8] = '\0';

        //
        // Have the display task write this string onto the display.
        //
        DisplayString(16, 231 - 10, g_pcTimeString);
    }

    //
    // Get the number of task that are running except idle task.
    //
    g_ui32Tasks = uxTaskGetNumberOfTasks() - 1;

    //
    // See if the number of tasks has changed.
    //
    if(g_ui32Tasks != g_ui32PreviousTasks)
    {
        //
        // Update the local copy of the number of tasks.
        //
        ui32Temp = g_ui32PreviousTasks = g_ui32Tasks;

        //
        // Convert the number of tasks into a text string and display it.
        //
        if(ui32Temp < 10)
        {
            g_pcTaskString[0] = ' ';
            g_pcTaskString[1] = '0' + (ui32Temp % 10);
            g_pcTaskString[2] = ' ';
            g_pcTaskString[3] = '\0';
            DisplayString(81, 231 - 10, g_pcTaskString);
        }
        else
        {
            g_pcTaskString[0] = '0' + ((ui32Temp / 10) % 10);
            g_pcTaskString[1] = '0' + (ui32Temp % 10);
            g_pcTaskString[2] = '\0';
            DisplayString(83, 231 - 10, g_pcTaskString);
        }
    }

    //
    // Get the current IP address.
    //
    ui32Temp = lwIPLocalIPAddrGet();

    //
    // See if the IP address has changed.
    //
    if(ui32Temp != g_ui32IPAddress)
    {
        //
        // Save the current IP address.
        //
        g_ui32IPAddress = ui32Temp;

        //
        // Update the display of the IP address.
        //
        DisplayIP(ui32Temp);
    }

    //
    // See if the number of transmitted packets has changed.
    //
    if(lwip_stats.link.xmit != g_ui32TXPackets)
    {
        //
        // Save the number of transmitted packets.
        //
        ui32Temp = g_ui32TXPackets = lwip_stats.link.xmit;

        //
        // Update the display of transmitted packets.
        //
        DisplayValue(g_pcTxString, ui32Temp, 219, 231 - 10);
    }

    //
    // See if the number of received packets has changed.
    //
    if(lwip_stats.link.recv != g_ui32RXPackets)
    {
        //
        // Save the number of received packets.
        //
        ui32Temp = g_ui32RXPackets = lwip_stats.link.recv;

        //
        // Update the display of received packets.
        //
        DisplayValue(g_pcRxString, ui32Temp, 268, 231 - 10);
    }

}
