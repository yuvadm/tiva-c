//*****************************************************************************
//
// commands.c - Command line functions for the qs-cloud demo
//
// Copyright (c) YEAR Texas Instruments Incorporated.  All rights reserved.
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
#include "drivers/exosite_hal_lwip.h"
#include "drivers/eth_client_lwip.h"
#include "exosite.h"
#include "stats.h"
#include "qs_iot.h"
#include "commands.h"
#include "tictactoe.h"

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
    { "stats",       Cmd_stats,       ": Display collected stats for this"
                                      " board" },
    { "activate",    Cmd_activate,    ": Get a CIK from exosite"},
    { "clear",       Cmd_clear,       ": Clear the display "},
    { "led",         Cmd_led,         ": Toggle LEDs. Type \"led help\" for "
                                      "more info."},
    { "connect",     Cmd_connect,     ": Tries to establish a connection with "
                                      "exosite."},
    { "getmac",      Cmd_getmac,      ": Prints the current MAC address."},
    { "setproxy",    Cmd_setproxy,    ": Setup or change proxy configuration."},
    { "setemail",    Cmd_setemail,    ": Change the email address used for "
                                      "alerts."},
    { "alert",       Cmd_alert,       ": Send an alert to the saved email "
                                      "address."},
    { "tictactoe",   Cmd_tictactoe,   ": Play tic-tac-toe!"},
    { 0, 0, 0 }
};

//*****************************************************************************
//
// Array of possible alert messages.
//
//*****************************************************************************
char *g_ppcAlertMessages[] =
{
    "Hello World!!",
    "Testing Exosite scripting features.",
    "Log into Exosite for a quick game of tic-tac-toe!"
};

#define NUM_ALERTS              sizeof(g_ppcAlertMessages)/sizeof(char *)

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
// This function prints a list of local statistics for this board.
//
//*****************************************************************************
int
Cmd_stats(int argc, char *argv[])
{
    //
    // Set the global flag to enable printing of statistics to the UART
    // console. The actual printing should be taken care of in a different
    // context.
    //
    g_bPrintingData = true;

    return 0;
}


//*****************************************************************************
//
// Connects to Exosite and attempts to obtain a CIK. If no connection is made
// Cmd_activate will return and report the failure.
//
//*****************************************************************************
int
Cmd_activate(int argc, char *argv[])
{
    //
    // Attempt to acquire a new CIK from Exosite. If successful, update the
    // global state variables to notify the main application.
    //
    if(ProvisionCIK())
    {
        //
        // Set the state to "online" with zero missed sync operations.
        //
        g_bOnline = true;
        g_ui32LinkRetries = 0;
    }

    return 0;
}

//*****************************************************************************
//
// The "led" command can be used to manually set the state of the two on-board
// LEDs. The new LED state will also be transmitted back to the exosite server,
// so the cloud representation of the LEDs should stay in sync with the board's
// actual behavior.
//
//*****************************************************************************
int
Cmd_led(int argc, char *argv[])
{
    uint32_t *pui32LEDValue;

    //
    // If we have too few arguments, or the second argument starts with 'h'
    // (like the first character of help), print out information about the
    // usage of this command.
    //
    if(argc < 3 || ((argc == 2) && (argv[1][0] == 'h')))
    {
        UARTprintf("LED command usage:\n\n");
        UARTprintf("Specify an LED name (d1 or d2) and a state (on or off),\n");
        UARTprintf("separated by a space.\n\n");
        UARTprintf("For example:\n");
        UARTprintf(" led d1 on\n");
        UARTprintf(" led d2 off\n");
        return 0;
    }

    else if(argv[1][1] == '1')
    {
        //
        // If the second letter of the second argument is a 3, assume the user
        // is trying to interact with led d3.
        //
        pui32LEDValue = &g_ui32LEDD1;
        g_sLEDD1.eReadWriteType = READ_WRITE;
    }
    else if(argv[1][1] == '2')
    {
        //
        // If the second letter of the second argument is a 4, assume the user
        // is trying to interact with led d4.
        //
        pui32LEDValue = &g_ui32LEDD2;
        g_sLEDD2.eReadWriteType = READ_WRITE;
    }
    else
    {
        UARTprintf("Invalid LED name.\n");
        return 0;
    }

    //
    // If the second letter of the third argument is the "n" of "on", turn the
    // LED on. Otherwise, turn it off.
    //
    *pui32LEDValue = (argv[2][1] == 'n') ? 1 : 0;

    return 0;
}

//*****************************************************************************
//
// The "connect" command alerts the main application that it should attempt to
// re-establish a link with the exosite server.
//
//*****************************************************************************
int
Cmd_connect(int argc, char *argv[])
{
    //
    // Set the flags to restart the connection to exosite. The main application
    // loop should handle the actual connection.
    //
    if(g_bOnline)
    {
        UARTprintf("Already connected. ");
        UARTprintf("Type 'stats' to see data for this board.\n");
    }
    else
    {
        UARTprintf("Connecting to Exosite...\r");

        //
        // If a CIK was found, try to sync with Exosite. This should tell us if
        // the CIK is valid or not.
        //
        if(SyncWithExosite(g_psDeviceStatistics))
        {
            //
            // If the sync worked, the CIK is valid. Alert the caller.
            //
            UARTprintf("Connected! Type 'stats' to see data for this board.");
            g_bOnline = true;
            g_ui32LinkRetries = 0;
            return 0;
        }
        else
        {
            UARTprintf("Sync failed.                 \n");
        }
    }

    return 0;
}

//*****************************************************************************
//
// The "clear" command sends an ascii control code to the UART that should
// clear the screen for most PC-side terminals.
//
//*****************************************************************************
int
Cmd_clear(int argc, char *argv[])
{
    UARTprintf("\033[2J\033[H");
    return 0;
}

//*****************************************************************************
//
// The "getmac" command prints the user's current MAC address to the UART.
//
//*****************************************************************************
int
Cmd_getmac(int argc, char *argv[])
{
    PrintMac();
    return 0;
}

//*****************************************************************************
//
// The "setproxy" command allows the user to change their proxy behavior
//
//*****************************************************************************
int
Cmd_setproxy(int argc, char *argv[])
{
    char pcProxyPort[10];

    //
    // Check the number of arguments.
    //
    if((argc == 2) && (ustrcmp("off",argv[1]) == 0 ))
    {
        g_bUseProxy = false;
        g_pcProxyAddress[0] = 0;
        g_ui16ProxyPort = 0;

        UARTprintf("Attempting to re-establish link with Exosite.\n\n");
        g_bOnline = LocateValidCIK();
    }
    else if(argc == 3)
    {
        //
        // Otherwise, copy the user-defined location into the global variable.
        //
        ustrncpy(g_pcProxyAddress, argv[1], 49);
        ustrncpy(pcProxyPort, argv[2], 9);

        //
        // Make sure that the global string remains terminated with a zero.
        //
        g_pcProxyAddress[49] = 0;
        pcProxyPort[9] = 0;
        g_ui16ProxyPort = ustrtoul(pcProxyPort, NULL, 0);
        g_bUseProxy = true;

        UARTprintf("New Proxy Address: %s\n", g_pcProxyAddress);
        UARTprintf("New Proxy Port: %d\n\n", g_ui16ProxyPort);

        UARTprintf("Attempting to re-establish link with Exosite.\n\n");
        g_bOnline = LocateValidCIK();

    }
    else
    {
        UARTprintf("\nProxy configuration help:\n");
        UARTprintf("    The setproxy command changes the proxy behavior of"
                   "this board.\n");
        UARTprintf("    To disable the proxy, type:\n\n");
        UARTprintf("    setproxy off\n\n");
        UARTprintf("    To enable the proxy with a specific proxy name and "
                   "port, type\n");
        UARTprintf("    setproxy <proxyaddress> <portnumber>. For "
                   "example:\n\n");
        UARTprintf("    setproxy your.proxy.address 80\n\n");
    }

    return 0;
}

//*****************************************************************************
//
// The tictactoe command allows users to play a game of tic-tac-toe.
//
//*****************************************************************************
int
Cmd_tictactoe(int argc, char *argv[])
{
    g_bGameActive = 1;

    GameInit();

    return 0;
}

//*****************************************************************************
//
// The setemail command allows the user to set a contact email address to be
// used for alert messages.
//
//*****************************************************************************
int
Cmd_setemail(int argc, char *argv[])
{
    //
    // Check the number of arguments.
    //
    if(argc < 2)
    {
        //
        // If there was no second term, prompt the user to enter one next time.
        //
        UARTprintf("Not enough arguments. Please enter an email address.\n");
        UARTprintf("For example \"setemail yourname@example.com\"");
    }
    else
    {
        //
        // Otherwise, copy the user-defined location into the global variable.
        //
        ustrncpy(g_pcContactEmail, argv[1], 100);

        //
        // Make sure that the global string remains terminated with a zero.
        //
        g_pcContactEmail[99] = 0;

        //
        // Mark the location as READ_WRITE, so it will get uploaded to the
        // server on the next sync.
        //
        g_sContactEmail.eReadWriteType = READ_WRITE;

        UARTprintf("Email set to: %s\n\n", g_pcContactEmail);
    }

    return 0;
}

//*****************************************************************************
//
// The alert command allows the user to send an alert message to the saved
// email address.
//
//*****************************************************************************
int
Cmd_alert(int argc, char *argv[])
{
    uint32_t ui32Index;

    //
    // Check the number of arguments.
    //
    if(argc < 2)
    {
        //
        // If there was no second term, prompt the user to enter one next time.
        //
        UARTprintf("Please specify the alert you want to send:\n");
        for(ui32Index = 0; ui32Index < NUM_ALERTS; ui32Index++)
        {
            //
            // Print a list of the available alert messages.
            //
            UARTprintf("alert %d: %s\n", ui32Index,
                       g_ppcAlertMessages[ui32Index]);
        }
    }
    else
    {
        ui32Index = ustrtoul(argv[1], NULL, 0);
        ustrncpy(g_pcAlert, g_ppcAlertMessages[ui32Index], 140);
        g_sAlert.eReadWriteType = READ_WRITE;

        UARTprintf("Alert message set. Sending to the server "
                   "on the next sync operation.");
    }

    return 0;
}
