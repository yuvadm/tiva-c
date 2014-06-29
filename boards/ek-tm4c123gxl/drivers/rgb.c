//*****************************************************************************
//
// rgb.c - Evaluation board driver for RGB LED.
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
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "rgb.h"

//*****************************************************************************
//
//! \addtogroup rgb_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// This is a custom driver that allows the easy manipulation of the RGB LED.
//
// The driver uses the general purpose timers to govern the brightness of the
// LED through simple PWM output mode of the GP Timers.
//
// A global array contains the current relative color of each of the three
// LEDs. A global float variable controls intensity of the overall mixed color.
//
// This implementation consumes the following hardware resources
// 		- Wide Timer 5B for blinking the entire RGB unit.
// 		- Timer 0B intensity of an RGB element
// 		- Timer 1A intensity of an RGB element
// 		- Timer 1B intensity of an RGB element
//
//*****************************************************************************
static uint32_t  g_ui32Colors[3];
static float g_fIntensity = 0.3f;

//*****************************************************************************
//
//! Wide Timer interrupt to handle blinking effect of the RGB 
//!
//! This function is called by the hardware interrupt controller on a timeout
//! of the wide timer.  This function must be in the NVIC table in the startup
//! file.  When called will toggle the enable flag to turn on or off the entire
//! RGB unit.  This creates a blinking effect.  A wide timer is used since the 
//! blink is intended to be visible to the human eye and thus is expected to 
//! have a frequency between 15 and 0.1 hz. Currently blink duty is fixed at
//! 50%.
//!
//! \return None.
//
//*****************************************************************************
void
RGBBlinkIntHandler(void)
{
    static unsigned long ulFlags;


    //
    // Clear the timer interrupt.
    //
    ROM_TimerIntClear(WTIMER5_BASE, TIMER_TIMB_TIMEOUT);

    //
    // Toggle the flag for the blink timer.
    //
    ulFlags ^= 1;

    if(ulFlags)
    {
        RGBEnable();
    }
    else
    {
        RGBDisable();
    }

}

//*****************************************************************************
//
//! Initializes the Timer and GPIO functionality associated with the RGB LED
//!
//! \param ui32Enable enables RGB immediately if set.
//!
//! This function must be called during application initialization to
//! configure the GPIO pins to which the LEDs are attached.  It enables
//! the port used by the LEDs and configures each color's Timer. It optionally
//! enables the RGB LED by configuring the GPIO pins and starting the timers.
//!
//! \return None.
//
//*****************************************************************************
void
RGBInit(uint32_t ui32Enable)
{
    //
    // Enable the GPIO Port and Timer for each LED
    //
    ROM_SysCtlPeripheralEnable(RED_GPIO_PERIPH);
    ROM_SysCtlPeripheralEnable(RED_TIMER_PERIPH);

    ROM_SysCtlPeripheralEnable(GREEN_GPIO_PERIPH);
    ROM_SysCtlPeripheralEnable(GREEN_TIMER_PERIPH);

    ROM_SysCtlPeripheralEnable(BLUE_GPIO_PERIPH);
    ROM_SysCtlPeripheralEnable(BLUE_TIMER_PERIPH);

    //
    // Configure each timer for output mode
    //
    HWREG(GREEN_TIMER_BASE + TIMER_O_CFG)   = 0x04;
    HWREG(GREEN_TIMER_BASE + TIMER_O_TAMR)  = 0x0A;
    HWREG(GREEN_TIMER_BASE + TIMER_O_TAILR) = 0xFFFF;

    HWREG(BLUE_TIMER_BASE + TIMER_O_CFG)   = 0x04;
    HWREG(BLUE_TIMER_BASE + TIMER_O_TBMR)  = 0x0A;
    HWREG(BLUE_TIMER_BASE + TIMER_O_TBILR) = 0xFFFF;

    HWREG(RED_TIMER_BASE + TIMER_O_CFG)   = 0x04;
    HWREG(RED_TIMER_BASE + TIMER_O_TBMR)  = 0x0A;
    HWREG(RED_TIMER_BASE + TIMER_O_TBILR) = 0xFFFF;

    //
    // Invert the output signals.
    //
    HWREG(RED_TIMER_BASE + TIMER_O_CTL)   |= 0x4000;
    HWREG(GREEN_TIMER_BASE + TIMER_O_CTL)   |= 0x40;
    HWREG(BLUE_TIMER_BASE + TIMER_O_CTL)   |= 0x4000;

    if(ui32Enable)
    {
        RGBEnable();
    }

    //
    // Setup the blink functionality
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER5);
    ROM_TimerConfigure(WTIMER5_BASE, TIMER_CFG_B_PERIODIC | TIMER_CFG_SPLIT_PAIR);
    ROM_TimerLoadSet64(WTIMER5_BASE, 0xFFFFFFFFFFFFFFFF);
    ROM_IntEnable(INT_WTIMER5B);
    ROM_TimerIntEnable(WTIMER5_BASE, TIMER_TIMB_TIMEOUT);


}

//*****************************************************************************
//
//! Enable the RGB LED with already configured timer settings
//!
//! This function or RGBDisable should be called during application
//! initialization to configure the GPIO pins to which the LEDs are attached.
//! This function enables the timers and configures the GPIO pins as timer
//! outputs.
//!
//! \return None.
//
//*****************************************************************************
void
RGBEnable(void)
{

    //
    // Enable timers to begin counting
    //
    ROM_TimerEnable(RED_TIMER_BASE, TIMER_BOTH);
    ROM_TimerEnable(GREEN_TIMER_BASE, TIMER_BOTH);
    ROM_TimerEnable(BLUE_TIMER_BASE, TIMER_BOTH);

    //
    // Reconfigure each LED's GPIO pad for timer control
    //
    ROM_GPIOPinConfigure(GREEN_GPIO_PIN_CFG);
    ROM_GPIOPinTypeTimer(GREEN_GPIO_BASE, GREEN_GPIO_PIN);
    MAP_GPIOPadConfigSet(GREEN_GPIO_BASE, GREEN_GPIO_PIN, GPIO_STRENGTH_8MA_SC,
                     GPIO_PIN_TYPE_STD);

    ROM_GPIOPinConfigure(BLUE_GPIO_PIN_CFG);
    ROM_GPIOPinTypeTimer(BLUE_GPIO_BASE, BLUE_GPIO_PIN);
    MAP_GPIOPadConfigSet(BLUE_GPIO_BASE, BLUE_GPIO_PIN, GPIO_STRENGTH_8MA_SC,
                     GPIO_PIN_TYPE_STD);

    ROM_GPIOPinConfigure(RED_GPIO_PIN_CFG);
    ROM_GPIOPinTypeTimer(RED_GPIO_BASE, RED_GPIO_PIN);
    MAP_GPIOPadConfigSet(RED_GPIO_BASE, RED_GPIO_PIN, GPIO_STRENGTH_8MA_SC,
                     GPIO_PIN_TYPE_STD);
}

//*****************************************************************************
//
//! Disable the RGB LED by configuring the GPIO's as inputs.
//!
//! This function or RGBEnable should be called during application
//! initialization to configure the GPIO pins to which the LEDs are attached.
//! This function disables the timers and configures the GPIO pins as inputs
//! for minimum current draw.
//!
//! \return None.
//
//*****************************************************************************
void
RGBDisable(void)
{
    //
    // Configure the GPIO pads as general purpose inputs.
    //
    ROM_GPIOPinTypeGPIOInput(RED_GPIO_BASE, RED_GPIO_PIN);
    ROM_GPIOPinTypeGPIOInput(GREEN_GPIO_BASE, GREEN_GPIO_PIN);
    ROM_GPIOPinTypeGPIOInput(BLUE_GPIO_BASE, BLUE_GPIO_PIN);

    //
    // Stop the timer counting.
    //
    ROM_TimerDisable(RED_TIMER_BASE, TIMER_BOTH);
    ROM_TimerDisable(GREEN_TIMER_BASE, TIMER_BOTH);
    ROM_TimerDisable(BLUE_TIMER_BASE, TIMER_BOTH);
}

//*****************************************************************************
//
//! Set the output color and intensity.
//!
//! \param pui32RGBColor points to a three element array representing the
//! relative intensity of each color.  Red is element 0, Green is element 1,
//! Blue is element 2. 0x0000 is off.  0xFFFF is fully on.
//!
//! \param fIntensity is used to scale the intensity of all three colors by
//! the same amount.  fIntensity should be between 0.0 and 1.0.  This scale
//! factor is applied to all three colors.
//!
//! This function should be called by the application to set the color and
//! intensity of the RGB LED.
//!
//! \return None.
//
//*****************************************************************************
void
RGBSet(volatile uint32_t * pui32RGBColor,  float fIntensity)
{
    RGBColorSet(pui32RGBColor);
    RGBIntensitySet(fIntensity);
}

//*****************************************************************************
//
//! Set the output color.
//!
//! \param pui32RGBColor points to a three element array representing the
//! relative intensity of each color.  Red is element 0, Green is element 1,
//! Blue is element 2. 0x0000 is off.  0xFFFF is fully on.
//!
//! This function should be called by the application to set the color
//! of the RGB LED.
//!
//! \return None.
//
//*****************************************************************************
void
RGBColorSet(volatile uint32_t * pui32RGBColor)
{
    uint32_t ui32Color[3];
    uint32_t ui32Index;

    for(ui32Index=0; ui32Index < 3; ui32Index++)
    {
        g_ui32Colors[ui32Index] = pui32RGBColor[ui32Index];
        ui32Color[ui32Index] = (uint32_t) (((float) pui32RGBColor[ui32Index]) *
                            g_fIntensity + 0.5f);

        if(ui32Color[ui32Index] > 0xFFFF)
        {
            ui32Color[ui32Index] = 0xFFFF;
        }
    }

    ROM_TimerMatchSet(RED_TIMER_BASE, RED_TIMER, ui32Color[RED]);
    ROM_TimerMatchSet(GREEN_TIMER_BASE, GREEN_TIMER, ui32Color[GREEN]);
    ROM_TimerMatchSet(BLUE_TIMER_BASE, BLUE_TIMER, ui32Color[BLUE]);
}

//*****************************************************************************
//
//! Set the current output intensity.
//!
//! \param fIntensity is used to scale the intensity of all three colors by
//! the same amount.  fIntensity should be between 0.0 and 1.0.  This scale
//! factor is applied individually to all three colors.
//!
//! This function should be called by the application to set the intensity
//! of the RGB LED.
//!
//! \return None.
//
//*****************************************************************************
void
RGBIntensitySet(float fIntensity)
{
    g_fIntensity = fIntensity;
    RGBColorSet(g_ui32Colors);
}

//*****************************************************************************
//
//! Sets the blink rate of the RGB Led
//!
//! \param fRate is the blink rate in hertz.
//!
//! This function controls the blink rate of the RGB LED in auto blink mode.
//! to enable blinking pass a non-zero floating pointer number.  To disable
//! pass 0.0f as the argument. Calling this function will override the current
//! RGBDisable or RGBEnable status.
//!
//! \return None.
//
//*****************************************************************************
void
RGBBlinkRateSet(float fRate)
{
    uint64_t ui64Load;

    if(fRate == 0.0f)
    {
        //
        // Disable the timer and enable the RGB.  If blink rate is zero we
        // assume we want the RGB to be enabled. To disable call RGBDisable
        //
        ROM_TimerDisable(WTIMER5_BASE, TIMER_B);
        RGBEnable();
    }
    else
    {
        //
        // Keep the math in floating pointing until the end so that we keep as
        // much precision as we can.
        //
        ui64Load = (uint64_t) (((float)SysCtlClockGet()) / (fRate * 2.0f));
        ROM_TimerLoadSet(WTIMER5_BASE, TIMER_B, ui64Load);
        ROM_TimerEnable(WTIMER5_BASE, TIMER_B);
    }

}

//*****************************************************************************
//
//! Get the output color.
//!
//! \param pui32RGBColor points to a three element array representing the
//! relative intensity of each color.  Red is element 0, Green is element 1,
//! Blue is element 2. 0x0000 is off.  0xFFFF is fully on. Caller must allocate
//! and pass a pointer to a three element array of uint32_ts.
//!
//! This function should be called by the application to get the current color
//! of the RGB LED.
//!
//! \return None.
//
//*****************************************************************************
void
RGBColorGet(uint32_t * pui32RGBColor)
{
    uint32_t ui32Index;

    for(ui32Index=0; ui32Index < 3; ui32Index++)
    {
        pui32RGBColor[ui32Index] = g_ui32Colors[ui32Index];
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
