//*****************************************************************************
//
// ui.c - User interface code for the USB composite HID keyboard/mouse example.
// This file seperates out the USB library accesses and the general hardware
// access and solely handles the user interface.
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
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/keyboard.h"
#include "grlib/pushbutton.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "ui.h"

//*****************************************************************************
//
// Global graphics context.
//
//*****************************************************************************
static tContext g_sContext;

static void ToggleMode(tWidget *psWidget);
static void Status1(tWidget *psWidget);
static void Status2(tWidget *psWidget);
static void Status3(tWidget *psWidget);

//*****************************************************************************
//
// Defines for the basic screen area used by the application.
//
//*****************************************************************************
#define STATUS_HEIGHT           40
#define BG_MIN_X                7
#define BG_MAX_X                (320 - 8)
#define BG_MIN_Y                24
#define BG_MAX_Y                (240 - 8)
#define BUTTON_HEIGHT           (STATUS_HEIGHT - 8)
#define BG_COLOR_SETTINGS       ClrGray
#define BG_COLOR_MAIN           ClrBlack

//*****************************************************************************
//
// The global UI state.
//
//*****************************************************************************
static struct
{
    uint32_t ui32Indicators;
}
g_sUIState;

//*****************************************************************************
//
// The defined values used with ui32Indicators.
//
//*****************************************************************************
#define UI_STATUS_MS_RIGHT      0x00000001
#define UI_STATUS_MS_MIDDLE     0x00000002
#define UI_STATUS_MS_LEFT       0x00000004

#define UI_STATUS_KEY_CAPS      0x00000001
#define UI_STATUS_KEY_SCROLL    0x00000002
#define UI_STATUS_KEY_NUM       0x00000004

#define UI_STATUS_UPDATE        0x80000000
#define UI_STATUS_KEYBOARD      0x00000008
#define UI_STATUS_MOUSE         0x00000000

//*****************************************************************************
//
// The graphical widgets that are used by this example.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;
extern tCanvasWidget g_sStatusPanel;

//
// The keyboard widget used by the application.
//
Keyboard(g_sKeyboard, &g_sBackground, 0, 0,
         &g_sKentec320x240x16_SSD2119, BG_MIN_X + 2, BG_MIN_Y + 4, 300, 160,
         KEYBOARD_STYLE_FILL | KEYBOARD_STYLE_AUTO_REPEAT |
         KEYBOARD_STYLE_PRESS_NOTIFY | KEYBOARD_STYLE_RELEASE_NOTIFY |
         KEYBOARD_STYLE_BG,
         ClrBlack, ClrGray, ClrDarkGray, ClrGray, ClrBlack, g_psFontCmtt14,
         100, 100, NUM_KEYBOARD_US_ENGLISH, g_psKeyboardUSEnglish, UIKeyEvent);

//
// The full background for the application.
//
Canvas(g_sBackground, WIDGET_ROOT, 0, &g_sStatusPanel,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X, BG_MAX_Y - BG_MIN_Y - STATUS_HEIGHT,
       CANVAS_STYLE_FILL, ClrBlack, ClrWhite, ClrWhite, 0, 0, 0 ,0 );


//
// Right button in mouse mode and Num Lock in keyboard mode.
//
RectangularButton(g_sStatus3, &g_sStatusPanel, 0, 0,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X + 250,
       BG_MAX_Y - STATUS_HEIGHT + 4, 50, BUTTON_HEIGHT,
       PB_STYLE_FILL | PB_STYLE_TEXT |
       PB_STYLE_RELEASE_NOTIFY, ClrLightGrey, ClrDarkGray, 0,
       ClrBlack, g_psFontCmss16, "Right", 0, 0, 0 ,0 , Status3);

//
// Middle button in mouse mode and Scroll Lock in keyboard mode.
//
RectangularButton(g_sStatus2, &g_sStatusPanel, &g_sStatus3, 0,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X + 196,
       BG_MAX_Y - STATUS_HEIGHT + 4, 50, BUTTON_HEIGHT,
       PB_STYLE_FILL | PB_STYLE_TEXT |
       PB_STYLE_RELEASE_NOTIFY, ClrLightGrey, ClrDarkGray, 0,
       ClrBlack, g_psFontCmss16, "Middle", 0, 0, 0 ,0 , Status2);

//
// Left button in mouse mode and Caps Lock in keyboard mode.
//
RectangularButton(g_sStatus1, &g_sStatusPanel, &g_sStatus2, 0,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X + 142,
       BG_MAX_Y - STATUS_HEIGHT + 4, 50, BUTTON_HEIGHT,
       PB_STYLE_FILL | PB_STYLE_TEXT |
       PB_STYLE_RELEASE_NOTIFY, ClrLightGrey, ClrDarkGray, 0,
       ClrBlack, g_psFontCmss16, "Left", 0, 0, 0 ,0 , Status1);

//
// The mode toggle button.
//
RectangularButton(g_sToggle, &g_sStatusPanel, &g_sStatus1, 0,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X + 4,
       BG_MAX_Y - STATUS_HEIGHT + 4, 80, BUTTON_HEIGHT,
       PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY, ClrLightGrey,
       ClrDarkGray, 0, ClrBlack, g_psFontCmss16, "Mouse", 0, 0, 0 ,0 ,
       ToggleMode);

//
// Background of the status area behind the buttons.
//
Canvas(g_sStatusPanel, &g_sBackground, 0, &g_sToggle,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X, BG_MAX_Y - STATUS_HEIGHT,
       BG_MAX_X - BG_MIN_X, STATUS_HEIGHT,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_TOP, ClrGray, ClrWhite, ClrBlack, 0,
       0, 0, 0);

tUIState g_eState;

//****************************************************************************
//
// This is the callback from the graphical keyboard that is defined above as
// g_sKeyboard global.
//
//****************************************************************************
void
UIKeyEvent(tWidget *psWidget, uint32_t ui32Key, uint32_t ui32Event)
{
    if(ui32Event == KEYBOARD_EVENT_PRESS)
    {
        USBKeyboardUpdate(0, ui32Key, true);

    }
    else if(ui32Event == KEYBOARD_EVENT_RELEASE)
    {
        USBKeyboardUpdate(0, ui32Key, false);
    }
}

//****************************************************************************
//
// Handles updating the Caps Lock status when in keyboard mode.
//
//****************************************************************************
void
UIUpdateCapsLock(void)
{
    if(g_sUIState.ui32Indicators & UI_STATUS_KEY_CAPS)
    {
        PushButtonTextSet(&g_sStatus1, "CAPS");
        PushButtonTextColorSet(&g_sStatus1, ClrRed);
    }
    else
    {
        PushButtonTextSet(&g_sStatus1, "caps");
        PushButtonTextColorSet(&g_sStatus1, ClrBlack);
    }

    WidgetPaint((tWidget *)&g_sStatus1);
}

void
UICapsLock(bool bEnable)
{
    if(bEnable)
    {
        //
        // Enable caps lock.
        //
        g_sUIState.ui32Indicators |= UI_STATUS_KEY_CAPS;
    }
    else
    {
        //
        // Disable caps lock.
        //
        g_sUIState.ui32Indicators &= ~UI_STATUS_KEY_CAPS;
    }

    if(g_sUIState.ui32Indicators & UI_STATUS_KEYBOARD)
    {
        UIUpdateCapsLock();
    }
}

//****************************************************************************
//
// Handles updating the Scroll Lock status when in keyboard mode.
//
//****************************************************************************
void
UIUpdateScrollLock(void)
{
    if(g_sUIState.ui32Indicators & UI_STATUS_KEY_SCROLL)
    {
        PushButtonTextSet(&g_sStatus2, "SCROLL");
        PushButtonTextColorSet(&g_sStatus2, ClrRed);
    }
    else
    {
        PushButtonTextSet(&g_sStatus2, "scroll");
        PushButtonTextColorSet(&g_sStatus2, ClrBlack);
    }

    WidgetPaint((tWidget *)&g_sStatus2);
}

void
UIScrollLock(bool bEnable)
{
    if(bEnable)
    {
        //
        // Enable caps lock.
        //
        g_sUIState.ui32Indicators |= UI_STATUS_KEY_SCROLL;
    }
    else
    {
        //
        // Disable caps lock.
        //
        g_sUIState.ui32Indicators &= ~UI_STATUS_KEY_SCROLL;
    }

    if(g_sUIState.ui32Indicators & UI_STATUS_KEYBOARD)
    {
        UIUpdateScrollLock();
    }
}

//****************************************************************************
//
// Handles updating the Num Lock status when in keyboard mode.
//
//****************************************************************************
void
UIUpdateNumLock(void)
{
    if(g_sUIState.ui32Indicators & UI_STATUS_KEY_NUM)
    {
        PushButtonTextSet(&g_sStatus3, "NUM");
        PushButtonTextColorSet(&g_sStatus3, ClrRed);
    }
    else
    {
        PushButtonTextSet(&g_sStatus3, "num");
        PushButtonTextColorSet(&g_sStatus3, ClrBlack);
    }

    WidgetPaint((tWidget *)&g_sStatus3);
}

void
UINumLock(bool bEnable)
{
    if(bEnable)
    {
        //
        // Enable caps lock.
        //
        g_sUIState.ui32Indicators |= UI_STATUS_KEY_NUM;
    }
    else
    {
        //
        // Disable caps lock.
        //
        g_sUIState.ui32Indicators &= ~UI_STATUS_KEY_NUM;
    }

    if(g_sUIState.ui32Indicators & UI_STATUS_KEYBOARD)
    {
        UIUpdateNumLock();
    }
}

//****************************************************************************
//
// Handles updates to the current status.
//
//****************************************************************************
void
UIUpdateStatus(uint32_t ui32Indicators)
{
    if(g_eState == UI_NOT_CONNECTED)
    {
        PushButtonTextSet(&g_sToggle, "---");

        PushButtonTextColorSet(&g_sStatus1, ClrBlack);
        PushButtonTextSet(&g_sStatus1, "---");

        PushButtonTextColorSet(&g_sStatus2, ClrBlack);
        PushButtonTextSet(&g_sStatus2, "---");

        PushButtonTextColorSet(&g_sStatus3, ClrBlack);
        PushButtonTextSet(&g_sStatus3, "---");

        if(ui32Indicators & UI_STATUS_KEYBOARD)
        {
            WidgetRemove((tWidget *)&g_sKeyboard);
        }

        WidgetPaint((tWidget *)&g_sBackground);

        return;
    }

    //
    // See if there is a change to update.
    //
    if(ui32Indicators == g_sUIState.ui32Indicators)
    {
        return;
    }

    //
    // Was there a global change in the keyboard/mouse state.
    //
    if(((ui32Indicators ^ g_sUIState.ui32Indicators) & UI_STATUS_KEYBOARD) ||
       (ui32Indicators & UI_STATUS_UPDATE))
    {
        //
        // Update to keyboard mode or mouse mode for the UI.
        //
        if(ui32Indicators & UI_STATUS_KEYBOARD)
        {
            PushButtonTextSet(&g_sToggle, "Keyboard");
            WidgetPaint((tWidget *)&g_sToggle);
            UIUpdateCapsLock();
            UIUpdateScrollLock();
            UIUpdateNumLock();

            WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sKeyboard);

            WidgetPaint((tWidget *)&g_sKeyboard);
        }
        else
        {
            //
            // Switch back to a mouse UI.
            //
            PushButtonTextSet(&g_sToggle, "Mouse");

            PushButtonTextColorSet(&g_sStatus1, ClrBlack);
            PushButtonTextSet(&g_sStatus1, "Left");

            PushButtonTextColorSet(&g_sStatus2, ClrBlack);
            PushButtonTextSet(&g_sStatus2, "Middle");

            PushButtonTextColorSet(&g_sStatus3, ClrBlack);
            PushButtonTextSet(&g_sStatus3, "Right");

            WidgetRemove((tWidget *)&g_sKeyboard);

            WidgetPaint((tWidget *)&g_sBackground);
        }
    }

    //
    // Update the new state of the indicators.
    //
    g_sUIState.ui32Indicators = ui32Indicators & ~UI_STATUS_UPDATE;
}


//****************************************************************************
//
// Called by the mode toggle button in the g_sToggle variable.
//
//****************************************************************************
static void
ToggleMode(tWidget *psWidget)
{
    if(g_eState == UI_CONNECTED)
    {
        UIUpdateStatus(g_sUIState.ui32Indicators ^ UI_STATUS_KEYBOARD);
    }
}

void
UIMode(tUIState eState)
{
    if(g_eState != UI_CONNECTED)
    {
        if(eState == UI_CONNECTED)
        {
            UIUpdateStatus(g_sUIState.ui32Indicators | UI_STATUS_UPDATE);
        }
    }
    else if(g_eState == UI_CONNECTED)
    {
        if(eState == UI_NOT_CONNECTED)
        {
            UIUpdateStatus(g_sUIState.ui32Indicators & ~UI_STATUS_UPDATE);
        }
    }

    g_eState = eState;
    UIUpdateStatus(g_sUIState.ui32Indicators);
}

//****************************************************************************
//
// Called by the button code controlled by the g_sStatus1 variable.
//
//****************************************************************************
static void
Status1(tWidget *psWidget)
{
    if(g_eState == UI_CONNECTED)
    {
        //
        // Toggle the state of the first status button.
        //
        if(g_sUIState.ui32Indicators & UI_STATUS_KEYBOARD)
        {
            //
            // Toggle the state of the caps lock.
            //
            g_sUIState.ui32Indicators ^= UI_STATUS_KEY_CAPS;

            UIUpdateCapsLock();

            USBKeyboardUpdate(0, UI_CAPS_LOCK, true);
        }
        else
        {
            USBMouseUpdate(0, 0, 0x01);
        }
    }
}

//****************************************************************************
//
// Called by the button code controlled by the g_sStatus2 variable.
//
//****************************************************************************
static void
Status2(tWidget *psWidget)
{
    if(g_eState == UI_CONNECTED)
    {
        //
        // Toggle the state of the first status button.
        //
        if(g_sUIState.ui32Indicators & UI_STATUS_KEYBOARD)
        {
            //
            // Toggle the state of the caps lock.
            //
            g_sUIState.ui32Indicators ^= UI_STATUS_KEY_SCROLL;

            UIUpdateScrollLock();

            USBKeyboardUpdate(0, UI_SCROLL_LOCK, true);
        }
        else
        {
            USBMouseUpdate(0, 0, 0x04);
        }
    }
}

//****************************************************************************
//
// Called by the button code controlled by the g_sStatus3 variable.
//
//****************************************************************************
static void
Status3(tWidget *psWidget)
{
    if(g_eState == UI_CONNECTED)
    {
        //
        // Toggle the state of the first status button.
        //
        if(g_sUIState.ui32Indicators & UI_STATUS_KEYBOARD)
        {
            //
            // Toggle the state of the caps lock.
            //
            g_sUIState.ui32Indicators ^= UI_STATUS_KEY_NUM;

            UIUpdateNumLock();
            USBKeyboardUpdate(0, UI_NUM_LOCK, true);
        }
        else
        {
            USBMouseUpdate(0, 0, 0x02);
        }
    }
}

static struct
{
    int32_t i32XLast;
    int32_t i32YLast;
    int8_t i8Buttons;
    bool bPressed;
}
sMouseState;

volatile uint32_t ui32PressCount;

//*****************************************************************************
//
// The callback function that is called by the touch screen driver to indicate
// activity on the touch screen.
//
//*****************************************************************************
int32_t
UITouchCallback(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    int32_t i32XDiff, i32YDiff;

    if(((g_sUIState.ui32Indicators & UI_STATUS_KEYBOARD) == 0) &&
       ((i32Y > BG_MIN_Y) && (i32Y < (BG_MAX_Y - STATUS_HEIGHT))))
    {

        switch(ui32Message)
        {
            //
            // .
            //
            case WIDGET_MSG_PTR_DOWN:
            {
                sMouseState.i32XLast = i32X;
                sMouseState.i32YLast = i32Y;

                sMouseState.bPressed = true;

                ui32PressCount = g_ui32SysTickCount;
                break;
            }


            // The touchscreen is no longer being pressed.
            //
            case WIDGET_MSG_PTR_UP:
            {
                sMouseState.bPressed = false;

                if(ui32PressCount > g_ui32SysTickCount)
                {
                    ui32PressCount = ui32PressCount - g_ui32SysTickCount;
                }
                else
                {
                    ui32PressCount = g_ui32SysTickCount - ui32PressCount;
                }

                if(ui32PressCount < 20)
                {
                    //
                    // Ensure that all buttons are not pressed.
                    //
                    sMouseState.i8Buttons = 0x01;
                }
                else
                {
                    //
                    // Ensure that all buttons are not pressed.
                    //
                    sMouseState.i8Buttons = 0x00;
                }

                //
                // Send the report back to the host.
                //
                USBMouseUpdate(0, 0, sMouseState.i8Buttons);

                break;
            }

            //
            // The touch position has moved.
            //
            case WIDGET_MSG_PTR_MOVE:
            {
                //
                // Send the difference not the absolute value.
                //
                i32XDiff = i32X - sMouseState.i32XLast;
                i32YDiff = i32Y - sMouseState.i32YLast;

                //
                // Send the report back to the host.
                //
                USBMouseUpdate(i32XDiff, i32YDiff, 0);

                //
                // Save these values to calculate the next move.
                //
                sMouseState.i32XLast = i32X;
                sMouseState.i32YLast = i32Y;

                break;
            }
            default:
            {
                break;
            }
        }
    }
    else
    {
        WidgetPointerMessage(ui32Message, i32X, i32Y);
    }

    return(0);
}


//****************************************************************************
//
// Application calls this once to initialize the UI.
//
//****************************************************************************
void
UIInit(void)
{
    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&g_sContext, "usb-dev-chid");

    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sBackground);
    WidgetPaint((tWidget *)&g_sBackground);

    //
    // Initially not connected.
    //
    g_eState = UI_NOT_CONNECTED;

    //
    // Not initialized.
    //
    UIUpdateStatus(UI_STATUS_UPDATE);

}

//****************************************************************************
//
// Application should periodically call this function.
//
//****************************************************************************
void
UIMain(void)
{
    WidgetMessageQueueProcess();

    if(g_sUIState.ui32Indicators & UI_STATUS_KEYBOARD)
    {

    }
    else
    {
        USBMouseMain();
    }
}
