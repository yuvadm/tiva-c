//*****************************************************************************
//
// qs_weather.c - Sample Weather application using lwIP.
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
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_gpio.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/emac.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/keyboard.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"
#include "utils/locator.h"
#include "utils/flash_pb.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "eth_client.h"
#include "images.h"
#include "json.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Quickstart Weather Application (qs_weather)</h1>
//!
//! This example application demonstrates the operation of the Tiva C series
//! evaluation kit as a weather reporting application.
//!
//! The application supports updating weather information from Open Weather Map
//! weather provider(http://openweathermap.org/).  The application uses the
//! lwIP stack to obtain an address through DNS, resolve the address of the
//! Open Weather Map site and then build and handle all of the requests
//! necessary to access the weather information.  The application can also use
//! a web proxy, allows for a custom city to be added to the list of cities and
//! toggles temperature units from Celsius to Fahrenheit.  The application uses
//! gestures to navigate between various screens.  To open the settings screen
//! just press and drag down on any city screen.  To exit the setting screen
//! press and drag up and you are returned to the main city display.  To
//! navigate between cities, press and drag left or right and the new city
//! information is displayed.
//!
//! For additional details about Open Weather Map refer to their web page at:
//! http://openweathermap.org/
//!
//! For additional details on lwIP, refer to the lwIP web page at:
//! http://savannah.nongnu.org/projects/lwip/
//
//*****************************************************************************

//*****************************************************************************
//
// Defines for setting up the system tick clock.
//
//*****************************************************************************
#define SYSTEM_TICK_MS          10
#define SYSTEM_TICK_S           100

//*****************************************************************************
//
// Define the size of the flash program blocks for saving configuration data.
//
//*****************************************************************************
#define FLASH_PB_START          0x40000
#define FLASH_PB_END            FLASH_PB_START + 0x4000

//*****************************************************************************
//
// The animation delay passed to SysCtlDelay().
//
//*****************************************************************************
#define SCREEN_ANIMATE_DELAY    0x10000

//*****************************************************************************
//
// Minimum change to be a swipe action.
//
//*****************************************************************************
#define SWIPE_MIN_DIFF          40

//*****************************************************************************
//
// Connection states for weather application.
//
//*****************************************************************************
volatile enum
{
    STATE_NOT_CONNECTED,
    STATE_NEW_CONNECTION,
    STATE_CONNECTED_IDLE,
    STATE_WAIT_DATA,
    STATE_UPDATE_CITY,
    STATE_WAIT_NICE,
}
g_iState = STATE_NOT_CONNECTED;

//*****************************************************************************
//
// The city being displayed.
//
//*****************************************************************************
uint32_t g_ui32CityActive;

//*****************************************************************************
//
// The city being updated.
//
//*****************************************************************************
uint32_t g_ui32CityUpdating;

//*****************************************************************************
//
// The delay count to reduce traffic to the weather server.
//
//*****************************************************************************
volatile uint32_t g_ui32Delay;

//*****************************************************************************
//
// Screen saver timeout.
//
//*****************************************************************************
volatile uint32_t g_ui32ScreenSaver;

//*****************************************************************************
//
// State information for the toggle buttons used in the settings panel.
//
//*****************************************************************************
typedef struct
{
    //
    // The outside area of the button.
    //
    tRectangle sRectContainer;

    //
    // The actual button area.
    //
    tRectangle sRectButton;

    //
    // The text for the on position.
    //
    const char *pcOn;

    //
    // The text for the off position.
    //
    const char *pcOff;

    //
    // The label for the button.
    //
    const char *pcLabel;
}
tButtonToggle;

//
// System clock rate in Hz.
//
uint32_t g_ui32SysClock;

//
// Global graphic context for the application.
//
tContext g_sContext;

//*****************************************************************************
//
// Storage for the filename list box widget string table.
//
//*****************************************************************************
#define NUM_CITIES              30

//*****************************************************************************
//
// The flash parameter block structure.
//
//*****************************************************************************
typedef struct
{
    //
    // Reserved space used by the flash program block code.
    //
    uint32_t ui32PBReserved;

    //
    // The custom city name.
    //
    char pcCustomCity[60];

    //
    // The web proxy name.
    //
    char pcProxy[80];

    //
    // The current temperature unit setting.
    //
    bool bCelsius;

    //
    // Current enable/disable setting for the custom city.
    //
    bool bCustomEnabled;

    //
    // Current enable/disable setting for the proxy.
    //
    bool bProxyEnabled;

    //
    // Indicates whether the current settings have been saved.
    //
    bool bSave;
}
sParameters;

//*****************************************************************************
//
// The defaults for the flash and application settings.
//
//*****************************************************************************
static const sParameters g_sDefaultParams =
{
    0,
    {"Custom City Name"},
    {"your.proxy.com"},
    true,
    false,
    false,
    false
};

//
// The current live configuration settings for the application.
//
static sParameters g_sConfig;

//*****************************************************************************
//
// The state of each city panel.
//
//*****************************************************************************
struct
{
    //
    // The last update time for this city.
    //
    uint32_t ui32LastUpdate;

    //
    // The current weather report data for this city.
    //
    tWeatherReport sReport;

    //
    // Indicates if the city needs updating.
    //
    bool bNeedsUpdate;

    //
    // The name of the current city.
    //
    const char *pcName;
}
g_psCityInfo[NUM_CITIES];

//
// The list of city names.
//
static const char *g_ppcCityNames[NUM_CITIES - 1] =
{
    "Austin, TX",
    "Beijing, China",
    "Berlin, Germany",
    "Boston, MA",
    "Buenos Aires, Argentina",
    "Chicago, IL",
    "Dallas, TX",
    "Frankfurt, Germany",
    "Hong Kong, HK",
    "Jerusalem, Israel",
    "Johannesburg, ZA",
    "London, England",
    "Mexico City, Mexico",
    "Moscow, Russia",
    "New Delhi, India",
    "New York, NY",
    "Paris, France",
    "Rome, Italy",
    "San Jose, CA",
    "Sao Paulo, Brazil",
    "Seoul, S. Korea",
    "Shanghai, China",
    "Shenzhen, China",
    "Singapore City, Singapore",
    "Sydney, Australia",
    "Taipei, Taiwan",
    "Tokyo, Japan",
    "Toronto, Canada",
    "Vancouver, Canada"
};

//*****************************************************************************
//
// Constant strings for status messages.
//
//*****************************************************************************
const char g_pcNotFound[] = "City Not Found";
const char g_pcServerBusy[] = "Server Busy";
const char g_pcWaitData[] = "Waiting for Data";

//*****************************************************************************
//
// Interrupt priority definitions.  The top 3 bits of these values are
// significant with lower values indicating higher priority interrupts.
//
//*****************************************************************************
#define SYSTICK_INT_PRIORITY    0x80
#define ETHERNET_INT_PRIORITY   0xC0

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
// Pre-declaration of the functions used by the graphical objects.
//
//*****************************************************************************
static void UpdateCity(uint32_t ui32Idx, bool bDraw);
static void ProxyEnable(tWidget *psWidget);
static void CustomEnable(tWidget *psWidget);
static void DrawToggle(const tButtonToggle *psButton, bool bOn);
static void OnProxyEntry(tWidget *psWidget);
static void OnCustomEntry(tWidget *psWidget);
static void OnTempUnit(tWidget *psWidget);
static void KeyEvent(tWidget *psWidget, uint32_t ui32Key, uint32_t ui32Event);

//*****************************************************************************
//
// Defines for the basic screen area used by the application.
//
//*****************************************************************************
#define BG_MIN_X                8
#define BG_MAX_X                (320 - 8)
#define BG_MIN_Y                24
#define BG_MAX_Y                (240 - 8)
#define BG_COLOR_SETTINGS       ClrGray
#define BG_COLOR_MAIN           ClrBlack

//*****************************************************************************
//
// The canvas widget acting as the background to the left portion of the
// display.
//
//*****************************************************************************
extern tCanvasWidget g_sMainBackground;

char g_pcTempHighLow[40]="--/--C";
Canvas(g_sTempHighLow, &g_sMainBackground, 0, 0,
       &g_sKentec320x240x16_SSD2119, 120, 195, 70, 30,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_TOP | CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN,
       ClrWhite, ClrWhite, g_psFontCmss20, g_pcTempHighLow, 0, 0);

char g_pcTemp[40]="--C";
Canvas(g_sTemp, &g_sMainBackground, &g_sTempHighLow, 0,
       &g_sKentec320x240x16_SSD2119, 20, 175, 100, 50,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_TOP | CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN,
       ClrWhite, ClrWhite, g_psFontCmss48, g_pcTemp, 0, 0);

char g_pcHumidity[40]="Humidity: --%";
Canvas(g_sHumidity, &g_sMainBackground, &g_sTemp, 0,
       &g_sKentec320x240x16_SSD2119, 20, 140, 160, 25,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN, ClrWhite, ClrWhite,
       g_psFontCmss20, g_pcHumidity, 0, 0);

char g_pcStatus[40];
Canvas(g_sStatus, &g_sMainBackground, &g_sHumidity, 0,
       &g_sKentec320x240x16_SSD2119, 20, 110, 160, 25,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN, ClrWhite, ClrWhite,
       g_psFontCmss20, g_pcStatus, 0, 0);

char g_pcCity[40];
Canvas(g_sCityName, &g_sMainBackground, &g_sStatus, 0,
       &g_sKentec320x240x16_SSD2119, 20, 40, 240, 25,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN, ClrWhite, ClrWhite,
       g_psFontCmss20, g_pcCity, 0, 0);

Canvas(g_sMainBackground, WIDGET_ROOT, 0, &g_sCityName,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X,
       BG_MAX_Y - BG_MIN_Y, CANVAS_STYLE_FILL,
       BG_COLOR_MAIN, ClrWhite, ClrWhite, g_psFontCmss20,
       0, 0, 0);

//*****************************************************************************
//
// Settings panel graphical variables.
//
//*****************************************************************************
extern tCanvasWidget g_sStatusPanel;

//
// The temperature toggle button.
//
const tButtonToggle sTempToggle =
{
    {12, 90, 164, 117},
    {14, 92, 54, 115},
    "C",
    "F",
    "Temperature"
};

//
// The actual button to receive the press events for temperature unit changes.
//
RectangularButton(g_sTempUnit, &g_sStatusPanel, 0, 0,
                  &g_sKentec320x240x16_SSD2119, 14, 92, 40, 24, 0, ClrDarkGray,
                  ClrDarkGray, ClrDarkGray, ClrDarkGray, 0, 0, 0, 0, 0 ,0 ,
                  OnTempUnit);

//*****************************************************************************
//
// Proxy button and proxy text entry widgets.
//
//*****************************************************************************

//
// The proxy toggle button.
//
const tButtonToggle sProxyToggle =
{
    //
    // Outer border of button.
    //
    {12, 60, 116, 87},

    //
    // Button border.
    //
    {14, 62, 54, 85},

    //
    // Button Text.
    //
    "On",
    "Off",
    "Proxy"
};

//
// The actual button widget that receives the press events.
//
RectangularButton(g_sProxyEnable, &g_sStatusPanel, &g_sTempUnit, 0,
       &g_sKentec320x240x16_SSD2119, 14, 62, 40, 24,
       0, ClrDarkGray, ClrDarkGray, ClrDarkGray,
       ClrDarkGray, 0, 0, 0, 0, 0 ,0 , ProxyEnable);

//
// The button that receives press events for text entry.
//
RectangularButton(g_sProxyAddr, &g_sStatusPanel, &g_sProxyEnable, 0,
       &g_sKentec320x240x16_SSD2119, 118, 60, 190, 28,
       PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY, ClrLightGrey,
       ClrLightGrey, ClrWhite, ClrGray, g_psFontCmss16,
       g_sConfig.pcProxy, 0, 0, 0 ,0 , OnProxyEntry);

//*****************************************************************************
//
// Custom city button and city text entry widgets.
//
//*****************************************************************************

//
// The custom city toggle button.
//
const tButtonToggle sCustomToggle =
{
    //
    // Outer border of button.
    //
    {12, 30, 116, 57},

    //
    // Button border.
    //
    {14, 32, 54, 55},

    "On",
    "Off",
    "City"
};

//
// The actual button widget that receives the press events.
//
RectangularButton(g_sCustomEnable, &g_sStatusPanel, &g_sProxyAddr, 0,
       &g_sKentec320x240x16_SSD2119, 14, 32, 40, 24,
       0, ClrLightGrey, ClrLightGrey, ClrLightGrey,
       ClrBlack, 0, 0, 0, 0, 0 ,0 , CustomEnable);

//
// The text entry button for the custom city.
//
RectangularButton(g_sCustomCity, &g_sStatusPanel, &g_sCustomEnable, 0,
       &g_sKentec320x240x16_SSD2119, 118, 30, 190, 28,
       PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY, ClrLightGrey,
       ClrLightGrey, ClrWhite, ClrGray, g_psFontCmss16,
       g_sConfig.pcCustomCity, 0, 0, 0 ,0 , OnCustomEntry);

//
// MAC Address display.
//
char g_pcMACAddr[40];
Canvas(g_sMACAddr, &g_sStatusPanel, &g_sCustomCity, 0,
       &g_sKentec320x240x16_SSD2119, 12, 180, 147, 20,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT,
       ClrGray, ClrDarkGray, ClrBlack, g_psFontCmss16,
       g_pcMACAddr, 0, 0);

//
// IP Address display.
//
char g_pcIPAddr[20];
Canvas(g_sIPAddr, &g_sStatusPanel, &g_sMACAddr, 0,
       &g_sKentec320x240x16_SSD2119, 12, 200, 147, 20,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT,
       ClrGray, ClrDarkGray, ClrBlack, g_psFontCmss16,
       g_pcIPAddr, 0, 0);

//
// Background of the settings panel.
//
Canvas(g_sStatusPanel, WIDGET_ROOT, 0, &g_sIPAddr,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X, BG_MAX_Y - BG_MIN_Y,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_TOP, ClrGray, ClrWhite, ClrBlack, 0,
       0, 0, 0);

//*****************************************************************************
//
// Keyboard
//
//*****************************************************************************

//
// Keyboard cursor blink rate.
//
#define KEYBOARD_BLINK_RATE     100

//
// The current string pointer for the keyboard.
//
static char *g_pcKeyStr;

//
// The current string index for the keyboard.
//
static uint32_t g_ui32StringIdx;

//
// A place holder string used when nothing is being displayed on the keyboard.
//
static const char g_cTempStr = 0;

//
// The current string width for the keyboard in pixels.
//
static int32_t g_i32StringWidth;

//
// The cursor blink counter.
//
static volatile uint32_t g_ui32CursorDelay;

extern tCanvasWidget g_sKeyboardBackground;

//
// The keyboard widget used by the application.
//
Keyboard(g_sKeyboard, &g_sKeyboardBackground, 0, 0,
         &g_sKentec320x240x16_SSD2119, 8, 90, 300, 140,
         KEYBOARD_STYLE_FILL | KEYBOARD_STYLE_AUTO_REPEAT |
         KEYBOARD_STYLE_PRESS_NOTIFY | KEYBOARD_STYLE_RELEASE_NOTIFY |
         KEYBOARD_STYLE_BG,
         ClrBlack, ClrGray, ClrDarkGray, ClrGray, ClrBlack, g_psFontCmss14,
         100, 100, NUM_KEYBOARD_US_ENGLISH, g_psKeyboardUSEnglish, KeyEvent);

//
// The keyboard text entry area.
//
Canvas(g_sKeyboardText, &g_sKeyboardBackground, &g_sKeyboard, 0,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X, 60,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT,
       ClrBlack, ClrWhite, ClrWhite, g_psFontCmss24, &g_cTempStr, 0 ,0 );

//
// The full background for the keyboard when it is takes over the screen.
//
Canvas(g_sKeyboardBackground, WIDGET_ROOT, 0, &g_sKeyboardText,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X, BG_MAX_Y - BG_MIN_Y,
       CANVAS_STYLE_FILL, ClrBlack, ClrWhite, ClrWhite, 0, 0, 0 ,0 );

//*****************************************************************************
//
// The main control paths for changing screens.
//
//*****************************************************************************
#define NUM_SCREENS             3

//
// Screen index values.
//
#define SCREEN_MAIN             0
#define SCREEN_SETTINGS         1
#define SCREEN_KEYBOARD         2

static struct
{
    tWidget *psWidget;
    uint32_t ui32Up;
    uint32_t ui32Down;
    uint32_t ui32Left;
    uint32_t ui32Right;
}
g_sScreens[NUM_SCREENS] =
{
    {
        (tWidget *)&g_sMainBackground,
        SCREEN_MAIN, SCREEN_SETTINGS, SCREEN_MAIN, SCREEN_MAIN
    },
    {
        (tWidget *)&g_sStatusPanel,
        SCREEN_MAIN, SCREEN_SETTINGS, SCREEN_SETTINGS, SCREEN_SETTINGS
    },
    {
        (tWidget *)&g_sKeyboardBackground,
        SCREEN_KEYBOARD, SCREEN_KEYBOARD, SCREEN_KEYBOARD, SCREEN_KEYBOARD
    }
};

//
// The current active screen index.
//
static uint32_t g_i32ScreenIdx;

//*****************************************************************************
//
// The state of the direction control for the application.
//
//*****************************************************************************
static struct
{
    //
    // The initial touch location.
    //
    int32_t i32InitX;
    int32_t i32InitY;

    //
    // The current movement that was detected.
    //
    enum
    {
        iSwipeUp,
        iSwipeDown,
        iSwipeLeft,
        iSwipeRight,
        iSwipeNone,
    }
    eMovement;

    //
    // Holds if the swipe detection is enabled.
    //
    bool bEnable;
}
g_sSwipe;

//*****************************************************************************
//
// The screen buttons state structure.
//
//*****************************************************************************
struct
{
    //
    // Indicates if an on-screen buttons are enabled.
    //
    bool bEnabled;

    //
    // Indicates if an on-screen buttons can be pressed.
    //
    bool bActive;

    //
    // The X position of an on-screen button.
    //
    int32_t i32X;

    //
    // The Y position of an on-screen button.
    //
    int32_t i32Y;

    //
    // The delay time before the on-screen button is removed.
    //
    volatile uint32_t ui32Delay;
}
g_sButtons;

//*****************************************************************************
//
// Celsius to Fahrenheit conversion.
//
//*****************************************************************************
static int32_t
TempCtoF(int32_t i32Temp)
{
    //
    // Only convert if measurements are not in Celsius.
    //
    if(g_sConfig.bCelsius == false)
    {
        i32Temp = ((i32Temp * 9) / 5) + 32;
    }

    return(i32Temp);
}

//*****************************************************************************
//
// Reset a cities information so that it can be properly updated.
//
//*****************************************************************************
void
ResetCity(uint32_t ui32Idx)
{
    g_psCityInfo[ui32Idx].sReport.i32Pressure = INVALID_INT;
    g_psCityInfo[ui32Idx].sReport.i32Humidity = INVALID_INT;
    g_psCityInfo[ui32Idx].sReport.i32Temp = INVALID_INT;
    g_psCityInfo[ui32Idx].sReport.i32TempHigh = INVALID_INT;
    g_psCityInfo[ui32Idx].sReport.i32TempLow = INVALID_INT;
    g_psCityInfo[ui32Idx].sReport.pcDescription = 0;
    g_psCityInfo[ui32Idx].sReport.pui8Image = 0;
    g_psCityInfo[ui32Idx].sReport.ui32SunRise = 0;
    g_psCityInfo[ui32Idx].sReport.ui32SunSet = 0;
    g_psCityInfo[ui32Idx].sReport.ui32Time = 0;
    g_psCityInfo[ui32Idx].ui32LastUpdate = 0;

    //
    // Make sure to copy the name into the string for a custom city.
    //
    if(ui32Idx == NUM_CITIES - 1)
    {
        //
        // Custom city is in another list.
        //
        g_psCityInfo[ui32Idx].pcName = g_sConfig.pcCustomCity;

        if(g_ui32CityActive == ui32Idx)
        {
            //
            // Update the custom city name.
            //
            ustrncpy(g_pcCity, g_psCityInfo[ui32Idx].pcName, sizeof(g_pcCity));
        }

        //
        // The custom city only needs an update if enabled.
        //
        if(g_sConfig.bCustomEnabled)
        {
            g_psCityInfo[ui32Idx].bNeedsUpdate = true;
        }
    }
    else
    {
        g_psCityInfo[ui32Idx].pcName = g_ppcCityNames[ui32Idx];
        g_psCityInfo[ui32Idx].bNeedsUpdate = true;
    }
}

//*****************************************************************************
//
// Handle keyboard updates.
//
//*****************************************************************************
void
HandleKeyboard(void)
{
    //
    // Nothing to do if the keyboard is not active.
    //
    if(g_i32ScreenIdx != SCREEN_KEYBOARD)
    {
        return;
    }

    //
    // If the mid value is hit then clear the cursor.
    //
    if(g_ui32CursorDelay == KEYBOARD_BLINK_RATE / 2)
    {
        GrContextForegroundSet(&g_sContext, ClrBlack);

        //
        // Keep the counter moving now that the clearing has been handled.
        //
        g_ui32CursorDelay--;
    }
    else if(g_ui32CursorDelay == 0)
    {
        GrContextForegroundSet(&g_sContext, ClrWhite);

        //
        // Reset the blink delay now that the drawing of the cursor has been
        // handled.
        //
        g_ui32CursorDelay = KEYBOARD_BLINK_RATE;
    }
    else
    {
        return;
    }

    //
    // Draw the cursor only if it is time.
    //
    GrLineDrawV(&g_sContext, BG_MIN_X + g_i32StringWidth , BG_MIN_Y + 20,
                BG_MIN_Y + 40);
}

//*****************************************************************************
//
// Draw the pop up buttons on the screen.
//
//*****************************************************************************
static void
DrawButtons(int32_t i32Offset, bool bClear)
{
    static const tRectangle sRectTop =
    {
        140,
        BG_MIN_Y,
        171,
        BG_MIN_Y + 10,
    };
    static const tRectangle sRectRight =
    {
        BG_MAX_X - 11,
        BG_MIN_Y - 20 + ((BG_MAX_Y - BG_MIN_Y) / 2),
        BG_MAX_X,
        BG_MIN_Y - 20 + ((BG_MAX_Y - BG_MIN_Y) / 2) + 40,
    };
    static const tRectangle sRectLeft =
    {
        BG_MIN_X,
        BG_MIN_Y - 20 + ((BG_MAX_Y - BG_MIN_Y) / 2),
        BG_MIN_X + 10,
        BG_MIN_Y - 20 + ((BG_MAX_Y - BG_MIN_Y) / 2) + 40,
    };

    //
    // Only draw if they are enabled.
    //
    if(g_sButtons.bEnabled == false)
    {
        return;
    }

    //
    // Draw the three pop up buttons.
    //
    if(g_i32ScreenIdx == SCREEN_MAIN)
    {
        GrContextForegroundSet(&g_sContext, ClrBlack);
        GrContextBackgroundSet(&g_sContext, ClrGray);

        GrRectFill(&g_sContext, &sRectRight);
        GrRectFill(&g_sContext, &sRectLeft);

        if(bClear == false)
        {
            GrLineDrawH(&g_sContext, 140, 171, BG_MIN_Y + 10 + i32Offset);

            GrImageDraw(&g_sContext, g_pui8DownTabImage, 140,
                        BG_MIN_Y + i32Offset);

            GrTransparentImageDraw(&g_sContext, g_pui8RightImage,
                                   BG_MAX_X - 10 + i32Offset,
                                   BG_MIN_Y - 20 + ((BG_MAX_Y - BG_MIN_Y) / 2),
                                   1);
            GrTransparentImageDraw(&g_sContext, g_pui8LeftImage,
                                   BG_MIN_X - i32Offset,
                                   BG_MIN_Y - 20 + ((BG_MAX_Y - BG_MIN_Y) / 2),
                                   1);
        }
        else
        {
            GrRectFill(&g_sContext, &sRectTop);
        }
    }
    else if(g_i32ScreenIdx == SCREEN_SETTINGS)
    {
        GrContextForegroundSet(&g_sContext, ClrGray);
        GrContextBackgroundSet(&g_sContext, ClrWhite);
        if(bClear == false)
        {
            GrLineDrawH(&g_sContext, 140, 171, BG_MAX_Y - 11 - i32Offset);
            GrImageDraw(&g_sContext, g_pui8UpTabImage, 140,
                        BG_MAX_Y - 10 - i32Offset);
        }
    }
}

//*****************************************************************************
//
// Disable the pop up buttons.
//
//*****************************************************************************
static void
ButtonsDisable(void)
{
    g_sButtons.bEnabled = false;
    g_sButtons.bActive = false;
}

//*****************************************************************************
//
// Draw the weather image icon.
//
//*****************************************************************************
static void
DrawIcon(uint32_t ui32Idx)
{
    if(g_psCityInfo[ui32Idx].sReport.pui8Image)
    {
        if((g_psCityInfo[ui32Idx].sReport.ui32Time >
            g_psCityInfo[ui32Idx].sReport.ui32SunRise) &&
           (g_psCityInfo[ui32Idx].sReport.ui32Time <
            g_psCityInfo[ui32Idx].sReport.ui32SunSet))
        {
            GrTransparentImageDraw(&g_sContext, g_pui8SunImage,
                                   176, 65, 0);

            if(g_psCityInfo[ui32Idx].sReport.pui8Image != g_pui8SunImage)
            {
                GrTransparentImageDraw(&g_sContext,
                                       g_psCityInfo[ui32Idx].sReport.pui8Image,
                                       176, 80, 0);
            }
        }
        else
        {
            GrTransparentImageDraw(&g_sContext, g_pui8MoonImage,
                                   176, 65, 0);
            if(g_psCityInfo[ui32Idx].sReport.pui8Image != g_pui8SunImage)
            {
                GrTransparentImageDraw(&g_sContext,
                                       g_psCityInfo[ui32Idx].sReport.pui8Image,
                                       176, 80, 0);
            }
        }
    }
}

//*****************************************************************************
//
// Handle the animation when switching between screens.
//
//*****************************************************************************
void
AnimatePanel(uint32_t ui32Color)
{
    int32_t i32Idx;

    GrContextForegroundSet(&g_sContext, ui32Color);

    if(g_i32ScreenIdx == SCREEN_SETTINGS)
    {
        for(i32Idx = BG_MIN_Y; i32Idx < BG_MAX_Y; i32Idx++)
        {
            GrLineDrawH(&g_sContext, BG_MIN_X, BG_MAX_X, i32Idx);

            if(i32Idx == 58)
            {
                if(g_sConfig.bCustomEnabled)
                {
                    //
                    // Draw the text black for the custom city when it is
                    // enabled.
                    //
                    PushButtonTextColorSet(&g_sCustomCity, ClrBlack);
                }
                else
                {
                    //
                    // Gray out the text entry for the custom city when it is
                    // disabled.
                    //
                    PushButtonTextColorSet(&g_sCustomCity, ClrGray);
                }
                WidgetPaint((tWidget *)&g_sCustomCity);
                DrawToggle(&sCustomToggle, g_sConfig.bCustomEnabled);
                GrContextForegroundSet(&g_sContext, ui32Color);
                WidgetMessageQueueProcess();
            }
            else if(i32Idx == 88)
            {
                if(g_sConfig.bProxyEnabled)
                {
                    //
                    // Enable text entry area for the proxy text entry.
                    //
                    PushButtonTextColorSet(&g_sProxyAddr, ClrBlack);
                }
                else
                {
                    //
                    // Gray out the text entry area for the proxy text entry.
                    //
                    PushButtonTextColorSet(&g_sProxyAddr, ClrGray);
                }
                DrawToggle(&sProxyToggle, g_sConfig.bProxyEnabled);
                WidgetPaint((tWidget *)&g_sProxyAddr);
                GrContextForegroundSet(&g_sContext, ui32Color);
                WidgetMessageQueueProcess();
            }
            else if(i32Idx == 116)
            {
                DrawToggle(&sTempToggle, g_sConfig.bCelsius);
                WidgetPaint((tWidget *)&g_sTempUnit);
                GrContextForegroundSet(&g_sContext, ui32Color);
                WidgetMessageQueueProcess();
            }
            else if(i32Idx == 200)
            {
                WidgetPaint((tWidget *)&g_sMACAddr);
                WidgetMessageQueueProcess();
            }
            else if(i32Idx == 220)
            {
                WidgetPaint((tWidget *)&g_sIPAddr);
                WidgetMessageQueueProcess();
            }

            SysCtlDelay(SCREEN_ANIMATE_DELAY);
        }
    }
    else if(g_i32ScreenIdx == SCREEN_MAIN)
    {
        for(i32Idx = BG_MAX_Y; i32Idx >= BG_MIN_Y; i32Idx--)
        {
            GrLineDrawH(&g_sContext, BG_MIN_X, BG_MAX_X, i32Idx);

            if(i32Idx == 175)
            {
                WidgetPaint((tWidget *)&g_sTempHighLow);
                WidgetPaint((tWidget *)&g_sTemp);
                WidgetMessageQueueProcess();
            }
            else if(i32Idx == 140)
            {
                WidgetPaint((tWidget *)&g_sHumidity);
                WidgetMessageQueueProcess();
            }
            else if(i32Idx == 110)
            {
                WidgetPaint((tWidget *)&g_sStatus);
                WidgetMessageQueueProcess();
            }
            else if(i32Idx == 65)
            {
                DrawIcon(g_ui32CityActive);
            }
            else if(i32Idx == 40)
            {
                WidgetPaint((tWidget *)&g_sCityName);
                WidgetMessageQueueProcess();
            }

            SysCtlDelay(SCREEN_ANIMATE_DELAY);
        }
    }
}

//*****************************************************************************
//
// Animate Buttons.
//
//*****************************************************************************
void
AnimateButtons(bool bInit)
{
    if(bInit)
    {
        g_sButtons.i32X = 0;
        g_sButtons.i32Y = 0;
        g_sButtons.bEnabled = true;
        g_sButtons.bActive = false;
        g_sButtons.ui32Delay = 0;
    }
    else if(g_sButtons.bEnabled == false)
    {
        //
        // Just return if the buttons are not on screen.
        //
        return;
    }

    if(g_sButtons.ui32Delay == 0)
    {
        g_sButtons.ui32Delay = 6;

        GrContextForegroundSet(&g_sContext, ClrBlack);
        GrContextBackgroundSet(&g_sContext, ClrGray);

        if((bInit == false) || (g_sButtons.bActive == true))
        {
            //
            // Update the buttons.
            //
            DrawButtons(g_sButtons.i32X - g_sButtons.i32Y, true);

            if(g_sButtons.i32X < 3)
            {
                g_sButtons.i32X++;
            }
            else
            {
                g_sButtons.i32Y++;
            }
        }

        if(g_sButtons.bActive == false)
        {
            //
            // Update the buttons.
            //
            DrawButtons(g_sButtons.i32X - g_sButtons.i32Y, false);

            if(g_sButtons.i32Y >= 3)
            {
                g_sButtons.bActive = true;
                g_sButtons.ui32Delay = 200;
            }
        }
        else if(g_i32ScreenIdx == SCREEN_MAIN)
        {
            ButtonsDisable();
        }
    }
}

//*****************************************************************************
//
// Clears the full screen.
//
//*****************************************************************************
void
ClearScreen(tContext *psContext)
{
    static const tRectangle sRect =
    {
        0,
        0,
        319,
        239,
    };

    GrRectFill(psContext, &sRect);
}

//*****************************************************************************
//
// Clears the main screens background.
//
//*****************************************************************************
void
ClearBackground(tContext *psContext)
{
    static const tRectangle sRect =
    {
        BG_MIN_X,
        BG_MIN_Y,
        BG_MAX_X,
        BG_MAX_Y,
    };

    GrRectFill(psContext, &sRect);
}

//*****************************************************************************
//
// Update the IP address string.
//
//*****************************************************************************
void
UpdateIPAddress(char *pcAddr, uint32_t ipAddr)
{
    uint8_t *pui8Temp = (uint8_t *)&ipAddr;

    if(ipAddr == 0)
    {
        ustrncpy(pcAddr, "IP: ---.---.---.---", sizeof(g_pcIPAddr));
    }
    else
    {
        usprintf(pcAddr,"IP: %d.%d.%d.%d", pui8Temp[0], pui8Temp[1],
                 pui8Temp[2], pui8Temp[3]);
    }

    //
    // If the status panel is active then update the address.
    //
    if(g_i32ScreenIdx == SCREEN_SETTINGS)
    {
        WidgetPaint((tWidget *)&g_sIPAddr);
    }
}

//*****************************************************************************
//
// Handles the proxy select button presses.
//
//*****************************************************************************
void
ProxyEnable(tWidget *psWidget)
{
    //
    // If a city was waiting to be updated then reset its data.
    //
    if(g_iState != STATE_CONNECTED_IDLE)
    {
        ResetCity(g_ui32CityUpdating);
    }

    //
    // Reset the state to not connected.
    //
    g_iState = STATE_NOT_CONNECTED;

    //
    // Toggle the proxy setting.
    //
    if(g_sConfig.bProxyEnabled)
    {
        g_sConfig.bProxyEnabled = false;

        //
        // Reset the IP address on the screen and disable the proxy which
        // resets the network interface and starts DHCP again.
        //
        UpdateIPAddress(g_pcIPAddr, 0);
        EthClientProxySet(0);

        //
        // Gray out the text entry area for the proxy text entry.
        //
        PushButtonTextColorSet(&g_sProxyAddr, ClrGray);
    }
    else
    {
        g_sConfig.bProxyEnabled = true;

        //
        // Enable the proxy which resets the network interface and starts DHCP
        // again.
        //
        EthClientProxySet(g_sConfig.pcProxy);

        //
        // Enable text entry area for the proxy text entry.
        //
        PushButtonTextColorSet(&g_sProxyAddr, ClrBlack);
    }

    //
    // Update the toggle button.
    //
    DrawToggle(&sProxyToggle, g_sConfig.bProxyEnabled);
    WidgetPaint((tWidget *)&g_sProxyAddr);

    //
    // A change was made so update the settings in flash.
    //
    g_sConfig.bSave = true;
}

//*****************************************************************************
//
// Handles the custom enable button presses.
//
//*****************************************************************************
void
CustomEnable(tWidget *psWidget)
{
    //
    // Toggle the custom city toggle button.
    //
    if(g_sConfig.bCustomEnabled)
    {
        g_sConfig.bCustomEnabled = false;

        //
        // Gray out the text entry for the custom city when it is disabled.
        //
        PushButtonTextColorSet(&g_sCustomCity, ClrGray);

        //
        // Reset the custom city data.
        //
        ResetCity(NUM_CITIES - 1);

        if(g_ui32CityActive == (NUM_CITIES - 1))
        {
            //
            // Move to the first city in the list.
            //
            g_ui32CityActive = 0;

            //
            // Since we must be on the settings screen to change this, then
            // just update the city and do not draw it.
            //
            UpdateCity(g_ui32CityActive, false);
        }
    }
    else
    {
        g_sConfig.bCustomEnabled = true;

        //
        // Draw the text black for the custom city when it is enabled.
        //
        PushButtonTextColorSet(&g_sCustomCity, ClrBlack);
    }

    //
    // Update the toggle button.
    //
    DrawToggle(&sCustomToggle, g_sConfig.bCustomEnabled);
    WidgetPaint((tWidget *)&g_sCustomCity);

    //
    // A change was made so update the settings in flash.
    //
    g_sConfig.bSave = true;
}

//*****************************************************************************
//
// Handles when a key is pressed on the keyboard.
//
//*****************************************************************************
void
KeyEvent(tWidget *psWidget, uint32_t ui32Key, uint32_t ui32Event)
{
    switch(ui32Key)
    {
        //
        // Look for a backspace key press.
        //
        case UNICODE_BACKSPACE:
        {
            if(ui32Event == KEYBOARD_EVENT_PRESS)
            {
                if(g_ui32StringIdx != 0)
                {
                    g_ui32StringIdx--;
                    g_pcKeyStr[g_ui32StringIdx] = 0;
                }

                WidgetPaint((tWidget *)&g_sKeyboardText);

                //
                // Save the pixel width of the current string.
                //
                g_i32StringWidth = GrStringWidthGet(&g_sContext, g_pcKeyStr,
                                                    40);
            }
            break;
        }
        //
        // Look for an enter/return key press.  This will exit the keyboard and
        // return to the current active screen.
        //
        case UNICODE_RETURN:
        {
            if(ui32Event == KEYBOARD_EVENT_RELEASE)
            {
                //
                // Get rid of the keyboard widget.
                //
                WidgetRemove(g_sScreens[g_i32ScreenIdx].psWidget);

                //
                // Switch back to the previous screen and add its widget back.
                //
                g_i32ScreenIdx = SCREEN_SETTINGS;
                WidgetAdd(WIDGET_ROOT, g_sScreens[g_i32ScreenIdx].psWidget);

                //
                // If the proxy was disabled and we were modifying the proxy
                // string the re-enable the proxy.
                //
                if((!g_sConfig.bProxyEnabled) &&
                   (g_pcKeyStr == g_sConfig.pcProxy))
                {
                    //
                    // Enable the proxy since it has just been updated.  This
                    // also restarts DHCP for a new address.
                    //
                    g_sConfig.bProxyEnabled = true;
                    EthClientProxySet(g_sConfig.pcProxy);
                }
                else if((!g_sConfig.bCustomEnabled) &&
                        (g_pcKeyStr == g_sConfig.pcCustomCity))
                {
                    //
                    // If the custom city string was being modified then make
                    // sure to update it.
                    //
                    g_sConfig.bCustomEnabled = true;

                    //
                    // Reset the city data.
                    //
                    ResetCity(NUM_CITIES - 1);

                    //
                    // Update the city with the reset values.
                    //
                    if(g_ui32CityActive == (NUM_CITIES - 1))
                    {
                        UpdateCity(g_ui32CityActive, false);
                    }
                }

                //
                // If returning to the main screen then re-draw the frame to
                // indicate the main screen.
                //
                if(g_i32ScreenIdx == SCREEN_MAIN)
                {
                    FrameDraw(&g_sContext, "qs-weather");
                    WidgetPaint(g_sScreens[g_i32ScreenIdx].psWidget);
                }
                else
                {
                    //
                    // Returning to the settings screen.
                    //
                    FrameDraw(&g_sContext, "Settings");
                    WidgetPaint(g_sScreens[g_i32ScreenIdx].psWidget);
                    AnimateButtons(true);
                    WidgetMessageQueueProcess();

                    //
                    // Redraw all the toggle buttons.
                    //
                    DrawToggle(&sTempToggle, g_sConfig.bCelsius);
                    DrawToggle(&sProxyToggle, g_sConfig.bProxyEnabled);
                    DrawToggle(&sCustomToggle, g_sConfig.bCustomEnabled);
                }

                //
                // Enable gestures.
                //
                g_sSwipe.bEnable = true;
            }
            break;
        }
        //
        // If the key is not special then update the text string.
        //
        default:
        {
            if(ui32Event == KEYBOARD_EVENT_PRESS)
            {
                //
                // If the proxy is enabled and we get a key stroke then disable
                // the proxy and wait for the new proxy string.
                //
                if((g_sConfig.bProxyEnabled) &&
                   (g_pcKeyStr == g_sConfig.pcProxy))
                {
                    //
                    // Disable proxy while updating the proxy string.  This
                    // starts DHCP for a new address.
                    //
                    g_sConfig.bProxyEnabled = false;
                    EthClientProxySet(0);
                }
                if((g_sConfig.bCustomEnabled) &&
                   (g_pcKeyStr == g_sConfig.pcCustomCity))
                {
                    //
                    // Temporarily disable custom city while it is being
                    // modified.
                    //
                    g_sConfig.bCustomEnabled = false;
                }

                //
                // A change was made so update the settings in flash.
                //
                g_sConfig.bSave = true;

                //
                // Set the string to the current string to be updated.
                //
                if(g_ui32StringIdx == 0)
                {
                    CanvasTextSet(&g_sKeyboardText, g_pcKeyStr);
                }
                g_pcKeyStr[g_ui32StringIdx] = (char)ui32Key;
                g_ui32StringIdx++;
                g_pcKeyStr[g_ui32StringIdx] = 0;

                WidgetPaint((tWidget *)&g_sKeyboardText);

                //
                // Save the pixel width of the current string.
                //
                g_i32StringWidth = GrStringWidthGet(&g_sContext, g_pcKeyStr,
                                                    40);
            }
            break;
        }
    }
}

//*****************************************************************************
//
// Draws a toggle button.
//
//*****************************************************************************
void
DrawToggle(const tButtonToggle *psButton, bool bOn)
{
    tRectangle sRect;
    int16_t i16X, i16Y;

    //
    // Copy the data out of the bounds of the button.
    //
    sRect = psButton->sRectButton;

    GrContextForegroundSet(&g_sContext, ClrLightGrey);
    GrRectFill(&g_sContext, &psButton->sRectContainer);

    //
    // Copy the data out of the bounds of the button.
    //
    sRect = psButton->sRectButton;

    GrContextForegroundSet(&g_sContext, ClrDarkGray);
    GrRectFill(&g_sContext, &psButton->sRectButton);

    if(bOn)
    {
        sRect.i16XMin += 2;
        sRect.i16YMin += 2;
        sRect.i16XMax -= 15;
        sRect.i16YMax -= 2;
    }
    else
    {
        sRect.i16XMin += 15;
        sRect.i16YMin += 2;
        sRect.i16XMax -= 2;
        sRect.i16YMax -= 2;
    }
    GrContextForegroundSet(&g_sContext, ClrLightGrey);
    GrRectFill(&g_sContext, &sRect);

    GrContextFontSet(&g_sContext, &g_sFontCm16);
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrContextBackgroundSet(&g_sContext, ClrLightGrey);

    i16X = sRect.i16XMin + ((sRect.i16XMax - sRect.i16XMin) / 2);
    i16Y = sRect.i16YMin + ((sRect.i16YMax - sRect.i16YMin) / 2);

    if(bOn)
    {
        GrStringDrawCentered(&g_sContext, psButton->pcOn, -1, i16X, i16Y,
                             true);
    }
    else
    {
        GrStringDrawCentered(&g_sContext, psButton->pcOff, -1, i16X, i16Y,
                             true);
    }

    if(psButton->pcLabel)
    {
        GrStringDraw(&g_sContext, psButton->pcLabel, -1,
                     psButton->sRectButton.i16XMax + 2,
                     psButton->sRectButton.i16YMin + 6,
                     true);
    }
}

//*****************************************************************************
//
// Handles the temperature unit select button is pressed.
//
//*****************************************************************************
static void
OnTempUnit(tWidget *psWidget)
{
    //
    // Toggle the Celsius state.
    //
    if(g_sConfig.bCelsius)
    {
        g_sConfig.bCelsius = false;

    }
    else
    {
        g_sConfig.bCelsius = true;
    }

    //
    // Update the toggle button and the current city.
    //
    DrawToggle(&sTempToggle, g_sConfig.bCelsius);
    UpdateCity(g_ui32CityActive, false);

    //
    // A change was made so update the settings in flash.
    //
    g_sConfig.bSave = true;
}

//*****************************************************************************
//
// Handles when the proxy text area is pressed.
//
//*****************************************************************************
static void
OnProxyEntry(tWidget *psWidget)
{
    //
    // Only respond if the proxy has been enabled.
    //
    if(g_sConfig.bProxyEnabled)
    {
        //
        // Disable swiping while the keyboard is active.
        //
        g_sSwipe.bEnable = false;

        //
        // The keyboard string is now the proxy address so set the string,
        // reset the string index and width to 0.
        //
        g_pcKeyStr = g_sConfig.pcProxy;
        g_ui32StringIdx = 0;
        g_i32StringWidth = 0;

        //
        // Set the initial string to a null string so that nothing is shown.
        //
        CanvasTextSet(&g_sKeyboardText, &g_cTempStr);

        //
        // Remove the current widget so that it is not used while keyboard
        // is active.
        //
        WidgetRemove(g_sScreens[g_i32ScreenIdx].psWidget);

        //
        // Activate the keyboard.
        //
        g_i32ScreenIdx = SCREEN_KEYBOARD;
        WidgetAdd(WIDGET_ROOT, g_sScreens[g_i32ScreenIdx].psWidget);

        //
        // Clear the main screen area with the settings background color.
        //
        GrContextForegroundSet(&g_sContext, BG_COLOR_SETTINGS);
        ClearBackground(&g_sContext);

        GrContextFontSet(&g_sContext, g_psFontCmss24);
        WidgetPaint((tWidget *)&g_sKeyboardBackground);
    }
}

//*****************************************************************************
//
// Handles when the custom text area is pressed.
//
//*****************************************************************************
static void
OnCustomEntry(tWidget *psWidget)
{
    //
    // Only respond if the custom has been enabled.
    //
    if(g_sConfig.bCustomEnabled)
    {
        //
        // Disable swiping while the keyboard is active.
        //
        g_sSwipe.bEnable = false;

        //
        // The keyboard string is now the custom city so set the string,
        // reset the string index and width to 0.
        //
        g_pcKeyStr = g_sConfig.pcCustomCity;
        g_ui32StringIdx = 0;
        g_i32StringWidth = 0;

        //
        // Set the initial string to a null string so that nothing is shown.
        //
        CanvasTextSet(&g_sKeyboardText, &g_cTempStr);

        //
        // Remove the current widget so that it is not used while keyboard
        // is active.
        //
        WidgetRemove(g_sScreens[g_i32ScreenIdx].psWidget);

        //
        // Activate the keyboard.
        //
        g_i32ScreenIdx = SCREEN_KEYBOARD;
        WidgetAdd(WIDGET_ROOT, g_sScreens[g_i32ScreenIdx].psWidget);

        //
        // Clear the main screen area with the settings background color.
        //
        GrContextForegroundSet(&g_sContext, BG_COLOR_SETTINGS);
        ClearBackground(&g_sContext);

        GrContextFontSet(&g_sContext, g_psFontCmss24);
        WidgetPaint((tWidget *)&g_sKeyboardBackground);
    }
}

//*****************************************************************************
//
// Update the MAC address string.
//
//*****************************************************************************
void
UpdateMACAddr(void)
{
    uint8_t pui8MACAddr[6];

    EthClientMACAddrGet(pui8MACAddr);

    usprintf(g_pcMACAddr, "MAC: %02x:%02x:%02x:%02x:%02x:%02x",
             pui8MACAddr[0], pui8MACAddr[1], pui8MACAddr[2], pui8MACAddr[3],
             pui8MACAddr[4], pui8MACAddr[5]);
}

//*****************************************************************************
//
// The weather event handler.
//
//*****************************************************************************
void
WeatherEvent(uint32_t ui32Event, void* pvData, uint32_t ui32Param)
{
    if(ui32Event == ETH_EVENT_RECEIVE)
    {
        //
        // Let the main loop update the city.
        //
        g_iState = STATE_UPDATE_CITY;

        g_psCityInfo[g_ui32CityUpdating].ui32LastUpdate =
                            g_psCityInfo[g_ui32CityUpdating].sReport.ui32Time;
    }
    else if(ui32Event == ETH_EVENT_INVALID_REQ)
    {
        g_psCityInfo[g_ui32CityUpdating].sReport.pcDescription = g_pcNotFound;
        g_iState = STATE_UPDATE_CITY;
    }
    else if(ui32Event == ETH_EVENT_CLOSE)
    {
        if(g_iState == STATE_WAIT_DATA)
        {
            g_psCityInfo[g_ui32CityUpdating].sReport.pcDescription =
                                                                g_pcServerBusy;

            g_iState = STATE_UPDATE_CITY;
        }
    }

    //
    // If the update indicated no time, then just set the time to a value
    // to indicate that at least the update occurred.
    //
    if(g_psCityInfo[g_ui32CityUpdating].ui32LastUpdate == 0)
    {
        //
        // Failed to update for some reason.
        //
        g_psCityInfo[g_ui32CityUpdating].ui32LastUpdate = 1;
    }
}

//*****************************************************************************
//
// The interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Call the lwIP timer handler.
    //
    EthClientTick(SYSTEM_TICK_MS);

    //
    // Handle the delay between accesses to the weather server.
    //
    if(g_ui32Delay != 0)
    {
        g_ui32Delay--;
    }

    if(g_sButtons.ui32Delay != 0)
    {
        g_sButtons.ui32Delay--;
    }

    //
    // Timeout for the screen saver.
    //
    if(g_ui32ScreenSaver != 0)
    {
        g_ui32ScreenSaver--;
    }

    //
    // Stop updating until the toggle event points have been handled.
    //
    if((g_ui32CursorDelay != 0) &&
       (g_ui32CursorDelay != (KEYBOARD_BLINK_RATE / 2)))
    {
        g_ui32CursorDelay--;
    }
}

//*****************************************************************************
//
// Network events handler.
//
//*****************************************************************************
void
EnetEvents(uint32_t ui32Event, void *pvData, uint32_t ui32Param)
{
    if(ui32Event == ETH_EVENT_CONNECT)
    {
        g_iState = STATE_NEW_CONNECTION;

        //
        // Update the string version of the address.
        //
        UpdateIPAddress(g_pcIPAddr, EthClientAddrGet());
    }
    else if(ui32Event == ETH_EVENT_DISCONNECT)
    {
        //
        // If a city was waiting to be updated then reset its data.
        //
        if(g_iState != STATE_CONNECTED_IDLE)
        {
            ResetCity(g_ui32CityUpdating);
        }

        g_iState = STATE_NOT_CONNECTED;

        //
        // Update the address to 0.0.0.0.
        //
        UpdateIPAddress(g_pcIPAddr, 0);
    }
}

//*****************************************************************************
//
// Update the information for the current city.
//
//*****************************************************************************
void
UpdateCity(uint32_t ui32Idx, bool bDraw)
{
    char cUnits;
    bool bIntDisabled;

    //
    // Need to disable interrupts since this can be called from interrupt
    // handlers for both System tick and Ethernet controller and from the
    // main routine.
    //
    bIntDisabled = IntMasterDisable();

    if(g_sConfig.bCelsius)
    {
        cUnits = 'C';
    }
    else
    {
        cUnits = 'F';
    }

    //
    // Update the city.
    //
    ustrncpy(g_pcCity, g_psCityInfo[ui32Idx].pcName, sizeof(g_pcCity));

    //
    // Check if the humidity value is valid.
    //
    if(g_psCityInfo[ui32Idx].sReport.i32Humidity == INVALID_INT)
    {
        usprintf(g_pcHumidity, "Humidity: --%%");
    }
    else
    {
        usprintf(g_pcHumidity, "Humidity: %d%%",
                 g_psCityInfo[ui32Idx].sReport.i32Humidity);
    }

    //
    // Copy the updated description.
    //
    if(g_psCityInfo[ui32Idx].sReport.pcDescription)
    {
        ustrncpy(g_pcStatus, g_psCityInfo[ui32Idx].sReport.pcDescription,
                 sizeof(g_pcStatus));
    }
    else if((g_ui32CityUpdating == g_ui32CityActive) &&
            (g_iState != STATE_NOT_CONNECTED))
    {
        //
        // Waiting on data for this city.
        //
        ustrncpy(g_pcStatus, g_pcWaitData, sizeof(g_pcStatus));
    }
    else
    {
        //
        // No current status.
        //
        ustrncpy(g_pcStatus, "--", sizeof(g_pcStatus));
    }


    //
    // Check if the temperature value is valid.
    //
    if(g_psCityInfo[ui32Idx].sReport.i32Temp == INVALID_INT)
    {
        usprintf(g_pcTemp, "--%c", cUnits);
        usprintf(g_pcTempHighLow, "--/--%c",
                 TempCtoF(g_psCityInfo[ui32Idx].sReport.i32TempHigh),
                 TempCtoF(g_psCityInfo[ui32Idx].sReport.i32TempLow), cUnits);
    }
    else
    {
        usprintf(g_pcTemp, "%d%c",
                 TempCtoF(g_psCityInfo[ui32Idx].sReport.i32Temp), cUnits);
        usprintf(g_pcTempHighLow, "%d/%d%c",
                 TempCtoF(g_psCityInfo[ui32Idx].sReport.i32TempHigh),
                 TempCtoF(g_psCityInfo[ui32Idx].sReport.i32TempLow), cUnits);
    }

    //
    // Update the screen contents if requested.
    //
    if(bDraw)
    {
        GrContextForegroundSet(&g_sContext, BG_COLOR_MAIN);
        ClearBackground(&g_sContext);
        WidgetPaint((tWidget *)&g_sCityName);
        WidgetPaint((tWidget *)&g_sStatus);
        WidgetPaint((tWidget *)&g_sHumidity);
        WidgetPaint((tWidget *)&g_sTemp);
        WidgetPaint((tWidget *)&g_sTempHighLow);

        DrawIcon(ui32Idx);

        DrawButtons(0, false);
    }

    if(bIntDisabled == false)
    {
        IntMasterEnable();
    }
}

//*****************************************************************************
//
// The callback function that is called by the touch screen driver to indicate
// activity on the touch screen.
//
//*****************************************************************************
int32_t
TouchCallback(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    int32_t i32XDiff, i32YDiff;

    //
    // Reset the timeout for the screen saver.
    //
    g_ui32ScreenSaver =  60 * SYSTEM_TICK_S;

    if(g_sSwipe.bEnable)
    {
        switch(ui32Message)
        {
            //
            // The user has just touched the screen.
            //
            case WIDGET_MSG_PTR_DOWN:
            {
                //
                // Save this press location.
                //
                g_sSwipe.i32InitX = i32X;
                g_sSwipe.i32InitY = i32Y;

                //
                // Done handling this message.
                //
                break;
            }

            //
            // The user has moved the touch location on the screen.
            //
            case WIDGET_MSG_PTR_MOVE:
            {
                //
                // Done handling this message.
                //
                break;
            }

            //
            // The user is no longer touching the screen.
            //
            case WIDGET_MSG_PTR_UP:
            {
                //
                // Indicate that no key is being pressed.
                //
                i32XDiff = i32X - g_sSwipe.i32InitX;
                i32YDiff = i32Y - g_sSwipe.i32InitY;

                //
                // Dead zone for just a button press.
                //
                if(((i32XDiff < SWIPE_MIN_DIFF) &&
                    (i32XDiff > -SWIPE_MIN_DIFF)) &&
                   ((i32YDiff < SWIPE_MIN_DIFF) &&
                    (i32YDiff > -SWIPE_MIN_DIFF)))
                {
                    if(g_sButtons.bActive)
                    {
                        //
                        // Reset the delay.
                        //
                        g_sButtons.ui32Delay = 200;

                        if(i32X < 30)
                        {
                            g_sSwipe.eMovement = iSwipeRight;
                        }
                        else if(i32X > 290)
                        {
                            g_sSwipe.eMovement = iSwipeLeft;
                        }
                        else if(i32Y < 40)
                        {
                            g_sSwipe.eMovement = iSwipeDown;
                        }
                        else if(i32Y > 200)
                        {
                            g_sSwipe.eMovement = iSwipeUp;
                        }
                        else
                        {
                            g_sSwipe.eMovement = iSwipeNone;
                        }
                    }
                    else
                    {
                        if(g_i32ScreenIdx == SCREEN_MAIN)
                        {
                            AnimateButtons(true);
                        }
                    }
                    break;
                }

                //
                // If Y movement dominates then this is an up/down motion.
                //
                if(i32YDiff/i32XDiff)
                {
                    if(i32YDiff < 0)
                    {
                        g_sSwipe.eMovement = iSwipeUp;
                    }
                    else
                    {
                        g_sSwipe.eMovement = iSwipeDown;
                    }
                }
                else
                {
                    if(i32XDiff > 0)
                    {
                        g_sSwipe.eMovement = iSwipeRight;
                    }
                    else
                    {
                        g_sSwipe.eMovement = iSwipeLeft;
                    }
                }

                //
                // Done handling this message.
                //
                break;
            }
        }
    }
    WidgetPointerMessage(ui32Message, i32X, i32Y);

    return(0);
}

//*****************************************************************************
//
// Handle the touch screen movements.
//
//*****************************************************************************
void
HandleMovement(void)
{
    uint32_t ui32NumCities;
    uint32_t ui32NewIdx;

    if(g_sSwipe.eMovement != iSwipeNone)
    {
        if(g_sConfig.bCustomEnabled)
        {
            ui32NumCities = NUM_CITIES;
        }
        else
        {
            ui32NumCities = NUM_CITIES - 1;
        }

        switch(g_sSwipe.eMovement)
        {
            case iSwipeUp:
            {
                ui32NewIdx = g_sScreens[g_i32ScreenIdx].ui32Up;

                break;
            }
            case iSwipeDown:
            {
                ui32NewIdx = g_sScreens[g_i32ScreenIdx].ui32Down;

                break;
            }
            case iSwipeRight:
            {
                ui32NewIdx = g_sScreens[g_i32ScreenIdx].ui32Left;

                //
                // Check if on main screen.
                //
                if(g_i32ScreenIdx == SCREEN_MAIN)
                {
                    if(g_ui32CityActive == 0)
                    {
                        g_ui32CityActive = ui32NumCities - 1;

                    }
                    else
                    {
                        g_ui32CityActive--;
                    }

                    UpdateCity(g_ui32CityActive, true);
                }

                break;
            }
            case iSwipeLeft:
            {
                ui32NewIdx = g_sScreens[g_i32ScreenIdx].ui32Right;

                //
                // Check if on main screen.
                //
                if(g_i32ScreenIdx == SCREEN_MAIN)
                {
                    if(g_ui32CityActive >= ui32NumCities - 1)
                    {
                        g_ui32CityActive = 0;
                    }
                    else
                    {
                        g_ui32CityActive++;
                    }

                    UpdateCity(g_ui32CityActive, true);
                }

                break;
            }
            default:
            {
                ui32NewIdx = g_i32ScreenIdx;
                break;
            }
        }

        //
        // Check if the panel has changed.
        //
        if(ui32NewIdx != g_i32ScreenIdx)
        {
            //
            // Remove the current widget.
            //
            WidgetRemove(g_sScreens[g_i32ScreenIdx].psWidget);

            WidgetAdd(WIDGET_ROOT, g_sScreens[ui32NewIdx].psWidget);

            g_i32ScreenIdx = ui32NewIdx;

            //
            // Screen switched so disable the overlay buttons.
            //
            ButtonsDisable();

            if(g_i32ScreenIdx == SCREEN_MAIN)
            {
                //
                // Update the frame.
                //
                FrameDraw(&g_sContext, "qs-weather");

                //
                // Change the status to updating if on the main screen.
                //
                UpdateCity(g_ui32CityActive, false);

                //
                // Animate the panel switch.
                //
                AnimatePanel(ClrBlack);

                //
                // If returning to the main screen then see if the settings
                // should be saved.
                //
                if(g_sConfig.bSave)
                {
                    g_sConfig.bSave = false;
                    FlashPBSave((uint8_t *)&g_sConfig);
                }
            }
            else
            {
                //
                // Update the frame.
                //
                FrameDraw(&g_sContext, "Settings");

                //
                // Animate the panel switch.
                //
                AnimatePanel(ClrGray);

                //
                // Animate the pull up tab once.
                //
                AnimateButtons(true);
            }
        }

        g_sSwipe.eMovement = iSwipeNone;
    }
}

//*****************************************************************************
//
// Linear scaling of the palette entries from white to normal color.
//
//*****************************************************************************
static uint8_t
PaletteScale(uint32_t ui32Entry, uint32_t ui32Scale)
{
    uint32_t ui32Value;

    ui32Value = ui32Entry + ((0xff - ui32Entry) * ui32Scale)/15;

    return((uint8_t)ui32Value);
}

//*****************************************************************************
//
// Display the TI logo screen.
//
//*****************************************************************************
void
TIWelcome(void)
{
    int32_t i32Idx, i32Line, i32Step;

    //
    // Initial color is white.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    ClearScreen(&g_sContext);

    //
    // Copy the default palette from the image.
    //
    for(i32Idx = 0; i32Idx < (g_pui8TIImage[5] * 3); i32Idx++)
    {
        g_pui8TIImagePalette[i32Idx] = g_pui8TIImage[6 + i32Idx];
    }

    //
    // Palette multiplier.
    //
    i32Step = 0xf;

    while(i32Step > 0)
    {
        //
        // Shift the palette of the image.
        //
        for(i32Idx = 0; i32Idx < g_pui8TIImage[5]; i32Idx++)
        {
            g_pui8TIImage[6 + (i32Idx * 3)] =
                    PaletteScale(g_pui8TIImagePalette[i32Idx * 3], i32Step);
            g_pui8TIImage[7 + (i32Idx * 3)] =
                    PaletteScale(g_pui8TIImagePalette[(i32Idx * 3) + 1],
                                 i32Step);
            g_pui8TIImage[8 + (i32Idx * 3)] =
                    PaletteScale(g_pui8TIImagePalette[(i32Idx * 3) + 2],
                                 i32Step);
        }

        //
        // Draw the areas of the screen.
        //
        GrImageDraw(&g_sContext, g_pui8TIImage, 0, 75);

        SysCtlDelay(g_ui32SysClock/50);

        //
        // Decrement the palette scaling.
        //
        i32Step--;
    }

    //
    // Hold the image for a few seconds.
    //
    SysCtlDelay(g_ui32SysClock);

    //
    // Set the initial scaling to not adjust the palette.
    //
    i32Step = 0;

    GrContextForegroundSet(&g_sContext, ClrBlack);

    //
    // Clear the screen from top and bottom while fading out the logo.
    //
    for(i32Line = 0; i32Line < 119; i32Line++)
    {
        //
        // Erase from the top and bottom of the screen.
        //
        GrLineDrawH(&g_sContext, 0, 329, i32Line);
        GrLineDrawH(&g_sContext, 0, 329, 239-i32Line);

        //
        // Fade the palette every 5th line and stop when the line draws cross
        // the image.
        //
        if(((i32Line % 5) == 0) && (i32Line < 75))
        {
            //
            // Shift the palette of the image.
            //
            for(i32Idx = 0; i32Idx < g_pui8TIImage[5]; i32Idx++)
            {
                g_pui8TIImage[6 + (i32Idx * 3)] =
                        PaletteScale(g_pui8TIImagePalette[i32Idx * 3],
                                     i32Step);
                g_pui8TIImage[7 + (i32Idx * 3)] =
                        PaletteScale(g_pui8TIImagePalette[(i32Idx * 3) + 1],
                                     i32Step);
                g_pui8TIImage[8 + (i32Idx * 3)] =
                        PaletteScale(g_pui8TIImagePalette[(i32Idx * 3) + 2],
                                     i32Step);

            }

            //
            // Draw the areas of the screen.
            //
            GrImageDraw(&g_sContext, g_pui8TIImage, 0, 75);

            //
            // Darken the background and decrement the palette multiplier.
            //
            i32Step++;
        }

        SysCtlDelay(g_ui32SysClock/2400);
    }

    //
    // Blank out one of the last pairs of lines.
    //
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrLineDrawH(&g_sContext, 0, 329, 120);

    //
    // "erase" the final line towards the middle.
    //
    for(i32Line = 0; i32Line < 159; i32Line++)
    {
        GrPixelDraw(&g_sContext,i32Line, 119);
        GrPixelDraw(&g_sContext,319- i32Line, 119);
        SysCtlDelay(g_ui32SysClock/2400);
    }

    //
    // Wait about a 1/4 second and set the background to black.
    //
    SysCtlDelay(g_ui32SysClock/12);
    GrContextForegroundSet(&g_sContext, ClrBlack);
    ClearScreen(&g_sContext);
}

//*****************************************************************************
//
// This example demonstrates the use of the Ethernet Controller.
//
//*****************************************************************************
int
main(void)
{
    enum
    {
        eRequestIdle,
        eRequestUpdate,
        eRequestForecast,
        eRequestCurrent,
        eRequestComplete,
    }
    iRequest;
    int32_t i32City;
    sParameters *pParams;
    iRequest = eRequestIdle;

    //
    // Run from the PLL at 120 MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(g_ui32SysClock);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Logo Screen
    //
    TIWelcome();

    //
    // Draw the application frame.
    //
    FrameDraw(&g_sContext, "qs-weather");

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sMainBackground);

    //
    // Start on the main screen.
    //
    g_i32ScreenIdx = SCREEN_MAIN;

    WidgetPaint(WIDGET_ROOT);

    //
    // Make sure the main oscillator is enabled because this is required by
    // the PHY.  The system must have a 25MHz crystal attached to the OSC
    // pins.  The SYSCTL_MOSC_HIGHFREQ parameter is used when the crystal
    // frequency is 10MHz or higher.
    //
    SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);

    //
    // Configure SysTick for a periodic interrupt at 10ms.
    //
    SysTickPeriodSet((g_ui32SysClock / 1000) * SYSTEM_TICK_MS);
    SysTickEnable();
    SysTickIntEnable();

    //
    // Initialized the flash program block and read the current settings.
    //
    FlashPBInit(FLASH_PB_START, FLASH_PB_END, 256);
    pParams = (sParameters *)FlashPBGet();

    //
    // Use the defaults if no settings were found.
    //
    if(pParams == 0)
    {
        g_sConfig = g_sDefaultParams;
    }
    else
    {
        g_sConfig = *pParams;
    }

    //
    // Initialize all of the cities.
    //
    for(i32City = 0; i32City < NUM_CITIES; i32City++)
    {
        ResetCity(i32City);
    }

    //
    // Initialize the swipe state.
    //
    g_sSwipe.eMovement = iSwipeNone;

    //
    // Set the IP address to 0.0.0.0.
    //
    UpdateIPAddress(g_pcIPAddr, 0);

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(g_ui32SysClock);
    TouchScreenCallbackSet(TouchCallback);

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Set the interrupt priorities.  We set the SysTick interrupt to a higher
    // priority than the Ethernet interrupt to ensure that the file system
    // tick is processed if SysTick occurs while the Ethernet handler is being
    // processed.  This is very likely since all the TCP/IP and HTTP work is
    // done in the context of the Ethernet interrupt.
    //
    IntPriorityGroupingSet(4);
    IntPrioritySet(INT_EMAC0, ETHERNET_INT_PRIORITY);
    IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);

    if(g_sConfig.bProxyEnabled)
    {
        EthClientProxySet(g_sConfig.pcProxy);
        EthClientInit(EnetEvents);
    }
    else
    {
        EthClientProxySet(0);
        EthClientInit(EnetEvents);
    }

    UpdateMACAddr();

    //
    // Initialize the city index and enable swipe detection.
    //
    g_ui32CityActive = 0;
    g_ui32CityUpdating = 0;
    g_sSwipe.bEnable = true;

    //
    // Display the current city information.
    //
    UpdateCity(g_ui32CityActive, true);

    //
    // One minute timeout for screen saver.
    //
    g_ui32ScreenSaver = SYSTEM_TICK_S * 60;

    //
    // Loop forever.  All the work is done in interrupt handlers.
    //
    while(1)
    {
        if(g_iState == STATE_NEW_CONNECTION)
        {
            iRequest = eRequestIdle;

            g_iState = STATE_CONNECTED_IDLE;
        }
        else if(g_iState == STATE_CONNECTED_IDLE)
        {
            if(iRequest == eRequestIdle)
            {
                //
                // If this city needs updating then start an update.
                //
                if((g_psCityInfo[g_ui32CityUpdating].bNeedsUpdate) &&
                   ((g_ui32CityUpdating < NUM_CITIES - 1) ||
                    (g_sConfig.bCustomEnabled == true)))
                {
                    iRequest = eRequestUpdate;

                    //
                    // Change the status to updating if on the main screen.
                    //
                    if((g_ui32CityUpdating == g_ui32CityActive) &&
                       (g_i32ScreenIdx == SCREEN_MAIN))
                    {
                        //
                        // Display the waiting message.
                        //
                        ustrncpy(g_pcStatus, g_pcWaitData, sizeof(g_pcStatus));
                        WidgetPaint((tWidget *)&g_sStatus);
                    }
                }

                if(iRequest != eRequestUpdate)
                {
                    //
                    // If the custom city is enabled and it needs updating,
                    // then update it first.
                    //
                    if((g_psCityInfo[NUM_CITIES - 1].bNeedsUpdate) &&
                       (g_sConfig.bCustomEnabled == true))
                    {
                        g_ui32CityUpdating = NUM_CITIES - 1;
                    }
                    else
                    {
                        //
                        // Move on to the next city to see if it needs to be
                        // updated on the next pass.
                        //
                        g_ui32CityUpdating++;
                    }
                    if(g_ui32CityUpdating >= NUM_CITIES)
                    {
                        g_ui32CityUpdating = 0;
                    }
                }
            }
            else if(iRequest == eRequestUpdate)
            {
                g_iState = STATE_WAIT_DATA;

                //
                // Timeout at 10 seconds.
                //
                g_ui32Delay = 1000;

                WeatherForecast(iWSrcOpenWeatherMap,
                                g_psCityInfo[g_ui32CityUpdating].pcName,
                                &g_psCityInfo[g_ui32CityUpdating].sReport,
                                WeatherEvent);

                iRequest = eRequestForecast;
            }
            else if(iRequest == eRequestForecast)
            {
                g_iState = STATE_WAIT_DATA;

                //
                // Timeout at 10 seconds.
                //
                g_ui32Delay = 1000;

                WeatherCurrent(iWSrcOpenWeatherMap,
                               g_psCityInfo[g_ui32CityUpdating].pcName,
                               &g_psCityInfo[g_ui32CityUpdating].sReport,
                               WeatherEvent);

                iRequest = eRequestCurrent;
            }
            else if(iRequest == eRequestCurrent)
            {
                //
                // Return to the idle request state.
                //
                iRequest = eRequestIdle;

                //
                // Done updating this city.
                //
                g_psCityInfo[g_ui32CityUpdating].bNeedsUpdate = false;
            }
        }
        else if(g_iState == STATE_UPDATE_CITY)
        {
            if(iRequest == eRequestCurrent)
            {
                //
                // If the city is the current active city and the the
                // application is on the main screen then update the screen
                // and not just the values.
                //
                if(g_ui32CityUpdating == g_ui32CityActive)
                {
                    //
                    // Update the current city.
                    //
                    if(g_i32ScreenIdx == SCREEN_MAIN)
                    {
                        UpdateCity(g_ui32CityUpdating, true);
                    }
                    else
                    {
                        UpdateCity(g_ui32CityUpdating, false);
                    }
                }

                //
                // Done updating this city.
                //
                g_psCityInfo[g_ui32CityUpdating].bNeedsUpdate = false;
            }

            //
            // Go to the wait nice state.
            //
            g_iState = STATE_WAIT_NICE;

            //
            // 10ms * 10 is a 1 second delay between updates.
            //
            g_ui32Delay = SYSTEM_TICK_MS * 10;
        }
        else if(g_iState == STATE_WAIT_NICE)
        {
            //
            // Wait for the "nice" delay to not hit the server too often.
            //
            if(g_ui32Delay == 0)
            {
                g_iState = STATE_CONNECTED_IDLE;
            }
        }
        else if(g_iState == STATE_WAIT_DATA)
        {
            //
            // If no data has been received by the timeout then close the
            // connection.
            //
            if(g_ui32Delay == 0)
            {
                //
                // Close out the current connection.
                //
                EthClientTCPDisconnect();
            }
        }

        //
        // Handle screen movements.
        //
        HandleMovement();

        //
        // Handle button animation.
        //
        AnimateButtons(false);

        //
        // Handle keyboard entry if it is open.
        //
        HandleKeyboard();

        //
        // If nothing has happened for awhile, then move to a new city.
        //
        if(g_ui32ScreenSaver == 0)
        {
            //
            // Reset the timeout for 10s to update the screen more often.
            //
            g_ui32ScreenSaver = 10 * SYSTEM_TICK_S;

            //
            // Trigger a left swipe.
            //
            g_sSwipe.eMovement = iSwipeLeft;
        }

        WidgetMessageQueueProcess();
    }
}
