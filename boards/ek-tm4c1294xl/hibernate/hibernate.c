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
// This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
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
#include "driverlib/systick.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "utils/cmdline.h"
#include "drivers/buttons.h"
#include "drivers/pinout.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Hibernate Example (hibernate)</h1>
//!
//! An example to demonstrate the use of the Hibernation module.  The user
//! can put the microcontroller in hibernation by typing 'hib' in the terminal
//! and pressing ENTER or by pressing USR_SW1 on the board.  The
//! microcontroller will then wake on its own after 5 seconds, or immediately
//! if the user presses the RESET button.  The External WAKE button, external
//! WAKE pins, and GPIO (PK6) wake sources can also be used to wake
//! immediately from hibernation.  The following wiring enables the use of
//! these pins as wake sources.
//!     WAKE on breadboard connection header (X11-95) to GND
//!     PK6 on BoosterPack 2 (X7-17) to GND
//!     PK6 on breadboard connection header (X11-63) to GND
//!
//! The program keeps a count of the number of times it has entered
//! hibernation.  The value of the counter is stored in the battery-backed
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
// Flag that informs that the user has requested hibernation.
//
//*****************************************************************************
volatile bool g_bHibernate;

//*****************************************************************************
//
// Variables that keep terminal position and status.
//
//*****************************************************************************
bool g_bFirstUpdate;
uint8_t g_ui8FirstLine;

//*****************************************************************************
//
// Flag that informs that date and time have to be set.
//
//*****************************************************************************
volatile bool g_bSetDate;

//*****************************************************************************
//
// Buffers to store display information.
//
//*****************************************************************************
char g_pcWakeBuf[40], g_pcHibBuf[40], g_pcDateTimeBuf[40];

//*****************************************************************************
//
// Buffer to store user command line input.
//
//*****************************************************************************
char g_pcInputBuf[40];

//*****************************************************************************
//
// Variables that keep track of the date and time.
//
//*****************************************************************************
uint32_t g_ui32MonthIdx, g_ui32DayIdx, g_ui32YearIdx;
uint32_t g_ui32HourIdx, g_ui32MinIdx;

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
DateTimeGet(struct tm *sTime)
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
    uint32_t ui32Len;

    //
    // Get the latest date and time and check the validity.
    //
    if(DateTimeGet(&sTime) == false)
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
    usnprintf(&pcBuf[ui32Len], ui32BufSize - ui32Len, "%02u : %02u : %02u",
              sTime.tm_hour, sTime.tm_min, sTime.tm_sec);

    //
    // Return true to indicate new information to display.
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
    // Update the calendar logic of hibernation module with the requested data.
    //
    HibernateCalendarSet(&sTime);
}

//*****************************************************************************
//
// This function sets the time to the default system time.
//
//*****************************************************************************
void
DateTimeDefaultSet(void)
{
    g_ui32MonthIdx = 7;
    g_ui32DayIdx = 29;
    g_ui32YearIdx = 13;
    g_ui32HourIdx = 8;
    g_ui32MinIdx = 30;

}
//*****************************************************************************
//
// This function updates individual buffers with valid date and time to be
// displayed on the date screen so that the date and time can be updated.
//
//*****************************************************************************
bool
DateTimeUpdateGet(void)
{
    struct tm sTime;

    //
    // Get the latest date and time and check the validity.
    //
    if(DateTimeGet(&sTime) == false)
    {
        //
        // Invalid - Return here with false as no information to update.  So
        // use default values.
        //
        DateTimeDefaultSet();
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
    // Return true to indicate new information has been updated.
    //
    return true;
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
// This function does some application level cleanup and alerts the user
// before sending the hibernate request to the hardware.
//
//*****************************************************************************
void
AppHibernateEnter(void)
{
    uint32_t ui32Status;
    struct tm sTime;

    //
    // Print the buffer to the terminal.
    //
    UARTprintf("To wake, wait for 5 seconds or press WAKE or"
               "RESET\n");
    UARTprintf("See README.txt for additional wake sources.\n");

    //
    // Wait for UART transmit to complete before proceeding to
    // hibernate.
    //
    UARTFlushTx(false);

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
    UARTprintf("The controller did not enter hibernate.  Press RESET"
               "button to restart example.\n");

    //
    // Wait here.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// This function is the interrupt handler for the SysTick timer.  It monitors
// both the USR_SW buttons on the board.  If a button is pressed then we
// request a hibernate cycle.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    uint32_t ui32Buttons;

    ui32Buttons = ButtonsPoll(0,0);

    switch(ui32Buttons & ALL_BUTTONS)
    {
        //
        // The user pressed USR_SW1.
        //
        case USR_SW1:
        {
            //
            // Set the hibernate flag to request a system hibernate cycle.
            //
            g_bHibernate = true;
            break;
        }

        //
        // For all other cases do nothing.
        //
        default:
        {
            break;
        }
    }

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
    bool bUpdate;
    uint32_t ui32SysClock, ui32Status, ui32HibernateCount, ui32Len;
    int32_t i32CmdStatus;

    //
    // Run from the PLL at 120 MHz.
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN |
                                           SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet(false, false);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, ui32SysClock);

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
        // the first location from the battery-backed memory, as the
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
        // Store that this was a system restart not wake from hibernation.
        //
        ui32Len = usnprintf(g_pcWakeBuf, sizeof(g_pcWakeBuf), "%s",
                            g_ppcWakeSource[4]);

        //
        // Set flag to indicate we need a valid date.  Date will then be set
        // in the while(1) loop.
        //
        g_bSetDate = true;
    }

    //
    // Store the hibernation count message into the respective char buffer.
    //
    usnprintf(g_pcHibBuf, sizeof(g_pcHibBuf), "Hibernate count = %u",
              ui32HibernateCount);

    //
    // Enable RTC mode.
    //
    HibernateRTCEnable();

    //
    // Configure the hibernate module counter to 24-hour calendar mode.
    //
    HibernateCounterMode(HIBERNATE_COUNTER_24HR);

    //
    // Configure GPIOs used as Hibernate wake source.  PK6 is configured as a
    // wake source.  It is available on EK-TM4C1294XL BoosterPack 2 (X7-17)
    // and on the breadboard breakout connector (X11-63).  Short to ground to
    // generate a wake request.
    //
    GPIOPadConfigSet(GPIO_PORTK_BASE, GPIO_PIN_6, GPIO_STRENGTH_2MA,
                     (GPIO_PIN_TYPE_WAKE_LOW | GPIO_PIN_TYPE_STD_WPU));

    //
    // Initialize the buttons
    //
    ButtonsInit();

    //
    // Initialize the SysTick interrupt to process user buttons.
    //
    SysTickPeriodSet(SysCtlClockGet() / 30);
    SysTickEnable();
    SysTickIntEnable();
    IntMasterEnable();

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
    // Initialize the necessary flags before entering indefinite loop.
    //
    g_bHibernate = false;

    //
    // Clear the terminal and print the banner.
    //
    UARTprintf("\033[2J\033[H");
    UARTprintf("%s\n", g_pcWakeBuf);
    UARTprintf("Welcome to the Tiva C Series TM4C1294 LaunchPad!\n");
    UARTprintf("Hibernation Example\n");
    UARTprintf("Type 'help' for a list of commands\n");
    UARTprintf("> ");
    UARTFlushTx(false);

    //
    // Set flag that next update is the first ever.
    // This triggers a screen clear on next update.
    //
    g_bFirstUpdate = true;
    g_ui8FirstLine = 5;

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Check the flag which indicates that an invalid time is in hibernate
        // module.  If set then force setting to the default time.
        //
        if(g_bSetDate)
        {
            //
            // Clear the flag.
            //
            g_bSetDate = false;

            //
            // Set the date to the default values and commit it to the
            // hibernate module.
            //
            DateTimeDefaultSet();
            DateTimeSet();
        }

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
            // Check if this is first ever update.
            //
            if(g_bFirstUpdate == false)
            {
                //
                // Save current cursor position.
                //
                UARTprintf("\033[s");
            }

            //
            // Resend the current status and time.
            //
            UARTprintf("\033[%d;1H\033[K", g_ui8FirstLine);
            UARTprintf("The current date and time is: %s\n", g_pcDateTimeBuf);
            UARTprintf("\033[K");
            UARTprintf("%s\n", g_pcHibBuf);
            UARTprintf("\033[K");
            UARTprintf("To Hibernate type 'hib' and press ENTER or press "
                       "USR_SW1\n");

            //
            // Check if this is first ever update.
            //
            if(g_bFirstUpdate == false)
            {
                //
                // Restore cursor position.
                //
                UARTprintf("\033[u");

            }
            else
            {
                UARTprintf(">");
            }

            //
            // Flush the TX Buffer.
            //
            UARTFlushTx(false);

            //
            // Clear the first update flag.
            //
            g_bFirstUpdate = false;

        }

        //
        // Check if a carriage return is present in the UART Buffer.
        //
        if(UARTPeek('\r') != -1)
        {
            //
            // A '\r' was detected, so get the line of text from the user.
            //
            UARTgets(g_pcInputBuf,sizeof(g_pcInputBuf));

            //
            // Pass the line from the user to the command processor.
            // It will be parsed and valid commands executed.
            //
            i32CmdStatus = CmdLineProcess(g_pcInputBuf);

            //
            // Handle the case of bad command.
            //
            if(i32CmdStatus == CMDLINE_BAD_CMD)
            {
                UARTprintf("Command not recognized!\n");
            }

            //
            // Handle the case of too many arguments.
            //
            else if(i32CmdStatus == CMDLINE_TOO_MANY_ARGS)
            {
                UARTprintf("Too many arguments for command processor!\n");
            }

            //
            // Handle the case of too few arguments.
            //
            else if(i32CmdStatus == CMDLINE_TOO_FEW_ARGS)
            {
                UARTprintf("Too few arguments for command processor!\n");
            }

            UARTprintf(">");
        }

        //
        // Check if user wants to enter hibernation.
        //
        if(g_bHibernate == true)
        {
            //
            // Increment the hibernation count, and store it in the
            // battery-backed memory.
            //
            ui32HibernateCount++;
            HibernateDataSet(&ui32HibernateCount, 1);

            //
            // Yes - Clear the flag.
            //
            g_bHibernate = false;
            AppHibernateEnter();
        }
    }
}
