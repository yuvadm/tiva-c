//*****************************************************************************
//
// qs-rgb.h - EK-TM4C123GXL Quick Start Application
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



//*****************************************************************************
//
// Globally defined constants
//
//*****************************************************************************
#define APP_SYSTICKS_PER_SEC            32
#define APP_BUTTON_POLL_DIVIDER          8
#define APP_NUM_MANUAL_COLORS            7
#define APP_PI                           3.1415926535897932384626433832795f
#define APP_AUTO_COLOR_STEP              (APP_PI/ 48.0f)
#define APP_INTENSITY_DEFAULT            0.3f
#define APP_AUTO_MODE_TIMEOUT            (APP_SYSTICKS_PER_SEC * 3)
#define APP_HIB_BUTTON_DEBOUNCE          (APP_SYSTICKS_PER_SEC * 3)
#define APP_HIB_FLASH_DURATION           2

#define APP_MODE_NORMAL                  0
#define APP_MODE_HIB                     1
#define APP_MODE_HIB_FLASH               2
#define APP_MODE_AUTO                    3
#define APP_MODE_REMOTE                  4

#define APP_INPUT_BUF_SIZE               128

//*****************************************************************************
//
// Structure typedef to make storing application state data to and from the 
// hibernate battery backed memory simpler.
//      ui32Colors:       [R, G, B] range is 0 to 0xFFFF per color.
//      ui32Mode:         The current application mode, system state variable.
//      ui32Buttons:      bit map representation of buttons being pressed
//      ui32ManualIndex:  Control variable for manual color increment/decrement
//      fColorWheelPos: Control variable to govern color mixing 
//      fIntensity:     Control variable to govern overall brightness of LED
//
//*****************************************************************************
typedef struct
{
    uint32_t ui32Colors[3];
    uint32_t ui32Mode;
    uint32_t ui32Buttons;
    uint32_t ui32ManualIndex;
    uint32_t ui32ModeTimer;
    float fColorWheelPos;
    float fIntensity;

}tAppState;

//*****************************************************************************
//
// Global variables originally defined in qs-rgb.c that are made available to 
// functions in other files.
//
//*****************************************************************************
extern volatile tAppState g_sAppState;

//*****************************************************************************
// 
// Functions defined in qs-rgb.c that are made available to other files.
//
//*****************************************************************************
extern void AppButtonHandler(void);
extern void AppRainbow(uint32_t ui32ForceUpdate);
extern void AppHibernateEnter(void);

