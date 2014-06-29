//*****************************************************************************
//
// mouse_ui.h - The DK-TM4C129X USB mouse application UI.
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
#include "mouse_ui.h"
#include "utils/ustdlib.h"

//*****************************************************************************
//
// These defines are used to define the screen constraints to the application.
//
//*****************************************************************************
#define DISPLAY_BANNER_HEIGHT   18
#define DISPLAY_TEXT_BORDER     8
#define DISPLAY_TEXT_BORDER_H   8
#define BUTTON_HEIGHT           18
#define BUTTON_WIDTH            30
#define STATUS_MIN_Y            (240 - 10 - BUTTON_HEIGHT)
#define STATUS_MIDDLE_X         140
#define BUTTON_MIN_X            (MOUSE_MAX_X - (BUTTON_WIDTH * 3) - 1)

//*****************************************************************************
//
// The cursor rectangle.
//
//*****************************************************************************
tRectangle g_sCursor;

//*****************************************************************************
//
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// Used to remember if the UI has updated the connection status yet.
//
//*****************************************************************************
bool g_bTypeUpdated;

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
    FrameDraw(&g_sContext, "usb-host-mouse");

    //
    // Set the font for the application.
    //
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);

    //
    // Default to device type not yet updated.
    //
    g_bTypeUpdated = false;

    //
    // Initial update of the screen.
    //
    UIUpdateStatus();
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
// This function updates the cursor position based on the current state
// information in the g_sStatus structure.  This function takes the inputs and
// forces them to be constrained to the display area of the screen.  If the
// left mouse button is pressed then the mouse will draw on the screen and if
// it is not it will move around normally.  A side effect of not being able to
// read the current state of the screen is that the cursor will erase anything
// it moves over even while the left mouse button is not pressed.
//
// \return None.
//
//*****************************************************************************
static void
UpdateCursor(void)
{
    //
    // If the left button is not pressed then erase the previous cursor
    // position.
    //
    if((g_sStatus.ui32Buttons & 1) == 0)
    {
        //
        // Erase the previous cursor.
        //
        GrContextForegroundSet(&g_sContext, DISPLAY_MOUSE_BG);
        GrRectFill(&g_sContext, &g_sCursor);
    }

    //
    // Update the X position.
    //
    if(g_sStatus.ui32XPos >= (MOUSE_MAX_X - DISPLAY_MOUSE_SIZE))
    {
        g_sCursor.i16XMin = MOUSE_MAX_X - DISPLAY_MOUSE_SIZE - 1;
    }
    else if(g_sStatus.ui32XPos < MOUSE_MIN_X)
    {
        g_sCursor.i16XMin = MOUSE_MIN_X;
    }
    else
    {
        g_sCursor.i16XMin = g_sStatus.ui32XPos;
    }

    g_sCursor.i16XMax = g_sCursor.i16XMin + DISPLAY_MOUSE_SIZE;

    //
    // Update the Y position.
    //
    if(g_sStatus.ui32YPos >= (MOUSE_MAX_Y - DISPLAY_MOUSE_SIZE))
    {
        g_sCursor.i16YMin = MOUSE_MAX_Y - DISPLAY_MOUSE_SIZE - 1;
    }
    else if(g_sStatus.ui32YPos < MOUSE_MIN_Y)
    {
        g_sCursor.i16YMin = MOUSE_MIN_Y;
    }
    else
    {
        g_sCursor.i16YMin = g_sStatus.ui32YPos;
    }

    g_sCursor.i16YMax = g_sCursor.i16YMin + DISPLAY_MOUSE_SIZE;

    //
    // Draw the cursor at the new position.
    //
    GrContextForegroundSet(&g_sContext, DISPLAY_MOUSE_FG);
    GrRectFill(&g_sContext, &g_sCursor);
}

//*****************************************************************************
//
// This function will update the mouse button indicators in the status
// bar area of the screen.
//
//*****************************************************************************
static void
UpdateButtons(void)
{
    tRectangle sRect, sRectInner;
    int iButton;

    //
    // Initialize the button indicator position.
    //
    sRect.i16XMin = BUTTON_MIN_X;
    sRect.i16YMin = STATUS_MIN_Y;
    sRect.i16XMax = sRect.i16XMin + BUTTON_WIDTH;
    sRect.i16YMax = sRect.i16YMin + BUTTON_HEIGHT ;
    sRectInner.i16XMin = sRect.i16XMin + 1;
    sRectInner.i16YMin = sRect.i16YMin + 1;
    sRectInner.i16XMax = sRect.i16XMax - 1;
    sRectInner.i16YMax = sRect.i16YMax - 1;

    //
    // Check all three buttons.
    //
    for(iButton = 0; iButton < 3; iButton++)
    {
        //
        // Draw the button indicator red if pressed and black if not pressed.
        //
        if(g_sStatus.ui32Buttons & (1 << iButton))
        {
            GrContextForegroundSet(&g_sContext, ClrRed);
        }
        else
        {
            GrContextForegroundSet(&g_sContext, ClrBlack);
        }

        //
        // Draw the back of the  button indicator.
        //
        GrRectFill(&g_sContext, &sRectInner);

        //
        // Draw the border on the button indicator.
        //
        GrContextForegroundSet(&g_sContext, ClrWhite);
        GrRectDraw(&g_sContext, &sRect);

        //
        // Move to the next button indicator position.
        //
        sRect.i16XMin += BUTTON_WIDTH;
        sRect.i16XMax += BUTTON_WIDTH;
        sRectInner.i16XMin += BUTTON_WIDTH;
        sRectInner.i16XMax += BUTTON_WIDTH;
    }
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
    static const char pcNoPos[] = "---,---";
    static const tRectangle psRect[2] =
    {
        {
            DISPLAY_TEXT_BORDER_H,
            STATUS_MIN_Y,
            DISPLAY_TEXT_BORDER_H + STATUS_MIDDLE_X,
            STATUS_MIN_Y + BUTTON_HEIGHT
        },
        {
            DISPLAY_TEXT_BORDER_H + STATUS_MIDDLE_X,
            STATUS_MIN_Y,
            BUTTON_MIN_X,
            STATUS_MIN_Y + BUTTON_HEIGHT
        },
    };

    char pcPos[8];

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);

    //
    // Default the protocol to none.
    //
    ui8DevProtocol = USB_HID_PROTOCOL_NONE;

    if(g_sStatus.bConnected)
    {
        ui8DevClass = USBHCDDevClass(g_sStatus.ui32Instance, 0);
        ui8DevProtocol = USBHCDDevProtocol(g_sStatus.ui32Instance, 0);

        //
        // If the class has not yet been recognized then print out the new
        // class of device that has been connected.
        //
        if(g_bTypeUpdated == false)
        {
            //
            // Now the class has been updated so do not update it again.
            //
            g_bTypeUpdated = true;

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
    }
    else
    {
        //
        // Unknown device is currently connected.
        //
        UpdateStatusBox(&psRect[0], "No Device", false);

        //
        // No device is present so allow the class to update when a new device
        // is connected.
        //
        g_bTypeUpdated = false;
    }

    if(ui8DevProtocol == USB_HID_PROTOCOL_MOUSE)
    {
        //
        // Update the current cursor position.
        //
        UpdateCursor();

        usprintf(pcPos, "%3d,%3d", g_sCursor.i16XMin, g_sCursor.i16YMin);
        UpdateStatusBox(&psRect[1], pcPos, false);
    }
    else
    {
        UpdateStatusBox(&psRect[1], pcNoPos, false);
    }

    UpdateButtons();
}
