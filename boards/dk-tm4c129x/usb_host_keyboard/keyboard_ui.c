//*****************************************************************************
//
// keyboard_ui.c - The DK-TM4C129X USB keyboard application UI.
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
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/host/usbhost.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "keyboard_ui.h"

//*****************************************************************************
//
// These defines are used to define the screen constraints to the application.
//
//*****************************************************************************
#define DISPLAY_BANNER_HEIGHT   18
#define DISPLAY_TEXT_BORDER     8
#define DISPLAY_TEXT_BORDER_H   8
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
// This is the current column for printing in the text area.
//
//*****************************************************************************
uint32_t g_ui32Column;

//*****************************************************************************
//
// Define the maximum number of lines and columns in the command windows.
//
//*****************************************************************************
#define MAX_LINES               23
#define MAX_COLUMNS             60

//*****************************************************************************
//
// The variables that are used to allow the screen to scroll.
//
//*****************************************************************************
static char g_ppcLines[MAX_LINES][MAX_COLUMNS];
static uint32_t g_ui32CurrentLine;
uint32_t g_ui32EntryLine;

//*****************************************************************************
//
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// Draws the prompt for each new line.
//
//*****************************************************************************
static void
DrawPrompt(void)
{
    int32_t i32Idx;

    g_ppcLines[g_ui32CurrentLine][0] = '>';

    for(i32Idx = 1; i32Idx < MAX_COLUMNS - 1; i32Idx++)
    {
        g_ppcLines[g_ui32CurrentLine][i32Idx] = ' ';
    }
    g_ppcLines[g_ui32CurrentLine][i32Idx] = 0;

    GrStringDraw(&g_sContext, g_ppcLines[g_ui32CurrentLine], MAX_COLUMNS,
              DISPLAY_TEXT_BORDER_H +
              (GrFontMaxWidthGet(g_psFontFixed6x8) * g_ui32Column),
              DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
              (g_ui32EntryLine * GrFontHeightGet(g_psFontFixed6x8)),
              true);

    g_ui32Column = 2;
}

//*****************************************************************************
//
// Initialize the application interface.
//
//*****************************************************************************
void
UIInit(uint32_t ui32SysClock)
{
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
    FrameDraw(&g_sContext, "usb-host-keyboard");

    //
    // Set the font for the application.
    //
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);

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
                            (2*(DISPLAY_BANNER_HEIGHT + 1)) -
                            BUTTON_HEIGHT) / GrFontHeightGet(g_psFontFixed6x8);

    //
    // Set up the text scrolling variables.
    //
    g_ui32CurrentLine = 0;
    g_ui32EntryLine = 0;

    //
    // Draw the initial prompt on the screen.
    //
    DrawPrompt();

    //
    // Initial update of the screen.
    //
    UIUpdateStatus();
}

//*****************************************************************************
//
// Handles scrolling the text on the screen.
//
//*****************************************************************************
static void
ScrollText(void)
{
    int32_t i32Idx;
    uint32_t ui32Line, ui32Start;

    ui32Line = 0;

    //
    // Skip the oldest entry in the circular list.
    //
    if(g_ui32CurrentLine == (MAX_LINES - 1))
    {
        //
        // If at the end of the list wrap to entry 1.
        //
        ui32Start = 1;
    }
    else
    {
        //
        // The oldest is 1 in front of the most recent.
        //
        ui32Start = g_ui32CurrentLine + 2;
    }

    //
    // Print lines from the current position down first.
    //
    for(i32Idx = ui32Start; i32Idx < MAX_LINES; i32Idx++)
    {
        GrStringDraw(&g_sContext, g_ppcLines[i32Idx],
                     strlen(g_ppcLines[i32Idx]), DISPLAY_TEXT_BORDER_H,
                     DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
                     (ui32Line * GrFontHeightGet(g_psFontFixed6x8)), 1);

        ui32Line++;
    }

    //
    // If not the special case of the last line where everything has already
    // printed, print the remaining lines.
    //
    if(g_ui32CurrentLine != (MAX_LINES - 1))
    {
        for(i32Idx = 0; i32Idx <= g_ui32CurrentLine; i32Idx++)
        {
            GrStringDraw(&g_sContext, g_ppcLines[i32Idx],
                         strlen(g_ppcLines[i32Idx]),
                         DISPLAY_TEXT_BORDER_H,
                         DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
                         (ui32Line * GrFontHeightGet(g_psFontFixed6x8)), 1);

            ui32Line++;
        }
    }

    //
    // Reset the column to zero.
    //
    g_ui32Column = 0;
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
UIPrintChar(const char cChar)
{
    bool bNewLine;
    int32_t i32Idx;

    GrContextForegroundSet(&g_sContext, ClrWhite);

    bNewLine = true;

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
                       (g_ui32EntryLine * GrFontHeightGet(g_psFontFixed6x8)),
                       1);

            g_ppcLines[g_ui32CurrentLine][g_ui32Column] = cChar;

            if(g_ui32Column < g_ui32CharsPerLine)
            {
                //
                // No line wrap yet so move one column over.
                //
                g_ui32Column++;

                bNewLine = false;
            }
        }
        else
        {
            //
            // We got a backspace.  If we are at the top left of the screen,
            // return since we don't need to do anything.
            //
            if(g_ui32Column || g_ui32EntryLine)
            {
                //
                // Adjust the cursor position to erase the last character.
                //
                if(g_ui32Column > 2)
                {
                    g_ui32Column--;
                }

                //
                // Print a space at this position then return without fixing up
                // the cursor again.
                //
                GrStringDraw(&g_sContext, " ", 1,
                          DISPLAY_TEXT_BORDER_H +
                          (GrFontMaxWidthGet(g_psFontFixed6x8) * g_ui32Column),
                          DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
                          (g_ui32EntryLine * GrFontHeightGet(g_psFontFixed6x8)),
                          true);

                g_ppcLines[g_ui32CurrentLine][g_ui32Column] = ' ';
            }

            bNewLine = false;
        }
    }

    //
    // .
    //
    if(bNewLine)
    {
        g_ui32Column = 0;

        if(g_ui32EntryLine < (MAX_LINES - 1))
        {
            g_ui32EntryLine++;
        }
        else
        {
            ScrollText();
        }

        g_ui32CurrentLine++;

        //
        // The line has gone past the end so go back to the first line.
        //
        if(g_ui32CurrentLine >= MAX_LINES)
        {
            g_ui32CurrentLine = 0;
        }

        //
        // Add a prompt to the new line.
        //
        if(cChar == '\n')
        {
            DrawPrompt();
        }
        else
        {
            //
            // Clear out the current line.
            //
            for(i32Idx = 0; i32Idx < MAX_COLUMNS - 1; i32Idx++)
            {
                g_ppcLines[g_ui32CurrentLine][i32Idx] = ' ';
            }
            g_ppcLines[g_ui32CurrentLine][i32Idx] = 0;

            GrStringDraw(&g_sContext, g_ppcLines[g_ui32CurrentLine],
                     strlen(g_ppcLines[g_ui32CurrentLine]),
                     DISPLAY_TEXT_BORDER_H,
                     DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
                     (g_ui32EntryLine * GrFontHeightGet(g_psFontFixed6x8)),
                     1);
        }
    }
}

//*****************************************************************************
//
// Update one of the status boxes at the bottom of the screen.
//
//*****************************************************************************
static void
UpdateStatusBox(const tRectangle *psRect, const char *pcString, bool bActive)
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
                   psRect->i16XMin + ((psRect->i16XMax - psRect->i16XMin) / 2),
                   psRect->i16YMin + (BUTTON_HEIGHT / 2), false);

}

//*****************************************************************************
//
// This function updates the status area of the screen.  It uses the current
// state of the application to print the status bar.
//
//*****************************************************************************
void
UIUpdateStatus(void)
{
    uint8_t ui8DevClass, ui8DevProtocol;
    static const tRectangle psRect[4] =
    {
        {
            DISPLAY_TEXT_BORDER_H,       240 - 10 - BUTTON_HEIGHT,
            DISPLAY_TEXT_BORDER_H + 124, 240 - 10
        },
        {
            DISPLAY_TEXT_BORDER_H + 124, 240 - 10 - BUTTON_HEIGHT,
            DISPLAY_TEXT_BORDER_H + 184, 240 - 10
        },
        {
            DISPLAY_TEXT_BORDER_H + 184, 240 - 10 - BUTTON_HEIGHT,
            DISPLAY_TEXT_BORDER_H + 244, 240 - 10
        },
        {
            DISPLAY_TEXT_BORDER_H + 244, 240 - 10 - BUTTON_HEIGHT,
            DISPLAY_TEXT_BORDER_H + 303, 240 - 10
        },
    };

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);

    //
    // Default the protocol to none so that non-HID devices do not update
    // the keyboard modifiers.
    //
    ui8DevProtocol = USB_HID_PROTOCOL_NONE;

    if(g_sStatus.bConnected)
    {
        ui8DevClass = USBHCDDevClass(g_sStatus.ui32Instance, 0);
        ui8DevProtocol = USBHCDDevProtocol(g_sStatus.ui32Instance, 0);

        if(ui8DevClass == USB_CLASS_HID)
        {
            if(ui8DevProtocol == USB_HID_PROTOCOL_MOUSE)
            {
                //
                // Mouse is currently connected.
                //
                UpdateStatusBox(&psRect[0], "Mouse", true);
            }
            else if(ui8DevProtocol == USB_HID_PROTOCOL_KEYB)
            {
                //
                // Keyboard is currently connected.
                //
                UpdateStatusBox(&psRect[0], "Keyboard", true);
            }
            else
            {
                //
                // Unknown device is currently connected.
                //
                UpdateStatusBox(&psRect[0], "Unknown", true);
            }
        }
        else if(ui8DevClass == USB_CLASS_MASS_STORAGE)
        {
            //
            // MSC device is currently connected.
            //
            UpdateStatusBox(&psRect[0], "Mass Storage", true);
        }
        else if(ui8DevClass == USB_CLASS_HUB)
        {
            //
            // MSC device is currently connected.
            //
            UpdateStatusBox(&psRect[0], "Hub", true);
        }
        else
        {
            //
            // Unknown device is currently connected.
            //
            UpdateStatusBox(&psRect[0], "Unknown", true);
        }
    }
    else
    {
        //
        // Unknown device is currently connected.
        //
        UpdateStatusBox(&psRect[0], "No Device", false);
    }

    if(ui8DevProtocol == USB_HID_PROTOCOL_KEYB)
    {
        if(g_sStatus.ui32Modifiers & HID_KEYB_CAPS_LOCK)
        {
            UpdateStatusBox(&psRect[1], "CAPS", true);
        }
        else
        {
            UpdateStatusBox(&psRect[1], "caps", false);
        }

        if(g_sStatus.ui32Modifiers & HID_KEYB_SCROLL_LOCK)
        {
            UpdateStatusBox(&psRect[2], "SCROLL", true);
        }
        else
        {
            UpdateStatusBox(&psRect[2], "scroll", false);
        }

        if(g_sStatus.ui32Modifiers & HID_KEYB_NUM_LOCK)
        {
            UpdateStatusBox(&psRect[3], "NUM", true);
        }
        else
        {
            UpdateStatusBox(&psRect[3], "num", false);
        }
    }
    else
    {
        UpdateStatusBox(&psRect[1], "caps", false);
        UpdateStatusBox(&psRect[2], "scroll", false);
        UpdateStatusBox(&psRect[3], "num", false);
    }
}
