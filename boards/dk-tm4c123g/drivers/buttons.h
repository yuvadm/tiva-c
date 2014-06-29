//*****************************************************************************
//
// buttons.h - Prototypes for the evaluation board buttons driver.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C123G Firmware Package.
//
//*****************************************************************************

#ifndef __BUTTONS_H__
#define __BUTTONS_H__

//*****************************************************************************
//
// Defines for the hardware resources used by the pushbuttons.
//
// The switches are on the following ports/pins:
//
// PM0 - Up
// PM1 - Down
// PM2 - Left
// PM3 - Right
// PM4 - Select/Wake
//
// The switches tie the GPIO to ground, so the GPIOs need to be configured
// with pull-ups, and a value of 0 means the switch is pressed.
//
//*****************************************************************************
#define BUTTONS_GPIO_PERIPH     SYSCTL_PERIPH_GPIOM
#define BUTTONS_GPIO_BASE       GPIO_PORTM_BASE

#define NUM_BUTTONS             5
#define UP_BUTTON               GPIO_PIN_0
#define DOWN_BUTTON             GPIO_PIN_1
#define LEFT_BUTTON             GPIO_PIN_2
#define RIGHT_BUTTON            GPIO_PIN_3
#define SELECT_BUTTON           GPIO_PIN_4

#define ALL_BUTTONS             (LEFT_BUTTON | RIGHT_BUTTON | UP_BUTTON |     \
                                 DOWN_BUTTON | SELECT_BUTTON)

//*****************************************************************************
//
// Useful macros for detecting button events.
//
//*****************************************************************************
#define BUTTON_PRESSED(button, buttons, changed)                              \
        (((button) & (changed)) && ((button) & (buttons)))

#define BUTTON_RELEASED(button, buttons, changed)                             \
        (((button) & (changed)) && !((button) & (buttons)))

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
// Functions exported from buttons.c
//
//*****************************************************************************
extern void ButtonsInit(void); 
extern uint8_t ButtonsPoll(uint8_t *pui8Delta, 
                                 uint8_t *pui8Raw); 

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

#endif // __BUTTONS_H__
