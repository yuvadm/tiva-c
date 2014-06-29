//*****************************************************************************
//
// qs-logger.c - Data logger Quickstart application for EK-LM4F232.
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
// This is part of revision 2.1.0.12573 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "driverlib/adc.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/hibernate.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "inc/hw_gpio.h"
#include "inc/hw_hibernate.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "usblib/usblib.h"
#include "utils/ustdlib.h"
#include "drivers/cfal96x64x16.h"
#include "drivers/buttons.h"
#include "drivers/slidemenuwidget.h"
#include "drivers/stripchartwidget.h"
#include "clocksetwidget.h"
#include "qs-logger.h"
#include "images.h"
#include "menus.h"
#include "acquire.h"
#include "usbstick.h"
#include "usbserial.h"
#include "flashstore.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Data Logger (qs-logger)</h1>
//!
//! This example application is a data logger.  It can be configured to collect
//! data from up to 10 data sources.  The possible data sources are:
//! - 4 analog inputs, 0-20V
//! - 3-axis accelerometer
//! - internal and external temperature sensors
//! - processor current consumption
//!
//! The data logger provides a menu navigation that is operated by the buttons
//! on the EK-LM4F232 board (up, down, left, right, select).  The data logger
//! can be configured by using the menus.  The following items can be
//! configured:
//! - data sources to be logged
//! - sample rate
//! - storage location
//! - sleep modes
//! - clock
//!
//! Using the data logger:
//!
//! Use the CONFIG menu to configure the data logger.  The following choices
//! are provided:
//!
//! - CHANNELS - enable specific channels of data that will be logged
//! - PERIOD - select the sample period
//! - STORAGE - select where the collected data will be stored:
//!     - FLASH - stored in the internal flash memory
//!     - USB - stored on a connected USB memory stick
//!     - HOST PC - transmitted to a host PC via USB OTG virtual serial port
//!     - NONE - the data will only be displayed and not stored
//! - SLEEP - select whether or not the board sleeps between samples.  Sleep
//! mode is allowed when storing to flash at with a period of 1 second or
//! longer.
//! - CLOCK - allows setting of internal time-of-day clock that is used for
//! time stamping of the sampled data
//!
//! Use the START menu to start the data logger running.  It will begin
//! collecting and storing the data.  It will continue to collect data until
//! stopped by pressing the left button or select button.
//!
//! While the data logger is collecting data and it is not configured to
//! sleep, a simple strip chart showing the collected data will appear on the
//! display.  If the data logger is configured to sleep, then no strip chart
//! will be shown.
//!
//! If the data logger is storing to internal flash memory, it will overwrite
//! the oldest data.  If storing to a USB memory device it will store data
//! until the device is full.
//!
//! The VIEW menu allows viewing the values of the data sources in numerical
//! format.  When viewed this way the data is not stored.
//!
//! The SAVE menu allows saving data that was stored in internal flash memory
//! to a USB stick.  The data will be saved in a text file in CSV format.
//!
//! The ERASE menu is used to erase the internal memory so more data can be
//! saved.
//!
//! When the EK-LM4F232 board running qs-logger is connected to a host PC via
//! the USB OTG connection for the first time, Windows will prompt for a device
//! driver for the board.
//! This can be found in /ti/TivaWare_C_Series-x.x/windows_drivers
//! assuming you installed the software in the default folder.
//!
//! A companion Windows application, logger, can be found in the
//! /ti/TivaWare_C_Series-x.x/tools/bin directory.  When the data logger's
//!  STORAGE option is set to "HOST PC" and the board is connected to a PC
//! via the USB OTG connection, captured data will be transfered back to the PC
//! using the virtual serial port that the EK board offers.  When the logger
//! application is run, it will search for the first connected EK-LM4F232 board
//! and display any sample data received.  The application also offers the
//! option to log the data to a file on the PC.
//
//*****************************************************************************

//*****************************************************************************
//
// The clock rate for the SysTick interrupt and a counter of system clock
// ticks.  The SysTick interrupt is used for basic timing in the application.
//
//*****************************************************************************
#define CLOCK_RATE              100
#define MS_PER_SYSTICK          (1000 / CLOCK_RATE)
static volatile uint32_t g_ui32TickCount;
uint32_t g_ui32LastTick = 0;

//*****************************************************************************
//
// A widget handle of the widget that should receive the focus of any button
// events.
//
//*****************************************************************************
static uint32_t g_ui32KeyFocusWidgetHandle;

//*****************************************************************************
//
// Tracks the data logging state.
//
//*****************************************************************************
typedef enum
{
    eSTATE_IDLE,
    eSTATE_LOGGING,
    eSTATE_VIEWING,
    eSTATE_SAVING,
    eSTATE_ERASING,
    eSTATE_FREEFLASH,
    eSTATE_CLOCKSET,
    eSTATE_CLOCKEXIT,
} tLoggerState;
static tLoggerState g_iLoggerState = eSTATE_IDLE;

//*****************************************************************************
//
// The configuration of the application.  This holds the information that
// will need to be saved if sleeping is used.
//
//*****************************************************************************
static tConfigState g_sConfigState;

//*****************************************************************************
//
// The current state of USB OTG in the system based on the detected mode.
//
//*****************************************************************************
volatile tUSBMode g_iCurrentUSBMode = eUSBModeNone;

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
uint8_t g_pui8HCDPool[HCD_MEMORY_SIZE];

//*****************************************************************************
//
// Forward declaration of callback function used for clock setter widget.
//
//*****************************************************************************
static void ClockSetOkCallback(tWidget *pWidget, bool bOk);

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
// Provide a simple function so other parts of the application can update
// a status display.
//
//*****************************************************************************
void
SetStatusText(const char *pcTitle, const char *pcLine1, const char *pcLine2,
              const char *pcLine3)
{
    static const char pcBlankLine[] = "                ";

    //
    // Check to see if each parameter was passed, and if so then update its
    // text field on the status dislay.
    //
    pcTitle = pcTitle ? pcTitle : pcBlankLine;
    MenuUpdateText(TEXT_ITEM_STATUS_TITLE, pcTitle);
    pcLine1 = pcLine1 ? pcLine1 : pcBlankLine;
    MenuUpdateText(TEXT_ITEM_STATUS1, pcLine1);
    pcLine2 = pcLine2 ? pcLine2 : pcBlankLine;
    MenuUpdateText(TEXT_ITEM_STATUS2, pcLine2);
    pcLine3 = pcLine3 ? pcLine3 : pcBlankLine;
    MenuUpdateText(TEXT_ITEM_STATUS3, pcLine3);

    //
    // Force a repaint after all the status text fields have been updated.
    //
    WidgetPaint(WIDGET_ROOT);
    WidgetMessageQueueProcess();
}
//*****************************************************************************
//
// Handles the SysTick timeout interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Increment the tick count.
    //
    g_ui32TickCount++;
}

//*****************************************************************************
//
// This function returns the number of ticks since the last time this function
// was called.
//
//*****************************************************************************
uint32_t
GetTickms(void)
{
    uint32_t ui32RetVal, ui32Saved;

    ui32RetVal = g_ui32TickCount;
    ui32Saved = ui32RetVal;

    if(ui32Saved > g_ui32LastTick)
    {
        ui32RetVal = ui32Saved - g_ui32LastTick;
    }
    else
    {
        ui32RetVal = g_ui32LastTick - ui32Saved;
    }

    //
    // This could miss a few milliseconds but the timings here are on a
    // much larger scale.
    //
    g_ui32LastTick = ui32Saved;

    //
    // Return the number of milliseconds since the last time this was called.
    //
    return(ui32RetVal * MS_PER_SYSTICK);
}

//*****************************************************************************
//
// Callback function for USB OTG mode changes.
//
//*****************************************************************************
static void
ModeCallback(uint32_t ui32Index, tUSBMode iMode)
{
    //
    // Save the new mode.
    //
    g_iCurrentUSBMode = iMode;

    //
    // Mode-specific handling code could go here.
    //
    switch(iMode)
    {
        case eUSBModeHost:
        {
            break;
        }
        case eUSBModeDevice:
        {
            break;
        }
        case eUSBModeNone:
        {
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
// Gets the logger configuration from battery backed memory.
// The configuration is read from the memory in the Hibernate module.
// It is checked for validity.  If found to be valid the function returns a
// 0.  If not valid, then it returns non-zero.
//
//*****************************************************************************
static int32_t
GetSavedState(tConfigState *psState)
{
    uint32_t ui32StateLen;
    uint16_t ui16Crc16;

    //
    // Check the arguments
    //
    ASSERT(psState);
    if(!psState)
    {
        return(1);
    }

    //
    // Initialize locals.
    //
    ui32StateLen = sizeof(tConfigState) / 4;

    //
    // Read a block from hibernation memory into the application state
    // structure.
    //
    HibernateDataGet((uint32_t *)psState, ui32StateLen);

    //
    // Check first to see if the "cookie" value is correct.
    //
    if(psState->ui32Cookie != STATE_COOKIE)
    {
        return(1);
    }

    //
    // Find the 16-bit CRC of the block.  The CRC is stored in the last
    // location, so subtract 1 word from the count.
    //
    ui16Crc16 = ROM_Crc16Array(ui32StateLen - 1, (const uint32_t *)psState);

    //
    // If the CRC does not match, then the block is not good.
    //
    if(psState->ui32Crc16 != (uint32_t)ui16Crc16)
    {
        return(1);
    }

    //
    // At this point the state structure that was retrieved from the
    // battery backed memory has been validated, so return it as a valid
    // logger state configuration.
    //
    return(0);
}

//*****************************************************************************
//
// Stores the logger configuration to battery backed memory in the
// Hibernation module.  The configuration is saved with a cookie value and
// a CRC in order to ensure validity.
//
//*****************************************************************************
static void
SetSavedState(tConfigState *psState)
{
    uint32_t ui32StateLen;
    uint16_t ui16Crc16;

    //
    // Check the arguments.
    //
    ASSERT(psState);

    //
    // Initialize locals.
    //
    ui32StateLen = sizeof(tConfigState) / 4;

    if(psState)
    {
        //
        // Write the cookie value to the block
        //
        psState->ui32Cookie = STATE_COOKIE;

        //
        // Find the 16-bit CRC of the block.  The CRC is stored in the last
        // location, so subtract 1 word from the count.
        //
        ui16Crc16 = ROM_Crc16Array(ui32StateLen - 1,
                                   (const uint32_t *)psState);

        //
        // Save the computed CRC into the structure.
        //
        psState->ui32Crc16 = (uint32_t)ui16Crc16;

        //
        // Now write the entire block to the Hibernate memory.
        //
        HibernateDataSet((uint32_t *)psState, ui32StateLen);
    }
}

//*****************************************************************************
//
// Populate the application configuration with default values.
//
//*****************************************************************************
static void
GetDefaultState(tConfigState *psState)
{
    //
    // Check the arguments
    //
    ASSERT(psState);
    if(psState)
    {
        //
        // get the default values from the menu system
        //
        MenuGetDefaultState(psState);

        //
        // Set the filename to a null string
        //
        psState->pcFilename[0] = 0;

        //
        // Set bogus address for flash storage
        //
        psState->ui32FlashStore = 0;

        //
        // Turn off sleep logging
        //
        psState->ui32SleepLogging = 0;
    }
}

//*****************************************************************************
//
// Sends a button press message to whichever widget has the  button focus.
//
//*****************************************************************************
static void
SendWidgetKeyMessage(uint32_t ui32Msg)
{
    WidgetMessageQueueAdd(WIDGET_ROOT, ui32Msg, g_ui32KeyFocusWidgetHandle, 0,
                          1, 1);
}

//*****************************************************************************
//
// Callback function from the menu widget.  This function is called whenever
// the menu is used to activate a child widget that is associated with the
// menu.  It is also called when the widget is deactivated and control is
// returned to the menu widget.  It can be used to trigger different actions
// depending on which menus are chosen, and to track the state of the
// application and control focus for the user interface.
//
// This function is called in the context of widget tree message processing
// so care should be taken if doing any operation that affects the display
// or widget tree.
//
//*****************************************************************************
static void
WidgetActivated(tWidget *psWidget, tSlideMenuItem *psMenuItem, bool bActivated)
{
    char *pcMenuText;
    uint32_t ui32RTC;

    //
    // Handle the activation or deactivation of the strip chart.  The strip
    // chart widget is activated when the user selects the START menu.
    //
    if(psWidget == &g_sStripChart.sBase)
    {
        //
        // If the strip chart is activated, start the logger running.
        //
        if(bActivated)
        {
            //
            // Get the current state of the menus
            //
            MenuGetState(&g_sConfigState);

            //
            // Save the state in battery backed memory
            //
            SetSavedState(&g_sConfigState);

            //
            // Start logger and update the logger state
            //
            AcquireStart(&g_sConfigState);
            g_iLoggerState = eSTATE_LOGGING;
        }
        else
        {
            //
            // If the strip chart is deactivated, stop the logger.
            //
            AcquireStop();
            g_iLoggerState = eSTATE_IDLE;
        }
    }
    else if((psWidget == &g_sAINContainerCanvas.sBase) ||
            (psWidget == &g_sAccelContainerCanvas.sBase) ||
            (psWidget == &g_sCurrentContainerCanvas.sBase) ||
            (psWidget == &g_sClockContainerCanvas.sBase) ||
            (psWidget == &g_sTempContainerCanvas.sBase))
    {
        //
        // Handle the activation or deactivation of any of the container
        // canvas that is used for showing the acquired data as a numerical
        // display.  This happens when the VIEW menu is used.
        //
        // A viewer has been activated.
        //
        if(bActivated)
        {
            static tConfigState sLocalState;

            //
            // Get the current menu configuration state and save it in a local
            // storage.
            //
            MenuGetState(&sLocalState);

            //
            // Modify the state to set values that are suitable for using
            // with the viewer.  The acquisition rate is set to 1/2 second
            // and all channels are selected.  The storage medium is set to
            // "viewer" so the acquistion module will write the value of
            // acquired data to the appropriate viewing canvas.
            //
            sLocalState.ui8Storage = CONFIG_STORAGE_VIEWER;
            sLocalState.ui32Period = 0x00000040;
            sLocalState.ui16SelectedMask = 0x3ff;

            //
            // Start the acquisition module running.
            //
            AcquireStart(&sLocalState);
            g_iLoggerState = eSTATE_VIEWING;
        }
        else
        {
            //
            // The viewer has been deactivated so turn off the acquisition
            // module.
            //
            AcquireStop();
            g_iLoggerState = eSTATE_IDLE;
        }
    }
    else if(psWidget == &g_sStatusContainerCanvas.sBase)
    {
        //
        // Handle the case when a status display has been activated.  This can
        // occur when any of several menu items are selected.
        //
        // Get pointer to the text of the current menu item.
        //
        if(psMenuItem)
        {
            pcMenuText = psMenuItem->pcText;
        }
        else
        {
            return;
        }

        //
        // If activated from the SAVE menu, then the flash data needs to be
        // saved to USB stick.  Enter the saving state.
        //
        if(!strcmp(pcMenuText, "SAVE"))
        {
            if(bActivated)
            {
                g_iLoggerState = eSTATE_SAVING;
            }
            else
            {
                g_iLoggerState = eSTATE_IDLE;
            }
        }
        else if(!strcmp(pcMenuText, "ERASE DATA?"))
        {
            //
            // If activated from the ERASE menu, then the flash data needs to
            // be erased.  Enter the erasing state.
            //
            if(bActivated)
            {
                g_iLoggerState = eSTATE_ERASING;
            }
            else
            {
                g_iLoggerState = eSTATE_IDLE;
            }
        }
        else if(!strcmp(pcMenuText, "FLASH SPACE"))
        {
            //
            // If activated from the FLASH SPACE menu, then the user will be
            // shown a report on the amount of free space in flash.  Enter the
            // reporting state.
            //
            if(bActivated)
            {
                g_iLoggerState = eSTATE_FREEFLASH;
            }
            else
            {
                g_iLoggerState = eSTATE_IDLE;
            }
        }
    }
    else if(psWidget == &g_sClockSetter.sBase)
    {
        //
        // Handle the activation of the clock setting widget.  Deactivation is
        // handled through a separate callback.
        //
        // If the clock setter is activated, load the time structure fields.
        //
        if(bActivated)
        {
            //
            // Get the current time in seconds from the RTC.
            //
            ui32RTC = HibernateRTCGet();

            //
            // Convert the RTC time to a time structure.
            //
            ulocaltime(ui32RTC, &g_sTimeClock);

            //
            // Set the callback that will be called when the clock setting
            // widget is deactivated.  Since the clock setting widget needs
            // to take over the focus for button events, it uses a separate
            // callback when it is finsihed.
            //
            ClockSetCallbackSet((tClockSetWidget *)psWidget,
                                ClockSetOkCallback);

            //
            // Give the clock setter widget focus for the button events
            //
            g_ui32KeyFocusWidgetHandle = (uint32_t)psWidget;
            g_iLoggerState = eSTATE_CLOCKSET;
        }
    }
}

//*****************************************************************************
//
// This function is called when the user clicks OK or CANCEL in the clock
// setting widget.
//
//*****************************************************************************
static void
ClockSetOkCallback(tWidget *psWidget, bool bOk)
{
    uint32_t ui32RTC;

    //
    // Only update the RTC if the OK button was selected.
    //
    if(bOk)
    {
        //
        // Convert the time structure that was altered by the clock setting
        // widget into seconds.
        //
        ui32RTC = umktime(&g_sTimeClock);

        //
        // If the conversion was valid, then write the updated clock to the
        // Hibernate RTC.
        //
        if(ui32RTC != (uint32_t)(-1))
        {
            HibernateRTCSet(ui32RTC);
        }
    }

    //
    // Set the state to clock exit so some cleanup can be done from the
    // main loop.
    //
    g_iLoggerState = eSTATE_CLOCKEXIT;
}

//*****************************************************************************
//
// Initialize and operate the data logger.
//
//*****************************************************************************
int
main(void)
{
    tContext sDisplayContext, sBufferContext;
    uint32_t ui32HibIntStatus, ui32SysClock, ui32LastTickCount;
    bool bSkipSplash;
    uint8_t ui8ButtonState, ui8ButtonChanged;
    uint_fast8_t ui8X, ui8Y;


    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    MAP_FPULazyStackingEnable();

    //
    // Set the clocking to run at 50 MHz.
    //
    MAP_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);
    ui32SysClock = MAP_SysCtlClockGet();

    //
    // Initialize locals.
    //
    bSkipSplash = false;
    ui32LastTickCount = 0;

    //
    // Initialize the data acquisition module.  This initializes the ADC
    // hardware.
    //
    AcquireInit();

    //
    // Enable access to  the hibernate peripheral.  If the hibernate peripheral
    // was already running then this will have no effect.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);

    //
    // Check to see if the hiberate module is already active and if so then
    // read the saved configuration state.  If both are okay, then proceed
    // to check and see if we are logging data using sleep mode.
    //
    if(HibernateIsActive() && !GetSavedState(&g_sConfigState))
    {
        //
        // Read the status of the hibernate module.
        //
        ui32HibIntStatus = HibernateIntStatus(1);

        //
        // If this is a pin wake, that means the user pressed the select
        // button and we should terminate the sleep logging.  In this case
        // we will fall out of this conditional section, and go through the
        // normal startup below, but skipping the splash screen so the user
        // gets immediate response.
        //
        if(ui32HibIntStatus & HIBERNATE_INT_PIN_WAKE)
        {
            //
            // Clear the interrupt flag so it is not seen again until another
            // wake.
            //
            HibernateIntClear(HIBERNATE_INT_PIN_WAKE);
            bSkipSplash = true;
        }

        //
        // Otherwise if we are waking from hibernate and it was not a pin
        // wake, then it must be from RTC match.  Check to see if we are
        // sleep logging and if so then go through an abbreviated startup
        // in order to collect the data and go back to sleep.
        //
        else if(g_sConfigState.ui32SleepLogging &&
                (ui32HibIntStatus & HIBERNATE_INT_RTC_MATCH_0))
        {
            //
            // Start logger and pass the configuration.  The logger should
            // configure itself to take one sample.
            //
            AcquireStart(&g_sConfigState);
            g_iLoggerState = eSTATE_LOGGING;

            //
            // Enter a forever loop to run the acquisition.  This will run
            // until a new sample has been taken and stored.
            //
            while(!AcquireRun())
            {
            }

            //
            // Getting here means that a data acquisition was performed and we
            // can now go back to sleep.  Save the configuration and then
            // activate the hibernate.
            //
            SetSavedState(&g_sConfigState);

            //
            // Set wake condition on pin-wake or RTC match.  Then put the
            // processor in hibernation.
            //
            HibernateWakeSet(HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_RTC);
            HibernateRequest();

            //
            // Hibernating takes a finite amount of time to occur, so wait
            // here forever until hibernate activates and the processor
            // power is removed.
            //
            for(;;)
            {
            }
        }

        //
        // Otherwise, this was not a pin wake, and we were not sleep logging,
        // so just fall out of this conditional and go through the normal
        // startup below.
        //
    }
    else
    {
        //
        // In this case, either the hibernate module was not already active, or
        // the saved configuration was not valid.  Initialize the configuration
        // to the default state and then go through the normal startup below.
        //
        GetDefaultState(&g_sConfigState);
    }

    //
    // Enable the Hibernate module to run.
    //
    HibernateEnableExpClk(SysCtlClockGet());

    //
    // The hibernate peripheral trim register must be set per silicon
    // erratum 2.1
    //
    HibernateRTCTrimSet(0x7FFF);

    //
    // Start the RTC running.  If it was already running then this will have
    // no effect.
    //
    HibernateRTCEnable();

    //
    // In case we were sleep logging and are now finished (due to user
    // pressing select button), then disable sleep logging so it doesnt
    // try to start up again.
    //
    g_sConfigState.ui32SleepLogging = 0;
    SetSavedState(&g_sConfigState);

    //
    // Initialize the display driver.
    //
    CFAL96x64x16Init();

    //
    // Initialize the buttons driver.
    //
    ButtonsInit();

    //
    // Pass the restored state to the menu system.
    //
    MenuSetState(&g_sConfigState);

    //
    // Enable the USB peripheral
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);

    //
    // Configure the required pins for USB operation.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    MAP_GPIOPinConfigure(GPIO_PG4_USB0EPEN);
    MAP_GPIOPinTypeUSBDigital(GPIO_PORTG_BASE, GPIO_PIN_4);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    MAP_GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    MAP_GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Erratum workaround for silicon revision A1.  VBUS must have pull-down.
    //
    if(CLASS_IS_TM4C123 && REVISION_IS_A1)
    {
        HWREG(GPIO_PORTB_BASE + GPIO_O_PDR) |= GPIO_PIN_1;
    }

    //
    // Initialize the USB stack mode and pass in a mode callback.
    //
    USBStackModeSet(0, eUSBModeOTG, ModeCallback);

    //
    // Initialize the stack to be used with USB stick.
    //
    USBStickInit();

    //
    // Initialize the stack to be used as a serial device.
    //
    USBSerialInit();

    //
    // Initialize the USB controller for dual mode operation with a 2ms polling
    // rate.
    //
    USBOTGModeInit(0, 2000, g_pui8HCDPool, HCD_MEMORY_SIZE);

    //
    // Initialize the menus module.  This module will control the user
    // interface menuing system.
    //
    MenuInit(WidgetActivated);

    //
    // Configure SysTick to periodically interrupt.
    //
    g_ui32TickCount = 0;
    MAP_SysTickPeriodSet(ui32SysClock / CLOCK_RATE);
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();

    //
    // Initialize the display context and another context that is used
    // as an offscreen drawing buffer for display animation effect
    //
    GrContextInit(&sDisplayContext, &g_sCFAL96x64x16);
    GrContextInit(&sBufferContext, &g_sOffscreenDisplayA);

    //
    // Show the splash screen if we are not skipping it.  The only reason to
    // skip it is if the application was in sleep-logging mode and the user
    // just waked it up with the select button.
    //
    if(!bSkipSplash)
    {
        const uint8_t *pui8SplashLogo = g_pui8Image_TI_Black;

        //
        // Draw the TI logo on the display.  Use an animation effect where the
        // logo will "slide" onto the screen.  Allow select button to break
        // out of animation.
        //
        for(ui8X = 0; ui8X < 96; ui8X++)
        {
            if(ButtonsPoll(0, 0) & SELECT_BUTTON)
            {
                break;
            }
            GrImageDraw(&sDisplayContext, pui8SplashLogo, 95 - ui8X, 0);
        }

        //
        // Leave the logo on the screen for a long duration.  Monitor the
        // buttons so that if the user presses the select button, the logo
        // display is terminated and the application starts immediately.
        //
        while(g_ui32TickCount < 400)
        {
            if(ButtonsPoll(0, 0) & SELECT_BUTTON)
            {
                break;
            }
        }

        //
        // Extended splash sequence
        //
        if(ButtonsPoll(0, 0) & UP_BUTTON)
        {
            for(ui8X = 0; ui8X < 96; ui8X += 4)
            {
                GrImageDraw(&sDisplayContext,
                            g_ppui8Image_Splash[(ui8X / 4) & 3],
                            (int32_t)ui8X - 96L, 0);
                GrImageDraw(&sDisplayContext, pui8SplashLogo, ui8X, 0);
                MAP_SysCtlDelay(ui32SysClock / 12);
            }
            MAP_SysCtlDelay(ui32SysClock / 3);
            pui8SplashLogo = g_ppui8Image_Splash[4];
            GrImageDraw(&sDisplayContext, pui8SplashLogo, 0, 0);
            MAP_SysCtlDelay(ui32SysClock / 12);
        }

        //
        // Draw the initial menu into the offscreen buffer.
        //
        SlideMenuDraw(&g_sMenuWidget, &sBufferContext, 0);

        //
        // Now, draw both the TI logo splash screen (from above) and the initial
        // menu on the screen at the same time, moving the coordinates so that
        // the logo "slides" off the display and the menu "slides" onto the
        // display.
        //
        for(ui8Y = 0; ui8Y < 64; ui8Y++)
        {
            GrImageDraw(&sDisplayContext, pui8SplashLogo, 0, -ui8Y);
            GrImageDraw(&sDisplayContext, g_pui8OffscreenBufA, 0, 63 - ui8Y);
        }
    }

    //
    // Add the menu widget to the widget tree and send an initial paint
    // request.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sMenuWidget);
    WidgetPaint(WIDGET_ROOT);

    //
    // Set the focus handle to the menu widget.  Any button events will be
    // sent to this widget
    //
    g_ui32KeyFocusWidgetHandle = (uint32_t)&g_sMenuWidget;

    //
    // Forever loop to run the application
    //
    while(1)
    {

        //
        // Each time the timer tick occurs, process any button events.
        //
        if(g_ui32TickCount != ui32LastTickCount)
        {
            //
            // Remember last tick count
            //
            ui32LastTickCount = g_ui32TickCount;

            //
            // Read the debounced state of the buttons.
            //
            ui8ButtonState = ButtonsPoll(&ui8ButtonChanged, 0);

            //
            // Pass any button presses through to the widget message
            // processing mechanism.  The widget that has the button event
            // focus (probably the menu widget) will catch these button events.
            //
            if(BUTTON_PRESSED(SELECT_BUTTON, ui8ButtonState, ui8ButtonChanged))
            {
                SendWidgetKeyMessage(WIDGET_MSG_KEY_SELECT);
            }
            if(BUTTON_PRESSED(UP_BUTTON, ui8ButtonState, ui8ButtonChanged))
            {
                SendWidgetKeyMessage(WIDGET_MSG_KEY_UP);
            }
            if(BUTTON_PRESSED(DOWN_BUTTON, ui8ButtonState, ui8ButtonChanged))
            {
                SendWidgetKeyMessage(WIDGET_MSG_KEY_DOWN);
            }
            if(BUTTON_PRESSED(LEFT_BUTTON, ui8ButtonState, ui8ButtonChanged))
            {
                SendWidgetKeyMessage(WIDGET_MSG_KEY_LEFT);
            }
            if(BUTTON_PRESSED(RIGHT_BUTTON, ui8ButtonState, ui8ButtonChanged))
            {
                SendWidgetKeyMessage(WIDGET_MSG_KEY_RIGHT);
            }
        }

        //
        // Tell the OTG library code how much time has passed in milliseconds
        // since the last call.
        //
        USBOTGMain(GetTickms());

        //
        // Call functions as needed to keep the host or device mode running.
        //
        if(g_iCurrentUSBMode == eUSBModeDevice)
        {
            USBSerialRun();
        }
        else if(g_iCurrentUSBMode == eUSBModeHost)
        {
            USBStickRun();
        }

        //
        // If in the logging state, then call the logger run function.  This
        // keeps the data acquisition running.
        //
        if((g_iLoggerState == eSTATE_LOGGING) ||
           (g_iLoggerState == eSTATE_VIEWING))
        {
            if(AcquireRun() && g_sConfigState.ui32SleepLogging)
            {
                //
                // If sleep logging is enabled, then at this point we have
                // stored the first data item, now save the state and start
                // hibernation.  Wait for the power to be cut.
                //
                SetSavedState(&g_sConfigState);
                HibernateWakeSet(HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_RTC);
                HibernateRequest();
                for(;;)
                {
                }
            }

            //
            // If viewing instead of logging then request a repaint to keep
            // the viewing window updated.
            //
            if(g_iLoggerState == eSTATE_VIEWING)
            {
                WidgetPaint(WIDGET_ROOT);
            }
        }

        //
        // If in the saving state, then save data from flash storage to
        // USB stick.
        //
        if(g_iLoggerState == eSTATE_SAVING)
        {
            //
            // Save data from flash to USB
            //
            FlashStoreSave();

            //
            // Return to idle state
            //
            g_iLoggerState = eSTATE_IDLE;
        }

        //
        // If in the erasing state, then erase the data stored in flash.
        //
        if(g_iLoggerState == eSTATE_ERASING)
        {
            //
            // Save data from flash to USB
            //
            FlashStoreErase();

            //
            // Return to idle state
            //
            g_iLoggerState = eSTATE_IDLE;
        }

        //
        // If in the flash reporting state, then show the report of the amount
        // of used and free flash memory.
        //
        if(g_iLoggerState == eSTATE_FREEFLASH)
        {
            //
            // Report free flash space
            //
            FlashStoreReport();

            //
            // Return to idle state
            //
            g_iLoggerState = eSTATE_IDLE;
        }

        //
        // If we are exiting the clock setting widget, that means that control
        // needs to be given back to the menu system.
        //
        if(g_iLoggerState == eSTATE_CLOCKEXIT)
        {
            //
            // Give the button event focus back to the menu system
            //
            g_ui32KeyFocusWidgetHandle = (uint32_t)&g_sMenuWidget;

            //
            // Send a button event to the menu widget that means the left
            // key was pressed.  This signals the menu widget to deactivate
            // the current child widget (which was the clock setting wigdet).
            // This will cause the menu widget to slide the clock set widget
            // off the screen and resume control of the display.
            //
            SendWidgetKeyMessage(WIDGET_MSG_KEY_LEFT);
            g_iLoggerState = eSTATE_IDLE;
        }

        //
        // Process any new messages that are in the widget queue.  This keeps
        // the user interface running.
        //
        WidgetMessageQueueProcess();
    }
}
