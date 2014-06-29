//*****************************************************************************
//
// menus.c - Application menus definition and supporting functions.
//
// Copyright (c) 2011-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C123G Firmware Package.
//
//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "utils/ustdlib.h"
#include "drivers/cfal96x64x16.h"
#include "drivers/slidemenuwidget.h"
#include "drivers/stripchartwidget.h"
#include "utils/ustdlib.h"
#include "stripchartmanager.h"
#include "clocksetwidget.h"
#include "qs-logger.h"
#include "acquire.h"
#include "menus.h"

//*****************************************************************************
//
// This is the menus module of the data logger example application.  It
// contains the definitions of all the menus that are used by the application
// and provides some helper functions.
//
//*****************************************************************************

//*****************************************************************************
//
// Define two offscreen buffers and display structures.  These are used for
// off-screen drawing for animation effects.  Only limited colors are needed
// so 4bpp buffers are used to save memory.
//
//*****************************************************************************
#define OFFSCREEN_BUF_SIZE      GrOffScreen4BPPSize(96, 64)
uint8_t g_pui8OffscreenBufA[OFFSCREEN_BUF_SIZE];
uint8_t g_pui8OffscreenBufB[OFFSCREEN_BUF_SIZE];
tDisplay g_sOffscreenDisplayA;
tDisplay g_sOffscreenDisplayB;

//*****************************************************************************
//
// Create a palette that is used by the on-screen menus and anything else that
// uses the (above) off-screen buffers.  This palette should contain any
// colors that are used by any widget using the offscreen buffers.  There can
// be up to 16 colors in this palette.
// The numerical values of colors in the list below were selected as colors
// that produced good results on the display and did not already have named
// values.
//
//*****************************************************************************
uint32_t g_pui32Palette[] =
{
    ClrBlack,
    ClrWhite,
    ClrDarkBlue,
    ClrLightBlue,
    ClrRed,
    ClrDarkGreen,
    ClrYellow,
    ClrBlue,
    ClrGreen,
    0x000040,
    ClrLime,
    ClrAqua,
    0x004000,
    ClrFuchsia,
    0xC00040,
    0x60E080,
};
#define NUM_PALETTE_ENTRIES     (sizeof(g_pui32Palette) / sizeof(uint32_t))

//*****************************************************************************
//
// Define the length of text strings used for holding data values.
//
//*****************************************************************************
#define TEXT_FIELD_LENGTH       20

//*****************************************************************************
//
// Defines a set of text fields that are used by various widgets for dynamic
// updating of text.
//
//*****************************************************************************
static char g_pcTextFields[NUM_TEXT_ITEMS][TEXT_FIELD_LENGTH];

//*****************************************************************************
//
// Defines a set of canvas widgets that are used for showing the temperature
// on a simple screen with an outline and a title.
//
//*****************************************************************************
tCanvasWidget g_sTempContainerCanvas;
Canvas(g_sTempExtValueCanvas, &g_sTempContainerCanvas, 0, 0,
       &g_sCFAL96x64x16, 0, 44, 96, 20, (CANVAS_STYLE_OUTLINE |
                                         CANVAS_STYLE_TEXT |
                                         CANVAS_STYLE_TEXT_OPAQUE |
                                         CANVAS_STYLE_TEXT_HCENTER |
                                         CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_EXTTEMP], 0, 0);
Canvas(g_sTempIntValueCanvas, &g_sTempContainerCanvas, &g_sTempExtValueCanvas,
       0, &g_sCFAL96x64x16, 0, 24, 96, 20, (CANVAS_STYLE_OUTLINE |
                                            CANVAS_STYLE_TEXT |
                                            CANVAS_STYLE_TEXT_OPAQUE |
                                            CANVAS_STYLE_TEXT_HCENTER |
                                            CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_INTTEMP], 0, 0);
Canvas(g_sTempTitleCanvas, &g_sTempContainerCanvas, &g_sTempIntValueCanvas, 0,
       &g_sCFAL96x64x16, 0, 0, 96, 24, (CANVAS_STYLE_OUTLINE |
                                        CANVAS_STYLE_TEXT |
                                        CANVAS_STYLE_TEXT_OPAQUE |
                                        CANVAS_STYLE_TEXT_HCENTER |
                                        CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8, "TEMPERATURE", 0, 0);
Canvas(g_sTempContainerCanvas, 0, 0, &g_sTempTitleCanvas, &g_sCFAL96x64x16, 0,
       0, 96, 64, 0, ClrDarkBlue, ClrWhite, ClrWhite, 0, 0, 0, 0);

//*****************************************************************************
//
// Defines a set of canvas widgets that are used for showing the accelerometer
// data on a simple screen with an outline and a title.
//
//*****************************************************************************
tCanvasWidget g_sAccelContainerCanvas;
Canvas(g_sAccelZCanvas, &g_sAccelContainerCanvas, 0, 0, &g_sCFAL96x64x16, 0,
       48, 96, 16, (CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE |
                    CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_ACCELZ], 0, 0);
Canvas(g_sAccelYCanvas, &g_sAccelContainerCanvas, &g_sAccelZCanvas, 0,
       &g_sCFAL96x64x16, 0, 32, 96, 16, (CANVAS_STYLE_TEXT |
                                         CANVAS_STYLE_TEXT_OPAQUE |
                                         CANVAS_STYLE_TEXT_HCENTER |
                                         CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_ACCELY], 0, 0);
Canvas(g_sAccelXCanvas, &g_sAccelContainerCanvas, &g_sAccelYCanvas, 0,
       &g_sCFAL96x64x16, 0, 16, 96, 16, (CANVAS_STYLE_TEXT |
                                         CANVAS_STYLE_TEXT_OPAQUE |
                                         CANVAS_STYLE_TEXT_HCENTER |
                                         CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_ACCELX], 0, 0);
Canvas(g_sAccelTitleCanvas, &g_sAccelContainerCanvas, &g_sAccelXCanvas, 0,
       &g_sCFAL96x64x16, 0, 0, 96, 16, (CANVAS_STYLE_OUTLINE |
                                        CANVAS_STYLE_TEXT |
                                        CANVAS_STYLE_TEXT_OPAQUE |
                                        CANVAS_STYLE_TEXT_HCENTER |
                                        CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8, "ACCEL", 0, 0);
Canvas(g_sAccelContainerCanvas, 0, 0, &g_sAccelTitleCanvas, &g_sCFAL96x64x16,
       0, 0, 96, 64, 0, ClrDarkBlue, ClrWhite, ClrWhite, 0, 0, 0, 0);

//*****************************************************************************
//
// Defines a set of canvas widgets that are used for showing the gyro
// data on a simple screen with an outline and a title.
//
//*****************************************************************************
tCanvasWidget g_sGyroContainerCanvas;
Canvas(g_sGyroZCanvas, &g_sGyroContainerCanvas, 0, 0, &g_sCFAL96x64x16, 0,
       48, 96, 16, (CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE |
                    CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_GYROZ], 0, 0);
Canvas(g_sGyroYCanvas, &g_sGyroContainerCanvas, &g_sGyroZCanvas, 0,
       &g_sCFAL96x64x16, 0, 32, 96, 16, (CANVAS_STYLE_TEXT |
                                         CANVAS_STYLE_TEXT_OPAQUE |
                                         CANVAS_STYLE_TEXT_HCENTER |
                                         CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_GYROY], 0, 0);
Canvas(g_sGyroXCanvas, &g_sGyroContainerCanvas, &g_sGyroYCanvas, 0,
       &g_sCFAL96x64x16, 0, 16, 96, 16, (CANVAS_STYLE_TEXT |
                                         CANVAS_STYLE_TEXT_OPAQUE |
                                         CANVAS_STYLE_TEXT_HCENTER |
                                         CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_GYROX], 0, 0);
Canvas(g_sGyroTitleCanvas, &g_sGyroContainerCanvas, &g_sGyroXCanvas, 0,
       &g_sCFAL96x64x16, 0, 0, 96, 16, (CANVAS_STYLE_OUTLINE |
                                        CANVAS_STYLE_TEXT |
                                        CANVAS_STYLE_TEXT_OPAQUE |
                                        CANVAS_STYLE_TEXT_HCENTER |
                                        CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8, "Gyro", 0, 0);
Canvas(g_sGyroContainerCanvas, 0, 0, &g_sGyroTitleCanvas, &g_sCFAL96x64x16,
       0, 0, 96, 64, 0, ClrDarkBlue, ClrWhite, ClrWhite, 0, 0, 0, 0);

//*****************************************************************************
//
// Defines a set of canvas widgets that are used for showing the mag/compass
// data on a simple screen with an outline and a title.
//
//*****************************************************************************
tCanvasWidget g_sMagContainerCanvas;
Canvas(g_sMagZCanvas, &g_sMagContainerCanvas, 0, 0, &g_sCFAL96x64x16, 0,
       48, 96, 16, (CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE |
                    CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_COMPASSZ], 0, 0);
Canvas(g_sMagYCanvas, &g_sMagContainerCanvas, &g_sMagZCanvas, 0,
       &g_sCFAL96x64x16, 0, 32, 96, 16, (CANVAS_STYLE_TEXT |
                                         CANVAS_STYLE_TEXT_OPAQUE |
                                         CANVAS_STYLE_TEXT_HCENTER |
                                         CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_COMPASSY], 0, 0);
Canvas(g_sMagXCanvas, &g_sMagContainerCanvas, &g_sMagYCanvas, 0,
       &g_sCFAL96x64x16, 0, 16, 96, 16, (CANVAS_STYLE_TEXT |
                                         CANVAS_STYLE_TEXT_OPAQUE |
                                         CANVAS_STYLE_TEXT_HCENTER |
                                         CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_COMPASSX], 0, 0);
Canvas(g_sMagTitleCanvas, &g_sMagContainerCanvas, &g_sMagXCanvas, 0,
       &g_sCFAL96x64x16, 0, 0, 96, 16, (CANVAS_STYLE_OUTLINE |
                                        CANVAS_STYLE_TEXT |
                                        CANVAS_STYLE_TEXT_OPAQUE |
                                        CANVAS_STYLE_TEXT_HCENTER |
                                        CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8, "Mag", 0, 0);
Canvas(g_sMagContainerCanvas, 0, 0, &g_sMagTitleCanvas, &g_sCFAL96x64x16,
       0, 0, 96, 64, 0, ClrDarkBlue, ClrWhite, ClrWhite, 0, 0, 0, 0);



//*****************************************************************************
//
// Defines a set of canvas widgets that are used for showing the analog input
// data on a simple screen with no decorations.
//
//*****************************************************************************
tCanvasWidget g_sAINContainerCanvas;
Canvas(g_sAIN3Canvas, &g_sAINContainerCanvas, 0, 0, &g_sCFAL96x64x16, 0, 48,
       96, 16, (CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE |
                CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkGreen, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_USER3], 0, 0);
Canvas(g_sAIN2Canvas, &g_sAINContainerCanvas, &g_sAIN3Canvas, 0,
       &g_sCFAL96x64x16, 0, 32, 96, 16, (CANVAS_STYLE_TEXT |
                                         CANVAS_STYLE_TEXT_OPAQUE |
                                         CANVAS_STYLE_TEXT_HCENTER |
                                         CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkGreen, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_USER2], 0, 0);
Canvas(g_sAIN1Canvas, &g_sAINContainerCanvas, &g_sAIN2Canvas, 0,
       &g_sCFAL96x64x16, 0, 16, 96, 16, (CANVAS_STYLE_TEXT |
                                         CANVAS_STYLE_TEXT_OPAQUE |
                                         CANVAS_STYLE_TEXT_HCENTER |
                                         CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkGreen, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_USER1], 0, 0);
Canvas(g_sAIN0Canvas, &g_sAINContainerCanvas, &g_sAIN1Canvas, 0,
       &g_sCFAL96x64x16, 0, 0, 96, 16, (CANVAS_STYLE_TEXT |
                                        CANVAS_STYLE_TEXT_OPAQUE |
                                        CANVAS_STYLE_TEXT_HCENTER |
                                        CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkGreen, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_USER0], 0, 0);
Canvas(g_sAINContainerCanvas, 0, 0, &g_sAIN0Canvas, &g_sCFAL96x64x16, 0, 0, 96,
       64, 0, ClrDarkGreen, ClrWhite, ClrWhite, 0, 0, 0, 0);

//*****************************************************************************
//
// Defines a set of canvas widgets that are used for showing the current
// on a simple screen with an outline and a title.
//
//*****************************************************************************
tCanvasWidget g_sCurrentContainerCanvas;
Canvas(g_sCurrentValueCanvas, &g_sCurrentContainerCanvas, 0, 0,
       &g_sCFAL96x64x16, 0, 24, 96, 40, (CANVAS_STYLE_TEXT |
                                         CANVAS_STYLE_TEXT_OPAQUE |
                                         CANVAS_STYLE_TEXT_HCENTER |
                                         CANVAS_STYLE_TEXT_VCENTER),
       ClrBlack, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[LOG_ITEM_CURRENT], 0, 0);
Canvas(g_sCurrentTitleCanvas, &g_sCurrentContainerCanvas,
       &g_sCurrentValueCanvas, 0, &g_sCFAL96x64x16, 0, 0, 96, 24,
       (CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE |
        CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_TEXT_VCENTER),
       ClrBlack, ClrWhite, ClrWhite, &g_sFontFixed6x8, "CURRENT", 0, 0);
Canvas(g_sCurrentContainerCanvas, 0, 0, &g_sCurrentTitleCanvas,
       &g_sCFAL96x64x16, 0, 0, 96, 64, 0, ClrBlack, ClrWhite, ClrWhite, 0, 0,
       0, 0);

//*****************************************************************************
//
// Defines a set of canvas widgets that are used for showing the clock date
// and time.
//
//*****************************************************************************
tCanvasWidget g_sClockContainerCanvas;
Canvas(g_sClockTimeCanvas, &g_sClockContainerCanvas, 0, 0, &g_sCFAL96x64x16, 0,
       38, 96, 16, (CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE |
                    CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[TEXT_ITEM_TIME], 0, 0);
Canvas(g_sClockDateCanvas, &g_sClockContainerCanvas, &g_sClockTimeCanvas,
       0, &g_sCFAL96x64x16, 0, 22, 96, 16, (CANVAS_STYLE_TEXT |
                                            CANVAS_STYLE_TEXT_OPAQUE |
                                            CANVAS_STYLE_TEXT_HCENTER |
                                            CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[TEXT_ITEM_DATE], 0, 0);
Canvas(g_sClockTitleCanvas, &g_sClockContainerCanvas, &g_sClockDateCanvas, 0,
       &g_sCFAL96x64x16, 0, 0, 96, 16, (CANVAS_STYLE_OUTLINE |
                                        CANVAS_STYLE_TEXT |
                                        CANVAS_STYLE_TEXT_OPAQUE |
                                        CANVAS_STYLE_TEXT_HCENTER |
                                        CANVAS_STYLE_TEXT_VCENTER),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontFixed6x8, "CLOCK", 0, 0);
Canvas(g_sClockContainerCanvas, 0, 0, &g_sClockTitleCanvas, &g_sCFAL96x64x16,
       0, 0, 96, 64, CANVAS_STYLE_OUTLINE, ClrDarkBlue, ClrWhite, ClrWhite, 0,
       0, 0, 0);

//*****************************************************************************
//
// Defines a set of canvas widgets that are used for showing a status
// screen.  It is a simple canvas container that can show a title and 3
// lines of text.
//
//*****************************************************************************
tCanvasWidget g_sStatusContainerCanvas;
Canvas(g_sStatus3Canvas, &g_sStatusContainerCanvas, 0, 0, &g_sCFAL96x64x16, 1,
       48, 94, 12, (CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE |
                    CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT_HCENTER |
                    CANVAS_STYLE_TEXT_VCENTER),
       ClrRed, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[TEXT_ITEM_STATUS3], 0, 0);
Canvas(g_sStatus2Canvas, &g_sStatusContainerCanvas, &g_sStatus3Canvas, 0,
       &g_sCFAL96x64x16, 1, 30, 94, 12, (CANVAS_STYLE_TEXT |
                                         CANVAS_STYLE_TEXT_OPAQUE |
                                         CANVAS_STYLE_FILL |
                                         CANVAS_STYLE_TEXT_HCENTER |
                                         CANVAS_STYLE_TEXT_VCENTER),
       ClrRed, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[TEXT_ITEM_STATUS2], 0, 0);
Canvas(g_sStatus1Canvas, &g_sStatusContainerCanvas, &g_sStatus2Canvas, 0,
       &g_sCFAL96x64x16, 1, 18, 94, 12, (CANVAS_STYLE_TEXT |
                                         CANVAS_STYLE_TEXT_OPAQUE |
                                         CANVAS_STYLE_FILL |
                                         CANVAS_STYLE_TEXT_HCENTER |
                                         CANVAS_STYLE_TEXT_VCENTER),
       ClrRed, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTextFields[TEXT_ITEM_STATUS1], 0, 0);
Canvas(g_sStatusTitleCanvas, &g_sStatusContainerCanvas, &g_sStatus1Canvas, 0,
       &g_sCFAL96x64x16, 0, 0, 96, 16, (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT |
                                        CANVAS_STYLE_TEXT_OPAQUE |
                                        CANVAS_STYLE_TEXT_HCENTER |
                                        CANVAS_STYLE_TEXT_VCENTER),
       ClrWhite, ClrWhite, ClrBlack, &g_sFontFixed6x8,
       g_pcTextFields[TEXT_ITEM_STATUS_TITLE], 0, 0);
Canvas(g_sStatusContainerCanvas, 0, 0, &g_sStatusTitleCanvas, &g_sCFAL96x64x16,
       0, 0, 96, 64, CANVAS_STYLE_OUTLINE, ClrRed, ClrWhite, ClrWhite, 0, 0, 0,
       0);

//*****************************************************************************
//
// Allocate space for a time structure that is used with the clock setting
// widget.
//
//*****************************************************************************
struct tm g_sTimeClock;

//*****************************************************************************
//
// Define a clock setting widget.  It is used to allow the user to set the
// RTC (real time clock).
//
//*****************************************************************************
ClockSet(g_sClockSetter, 0, 0, 0, &g_sCFAL96x64x16, 0, 0, 96, 64,
         g_psFontFixed6x8, ClrWhite, ClrDarkGreen, &g_sTimeClock, 0);

//*****************************************************************************
//
// Define the main (root) menu.
//
//*****************************************************************************
tSlideMenu g_sConfigMenu;
tSlideMenu g_sViewMenu;
tSlideMenu g_sConfirmMenu;
tSlideMenuItem g_psMainMenuItems[] =
{
    {"CONFIG", &g_sConfigMenu, 0, 0},
    {"START", 0, &g_sStripChart.sBase, ClrBlack},
    {"VIEW", &g_sViewMenu, 0, 0},
    {"SAVE", 0, &g_sStatusContainerCanvas.sBase, ClrRed},
    {"ERASE", &g_sConfirmMenu, 0, 0}
};
tSlideMenu g_psMainMenu =
{
    0, (sizeof(g_psMainMenuItems) / sizeof(tSlideMenuItem)), g_psMainMenuItems,
    0, 0, 0, 0
};

//*****************************************************************************
//
// Define the configuration (CONFIG) sub-menu.
//
//  (root)-->CONFIG
//
//*****************************************************************************
tSlideMenu g_sChannelsMenu;
tSlideMenu g_sPeriodMenu;
tSlideMenu g_sStorageMenu;
tSlideMenu g_sSleepMenu;
tSlideMenuItem g_psConfigMenuItems[] =
{
    {"CHANNELS", &g_sChannelsMenu, 0, 0}, {"PERIOD", &g_sPeriodMenu, 0, 0},
    {"STORAGE", &g_sStorageMenu, 0, 0}, {"SLEEP", &g_sSleepMenu, 0, 0},
    {"CLOCK", 0, &g_sClockSetter.sBase, ClrDarkGreen}
};
tSlideMenu g_sConfigMenu =
{
    &g_psMainMenu, (sizeof(g_psConfigMenuItems) / sizeof(tSlideMenuItem)),
    g_psConfigMenuItems, 0, 0, 0, 0
};

//*****************************************************************************
//
// Define the channel selection sub-menu.
//
//  (root)-->CONFIG-->CHANNELS
//
//*****************************************************************************
tSlideMenuItem g_psChannelsMenuItems[] =
{
    {"CHAN 0", 0, 0, 0}, {"CHAN 1", 0, 0, 0}, {"CHAN 2", 0, 0, 0},
    {"CHAN 3", 0, 0, 0}, {"ACCEL X", 0, 0, 0}, {"ACCEL Y", 0, 0, 0},
    {"ACCEL Z", 0, 0, 0},{"EXT TEMP", 0, 0, 0}, {"INT TEMP", 0, 0, 0},
    {"CURRENT", 0, 0, 0}, {"GYRO X", 0, 0, 0}, {"GYRO Y", 0, 0, 0},
    {"GYRO Z", 0, 0, 0}, {"MAG X", 0, 0, 0}, {"MAG Y", 0, 0, 0},
    {"MAG Z", 0, 0, 0}, 
};
tSlideMenu g_sChannelsMenu =
{
    &g_sConfigMenu, (sizeof(g_psChannelsMenuItems) / sizeof(tSlideMenuItem)),
    g_psChannelsMenuItems, 0, 0, 1, 0
};

//*****************************************************************************
//
// Define the logging period sub-menu.
//
//  (root)-->CONFIG-->PERIOD
//
//*****************************************************************************
#define MENU_CONFIG_PERIOD_DEFAULT 3
tSlideMenuItem g_psPeriodMenuItems[] =
{
    {"1/32 sec", 0, 0, 0}, {"1/16 sec", 0, 0, 0}, {"1/8 sec", 0, 0, 0},
    {"1/4 sec", 0, 0, 0}, {"1/2 sec", 0, 0, 0}, {"1 sec", 0, 0, 0},
    {"5 sec", 0, 0, 0}, {"10 sec", 0, 0, 0}, {"1 min", 0, 0, 0},
    {"5 min", 0, 0, 0}, {"10 min", 0, 0, 0}, {"1 hour", 0, 0, 0},
    {"5 hour", 0, 0, 0}, {"10 hour", 0, 0, 0}, {"1 day", 0, 0, 0},
};
tSlideMenu g_sPeriodMenu =
{
    &g_sConfigMenu, (sizeof(g_psPeriodMenuItems) / sizeof(tSlideMenuItem)),
    g_psPeriodMenuItems, 0, 0, 0, 0
};

//*****************************************************************************
//
// Maps items from the period menu to match values for RTC.  Lower 8 bits
// is subsecond value, upper 24 bits is seconds.  The lower 8 bits represent
// subseconds in 7 bits, right-justififed.  So 0x01 is 1/128 of a second.
// The order of the values here needs to match what appears in the PERIOD
// menu in menus.c.
//
//*****************************************************************************
static uint32_t g_pui32LogPeriod[] =
{
    0x00000004, // 1/32
    0x00000008, // 1/16
    0x00000010, // 1/8
    0x00000020, // 1/4
    0x00000040, // 1/2
    0x00000100, // 1s
    0x00000500, // 5s
    0x00000A00, // 10s
    0x00003C00, // 1m - 60s
    0x00012C00, // 5m - 300s
    0x00025800, // 10m - 600s
    0x000E1000, // 1hr - 3600s
    0x00465000, // 5hr - 18000s
    0x008CA000, // 10hr - 36000s
    0x01518000, // 1d - 86400s
};

//*****************************************************************************
//
// Define the storage options sub-menu.
//
//  (root)-->CONFIG-->STORAGE
//
//*****************************************************************************
tSlideMenuItem g_psStorageMenuItems[] =
{
    {"NONE", 0, 0, 0}, {"USB", 0, 0, 0}, {"HOST PC", 0, 0, 0},
    {"FLASH", 0, 0, 0},
};
tSlideMenu g_sStorageMenu =
{
    &g_sConfigMenu, (sizeof(g_psStorageMenuItems) / sizeof(tSlideMenuItem)),
    g_psStorageMenuItems, 0, 0, 0, 0
};

//*****************************************************************************
//
// Define the sleep option sub-menu.
//
//  (root)-->CONFIG-->SLEEP
//
//*****************************************************************************
tSlideMenuItem g_psSleepMenuItems[] =
{
    {"NO", 0, 0, 0}, {"YES", 0, 0, 0},
};

tSlideMenu g_sSleepMenu =
{
    &g_sConfigMenu, (sizeof(g_psSleepMenuItems) / sizeof(tSlideMenuItem)),
    g_psSleepMenuItems, 0, 0, 0, 0
};

//*****************************************************************************
//
// Define the view options sub-menu.
//
//  (root)-->VIEW
//
//*****************************************************************************
tSlideMenuItem g_psViewMenuItems[] =
{
    {"AIN0-3", 0, (tWidget *)&g_sAINContainerCanvas, ClrDarkGreen},
    {"ACCEL", 0, (tWidget *)&g_sAccelContainerCanvas, ClrDarkBlue},
    {"TEMPERATURE", 0, (tWidget *)&g_sTempContainerCanvas, ClrDarkBlue},
    {"CURRENT", 0, (tWidget *)&g_sCurrentContainerCanvas, ClrBlack},
    {"GYRO", 0, (tWidget *)&g_sGyroContainerCanvas, ClrDarkBlue},
    {"MAG", 0, (tWidget *)&g_sMagContainerCanvas, ClrDarkBlue},
    {"CLOCK", 0, &g_sClockContainerCanvas.sBase, ClrDarkBlue},
    {"FLASH SPACE", 0, &g_sStatusContainerCanvas.sBase, ClrRed},
};
tSlideMenu g_sViewMenu =
{
    &g_psMainMenu, (sizeof(g_psViewMenuItems) / sizeof(tSlideMenuItem)),
    g_psViewMenuItems, 0, 0, 0, 0
};

//*****************************************************************************
//
// Define the ERASE confirmation menu.
//
//  (root)-->ERASE
//
//*****************************************************************************
tSlideMenuItem g_sConfirmMenuItems[] =
{
    {"ERASE DATA?", 0, &g_sStatusContainerCanvas.sBase, ClrRed},
};
tSlideMenu g_sConfirmMenu =
{
    &g_psMainMenu, (sizeof(g_sConfirmMenuItems) / sizeof(tSlideMenuItem)),
    g_sConfirmMenuItems, 0, 0, 0, 0
};

//*****************************************************************************
//
// Define the slide menu widget.  This is the wigdet that controls and
// displays all the above menus.
//
//*****************************************************************************
SlideMenu(g_sMenuWidget, WIDGET_ROOT, 0, 0, &g_sCFAL96x64x16, 0, 0, 96, 64,
          &g_sOffscreenDisplayA, &g_sOffscreenDisplayB, 16, ClrWhite, ClrRed,
          ClrBlack, &g_sFontFixed6x8, &g_psMainMenu, 0);

//*****************************************************************************
//
// This function allows dynamic text fields to be updated.
//
//*****************************************************************************
void
MenuUpdateText(uint32_t ui32TextID, const char *pcText)
{
    //
    // If the text field ID is valid, then update the string for that text
    // field.  The next time the associated widget is painted, the new text
    // will be shown.
    //
    if(ui32TextID < NUM_TEXT_ITEMS)
    {
        usnprintf(g_pcTextFields[ui32TextID], TEXT_FIELD_LENGTH, "%s", pcText);
    }
}

//*****************************************************************************
//
// Get the state defined by the configuration menu items.
// This function will query the configuration menus to determine the current
// choices for channels, period, etc.  These will be stored in the structure
// that was passed to the function.
//
//*****************************************************************************
void
MenuGetState(tConfigState *psState)
{
    int32_t ui32Temp;

    //
    // Check the arguments
    //
    ASSERT(psState);

    if(!psState)
    {
        return;
    }

    //
    // Read the various configuration menus and store their state
    //
    if(SlideMenuFocusItemGet(&g_sSleepMenu) > 0)
    {
        psState->bSleep = true;
    }
    else
    {
        psState->bSleep = false;
    }
    ui32Temp = SlideMenuFocusItemGet(&g_sStorageMenu);;
    psState->ui8Storage = (uint8_t)ui32Temp;
    ui32Temp = SlideMenuSelectedGet(&g_sChannelsMenu);
    psState->ui16SelectedMask = (uint16_t)ui32Temp;
    ui32Temp = g_pui32LogPeriod[SlideMenuFocusItemGet(&g_sPeriodMenu)];
    psState->ui32Period = ui32Temp;
}

//*****************************************************************************
//
// Set the configuration menu state.
// This function is used to set the state of the configuration menus.  This
// is used to "remember" a prior setting when the application is restarted
// or wakes from sleep.
//
//*****************************************************************************
void
MenuSetState(tConfigState *psState)
{
    uint32_t ui32Idx;

    //
    // Check the parameters
    //
    ASSERT(psState);
    ASSERT(psState->ui8Storage < CONFIG_STORAGE_CHOICES);
    ASSERT(psState->ui32Period & (psState->ui32Period < 0x2000000));

    //
    // Update each configuration menu item to match the new state
    //
    SlideMenuFocusItemSet(&g_sSleepMenu, psState->bSleep);
    SlideMenuFocusItemSet(&g_sStorageMenu, psState->ui8Storage);
    SlideMenuSelectedSet(&g_sChannelsMenu, psState->ui16SelectedMask);

    //
    // For the period, search the values table to find the index
    //
    for(ui32Idx = 0; ui32Idx < (sizeof(g_pui32LogPeriod) / sizeof(uint32_t));
        ui32Idx++)
    {
        if(psState->ui32Period == g_pui32LogPeriod[ui32Idx])
        {
            SlideMenuFocusItemSet(&g_sPeriodMenu, ui32Idx);

            //
            // Everything has been updated now so return to caller
            //
            return;
        }
    }

    //
    // If we get to this point, that means that there was no match for the
    // period.  In that case, set it to default value.
    //
    SlideMenuFocusItemSet(&g_sPeriodMenu, MENU_CONFIG_PERIOD_DEFAULT);
    psState->ui32Period = g_pui32LogPeriod[MENU_CONFIG_PERIOD_DEFAULT];
}

//*****************************************************************************
//
// Set the default values for the menu configuration.
// This function is used to initialize the application state configuration with
// some default values for the menu settings.
//
//*****************************************************************************
void
MenuGetDefaultState(tConfigState *psState)
{
    //
    // Check the parameters.
    //
    ASSERT(psState);

    //
    // Set default values for all the menu items
    //
    if(psState)
    {
        psState->bSleep = false;
        psState->ui8Storage = CONFIG_STORAGE_NONE;
        psState->ui16SelectedMask = 0;
        psState->ui32Period = 0x00000100;   // 1 second
    }
}

//*****************************************************************************
//
// Initialize the offscreen buffers and the menu structure.  This should be
// called before using the application menus.
// The parameter is a function pointer to a callback function whenever the
// menu widget activates or deactivates a child widget.
//
//*****************************************************************************
void
MenuInit(void (*pfnActive)(tWidget *, tSlideMenuItem *, bool))
{
    uint32_t ui32Idx;

    //
    // Initialize two offscreen displays and assign the palette.  These
    // buffers are used by the slide menu widget and other parts of the
    // application to allow for animation effects.
    //
    GrOffScreen4BPPInit(&g_sOffscreenDisplayA, g_pui8OffscreenBufA, 96, 64);
    GrOffScreen4BPPPaletteSet(&g_sOffscreenDisplayA, g_pui32Palette, 0,
                              NUM_PALETTE_ENTRIES);
    GrOffScreen4BPPInit(&g_sOffscreenDisplayB, g_pui8OffscreenBufB, 96, 64);
    GrOffScreen4BPPPaletteSet(&g_sOffscreenDisplayB, g_pui32Palette, 0,
                              NUM_PALETTE_ENTRIES);

    //
    // Initialize each of the text fields with a "blank" indication.
    //
    for(ui32Idx = 0; ui32Idx < NUM_LOG_ITEMS; ui32Idx++)
    {
        usnprintf(g_pcTextFields[ui32Idx], TEXT_FIELD_LENGTH, "----");
    }

    //
    // Set the slide menu widget callback function.
    //
    SlideMenuActiveCallbackSet(&g_sMenuWidget, pfnActive);
}
