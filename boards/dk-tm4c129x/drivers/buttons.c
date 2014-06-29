//*****************************************************************************
//
// buttons.c - DK-TM4C129X development board buttons driver.
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
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "drivers/buttons.h"

//*****************************************************************************
//
//! \addtogroup buttons_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Holds the current, debounced state of each button.  A 0 in a bit indicates
// that that button is currently pressed, otherwise it is released.
// We assume that we start with all the buttons released (though if one is
// pressed when the application starts, this will be detected).
//
//*****************************************************************************
static uint8_t g_ui8ButtonStates;

static uint8_t g_uiButtonsEnabled;

//*****************************************************************************
//
//! Polls the current state of the buttons and determines which have changed.
//!
//! \param pui8Delta points to a character that will be written to indicate
//! which button states changed since the last time this function was called.
//! This value is derived from the debounced state of the buttons.
//! \param pui8RawState points to a location where the raw button state will
//! be stored.
//!
//! This function should be called periodically by the application to poll the
//! pushbuttons.  It determines both the current debounced state of the buttons
//! and also which buttons have changed state since the last time the function
//! was called.
//!
//! In order for button debouncing to work properly, this function should be
//! caled at a regular interval, even if the state of the buttons is not needed
//! that often.
//!
//! If button debouncing is not required, the the caller can pass a pointer
//! for the \e pui8RawState parameter in order to get the raw state of the
//! buttons.  The value returned in \e pui8RawState will be a bit mask where
//! a 1 indicates the buttons is pressed.
//!
//! \return Returns the current debounced state of the buttons where a 1 in the
//! button ID's position indicates that the button is pressed and a 0
//! indicates that it is released.
//
//*****************************************************************************
uint8_t
ButtonsPoll(uint8_t *pui8Delta, uint8_t *pui8RawState)
{
    uint32_t ui32Delta;
    uint32_t ui32Data;
    static uint8_t ui8SwitchClockA = 0;
    static uint8_t ui8SwitchClockB = 0;

    //
    // Read the raw state of the push buttons.  Save the raw state
    // (inverting the bit sense) if the caller supplied storage for the
    // raw value.
    //
    ui32Data = 0;
    if(g_uiButtonsEnabled & UP_BUTTON)
    {
        ui32Data = ROM_GPIOPinRead(GPIO_PORTN_BASE, UP_BUTTON);
    }
    if(g_uiButtonsEnabled & DOWN_BUTTON)
    {
        ui32Data |= ROM_GPIOPinRead(GPIO_PORTE_BASE, DOWN_BUTTON);
    }
    if(g_uiButtonsEnabled & SELECT_BUTTON)
    {
        ui32Data |= ROM_GPIOPinRead(GPIO_PORTP_BASE, SELECT_BUTTON);
    }

    if(pui8RawState)
    {
        *pui8RawState = (uint8_t)~ui32Data;
    }

    //
    // Determine the switches that are at a different state than the debounced
    // state.
    //
    ui32Delta = ui32Data ^ g_ui8ButtonStates;

    //
    // Increment the clocks by one.
    //
    ui8SwitchClockA ^= ui8SwitchClockB;
    ui8SwitchClockB = ~ui8SwitchClockB;

    //
    // Reset the clocks corresponding to switches that have not changed state.
    //
    ui8SwitchClockA &= ui32Delta;
    ui8SwitchClockB &= ui32Delta;

    //
    // Get the new debounced switch state.
    //
    g_ui8ButtonStates &= ui8SwitchClockA | ui8SwitchClockB;
    g_ui8ButtonStates |= (~(ui8SwitchClockA | ui8SwitchClockB)) & ui32Data;

    //
    // Determine the switches that just changed debounced state.
    //
    ui32Delta ^= (ui8SwitchClockA | ui8SwitchClockB);

    //
    // Store the bit mask for the buttons that have changed for return to
    // caller.
    //
    if(pui8Delta)
    {
        *pui8Delta = (uint8_t)ui32Delta;
    }

    //
    // Return the debounced buttons states to the caller.  Invert the bit
    // sense so that a '1' indicates the button is pressed, which is a
    // sensible way to interpret the return value.
    //
    return(~g_ui8ButtonStates);
}

//*****************************************************************************
//
//! Initializes the GPIO pins used by the board pushbuttons.
//!
//! \param ui8Buttons is the logical OR of the buttons to initialize.
//!
//! This function must be called during application initialization to
//! configure the GPIO pins to which the pushbuttons are attached.  It enables
//! the port used by the buttons and configures each button GPIO as an input
//! with a weak pull-up.  The \e ui8Buttons value must be a logical OR
//! combination of the following three buttons on the board: \b UP_BUTTON,
//! \b DOWN_BUTTON, or \b SELECT_BUTTON.
//!
//! \return None.
//
//*****************************************************************************
void
ButtonsInit(uint8_t ui8Buttons)
{
    //
    // Initialize the button state.
    //
    g_ui8ButtonStates = 0;

    //
    // Save the buttons in use.
    //
    g_uiButtonsEnabled = ui8Buttons;

    if(ui8Buttons & UP_BUTTON)
    {
        //
        // Enable the GPIO port to which the up button is connected.
        //
        ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

        //
        // Set the up button's GPIO pins as an input with a pull-up.
        //
        ROM_GPIODirModeSet(GPIO_PORTN_BASE, UP_BUTTON, GPIO_DIR_MODE_IN);
        MAP_GPIOPadConfigSet(GPIO_PORTN_BASE, UP_BUTTON,
                             GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);

        //
        // Initialize the debounced button state with the current state read
        // from the GPIO bank.
        //
        g_ui8ButtonStates = ROM_GPIOPinRead(GPIO_PORTN_BASE, UP_BUTTON);
    }

    if(ui8Buttons & DOWN_BUTTON)
    {
        //
        // Enable the GPIO port to which the down button is connected.
        //
        ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

        //
        // Set the down button's GPIO pins as an input with a pull-up.
        //
        ROM_GPIODirModeSet(GPIO_PORTE_BASE, DOWN_BUTTON, GPIO_DIR_MODE_IN);
        MAP_GPIOPadConfigSet(GPIO_PORTE_BASE, DOWN_BUTTON,
                             GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);

        //
        // Initialize the debounced button state with the current state read
        // from the GPIO bank.
        //
        g_ui8ButtonStates |= ROM_GPIOPinRead(GPIO_PORTE_BASE, DOWN_BUTTON);
    }

    if(ui8Buttons & SELECT_BUTTON)
    {
        //
        // Enable the GPIO port to which the select button is connected.
        //
        ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);

        //
        // Set the select button's GPIO pins as an input with a pull-up.
        //
        ROM_GPIODirModeSet(GPIO_PORTP_BASE, SELECT_BUTTON, GPIO_DIR_MODE_IN);
        MAP_GPIOPadConfigSet(GPIO_PORTP_BASE, SELECT_BUTTON,
                             GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);

        //
        // Initialize the debounced button state with the current state read
        // from the GPIO bank.
        //
        g_ui8ButtonStates |= ROM_GPIOPinRead(GPIO_PORTP_BASE, SELECT_BUTTON);
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
