//*****************************************************************************
//
// rgb.h - Prototypes for the evaluation board RGB LED driver.
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

#ifndef __RGBLED_H__
#define __RGBLED_H__

//*****************************************************************************
//
// Defines for the hardware resources used by the pushbuttons.
//
// The switches are on the following ports/pins:
//
// PF1 - RED    (632 nanometer)
// PF2 - GREEN  (518 nanometer)
// PF3 - BLUE   (465 nanometer)

//
// The RGB LED is tied up to 5V but since the lowest Vf is 1.75 we can still
// use a General Purpose Timer in pulse out mode.
//
//*****************************************************************************

//
// Indexes into the array of colors
//
#define RED                     0
#define GREEN                   1
#define BLUE                    2

//
// Ratio for percent of full on that should be "true" white.
//
#define RED_WHITE_BALANCE        0.497f
#define GREEN_WHITE_BALANCE      0.6f
#define BLUE_WHITE_BALANCE       1.0f

//
// GPIO, Timer, Peripheral, and Pin assignments for the colors
//
#define RED_GPIO_PERIPH         SYSCTL_PERIPH_GPIOF
#define RED_TIMER_PERIPH        SYSCTL_PERIPH_TIMER0
#define BLUE_GPIO_PERIPH        SYSCTL_PERIPH_GPIOF
#define BLUE_TIMER_PERIPH       SYSCTL_PERIPH_TIMER1
#define GREEN_GPIO_PERIPH       SYSCTL_PERIPH_GPIOF
#define GREEN_TIMER_PERIPH      SYSCTL_PERIPH_TIMER1


#define RED_GPIO_BASE           GPIO_PORTF_BASE
#define RED_TIMER_BASE          TIMER0_BASE
#define BLUE_GPIO_BASE          GPIO_PORTF_BASE
#define BLUE_TIMER_BASE         TIMER1_BASE
#define GREEN_GPIO_BASE         GPIO_PORTF_BASE
#define GREEN_TIMER_BASE        TIMER1_BASE

#define RED_GPIO_PIN            GPIO_PIN_1
#define BLUE_GPIO_PIN           GPIO_PIN_2
#define GREEN_GPIO_PIN          GPIO_PIN_3


#define RED_GPIO_PIN_CFG        GPIO_PF1_T0CCP1
#define BLUE_GPIO_PIN_CFG       GPIO_PF2_T1CCP0
#define GREEN_GPIO_PIN_CFG      GPIO_PF3_T1CCP1

#define RED_TIMER_CFG           TIMER_CFG_B_PWM
#define BLUE_TIMER_CFG          TIMER_CFG_A_PWM
#define GREEN_TIMER_CFG         TIMER_CFG_B_PWM

#define RED_TIMER               TIMER_B
#define BLUE_TIMER              TIMER_A
#define GREEN_TIMER             TIMER_B


//*****************************************************************************
//
// Useful macros
//
//*****************************************************************************

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Functions exported from rgb.c
//
//*****************************************************************************
extern void RGBInit(uint32_t ui32Enable);

extern void RGBEnable(void);
extern void RGBDisable(void);
extern void RGBSet(volatile uint32_t * pui32RGBColor, float fIntensity);
extern void RGBColorSet(volatile uint32_t * pui32RGBColor);

extern void RGBIntensitySet(float fIntensity);
extern void RGBBlinkRateSet(float fRate);
extern void RGBGet(uint32_t * pui32RGBColor, float * pfIntensity);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Prototypes for the globals exported by this driver.
//
//*****************************************************************************

#endif // __RGBLED_H__
