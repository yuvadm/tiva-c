//*****************************************************************************
//
// hibernate.c - Hibernation Example.
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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "inc/hw_gpio.h"
#include "inc/hw_hibernate.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/hibernate.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"
#include "utils/ustdlib.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Hibernate Example (hibernate)</h1>
//!
//! An example to demonstrate the use of the Hibernation module.  The user
//! can put the microcontroller in hibernation by touching the display.  The
//! microcontroller will then wake on its own after 5 seconds, or immediately
//! if the user presses the RESET button.  External WAKE pin and GPIO (PK5)
//! wake sources can also be used to wake immediately from hibernation.  The
//! following wiring enables the use of these pins as wake sources.
//!     WAKE on J27 to SEL on J37
//!     PK5 on J28 to UP on J37
//!
//! The program keeps a count of the number of times it has entered
//! hibernation.  The value of the counter is stored in the battery backed
//! memory of the Hibernation module so that it can be retrieved when the
//! microcontroller wakes.  The program displays the wall time and date by
//! making use of the calendar function of the Hibernate module.  User can
//! modify the date and time if so desired.
//
//*****************************************************************************

//*****************************************************************************
//
// A collection of wake sources that will be displayed to indicate the source
// of the most recent wake.
//
//*****************************************************************************
static char *g_ppcWakeSource[] =
{
    "RTC TIMEOUT",
    "RESET",
    "WAKE PIN",
    "GPIO WAKE",
    "SYSTEM RESET"
};

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
// Flag to indicate that main screen is active so that main screen buffers can
// be updated and widgets redrawn.
//
//*****************************************************************************
bool g_bMainScreen;

//*****************************************************************************
//
// Flag that informs that the user has requested hibernation.
//
//*****************************************************************************
volatile bool g_bHibernate;

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
char g_pcWakeBuf[40], g_pcHibBuf[40], g_pcDateTimeBuf[40];
char g_pcInfoBuf0[40], g_pcInfoBuf1[40], g_pcInfoBuf2[40], g_pcInfoBuf3[40];
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
// Forward reference for widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sMainScreen;
extern tCanvasWidget g_sWakeInfo, g_sHibCount, g_sDateTime;
extern tPushButtonWidget g_sInfoStrBtn;
extern tCanvasWidget g_sInfoStr1, g_sInfoStr2, g_sInfoStr3;
extern tPushButtonWidget g_sDateTimeSetBtn;
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
void OnInfoStrBtnPress(tWidget *psWidget);
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
// The graphics library structures for the Main screen.
//
//*****************************************************************************
RectangularButton(g_sDateTimeSetBtn, &g_sMainScreen, 0, 0,
                  &g_sKentec320x240x16_SSD2119, (319 - 9 - 95), 34, 95, 45,
                  (PB_STYLE_TEXT | PB_STYLE_FILL | PB_STYLE_OUTLINE |
                   PB_STYLE_TEXT_OPAQUE), ClrDarkBlue, ClrBlue, 0, ClrWhite,
                  g_psFontCm16, "DATE/TIME", 0, 0, 0, 0,
                  OnDateTimeSetBtnPress);
Canvas(g_sDateTime, &g_sMainScreen, &g_sDateTimeSetBtn, 0,
       &g_sKentec320x240x16_SSD2119, 9, 50, (310 - 9 - 100), 17,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT), ClrBlack, 0, ClrWhite,
       g_psFontCm16, g_pcDateTimeBuf, 0, 0);
Canvas(g_sWakeInfo, &g_sMainScreen, &g_sDateTime, 0,
       &g_sKentec320x240x16_SSD2119, 9, 100, (310 - 9), 17, CANVAS_STYLE_TEXT,
       ClrBlack, 0, ClrWhite, g_psFontCm16, g_pcWakeBuf, 0, 0);
Canvas(g_sHibCount, &g_sMainScreen, &g_sWakeInfo, 0,
       &g_sKentec320x240x16_SSD2119, 9, 120, (310 - 9), 17, CANVAS_STYLE_TEXT,
       ClrBlack, 0, ClrWhite, g_psFontCm16, g_pcHibBuf, 0, 0);
RectangularButton(g_sInfoStrBtn, &g_sMainScreen, &g_sHibCount, 0,
                  &g_sKentec320x240x16_SSD2119, 9, 150, (310 - 9), 25,
                  (PB_STYLE_TEXT | PB_STYLE_FILL | PB_STYLE_OUTLINE |
                   PB_STYLE_TEXT_OPAQUE), ClrDarkBlue, ClrBlue, 0, ClrWhite,
                  g_psFontCm16, g_pcInfoBuf0, 0, 0, 0, 0, OnInfoStrBtnPress);
Canvas(g_sInfoStr1, &g_sMainScreen, &g_sInfoStrBtn, 0,
       &g_sKentec320x240x16_SSD2119, 9, 175, (310 - 9), 17,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT), ClrBlack, 0, ClrWhite,
       g_psFontCm16, g_pcInfoBuf1, 0, 0);
Canvas(g_sInfoStr2, &g_sMainScreen, &g_sInfoStr1, 0,
       &g_sKentec320x240x16_SSD2119, 9, 192, (310 - 9), 17,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT), ClrBlack, 0, ClrWhite,
       g_psFontCm16, g_pcInfoBuf2, 0, 0);
Canvas(g_sInfoStr3, &g_sMainScreen, &g_sInfoStr2, 0,
       &g_sKentec320x240x16_SSD2119, 9, 209, (310 - 9), 17,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT), ClrBlack, 0, ClrWhite,
       g_psFontCm16, g_pcInfoBuf3, 0, 0);
Canvas(g_sMainScreen, WIDGET_ROOT, 0, &g_sInfoStr3,
       &g_sKentec320x240x16_SSD2119, 9, 25, (310 - 9), (230 - 25),
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

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
       &g_sKentec320x240x16_SSD2119, 9, 25, (310 - 9), (230 - 25),
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
// This function reads the current date and time from the calendar logic of the
// hibernate module.  Return status indicates the validity of the data read.
// If the received data is valid, the 24-hour time format is converted to
// 12-hour format.
//
//*****************************************************************************
bool
DateTimeGet(struct tm *sTime, char *pcAMPM)
{
    //
    // Get the latest time.
    //
    HibernateCalendarGet(sTime);

    //
    // Is valid data read?
    //
    if(((sTime->tm_sec < 0) || (sTime->tm_sec > 59)) ||
       ((sTime->tm_min < 0) || (sTime->tm_min > 59)) ||
       ((sTime->tm_hour < 0) || (sTime->tm_hour > 23)) ||
       ((sTime->tm_mday < 1) || (sTime->tm_mday > 31)) ||
       ((sTime->tm_mon < 0) || (sTime->tm_mon > 11)) ||
       ((sTime->tm_year < 100) || (sTime->tm_year > 199)))
    {
        //
        // No - Let the application know the same by returning relevant
        // message.
        //
        return false;
    }

    //
    // Set AM as default.
    //
    strcpy(pcAMPM, "AM");

    //
    // Convert 24-hour format into 12-hour format with AM/PM indication.
    //
    if(sTime->tm_hour == 0)
    {
        sTime->tm_hour = 12;
        strcpy(pcAMPM, "AM");
    }
    else if(sTime->tm_hour == 12)
    {
        strcpy(pcAMPM, "PM");
    }
    else if(sTime->tm_hour > 12)
    {
        sTime->tm_hour -= 12;
        strcpy(pcAMPM, "PM");
    }

    //
    // Return that new data is available so that it can be displayed.
    //
    return true;
}

//*****************************************************************************
//
// This function formats valid new date and time to be displayed on the home
// screen in the format "MMM DD, YYYY  HH : MM : SS AM/PM".  Example of this
// format is Aug 01, 2013  08:15:30 AM.  It also indicates if valid new data
// is available or not.  If date and time is invalid, this function sets the
// date and time to default value.
//
//*****************************************************************************
bool
DateTimeDisplayGet(char *pcBuf, uint32_t ui32BufSize)
{
    static uint32_t ui32SecondsPrev = 0xFF;
    struct tm sTime;
    char pcAMPM[3];
    uint32_t ui32Len;

    //
    // Get the latest date and time and check the validity.
    //
    if(DateTimeGet(&sTime, pcAMPM) == false)
    {
        //
        // Invalid - Force set the date and time to default values and return
        // false to indicate no information to display.
        //
        g_bSetDate = true;
        return false;
    }

    //
    // If date and time is valid, check if seconds have updated from previous
    // visit.
    //
    if(ui32SecondsPrev == sTime.tm_sec)
    {
        //
        // No - Return false to indicate no information to display.
        //
        return false;
    }

    //
    // If valid new date and time is available, update a local variable to keep
    // track of seconds to determine new data for next visit.
    //
    ui32SecondsPrev = sTime.tm_sec;

    //
    // Format the date and time into a user readable format.
    //
    ui32Len = usnprintf(pcBuf, ui32BufSize, "%s %02u, 20%02u  ",
                        g_ppcMonth[sTime.tm_mon], sTime.tm_mday,
                        sTime.tm_year - 100);
    usnprintf(&pcBuf[ui32Len], ui32BufSize - ui32Len, "%02u : %02u : %02u %s",
              sTime.tm_hour, sTime.tm_min, sTime.tm_sec, pcAMPM);

    //
    // Return true to indicate new information to display.
    //
    return true;
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

    //
    // Get the latest date and time and check the validity.
    //
    if(DateTimeGet(&sTime, pcAMPM) == false)
    {
        //
        // Invalid - Return here with false as no information to update.  So
        // use default values.
        //
        return false;
    }

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
// This function returns the date and time value that is written to the
// calendar match register.  5 seconds are added to the current time.  Any
// side-effects due to this addition are handled here.
//
//*****************************************************************************
void
GetCalendarMatchValue(struct tm* sTime)
{
    uint32_t ui32MonthDays;

    //
    // Get the current date and time and add 5 secs to it.
    //
    HibernateCalendarGet(sTime);
    sTime->tm_sec += 5;

    //
    // Check if seconds is out of bounds.  If so subtract seconds by 60 and
    // increment minutes.
    //
    if(sTime->tm_sec > 59)
    {
        sTime->tm_sec -= 60;
        sTime->tm_min++;
    }

    //
    // Check if minutes is out of bounds.  If so subtract minutes by 60 and
    // increment hours.
    //
    if(sTime->tm_min > 59)
    {
        sTime->tm_min -= 60;
        sTime->tm_hour++;
    }

    //
    // Check if hours is out of bounds.  If so subtract minutes by 24 and
    // increment days.
    //
    if(sTime->tm_hour > 23)
    {
        sTime->tm_hour -= 24;
        sTime->tm_mday++;
    }

    //
    // Since different months have varying number of days, get the number of
    // days for the current month and year.
    //
    ui32MonthDays = GetDaysInMonth(sTime->tm_year, sTime->tm_mon);

    //
    // Check if days is out of bounds for the current month and year.  If so
    // subtract days by the number of days in the current month and increment
    // months.
    //
    if(sTime->tm_mday > ui32MonthDays)
    {
        sTime->tm_mday -= ui32MonthDays;
        sTime->tm_mon++;
    }

    //
    // Check if months is out of bounds.  If so subtract months by 11 and
    // increment years.
    //
    if(sTime->tm_mon > 11)
    {
        sTime->tm_mon -= 11;
        sTime->tm_year++;
    }

    //
    // Check if years is out of bounds.  If so subtract years by 100.
    //
    if(sTime->tm_year > 99)
    {
        sTime->tm_year -= 100;
    }
}
//*****************************************************************************
//
// Handle the Info String button press on Main Screen.
//
//*****************************************************************************
void
OnInfoStrBtnPress(tWidget *psWidget)
{
    //
    // If this button is pressed then user wants to enter hibernation.  So
    // set the respective flag.
    //
    g_bHibernate = true;
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
    // Remove the main screen from the root.
    //
    WidgetRemove((tWidget *)&g_sTimeScreen);

    //
    // Add the date screen to the root.
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
// This example demonstrates the different hibernate wake sources.  The
// microcontroller is put into hibernation by the user and wakes up based on
// timeout or one of the user inputs.  This example also demonstrates the RTC
// calendar function that keeps track of date and time.
//
//*****************************************************************************
int
main(void)
{
    tContext sContext;
    struct tm sTime;
    bool bUpdate;
    uint32_t ui32SysClock, ui32Status, ui32HibernateCount, ui32Len;

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
    FrameDraw(&sContext, "hibernate");

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&sContext);

    //
    // Set a font henceforth.
    //
    GrContextFontSet(&sContext, g_psFontCm16);

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(ui32SysClock);
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sMainScreen);

    //
    // Enable the hibernate module.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);

    //
    // Initialize these variables before they are used.
    //
    ui32Status = 0;
    ui32HibernateCount = 0;

    //
    // Check to see if Hibernation module is already active, which could mean
    // that the processor is waking from a hibernation.
    //
    if(HibernateIsActive())
    {
        //
        // Read the status bits to see what caused the wake.  Clear the wake
        // source so that the device can be put into hibernation again.
        //
        ui32Status = HibernateIntStatus(0);
        HibernateIntClear(ui32Status);

        //
        // Store the common part of the wake information message into a buffer.
        // The wake source will be appended based on the status bits.
        //
        ui32Len = usnprintf(g_pcWakeBuf, sizeof(g_pcWakeBuf),
                            "Wake Due To : ");

        //
        // Wake was due to RTC match.
        //
        if(ui32Status & HIBERNATE_INT_RTC_MATCH_0)
        {
            ui32Len = usnprintf(&g_pcWakeBuf[ui32Len],
                                sizeof(g_pcWakeBuf) - ui32Len, "%s",
                                g_ppcWakeSource[0]);
        }

        //
        // Wake was due to Reset button.
        //
        else if(ui32Status & HIBERNATE_INT_RESET_WAKE)
        {
            ui32Len = usnprintf(&g_pcWakeBuf[ui32Len],
                                sizeof(g_pcWakeBuf) - ui32Len, "%s",
                                g_ppcWakeSource[1]);
        }

        //
        // Wake was due to the External Wake pin.
        //
        else if(ui32Status & HIBERNATE_INT_PIN_WAKE)
        {
            ui32Len = usnprintf(&g_pcWakeBuf[ui32Len],
                                sizeof(g_pcWakeBuf) - ui32Len, "%s",
                                g_ppcWakeSource[2]);
        }

        //
        // Wake was due to GPIO wake.
        //
        else if(ui32Status & HIBERNATE_INT_GPIO_WAKE)
        {
            ui32Len = usnprintf(&g_pcWakeBuf[ui32Len],
                                sizeof(g_pcWakeBuf) - ui32Len, "%s",
                                g_ppcWakeSource[3]);
        }

        //
        // If the wake is due to any of the configured wake sources, then read
        // the first location from the battery backed memory, as the
        // hibernation count.
        //
        if(ui32Status & (HIBERNATE_INT_PIN_WAKE | HIBERNATE_INT_RTC_MATCH_0 |
                         HIBERNATE_INT_GPIO_WAKE | HIBERNATE_INT_RESET_WAKE))
        {
            HibernateDataGet(&ui32HibernateCount, 1);
        }
    }

    //
    // Configure Hibernate module clock.
    //
    HibernateEnableExpClk(ui32SysClock);

    //
    // If the wake was not due to the above sources, then it was a system
    // reset.
    //
    if(!(ui32Status & (HIBERNATE_INT_PIN_WAKE | HIBERNATE_INT_RTC_MATCH_0 |
                       HIBERNATE_INT_GPIO_WAKE | HIBERNATE_INT_RESET_WAKE)))
    {
        //
        // Configure the module clock source.
        //
        HibernateClockConfig(HIBERNATE_OSC_LOWDRIVE);

        //
        // Store that this was a system restart, not wake from hibernation.
        //
        ui32Len = usnprintf(g_pcWakeBuf, sizeof(g_pcWakeBuf), "%s",
                            g_ppcWakeSource[4]);

        //
        // Set Calendar time to start from 08/29/2013 at 8:30AM
        //
        g_ui32MonthIdx = 7;
        g_ui32DayIdx = 29;
        g_ui32YearIdx = 13;
        g_ui32HourIdx = 8;
        g_ui32MinIdx = 30;
        strcpy(g_pcMonBuf, "AUG");
        strcpy(g_pcDayBuf, "29");
        strcpy(g_pcYearBuf, "2013");
        strcpy(g_pcHourBuf, "8");
        strcpy(g_pcMinBuf, "30");
        strcpy(g_pcAMPMBuf, "AM");
    }

    //
    // Store the hibernation count message into the respective widget buffer.
    //
    usnprintf(g_pcHibBuf, sizeof(g_pcHibBuf), "Hibernate count = %u",
              ui32HibernateCount);

    //
    // Store the text, that informs the user on how to hibernate, into the
    // respective widget buffer.
    //
    usnprintf(g_pcInfoBuf0, sizeof(g_pcInfoBuf0), "To hibernate touch HERE.");

    //
    // Add the widget tree to the message queue so that it can be drawn.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Enable RTC mode.
    //
    HibernateRTCEnable();

    //
    // Configure the hibernate module counter to 24-hour calendar mode.
    //
    HibernateCounterMode(HIBERNATE_COUNTER_24HR);

    //
    // Configure GPIOs used as Hibernate wake source
    //
    GPIOPadConfigSet(GPIO_PORTK_BASE, 0x20, GPIO_STRENGTH_2MA,
                     (GPIO_PIN_TYPE_WAKE_LOW | GPIO_PIN_TYPE_STD_WPU));

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // If hibernation count is very large, it may be that there was already
    // a value in the hibernate memory, so reset the count.
    //
    ui32HibernateCount = (ui32HibernateCount > 10000) ? 0 : ui32HibernateCount;

    //
    // Increment the hibernation count, and store it in the battery backed
    // memory.
    //
    ui32HibernateCount++;
    HibernateDataSet(&ui32HibernateCount, 1);

    //
    // Initialize the necessary flags before entering indefinite loop.
    //
    g_bMainScreen = true;
    g_bHibernate = false;
    g_bSetDate = false;

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Process any messages for/from the widgets.
        //
        WidgetMessageQueueProcess();

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

        //
        // Check if main screen is active and update the required buffers and
        // redraw the required widgets.
        //
        if(g_bMainScreen)
        {
            //
            // Update the buffer that displays date and time on the main
            // screen.
            //
            bUpdate = DateTimeDisplayGet(g_pcDateTimeBuf,
                                         sizeof(g_pcDateTimeBuf));

            //
            // Is a new value of date and time available to be displayed?
            //
            if(bUpdate == true)
            {
                //
                // Yes - Redraw the widget.
                //
                WidgetPaint(&g_sDateTime.sBase);
            }
        }

        //
        // Check if user wants to enter hibernation.
        //
        if(g_bHibernate == true)
        {
            //
            // Yes - Clear the flag.
            //
            g_bHibernate = false;

            //
            // Store the text, that informs the user on how to wake-up from
            // hibernation, into the respective widget buffers.
            //
            usnprintf(g_pcInfoBuf1, sizeof(g_pcInfoBuf1), "To wake up wait "
                      "for 5 secs or press the");
            usnprintf(g_pcInfoBuf2, sizeof(g_pcInfoBuf2), "RESET button.  "
                      "Refer document for");
            usnprintf(g_pcInfoBuf3, sizeof(g_pcInfoBuf3), "additional "
                      "wake sources.");

            //
            // Redraw the widgets to display the updated buffers.  These
            // widgets have to be redrawn here instead of adding to the message
            // queue by using WidgetPaint() function as
            // WidgetMessageQueueProcess() function won't be executed before
            // entering hibernation.
            //
            CanvasMsgProc(&g_sInfoStr1.sBase, WIDGET_MSG_PAINT, 0, 0);
            CanvasMsgProc(&g_sInfoStr2.sBase, WIDGET_MSG_PAINT, 0, 0);
            CanvasMsgProc(&g_sInfoStr3.sBase, WIDGET_MSG_PAINT, 0, 0);

            //
            // Get calendar match value to be 5 seconds from the current time.
            //
            GetCalendarMatchValue(&sTime);

            //
            // Set the calendar match register such that it wakes up from
            // hibernation in 5 seconds.
            //
            HibernateCalendarMatchSet(0, &sTime);

            //
            // Read and clear any status bits that might have been set since
            // last clearing them.
            //
            ui32Status = HibernateIntStatus(0);
            HibernateIntClear(ui32Status);

            //
            // Configure Hibernate wake sources.
            //
            HibernateWakeSet(HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_GPIO |
                             HIBERNATE_WAKE_RESET | HIBERNATE_WAKE_RTC);

            //
            // Request Hibernation.
            //
            HibernateRequest();

            //
            // Wait for a while for hibernate to activate.  It should never get
            // past this point.
            //
            SysCtlDelay(100);

            //
            // If it ever gets here, store the text, that informs the user on
            // what to do, into the respective widget buffers.
            //
            usnprintf(g_pcInfoBuf1, sizeof(g_pcInfoBuf1), "The controller did "
                      "not enter hibernate mode.");
            usnprintf(g_pcInfoBuf2, sizeof(g_pcInfoBuf2), "TOUCH THE DISPLAY "
                      "TO RESTART.");
            usnprintf(g_pcInfoBuf3, sizeof(g_pcInfoBuf3), "");

            //
            // Redraw the widgets to display the updated buffers.
            //
            CanvasMsgProc(&g_sInfoStr1.sBase, WIDGET_MSG_PAINT, 0, 0);
            CanvasMsgProc(&g_sInfoStr2.sBase, WIDGET_MSG_PAINT, 0, 0);
            CanvasMsgProc(&g_sInfoStr3.sBase, WIDGET_MSG_PAINT, 0, 0);

            //
            // Wait for the user request to reset processor.  Then reset the
            // processor.
            //
            g_bHibernate = false;
            while(g_bHibernate != true)
            {
            }

            //
            // Reset the processor.
            //
            ROM_SysCtlReset();

            //
            // Wait here.
            //
            while(1)
            {
            }
        }
    }
}
