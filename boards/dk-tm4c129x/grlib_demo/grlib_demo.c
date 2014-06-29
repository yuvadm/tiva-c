//*****************************************************************************
//
// grlib_demo.c - Demonstration of the TivaWare Graphics Library.
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
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/checkbox.h"
#include "grlib/container.h"
#include "grlib/pushbutton.h"
#include "grlib/radiobutton.h"
#include "grlib/slider.h"
#include "utils/ustdlib.h"
#include "utils/sine.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/sound.h"
#include "drivers/touch.h"
#include "images.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Graphics Library Demonstration (grlib_demo)</h1>
//!
//! This application provides a demonstration of the capabilities of the
//! TivaWare Graphics Library.  A series of panels show different features of
//! the library.  For each panel, the bottom provides a forward and back button
//! (when appropriate), along with a brief description of the contents of the
//! panel.
//!
//! The first panel provides some introductory text and basic instructions for
//! operation of the application.
//!
//! The second panel shows the available drawing primitives: lines, circles,
//! rectangles, strings, and images.
//!
//! The third panel shows the canvas widget, which provides a general drawing
//! surface within the widget hierarchy.  A text, image, and application-drawn
//! canvas are displayed.
//!
//! The fourth panel shows the check box widget, which provides a means of
//! toggling the state of an item.  Three check boxes are provided, with each
//! having a red ``LED'' to the right.  The state of the LED tracks the state
//! of the check box via an application callback.
//!
//! The fifth panel shows the container widget, which provides a grouping
//! construct typically used for radio buttons.  Containers with a title, a
//! centered title, and no title are displayed.
//!
//! The sixth panel shows the push button widget.  Two rows of push buttons
//! are provided; the appearance of each row is the same but the top row
//! does not utilize auto-repeat while the bottom row does. Each push button
//! has a red ``LED'' beneath it, which is toggled via an application callback
//! each time the push button is pressed. While holding down any of
//! auto-repeat buttons, the ``LED'' for that button should be toggled as long
//! as the button is being held down.
//!
//! The seventh panel shows the radio button widget.  Two groups of radio
//! buttons are displayed, the first using text and the second using images for
//! the selection value.  Each radio button has a red ``LED'' to its right,
//! which tracks the selection state of the radio buttons via an application
//! callback.  Only one radio button from each group can be selected at a time,
//! though the radio buttons in each group operate independently.
//!
//! The eighth and final panel shows the slider widget.  Six sliders
//! constructed using the various supported style options are shown.  The
//! slider value callback is used to update two widgets to reflect the values
//! reported by sliders.  A canvas widget near the top right of the display
//! tracks the value of the red and green image-based slider to its left and
//! the text of the grey slider on the left side of the panel is update to show
//! its own value.  The slider on the right is configured as an indicator
//! which tracks the state of the upper slider and ignores user input.
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
// The DMA control structure table.
//
//*****************************************************************************
#ifdef ewarm
#pragma data_alignment=1024
tDMAControlTable psDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(psDMAControlTable, 1024)
tDMAControlTable psDMAControlTable[64];
#else
tDMAControlTable psDMAControlTable[64] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// The sound effect that is played when a key is pressed.
//
//*****************************************************************************
#define AUDIO_SIZE              2048
static int16_t g_pi16AudioBuffer[AUDIO_SIZE];

//*****************************************************************************
//
// A set of flags that indicate the current state of the application.
//
//*****************************************************************************
static uint32_t g_ui32Flags;
#define FLAG_PING               0           // The "ping" half of the sound
                                            // buffer needs to be filled
#define FLAG_PONG               1           // The "pong" half of the sound
                                            // buffer needs to be filled

//*****************************************************************************
//
// The screen offset of the upper left hand corner where we start to draw.
//
//*****************************************************************************
#define X_OFFSET                8
#define Y_OFFSET                24

//*****************************************************************************
//
// Forward declarations for the globals required to define the widgets at
// compile-time.
//
//*****************************************************************************
void OnPrevious(tWidget *psWidget);
void OnNext(tWidget *psWidget);
void OnIntroPaint(tWidget *psWidget, tContext *psContext);
void OnPrimitivePaint(tWidget *psWidget, tContext *psContext);
void OnCanvasPaint(tWidget *psWidget, tContext *psContext);
void OnCheckChange(tWidget *psWidget, uint32_t bSelected);
void OnButtonPress(tWidget *psWidget);
void OnRadioChange(tWidget *psWidget, uint32_t bSelected);
void OnSliderChange(tWidget *psWidget, int32_t i32Value);
extern tCanvasWidget g_psPanels[];
void SoundCallback(uint32_t ui32Half);

//*****************************************************************************
//
// The first panel, which contains introductory text explaining the
// application.
//
//*****************************************************************************
Canvas(g_sIntroduction, g_psPanels, 0, 0, &g_sKentec320x240x16_SSD2119,
       X_OFFSET, Y_OFFSET, (320 - (X_OFFSET*2)),  158,
       CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnIntroPaint);

//*****************************************************************************
//
// The second panel, which demonstrates the graphics primitives.
//
//*****************************************************************************
Canvas(g_sPrimitives, g_psPanels + 1, 0, 0, &g_sKentec320x240x16_SSD2119,
       X_OFFSET, Y_OFFSET, (320 - (X_OFFSET*2)),  158,
       CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnPrimitivePaint);

//*****************************************************************************
//
// The third panel, which demonstrates the canvas widget.
//
//*****************************************************************************
Canvas(g_sCanvas3, g_psPanels + 2, 0, 0, &g_sKentec320x240x16_SSD2119, 200,
       Y_OFFSET, 110, 152, CANVAS_STYLE_OUTLINE | CANVAS_STYLE_APP_DRAWN, 0,
       ClrGray, 0, 0, 0, 0, OnCanvasPaint);
Canvas(g_sCanvas2, g_psPanels + 2, &g_sCanvas3, 0,
       &g_sKentec320x240x16_SSD2119, X_OFFSET, (76+Y_OFFSET), 190, 76,
       CANVAS_STYLE_OUTLINE | CANVAS_STYLE_IMG, 0, ClrGray, 0, 0, 0,
       g_ui8Logo, 0);
Canvas(g_sCanvas1, g_psPanels + 2, &g_sCanvas2, 0,
       &g_sKentec320x240x16_SSD2119, X_OFFSET, Y_OFFSET, 190, 76,
       CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT,
       ClrMidnightBlue, ClrGray, ClrSilver, g_psFontCm22, "Text", 0, 0);

//*****************************************************************************
//
// The fourth panel, which demonstrates the checkbox widget.
//
//*****************************************************************************
tCanvasWidget g_psCheckBoxIndicators[] =
{
    CanvasStruct(g_psPanels + 3, g_psCheckBoxIndicators + 1, 0,
                 &g_sKentec320x240x16_SSD2119, 230, 30, 50, 42,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(g_psPanels + 3, g_psCheckBoxIndicators + 2, 0,
                 &g_sKentec320x240x16_SSD2119, 230, 82, 50, 48,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(g_psPanels + 3, 0, 0,
                 &g_sKentec320x240x16_SSD2119, 230, 134, 50, 42,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0)
};
tCheckBoxWidget g_psCheckBoxes[] =
{
    CheckBoxStruct(g_psPanels + 3, g_psCheckBoxes + 1, 0,
                   &g_sKentec320x240x16_SSD2119, 40, 25, 185, 42,
                   CB_STYLE_OUTLINE | CB_STYLE_FILL | CB_STYLE_TEXT, 16,
                   ClrMidnightBlue, ClrGray, ClrSilver, g_psFontCm22,
                   "Select", 0, OnCheckChange),
    CheckBoxStruct(g_psPanels + 3, g_psCheckBoxes + 2, 0,
                   &g_sKentec320x240x16_SSD2119, 40, 78, 185, 48,
                   CB_STYLE_IMG, 16, 0, ClrGray, 0, 0, 0, g_ui8Logo,
                   OnCheckChange),
    CheckBoxStruct(g_psPanels + 3, g_psCheckBoxIndicators, 0,
                   &g_sKentec320x240x16_SSD2119, 40, 129, 189, 42,
                   CB_STYLE_OUTLINE | CB_STYLE_TEXT, 16,
                   0, ClrGray, ClrGreen, g_psFontCm20, "Select",
                   0, OnCheckChange),
};
#define NUM_CHECK_BOXES         (sizeof(g_psCheckBoxes) /   \
                                 sizeof(g_psCheckBoxes[0]))

//*****************************************************************************
//
// The fifth panel, which demonstrates the container widget.
//
//*****************************************************************************
Container(g_sContainer3, g_psPanels + 4, 0, 0, &g_sKentec320x240x16_SSD2119,
          205, 47, 105, 118, CTR_STYLE_OUTLINE | CTR_STYLE_FILL,
          ClrMidnightBlue, ClrGray, 0, 0, 0);
Container(g_sContainer2, g_psPanels + 4, &g_sContainer3, 0,
          &g_sKentec320x240x16_SSD2119, X_OFFSET, 100, 190, 70,
          (CTR_STYLE_OUTLINE | CTR_STYLE_FILL | CTR_STYLE_TEXT |
           CTR_STYLE_TEXT_CENTER), ClrMidnightBlue, ClrGray, ClrSilver,
          g_psFontCm22, "Group2");
Container(g_sContainer1, g_psPanels + 4, &g_sContainer2, 0,
          &g_sKentec320x240x16_SSD2119, X_OFFSET, Y_OFFSET, 190, 70,
          CTR_STYLE_OUTLINE | CTR_STYLE_FILL | CTR_STYLE_TEXT, ClrMidnightBlue,
          ClrGray, ClrSilver, g_psFontCm22, "Group1");

//*****************************************************************************
//
// The sixth panel, which contains a selection of push buttons.
//
//*****************************************************************************
tCanvasWidget g_psPushButtonIndicators[] =
{
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 1, 0,
                 &g_sKentec320x240x16_SSD2119, 40, 80, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 2, 0,
                 &g_sKentec320x240x16_SSD2119, 90, 80, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 3, 0,
                 &g_sKentec320x240x16_SSD2119, 145, 80, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 4, 0,
                 &g_sKentec320x240x16_SSD2119, 40, 160, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 5, 0,
                 &g_sKentec320x240x16_SSD2119, 90, 160, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 6, 0,
                 &g_sKentec320x240x16_SSD2119, 145, 160, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 7, 0,
                 &g_sKentec320x240x16_SSD2119, 190, 30, 110, 24,
                 CANVAS_STYLE_TEXT, 0, 0, ClrSilver, g_psFontCm20, "Non-auto",
                 0, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 8, 0,
                 &g_sKentec320x240x16_SSD2119, 190, 50, 110, 24,
                 CANVAS_STYLE_TEXT, 0, 0, ClrSilver, g_psFontCm20, "repeat",
                 0, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 9, 0,
                 &g_sKentec320x240x16_SSD2119, 190, 110, 110, 24,
                 CANVAS_STYLE_TEXT, 0, 0, ClrSilver, g_psFontCm20, "Auto",
                 0, 0),
    CanvasStruct(g_psPanels + 5, 0, 0,
                 &g_sKentec320x240x16_SSD2119, 190, 130, 110, 24,
                 CANVAS_STYLE_TEXT, 0, 0, ClrSilver, g_psFontCm20, "repeat",
                 0, 0),
};
tPushButtonWidget g_psPushButtons[] =
{
    RectangularButtonStruct(g_psPanels + 5, g_psPushButtons + 1, 0,
                            &g_sKentec320x240x16_SSD2119, 30, 30, 40, 40,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
                            ClrMidnightBlue, ClrBlack, ClrGray, ClrSilver,
                            g_psFontCm22, "1", 0, 0, 0, 0, OnButtonPress),
    CircularButtonStruct(g_psPanels + 5, g_psPushButtons + 2, 0,
                         &g_sKentec320x240x16_SSD2119, 100, 50, 20,
                         PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
                         ClrMidnightBlue, ClrBlack, ClrGray, ClrSilver,
                         g_psFontCm22, "3", 0, 0, 0, 0, OnButtonPress),
    RectangularButtonStruct(g_psPanels + 5, g_psPushButtons + 3, 0,
                            &g_sKentec320x240x16_SSD2119, 130, 25, 50, 50,
                            PB_STYLE_IMG | PB_STYLE_TEXT, 0, 0, 0, ClrSilver,
                            g_psFontCm22, "5", g_pui8Blue50x50,
                            g_pui8Blue50x50Press, 0, 0, OnButtonPress),
    RectangularButtonStruct(g_psPanels + 5, g_psPushButtons + 4, 0,
                            &g_sKentec320x240x16_SSD2119, 30, 110, 40, 40,
                            (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT |
                             PB_STYLE_AUTO_REPEAT), ClrMidnightBlue, ClrBlack,
                            ClrGray, ClrSilver, g_psFontCm22, "2", 0, 0, 125,
                            25, OnButtonPress),
    CircularButtonStruct(g_psPanels + 5, g_psPushButtons + 5, 0,
                         &g_sKentec320x240x16_SSD2119, 100, 130, 20,
                         (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT |
                          PB_STYLE_AUTO_REPEAT), ClrMidnightBlue, ClrBlack,
                         ClrGray, ClrSilver, g_psFontCm22, "4", 0, 0, 125, 25,
                         OnButtonPress),
    RectangularButtonStruct(g_psPanels + 5, g_psPushButtonIndicators, 0,
                            &g_sKentec320x240x16_SSD2119, 130, 105, 50, 50,
                            (PB_STYLE_IMG | PB_STYLE_TEXT |
                             PB_STYLE_AUTO_REPEAT), 0, 0, 0, ClrSilver,
                            g_psFontCm22, "6", g_pui8Blue50x50,
                            g_pui8Blue50x50Press, 125, 25, OnButtonPress),
};
#define NUM_PUSH_BUTTONS        (sizeof(g_psPushButtons) /   \
                                 sizeof(g_psPushButtons[0]))
uint32_t g_ui32ButtonState;

//*****************************************************************************
//
// The seventh panel, which contains a selection of radio buttons.
//
//*****************************************************************************
tContainerWidget g_psRadioContainers[];
tCanvasWidget g_psRadioButtonIndicators[] =
{
    CanvasStruct(g_psRadioContainers, g_psRadioButtonIndicators + 1, 0,
                 &g_sKentec320x240x16_SSD2119, 95, 52, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(g_psRadioContainers, g_psRadioButtonIndicators + 2, 0,
                 &g_sKentec320x240x16_SSD2119, 95, 97, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(g_psRadioContainers, 0, 0,
                 &g_sKentec320x240x16_SSD2119, 95, 142, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(g_psRadioContainers + 1, g_psRadioButtonIndicators + 4, 0,
                 &g_sKentec320x240x16_SSD2119, 260, 52, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(g_psRadioContainers + 1, g_psRadioButtonIndicators + 5, 0,
                 &g_sKentec320x240x16_SSD2119, 260, 97, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(g_psRadioContainers + 1, 0, 0,
                 &g_sKentec320x240x16_SSD2119, 260, 142, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
};
tRadioButtonWidget g_psRadioButtons1[] =
{
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 1, 0,
                      &g_sKentec320x240x16_SSD2119, 10, 40, 80, 45,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, g_psFontCm20,
                      "One", 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 2, 0,
                      &g_sKentec320x240x16_SSD2119, 10, 85, 80, 45,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, g_psFontCm20,
                      "Two", 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtonIndicators, 0,
                      &g_sKentec320x240x16_SSD2119, 10, 130, 80, 45,
                      RB_STYLE_TEXT, 24, 0, ClrSilver, ClrSilver, g_psFontCm20,
                      "Three", 0, OnRadioChange)
};
#define NUM_RADIO1_BUTTONS      (sizeof(g_psRadioButtons1) /   \
                                 sizeof(g_psRadioButtons1[0]))
tRadioButtonWidget g_psRadioButtons2[] =
{
    RadioButtonStruct(g_psRadioContainers + 1, g_psRadioButtons2 + 1, 0,
                      &g_sKentec320x240x16_SSD2119, 175, 40, 80, 45,
                      RB_STYLE_IMG, 16, 0, ClrSilver, 0, 0, 0, g_ui8Logo,
                      OnRadioChange),
    RadioButtonStruct(g_psRadioContainers + 1, g_psRadioButtons2 + 2, 0,
                      &g_sKentec320x240x16_SSD2119, 175, 85, 80, 45,
                      RB_STYLE_IMG, 24, 0, ClrSilver, 0, 0, 0, g_ui8Logo,
                      OnRadioChange),
    RadioButtonStruct(g_psRadioContainers + 1, g_psRadioButtonIndicators + 3,
                      0, &g_sKentec320x240x16_SSD2119, 175, 130, 80, 45,
                      RB_STYLE_IMG, 24, 0, ClrSilver, 0, 0, 0, g_ui8Logo,
                      OnRadioChange)
};
#define NUM_RADIO2_BUTTONS      (sizeof(g_psRadioButtons2) /   \
                                 sizeof(g_psRadioButtons2[0]))
tContainerWidget g_psRadioContainers[] =
{
    ContainerStruct(g_psPanels + 6, g_psRadioContainers + 1, g_psRadioButtons1,
                    &g_sKentec320x240x16_SSD2119, 8, 24, 145, 154,
                    CTR_STYLE_OUTLINE | CTR_STYLE_TEXT, 0, ClrGray, ClrSilver,
                    g_psFontCm20, "Group One"),
    ContainerStruct(g_psPanels + 6, 0, g_psRadioButtons2,
                    &g_sKentec320x240x16_SSD2119, 167, 24, 145, 154,
                    CTR_STYLE_OUTLINE | CTR_STYLE_TEXT, 0, ClrGray, ClrSilver,
                    g_psFontCm20, "Group Two")
};

//*****************************************************************************
//
// The eighth panel, which demonstrates the slider widget.
//
//*****************************************************************************
Canvas(g_sSliderValueCanvas, g_psPanels + 7, 0, 0,
       &g_sKentec320x240x16_SSD2119, 210, 30, 60, 40,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, 0, ClrSilver,
       g_psFontCm24, "50%",
       0, 0);

tSliderWidget g_psSliders[] =
{
    SliderStruct(g_psPanels + 7, g_psSliders + 1, 0,
                 &g_sKentec320x240x16_SSD2119, 10, 105, 220, 30, 0, 100, 25,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                  SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                 ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                 g_psFontCm20, "25%", 0, 0, OnSliderChange),
    SliderStruct(g_psPanels + 7, g_psSliders + 2, 0,
                 &g_sKentec320x240x16_SSD2119, 10, 145, 220, 25, 0, 100, 25,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                  SL_STYLE_TEXT),
                 ClrWhite, ClrBlueViolet, ClrSilver, ClrBlack, 0,
                 g_psFontCm18, "Foreground Text Only", 0, 0, OnSliderChange),
    SliderStruct(g_psPanels + 7, g_psSliders + 3, 0,
                 &g_sKentec320x240x16_SSD2119, 240, 70, 26, 110, 0, 100, 50,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_VERTICAL |
                  SL_STYLE_OUTLINE | SL_STYLE_LOCKED), ClrDarkGreen,
                  ClrDarkRed, ClrSilver, 0, 0, 0, 0, 0, 0, 0),
    SliderStruct(g_psPanels + 7, g_psSliders + 4, 0,
                 &g_sKentec320x240x16_SSD2119, 275, 30, 30, 150, 0, 100, 75,
                 (SL_STYLE_IMG | SL_STYLE_BACKG_IMG | SL_STYLE_VERTICAL |
                 SL_STYLE_OUTLINE), 0, ClrBlack, ClrSilver, 0, 0, 0,
                 0, g_pui8GettingHotter28x148, g_pui8GettingHotter28x148Mono,
                 OnSliderChange),
    SliderStruct(g_psPanels + 7, g_psSliders + 5, 0,
                 &g_sKentec320x240x16_SSD2119, 10, 30, 195, 37, 0, 100, 50,
                 SL_STYLE_IMG | SL_STYLE_BACKG_IMG, 0, 0, 0, 0, 0, 0,
                 0, g_pui8GreenSlider195x37, g_pui8RedSlider195x37,
                 OnSliderChange),
    SliderStruct(g_psPanels + 7, &g_sSliderValueCanvas, 0,
                 &g_sKentec320x240x16_SSD2119, 10, 70, 220, 25, 0, 100, 50,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_TEXT |
                  SL_STYLE_BACKG_TEXT | SL_STYLE_TEXT_OPAQUE |
                  SL_STYLE_BACKG_TEXT_OPAQUE),
                 ClrBlue, ClrYellow, ClrSilver, ClrYellow, ClrBlue,
                 g_psFontCm18, "Text in both areas", 0, 0,
                 OnSliderChange),
};

#define SLIDER_TEXT_VAL_INDEX   0
#define SLIDER_LOCKED_INDEX     2
#define SLIDER_CANVAS_VAL_INDEX 4

#define NUM_SLIDERS (sizeof(g_psSliders) / sizeof(g_psSliders[0]))

//*****************************************************************************
//
// An array of canvas widgets, one per panel.  Each canvas is filled with
// black, overwriting the contents of the previous panel.
//
//*****************************************************************************
tCanvasWidget g_psPanels[] =
{
    CanvasStruct(0, 0, &g_sIntroduction, &g_sKentec320x240x16_SSD2119,
                 X_OFFSET, Y_OFFSET, (320 - (X_OFFSET*2)), 158,
                 CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &g_sPrimitives, &g_sKentec320x240x16_SSD2119,
                 X_OFFSET, Y_OFFSET, (320 - (X_OFFSET*2)), 158,
                 CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &g_sCanvas1, &g_sKentec320x240x16_SSD2119,
                 X_OFFSET, Y_OFFSET, (320 - (X_OFFSET*2)), 158,
                 CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, g_psCheckBoxes, &g_sKentec320x240x16_SSD2119,
                 X_OFFSET, Y_OFFSET, (320 - (X_OFFSET*2)), 158,
                 CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &g_sContainer1, &g_sKentec320x240x16_SSD2119,
                 X_OFFSET, Y_OFFSET, (320 - (X_OFFSET*2)), 158,
                 CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, g_psPushButtons, &g_sKentec320x240x16_SSD2119,
                 X_OFFSET, Y_OFFSET, (320 - (X_OFFSET*2)), 158,
                 CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, g_psRadioContainers, &g_sKentec320x240x16_SSD2119,
                 X_OFFSET, Y_OFFSET, (320 - (X_OFFSET*2)), 158,
                 CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, g_psSliders, &g_sKentec320x240x16_SSD2119,
                 X_OFFSET, Y_OFFSET, (320 - (X_OFFSET*2)), 158,
                 CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
};

//*****************************************************************************
//
// The number of panels.
//
//*****************************************************************************
#define NUM_PANELS              (sizeof(g_psPanels) / sizeof(g_psPanels[0]))

//*****************************************************************************
//
// The names for each of the panels, which is displayed at the bottom of the
// screen.
//
//*****************************************************************************
char *g_pcPanelNames[] =
{
    "     Introduction     ",
    "     Primitives     ",
    "     Canvas     ",
    "     Checkbox     ",
    "     Container     ",
    "     Push Buttons     ",
    "     Radio Buttons     ",
    "     Sliders     ",
    "     S/W Update    "
};

//*****************************************************************************
//
// The buttons and text across the bottom of the screen.
//
//*****************************************************************************
RectangularButton(g_sPrevious, 0, 0, 0, &g_sKentec320x240x16_SSD2119,
                  X_OFFSET, 182, 50, 50,
                  PB_STYLE_FILL, ClrBlack, ClrBlack, 0, ClrSilver,
                  g_psFontCm20, "-", g_pui8Blue50x50, g_pui8Blue50x50Press,
                  0, 0, OnPrevious);
Canvas(g_sTitle, 0, 0, 0, &g_sKentec320x240x16_SSD2119,
       (X_OFFSET+50), 182, 204, 50,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, 0, 0, ClrSilver,
       g_psFontCm20, 0, 0, 0);
RectangularButton(g_sNext, 0, 0, 0, &g_sKentec320x240x16_SSD2119,
                  (320-50-X_OFFSET), 182, 50, 50,
                  PB_STYLE_IMG | PB_STYLE_TEXT, ClrBlack, ClrBlack, 0,
                  ClrSilver, g_psFontCm20, "+", g_pui8Blue50x50,
                  g_pui8Blue50x50Press, 0, 0, OnNext);

//*****************************************************************************
//
// The panel that is currently being displayed.
//
//*****************************************************************************
uint32_t g_ui32Panel;

//*****************************************************************************
//
// The position within the waveform of the click sound.
//
//*****************************************************************************
uint32_t g_ui32AudioPos;

//*****************************************************************************
//
// The step rate of the waveform for the click sound.
//
//*****************************************************************************
uint32_t g_ui32AudioStep;

//*****************************************************************************
//
// The amplitude of the waveformfor the click sound.
//
//*****************************************************************************
uint32_t g_ui32Amp;

//*****************************************************************************
//
// The step rate of the amplitude for the click sound.
//
//*****************************************************************************
uint32_t g_ui32AmpStep;

//*****************************************************************************
//
// Initialize the variables for generating the click sound waveform
//
//*****************************************************************************
void
PlayClick(void)
{
    //
    // Start the new waveform at zero.
    //
    g_ui32AudioPos  = 0;

    //
    // Set the fixed Audio step
    //
    g_ui32AudioStep = ((265 * 65536) / 64000) * 65536;

    //
    // Set the amplitude of the waveform generator to full volume.
    //
    g_ui32Amp = 2048;

    //
    // Set the decreasing step of amplitude.
    //
    g_ui32AmpStep = 1;
}

//*****************************************************************************
//
// Handles presses of the previous panel button.
//
//*****************************************************************************
void
OnPrevious(tWidget *psWidget)
{
    //
    // There is nothing to be done if the first panel is already being
    // displayed.
    //
    if(g_ui32Panel == 0)
    {
        return;
    }

    //
    // Remove the current panel.
    //
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Decrement the panel index.
    //
    g_ui32Panel--;

    //
    // Add and draw the new panel.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Set the title of this panel.
    //
    CanvasTextSet(&g_sTitle, g_pcPanelNames[g_ui32Panel]);
    WidgetPaint((tWidget *)&g_sTitle);

    //
    // See if this is the first panel.
    //
    if(g_ui32Panel == 0)
    {
        //
        // Clear the previous button from the display since the first panel is
        // being displayed.
        //
        PushButtonImageOff(&g_sPrevious);
        PushButtonTextOff(&g_sPrevious);
        PushButtonFillOn(&g_sPrevious);
        WidgetPaint((tWidget *)&g_sPrevious);
    }

    //
    // See if the previous panel was the last panel.
    //
    if(g_ui32Panel == (NUM_PANELS - 2))
    {
        //
        // Display the next button.
        //
        PushButtonImageOn(&g_sNext);
        PushButtonTextOn(&g_sNext);
        PushButtonFillOff(&g_sNext);
        WidgetPaint((tWidget *)&g_sNext);
    }

    //
    // Play the key click sound.
    //
    PlayClick();
}

//*****************************************************************************
//
// Handles presses of the next panel button.
//
//*****************************************************************************
void
OnNext(tWidget *psWidget)
{
    //
    // There is nothing to be done if the last panel is already being
    // displayed.
    //
    if(g_ui32Panel == (NUM_PANELS - 1))
    {
        return;
    }

    //
    // Remove the current panel.
    //
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Increment the panel index.
    //
    g_ui32Panel++;

    //
    // Add and draw the new panel.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Set the title of this panel.
    //
    CanvasTextSet(&g_sTitle, g_pcPanelNames[g_ui32Panel]);
    WidgetPaint((tWidget *)&g_sTitle);

    //
    // See if the previous panel was the first panel.
    //
    if(g_ui32Panel == 1)
    {
        //
        // Display the previous button.
        //
        PushButtonImageOn(&g_sPrevious);
        PushButtonTextOn(&g_sPrevious);
        PushButtonFillOff(&g_sPrevious);
        WidgetPaint((tWidget *)&g_sPrevious);
    }

    //
    // See if this is the last panel.
    //
    if(g_ui32Panel == (NUM_PANELS - 1))
    {
        //
        // Clear the next button from the display since the last panel is being
        // displayed.
        //
        PushButtonImageOff(&g_sNext);
        PushButtonTextOff(&g_sNext);
        PushButtonFillOn(&g_sNext);
        WidgetPaint((tWidget *)&g_sNext);
    }

    //
    // Play the key click sound.
    //
    PlayClick();
}

//*****************************************************************************
//
// Handles paint requests for the introduction canvas widget.
//
//*****************************************************************************
void
OnIntroPaint(tWidget *psWidget, tContext *psContext)
{
    //
    // Display the introduction text in the canvas.
    //
    GrContextFontSet(psContext, g_psFontCm16);
    GrContextForegroundSet(psContext, ClrSilver);
    GrStringDraw(psContext, "This application demonstrates the ", -1,
                 10, 30, 0);
    GrStringDraw(psContext, "TivaWare Graphics Library.", -1,
                 10, (30+16), 0);
    GrStringDraw(psContext, "Each panel shows a different feature of", -1,
                 10, (30+(16*2)), 0);
    GrStringDraw(psContext, "the graphics library. Widgets on the panels", -1,
                 10, (30+(16*3)), 0);
    GrStringDraw(psContext, "are fully operational; pressing them will", -1,
                 10, (30+(16*4)), 0);
    GrStringDraw(psContext, "result in visible feedback of some kind.", -1,
                 10, (30+(16*5)), 0);
    GrStringDraw(psContext, "Press the + and - buttons at the bottom", -1,
                 10, (30+(16*6)), 0);
    GrStringDraw(psContext, "of the screen to move between the panels.", -1,
                 10, (30+(16*7)), 0);
}

//*****************************************************************************
//
// Handles paint requests for the primitives canvas widget.
//
//*****************************************************************************
void
OnPrimitivePaint(tWidget *psWidget, tContext *psContext)
{
    uint32_t ui32Idx;
    tRectangle sRect;

    //
    // Draw a vertical sweep of lines from red to green.
    //
    for(ui32Idx = 0; ui32Idx <= 8; ui32Idx++)
    {
        GrContextForegroundSet(psContext,
                               (((((10 - ui32Idx) * 255) / 10) << ClrRedShift) |
                                (((ui32Idx * 255) / 10) << ClrGreenShift)));
        GrLineDraw(psContext, 115, 120, 5, 120 - (11 * ui32Idx));
    }

    //
    // Draw a horizontal sweep of lines from green to blue.
    //
    for(ui32Idx = 1; ui32Idx <= 10; ui32Idx++)
    {
        GrContextForegroundSet(psContext,
                               (((((10 - ui32Idx) * 255) / 10) <<
                                 ClrGreenShift) |
                                (((ui32Idx * 255) / 10) << ClrBlueShift)));
        GrLineDraw(psContext, 115, 120, 5 + (ui32Idx * 11), 29);
    }

    //
    // Draw a filled circle with an overlapping circle.
    //
    GrContextForegroundSet(psContext, ClrBrown);
    GrCircleFill(psContext, 185, 69, 40);
    GrContextForegroundSet(psContext, ClrSkyBlue);
    GrCircleDraw(psContext, 205, 99, 30);

    //
    // Draw a filled rectangle with an overlapping rectangle.
    //
    GrContextForegroundSet(psContext, ClrSlateGray);
    sRect.i16XMin = 20;
    sRect.i16YMin = 100;
    sRect.i16XMax = 75;
    sRect.i16YMax = 160;
    GrRectFill(psContext, &sRect);
    GrContextForegroundSet(psContext, ClrSlateBlue);
    sRect.i16XMin += 40;
    sRect.i16YMin += 30;
    sRect.i16XMax += 30;
    sRect.i16YMax += 18;
    GrRectDraw(psContext, &sRect);

    //
    // Draw a piece of text in fonts of increasing size.
    //
    GrContextForegroundSet(psContext, ClrSilver);
    GrContextFontSet(psContext, g_psFontCm14);
    GrStringDraw(psContext, "Strings", -1, 120, 104, 0);
    GrContextFontSet(psContext, g_psFontCm18);
    GrStringDraw(psContext, "Strings", -1, 140, 118, 0);
    GrContextFontSet(psContext, g_psFontCm22);
    GrStringDraw(psContext, "Strings", -1, 160, 136, 0);
    GrContextFontSet(psContext, g_psFontCm24);
    GrStringDraw(psContext, "Strings", -1, 180, 158, 0);

    //
    // Draw an image.
    //
    GrImageDraw(psContext, g_ui8Logo, 262, 80);
}

//*****************************************************************************
//
// Handles paint requests for the canvas demonstration widget.
//
//*****************************************************************************
void
OnCanvasPaint(tWidget *psWidget, tContext *psContext)
{
    uint32_t ui32Idx;

    //
    // Draw a set of radiating lines.
    //
    GrContextForegroundSet(psContext, ClrGoldenrod);
    for(ui32Idx = 50; ui32Idx <= 180; ui32Idx += 10)
    {
        GrLineDraw(psContext, 210, ui32Idx, 310, 230 - ui32Idx);
    }

    //
    // Indicate that the contents of this canvas were drawn by the application.
    //
    GrContextFontSet(psContext, g_psFontCm12);
    GrStringDrawCentered(psContext, "App Drawn", -1, 260, 50, 1);
}

//*****************************************************************************
//
// Handles change notifications for the check box widgets.
//
//*****************************************************************************
void
OnCheckChange(tWidget *psWidget, uint32_t bSelected)
{
    uint32_t ui32Idx;

    //
    // Find the index of this check box.
    //
    for(ui32Idx = 0; ui32Idx < NUM_CHECK_BOXES; ui32Idx++)
    {
        if(psWidget == (tWidget *)(g_psCheckBoxes + ui32Idx))
        {
            break;
        }
    }

    //
    // Return if the check box could not be found.
    //
    if(ui32Idx == NUM_CHECK_BOXES)
    {
        return;
    }

    //
    // Set the matching indicator based on the selected state of the check box.
    //
    CanvasImageSet(g_psCheckBoxIndicators + ui32Idx,
                   bSelected ? g_pui8LightOn : g_pui8LightOff);
    WidgetPaint((tWidget *)(g_psCheckBoxIndicators + ui32Idx));

    //
    // Play the key click sound.
    //
    PlayClick();
}

//*****************************************************************************
//
// Handles press notifications for the push button widgets.
//
//*****************************************************************************
void
OnButtonPress(tWidget *psWidget)
{
    uint32_t ui32Idx;

    //
    // Find the index of this push button.
    //
    for(ui32Idx = 0; ui32Idx < NUM_PUSH_BUTTONS; ui32Idx++)
    {
        if(psWidget == (tWidget *)(g_psPushButtons + ui32Idx))
        {
            break;
        }
    }

    //
    // Return if the push button could not be found.
    //
    if(ui32Idx == NUM_PUSH_BUTTONS)
    {
        return;
    }

    //
    // Toggle the state of this push button indicator.
    //
    g_ui32ButtonState ^= 1 << ui32Idx;

    //
    // Set the matching indicator based on the selected state of the check box.
    //
    CanvasImageSet(g_psPushButtonIndicators + ui32Idx,
                   (g_ui32ButtonState & (1 << ui32Idx)) ? g_pui8LightOn :
                   g_pui8LightOff);
    WidgetPaint((tWidget *)(g_psPushButtonIndicators + ui32Idx));

    //
    // Play the key click sound.
    //
    PlayClick();
}

//*****************************************************************************
//
// Handles notifications from the slider controls.
//
//*****************************************************************************
void
OnSliderChange(tWidget *psWidget, int32_t i32Value)
{
    static char pcCanvasText[5];
    static char pcSliderText[5];

    //
    // Is this the widget whose value we mirror in the canvas widget and the
    // locked slider?
    //
    if(psWidget == (tWidget *)&g_psSliders[SLIDER_CANVAS_VAL_INDEX])
    {
        //
        // Yes - update the canvas to show the slider value.
        //
        usprintf(pcCanvasText, "%3d%%", i32Value);
        CanvasTextSet(&g_sSliderValueCanvas, pcCanvasText);
        WidgetPaint((tWidget *)&g_sSliderValueCanvas);

        //
        // Also update the value of the locked slider to reflect this one.
        //
        SliderValueSet(&g_psSliders[SLIDER_LOCKED_INDEX], i32Value);
        WidgetPaint((tWidget *)&g_psSliders[SLIDER_LOCKED_INDEX]);
    }

    if(psWidget == (tWidget *)&g_psSliders[SLIDER_TEXT_VAL_INDEX])
    {
        //
        // Yes - update the canvas to show the slider value.
        //
        usprintf(pcSliderText, "%3d%%", i32Value);
        SliderTextSet(&g_psSliders[SLIDER_TEXT_VAL_INDEX], pcSliderText);
        WidgetPaint((tWidget *)&g_psSliders[SLIDER_TEXT_VAL_INDEX]);
    }
}

//*****************************************************************************
//
// Handles change notifications for the radio button widgets.
//
//*****************************************************************************
void
OnRadioChange(tWidget *psWidget, uint32_t bSelected)
{
    uint32_t ui32Idx;

    //
    // Find the index of this radio button in the first group.
    //
    for(ui32Idx = 0; ui32Idx < NUM_RADIO1_BUTTONS; ui32Idx++)
    {
        if(psWidget == (tWidget *)(g_psRadioButtons1 + ui32Idx))
        {
            break;
        }
    }

    //
    // See if the radio button was found.
    //
    if(ui32Idx == NUM_RADIO1_BUTTONS)
    {
        //
        // Find the index of this radio button in the second group.
        //
        for(ui32Idx = 0; ui32Idx < NUM_RADIO2_BUTTONS; ui32Idx++)
        {
            if(psWidget == (tWidget *)(g_psRadioButtons2 + ui32Idx))
            {
                break;
            }
        }

        //
        // Return if the radio button could not be found.
        //
        if(ui32Idx == NUM_RADIO2_BUTTONS)
        {
            return;
        }

        //
        // Sind the radio button is in the second group, offset the index to
        // the indicators associated with the second group.
        //
        ui32Idx += NUM_RADIO1_BUTTONS;
    }

    //
    // Set the matching indicator based on the selected state of the radio
    // button.
    //
    CanvasImageSet(g_psRadioButtonIndicators + ui32Idx,
                   bSelected ? g_pui8LightOn : g_pui8LightOff);
    WidgetPaint((tWidget *)(g_psRadioButtonIndicators + ui32Idx));

    //
    // Play the key click sound.
    //
    PlayClick();
}

//*****************************************************************************
//
// The callback function that is called by the sound driver to indicate that
// half of the sound buffer has been played.
//
//*****************************************************************************
void
SoundCallback(uint32_t ui32Half)
{
    //
    // See which half of the sound buffer has been played.
    //
    if(ui32Half == 0)
    {
        //
        // The first half of the sound buffer needs to be filled.
        //
        HWREGBITW(&g_ui32Flags, FLAG_PING) = 1;
    }
    else
    {
        //
        // The second half of the sound buffer needs to be filled.
        //
        HWREGBITW(&g_ui32Flags, FLAG_PONG) = 1;
    }
}

//*****************************************************************************
//
// Generates an additional section of the audio output
//
//*****************************************************************************
void
GenerateAudio(int16_t *pi16Buffer, uint32_t ui32Count)
{
    int32_t i32Val;

    //
    // if g_ui32Amp is down to 0, fill the buffer with silence.
    //
    if(g_ui32Amp == 0)
    {
       //
       // Fill the buffer with silence.
       //
       while(ui32Count--)
       {
           *pi16Buffer++ = 0;
       }
       return;
    }

    //
    // Loop through the samples to be generated.
    //
    while(ui32Count--)
    {
        //
        // Compute the value of the waveform.
        //
        i32Val = sine(g_ui32AudioPos + (sine(g_ui32AudioPos * 3) * 10922));

        //
        // Increment the position of the waveform.
        //
        g_ui32AudioPos += g_ui32AudioStep;

        //
        // Scale the waveform value by the volume.
        //
        i32Val = (i32Val * g_ui32Amp) / 1024;

        //
        // Decrement the waveform amplitude by the step.
        //
        g_ui32Amp -= g_ui32AmpStep;

        //
        // Cilp the waveform to min/max if required.
        //
        i32Val /= 2;
        if(i32Val > 32767)
        {
            i32Val = 32767;
        }
        if(i32Val < -32768)
        {
            i32Val = -32768;
        }

        //
        // Add the new waveform value to the sample buffer.
        //
        *pi16Buffer++ = (int16_t)i32Val;
    }
}
//*****************************************************************************
//
// A simple demonstration of the features of the TivaWare Graphics Library.
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
    FrameDraw(&sContext, "grlib-demo");

    //
    // Configure and enable uDMA
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);
    ROM_uDMAControlBaseSet(&psDMAControlTable[0]);
    ROM_uDMAEnable();

    //
    // Initialize the sound driver.
    //
    SoundInit(ui32SysClock);
    SoundVolumeSet(128);
    SoundStart(g_pi16AudioBuffer, AUDIO_SIZE, 64000, SoundCallback);

    //
    // Initialize the touch screen driver and have it route its messages to the
    // widget tree.
    //
    TouchScreenInit(ui32SysClock);
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the title block and the previous and next buttons to the widget
    // tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sPrevious);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sTitle);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sNext);

    //
    // Add the first panel to the widget tree.
    //
    g_ui32Panel = 0;
    WidgetAdd(WIDGET_ROOT, (tWidget *)g_psPanels);
    CanvasTextSet(&g_sTitle, g_pcPanelNames[0]);

    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Loop forever handling widget messages.
    //
    while(1)
    {
        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();

        //
        // See if the first half of the sound buffer needs to be filled.
        //
        if(HWREGBITW(&g_ui32Flags, FLAG_PING) == 1)
        {
            //
            // generate new audio into the first half of the sound buffer.
            //
            GenerateAudio(g_pi16AudioBuffer, AUDIO_SIZE / 2);

            //
            // Clear the flag for the first half of the sound buffer.
            //
            HWREGBITW(&g_ui32Flags, FLAG_PING) = 0;
        }

        //
        // See if the second half of the sound buffer needs to be filled.
        //
        if(HWREGBITW(&g_ui32Flags, FLAG_PONG) == 1)
        {
            //
            // generate new audio into the second half of the sound buffer.
            //
            GenerateAudio(g_pi16AudioBuffer + (AUDIO_SIZE / 2),
                                       AUDIO_SIZE / 2);

            //
            // Clear the flag for the second half of the sound buffer.
            //
            HWREGBITW(&g_ui32Flags, FLAG_PONG) = 0;
        }
    }
}
