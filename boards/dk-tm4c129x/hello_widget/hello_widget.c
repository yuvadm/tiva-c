//*****************************************************************************
//
// hello_widget.c - Simple hello world example using Widgets
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
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Hello using Widgets (hello_widget)</h1>
//!
//! A very simple ``hello world'' example written using the TivaWare Graphics
//! Library widgets.  It displays a button which, when pressed, toggles
//! display of the words ``Hello World!'' on the screen.  This may be used as
//! a starting point for more complex widget-based applications.
//
//*****************************************************************************

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;
extern tPushButtonWidget g_sPushBtn;

//*****************************************************************************
//
// Forward reference to the button press handler.
//
//*****************************************************************************
void OnButtonPress(tWidget *psWidget);

//*****************************************************************************
//
// The canvas widget acting as the background to the display.
//
//*****************************************************************************
Canvas(g_sBackground, WIDGET_ROOT, 0, &g_sPushBtn,
       &g_sKentec320x240x16_SSD2119, 10, 25, 300, (240 - 25 -10),
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The button used to hide or display the "Hello World" message.
//
//*****************************************************************************
RectangularButton(g_sPushBtn, &g_sBackground, 0, 0,
                  &g_sKentec320x240x16_SSD2119, 60, 60, 200, 40,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
                   g_psFontCmss22b, "Show Welcome", 0, 0, 0, 0, OnButtonPress);

//*****************************************************************************
//
// The canvas widget used to display the "Hello!" string.  Note that this
// is NOT hooked into the active widget tree (by making it a child of the
// g_sPushBtn widget above) yet since we do not want the widget to be displayed
// until the button is pressed.
//
//*****************************************************************************
Canvas(g_sHello, &g_sPushBtn, 0, 0,
       &g_sKentec320x240x16_SSD2119, 10, 150, 300, 40,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, g_psFontCm40, "Hello World!", 0, 0);

//*****************************************************************************
//
// A global we use to keep track of whether or not the "Hello" widget is
// currently visible.
//
//*****************************************************************************
bool g_bHelloVisible = false;

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
// This function is called by the graphics library widget manager in the
// context of WidgetMessageQueueProcess whenever the user releases the
// "Press Me!" button.  We use this notification to display or hide the
// "Hello!" widget.
//
// This is actually a rather inefficient way to accomplish this but it's
// a good example of how to add and remove widgets dynamically.  In
// normal circumstances, you would likely leave the g_sHello widget
// linked into the tree and merely add or remove the text by changing
// its style then repainting.
//
// If using this dynamic add/remove strategy, another useful optimization
// is to use a black canvas widget that covers the same area of the screen
// as the widgets that you will be adding and removing.  If this is the used
// as the point in the tree where the subtree is added or removed, you can
// repaint just the desired area by repainting the black canvas rather than
// repainting the whole tree.
//
//*****************************************************************************
void
OnButtonPress(tWidget *psWidget)
{
    g_bHelloVisible = !g_bHelloVisible;

    if(g_bHelloVisible)
    {
        //
        // Add the Hello widget to the tree as a child of the push
        // button.  We could add it elsewhere but this seems as good a
        // place as any.  It also means we can repaint from g_sPushBtn and
        // this will paint both the button and the welcome message.
        //
        WidgetAdd((tWidget *)&g_sPushBtn, (tWidget *)&g_sHello);

        //
        // Change the button text to indicate the new function.
        //
        PushButtonTextSet(&g_sPushBtn, "Hide Welcome");

        //
        // Repaint the pushbutton and all widgets beneath it (in this case,
        // the welcome message).
        //
        WidgetPaint((tWidget *)&g_sPushBtn);
    }
    else
    {
        //
        // Remove the Hello widget from the tree.
        //
        WidgetRemove((tWidget *)&g_sHello);

        //
        // Change the button text to indicate the new function.
        //
        PushButtonTextSet(&g_sPushBtn, "Show Welcome");

        //
        // Repaint the widget tree to remove the Hello widget from the
        // display.  This is rather inefficient but saves having to use
        // additional widgets to overpaint the area of the Hello text (since
        // disabling a widget does not automatically erase whatever it
        // previously displayed on the screen).
        //
        WidgetPaint(WIDGET_ROOT);
    }
}

//*****************************************************************************
//
// Print "Hello World!" to the display on the Intelligent Display Module.
//
//*****************************************************************************
int
main(void)
{
    tContext sContext;
    uint32_t ui32SysClock;

    //
    // Run from the PLL at 120 MHz.
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(ui32SysClock);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&sContext, "hello-widget");

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(ui32SysClock);

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sBackground);

    //
    // Paint the widget tree to make sure they all appear on the display.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Loop forever, processing widget messages.
    //
    while(1)
    {
        //
        // Process any messages from or for the widgets.
        //
        WidgetMessageQueueProcess();
    }
}
