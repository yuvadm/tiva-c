//*****************************************************************************
//
// qs_iot.c - Quickstart application that connects to a cloud server.
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
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_adc.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "utils/cmdline.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "utils/lwiplib.h"
#include "drivers/pinout.h"
#include "drivers/buttons.h"
#include "drivers/exosite_hal_lwip.h"
#include "drivers/eth_client_lwip.h"
#include "exosite.h"
#include "stats.h"
#include "qs_iot.h"
#include "requests.h"
#include "commands.h"
#include "tictactoe.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Internet of Things Quickstart (qs_iot)</h1>
//!
//!
//! This application records various information about user activity on the
//! board, and periodically reports it to a cloud server managed by Exosite. In
//! order to use all of the features of this application, you will need to have
//! an account with Exosite, and make sure that the device you are using is
//! registered to your Exosite profile with its original MAC address from the
//! factory.
//!
//! If you do not yet have an Exosite account, you can create one at
//! http://ti.exosite.com. The web interface there will help guide you through
//! the account creation process. There is also information in the Quickstart
//! document that is shipped along with the EK-TM4C1294XL evaluation kit.
//!
//! This application uses a command-line based interface through a virtual COM
//! port on UART 0, with the settings 115,200-8-N-1. This application also
//! requires a wired Ethernet connection with internet access to perform
//! cloud-connected activities.
//!
//! Once the application is running you should be able to see program output
//! over the virtual COM port, and interact with the command-line. The command
//! line will allow you to see the information being sent to and from Exosite's
//! servers, change the state of LEDs, and play a game of tic-tac-toe. If you
//! have internet connectivity issues, need to find your MAC address, or need
//! to re-activate your EK-TM4C1294XL board with Exosite, the command line
//! interface also has options to support these operations. Type
//! 'help' at the command prompt to see a list of available commands.
//!
//! If your local internet connection requires the use of a proxy server, you
//! will need to enter a command over the virtual COM port terminal before the
//! device will be able to connect to Exosite. When prompted by the
//! application, type 'setproxy help' for information on how to configure the
//! proxy.  Alternatively, you may uncomment the define statements below for
//! "CUSTOM_PROXY" settings, fill in the correct information for your local
//! http proxy server, and recompile this example. This will permanently set
//! your proxy as the default connection point.
//!
//
//*****************************************************************************

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Global macro definitions.
//
//*****************************************************************************
#define MAX_SYNC_RETRIES        10

// #define CUSTOM_PROXY
// #define PROXY_ADDRESS           "your.proxy.address"
// #define PROXY_PORT              80

//*****************************************************************************
//
// Global variables that will be linked to Exosite.
//
//*****************************************************************************
uint32_t g_ui32SW1Presses = 0;
uint32_t g_ui32SW2Presses = 0;
uint32_t g_ui32InternalTempF = 0;
uint32_t g_ui32InternalTempC = 0;
uint32_t g_ui32TimerIntCount = 0;
uint32_t g_ui32SecondsOnTime = 0;
uint32_t g_ui32LEDD1 = 0;
uint32_t g_ui32LEDD2 = 0;
char g_pcLocation[50] = "";
char g_pcContactEmail[100] = "";
char g_pcAlert[140] = "";

//*****************************************************************************
//
// Global structures used to interface with Exosite.
//
//*****************************************************************************
tStat g_sSW1Presses =
    {"SW1-presses", &g_ui32SW1Presses, "usrsw1", INT, WRITE_ONLY};

tStat g_sSW2Presses =
    {"SW2-presses", &g_ui32SW2Presses, "usrsw2", INT, WRITE_ONLY};

tStat g_sInternalTempF =
    {"Temp(F)", &g_ui32InternalTempF, 0, INT, WRITE_ONLY};

tStat g_sInternalTempC =
    {"Temp(C)", &g_ui32InternalTempC, "jtemp", INT, WRITE_ONLY};

tStat g_sSecondsOnTime =
    {"Time since reset", &g_ui32SecondsOnTime, "ontime", INT, WRITE_ONLY};

tStat g_sLEDD1 =
    {"LED D1", &g_ui32LEDD1, "ledd1", INT, READ_WRITE};

tStat g_sLEDD2 =
    {"LED D2", &g_ui32LEDD2, "ledd2", INT, READ_WRITE};

tStat g_sLocation =
    {"Location", g_pcLocation, "location", STRING, READ_ONLY};

tStat g_sBoardState =
    {NULL, &g_ui32BoardState, "gamestate", HEX, WRITE_ONLY};

tStat g_sContactEmail =
    {"Contact Email", g_pcContactEmail, "emailaddr", STRING, READ_WRITE};

tStat g_sAlert =
    {"Alert Message", g_pcAlert, "alert", STRING, NONE};

//*****************************************************************************
//
// Global array of pointers to all tStat structures to be synced with Exosite.
//
//*****************************************************************************
tStat *g_psDeviceStatistics[NUM_STATS]=
{
    &g_sSW1Presses,
    &g_sSW2Presses,
    &g_sInternalTempF,
    &g_sInternalTempC,
    &g_sSecondsOnTime,
    &g_sLEDD1,
    &g_sLEDD2,
    &g_sLocation,
    &g_sBoardState,
    &g_sContactEmail,
    &g_sAlert,
    NULL
};

//*****************************************************************************
//
// Global variable to keep track of the system clock.
//
//*****************************************************************************
uint32_t g_ui32SysClock = 0;

//*****************************************************************************
//
// Flags to keep track of application state.
//
//*****************************************************************************
bool g_bPrintingData = false;
bool g_bGameActive = false;
volatile bool g_bOnline = false;
uint32_t g_ui32LinkRetries = 0;

//*****************************************************************************
//
// Input buffer for the command line interpreter.
//
//*****************************************************************************
char g_cInput[APP_INPUT_BUF_SIZE];

//*****************************************************************************
//
// Given a list of statistics, prints each item to the UART.
//
//*****************************************************************************
void
PrintStats(tStat **psStats)
{
    uint32_t ui32Index;
    char pcStatValue[256];
    char *pcStatName;

    //
    // Loop over all statistics in the list.
    //
    for(ui32Index = 0; psStats[ui32Index] != NULL; ui32Index++)
    {
        if(psStats[ui32Index]->pcName)
        {
            //
            // For each statistic, print the name and current value to the UART.
            //
            pcStatName = psStats[ui32Index]->pcName;
            StatPrintValue(psStats[ui32Index], pcStatValue);

            UARTprintf("%25s= %s\n", pcStatName, pcStatValue);
        }
    }
}

//*****************************************************************************
//
// Prints the current MAC address to the UART.
//
//*****************************************************************************
void
PrintMac(void)
{
    uint8_t ui8Idx;
    uint8_t pui8MACAddr[6];

    //
    // Get the MAC address from the Ethernet Client layer.
    //
    EthClientMACAddrGet(pui8MACAddr);

    UARTprintf("Current MAC: ");

    //
    // Extract each pair of characters and print them to the UART.
    //
    for(ui8Idx = 0; ui8Idx < 6; ui8Idx++)
    {
        UARTprintf("%02x", pui8MACAddr[ui8Idx]);
    }

    UARTprintf("\n");
}

//*****************************************************************************
//
// This function prints a list of local statistics for this board.
//
//*****************************************************************************
void
PrintAllData(void)
{
    char cExositeCIK[CIK_LENGTH];

    if(UARTPeek('\r') != -1)
    {
        g_bPrintingData = false;

        //
        // Get a user command back
        //
        UARTgets(g_cInput, APP_INPUT_BUF_SIZE);

        //
        // Print a prompt
        //
        UARTprintf("\n> ");

        return;
    }

    UARTprintf("\033[2J\033[H");
    UARTprintf("Welcome to the Connected LaunchPad!!\n");
    UARTprintf("Internet of Things Demo\n");
    UARTprintf("Type 'help' for help.\n\n");

    //
    // Print out the MAC address for reference
    //
    PrintMac();

    //
    // Check to see if we already have a CIK, and print it to the UART
    //
    if(Exosite_GetCIK(cExositeCIK))
    {
        UARTprintf("Current CIK: %s\n", cExositeCIK);
    }
    else
    {
        UARTprintf("No CIK found. Connect to Exosite to obtain one.\n");
    }

    //
    // Check to see how many times (if any) we've failed to connect to the
    // server.
    //
    if((g_ui32LinkRetries == 0) && g_bOnline)
    {
        //
        // For zero failures, report a "Link OK"
        //
        UARTprintf("Link Status: OK\n");
    }
    else if((g_ui32LinkRetries < MAX_SYNC_RETRIES) && g_bOnline)
    {
        //
        // For the first few failures, report that we are trying to
        // re-establish a link.
        //
        UARTprintf("Link Status: Lost (Retries: %d)\n", g_ui32LinkRetries);
    }
    else
    {
        //
        // If we have exceeded the maximum number of retries, show status as
        // offline.
        //
        UARTprintf("Link Status: Offline");
    }

    //
    // Print some header text.
    //
    UARTprintf("\nCollected Statistics\n");
    UARTprintf("--------------------\n");

    PrintStats(g_psDeviceStatistics);

    UARTprintf("\nPress Enter to return to the command prompt...\n");

    UARTFlushTx(0);

    return;
}

//*****************************************************************************
//
// Prints a help message to the UART to help with troubleshooting Exosite
// connection issues.
//
//*****************************************************************************
void
PrintConnectionHelp(void)
{
    UARTprintf("Troubleshooting Exosite Connection:\n\n");

    UARTprintf("    + Make sure you are connected to the internet.\n\n");

    UARTprintf("    + Make sure you have created an Exosite profile.\n\n");

    UARTprintf("    + Make sure you have a \"Connected Launchpad\" device\n");
    UARTprintf("      created in your Exosite profile.\n\n");

    UARTprintf("    + Make sure your that your board's MAC address is\n");
    UARTprintf("      correctly registered with your exosite profile.\n\n");

    UARTprintf("    + If you have a CIK, make sure it matches the CIK for\n");
    UARTprintf("      this device in your online profile with Exosite.\n\n");

    UARTprintf("    + If you have a proxy, make sure to configure it using\n");
    UARTprintf("      this terminal. Type 'setproxy help' to get started.\n");
    UARTprintf("      Once the proxy is set, type 'activate' to obtain a\n");
    UARTprintf("      new CIK, or 'connect' to connect to exosite using an\n");
    UARTprintf("      existing CIK.\n\n");

    UARTprintf("    + Make sure your device is available for provisioning.\n");
    UARTprintf("      If you are not sure that provisioning is enabled,\n");
    UARTprintf("      check the Read Me First documentation or the online\n");
    UARTprintf("      exosite portal for more information.\n\n");
}

//*****************************************************************************
//
// Attempts to find a CIK in the EEPROM. Reports the status of this operation
// to the UART.
//
//*****************************************************************************
bool
GetEEPROMCIK(void)
{
    char pcExositeCIK[50];

    //
    // Try to read the CIK from EEPROM, and alert the user based on what we
    // find.
    //
    if(Exosite_GetCIK(pcExositeCIK))
    {
        //
        // If a CIK is found, continue on to make sure that the CIK is valid.
        //
        UARTprintf("CIK found in EEPROM storage.\n\nCIK: %s\n\n",
                   pcExositeCIK);
    }
    else
    {
        //
        // If a CIK was not found, return immediately and indicate the failure.
        //
        UARTprintf("No CIK found in EEPROM.\n");
        return 0;
    }

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
        return 1;
    }
    else
    {
        //
        // If the sync failed, the CIK is probably invalid, so pass the error
        // back to the caller.
        //
        UARTprintf("Initial sync failed. CIK may be invalid.\n");
        return 0;
    }
}

//*****************************************************************************
//
// Attempts to provision a new CIK through a request to Exosite's servers. This
// should be used when Exosite's CIK does not match the CIK for this device, or
// when a CIK is not found in EEPROM.
//
//*****************************************************************************
bool
ProvisionCIK(void)
{
    uint32_t ui32Idx;
    char pcExositeCIK[50];

    //
    // If we get here, no CIK was found in EEPROM storage. We may need to
    // obtain a CIK from the server.
    //
    UARTprintf("Connecting to exosite to obtain a new CIK... \n");

    //
    // Try to activate with Exosite a few times. If we succeed move on with the
    // new CIK. Otherwise, fail.
    //
    for(ui32Idx = 0; ui32Idx < 1; ui32Idx++)
    {
        if(Exosite_Activate())
        {
            //
            // If exosite gives us a CIK, send feedback to the user
            //
            UARTprintf("CIK acquired!\n\n");

            if(Exosite_GetCIK(pcExositeCIK))
            {
                UARTprintf("CIK: %s\n\n", pcExositeCIK);
                UARTprintf("Connected! ");
                UARTprintf("Type 'stats' to see data for this board.");
            }
            else
            {
                //
                // This shouldn't ever happen, but print an error message in
                // case it does.
                //
                UARTprintf("ERROR reading new CIK from EEPROM.\n");
            }

            //
            // Return "true" indicating that we found a valid CIK.
            //
            return true;
        }
        else
        {
            //
            // If the activation fails, wait at least one second before
            // retrying.
            //
            //ROM_SysCtlDelay(g_ui32SysClock/3);
            if(Exosite_StatusCode() == EXO_STATUS_CONFLICT)
            {
                //
                // This can occur if the MAC address for this board has already
                // been activated, and the device has not been re-enabled for a
                // new CIK.
                //
                UARTprintf("\nExosite reported that this device is not\n");
                UARTprintf("available for provisioning. Check to make sure\n");
                UARTprintf("that you have the correct MAC address, and that\n");
                UARTprintf("this device is enabled for provisioning in your\n");
                UARTprintf("Exosite profile.\n\n");

                return false;
            }
        }
    }

    //
    // Exosite didn't respond, so let the user know.
    //
    UARTprintf("No CIK could be obtained.\n\n");

    PrintConnectionHelp();

    //
    // Return "false", indicating that no CIK was found.
    //
    return false;
}

//*****************************************************************************
//
// Attempts to provision a new CIK through a request to Exosite's servers. This
// should be used when Exosite's CIK does not match the CIK for this device, or
// when a CIK is not found in EEPROM.
//
//*****************************************************************************
bool
LocateValidCIK(void)
{
    //
    // Try to obtain a valid CIK.
    //
    UARTprintf("Locating CIK... ");

    //
    // Check the EEPROM for a valid CIK first. If none can be found
    // there, try to provision a CIK from exosite. If we can obtain a
    // CIK, make sure to set the global state variable that indicates
    // that we can connect to exosite.
    //
    if(GetEEPROMCIK())
    {
        return true;
    }
    else if(ProvisionCIK())
    {
        return true;
    }
    else
    {
        //
        // If both cases above fail, return false, indicating that we did not
        // find a CIK.
        //
        return false;
    }
}

//*****************************************************************************
//
// Takes a reading from the internal temperature sensor, and updates the
// corresponding global statistics.
//
//*****************************************************************************
void
UpdateInternalTemp(void)
{
    uint32_t pui32ADC0Value[1], ui32TempValueC, ui32TempValueF;

    //
    // Take a temperature reading with the ADC.
    //
    ROM_ADCProcessorTrigger(ADC0_BASE, 3);

    //
    // Wait for the ADC to finish taking the sample
    //
    while(!ROM_ADCIntStatus(ADC0_BASE, 3, false))
    {
    }

    //
    // Clear the interrupt
    //
    ROM_ADCIntClear(ADC0_BASE, 3);

    //
    // Read the analog voltage measurement.
    //
    ROM_ADCSequenceDataGet(ADC0_BASE, 3, pui32ADC0Value);

    //
    // Convert the measurement to degrees Celcius and Fahrenheit, and save to
    // the global state variables.
    //
    ui32TempValueC = ((1475 * 4096) - (2250 * pui32ADC0Value[0])) / 40960;
    g_ui32InternalTempC = ui32TempValueC;
    ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;
    g_ui32InternalTempF = ui32TempValueF;
}

//*****************************************************************************
//
// Polls the buttons, and updates global state accordingly.
//
//*****************************************************************************
void
UpdateButtons(void)
{
    uint8_t ui8Buttons, ui8ButtonsChanged;

    //
    // Check the current debounced state of the buttons.
    //
    ui8Buttons = ButtonsPoll(&ui8ButtonsChanged,0);

    //
    // If either button has been pressed, record that status to the
    // corresponding global variable.
    //
    if(BUTTON_PRESSED(USR_SW1, ui8Buttons, ui8ButtonsChanged))
    {
        g_ui32SW1Presses++;
    }
    else if(BUTTON_PRESSED(USR_SW2, ui8Buttons, ui8ButtonsChanged))
    {
        g_ui32SW2Presses++;
    }
}

//*****************************************************************************
//
// Turns LEDs on or off based on global state variables.
//
//*****************************************************************************
void
UpdateLEDs(void)
{
    //
    // If either LED's global flag is set, turn that LED on. Otherwise, turn
    // them off.
    //
    if(g_ui32LEDD1)
    {
        ROM_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
    }
    else
    {
        ROM_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
    }

    if(g_ui32LEDD2)
    {
        ROM_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
    }
    else
    {
        ROM_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
    }
}

//*****************************************************************************
//
// Prompts the user for a command, and blocks while waiting for the user's
// input. This function will return after the execution of a single command.
//
//*****************************************************************************
void
CheckForUserCommands(void)
{
    int iStatus;

    //
    // Peek to see if a full command is ready for processing
    //
    if(UARTPeek('\r') == -1)
    {
        //
        // If not, return so other functions get a chance to run.
        //
        return;
    }

    //
    // If we do have commands, process them immediately in the order they were
    // received.
    //
    while(UARTPeek('\r') != -1)
    {
        //
        // Get a user command back
        //
        UARTgets(g_cInput, APP_INPUT_BUF_SIZE);

        //
        // Process the received command
        //
        iStatus = CmdLineProcess(g_cInput);

        //
        // Handle the case of bad command.
        //
        if(iStatus == CMDLINE_BAD_CMD)
        {
            UARTprintf("Bad command!\n");
        }

        //
        // Handle the case of too many arguments.
        //
        else if(iStatus == CMDLINE_TOO_MANY_ARGS)
        {
            UARTprintf("Too many arguments for command processor!\n");
        }
    }

    //
    // Print a prompt
    //
    UARTprintf("\n> ");

}


//*****************************************************************************
//
// Interrupt handler for Timer0A.
//
// This function will be called periodically on the expiration of Timer0A It
// performs periodic tasks, such as looking for input on the physical buttons,
// and reporting usage statistics to the cloud.
//
//*****************************************************************************
void
Timer0IntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    //
    // Keep track of the number of times this interrupt handler has been
    // called.
    //
    g_ui32TimerIntCount++;

    //
    // Poll the GPIOs for the buttons to check for press events. Update global
    // variables as necessary.
    //
    UpdateButtons();

    if((!g_bPrintingData) && (!g_bGameActive))
    {
        CheckForUserCommands();
    }

    //
    // Once per second, perform the following operations.
    //
    if(!(g_ui32TimerIntCount % APP_TICKS_PER_SEC))
    {
        //
        // Keep track of the total seconds of on-time
        //
        g_ui32SecondsOnTime++;

        //
        // Take a reading from the internal temperature sensor.
        //
        UpdateInternalTemp();

        //
        // Set the LEDs to the correct state.
        //
        UpdateLEDs();

        //
        // Check to see if we have any on-going actions that require the UART
        //
        if(g_bPrintingData)
        {
            //
            // If the user has requested a data print-out, perform that here.
            //
            PrintAllData();
        }
        else if(g_bGameActive)
        {
            //
            // If the user is playing a game of tic-tac-toe, enter the game
            // state machine here.
            //
            if(AdvanceGameState())
            {
                //
                // When the tic-tac-toe game state function returns a '1', the
                // game is over. Print a newline, remove the 'g_bGameActive'
                // flag, and resume normal operation.
                //
                UARTprintf("\n> ");
                g_bGameActive = 0;
            }
        }
    }

    //
    // Make sure the running tally of the number of interrupts doesn't
    // overflow.
    //
    if(g_ui32TimerIntCount == (20 * APP_TICKS_PER_SEC))
    {
        //
        // Reset the interrupt count to zero.
        //
        g_ui32TimerIntCount = 0;

    }

}

//*****************************************************************************
//
// Configures Timer 0 as a general purpose, periodic timer for handling button
// presses.
//
//*****************************************************************************
void
ConfigureTimer0(void)
{
    //
    // Enable the peripherals used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    //
    // Configure the two 32-bit periodic timers.
    //
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock / APP_TICKS_PER_SEC);

    //
    // Lower the priority of this interrupt
    //
    ROM_IntPriorityGroupingSet(4);
    ROM_IntPrioritySet(INT_TIMER0A, 0xE0);

    //
    // Setup the interrupts for the timer timeouts.
    //
    ROM_IntEnable(INT_TIMER0A);
    ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
}

//*****************************************************************************
//
// Enables and configures ADC0 to read the internal temperature sensor into
// sample sequencer 3.
//
//*****************************************************************************
void
ConfigureADC0(void)
{
    //
    // Enable clock to ADC0.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    //
    // Configure ADC0 Sample Sequencer 3 for processor trigger operation.
    //
    ROM_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);

    //
    // Increase the hold time of this sample sequencer to account for the
    // temperature sensor erratum (ADC#09).
    //
    HWREG(ADC0_BASE + ADC_O_SSTSH3) = 0x4;

    //
    // Configure ADC0 sequencer 3 for a single sample of the temperature
    // sensor.
    //
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_TS | ADC_CTL_IE |
                                 ADC_CTL_END);

    //
    // Enable the sequencer.
    //
    ROM_ADCSequenceEnable(ADC0_BASE, 3);

    //
    // Clear the interrupt bit for sequencer 3 to make sure it is not set
    // before the first sample is taken.
    //
    ROM_ADCIntClear(ADC0_BASE, 3);
}

//*****************************************************************************
//
// Main function.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32Timeout;

    //
    // Run from the PLL at 120 MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);

    //
    // Set the pinout for the board, including required pins for Ethernet
    // operation.
    //
    PinoutSet(1,0);

    //
    // Enable the UART, clear the terminal, and print a brief message.
    //
    UARTStdioConfig(0, 115200, g_ui32SysClock);

    //
    // Configure necessary peripherals.
    //
    ConfigureTimer0();
    ConfigureADC0();

    //
    // Take an initial reading of the internal temperature
    //
    UpdateInternalTemp();

    //
    // Initialize the buttons
    //
    ButtonsInit();

    //
    // If a proxy has been pre-defined, enable it here.
    //
#ifdef CUSTOM_PROXY
    usprintf(g_pcProxyAddress, PROXY_ADDRESS);
    g_ui16ProxyPort = PROXY_PORT;
    g_bUseProxy = true;
#endif

    //
    // Clear the screen, and print a welcome message.
    //
    UARTprintf("\033[2J\033[H");
    UARTprintf("Welcome to the Connected LaunchPad!!\n");
    UARTprintf("Internet of Things Demo\n");
    UARTprintf("Type \'help\' for help.\n\n");

    //
    // Initialize Exosite layer to allow Exosite-based user commands later.
    //
    Exosite_Init("texasinstruments", "ek-tm4c1294xl", IF_ENET, 0);

    //
    // Start with the assumption that we are not online yet.
    //
    g_bOnline = false;

    //
    // Print the MAC address, which users will need to register with Exosite.
    //
    PrintMac();

    //
    // Notify the user that we are obtaining an IP address.
    //
    UARTprintf("Obtaining IP... ");

    //
    // Loop a few times to make sure that DHCP has time to find an IP.
    //
    for(ui32Timeout = 10; ui32Timeout > 0; ui32Timeout--)
    {
        //
        // Check to see if we have an IP yet.
        //
        if((lwIPLocalIPAddrGet() != 0xffffffff) &&
           (lwIPLocalIPAddrGet() != 0x00000000))
        {
            //
            // Report that we found an IP address.
            //
            UARTprintf("IP Address Found.\n");

            //
            // If we can find and validate a CIK with Exosite, set the flag to
            // indicate have a valid connection to the cloud.
            //
            g_bOnline = LocateValidCIK();

            break;
        }
        else if(ui32Timeout == 0)
        {
            //
            // Alert the user if it takes a long time to find an IP address. An
            // IP address can still be found later, so this is not an
            // indication of failure.
            //
            UARTprintf("No IP address found, continuing \n"
                       "to search in the background\n");
        }

        //
        // Delay a second to allow DHCP to find us an IP address.
        //
        ROM_SysCtlDelay(g_ui32SysClock / 3);
    }

    //
    // If we don't have a valid exosite connection, let the user know that the
    // device is "offline" and not performing any data synchronization with the
    // cloud.
    //
    if(!g_bOnline)
    {
        UARTprintf("Continuing in offline mode.\n\n");
    }

    //
    // Print a prompt
    //
    UARTprintf("\n> ");

    //
    // Enable interrupts and start the timer. This will enable the UART console
    // input, and also enable updates to the various cloud-enabled variables.
    //
    ROM_IntMasterEnable();
    ROM_TimerEnable(TIMER0_BASE, TIMER_A);

    //
    // Main application loop.
    //
    while(1)
    {
        //
        // Only run the following loop if we have a valid connection to
        // Exosite.
        //
        if(g_bOnline)
        {
            //
            // Attempt to sync data with Exosite
            //
            if(SyncWithExosite(g_psDeviceStatistics))
            {
                //
                // If the sync is successful, reset the "retries" count to zero
                //
                g_ui32LinkRetries = 0;
            }
            else if(Exosite_StatusCode() == EXO_STATUS_NOAUTH)
            {
                //
                // Check to see if we failed for having an old CIK. If we did,
                // flush the UART output, and stop any data-printing operation.
                //
                g_bPrintingData = 0;
                UARTFlushTx(0);

                //
                // Alert the user of the expired CIK.
                //
                UARTprintf("\nCIK no longer valid. ");
                UARTprintf("Please try typing 'activate'.\n");
                UARTprintf("If this does not work, ");
                UARTprintf("log in to exosite to check on\n");
                UARTprintf("the status of your devices.\n");
                UARTprintf("\n> ");

                //
                // We did connect to Exosite, so the link is still valid, but
                // data syncing will not work. Do not increment the number of
                // link retries, but do consider the board "offline" for data
                // syncing.
                //
                g_ui32LinkRetries = 0;
                g_bOnline = false;
            }
            else
            {
                //
                // If the sync fails for some other reason, make sure to record
                // the failure.
                //
                g_ui32LinkRetries++;

                //
                // If there are too many failures, assume that the connection
                // was dropped.
                //
                if(g_ui32LinkRetries > MAX_SYNC_RETRIES)
                {
                    g_bOnline = false;
                }
            }
        }
    }
}
