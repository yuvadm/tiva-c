//*****************************************************************************
//
// tamper.c - tamper example.
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
#include <time.h>
#include <string.h>
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_hibernate.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/hibernate.h"
#include "driverlib/interrupt.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/listbox.h"
#include "grlib/pushbutton.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"
#include "utils/ustdlib.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Tamper (tamper)</h1>
//!
//! An example to demonstrate the use of tamper function in Hibernate module.
//! The user can ground any of these four GPIO pins(PM4, PM5, PM6, PM7 on
//! J28 and J30 headers on the development kit) to manually
//! triggle tamper events(s). The red indicators on the top of display should
//! reflect on which pin has triggered a tamper event.
//! The event along with the time stamp will be printed on the display.
//! The user can put the system in hibernation by pressing the HIB button.
//! The system should wake when the user either press RESET button,
//! or ground any of the four pins to trigger tamper event(s). When the
//! system boots up, the display should show whether the system woke
//! from hibernation or booted up from POR in which case a description of
//! howto instruction is printed on the display.
//! The RTC clock is displayed on the bottom of the display, the clock starts
//! from August 1st, 2013 at midnight when the app starts. The date and time
//! can be changed by pressing the CLOCK button. The clock is update every
//! second using hibernate calendar match interrupt. When the system
//! is in hibernation, the clock update on the display is paused, it resumes
//! once the system wakes up from hibernation.
//!
//! WARNING: XOSC failure is implemented in this example code, care must be
//! taken to ensure that the XOSCn pin(Y3) is properly grounded in order to
//! safely generate the external oscillator failure without damaging the
//! external oscillator. XOSCFAIL can be triggered as a tamper event,
//! as well as wakeup event from hibernation.
//
//*****************************************************************************

//*****************************************************************************
//
// Global variables used for Tamper events.
//
//*****************************************************************************
static volatile uint32_t g_ui32NMIEvent = 0;
static volatile uint32_t g_ui32TamperEventFlag = 0;
static volatile uint32_t g_ui32TamperRTCLog = 0;
static volatile uint32_t g_ui32TamperXOSCFailEvent = 0;
static uint32_t g_ui32RTCLog[4];
static uint32_t g_ui32EventLog[4];

//*****************************************************************************
//
// Flag to indicate that main screen is active so that main screen buffers can
// be updated and widgets redrawn.
//
//*****************************************************************************
static bool g_bMainScreen;

//*****************************************************************************
//
// Flag that informs that the user has requested hibernation.
//
//*****************************************************************************
static volatile bool g_bHibernate = false;

//*****************************************************************************
//
// Flag that indicates clock update on the display.
//
//*****************************************************************************
static bool g_bUpdateRTC = 0;

//*****************************************************************************
//
// Flag that informs that date and time have to be set.
//
//*****************************************************************************
volatile bool g_bSetDate;

//*****************************************************************************
//
// Buffers to store widget display information.
//
//*****************************************************************************
char g_pcMonBuf[4], g_pcDayBuf[3], g_pcYearBuf[5];
char g_pcHourBuf[3], g_pcMinBuf[3], g_pcAMPMBuf[3];

//*****************************************************************************
//
// Variables that keep track of the date and time across different screens.
//
//*****************************************************************************
uint32_t g_ui32MonthIdx, g_ui32DayIdx, g_ui32YearIdx;
uint32_t g_ui32HourIdx, g_ui32MinIdx;


//*****************************************************************************
//
// Lookup table to convert numerical value of a month into text.
//
//*****************************************************************************
static char *g_ppcMonth[12] =
{
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

//*****************************************************************************
//
// This is the image of a red LED that is turned off.
//
//*****************************************************************************
const uint8_t g_pui8LightOff[] =
{
    IMAGE_FMT_4BPP_COMP,
    20, 0,
    20, 0,

    15,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x09,
    0x00, 0x00, 0x19,
    0x00, 0x00, 0x2a,
    0x00, 0x00, 0x30,
    0x00, 0x00, 0x34,
    0x00, 0x00, 0x37,
    0x00, 0x00, 0x3a,
    0x00, 0x00, 0x3d,
    0x00, 0x00, 0x3f,
    0x00, 0x00, 0x40,
    0x00, 0x00, 0x42,
    0x00, 0x00, 0x45,
    0x00, 0x00, 0x4a,
    0x00, 0x00, 0x50,
    0x00, 0x00, 0x56,

    0x84, 0x01, 0x13, 0xaf, 0xe8, 0x31, 0x03, 0x1b, 0xff, 0x08, 0xff, 0xee,
    0xed, 0x71, 0x01, 0x02, 0xff, 0xff, 0x00, 0xfe, 0xed, 0xdd, 0xcc, 0x20,
    0x00, 0x00, 0x2f, 0x00, 0xff, 0xfe, 0xee, 0xdd, 0xcc, 0xcb, 0xa2, 0x00,
    0x40, 0x01, 0x21, 0xdd, 0xdc, 0xcc, 0xbb, 0xa8, 0x10, 0x00, 0x09, 0xff,
    0xfe, 0xdd, 0xdc, 0xcb, 0xbb, 0xba, 0x00, 0x98, 0x30, 0x1e, 0xfe, 0xed,
    0xdc, 0xcb, 0xaa, 0x00, 0xaa, 0xaa, 0x98, 0x61, 0x3e, 0xfe, 0xdd, 0xcc,
    0x00, 0xba, 0x99, 0x9a, 0xaa, 0x98, 0x62, 0x7d, 0xee, 0x00, 0xdc, 0xcb,
    0xa9, 0x99, 0x99, 0x99, 0x88, 0x73, 0x08, 0xcd, 0xed, 0xdc, 0xba, 0xb9,
    0x99, 0x88, 0x64, 0x20, 0xcc, 0xdd, 0x6b, 0x99, 0x87, 0x63, 0x6c, 0xcc,
    0x40, 0xba, 0x1a, 0x98, 0x76, 0x42, 0x2b, 0xba, 0xaa, 0x88, 0x1b, 0x65,
    0x42, 0x1a, 0x2a, 0x99, 0x98, 0x87, 0x00, 0x64, 0x30, 0x04, 0x78, 0x88,
    0x88, 0x89, 0x99, 0x00, 0x88, 0x76, 0x54, 0x20, 0x01, 0x67, 0x77, 0x88,
    0x00, 0x88, 0x88, 0x87, 0x65, 0x43, 0x00, 0x00, 0x25, 0x00, 0x67, 0x77,
    0x77, 0x77, 0x65, 0x44, 0x31, 0x00, 0x00, 0x00, 0x01, 0x45, 0x66, 0x66,
    0x65, 0x54, 0x43, 0x00, 0x10, 0x00, 0x00, 0x00, 0x13, 0x44, 0x45, 0x44,
    0x20, 0x33, 0x20, 0xb9, 0x00, 0x00, 0x12, 0x23, 0x32, 0x80, 0x72,
};

//*****************************************************************************
//
// This is the image of a red LED that is turned on.
//
//*****************************************************************************
const uint8_t g_pui8LightOn[] =
{
    IMAGE_FMT_4BPP_COMP,
    20, 0,
    20, 0,

    15,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x16,
    0x00, 0x00, 0x40,
    0x00, 0x00, 0x76,
    0x00, 0x00, 0x9b,
    0x00, 0x00, 0xac,
    0x00, 0x00, 0xb6,
    0x00, 0x00, 0xbc,
    0x00, 0x00, 0xbe,
    0x00, 0x00, 0xbf,
    0x00, 0x00, 0xc1,
    0x00, 0x00, 0xc5,
    0x00, 0x00, 0xcd,
    0x00, 0x00, 0xdb,
    0x00, 0x00, 0xef,
    0x00, 0x00, 0xfc,

    0x84, 0x01, 0x13, 0xaf, 0xe7, 0x31, 0x03, 0x1b, 0xff, 0x09, 0xff, 0xee,
    0xee, 0x51, 0x01, 0x02, 0xff, 0xb1, 0x02, 0xdd, 0xcc, 0x20, 0x00, 0x00,
    0x2f, 0xbb, 0xcc, 0x10, 0x92, 0x00, 0x01, 0x72, 0xdc, 0xcc, 0xcb, 0x96,
    0x00, 0x10, 0x07, 0xff, 0xfe, 0xed, 0xdc, 0xcc, 0xbb, 0x08, 0xba, 0x96,
    0x30, 0x1e, 0x22, 0xba, 0xaa, 0xaa, 0x10, 0x86, 0x51, 0x3e, 0x69, 0xba,
    0x99, 0x99, 0x99, 0x01, 0x76, 0x52, 0x5d, 0xee, 0xdd, 0xcb, 0xa9, 0xb2,
    0x20, 0x53, 0xcd, 0x6c, 0x98, 0x76, 0x54, 0xcc, 0xdd, 0x40, 0xcc, 0x6a,
    0x97, 0x66, 0x43, 0x5c, 0xcc, 0xba, 0x40, 0x98, 0x19, 0x87, 0x65, 0x43,
    0x3c, 0xcb, 0xa9, 0x20, 0x98, 0x89, 0x1a, 0x42, 0x1a, 0x88, 0x98, 0x77,
    0x40, 0x88, 0x69, 0x44, 0x31, 0x04, 0x67, 0x77, 0x77, 0x00, 0x78, 0x88,
    0x76, 0x55, 0x44, 0x20, 0x01, 0x55, 0x00, 0x66, 0x66, 0x77, 0x77, 0x65,
    0x54, 0x43, 0x10, 0x00, 0x00, 0x24, 0x55, 0x55, 0x66, 0x65, 0x54, 0x44,
    0x00, 0x31, 0x00, 0x00, 0x02, 0x44, 0x45, 0x55, 0x54, 0x00, 0x44, 0x33,
    0x10, 0x00, 0x00, 0x00, 0x13, 0x44, 0x08, 0x44, 0x44, 0x33, 0x21, 0xb9,
    0x00, 0x00, 0x12, 0x10, 0x33, 0x32, 0x20, 0xba,
};

//*****************************************************************************
//
// The screen offset of the upper left hand corner where we start to draw.
//
//*****************************************************************************
#define X_OFFSET                8
#define Y_OFFSET                24

//*****************************************************************************
//
// Defines for Hibernate memory value. It is used to determine if a wakeup is
// due to a tamper event.
//
//*****************************************************************************
#define HIBERNATE_TAMPER_DATA0  0xdeadbeef

//*****************************************************************************
//
// Storage for the strings which appear in the status box in the middle of the
// display.
//
//****************************************************************************
#define NUM_STATUS_STRINGS 10
#define MAX_STATUS_STRING_LEN (64 + 1)
char g_pcStatus[NUM_STATUS_STRINGS][MAX_STATUS_STRING_LEN];

//*****************************************************************************
//
// Storage for the status listbox widget string table.
//
//*****************************************************************************
const char *g_ppcStatusStrings[NUM_STATUS_STRINGS] =
{
    g_pcStatus[0],     g_pcStatus[1],     g_pcStatus[2],     g_pcStatus[3],
    g_pcStatus[4],     g_pcStatus[5],     g_pcStatus[6],     g_pcStatus[7],
    g_pcStatus[8],     g_pcStatus[9]
};
uint32_t g_ui32StatusStringIndex = 0;

//*****************************************************************************
//
// Forward reference to various widget structures in the main screen.
//
//*****************************************************************************
extern tCanvasWidget g_sMainScreen;
extern tCanvasWidget g_sIndicator0;
extern tCanvasWidget g_sIndicator1;
extern tCanvasWidget g_sIndicator2;
extern tCanvasWidget g_sIndicator3;
extern tCanvasWidget g_sIndicatorMarker;
extern tCanvasWidget g_sRTC;
extern tListBoxWidget g_sStatusList;
extern tPushButtonWidget g_sDateTimeSetBtn;
extern tPushButtonWidget g_sHIBBtn;

//*****************************************************************************
//
// Forward reference to various widget structures in the clock setting screen.
//
//*****************************************************************************
extern tCanvasWidget g_sDateScreen;
extern tCanvasWidget g_sMonText;
extern tPushButtonWidget g_sMonUpBtn, g_sMonDwnBtn;
extern tCanvasWidget g_sDayText;
extern tPushButtonWidget g_sDayUpBtn, g_sDayDwnBtn;
extern tCanvasWidget g_sYearText;
extern tPushButtonWidget g_sYearUpBtn, g_sYearDwnBtn;
extern tPushButtonWidget g_sDateNextBtn;
extern tCanvasWidget g_sTimeScreen;
extern tCanvasWidget g_sHourText;
extern tPushButtonWidget g_sHourUpBtn, g_sHourDwnBtn;
extern tCanvasWidget g_sMinText;
extern tPushButtonWidget g_sMinUpBtn, g_sMinDwnBtn;
extern tCanvasWidget g_sAMPMText;
extern tPushButtonWidget g_sAMPMUpBtn, g_sAMPMDwnBtn;
extern tPushButtonWidget g_sTimeDoneBtn;

//*****************************************************************************
//
// Forward reference of functions that are called on a button press.
//
//*****************************************************************************
void OnHIBBtnPress(tWidget *psWidget);
void OnDateTimeSetBtnPress(tWidget *psWidget);
void OnMonUpBtnPress(tWidget *psWidget);
void OnMonDwnBtnPress(tWidget *psWidget);
void OnDayUpBtnPress(tWidget *psWidget);
void OnDayDwnBtnPress(tWidget *psWidget);
void OnYearUpBtnPress(tWidget *psWidget);
void OnYearDwnBtnPress(tWidget *psWidget);
void OnDateNextBtnPress(tWidget *psWidget);
void OnHourUpBtnPress(tWidget *psWidget);
void OnHourDwnBtnPress(tWidget *psWidget);
void OnMinUpBtnPress(tWidget *psWidget);
void OnMinDwnBtnPress(tWidget *psWidget);
void OnAMPMBtnPress(tWidget *psWidget);
void OnTimeDoneBtnPress(tWidget *psWidget);

//*****************************************************************************
//
// The canvas widget acting as the background to the display.
//
//*****************************************************************************
Canvas(g_sMainScreen, WIDGET_ROOT, 0, &g_sIndicator0,
       &g_sKentec320x240x16_SSD2119, X_OFFSET, Y_OFFSET, (320 - X_OFFSET*2),
       240-X_OFFSET-Y_OFFSET,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);
//*****************************************************************************
//
// The four indicators on the top of the screen.
//
//*****************************************************************************
Canvas(g_sIndicator0, &g_sMainScreen, &g_sIndicator1, 0,
       &g_sKentec320x240x16_SSD2119, 20, 30, 50, 20,
       CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0);
Canvas(g_sIndicator1, &g_sMainScreen, &g_sIndicator2, 0,
       &g_sKentec320x240x16_SSD2119, 20 + 75*1, 30, 50, 20,
       CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0);
Canvas(g_sIndicator2, &g_sMainScreen, &g_sIndicator3, 0,
       &g_sKentec320x240x16_SSD2119, 20 + 75*2, 30, 50, 20,
       CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0);
Canvas(g_sIndicator3, &g_sMainScreen, 0, &g_sIndicatorMarker,
       &g_sKentec320x240x16_SSD2119, 20 + 75*3, 30, 50, 20,
       CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0);

//*****************************************************************************
//
// The canvas widget used to show the text under indicators
//
//*****************************************************************************
Canvas(g_sIndicatorMarker, &g_sIndicator3, 0, &g_sStatusList,
       &g_sKentec320x240x16_SSD2119, X_OFFSET, 60, 304, 16,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, 0, 0, ClrWhite,
       g_psFontCm16, "PM7        PM6       PM5        PM4", 0, 0);

//*****************************************************************************
//
// The listbox used to display tamper events.
//
//*****************************************************************************
ListBox(g_sStatusList, &g_sIndicatorMarker, 0, &g_sDateTimeSetBtn,
        &g_sKentec320x240x16_SSD2119,
        X_OFFSET, 90, (320-(X_OFFSET*2)), 90,
        (LISTBOX_STYLE_OUTLINE | LISTBOX_STYLE_LOCKED |
        LISTBOX_STYLE_WRAP), ClrBlack, ClrBlack, ClrSilver, ClrSilver, ClrWhite,
        g_psFontFixed6x8, g_ppcStatusStrings,  NUM_STATUS_STRINGS,
        NUM_STATUS_STRINGS, 0);
//*****************************************************************************
//
// The button used to enter hibernation.
//
//*****************************************************************************
RectangularButton(g_sHIBBtn, &g_sStatusList, 0, 0,
                  &g_sKentec320x240x16_SSD2119, 320-50-X_OFFSET, 190, 50, 40,
                  (PB_STYLE_TEXT | PB_STYLE_FILL | PB_STYLE_OUTLINE |
                   PB_STYLE_TEXT_OPAQUE), ClrDarkBlue, ClrDarkRed, 0,
                  ClrWhite, g_psFontCm14, "HIB", 0, 0, 0, 0, OnHIBBtnPress);

//*****************************************************************************
//
// The Canvas used to display the time.
//
//*****************************************************************************
Canvas(g_sRTC, &g_sStatusList, &g_sHIBBtn, 0, &g_sKentec320x240x16_SSD2119,
       (X_OFFSET+50), 200, 204, 20,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, 0, 0, ClrSilver,
       g_psFontCm16, 0, 0, 0);

//*****************************************************************************
//
// The button used to set the clock.
//
//*****************************************************************************
RectangularButton(g_sDateTimeSetBtn, &g_sStatusList, &g_sRTC, 0,
                  &g_sKentec320x240x16_SSD2119, X_OFFSET, 190, 50, 40,
                  (PB_STYLE_TEXT | PB_STYLE_FILL | PB_STYLE_OUTLINE |
                   PB_STYLE_TEXT_OPAQUE), ClrDarkBlue, ClrBlue, 0, ClrWhite,
                  g_psFontCm14, "CLOCK", 0, 0, 0, 0,
                  OnDateTimeSetBtnPress);

//*****************************************************************************
//
// The graphics library structures for the Date screen.
//
//*****************************************************************************
RectangularButton(g_sDateNextBtn, &g_sDateScreen, 0, 0,
                  &g_sKentec320x240x16_SSD2119, 240, 190, 60, 30,
                  (PB_STYLE_TEXT | PB_STYLE_FILL | PB_STYLE_OUTLINE |
                  PB_STYLE_TEXT_OPAQUE), ClrDarkBlue, ClrBlue, 0, ClrWhite,
                  g_psFontCm16, "NEXT", 0, 0, 0, 0, OnDateNextBtnPress);
CircularButton(g_sYearDwnBtn, &g_sDateScreen, &g_sDateNextBtn, 0,
               &g_sKentec320x240x16_SSD2119, 260, 90, 15, (PB_STYLE_TEXT |
               PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE | PB_STYLE_AUTO_REPEAT),
               ClrDarkBlue, ClrBlue, 0, ClrWhite, g_psFontCm20, "+", 0, 0, 100,
               10, OnYearDwnBtnPress);
CircularButton(g_sYearUpBtn, &g_sDateScreen, &g_sYearDwnBtn, 0,
               &g_sKentec320x240x16_SSD2119, 260, 153, 15, (PB_STYLE_TEXT |
               PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE | PB_STYLE_AUTO_REPEAT),
               ClrDarkBlue, ClrBlue, 0, ClrWhite, g_psFontCm20, "-", 0, 0, 100,
               10, OnYearUpBtnPress);
Canvas(g_sYearText, &g_sDateScreen, &g_sYearUpBtn, 0,
       &g_sKentec320x240x16_SSD2119, 230, 110, 60, 25, (CANVAS_STYLE_OUTLINE |
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT), ClrBlack, ClrWhite, ClrWhite,
       g_psFontCm16, g_pcYearBuf, 0, 0);
CircularButton(g_sDayDwnBtn, &g_sDateScreen, &g_sYearText, 0,
               &g_sKentec320x240x16_SSD2119, 160, 90, 15, (PB_STYLE_TEXT |
               PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE | PB_STYLE_AUTO_REPEAT),
               ClrDarkBlue, ClrBlue, 0, ClrWhite, g_psFontCm20, "+", 0, 0, 100,
               10, OnDayDwnBtnPress);
CircularButton(g_sDayUpBtn, &g_sDateScreen, &g_sDayDwnBtn, 0,
               &g_sKentec320x240x16_SSD2119, 160, 153, 15, (PB_STYLE_TEXT |
               PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE | PB_STYLE_AUTO_REPEAT),
               ClrDarkBlue, ClrBlue, 0, ClrWhite, g_psFontCm20, "-", 0, 0, 100,
               10, OnDayUpBtnPress);
Canvas(g_sDayText, &g_sDateScreen, &g_sDayUpBtn, 0,
       &g_sKentec320x240x16_SSD2119, 130, 110, 60, 25, (CANVAS_STYLE_OUTLINE |
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT), ClrBlack, ClrWhite, ClrWhite,
       g_psFontCm16, g_pcDayBuf, 0, 0);
CircularButton(g_sMonDwnBtn, &g_sDateScreen, &g_sDayText, 0,
               &g_sKentec320x240x16_SSD2119, 60, 90, 15, (PB_STYLE_TEXT |
               PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE | PB_STYLE_AUTO_REPEAT),
               ClrDarkBlue, ClrBlue, 0, ClrWhite, g_psFontCm20, "+", 0, 0, 100,
               20, OnMonDwnBtnPress);
CircularButton(g_sMonUpBtn, &g_sDateScreen, &g_sMonDwnBtn, 0,
               &g_sKentec320x240x16_SSD2119, 60, 153, 15, (PB_STYLE_TEXT |
               PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE | PB_STYLE_AUTO_REPEAT),
               ClrDarkBlue, ClrBlue, 0, ClrWhite, g_psFontCm20, "-", 0, 0, 100,
               20, OnMonUpBtnPress);
Canvas(g_sMonText, &g_sDateScreen, &g_sMonUpBtn, 0,
       &g_sKentec320x240x16_SSD2119, 30, 110, 60, 25, (CANVAS_STYLE_OUTLINE |
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT), ClrBlack, ClrWhite, ClrWhite,
       g_psFontCm16, g_pcMonBuf, 0, 0);
Canvas(g_sDateScreen, WIDGET_ROOT, 0, &g_sMonText,
       &g_sKentec320x240x16_SSD2119, X_OFFSET, Y_OFFSET, (320 - X_OFFSET*2),
       (240-X_OFFSET-Y_OFFSET),
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The graphics library structures for the Time screen.
//
//*****************************************************************************
RectangularButton(g_sTimeDoneBtn, &g_sTimeScreen, 0, 0,
                  &g_sKentec320x240x16_SSD2119, 240, 190, 60, 30,
                  (PB_STYLE_TEXT | PB_STYLE_FILL | PB_STYLE_OUTLINE |
                  PB_STYLE_TEXT_OPAQUE), ClrDarkBlue, ClrBlue, 0, ClrWhite,
                  g_psFontCm16, "DONE", 0, 0, 0, 0, OnTimeDoneBtnPress);
CircularButton(g_sAMPMDwnBtn, &g_sTimeScreen, &g_sTimeDoneBtn, 0,
               &g_sKentec320x240x16_SSD2119, 260, 90, 15, (PB_STYLE_TEXT |
               PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE), ClrDarkBlue, ClrBlue, 0,
               ClrWhite, g_psFontCm20, "+", 0, 0, 0, 0, OnAMPMBtnPress);
CircularButton(g_sAMPMUpBtn, &g_sTimeScreen, &g_sAMPMDwnBtn, 0,
               &g_sKentec320x240x16_SSD2119, 260, 153, 15, (PB_STYLE_TEXT |
               PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE), ClrDarkBlue, ClrBlue, 0,
               ClrWhite, g_psFontCm20, "-", 0, 0, 0, 0, OnAMPMBtnPress);
Canvas(g_sAMPMText, &g_sTimeScreen, &g_sAMPMUpBtn, 0,
       &g_sKentec320x240x16_SSD2119, 230, 110, 60, 25, (CANVAS_STYLE_OUTLINE |
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT), ClrBlack, ClrWhite, ClrWhite,
       g_psFontCm16, g_pcAMPMBuf, 0, 0);
CircularButton(g_sMinDwnBtn, &g_sTimeScreen, &g_sAMPMText, 0,
               &g_sKentec320x240x16_SSD2119, 160, 90, 15, (PB_STYLE_TEXT |
               PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE | PB_STYLE_AUTO_REPEAT),
               ClrDarkBlue, ClrBlue, 0, ClrWhite, g_psFontCm20, "+", 0, 0, 100,
               10, OnMinDwnBtnPress);
CircularButton(g_sMinUpBtn, &g_sTimeScreen, &g_sMinDwnBtn, 0,
               &g_sKentec320x240x16_SSD2119, 160, 153, 15, (PB_STYLE_TEXT |
               PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE | PB_STYLE_AUTO_REPEAT),
               ClrDarkBlue, ClrBlue, 0, ClrWhite, g_psFontCm20, "-", 0, 0, 100,
               10, OnMinUpBtnPress);
Canvas(g_sMinText, &g_sTimeScreen, &g_sMinUpBtn, 0,
       &g_sKentec320x240x16_SSD2119, 130, 110, 60, 25, (CANVAS_STYLE_OUTLINE |
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT), ClrBlack, ClrWhite, ClrWhite,
       g_psFontCm16, g_pcMinBuf, 0, 0);
CircularButton(g_sHourDwnBtn, &g_sTimeScreen, &g_sMinText, 0,
               &g_sKentec320x240x16_SSD2119, 60, 90, 15, (PB_STYLE_TEXT |
               PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE | PB_STYLE_AUTO_REPEAT),
               ClrDarkBlue, ClrBlue, 0, ClrWhite, g_psFontCm20, "+", 0, 0, 100,
               20, OnHourDwnBtnPress);
CircularButton(g_sHourUpBtn, &g_sTimeScreen, &g_sHourDwnBtn, 0,
               &g_sKentec320x240x16_SSD2119, 60, 153, 15, (PB_STYLE_TEXT |
               PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE | PB_STYLE_AUTO_REPEAT),
               ClrDarkBlue, ClrBlue, 0, ClrWhite, g_psFontCm20, "-", 0, 0, 100,
               20, OnHourUpBtnPress);
Canvas(g_sHourText, &g_sTimeScreen, &g_sHourUpBtn, 0,
       &g_sKentec320x240x16_SSD2119, 30, 110, 60, 25, (CANVAS_STYLE_OUTLINE |
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT), ClrBlack, ClrWhite, ClrWhite,
       g_psFontCm16, g_pcHourBuf, 0, 0);
Canvas(g_sTimeScreen, WIDGET_ROOT, 0, &g_sHourText,
       &g_sKentec320x240x16_SSD2119, 9, 25, (310 - 9), (230 - 25),
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

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
// Handle the "Date/Time" button press on Main Screen.
//
//*****************************************************************************
void
OnHIBBtnPress(tWidget *psWidget)
{
    //
    // The button has been touched.  So indicate that the
    // user wants the device to enter hibernation.
    //
    g_bHibernate = true;
}

//*****************************************************************************
//
// This function is used to add a new string to the status list box in the
// middle of the display.  This shows tamper events and system status.
//
//*****************************************************************************
static int
PrintfStatus(char *pcFormat, ...)
{
    int32_t i32Ret;
    va_list vaArgP;

    //
    // Start the varargs processing.
    //
    va_start(vaArgP, pcFormat);

    //
    // Call vsnprintf to perform the conversion.
    //
    i32Ret = uvsnprintf(g_pcStatus[g_ui32StatusStringIndex],
                        MAX_STATUS_STRING_LEN, pcFormat, vaArgP);

    //
    // End the varargs processing.
    //
    va_end(vaArgP);

    //
    // Add the new string to the status listbox.
    //
    ListBoxTextAdd(&g_sStatusList, g_pcStatus[g_ui32StatusStringIndex]);

    //
    // Update our string index.
    //
    g_ui32StatusStringIndex++;
    if(g_ui32StatusStringIndex == NUM_STATUS_STRINGS)
    {
        g_ui32StatusStringIndex = 0;
    }

    //
    // Repaint the status listbox.
    //
    WidgetPaint((tWidget *)&g_sStatusList);

    //
    // Return the conversion count.
    //
    return(i32Ret);

}

//*****************************************************************************
//
// Handles an NMI interrupt generated by a Tamper event.
//
//*****************************************************************************
void
NMITamperEventHandler(void)
{
    uint32_t ui32NMIStatus, ui32TamperStatus;
    uint32_t pui32Buf[3];
    uint8_t  ui8Idx, ui8StartIdx;
    bool     bDetectedEventsDuringClear;

    //
    // Get the cause of the NMI event.
    //
    ui32NMIStatus = SysCtlNMIStatus();

    //
    // Get the tamper event.
    //
    ui32TamperStatus = HibernateTamperStatusGet();

    if(CLASS_IS_TM4C129 && REVISION_IS_A0)
    {
        //
        // We should have got the cause of the NMI event in ui32NMIStatus.
        // But in Snowflake RA0 the NMIC register is not set correctly when an
        // event occurs.  So as a work around check if the NMI event is caused by a
        // tamper event and append this to the return value from SysCtlNMIStatus().
        // This way only this section can be removed once the bug is fixed in next
        // silicon rev.
        //
        if(ui32TamperStatus & (HIBERNATE_TAMPER_STATUS_EVENT |
                             HIBERNATE_TAMPER_STATUS_EXT_OSC_FAILED))
        {
            ui32NMIStatus |= SYSCTL_NMI_TAMPER;
        }
    }

    //
    // Clear NMI events if SysCtlNMIStatus() returned a value.
    //
    if(ui32NMIStatus)
    {
        SysCtlNMIClear(ui32NMIStatus);
    }

    //
    // Check if NMI interrupt is due to a tamper event.
    //
    if((ui32NMIStatus & SYSCTL_NMI_TAMPER) == 0)
    {

        //
        // Not due to tamper event.
        // Should not get here.
        //
        return;
    }

    //
    // Now handle NMI Interrupt due to a tamper event.
    //

    //
    // If the previous NMI event has not been processed by main
    // thread, we need to OR the new event along with the old ones.
    // Otherwise, clear the variables here.
    //
    if(g_ui32NMIEvent == 0)
    {
        //
        // Reset variables that used for tamper event.
        //
        g_ui32TamperEventFlag = 0;
        g_ui32TamperRTCLog = 0;

        //
        // Clean the log data for debugging purpose.
        //
        memset(g_ui32RTCLog, 0, sizeof(g_ui32RTCLog));
        memset(g_ui32EventLog, 0, sizeof(g_ui32EventLog));
    }


    //
    // Log the tamper event data before clearing tamper events.
    //
    for(ui8Idx = 0; ui8Idx< 4; ui8Idx++)
    {
        if(HibernateTamperEventsGet(ui8Idx,
                                    &g_ui32RTCLog[ui8Idx],
                                    &g_ui32EventLog[ui8Idx]))
        {
            //
            // If the timestamp entry has zero, ignore the tamper
            // log entry, otherwise, save the event.
            //
            if(g_ui32RTCLog[ui8Idx])
            {
                //
                // Event in this log entry, store it.
                //
                g_ui32TamperEventFlag |= g_ui32EventLog[ui8Idx];
                g_ui32TamperRTCLog = g_ui32RTCLog[ui8Idx];
            }
            else
            {
                //
                // Not valid event in this log entry. Done checking the logs.
                //
                break;
            }
        }
        else
        {
            //
            // No event in this log entry. Done checking the logs.
            //
            break;
        }
    }

    //
    // Process external oscillator failed event.
    //
    if(ui32TamperStatus & HIBERNATE_TAMPER_STATUS_EXT_OSC_FAILED)
    {
        g_ui32TamperXOSCFailEvent++;

        if(CLASS_IS_TM4C129 && REVISION_IS_A0)
        {
            //
            // Snowflake A0 bug: XOSCFAIL doesn't get logged
            //
            g_ui32TamperEventFlag |= HIBERNATE_TAMPER_EVENT_EXT_OSC;
            g_ui32TamperRTCLog = HWREG(HIB_TPLOG0);
        }
    }

    //
    // Clear tamper event.
    //
    //
    // The following block of code is to workaround hardware defect
    // which results in missing new tamper events during tamper clear
    // synchronization.
    //
    // There is a window after the application code writes the tamper
    // clear where a new tamper event can be missed if the application
    // requires more than one tamper event pins detection.
    //
    // The tamper Clear is synchronized to the hibernate 32kHz clock
    // domain.  The clear takes 3 rising edges of the 32KHz clock.
    // During this window, new tamper events could be missed.
    // A software workaround is to poll the tamper log during the
    // tamper event clear synchronization.
    //


    //
    // Record the index for the first empty log entry, this is the first
    // entry log we will be polling from for any new events.
    //
    if(ui8Idx >= 4)
    {
        //
        // All 4 log entries have data, new event will be ORed in the last
        // entry.
        //
        ui8StartIdx = 3;
    }
    else
    {
        ui8StartIdx = ui8Idx;
    }

    //
    // Clear the flag for the case there are events triggered
    // during clear execution.
    //
    bDetectedEventsDuringClear = false;

    //
    // Unlock the Tamper Control register. This is required before
    // calling HibernateTamperEventsClearNoLock().
    //
    HibernateTamperUnLock();
    do
    {

        //
        // Clear the Tamper event.
        // Note this API doesn't wait for synchronization, which
        // allows us to check the tamper log during synchronization.
        //
        HibernateTamperEventsClearNoLock();

        //
        // Check new tamper event during tamper event clear
        // synchronization, start polling until clear is done.
        // This will take about 92us(three clock cycles) at most.
        //
        while(HibernateTamperStatusGet() & HIBERNATE_TAMPER_STATUS_EVENT)
        {

            //
            // Clear execution isn't done yet , poll for new events.
            // If there were any new event, it will be logged starting
            // the first empty log entry.
            //
            for(ui8Idx = ui8StartIdx; ui8Idx< 4; ui8Idx++)
            {
                if(HibernateTamperEventsGet(ui8Idx,
                                            &g_ui32RTCLog[ui8Idx],
                                            &g_ui32EventLog[ui8Idx]))
                {
                    //
                    // If the timestamp log has zero, ignore the tamper
                    // log entry.
                    //
                    if(g_ui32RTCLog[ui8Idx])
                    {
                        //
                        // detected new event, store it.
                        //
                        g_ui32TamperEventFlag |= g_ui32EventLog[ui8Idx];
                    }
                    else
                    {
                        //
                        // Not valid event in this log, update the log
                        // entry index to be checked in next iteration.
                        // Break out of loop.
                        //
                        ui8StartIdx = ui8Idx;
                        break;
                    }

                    //
                    // check for more event.
                    //
                    continue;
                }
                else
                {
                    //
                    // No new event in this log, update the log
                    // entry index to be checked in next iteration.
                    // Break out of loop.
                    //
                    ui8StartIdx = ui8Idx;
                    break;
                }
            }

            //
            // all last three logs have info. Check if all 4 logs
            // have the same info. This is to detect the case that
            // events happen during clear execution.
            //
            if(ui8Idx == 4)
            {
                //
                // If events happens during clear
                // execution, all four log registers will be
                // logged with the same event, to detect this
                // condition, we will compare with all four log data.
                //
                if(HibernateTamperEventsGet(0, &g_ui32RTCLog[0], &g_ui32EventLog[0]))
                {
                    if((g_ui32RTCLog[0] == g_ui32RTCLog[1])     &&
                       (g_ui32EventLog[0] == g_ui32EventLog[1]) &&
                       (g_ui32RTCLog[0] == g_ui32RTCLog[2])     &&
                       (g_ui32EventLog[0] == g_ui32EventLog[2]) &&
                       (g_ui32RTCLog[0] == g_ui32RTCLog[3])     &&
                       (g_ui32EventLog[0] == g_ui32EventLog[3]))
                    {
                        //
                        // Detected events during clear execution.
                        // Event logging takes priority, the clear
                        // will not be done in this case. We will need
                        // to go back to the beginning of the loop and
                        // clear the events.
                        //
                        if(bDetectedEventsDuringClear)
                        {
                            //
                            // This condition has already detected,
                            // we have cleared the event,
                            // clear the flag.
                            //
                            bDetectedEventsDuringClear = false;
                        }
                        else
                        {
                            // This is the first time it has been
                            // detected, set the flag.
                            //
                            bDetectedEventsDuringClear = true;
                        }

                        //
                        // Break out of while loop so that we can
                        // clear the events, and start the
                        // workaround all over again.
                        //
                        break;
                    }
                }
                else
                {
                    //
                    // Log 0 didn't detect any events. So this is not
                    // the case of missing events during clear
                    // execution.
                    // Update the log index at which we will poll next.
                    // It should be the last log entry that OR all the
                    // new events.
                    //
                    ui8StartIdx = 3;
                }
            }
        }
    }
    while(bDetectedEventsDuringClear);

    //
    // Lock the Tamper Control register.
    //
    HibernateTamperLock();

    //
    // Save the tamper event and RTC log info in the Hibernate Memory
    //
    HibernateDataGet(pui32Buf, 3);
    pui32Buf[1] = g_ui32TamperEventFlag;
    pui32Buf[2] = g_ui32TamperRTCLog;
    HibernateDataSet(pui32Buf, 3);

    //
    // Signal the main loop that an NMI event occurred.
    //
    g_ui32NMIEvent++;

}

//*****************************************************************************
//
// The interrupt handler for the Hibernate interrupt. It clears any pending
// interrupts and set the flag used by the application to update calendar time
// on the display.
//
//*****************************************************************************
void
HibernateIntHandler(void)
{
    uint32_t ui32Status;

    //
    // Get the interrupt status, and clear any pending interrupts.
    //
    ui32Status = ROM_HibernateIntStatus(1);
    ROM_HibernateIntClear(ui32Status);

    //
    // Process the RTC match 0 interrupt
    //
    if(ui32Status & HIBERNATE_INT_RTC_MATCH_0)
    {
        g_bUpdateRTC = 1;
    }
}

//*****************************************************************************
//
// Returns whether the system has come out of Hibernation due to a tamper
// event or reset event.
//
//*****************************************************************************
void
HibernateTamperWakeUp(bool *pbWakeupFromTamper, bool *pbWakeupFromReset)
{
    uint32_t ui32TempBuf[3];

    //
    // Clear both flags.
    //
    *pbWakeupFromReset  = false;
    *pbWakeupFromTamper = false;

    //
    // Read the status bits to see what caused the wake.  Clear the wake
    // source so that the device can be put into hibernation again.
    //
    ui32TempBuf[0] = HibernateIntStatus(0);
    HibernateIntClear(ui32TempBuf[0]);

    //
    // Check the wake was due to reset.
    //
    if(ui32TempBuf[0] & HIBERNATE_INT_RESET_WAKE)
    {
        *pbWakeupFromReset = true;
        return;
    }

    //
    // The wake was not due to reset.
    // Check if it is due to tamper event.
    //
    // Read the Hibernate module memory registers that show the state of the
    // system.
    //
    HibernateDataGet(ui32TempBuf, 3);

    //
    // Determine if system came out of hibernation due to a tamper event.
    //
    if(ui32TempBuf[0] == HIBERNATE_TAMPER_DATA0)
    {
        //
        // It is due to a tamper event.
        // Read the saved tamper event and RTC log info from the hibernate
        // memory, so the main routine can print the info on the display.
        //
        g_ui32TamperEventFlag = ui32TempBuf[1];
        g_ui32TamperRTCLog = ui32TempBuf[2];
        g_ui32NMIEvent++;
        *pbWakeupFromTamper = true;
        return;
    }

    return;
}

//*****************************************************************************
//
// This function convert the hour in 24 hour format to 12 hour format
//
//*****************************************************************************
void
ConvertHourTo12Mode(uint8_t *pui8Hour, bool *pbPM)
{
    if(*pui8Hour == 0)
    {
        *pbPM = false;
        *pui8Hour += 12;
    }
    else if(*pui8Hour == 12)
    {
        *pbPM = true;
    }
    else if(*pui8Hour > 12)
    {
        *pui8Hour -= 12;
        *pbPM = true;
    }
    else
    {
        *pbPM = false;
    }
}

//*****************************************************************************
//
// This function returns the number of days in a month including for a leap
// year.
//
//*****************************************************************************
uint32_t
GetDaysInMonth(uint32_t ui32Year, uint32_t ui32Mon)
{
    //
    // Return the number of days based on the month.
    //
    if(ui32Mon == 1)
    {
        //
        // For February return the number of days based on the year being a
        // leap year or not.
        //
        if((ui32Year % 4) == 0)
        {
            //
            // If leap year return 29.
            //
            return 29;
        }
        else
        {
            //
            // If not leap year return 28.
            //
            return 28;
        }
    }
    else if((ui32Mon == 3) || (ui32Mon == 5) || (ui32Mon == 8) ||
            (ui32Mon == 10))
    {
        //
        // For April, June, September and November return 30.
        //
        return 30;
    }

    //
    // For all the other months return 31.
    //
    return 31;
}

//*****************************************************************************
//
// This function updates individual buffers with valid date and time to be
// displayed on the date screen so that the date and time can be updated.
//
//*****************************************************************************
bool
DateTimeUpdateGet(char *pcMon, char *pcDay, char *pcYear, char *pcHour,
                  char *pcMin, char *pcAMPM)
{
    struct tm sTime;
    bool   bPM;


    //
    // Get the latest time.
    //
    HibernateCalendarGet(&sTime);

    //
    // Set AM as default.
    //
    bPM = false;

    //
    // Convert 24-hour format into 12-hour format with AM/PM indication.
    //
    ConvertHourTo12Mode((uint8_t*)&sTime.tm_hour, &bPM);
    strcpy(pcAMPM, bPM?"PM":"AM");

    //
    // If date and time is valid, copy the date and time values into respective
    // indexes.
    //
    g_ui32MonthIdx = sTime.tm_mon;
    g_ui32DayIdx = sTime.tm_mday;
    g_ui32YearIdx = sTime.tm_year - 100;
    g_ui32HourIdx = sTime.tm_hour;
    g_ui32MinIdx = sTime.tm_min;

    //
    // Convert the date and time into string format and copy into supplied
    // buffers.
    //
    usnprintf(pcMon, 4, "%s", g_ppcMonth[g_ui32MonthIdx]);
    usnprintf(pcDay, 3, "%u", g_ui32DayIdx);
    usnprintf(pcYear, 5, "20%02u", g_ui32YearIdx);
    usnprintf(pcHour, 3, "%u", g_ui32HourIdx);
    usnprintf(pcMin, 3, "%02u", g_ui32MinIdx);

    //
    // Return true to indicate new information has been updated.
    //
    return true;
}

//*****************************************************************************
//
// This function writes the requested date and time to the calendar logic of
// hibernation module.
//
//*****************************************************************************
void
DateTimeSet(void)
{
    struct tm sTime;

    //
    // Get the latest date and time.  This is done here so that unchanged
    // parts of date and time can be written back as is.
    //
    HibernateCalendarGet(&sTime);

    //
    // Set the date and time values that are to be updated.
    //
    sTime.tm_hour = g_ui32HourIdx;
    sTime.tm_min = g_ui32MinIdx;
    sTime.tm_mon = g_ui32MonthIdx;
    sTime.tm_mday = g_ui32DayIdx;
    sTime.tm_year = 100 + g_ui32YearIdx;

    //
    // Convert 12-hour format into 24-hour format.
    //
    if(strcmp(g_pcAMPMBuf, "PM") == 0)
    {
        if(sTime.tm_hour < 12)
        {
            sTime.tm_hour += 12;
        }
    }
    else
    {
        if(sTime.tm_hour > 11)
        {
            sTime.tm_hour -= 12;
        }
    }

    //
    // Update the calendar logic of hibernation module with the requested data.
    //
    HibernateCalendarSet(&sTime);
}

//*****************************************************************************
//
// Handle the "Date/Time" button press on Main Screen.
//
//*****************************************************************************
void
OnDateTimeSetBtnPress(tWidget *psWidget)
{
    //
    // Update the date and time screen buffers before painting the screens.
    // Even though only buffers used with the date screen are needed at this
    // time, we update both to save few cycles.
    //
    DateTimeUpdateGet(g_pcMonBuf, g_pcDayBuf, g_pcYearBuf, g_pcHourBuf,
                      g_pcMinBuf, g_pcAMPMBuf);

    //
    // Remove the main screen from the root.
    //
    WidgetRemove((tWidget *)&g_sMainScreen);

    //
    // Add the date screen to the root.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sDateScreen);

    //
    // Add the widget tree to message queue so that it can be drawn on the
    // display.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Set the flag to indicate that the main screen is not active.
    //
    g_bMainScreen = false;
}

//*****************************************************************************
//
// Handle the Month "-" button press on Date Screen.
//
//*****************************************************************************
void
OnMonUpBtnPress(tWidget *psWidget)
{
    uint32_t ui32DaysInMonth;

    //
    // User wants to decrement the month.  So reduce index by one such that the
    // index cycles back to the bottom of the list if it is at the top.
    //
    g_ui32MonthIdx = (g_ui32MonthIdx == 0) ? 11 : g_ui32MonthIdx - 1;

    //
    // Store the index in a buffer as a string and add the widget to the
    // message queue so that it can be redrawn.
    //
    strcpy(g_pcMonBuf, g_ppcMonth[g_ui32MonthIdx]);
    WidgetPaint(&g_sMonText.sBase);

    //
    // Makes sure that the day value does not exceed the number of days for the
    // selected month.
    //
    ui32DaysInMonth = GetDaysInMonth(g_ui32YearIdx, g_ui32MonthIdx);
    g_ui32DayIdx = (g_ui32DayIdx > ui32DaysInMonth) ? ui32DaysInMonth :
                   g_ui32DayIdx;

    //
    // Since the day value might also have been modified, store the day index
    // in a buffer as a string and add the widget to the message queue so that
    // it can be redrawn.
    //
    usnprintf(g_pcDayBuf, sizeof(g_pcDayBuf), "%02u", g_ui32DayIdx);
    WidgetPaint(&g_sDayText.sBase);
}

//*****************************************************************************
//
// Handle the Month "+" button press on Date Screen.
//
//*****************************************************************************
void
OnMonDwnBtnPress(tWidget *psWidget)
{
    uint32_t ui32DaysInMonth;

    //
    // User wants to increment the month.  So increment index by one such that
    // the index cycles back to the top of the list if it is at the bottom.
    //
    g_ui32MonthIdx = (g_ui32MonthIdx == 11) ? 0 : g_ui32MonthIdx + 1;

    //
    // Store the index in a buffer as a string and add the widget to the
    // message queue so that it can be redrawn.
    //
    strcpy(g_pcMonBuf, g_ppcMonth[g_ui32MonthIdx]);
    WidgetPaint(&g_sMonText.sBase);

    //
    // Makes sure that the day value does not exceed the number of days for the
    // selected month.
    //
    ui32DaysInMonth = GetDaysInMonth(g_ui32YearIdx, g_ui32MonthIdx);
    g_ui32DayIdx = (g_ui32DayIdx > ui32DaysInMonth) ? ui32DaysInMonth :
                   g_ui32DayIdx;

    //
    // Since the day value might also have been modified, store the day index
    // in a buffer as a string and add the widget to the message queue so that
    // it can be redrawn.
    //
    usnprintf(g_pcDayBuf, sizeof(g_pcDayBuf), "%02u", g_ui32DayIdx);
    WidgetPaint(&g_sDayText.sBase);
}

//*****************************************************************************
//
// Handle the Day "-" button press on Date Screen.
//
//*****************************************************************************
void
OnDayUpBtnPress(tWidget *psWidget)
{
    uint32_t ui32DaysInMonth;

    //
    // User wants to decrement the day.  So reduce index by one such that the
    // index cycles back to the bottom of the list if it is at the top.  Make
    // sure that the day index is adjusted based on the number of days in a
    // month.
    //
    ui32DaysInMonth = GetDaysInMonth(g_ui32YearIdx, g_ui32MonthIdx);
    g_ui32DayIdx = (g_ui32DayIdx < 2) ? ui32DaysInMonth : g_ui32DayIdx - 1;

    //
    // Store the index in a buffer as a string and add the widget to the
    // message queue so that it can be redrawn.
    //
    usnprintf(g_pcDayBuf, sizeof(g_pcDayBuf), "%02u", g_ui32DayIdx);
    WidgetPaint(&g_sDayText.sBase);
}

//*****************************************************************************
//
// Handle the Day "+" button press on Date Screen.
//
//*****************************************************************************
void
OnDayDwnBtnPress(tWidget *psWidget)
{
    uint32_t ui32DaysInMonth;

    //
    // User wants to increment the day.  So increment index by one such that
    // the index cycles back to the top of the list if it is at the bottom.
    // Make sure that the day index does not exceed the number of days for the
    // selected month.
    //
    ui32DaysInMonth = GetDaysInMonth(g_ui32YearIdx, g_ui32MonthIdx);
    g_ui32DayIdx = (g_ui32DayIdx > (ui32DaysInMonth - 1)) ? 1 :
                   (g_ui32DayIdx + 1);

    //
    // Store the index in a buffer as a string and add the widget to the
    // message queue so that it can be redrawn.
    //
    usnprintf(g_pcDayBuf, sizeof(g_pcDayBuf), "%02u", g_ui32DayIdx);
    WidgetPaint(&g_sDayText.sBase);
}

//*****************************************************************************
//
// Handle the Year "-" button press on Date Screen.
//
//*****************************************************************************
void
OnYearUpBtnPress(tWidget *psWidget)
{
    uint32_t ui32DaysInMonth;

    //
    // User wants to decrement the year.  So reduce index by one such that the
    // index cycles back to the bottom of the list if it is at the top.
    //
    g_ui32YearIdx = (g_ui32YearIdx == 0) ? 99 : g_ui32YearIdx - 1;

    //
    // Store the index in a buffer as a string and add the widget to the
    // message queue so that it can be redrawn.
    //
    usnprintf(g_pcYearBuf, sizeof(g_pcYearBuf), "20%02u", g_ui32YearIdx);
    WidgetPaint(&g_sYearText.sBase);

    //
    // Makes sure that the day value does not exceed the number of days for the
    // selected month.
    //
    ui32DaysInMonth = GetDaysInMonth(g_ui32YearIdx, g_ui32MonthIdx);
    g_ui32DayIdx = (g_ui32DayIdx > ui32DaysInMonth) ? ui32DaysInMonth :
                   g_ui32DayIdx;

    //
    // Since the day value might also have been modified, store the day index
    // in a buffer as a string and add the widget to the message queue so that
    // it can be redrawn.
    //
    usnprintf(g_pcDayBuf, sizeof(g_pcDayBuf), "%02u", g_ui32DayIdx);
    WidgetPaint(&g_sDayText.sBase);
}

//*****************************************************************************
//
// Handle the Year "+" button press on Date Screen.
//
//*****************************************************************************
void
OnYearDwnBtnPress(tWidget *psWidget)
{
    uint32_t ui32DaysInMonth;

    //
    // User wants to increment the year.  So increment index by one such that
    // the index cycles back to the top of the list if it is at the bottom.
    //
    g_ui32YearIdx = (g_ui32YearIdx == 99) ? 0 : g_ui32YearIdx + 1;

    //
    // Store the index in a buffer as a string and add the widget to the
    // message queue so that it can be redrawn.
    //
    usnprintf(g_pcYearBuf, sizeof(g_pcYearBuf), "20%02u", g_ui32YearIdx);
    WidgetPaint(&g_sYearText.sBase);

    //
    // Makes sure that the day value does not exceed the number of days for the
    // selected month.
    //
    ui32DaysInMonth = GetDaysInMonth(g_ui32YearIdx, g_ui32MonthIdx);
    g_ui32DayIdx = (g_ui32DayIdx > ui32DaysInMonth) ? ui32DaysInMonth :
                   g_ui32DayIdx;

    //
    // Since the day value might also have been modified, store the day index
    // in a buffer as a string and add the widget to the message queue so that
    // it can be redrawn.
    //
    usnprintf(g_pcDayBuf, sizeof(g_pcDayBuf), "%02u", g_ui32DayIdx);
    WidgetPaint(&g_sDayText.sBase);
}

//*****************************************************************************
//
// Handle the "NEXT" button press on Date Screen.
//
//*****************************************************************************
void
OnDateNextBtnPress(tWidget *psWidget)
{
    //
    // Remove the main screen from the root.
    //
    WidgetRemove((tWidget *)&g_sDateScreen);

    //
    // Add the date screen to the root.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sTimeScreen);

    //
    // Add the widget tree to message queue so that it can be drawn on the
    // display.
    //
    WidgetPaint(WIDGET_ROOT);
}

//*****************************************************************************
//
// Handle the Hour "-" button press on Time Screen.
//
//*****************************************************************************
void
OnHourUpBtnPress(tWidget *psWidget)
{
    //
    // User wants to decrement the hour.  So reduce index by one such that the
    // index cycles back to the bottom of the list if it is at the top.
    //
    g_ui32HourIdx = (g_ui32HourIdx == 1) ? 12 : g_ui32HourIdx - 1;

    //
    // Store the index in a buffer as a string and add the widget to the
    // message queue so that it can be redrawn.
    //
    usnprintf(g_pcHourBuf, sizeof(g_pcHourBuf), "%u", g_ui32HourIdx);
    WidgetPaint(&g_sHourText.sBase);
}

//*****************************************************************************
//
// Handle the Hour "+" button press on Time Screen.
//
//*****************************************************************************
void
OnHourDwnBtnPress(tWidget *psWidget)
{
    //
    // User wants to increment the hour.  So increment index by one such that
    // the index cycles back to the top of the list if it is at the bottom.
    //
    g_ui32HourIdx = (g_ui32HourIdx == 12) ? 1 : g_ui32HourIdx + 1;

    //
    // Store the index in a buffer as a string and add the widget to the
    // message queue so that it can be redrawn.
    //
    usnprintf(g_pcHourBuf, sizeof(g_pcHourBuf), "%u", g_ui32HourIdx);
    WidgetPaint(&g_sHourText.sBase);
}

//*****************************************************************************
//
// Handle the Minute "-" button press on Time Screen.
//
//*****************************************************************************
void
OnMinUpBtnPress(tWidget *psWidget)
{
    //
    // User wants to decrement the minute.  So reduce index by one such that
    // the index cycles back to the bottom of the list if it is at the top.
    //
    g_ui32MinIdx = (g_ui32MinIdx == 0) ? 59 : g_ui32MinIdx - 1;

    //
    // Store the index in a buffer as a string and add the widget to the
    // message queue so that it can be redrawn.
    //
    usnprintf(g_pcMinBuf, sizeof(g_pcMinBuf), "%02u", g_ui32MinIdx);
    WidgetPaint(&g_sMinText.sBase);
}

//*****************************************************************************
//
// Handle the Minute "+" button press on Time Screen.
//
//*****************************************************************************
void
OnMinDwnBtnPress(tWidget *psWidget)
{
    //
    // User wants to increment the minute.  So increment index by one such that
    // the index cycles back to the top of the list if it is at the bottom.
    //
    g_ui32MinIdx = (g_ui32MinIdx == 59) ? 0 : g_ui32MinIdx + 1;

    //
    // Store the index in a buffer as a string and add the widget to the
    // message queue so that it can be redrawn.
    //
    usnprintf(g_pcMinBuf, sizeof(g_pcMinBuf), "%02u", g_ui32MinIdx);
    WidgetPaint(&g_sMinText.sBase);
}

//*****************************************************************************
//
// Handle the AM/PM "+" and "-" button press on Time Screen.  The same function
// handles both the button presses.
//
//*****************************************************************************
void
OnAMPMBtnPress(tWidget *psWidget)
{
    //
    // User wants to change AM to PM or vice versa.  Change to "PM" if "AM" and
    // vice versa.
    //
    if(strcmp(g_pcAMPMBuf, "AM") == 0)
    {
        strcpy(g_pcAMPMBuf, "PM");
    }
    else
    {
        strcpy(g_pcAMPMBuf, "AM");
    }

    //
    // Add the widget to the message queue so that it can be redrawn.
    //
    WidgetPaint(&g_sAMPMText.sBase);
}

//*****************************************************************************
//
// Handle the "DONE" button press on Time Screen.
//
//*****************************************************************************
void
OnTimeDoneBtnPress(tWidget *psWidget)
{
    //
    // Remove the time screen from the root.
    //
    WidgetRemove((tWidget *)&g_sTimeScreen);

    //
    // Add the main screen to the root.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sMainScreen);

    //
    // Add the widget tree to message queue so that it can be drawn on the
    // display.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Set the flag to indicate that date and time have to updated.
    //
    g_bSetDate = true;
}

//*****************************************************************************
//
// This example demonstrates the use of the Tamper module.
//
//*****************************************************************************
int
main(void)
{
    tContext sContext;
    uint32_t ui32SysClock;
    uint32_t ui32Index;
    uint32_t ui32TempBuf[3];
    struct tm sTime;
    bool bWakeFromTamper;
    bool bWakeFromReset;
    char pcBuf[64];
    char pcRTCBuf[32];

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
    FrameDraw(&sContext, "tamper");

    //
    // Initialize the touch screen driver and have it route its messages to the
    // widget tree.
    //
    TouchScreenInit(ui32SysClock);
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Check to see if the processor is waking from a hibernation.
    //
    HibernateTamperWakeUp(&bWakeFromTamper, &bWakeFromReset);

    if(!bWakeFromTamper && !bWakeFromReset)
    {
        //
        // If the system didn't wake from hibernation,
        // Initialize the Hibernate module and enable the RTC/calendar mode.
        //
        HibernateEnableExpClk(0);
        HibernateRTCEnable();
        HibernateCounterMode(HIBERNATE_COUNTER_24HR);

        //
        // Fill the first three words in the Hibernate module memory with
        // the required values.
        //
        ui32TempBuf[0] = HIBERNATE_TAMPER_DATA0;
        ui32TempBuf[1] = 0;
        ui32TempBuf[2] = 0;
        HibernateDataSet(ui32TempBuf, 3);

        //
        // Configure Hibernate wake sources.
        //
        HibernateWakeSet(HIBERNATE_WAKE_RESET);

        //
        // Start Calendar time from 2013/8/1 00:00:00
        //
        sTime.tm_hour = 0;
        sTime.tm_min = 0;
        sTime.tm_sec = 0;
        sTime.tm_wday = 4;
        sTime.tm_mon = 7;    // [0, 11]
        sTime.tm_mday = 1;   // [1, 31]
        sTime.tm_year = 113; // since 1900
        HibernateCalendarSet(&sTime);

        //
        // Set Calendar match to every second
        //
        memset((void*)&sTime, 0, sizeof(struct tm));
        sTime.tm_sec = 0xFF;
        sTime.tm_min = 0xFF;
        sTime.tm_hour = 0xFF;
        sTime.tm_mday = 0xFF;
        HibernateCalendarMatchSet(0, &sTime);

        //
        // Enable Calendar Match interrupt
        //
        ROM_HibernateIntClear(HIBERNATE_INT_RTC_MATCH_0);
        ROM_HibernateIntEnable(HIBERNATE_INT_RTC_MATCH_0);


        //
        // Configure the TPIO0~3 signals by enabling trigger on low,
        // weak pull-up and glitch filtering.
        //
        for(ui32Index = 0; ui32Index < 4; ui32Index++)
        {
            HibernateTamperIOEnable(ui32Index,
                                    HIBERNATE_TAMPER_IO_TRIGGER_LOW |
                                    HIBERNATE_TAMPER_IO_WPU_ENABLED |
                                    HIBERNATE_TAMPER_IO_MATCH_SHORT);
        }

        //
        // Configure Hibernate module to wake up from tamper event.
        //
        HibernateTamperEventsConfig(HIBERNATE_TAMPER_EVENTS_HIB_WAKE);

        //
        // Enable Tamper Module.
        //
        HibernateTamperEnable();
    }
    else
    {
        if(bWakeFromTamper &&
           (g_ui32TamperEventFlag & HIBERNATE_TAMPER_EVENT_EXT_OSC))
        {
            //
            // XOSCFAIL was used to trigger a tamper event, set the flag
            // g_ui32TamperXOSCFailEvent so that it will clear external
            // oscillator failure after the external oscillator becomes
            // active.
            //
            g_ui32TamperXOSCFailEvent = 1;
        }
    }

    //
    // Clear Tamper event flag.
    //
    g_ui32NMIEvent = 0;


    //
    // Enable interrupts.
    //
    IntEnable(INT_HIBERNATE);
    IntMasterEnable();

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sMainScreen);

    //
    // Print instruction
    //
    if(!bWakeFromTamper && !bWakeFromReset)
    {
        PrintfStatus("Tamper Example app instruction:");
        PrintfStatus("-Ground PM4~7 to GND to trigger tamper events.");
        PrintfStatus(" Corresponding indicator above should lightup") ;
        PrintfStatus(" upon detection, the event along with the time");
        PrintfStatus(" stamp should be logged on the display.");
        PrintfStatus("-Tap HIB button on the display to hibernate, and");
        PrintfStatus(" press RESET button or ground PM4~7 to wake up");
        PrintfStatus(" from hibernation.");
    }
    else
    {
        PrintfStatus("Wake from %s...",
                     bWakeFromReset? "RESET":"tamper event");
    }

    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);

    g_bMainScreen = true;
    g_bHibernate = false;
    g_bSetDate = false;

    while(1)
    {

        if( g_bMainScreen && g_bUpdateRTC )
        {
            bool bPM = false;
            uint8_t ui8Hour;

            g_bUpdateRTC = 0;
            HibernateCalendarGet(&sTime);
            ui8Hour = sTime.tm_hour;
            //
            // covert the hour to 12 hour format
            //
            ConvertHourTo12Mode(&ui8Hour, &bPM);
            usprintf(pcRTCBuf, " %02d/%02d/%04d %02d:%02d:%02d %s",
                       (sTime.tm_mon+1), sTime.tm_mday, (sTime.tm_year+1900),
                       ui8Hour, sTime.tm_min, sTime.tm_sec, (bPM?"PM":"AM") );
            CanvasTextSet(&g_sRTC, pcRTCBuf);
            WidgetPaint((tWidget *)&g_sRTC.sBase);
        }

        //
        // Check if date and time has to be written to the calendar logic of
        // hibernate module.
        //
        if(g_bSetDate == true)
        {
            //
            // Yes - Clear the flag to avoid unwanted writes to the calendar
            // logic.
            //
            g_bSetDate = false;

            //
            // Write the requested date and time.
            //
            DateTimeSet();

            //
            // Set the flag to indicate that the main screen is active.
            //
            g_bMainScreen = true;
        }

        if(g_ui32NMIEvent && g_bMainScreen)
        {
            uint8_t ui8Pos= 0;
            uint8_t ui8Hour;
            bool    bPM;

            //
            // Save the temper event first, so that they don't get overwriten
            // from IRQ while we reference them.
            //
            ui32TempBuf[0] = g_ui32TamperEventFlag;
            ui32TempBuf[1] = g_ui32TamperRTCLog;

            //
            // Clear NMI event counter
            //
            g_ui32NMIEvent = 0;

            //
            // Update GPIO indicator lights and status list box
            //
            if(ui32TempBuf[0] & HIBERNATE_TAMPER_EVENT_0)
            {
                CanvasImageSet(&g_sIndicator0,g_pui8LightOn);
                strcpy(pcBuf, "PM7/TMPR0 ");
                ui8Pos = 10;
            }
            else
            {
                CanvasImageSet(&g_sIndicator0, g_pui8LightOff);
            }

            if(ui32TempBuf[0] & HIBERNATE_TAMPER_EVENT_1)
            {
                CanvasImageSet(&g_sIndicator1, g_pui8LightOn);
                strcpy(pcBuf + ui8Pos, "PM6/TMPR1 ");
                ui8Pos += 10;
            }
            else
            {
                CanvasImageSet(&g_sIndicator1, g_pui8LightOff);
            }

            if(ui32TempBuf[0] & HIBERNATE_TAMPER_EVENT_2)
            {
                CanvasImageSet(&g_sIndicator2,g_pui8LightOn);
                strcpy(pcBuf + ui8Pos, "PM5/TMPR2 ");
                ui8Pos += 10;
            }
            else
            {
                CanvasImageSet(&g_sIndicator2, g_pui8LightOff);
            }

            if(ui32TempBuf[0] & HIBERNATE_TAMPER_EVENT_3)
            {
                CanvasImageSet(&g_sIndicator3,g_pui8LightOn);
                strcpy(pcBuf + ui8Pos, "PM4/TMPR3 ");
                ui8Pos += 10;
            }
            else
            {
                CanvasImageSet(&g_sIndicator3,g_pui8LightOff);
            }

            if(ui32TempBuf[0] & HIBERNATE_TAMPER_EVENT_EXT_OSC)
            {
                strcpy(pcBuf + ui8Pos, "XOSCFAIL ");
            }

            WidgetPaint((tWidget *)&g_sMainScreen);

            //
            // covert the hour to 12 hour format
            //
            ui8Hour = (ui32TempBuf[1]&0x0001F000)>>12;
            ConvertHourTo12Mode(&ui8Hour, &bPM);
            PrintfStatus("%s triggered on", pcBuf);
            PrintfStatus("    %02d/%02d/%04d %02d:%02d:%02d %s",
                         (ui32TempBuf[1]&0x03C00000)>>22,
                         (ui32TempBuf[1]&0x003E0000)>>17,
                         (((ui32TempBuf[1]&0xFC000000)>>26) + 2000),
                         ui8Hour,
                         (ui32TempBuf[1]&0x00000FC0)>>6,
                         (ui32TempBuf[1]&0x0000003F),
                         (bPM?"PM":"AM"));

        }

        //
        // Wait for the user to touch the display panel.
        //
        if(g_bHibernate == true)
        {
            //
            // Clear the flag.
            //
            g_bHibernate = false;

            //
            // Read and clear any status bits that might have been set since
            // last clearing them.
            //
            ui32Index = HibernateIntStatus(0);
            HibernateIntClear(ui32Index);

            PrintfStatus("Hibernating...");

            //
            // Process any messages in the widget message queue.
            //
            WidgetMessageQueueProcess();

            //
            // Request Hibernation.
            //
            HibernateRequest();

            //
            // Wait for a while for hibernate to activate.  It should never
            // get past this point.
            //
            SysCtlDelay(100);
        }

        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();

        //
        // If we have external oscillator failure, wait till the external
        // oscillator becomes active, then clear external oscillator failure.
        //
        if(g_ui32TamperXOSCFailEvent)
        {
            while(HibernateTamperExtOscValid() == 0);
            HibernateTamperExtOscRecover();
            g_ui32TamperXOSCFailEvent = 0;
        }
    }

}
