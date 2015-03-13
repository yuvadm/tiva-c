//*****************************************************************************
//
// hw_tmp006.h - Macros used when accessing the Texas Instruments TMP006
//               Infrared Temperature Sensor
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

#ifndef __SENSORLIB_HW_TMP006_H__
#define __SENSORLIB_HW_TMP006_H__

//*****************************************************************************
//
// The following are defines for the TMP006 Register Addresses
//
//*****************************************************************************
#define TMP006_O_VOBJECT        0x00        // Raw object voltage measurement
#define TMP006_O_TAMBIENT       0x01        // Die temperature of the TMP006
#define TMP006_O_CONFIG         0x02        // TMP006 Configuration
#define TMP006_O_MFG_ID         0xFE        // TMP006 Manufacture
                                            // Identification
#define TMP006_O_DEV_ID         0xFF        // TMP006 Device Identification

//*****************************************************************************
//
// The following are defines for the bit fields in the TMP006_O_CONFIG
// register.
//
//*****************************************************************************
#define TMP006_CONFIG_RESET_M   0x8000      // TMP006 device reset
#define TMP006_CONFIG_RESET_ASSERT                                            \
                                0x8000      // Reset TMP006; self clearing
#define TMP006_CONFIG_MODE_M    0x7000      // Operation mode
#define TMP006_CONFIG_MODE_PD   0x0000      // Power down
#define TMP006_CONFIG_MODE_CONT 0x7000      // Continuous sampling
#define TMP006_CONFIG_CR_M      0x0E00      // Conversion rate setting
#define TMP006_CONFIG_CR_4      0x0000      // 4Hz conversion rate
#define TMP006_CONFIG_CR_2      0x0200      // 2Hz conversion rate
#define TMP006_CONFIG_CR_1      0x0400      // 1Hz conversion rate
#define TMP006_CONFIG_CR_0_5    0x0600      // 0.5Hz conversion rate
#define TMP006_CONFIG_CR_0_25   0x0800      // 0.25Hz conversion rate
#define TMP006_CONFIG_EN_DRDY_PIN_M                                           \
                                0x0100      // Enable the DRDY output pin
#define TMP006_CONFIG_DIS_DRDY_PIN                                            \
                                0x0000      // DRDY pin disabled
#define TMP006_CONFIG_EN_DRDY_PIN                                             \
                                0x0100      // DRDY pin enabled
#define TMP006_CONFIG_DRDY_M    0x0080      // Data ready flag
#define TMP006_CONFIG_IN_PROG   0x0000      // Conversion in progress
#define TMP006_CONFIG_DRDY      0x0080      // Conversion complete
#define TMP006_CONFIG_RESET_S   15
#define TMP006_CONFIG_MODE_S    12
#define TMP006_CONFIG_CR_S      9
#define TMP006_CONFIG_EN_DRDY_PIN_S                                           \
                                8
#define TMP006_CONFIG_DRDY_S    7

#endif // __SENSORLIB_HW_TMP006_H__
