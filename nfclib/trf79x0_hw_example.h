//*****************************************************************************
//
// trf79x0_hw_example.h - Hardware Pin configuration for TRF79x0 ATB on
// Tiva C Series Snowflake Class silicon. Tailored for DK-tm4c129x, but will
// work for any board with a Snowflake chip with RF Headers.
//
// Copyright (c) 2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the Tiva Firmware Development Package.
//
//*****************************************************************************

#ifndef __TRF79X0_HW_H__
#define __TRF79X0_HW_H__

//*****************************************************************************
//
// Enable the TRF79x0 that will be used with the TM4C129X board
// Enabled = 1, Disabled = 0
//
//*****************************************************************************
#define RF_DAUGHTER_TRF7960         0
#define RF_DAUGHTER_TRF7970         1

//*****************************************************************************
//
// Check for correct definition of RF_DAUGTHER_TRF79X0
//
//*****************************************************************************
#if (RF_DAUGHTER_TRF7960 && RF_DAUGHTER_TRF7970)
#error "Only one TRF79X0 can be defined at the same time."
#elif (!(RF_DAUGHTER_TRF7960 || RF_DAUGHTER_TRF7970))
#error "Define the TRF79X0 to be used, none currently defined."
#endif

//*****************************************************************************
//
// Pin definitions for the DK-TM4C129X development board connections to the
// BoosterPack board.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup nfc_hw NFC Hardware Definitions
//! @{
//! This section covers the definitions that control which hardware is used to
//! communicate with the TRF79x0 EM module.  These defines configure which SSI
//! peripheral is used as well as which pins are assigned to the other
//! connections to the TRF79x0 EM module.  The \b TRF79X0_SSI_* defines are
//! used to specify the SSI peripheral that is used by the application.  The
//! remaining defines specify the pins used by the NFC APIs.  The TRF79x0
//! EM module requires the following signal connections: CLK, RX, TX, CS, ASKOK,
//! EN, EN2, IRQ, MOD.  To configure these signals, three defines must be set
//! for each. For example, for the CS signal, the \ref TRF79X0_CS_BASE,
//! \ref TRF79X0_CS_PERIPH and \ref TRF79X0_CS_PIN defines must be set.
//!
//! \b Example:  CS pin is on GPIO port E pin 1.
//! \verbatim
//!
//! #define TRF79X0_CS_BASE         GPIO_PORTA_BASE
//! #define TRF79X0_CS_PERIPH       SYSCTL_PERIPH_GPIOA
//! #define TRF79X0_CS_PIN          GPIO_PIN_4
//! \endverbatim
//!
//*****************************************************************************

//*****************************************************************************
//
//! The clock rate of the SSI clock specified in Hz.
//!
//! \b Example: 2-MHz SSI data clock.
//!
//! <tt>\#define SSI_CLK_RATE        2000000</tt>
//!
//*****************************************************************************
#define SSI_CLK_RATE            2000000
#define SSI_CLKS_PER_MS         (SSI_CLK_RATE / 1000)
#define STATUS_READS_PER_MS     (SSI_CLKS_PER_MS / 16)
#define SSI_NO_DATA              0

//*****************************************************************************
//
//! Specifies the SSI peripheral for the SSI port that is connected to the
//! TRF79x0 EM board.  The value should be set to SYSCTL_PERIPH_SSIn, where n is
//! the number of the SSI port being used.
//!
//! \b Example: Uses SSI0 peripheral
//!
//! <tt>\#define TRF79X0_SSI_PERIPH      SYSCTL_PERIPH_SSI0</tt>
//!
//*****************************************************************************
#define TRF79X0_SSI_PERIPH      SYSCTL_PERIPH_SSI0

//*****************************************************************************
//
//! Specifies the SSI @a base address for the SSI port that is connected to the
//! TRF79x0 EM board.  The value should be set to SYSCTL_PERIPH_SSIn, where n is
//! the number of the SSI port being used.
//!
//! \b Example: Uses SSI0 peripheral
//!
//! <tt>\#define TRF79X0_SSI_BASE        SSI0_BASE</tt>
//!
//*****************************************************************************
#define TRF79X0_SSI_BASE        SSI0_BASE

//*****************************************************************************
//
// GPIO pin deffinitions for TRF79x0 SSI signals
//
//*****************************************************************************

//
//! Specifies the @a base address of the GPIO port that is connected to the SSI
//! Clock signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI peripheral CLK signal is on GPIO port A.
//!
//! <tt>\#define TRF79X0_CLK_BASE        GPIO_PORTA_BASE</tt>
//
#define TRF79X0_CLK_BASE        GPIO_PORTA_BASE

//
//! Specifies the @a peripheral for the GPIO port that is connected to the SSI
//! Clock signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI peripheral CLK signal is on GPIO port A.
//!
//! <tt>\#define TRF79X0_CLK_PERIPH      SYSCTL_PERIPH_GPIOA</tt>
//
#define TRF79X0_CLK_PERIPH      SYSCTL_PERIPH_GPIOA

//
//! Specifies the GPIO pin that is connected to the SSI
//! Clock signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI peripheral CLK signal is on GPIO pin 2.
//!
//! <tt>\#define TRF79X0_CLK_PIN         GPIO_PIN_2</tt>
//
#define TRF79X0_CLK_PIN         GPIO_PIN_2

//
//! Specifies the GPIO pin that is connected to
//! the SSI Clock signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI Clock signal is on GPIO port A pin 2.
//!
//! <tt>\#define TRF79X0_CLK_CONFIG      GPIO_PA2_SSI0CLK</tt>
//
#define TRF79X0_CLK_CONFIG      GPIO_PA2_SSI0CLK

//
//! Specifies the @a base address of the GPIO port that is connected to the SSI
//! TX signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI peripheral TX signal is on GPIO port A.
//!
//! <tt>\#define TRF79X0_TX_BASE         GPIO_PORTA_BASE</tt>
//
#define TRF79X0_TX_BASE         GPIO_PORTA_BASE

//
//! Specifies the @a peripheral for the GPIO port that is connected to the SSI
//! TX signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI peripheral TX signal is on GPIO port A.
//!
//! <tt>\#define TRF79X0_TX_PERIPH       SYSCTL_PERIPH_GPIOA</tt>
//
#define TRF79X0_TX_PERIPH       SYSCTL_PERIPH_GPIOA

//
//! Specifies the GPIO pin that is connected to the SSI
//! TX signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI peripheral TX signal is on GPIO pin 4.
//!
//! <tt>\#define TRF79X0_TX_PIN          GPIO_PIN_4</tt>
//
#define TRF79X0_TX_PIN          GPIO_PIN_4

//
//! Specifies the GPIO pin that is connected to
//! the SSITX (DAT0) signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI 1 TX signal is on GPIO port A pin 4.
//!
//! <tt>\#define TRF79X0_TX_CONFIG       GPIO_PA4_SSI0XDAT0</tt>
//
#define TRF79X0_TX_CONFIG       GPIO_PA4_SSI0XDAT0

//
//! Specifies the @a base address of the GPIO port that is connected to the SSI
//! RX signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI peripheral RX signal is on GPIO port A.
//!
//! <tt>\#define TRF79X0_RX_BASE         GPIO_PORTA_BASE</tt>
//
#define TRF79X0_RX_BASE         GPIO_PORTA_BASE

//
//! Specifies the @a peripheral for the GPIO port that is connected to the SSI
//! RX signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI peripheral RX signal is on GPIO port A.
//!
//! <tt>\#define TRF79X0_RX_PERIPH       SYSCTL_PERIPH_GPIOA</tt>
//
#define TRF79X0_RX_PERIPH       SYSCTL_PERIPH_GPIOA

//
//! Specifies the GPIO pin that is connected to the SSI
//! RX signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI peripheral RX signal is on GPIO pin 5.
//!
//! <tt>\#define TRF79X0_RX_PIN          GPIO_PIN_5</tt>
//
#define TRF79X0_RX_PIN          GPIO_PIN_5

//
//! Specifies the GPIO pin that is connected to
//! the SSIRX (DAT1) signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI 1 RX signal is on GPIO port A pin 5.
//!
//! <tt>\#define TRF79X0_RX_CONFIG       GPIO_PA5_SSI0XDAT1</tt>
//
#define TRF79X0_RX_CONFIG       GPIO_PA5_SSI0XDAT1

//*****************************************************************************
//
// Hardware connection definitions for the TRF79x0 board.
//
//*****************************************************************************

//
//! Specifies the @a base address of the GPIO port that is connected to the SSI
//! CS signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI CS signal is on GPIO port A.
//!
//! <tt>\#define TRF79X0_CS_BASE         GPIO_PORTA_BASE</tt>
//
#define TRF79X0_CS_BASE         GPIO_PORTA_BASE

//
//! Specifies the @a peripheral for the GPIO port that is connected to the SSI
//! CS signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI CS signal is on GPIO port A.
//!
//! <tt>\#define TRF79X0_CS_PERIPH       SYSCTL_PERIPH_GPIOA</tt>
//
#define TRF79X0_CS_PERIPH       SYSCTL_PERIPH_GPIOA

//
//! Specifies the GPIO pin that is connected to the SSI
//! CS signal on the TRF79x0 EM board.
//!
//! \b Example: The SSI peripheral CS signal is on GPIO pin 4.
//!
//! <tt>\#define TRF79X0_CS_PIN          GPIO_PIN_4</tt>
//
#define TRF79X0_CS_PIN          GPIO_PIN_3

//
//! Specifies the @a base address of the GPIO port that is connected to the EN
//! signal on the TRF79x0 EM board.
//!
//! \b Example: The EN signal is on GPIO port D.
//!
//! <tt>\#define TRF79X0_EN_BASE         GPIO_PORTD_BASE</tt>
//
#define TRF79X0_EN_BASE         GPIO_PORTD_BASE

//
//! Specifies the @a peripheral for the GPIO port that is connected to the EN
//! signal on the TRF79x0 EM board.
//!
//! \b Example: The EN signal is on GPIO port D.
//!
//! <tt>\#define TRF79X0_EN_PERIPH       SYSCTL_PERIPH_GPIOD</tt>
//
#define TRF79X0_EN_PERIPH       SYSCTL_PERIPH_GPIOD

//
//! Specifies the GPIO pin that is connected to the EN pin on the
//! TRF79x0 EM board.
//!
//! \b Example: The EN signal is on GPIO pin 2.
//!
//! <tt>\#define TRF79X0_EN_PIN          GPIO_PIN_2</tt>
//
#define TRF79X0_EN_PIN          GPIO_PIN_2

//
//! Specifies the @a base address of the GPIO port that is connected to the EN2
//! signal on the TRF79x0 EM board.
//!
//! \b Example: The EN2 signal is on GPIO port D.
//!
//! <tt>\#define TRF79X0_EN2_BASE        GPIO_PORTD_BASE</tt>
//
#define TRF79X0_EN2_BASE        GPIO_PORTD_BASE

//
//! Specifies the @a peripheral for the GPIO port that is connected to the EN2
//! signal on the TRF79x0 EM board.
//!
//! \b Example: The EN2 signal is on GPIO port D.
//!
//! <tt>\#define TRF79X0_EN2_PERIPH      SYSCTL_PERIPH_GPIOD</tt>
//
#define TRF79X0_EN2_PERIPH      SYSCTL_PERIPH_GPIOD

//
//! Specifies the GPIO pin that is connected to the EN2 signal on the
//! TRF79x0 EM board.
//!
//! \b Example: The EN2 signal is on GPIO pin 3.
//!
//! <tt>\#define TRF79X0_EN2_PIN         GPIO_PIN_3</tt>
//
#define TRF79X0_EN2_PIN         GPIO_PIN_3

//
//! Specifies the @a base address of the GPIO port that is connected to the
//! ASKOK signal on the TRF79x0 EM board.
//!
//! \b Example: The ASKOK signal is on GPIO port J.
//!
//! <tt>\#define TRF79X0_ASKOK_BASE      GPIO_PORTJ_BASE</tt>
//
#define TRF79X0_ASKOK_BASE      GPIO_PORTJ_BASE

//
//! Specifies the @a peripheral for the GPIO port that is connected to the ASKOK
//! signal on the TRF79x0 EM board.
//!
//! \b Example: The ASKOK signal is on GPIO port J.
//!
//! <tt>\#define TRF79X0_ASKOK_PERIPH    SYSCTL_PERIPH_GPIOJ</tt>
//
#define TRF79X0_ASKOK_PERIPH    SYSCTL_PERIPH_GPIOJ

//
//! Specifies the GPIO pin that is connected to the ASKOK signal on
//! the TRF79x0 EM board.
//!
//! \b Example: The ASKOK signal is on GPIO pin 5.
//!
//! <tt>\#define TRF79X0_ASKOK_PIN       GPIO_PIN_5</tt>
//
#define TRF79X0_ASKOK_PIN       GPIO_PIN_5

//
//! Specifies the @a base address of the GPIO port that is connected to the MOD
//! signal on the TRF79x0 EM board.
//!
//! \b Example: The MOD signal is on GPIO port J.
//!
//! <tt>\#define TRF79X0_MOD_BASE        GPIO_PORTJ_BASE</tt>
//
#define TRF79X0_MOD_BASE        GPIO_PORTJ_BASE

//
//! Specifies the @a peripheral for the GPIO port that is connected to the MOD
//! signal on the TRF79x0 EM board.
//!
//! \b Example: The MOD signal is on GPIO port J.
//!
//! <tt>\#define TRF79X0_MOD_PERIPH      SYSCTL_PERIPH_GPIOJ</tt>
//
#define TRF79X0_MOD_PERIPH      SYSCTL_PERIPH_GPIOJ

//
//! Specifies the GPIO pin that is connected to the MOD signal on the
//! TRF79x0 EM board.
//!
//! \b Example: The MOD signal is on GPIO pin 4.
//!
//! <tt>\#define TRF79X0_MOD_PIN         GPIO_PIN_4</tt>
//
#define TRF79X0_MOD_PIN         GPIO_PIN_4

//
//! Specifies the @a base address of the GPIO port that is connected to the IRQ
//! signal on the TRF79x0 EM board.
//!
//! \b Example: The IRQ signal is on GPIO port J.
//!
//! <tt>\#define TRF79X0_IRQ_BASE        GPIO_PORTJ_BASE</tt>
//
#define TRF79X0_IRQ_BASE        GPIO_PORTJ_BASE

//
//! Specifies the @a peripheral for the GPIO port that is connected to the IRQ
//! signal on the TRF79x0 EM board.
//!
//! \b Example: The IRQ signal is on GPIO port J.
//!
//! <tt>\#define TRF79X0_IRQ_PERIPH      SYSCTL_PERIPH_GPIOJ</tt>
//
#define TRF79X0_IRQ_PERIPH      SYSCTL_PERIPH_GPIOJ

//
//! Specifies the GPIO pin that is connected to the IRQ signal on the
//! TRF79x0 EM board.
//!
//! \b Example: The IRQ signal is on GPIO pin 1.
//!
//! <tt>\#define TRF79X0_IRQ_PIN         GPIO_PIN_1</tt>
//
#define TRF79X0_IRQ_PIN         GPIO_PIN_1

//
//! Specifies GPIO interrupt that is tied to the GPIO port that the IRQ signal
//! is connected to TRF79x0 EM board.
//!
//! \b Example: SSI GPIO interrupt is on GPIO port C.
//!
//! <tt>\#define TRF79X0_IRQ_INT         INT_GPIOC</tt>
//
#define TRF79X0_IRQ_INT         INT_GPIOJ

//
//  Uses Blue LED part of RGB tricolor LED (arbitrary color choice)
//
#define ENABLE_LED_PERIPHERAL SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
#define SET_LED_DIRECTION GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_4 );
#define TURN_ON_LED  GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_4, GPIO_PIN_4);
#define TURN_OFF_LED GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_4, 0);

//*****************************************************************************
//
// Optional LED Defines, useful for boards that have tricolor LED's
//
//*****************************************************************************
#define BOARD_HAS_TRICOLOR_LED              1

#define ENABLE_LED_TRICOLOR_RED_PERIPH      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
#define SET_LED_TRICOLOR_RED_DIRECTION      GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_5 );
#define TURN_ON_LED_TRICOLOR_RED            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_5, GPIO_PIN_5);
#define TURN_OFF_LED_TRICOLOR_RED           GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_5, 0);

#define ENABLE_LED_TRICOLOR_BLUE_PERIPH     SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
#define SET_LED_TRICOLOR_BLUE_DIRECTION     GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_4 );
#define TURN_ON_LED_TRICOLOR_BLUE           GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_4, GPIO_PIN_4);
#define TURN_OFF_LED_TRICOLOR_BLUE          GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_4, 0);

#define ENABLE_LED_TRICOLOR_GREEN_PERIPH    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
#define SET_LED_TRICOLOR_GREEN_DIRECTION    GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_7 );
#define TURN_ON_LED_TRICOLOR_GREEN          GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_7, GPIO_PIN_7);
#define TURN_OFF_LED_TRICOLOR_GREEN         GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_7, 0);

//*****************************************************************************
//
// Macro for IRQ signal from TRF79x0 -> Board
// left in this format for cross platform compatibility.
//
//*****************************************************************************
#define IRQ_IS_SET()        GPIOPinRead(TRF79X0_IRQ_BASE, TRF79X0_IRQ_PIN)

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif // __TRF79X0_HW_H__
