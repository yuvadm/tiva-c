//*****************************************************************************
//
// qs-rgb.c - Quickstart for the EK-TM4C123GXL.
//
// Copyright (c) 2012-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_hibernate.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/hibernate.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "utils/cmdline.h"
#include "drivers/rgb.h"
#include "drivers/buttons.h"
#include "rgb_commands.h"
#include "qs-rgb.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>EK-TM4C123GXL Quickstart Application (qs-rgb)</h1>
//!
//! A demonstration of the Tiva C Series LaunchPad (EK-TM4C123GXL)
//! capabilities.
//!
//! Press and/or hold the left button to traverse towards the red end of the
//! ROYGBIV color spectrum.  Press and/or hold the right button to traverse
//! toward the violet end of the ROYGBIV color spectrum.
//!
//! If no input is received for 5 seconds, the application will start
//! automatically changing the color displayed.
//!
//! Press and hold both left and right buttons for 3 seconds to enter
//! hibernation.  During hibernation, the last color displayed will blink
//! for 0.5 seconds every 3 seconds.
//!
//! The system can also be controlled via a command line provided via the UART.
//! Configure your host terminal emulator for 115200, 8-N-1 to access this
//! feature.
//!
//! - Command 'help' generates a list of commands and helpful information.
//! - Command 'hib' will place the device into hibernation mode.
//! - Command 'rand' will initiate the pseudo-random color sequence.
//! - Command 'intensity' followed by a number between 0 and 100 will set the
//! brightness of the LED as a percentage of maximum brightness.
//! - Command 'rgb' followed by a six character hex value will set the color.
//! For example 'rgb FF0000' will produce a red color.
//
//*****************************************************************************

//*****************************************************************************
//
// Entry counter to track how int32_t to stay in certain staging states before
// making transition into hibernate.
//
//*****************************************************************************
static volatile uint32_t ui32HibModeEntryCount;

//*****************************************************************************
//
// Array of pre-defined colors for use when buttons cause manual color steps.
//
//*****************************************************************************
static float fManualColors[7] = {0.0f, .214f, .428f, .642f, .856f, 1.07f,
                                 1.284f};

//*****************************************************************************
//
// Input buffer for the command line interpreter.
//
//*****************************************************************************
static char g_cInput[APP_INPUT_BUF_SIZE];

//*****************************************************************************
//
// Application state structure.  Gets stored to hibernate memory for
// preservation across hibernate events.
//
//*****************************************************************************
volatile tAppState g_sAppState;

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
// Handler to manage the button press events and state machine transitions
// that result from those button events.
//
// This function is called by the SysTickIntHandler if a button event is
// detected. Function will determine which button was pressed and tweak various
// elements of the global state structure accordingly.
//
//*****************************************************************************
void
AppButtonHandler(void)
{
    static uint32_t ui32TickCounter;

    ui32TickCounter++;

    //
    // Switch statement to adjust the color wheel position based on buttons
    //
    switch(g_sAppState.ui32Buttons & ALL_BUTTONS)
    {

    case LEFT_BUTTON:

        //
        // Check if the button has been held int32_t enough to peform another
        // color wheel increment.
        //
        if((ui32TickCounter % APP_BUTTON_POLL_DIVIDER) == 0)
        {
            //
            // Perform the increment and index wrap around.
            //
            g_sAppState.ui32ManualIndex++;
            if(g_sAppState.ui32ManualIndex >= APP_NUM_MANUAL_COLORS)
            {
                g_sAppState.ui32ManualIndex = 0;
            }
            g_sAppState.fColorWheelPos = APP_PI *
                                      fManualColors[g_sAppState.ui32ManualIndex];
        }

        //
        // Reset some state counts and system mode so that we know the user
        // is present and actively engaging with the application.
        //
        ui32HibModeEntryCount = 0;
        g_sAppState.ui32ModeTimer = 0;
        g_sAppState.ui32Mode = APP_MODE_NORMAL;
        break;

    case RIGHT_BUTTON:

        //
        // Check if the button has been held int32_t enough to perform another
        // color wheel decrement.
        //
        if((ui32TickCounter % APP_BUTTON_POLL_DIVIDER) == 0)
        {
            //
            // Perform the decrement and index wrap around.
            //
            if(g_sAppState.ui32ManualIndex == 0)
            {
                //
                // set to one greater than the last color so that we decrement
                // back into range with next instruction.
                //
                g_sAppState.ui32ManualIndex = APP_NUM_MANUAL_COLORS;
            }
            g_sAppState.ui32ManualIndex--;
            g_sAppState.fColorWheelPos = APP_PI *
                                      fManualColors[g_sAppState.ui32ManualIndex];
        }
        //
        // Reset some state counts and system mode so that we know the user
        // is present and actively engaging with the application.
        //
        ui32HibModeEntryCount = 0;
        g_sAppState.ui32ModeTimer = 0;
        g_sAppState.ui32Mode = APP_MODE_NORMAL;
        break;

    case ALL_BUTTONS:

        //
        // Both buttons for longer than debounce time will cause hibernation
        //
        if(ui32HibModeEntryCount < APP_HIB_BUTTON_DEBOUNCE)
        {
            ui32HibModeEntryCount++;
            g_sAppState.ui32Mode = APP_MODE_NORMAL;
        }
        else
        {
            g_sAppState.ui32Mode = APP_MODE_HIB;
        }
        g_sAppState.ui32ModeTimer = 0;
        break;

    default:
        if(g_sAppState.ui32Mode == APP_MODE_HIB_FLASH)
        {
            //
            // Waking from hibernate RTC just do a quick flash then back to
            // hibernation.
            //
            if(ui32HibModeEntryCount < APP_HIB_FLASH_DURATION)
            {
                ui32HibModeEntryCount++;
            }
            else
            {
                g_sAppState.ui32Mode = APP_MODE_HIB;
            }
        }
        else
        {
            //
            // Normal or remote mode and no user action will cause transition
            // to automatic scrolling mode.
            //
            ui32HibModeEntryCount = 0;
            if(g_sAppState.ui32ModeTimer < APP_AUTO_MODE_TIMEOUT)
            {
                g_sAppState.ui32ModeTimer++;
            }
            else
            {
                g_sAppState.ui32Mode = APP_MODE_AUTO;
            }

            //
            // reset the tick counter when no buttons are pressed
            // this makes the first button reaction speed quicker
            //
            ui32TickCounter = APP_BUTTON_POLL_DIVIDER - 1;
        }
        break;
    }
}

//*****************************************************************************
//
// Uses the fColorWheelPos variable to update the color mix shown on the RGB
//
// ui32ForceUpdate when set forces a color update even if a color change
// has not been detected.  Used primarily at startup to init the color after
// a hibernate.
//
// This function is called by the SysTickIntHandler to update the colors on
// the RGB LED whenever a button or timeout event has changed the color wheel
// position.  Color is determined by a series of sine functions and conditions
//
//*****************************************************************************
void
AppRainbow(uint32_t ui32ForceUpdate)
{
    static float fPrevPos;
    float fCurPos;
    float fTemp;
    volatile uint32_t * pui32Colors;

    pui32Colors = g_sAppState.ui32Colors;
    fCurPos = g_sAppState.fColorWheelPos;


    if((fCurPos != fPrevPos) || ui32ForceUpdate)
    {
        //
        // Preserve the new color wheel position
        //
        fPrevPos = fCurPos;

        //
        // Adjust the BLUE value based on the control state
        //
        fTemp = 65535.0f * sinf(fCurPos);
        if(fTemp < 0)
        {
            pui32Colors[GREEN] = 0;
        }
        else
        {
            pui32Colors[GREEN] = (uint32_t) fTemp;
        }


        //
        // Adjust the RED value based on the control state
        //
        fTemp = 65535.0f * sinf(fCurPos - APP_PI / 2.0f);
        if(fTemp < 0)
        {
            pui32Colors[BLUE] = 0;
        }
        else
        {
            pui32Colors[BLUE] = (uint32_t) fTemp;
        }


        //
        // Adjust the GREEN value based on the control state
        //
        if(fCurPos < APP_PI)
        {
            fTemp = 65535.0f * sinf(fCurPos + APP_PI * 0.5f);
        }
        else
        {
            fTemp = 65535.0f * sinf(fCurPos + APP_PI);
        }
        if(fTemp < 0)
        {
            pui32Colors[RED] = 0;
        }
        else
        {
            pui32Colors[RED] = (uint32_t) fTemp;
        }

        //
        // Update the actual LED state
        //
        RGBColorSet(pui32Colors);
    }

}

//*****************************************************************************
//
// Called by the NVIC as a result of SysTick Timer rollover interrupt flag
//
// Checks buttons and calls AppButtonHandler to manage button events.
// Tracks time and auto mode color stepping.  Calls AppRainbow to implement
// RGB color changes.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{

    static float x;

    g_sAppState.ui32Buttons = ButtonsPoll(0,0);
    AppButtonHandler();

    //
    // Auto increment the color wheel if in the AUTO mode. AUTO mode is when
    // device is active but user interaction has timed out.
    //
    if(g_sAppState.ui32Mode == APP_MODE_AUTO)
    {
        g_sAppState.fColorWheelPos += APP_AUTO_COLOR_STEP;
    }

    //
    // Provide wrap around of the control variable from 0 to 1.5 times PI
    //
    if(g_sAppState.fColorWheelPos > (APP_PI * 1.5f))
    {
        g_sAppState.fColorWheelPos = 0.0f;
    }
    if(x < 0.0f)
    {
        g_sAppState.fColorWheelPos = APP_PI * 1.5f;
    }

    //
    //    Set the RGB Color based on current control variable value.
    //
    AppRainbow(0);


}

//*****************************************************************************
//
// Uses the fColorWheelPos variable to update the color mix shown on the RGB
//
// This function is called when system has decided it is time to enter
// Hibernate.  This will prepare the hibernate peripheral, save the system
// state and then enter hibernate mode.
//
//*****************************************************************************
void
AppHibernateEnter(void)
{
    //
    // Alert UART command line users that we are going to hibernate
    //
    UARTprintf("Entering Hibernate...\n");

    //
    // Prepare Hibernation Module
    //
    HibernateGPIORetentionEnable();
    HibernateRTCSet(0);
    HibernateRTCEnable();
    HibernateRTCMatchSet(0, 5);
    HibernateWakeSet(HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_RTC);

    //
    // Store state information to battery backed memory
    // since sizeof returns number of bytes we convert to words and force
    // a rounding up to next whole word.
    //
    HibernateDataSet((uint32_t*)&g_sAppState, sizeof(tAppState)/4+1);

    //
    // Disable the LED for 100 milliseconds to let user know we are
    // ready for hibernate and will hibernate on relase of buttons
    //
    RGBDisable();
    SysCtlDelay(SysCtlClockGet()/3/10);
    RGBEnable();

    //
    // Wait for wake button to be released prior to going into hibernate
    //
    while(g_sAppState.ui32Buttons & RIGHT_BUTTON)
    {
        //
        //Delay for about 300 clock ticks to allow time for interrupts to
        //sense that button is released
        //
        SysCtlDelay(100);
    }

    //
    // Disable the LED for power savings and go to hibernate mode
    //
    RGBDisable();
    HibernateRequest();


}

//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************
void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, 16000000);
}

//*****************************************************************************
//
// Main function performs init and manages system.
//
// Called automatically after the system and compiler pre-init sequences.
// Performs system init calls, restores state from hibernate if needed and
// then manages the application context duties of the system.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32Status;
    uint32_t ui32ResetCause;
    int32_t i32CommandStatus;

    //
    // Enable stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPUEnable();
    ROM_FPUStackingEnable();

    //
    // Set the system clock to run at 40Mhz off PLL with external crystal as
    // reference.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    //
    // Enable the hibernate module
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);

    //
    // Enable and Initialize the UART.
    //
    ConfigureUART();

    UARTprintf("Welcome to the Tiva C Series TM4C123G LaunchPad!\n");
    UARTprintf("Type 'help' for a list of commands\n");
    UARTprintf("> ");

    //
    // Determine why system reset occurred and respond accordingly.
    //
    ui32ResetCause = SysCtlResetCauseGet();
    SysCtlResetCauseClear(ui32ResetCause);
    if(ui32ResetCause == SYSCTL_CAUSE_POR)
    {
        if(HibernateIsActive())
        {
            //
            // Read the status bits to see what caused the wake.
            //
            ui32Status = HibernateIntStatus(0);
            HibernateIntClear(ui32Status);

            //
            // Wake was due to the push button.
            //
            if(ui32Status & HIBERNATE_INT_PIN_WAKE)
            {
                UARTprintf("Hibernate Wake Pin Wake Event\n");
                UARTprintf("> ");

                //
                // Recover the application state variables from battery backed
                // hibernate memory.  Set ui32Mode to normal.
                //
                HibernateDataGet((uint32_t*) &g_sAppState,
                                 sizeof(tAppState) / 4 + 1);
                g_sAppState.ui32Mode = APP_MODE_NORMAL;
            }

            //
            // Wake was due to RTC match
            //
            else if(ui32Status & HIBERNATE_INT_RTC_MATCH_0)
            {
                UARTprintf("Hibernate RTC Wake Event\n");
                UARTprintf("> ");
                //
                // Recover the application state variables from battery backed
                // hibernate memory. Set ui32Mode to briefly flash the RGB.
                //
                HibernateDataGet((uint32_t*) &g_sAppState,
                                sizeof(tAppState) / 4 + 1);
                g_sAppState.ui32Mode = APP_MODE_HIB_FLASH;
            }
        }

        else
        {
            //
            // Reset was do to a cold first time power up.
            //
            UARTprintf("Power on reset. Hibernate not active.\n");
            UARTprintf("> ");

            g_sAppState.ui32Mode = APP_MODE_NORMAL;
            g_sAppState.fColorWheelPos = 0;
            g_sAppState.fIntensity = APP_INTENSITY_DEFAULT;
            g_sAppState.ui32Buttons = 0;
        }
    }
    else
    {
        //
        // External Pin reset or other reset event occured.
        //
        UARTprintf("External or other reset\n");
        UARTprintf("> ");

        //
        // Treat this as a cold power up reset without restore from hibernate.
        //
        g_sAppState.ui32Mode = APP_MODE_NORMAL;
        g_sAppState.fColorWheelPos = APP_PI;
        g_sAppState.fIntensity = APP_INTENSITY_DEFAULT;
        g_sAppState.ui32Buttons = 0;

    //
        // colors get a default initialization later when we call AppRainbow.
        //
    }

    //
    // Initialize clocking for the Hibernate module
    //
    HibernateEnableExpClk(SysCtlClockGet());

    //
    // Initialize the RGB LED. AppRainbow typically only called from interrupt
    // context. Safe to call here to force initial color update because
    // interrupts are not yet enabled.
    //
    RGBInit(0);
    RGBIntensitySet(g_sAppState.fIntensity);
    AppRainbow(1);
    RGBEnable();

    //
    // Initialize the buttons
    //
    ButtonsInit();

    //
    // Initialize the SysTick interrupt to process colors and buttons.
    //
    SysTickPeriodSet(SysCtlClockGet() / APP_SYSTICKS_PER_SEC);
    SysTickEnable();
    SysTickIntEnable();
    IntMasterEnable();

    //
    // spin forever and wait for carriage returns or state changes.
    //
    while(1)
    {

        UARTprintf("\n>");


        //
        // Peek to see if a full command is ready for processing
        //
        while(UARTPeek('\r') == -1)
        {
            //
            // millisecond delay.  A SysCtlSleep() here would also be OK.
            //
            SysCtlDelay(SysCtlClockGet() / (1000 / 3));

            //
            // Check for change of mode and enter hibernate if requested.
            // all other mode changes handled in interrupt context.
            //
            if(g_sAppState.ui32Mode == APP_MODE_HIB)
            {
                AppHibernateEnter();
            }
        }

        //
        // a '\r' was detected get the line of text from the user.
        //
        UARTgets(g_cInput,sizeof(g_cInput));

        //
        // Pass the line from the user to the command processor.
        // It will be parsed and valid commands executed.
        //
        i32CommandStatus = CmdLineProcess(g_cInput);

        //
        // Handle the case of bad command.
        //
        if(i32CommandStatus == CMDLINE_BAD_CMD)
        {
            UARTprintf("Bad command!\n");
        }

        //
        // Handle the case of too many arguments.
        //
        else if(i32CommandStatus == CMDLINE_TOO_MANY_ARGS)
        {
            UARTprintf("Too many arguments for command processor!\n");
        }
    }
}
