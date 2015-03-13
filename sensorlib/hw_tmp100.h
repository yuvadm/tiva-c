//*****************************************************************************
//
// hw_tmp100.h - Macros used when accessing the Texas Instruments TMP100
//               Temperature Sensor
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

#ifndef __SENSORLIB_HW_TMP100_H__
#define __SENSORLIB_HW_TMP100_H__

//*****************************************************************************
//
// The following are defines for the TMP100 Register Addresses
//
//*****************************************************************************
#define TMP100_O_TEMP           0x00        // Temperature register
#define TMP100_O_CONFIG         0x01        // Configuration register
#define TMP100_O_TEMP_LOW       0x02        // Temperature low register
#define TMP100_O_TEMP_HIGH      0x03        // Temperature high register

//*****************************************************************************
//
// The following are defines for the bit fields in the TMP100_O_TEMP register.
//
//*****************************************************************************
#define TMP100_TEMP_M           0xFFFF      // Temperature data
#define TMP100_TEMP_S           0

//*****************************************************************************
//
// The following are defines for the bit fields in the TMP100_O_CONFIG
// register.
//
//*****************************************************************************
#define TMP100_CONFIG_OS_ALERT  0x80        // Starts a one-shot when written,
                                            // reports alert when read
#define TMP100_CONFIG_RES_M     0x60        // Converter resolution
#define TMP100_CONFIG_RES_9BIT  0x00        // 9-bit resolution (40 ms
                                            // conversion time)
#define TMP100_CONFIG_RES_10BIT 0x20        // 10-bit resolution (80 ms
                                            // conversion time)
#define TMP100_CONFIG_RES_11BIT 0x40        // 11-bit resolution (160 ms
                                            // conversion time)
#define TMP100_CONFIG_RES_12BIT 0x60        // 12-bit resolution (320 ms
                                            // conversion time)
#define TMP100_CONFIG_CR_M      0x18        // Consecutive fault configuration
#define TMP100_CONFIG_FAULT_1   0x00        // 1 consecutive fault
#define TMP100_CONFIG_FAULT_2   0x08        // 2 consecutive faults
#define TMP100_CONFIG_FAULT_4   0x10        // 4 consecutive faults
#define TMP100_CONFIG_FAULT_6   0x18        // 6 consecutive faults
#define TMP100_CONFIG_POL       0x04        // Alert pin polarity
#define TMP100_CONFIG_TM        0x02        // Thermostat mode
#define TMP100_CONFIG_SD        0x01        // Shutdown mode
#define TMP100_CONFIG_RES_S     5
#define TMP100_CONFIG_FAULT_S   3

//*****************************************************************************
//
// The following are defines for the bit fields in the TMP100_O_TEMP_LOW
// register.
//
//*****************************************************************************
#define TMP100_TEMP_LOW_M       0xFFFF      // Temperature low data
#define TMP100_TEMP_LOW_S       0

//*****************************************************************************
//
// The following are defines for the bit fields in the TMP100_O_TEMP_HIGH
// register.
//
//*****************************************************************************
#define TMP100_TEMP_HIGH_M      0xFFFF      // Temperature high data
#define TMP100_TEMP_HIGH_S      0

#endif // __SENSORLIB_HW_TMP100_H__
