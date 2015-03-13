//*****************************************************************************
//
// hw_cm3218.h - Macros used for accessing the Capella CM3218 ambient light
//               sensor
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
// This is part of revision 2.1.0.12573 of the Tiva Firmware Development Package.
//
//*****************************************************************************

#ifndef __SENSORLIB_HW_CM3218_H__
#define __SENSORLIB_HW_CM3218_H__

//*****************************************************************************
//
// The following are defines for the CM3218 Commands and Registers
//
//*****************************************************************************
#define CM3218_CMD_CONFIG       0x00        // Configure sensitivity,
                                            // integration time, persistence
                                            // protect, interrupts, and power
#define CM3218_CMD_HIGH_THRESHOLD                                             \
                                0x01        // High threshold window setting
#define CM3218_CMD_LOW_THRESHOLD                                              \
                                0x02        // Low threshold window setting
#define CM3218_CMD_ALS_DATA     0x04        // Read ambient light data

//*****************************************************************************
//
// The following are defines for the bit fields in the CM3218_CMD_CONFIG
// register.
//
//*****************************************************************************
#define CM3218_CMD_CONFIG_SM_M  0x1800      // ALS sensitivity mode selection
#define CM3218_CMD_CONFIG_SM_10 0x0000      // Sensitivty * 1.0
#define CM3218_CMD_CONFIG_SM_20 0x0800      // Sensitivty * 2.0
#define CM3218_CMD_CONFIG_SM_05 0x1000      // Sensitivty * 0.5
#define CM3218_CMD_CONFIG_SM_RSVD                                             \
                                0x1800      // Reserved
#define CM3218_CMD_CONFIG_IT_M  0x00C0      // ALS integration time setting
#define CM3218_CMD_CONFIG_IT_05 0x0000      // integration time 0.5T
#define CM3218_CMD_CONFIG_IT_10 0x0040      // integration time 1.0T (default)
#define CM3218_CMD_CONFIG_IT_20 0x0080      // integration time 2.0T
#define CM3218_CMD_CONFIG_IT_40 0x00C0      // integration time 4.0T
#define CM3218_CMD_CONFIG_PERS_M                                              \
                                0x0030      // ALS persistence protect number
                                            // setting
#define CM3218_CMD_CONFIG_PERS_1                                              \
                                0x0000      // Persistence setting of 1
#define CM3218_CMD_CONFIG_PERS_2                                              \
                                0x0010      // Persistence setting of 2
#define CM3218_CMD_CONFIG_PERS_4                                              \
                                0x0020      // Persistence setting of 4
#define CM3218_CMD_CONFIG_PERS_8                                              \
                                0x0030      // Persistence setting of 8
#define CM3218_CMD_CONFIG_RSVD_M                                              \
                                0x00C       // Reserved
#define CM3218_CMD_CONFIG_RSVD_DEFAULT                                        \
                                0x0004      // Default setting
#define CM3218_CMD_CONFIG_INT_M 0x2         // Interrupt Control
#define CM3218_CMD_CONFIG_INT_DISABLE                                         \
                                0x0         // Disable interrupts
#define CM3218_CMD_CONFIG_INT_ENABLE                                          \
                                0x2         // Enable interrupts
#define CM3218_CMD_CONFIG_POWER_M                                             \
                                0x1         // Power Control
#define CM3218_CMD_CONFIG_POWER_ON                                            \
                                0x0         // Power On
#define CM3218_CMD_CONFIG_POWER_OFF                                           \
                                0x1         // Power Off
#define CM3218_CMD_CONFIG_SM_S  11
#define CM3218_CMD_CONFIG_IT_S  6
#define CM3218_CMD_CONFIG_PERS_S                                              \
                                4
#define CM3218_CMD_CONFIG_RSVD_S                                              \
                                2
#define CM3218_CMD_CONFIG_INT_S 1
#define CM3218_CMD_CONFIG_POWER_S                                             \
                                0

#endif // __SENSORLIB_HW_CM3218_H__
