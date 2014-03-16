//*****************************************************************************
//
// commands.c - Command line functions for the qs-cloud demo
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

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/sysctl.h"
#include "utils/cmdline.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "commands.h"
#include "enet_weather.h"

extern uint32_t g_ui32ShowCities;
extern uint32_t g_ui32ProcessingCmds;
extern uint32_t g_ui32IPaddr;
extern sParameters g_sConfig;
extern char g_pcIPAddr[20];
extern void UpdateCity(uint32_t ui32Idx, bool bDraw);
extern void PrintIPAddress(char *pcAddr, uint32_t ipaddr);
extern void ProxyEnable(void);

//*****************************************************************************
//
// This is the table that holds the command names, implementing functions, and
// brief description.
//
//*****************************************************************************
tCmdLineEntry g_psCmdTable[] =
{
    { "help",        Cmd_help,        ": Display list of commands" },
    { "h",           Cmd_help,        ": alias for help" },
    { "?",           Cmd_help,        ": alias for help" },
    { "scroll",      Cmd_cityscroll,  ": Scroll the cities"},
    { "setproxy",    Cmd_setproxy,    ": Setup or change proxy configuration"},
    { 0, 0, 0 }
};

//*****************************************************************************
//
// This function implements the "help" command.  It prints a simple list of the
// available commands with a brief description.
//
//*****************************************************************************
int
Cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;

    //
    // Print some header text.
    //
    UARTprintf("\nAvailable commands\n");
    UARTprintf("------------------\n");

    //
    // Point at the beginning of the command table.
    //
    pEntry = &g_psCmdTable[0];

    //
    // Enter a loop to read each entry from the command table.  The end of the
    // table has been reached when the command name is NULL.
    //
    while(pEntry->pcCmd)
    {
        //
        // Print the command name and the brief description.
        //
        UARTprintf("%15s%s\n", pEntry->pcCmd, pEntry->pcHelp);

        //
        // Advance to the next entry in the table.
        //
        pEntry++;
    }

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// The "setproxy" command allows the user to change their proxy behavior
//
//*****************************************************************************
int
Cmd_setproxy(int argc, char *argv[])
{
    //
    // Check the number of arguments.
    //
    if((argc == 1) && (ustrcmp("off",argv[1]) == 0 ))
    {
        g_sConfig.bProxyEnabled = false;
        g_sConfig.pcProxy[0] = 0;
    }
    else if(argc == 2)
    {
        //
        // Otherwise, copy the user-defined location into the global variable.
        //
        ustrncpy(g_sConfig.pcProxy, argv[1], 49);

        //
        // Make sure that the global string remains terminated with a zero.
        //
        g_sConfig.pcProxy[49] = 0;
        g_sConfig.bProxyEnabled = true;

        UARTprintf("New Proxy Address: %s\n", g_sConfig.pcProxy);

            //
            // Enable the proxy.
            //
            ProxyEnable();

    }
    else
    {
        UARTprintf("\nProxy configuration help:\n");
        UARTprintf("    The setproxy command changes the proxy behavior of"
                "this board.\n");
        UARTprintf("    To disable the proxy, type:\n\n");
        UARTprintf("    setproxy off\n\n");
        UARTprintf("    To enable the proxy with a specific proxy name, "
                "type\n");
        UARTprintf("    setproxy <proxyaddress>. For "
                "example:\n\n");
        UARTprintf("    setproxy your.proxy.address\n\n");
    }

    //
    // No longer processing commands.
    //
    g_ui32ProcessingCmds = 0;

    return 0;
}

//*****************************************************************************
//
// The tictactoe command allows users to play a game of tic-tac-toe.
//
//*****************************************************************************
int
Cmd_cityscroll(int argc, char *argv[])
{
    //
    // Clear the terminal.  Print the banner and IP place holder.
    //
    UARTprintf("\033[2J\033[H");
        UARTprintf("Ethernet Weather Example\n\n");
        UARTprintf("IP: ");

        //
        // Print the IP address.
        //
        PrintIPAddress(g_pcIPAddr, g_ui32IPaddr);
        UARTprintf("\n");
        UARTprintf("Hit 'ENTER' to exit\n\n");

        //
        // Print the current city information.
        //
        UpdateCity(0, true);

        //
        // 'Updating' banner.
        //
        UARTprintf("\n__________________________\n\nUpdating:");

        //
        // Update the variable to begin scrolling through the cities.
        //
        g_ui32ShowCities = 1;

    //
    // No longer processing commands.
    //
    g_ui32ProcessingCmds = 0;

    return 0;
}
