//*****************************************************************************
//
// usb_host_hub.c - An example using that supports a USB Hub, USB keyboard, and
// a USB mass storage device.
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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhhid.h"
#include "usblib/host/usbhhub.h"
#include "usblib/host/usbhhidkeyboard.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "third_party/fatfs/src/ff.h"
#include "utils/cmdline.h"
#include "utils/ustdlib.h"
#include "usb_host_hub.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB HUB Host example(usb_host_hub)</h1>
//!
//! This example application demonstrates how to support a USB keyboard
//! and a USB mass storage with a USB Hub.  The application emulates a very
//! simple console with the USB keyboard used for input.  The application
//! requires that the mass storage device is also inserted or the console will
//! generate errors when accessing the file system.  The console supports the
//! following commands: "ls", "cat", "pwd", "cd" and "help".  The "ls" command
//! will provide a listing of the files in the current directory.  The "cat"
//! command can be used to print the contents of a file to the screen.  The
//! "pwd" command displays the current working directory.  The "cd" command
//! allows the application to move to a new directory.  The cd command is
//! simplified and only supports "cd .." but not directory changes like
//! "cd ../somedir".  The "help" command has other aliases that are displayed
//! when the "help" command is issued.
//!
//! Any keyboard that supports the USB HID BIOS protocol should work with this
//! demo application.
//!
//! The application can be recompiled to run using and external USB phy to
//! implement a high speed host using an external USB phy.  To use the external
//! phy the application must be built with \b USE_ULPI defined.  This disables
//! the internal phy and the connector on the DK-TM4C129X board and enables the
//! connections to the external ULPI phy pins on the DK-TM4C129X board.
//
//*****************************************************************************

//*****************************************************************************
//
// The size of the host controller's memory pool in bytes.
//
//*****************************************************************************
#define HCD_MEMORY_SIZE         128

//*****************************************************************************
//
// The memory pool to provide to the Host controller driver.
//
//*****************************************************************************
uint8_t g_pui8HCDPool[HCD_MEMORY_SIZE * MAX_USB_DEVICES];

//*****************************************************************************
//
// Declare the USB Events driver interface.
//
//*****************************************************************************
DECLARE_EVENT_DRIVER(g_sUSBEventDriver, 0, 0, USBHCDEvents);

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the Keyboard class is loaded.
//
//*****************************************************************************
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
    &g_sUSBHostMSCClassDriver,
    &g_sUSBHIDClassDriver,
    &g_sUSBHubClassDriver,
    &g_sUSBEventDriver
};

//*****************************************************************************
//
// This global holds the number of class drivers in the g_ppHostClassDrivers
// list.
//
//*****************************************************************************
static const uint32_t g_ui32NumHostClassDrivers =
                  sizeof(g_ppHostClassDrivers) / sizeof(tUSBHostClassDriver *);

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND        100
#define MS_PER_SYSTICK          (1000 / TICKS_PER_SECOND)

//*****************************************************************************
//
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// The global flags for this application.  The only flag defined is the
// FLAG_CMD_READY which indicates that a command has been entered and is ready
// to be processed.
//
//*****************************************************************************
static uint32_t g_ui32Flags;

#define FLAG_CMD_READY          0x00000001

//*****************************************************************************
//
// These defines are used to define the screen constraints to the application.
//
//*****************************************************************************
#define DISPLAY_BANNER_HEIGHT   18
#define DISPLAY_TEXT_BORDER     8
#define DISPLAY_TEXT_BORDER_H   8
#define BUTTON_WIDTH            ((320 - (2 * DISPLAY_TEXT_BORDER_H)) /        \
                                 NUM_HUB_STATUS)
#define BUTTON_HEIGHT           18

//*****************************************************************************
//
// This is the number of characters that will fit on a line in the text area.
//
//*****************************************************************************
uint32_t g_ui32CharsPerLine;

//*****************************************************************************
//
// This is the number of lines that will fit in the text area.
//
//*****************************************************************************
uint32_t g_ui32LinesPerScreen;

//*****************************************************************************
//
// This is the current line for printing in the text area.
//
//*****************************************************************************
uint32_t g_ui32Line;

//*****************************************************************************
//
// This is the current column for printing in the text area.
//
//*****************************************************************************
uint32_t g_ui32Column;

//*****************************************************************************
//
// File system variables.
//
//*****************************************************************************
const char * StringFromFresult(FRESULT fresult);

//*****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE            64

//*****************************************************************************
//
// Define the maximum number of lines and columns in the command windows.
//
//*****************************************************************************
#define MAX_LINES               23
#define MAX_COLUMNS             60

//*****************************************************************************
//
// The buffer that holds the command line.
//
//*****************************************************************************
static char g_pcCmdBuf[CMD_BUF_SIZE];

//*****************************************************************************
//
// The variables that are used to allow the screen to scroll.
//
//*****************************************************************************
static uint32_t g_ui32CmdIdx;
static char g_pcLines[MAX_LINES * MAX_COLUMNS];
static uint32_t g_ui32CurrentLine;

//*****************************************************************************
//
// Status bar boxes for hub ports.
//
//*****************************************************************************
#define NUM_HUB_STATUS          4
struct
{
    //
    // Holds if there is a device connected to this port.
    //
    bool bConnected;

    //
    // The instance data for the device if bConnected is true.
    //
    uint32_t ui32Instance;
}
g_psHubStatus[NUM_HUB_STATUS];

//*****************************************************************************
//
// Print a string to the screen and save it to the screen buffer.
//
//*****************************************************************************
void
WriteString(const char *pcString)
{
    uint32_t ui32Size, ui32StrSize;
    char *pcCurLine;
    int32_t i32Idx;

    ui32StrSize = ustrlen(pcString);

    //
    // Check if the string requires scrolling the text in order to print.
    //
    if((g_ui32Line >= MAX_LINES) && (g_ui32Column == 0))
    {
        //
        // Start redrawing at line 0.
        //
        g_ui32Line = 0;

        //
        // Print lines from the current position down first.
        //
        for(i32Idx = g_ui32CurrentLine + 1; i32Idx < MAX_LINES; i32Idx++)
        {
            GrStringDraw(&g_sContext, g_pcLines + (MAX_COLUMNS * i32Idx),
                         ustrlen(g_pcLines + (MAX_COLUMNS * i32Idx)),
                         DISPLAY_TEXT_BORDER_H,
                         DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
                         (g_ui32Line * GrFontHeightGet(g_psFontFixed6x8)), 1);

            g_ui32Line++;
        }

        //
        // If not already at the top then print the lines starting at the
        // top of the buffer.
        //
        if(g_ui32CurrentLine != 0)
        {
            for(i32Idx = 0; i32Idx < g_ui32CurrentLine; i32Idx++)
            {
                GrStringDraw(&g_sContext, g_pcLines + (MAX_COLUMNS * i32Idx),
                             ustrlen(g_pcLines + (MAX_COLUMNS * i32Idx)),
                             DISPLAY_TEXT_BORDER_H,
                             DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
                             (g_ui32Line * GrFontHeightGet(g_psFontFixed6x8)),
                             1);

                g_ui32Line++;
            }
        }
    }

    //
    // Save the current line pointer to use in references below.
    //
    pcCurLine = g_pcLines + (MAX_COLUMNS * g_ui32CurrentLine);

    if(g_ui32Column + ui32StrSize >= MAX_COLUMNS - 1)
    {
        ui32Size = MAX_COLUMNS - g_ui32Column - 1;
    }
    else
    {
        ui32Size = ui32StrSize;
    }

    //
    // Handle the case where the string has a new line at the end.
    //
    if(pcString[ui32StrSize-1] == '\n')
    {
        //
        // Make sure that this is not a single new line.
        //
        if(ui32Size > 0 )
        {
            //
            // Copy the string into the screen buffer.
            //
            ustrncpy(pcCurLine + g_ui32Column, pcString, ui32Size - 1);
        }

        //
        // If this is the start of a new line then clear out the rest of the
        // line by writing spaces to the end of the line.
        //
        if(g_ui32Column == 0)
        {
            //
            // Clear out the string with spaces to overwrite any existing
            // characters with spaces.
            //
            for(i32Idx = ui32Size - 1; i32Idx < MAX_COLUMNS; i32Idx++)
            {
                pcCurLine[i32Idx] = ' ';
            }
        }

        //
        // Null terminate the string.
        //
        pcCurLine[g_ui32Column + MAX_COLUMNS - 1] = 0;

        //
        // Draw the new string.
        //
        GrStringDraw(&g_sContext, pcCurLine + g_ui32Column, ui32Size - 1,
                     DISPLAY_TEXT_BORDER_H +
                     (GrFontMaxWidthGet(g_psFontFixed6x8) * g_ui32Column),
                     DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
                     (g_ui32Line * GrFontHeightGet(g_psFontFixed6x8)), 1);

        //
        // Increment the line values and reset the column to 0.
        //
        g_ui32Line++;
        g_ui32CurrentLine++;

        if(g_ui32CurrentLine >= MAX_LINES)
        {
            g_ui32CurrentLine = 0;
        }
        g_ui32Column = 0;
    }
    else
    {
        //
        // Copy the string into the screen buffer.
        //
        ustrncpy(pcCurLine + g_ui32Column, pcString, ui32Size);

        //
        // See if this was the first string draw on this line.
        //
        if(g_ui32Column == 0)
        {
            //
            // Pad the rest of the string with spaces to overwrite any existing
            // characters with spaces.
            //
            for(i32Idx = ui32Size; i32Idx < MAX_COLUMNS - 1; i32Idx++)
            {
                pcCurLine[i32Idx] = ' ';
            }

            //
            // Draw the new string.
            //
            GrStringDraw(&g_sContext,
                         pcCurLine + g_ui32Column, MAX_COLUMNS - 1,
                         DISPLAY_TEXT_BORDER_H +
                         (GrFontMaxWidthGet(g_psFontFixed6x8) * g_ui32Column),
                         DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
                         (g_ui32Line * GrFontHeightGet(g_psFontFixed6x8)), 1);
        }
        else
        {
            //
            // Draw the new string.
            //
            GrStringDraw(&g_sContext, pcCurLine + g_ui32Column,
                         g_ui32Column + ui32Size,
                         DISPLAY_TEXT_BORDER_H +
                         (GrFontMaxWidthGet(g_psFontFixed6x8) * g_ui32Column),
                         DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
                         (g_ui32Line * GrFontHeightGet(g_psFontFixed6x8)), 1);
        }

        //
        // Update the current column.
        //
        g_ui32Column += ui32Size;
    }
}

//*****************************************************************************
//
// This function prints the character out the screen and into the command
// buffer.
//
// ucChar is the character to print out.
//
// This function handles all of the detail of printing a character to the
// screen and into the command line buffer.
//
// No return value.
//
//*****************************************************************************
void
PrintChar(const char cChar)
{
    int32_t i32Char;
    char *pcCurLine;

    GrContextForegroundSet(&g_sContext, ClrWhite);

    pcCurLine = g_pcLines + (MAX_COLUMNS * g_ui32CurrentLine);

    //
    // Allow new lines to cause the column to go back to zero.
    //
    if(cChar != '\n')
    {
        //
        // Handle when receiving a backspace character.
        //
        if(cChar != ASCII_BACKSPACE)
        {
            //
            // This is not a backspace so print the character to the screen.
            //
            GrStringDraw(&g_sContext, &cChar, 1,
                         DISPLAY_TEXT_BORDER_H +
                         (GrFontMaxWidthGet(g_psFontFixed6x8) * g_ui32Column),
                         DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
                         (g_ui32Line * GrFontHeightGet(g_psFontFixed6x8)), 1);

            pcCurLine[g_ui32Column] = cChar;
        }
        else
        {
            //
            // We got a backspace.  If we are at the top left of the screen,
            // return since we don't need to do anything.
            //
            if(g_ui32Column || g_ui32Line)
            {
                //
                // Adjust the cursor position to erase the last character.
                //
                if(g_ui32Column > 2)
                {
                    g_ui32Column--;
                    g_ui32CmdIdx--;
                }

                //
                // Print a space at this position then return without fixing up
                // the cursor again.
                //
                GrStringDraw(&g_sContext, " ", 1,
                          DISPLAY_TEXT_BORDER_H +
                          (GrFontMaxWidthGet(g_psFontFixed6x8) * g_ui32Column),
                          DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
                          (g_ui32Line * GrFontHeightGet(g_psFontFixed6x8)),
                          true);

                pcCurLine[g_ui32Column] = ' ';
            }
            return;
        }
    }
    else
    {
        for(i32Char = g_ui32Column; i32Char < MAX_COLUMNS - 1; i32Char++)
        {
            pcCurLine[i32Char] = ' ';
        }

        //
        // Null terminate the string when enter is pressed.
        //
        pcCurLine[MAX_COLUMNS - 1] = 0;

        g_ui32CurrentLine++;
        if(g_ui32CurrentLine >= MAX_LINES)
        {
            g_ui32CurrentLine = 0;
        }


        //
        // This will allow the code below to properly handle the new line.
        //
        g_ui32Column = g_ui32CharsPerLine;

        g_ui32Flags |= FLAG_CMD_READY;

        g_pcCmdBuf[g_ui32CmdIdx++] = ' ';
    }

    //
    // Update the text row and column that the next character will use.
    //
    if(g_ui32Column < g_ui32CharsPerLine)
    {
        //
        // No line wrap yet so move one column over.
        //
        g_ui32Column++;
    }
    else
    {
        //
        // Line wrapped so go back to the first column and update the line.
        //
        g_ui32Column = 0;
        g_ui32Line++;

        //
        // The line has gone past the end so go back to the first line.
        //
        if(g_ui32Line >= g_ui32LinesPerScreen)
        {
            g_ui32Line = g_ui32LinesPerScreen - 1;
        }
    }

    //
    // Save the new character in the buffer.
    //
    g_pcCmdBuf[g_ui32CmdIdx++] = cChar;
}

//*****************************************************************************
//
// This function implements the "help" command.  It prints a simple list
// of the available commands with a brief description.
//
//*****************************************************************************
int
Cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *psEntry;

    //
    // Print some header text.
    //
    WriteString("Available commands\n");
    WriteString("------------------\n");

    //
    // Point at the beginning of the command table.
    //
    psEntry = &g_psCmdTable[0];

    //
    // Enter a loop to read each entry from the command table.  The
    // end of the table has been reached when the command name is NULL.
    //
    while(psEntry->pcCmd)
    {
        //
        // Print the command name and the brief description.
        //
        WriteString(psEntry->pcCmd);
        WriteString(psEntry->pcHelp);
        WriteString("\n");

        //
        // Advance to the next entry in the table.
        //
        psEntry++;
    }

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// This is the table that holds the command names, implementing functions,
// and brief description.
//
//*****************************************************************************
tCmdLineEntry g_psCmdTable[] =
{
    { "help",   Cmd_help,      " : Display list of commands" },
    { "h",      Cmd_help,   "    : alias for help" },
    { "?",      Cmd_help,   "    : alias for help" },
    { "ls",     Cmd_ls,      "   : Display list of files" },
    { "chdir",  Cmd_cd,         ": Change directory" },
    { "cd",     Cmd_cd,      "   : alias for chdir" },
    { "pwd",    Cmd_pwd,      "  : Show current working directory" },
    { "cat",    Cmd_cat,      "  : Show contents of a text file" },
    { 0, 0, 0 }
};

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
// Update one of the status boxes at the bottom of the screen.
//
//*****************************************************************************
static void
UpdateStatusBox(tRectangle *psRect, const char *pcString, bool bActive)
{
    uint32_t ui32TextColor;

    //
    // Change the status box to green for active devices.
    //
    if(bActive)
    {
        GrContextForegroundSet(&g_sContext, ClrOrange);
        ui32TextColor = ClrBlack;
    }
    else
    {
        GrContextForegroundSet(&g_sContext, ClrBlack);
        ui32TextColor = ClrWhite;
    }

    //
    // Draw the background box.
    //
    GrRectFill(&g_sContext, psRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);

    //
    // Draw the box border.
    //
    GrRectDraw(&g_sContext, psRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ui32TextColor);

    //
    // Unknown device is currently connected.
    //
    GrStringDrawCentered(&g_sContext, pcString, -1,
                         psRect->i16XMin + (BUTTON_WIDTH / 2),
                         psRect->i16YMin + 8, false);

}

//*****************************************************************************
//
// This function updates the status area of the screen.  It uses the current
// state of the application to print the status bar.
//
//*****************************************************************************
void
UpdateStatus(int32_t i32Port)
{
    tRectangle sRect;
    uint8_t ui8DevClass, ui8DevProtocol;

    //
    // Calculate the box size based on the lPort value to update one of the
    // status boxes.  The first and last boxes will draw slightly off screen
    // to make the status area have no borders on the sides and bottom.
    //
    sRect.i16XMin = DISPLAY_TEXT_BORDER_H + (BUTTON_WIDTH * i32Port);
    sRect.i16XMax = sRect.i16XMin + BUTTON_WIDTH;
    sRect.i16YMin = 240 - 10 - BUTTON_HEIGHT;
    sRect.i16YMax = sRect.i16YMin + BUTTON_HEIGHT;

    //
    // Slightly adjust the first box to draw it off screen properly.
    //
    if(i32Port == (NUM_HUB_STATUS - 1))
    {
        sRect.i16XMax -= 2;
    }

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);

    if(g_psHubStatus[i32Port].bConnected)
    {
        ui8DevClass = USBHCDDevClass(g_psHubStatus[i32Port].ui32Instance, 0);
        ui8DevProtocol = USBHCDDevProtocol(
                                       g_psHubStatus[i32Port].ui32Instance, 0);

        if(ui8DevClass == USB_CLASS_HID)
        {
            if(ui8DevProtocol == USB_HID_PROTOCOL_MOUSE)
            {
                //
                // Mouse is currently connected.
                //
                UpdateStatusBox(&sRect, "Mouse", true);
            }
            else if(ui8DevProtocol == USB_HID_PROTOCOL_KEYB)
            {
                //
                // Keyboard is currently connected.
                //
                UpdateStatusBox(&sRect, "Keyboard", true);
            }
            else
            {
                //
                // Unknown device is currently connected.
                //
                UpdateStatusBox(&sRect, "Unknown", true);
            }
        }
        else if(ui8DevClass == USB_CLASS_MASS_STORAGE)
        {
            //
            // MSC device is currently connected.
            //
            UpdateStatusBox(&sRect, "Mass Storage", true);
        }
        else if(ui8DevClass == USB_CLASS_HUB)
        {
            //
            // MSC device is currently connected.
            //
            UpdateStatusBox(&sRect, "Hub", true);
        }
        else
        {
            //
            // Unknown device is currently connected.
            //
            UpdateStatusBox(&sRect, "Unknown", true);
        }
    }
    else
    {
        //
        // Unknown device is currently connected.
        //
        UpdateStatusBox(&sRect, "No Device", false);
    }
}

//*****************************************************************************
//
// This is the generic callback from host stack.
//
// pvData is actually a pointer to a tEventInfo structure.
//
// This function will be called to inform the application when a USB event has
// occurred that is outside those related to the keyboard device.  At this
// point this is used to detect unsupported devices being inserted and removed.
// It is also used to inform the application when a power fault has occurred.
// This function is required when the g_USBGenericEventDriver is included in
// the host controller driver array that is passed in to the
// USBHCDRegisterDrivers() function.
//
//*****************************************************************************
void
USBHCDEvents(void *pvData)
{
    tEventInfo *pEventInfo;
    uint8_t ui8Port;

    //
    // Cast this pointer to its actual type.
    //
    pEventInfo = (tEventInfo *)pvData;

    //
    // Get the hub port number that the device is connected to.
    //
    ui8Port = USBHCDDevHubPort(pEventInfo->ui32Instance);

    switch(pEventInfo->ui32Event)
    {
        case USB_EVENT_UNKNOWN_CONNECTED:
        case USB_EVENT_CONNECTED:
        {
            //
            // If this is the hub then ignore this connection.
            //
            if(USBHCDDevClass(pEventInfo->ui32Instance, 0) == USB_CLASS_HUB)
            {
                break;
            }

            //
            // If this is not a direct connection, then the hub is on
            // port 0 so the index should be moved down from 1-4 to 0-3.
            //
            if(ui8Port > 0)
            {
                ui8Port--;
            }

            //
            // Save the device instance data.
            //
            g_psHubStatus[ui8Port].ui32Instance = pEventInfo->ui32Instance;
            g_psHubStatus[ui8Port].bConnected = true;

            //
            // Update the port status for the new device.
            //
            UpdateStatus(ui8Port);

            break;
        }
        //
        // A device has been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // If this is not a direct connection, then the hub is on
            // port 0 so the index should be moved down from 1-4 to 0-3.
            //
            if(ui8Port > 0)
            {
                ui8Port--;
            }

            //
            // Device is no longer connected.
            //
            g_psHubStatus[ui8Port].bConnected = false;

            //
            // Update the port status for the new device.
            //
            UpdateStatus(ui8Port);

            break;
        }
        default:
        {
            break;
        }
    }
}

//*****************************************************************************
//
// This is the callback from the USB HUB mouse handler.
//
// pvCBData is ignored by this function.
// ui32Event is one of the valid events for a mouse device.
// ui32MsgParam is defined by the event that occurs.
// pvMsgData is a pointer to data that is defined by the event that occurs.
//
// This function will be called to inform the application when a mouse has
// been plugged in or removed and any time mouse movement or button pressed
// is detected.
//
// This function will return 0.
//
//*****************************************************************************
void
HubCallback(tHubInstance *psHubInstance, uint32_t ui32Event,
            uint32_t ui32MsgParam, void *pvMsgData)
{
}

//*****************************************************************************
//
// The main application loop.
//
//*****************************************************************************
int
main(void)
{
    int32_t i32Status, i32Idx;
    uint32_t ui32SysClock, ui32PLLRate;
#ifdef USE_ULPI
    uint32_t ui32Setting;
#endif

    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Set the part pin out appropriately for this device.
    //
    PinoutSet();

#ifdef USE_ULPI
    //
    // Switch the USB ULPI Pins over.
    //
    USBULPIPinoutSet();

    //
    // Enable USB ULPI with high speed support.
    //
    ui32Setting = USBLIB_FEATURE_ULPI_HS;
    USBOTGFeatureSet(0, USBLIB_FEATURE_USBULPI, &ui32Setting);

    //
    // Setting the PLL frequency to zero tells the USB library to use the
    // external USB clock.
    //
    ui32PLLRate = 0;
#else
    //
    // Save the PLL rate used by this application.
    //
    ui32PLLRate = 480000000;
#endif

    //
    // Initialize the hub port status.
    //
    for(i32Idx = 0; i32Idx < NUM_HUB_STATUS; i32Idx++)
    {
        g_psHubStatus[i32Idx].bConnected = false;
    }

    //
    // Enable Clocking to the USB controller.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

    //
    // Initialize the USB stack mode and pass in a mode callback.
    //
    USBStackModeSet(0, eUSBModeHost, 0);

    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ui32NumHostClassDrivers);

    //
    // Open the Keyboard interface.
    //
    KeyboardOpen();
    MSCOpen(ui32SysClock);

    //
    // Open a hub instance and provide it with the memory required to hold
    // configuration descriptors for each attached device.
    //
    USBHHubOpen(HubCallback);

    //
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);

    //
    // Tell the USB library the CPU clock and the PLL frequency.
    //
    USBOTGFeatureSet(0, USBLIB_FEATURE_CPUCLK, &ui32SysClock);
    USBOTGFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

    //
    // Initialize the USB controller for Host mode.
    //
    USBHCDInit(0, g_pui8HCDPool, sizeof(g_pui8HCDPool));

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(ui32SysClock);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&g_sContext, "usb-host-hub");

    //
    // Calculate the number of characters that will fit on a line.
    // Make sure to leave a small border for the text box.
    //
    g_ui32CharsPerLine = (GrContextDpyWidthGet(&g_sContext) - 16) /
                         GrFontMaxWidthGet(g_psFontFixed6x8);

    //
    // Calculate the number of lines per usable text screen.  This requires
    // taking off space for the top and bottom banners and adding a small bit
    // for a border.
    //
    g_ui32LinesPerScreen = (GrContextDpyHeightGet(&g_sContext) -
                           (2*(DISPLAY_BANNER_HEIGHT + 1)))/
                           GrFontHeightGet(g_psFontFixed6x8);

    //
    // Initial update of the screen.
    //
    UpdateStatus(0);
    UpdateStatus(1);
    UpdateStatus(2);
    UpdateStatus(3);

    g_ui32CmdIdx = 0;
    g_ui32CurrentLine = 0;

    //
    // Initialize the file system.
    //
    FileInit();

    //
    // The main loop for the application.
    //
    while(1)
    {
        //
        // Print a prompt to the console.  Show the CWD.
        //
        WriteString("> ");

        //
        // Is there a command waiting to be processed?
        //
        while((g_ui32Flags & FLAG_CMD_READY) == 0)
        {
            //
            // Call the YSB library to let non-interrupt code run.
            //
            USBHCDMain();

            //
            // Call the keyboard and mass storage main routines.
            //
            KeyboardMain();
            MSCMain();
        }

        //
        // Pass the line from the user to the command processor.
        // It will be parsed and valid commands executed.
        //
        i32Status = CmdLineProcess(g_pcCmdBuf);

        //
        // Handle the case of bad command.
        //
        if(i32Status == CMDLINE_BAD_CMD)
        {
            WriteString("Bad command!\n");
        }
        //
        // Handle the case of too many arguments.
        //
        else if(i32Status == CMDLINE_TOO_MANY_ARGS)
        {
            WriteString("Too many arguments for command processor!\n");
        }
        //
        // Otherwise the command was executed.  Print the error
        // code if one was returned.
        //
        else if(i32Status != 0)
        {
            WriteString("Command returned error code\n");
            WriteString((char *)StringFromFresult((FRESULT)i32Status));
            WriteString("\n");
        }

        //
        // Reset the command flag and the command index.
        //
        g_ui32Flags &= ~FLAG_CMD_READY;
        g_ui32CmdIdx = 0;
    }
}
