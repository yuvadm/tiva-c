//*****************************************************************************
//
// pinout.c - Function to configure the device pins on the EK-TM4C1294XL.
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

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "drivers/pinout.h"

//*****************************************************************************
//
//! \addtogroup pinout_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Configures the device pins for the standard usages on the EK-TM4C1294XL.
//!
//! \param bEthernet is a boolean used to determine function of Ethernet pins.
//! If true Ethernet pins are  configured as Ethernet LEDs.  If false GPIO are
//! available for application use.
//! \param bUSB is a boolean used to determine function of USB pins. If true USB
//! pins are configured for USB use.  If false then USB pins are available for
//! application use as GPIO.
//!
//! This function enables the GPIO modules and configures the device pins for
//! the default, standard usages on the EK-TM4C1294XL.  Applications that
//! require alternate configurations of the device pins can either not call
//! this function and take full responsibility for configuring all the device
//! pins, or can reconfigure the required device pins after calling this
//! function.
//!
//! \return None.
//
//*****************************************************************************
void
PinoutSet(bool bEthernet, bool bUSB)
{
    //
    // Enable all the GPIO peripherals.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);

    //
    // PA0-1 are used for UART0.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // PB0-1/PD6/PL6-7 are used for USB.
    // PQ4 can be used as a power fault detect on this board but it is not
    // the hardware peripheral power fault input pin.
    //
    if(bUSB)
    {
        HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
        HWREG(GPIO_PORTD_BASE + GPIO_O_CR) = 0xff;
        ROM_GPIOPinConfigure(GPIO_PD6_USB0EPEN);
        ROM_GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
        ROM_GPIOPinTypeUSBDigital(GPIO_PORTD_BASE, GPIO_PIN_6);
        ROM_GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);
        ROM_GPIOPinTypeGPIOInput(GPIO_PORTQ_BASE, GPIO_PIN_4);
    }
    else
    {
        //
        // Keep the default config for most pins used by USB.
        // Add a pull down to PD6 to turn off the TPS2052 switch
        //
        ROM_GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_6);
        MAP_GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_6, GPIO_STRENGTH_2MA,
                             GPIO_PIN_TYPE_STD_WPD);

    }

    //
    // PF0/PF4 are used for Ethernet LEDs.
    //
    if(bEthernet)
    {
        //
        // this app wants to configure for ethernet LED function.
        //
        ROM_GPIOPinConfigure(GPIO_PF0_EN0LED0);
        ROM_GPIOPinConfigure(GPIO_PF4_EN0LED1);

        GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);

    }
    else
    {

        //
        // This app does not want Ethernet LED function so configure as
        // standard outputs for LED driving.
        //
        ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);

        //
        // Default the LEDs to OFF.
        //
        ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, 0);
        MAP_GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4,
                             GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);


    }

    //
    // PJ0 and J1 are used for user buttons
    //
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    ROM_GPIOPinWrite(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0);

    //
    // PN0 and PN1 are used for USER LEDs.
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    MAP_GPIOPadConfigSet(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1,
                             GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);

    //
    // Default the LEDs to OFF.
    //
    ROM_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0);
}

//*****************************************************************************
//
//! This function writes a state to the LED bank.
//!
//! \param ui32LEDMask is a bit mask for which GPIO should be changed by this
//! call.
//! \param ui32LEDValue is the new value to be applied to the LEDs after the
//! ui32LEDMask is applied.
//!
//! The first parameter acts as a mask.  Only bits in the mask that are set
//! will correspond to LEDs that may change.  LEDs with a mask that is not set
//! will not change. This works the same as GPIOPinWrite. After applying the
//! mask the setting for each unmasked LED is written to the corresponding
//! LED port pin via GPIOPinWrite.
//!
//! \return None.
//
//*****************************************************************************
void
LEDWrite(uint32_t ui32LEDMask, uint32_t ui32LEDValue)
{

    //
    // Check the mask and set or clear the LED as directed.
    //
    if(ui32LEDMask & CLP_D1)
    {
        if(ui32LEDValue & CLP_D1)
        {
            GPIOPinWrite(CLP_D1_PORT, CLP_D1_PIN, CLP_D1_PIN);
        }
        else
        {
            GPIOPinWrite(CLP_D1_PORT, CLP_D1_PIN, 0);
        }
    }

    if(ui32LEDMask & CLP_D2)
    {
        if(ui32LEDValue & CLP_D2)
        {
            GPIOPinWrite(CLP_D2_PORT, CLP_D2_PIN, CLP_D2_PIN);
        }
        else
        {
            GPIOPinWrite(CLP_D2_PORT, CLP_D2_PIN, 0);
        }
    }

    if(ui32LEDMask & CLP_D3)
    {
        if(ui32LEDValue & CLP_D3)
        {
            GPIOPinWrite(CLP_D3_PORT, CLP_D3_PIN, CLP_D3_PIN);
        }
        else
        {
            GPIOPinWrite(CLP_D3_PORT, CLP_D3_PIN, 0);
        }
    }

    if(ui32LEDMask & CLP_D4)
    {
        if(ui32LEDValue & CLP_D4)
        {
            GPIOPinWrite(CLP_D4_PORT, CLP_D4_PIN, CLP_D4_PIN);
        }
        else
        {
            GPIOPinWrite(CLP_D4_PORT, CLP_D4_PIN, 0);
        }
    }
}

//*****************************************************************************
//
//! This function reads the state to the LED bank.
//!
//! \param pui32LEDValue is a pointer to where the LED value will be stored.
//!
//! This function reads the state of the CLP LEDs and stores that state
//! information into the variable pointed to by pui32LEDValue.
//!
//! \return None.
//
//*****************************************************************************
void LEDRead(uint32_t *pui32LEDValue)
{
    *pui32LEDValue = 0;

    //
    // Read the pin state and set the variable bit if needed.
    //
    if(GPIOPinRead(CLP_D4_PORT, CLP_D4_PIN))
    {
        *pui32LEDValue |= CLP_D4;
    }

    //
    // Read the pin state and set the variable bit if needed.
    //
    if(GPIOPinRead(CLP_D3_PORT, CLP_D3_PIN))
    {
        *pui32LEDValue |= CLP_D3;
    }

    //
    // Read the pin state and set the variable bit if needed.
    //
    if(GPIOPinRead(CLP_D2_PORT, CLP_D2_PIN))
    {
        *pui32LEDValue |= CLP_D2;
    }

    //
    // Read the pin state and set the variable bit if needed.
    //
    if(GPIOPinRead(CLP_D1_PORT, CLP_D1_PIN))
    {
        *pui32LEDValue |= CLP_D1;
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
