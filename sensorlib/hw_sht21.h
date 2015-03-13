//*****************************************************************************
//
// hw_sht21.h - Macros used for accessing the Intersil SHT21 humidity sensor
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

#ifndef __SENSORLIB_HW_SHT21_H__
#define __SENSORLIB_HW_SHT21_H__

//*****************************************************************************
//
// The following are defines for the SHT21 Commands and Registers
//
//*****************************************************************************
#define SHT21_CMD_MEAS_T_HOLD   0xE3        // Measure temperatue with I2C bus
                                            // hold
#define SHT21_CMD_MEAS_RH_HOLD  0xE5        // Measure humidity with I2C bus
                                            // hold
#define SHT21_CMD_WRITE_CONFIG  0xE6        // Write the user config register
#define SHT21_CMD_READ_CONFIG   0xE7        // Read the user config register
#define SHT21_CMD_MEAS_T        0xF3        // Measure temperature polled
#define SHT21_CMD_MEAS_RH       0xF5        // Measure humidity polled
#define SHT21_CMD_SOFT_RESET    0xFE        // Perform a device reset

//*****************************************************************************
//
// The following are defines for the bit fields in the SHT21_CONFIG register.
//
//*****************************************************************************
#define SHT21_CONFIG_RES_M      0x81        // Resolution config for
                                            // temperature and humidity
#define SHT21_CONFIG_RES_12     0x00        // RH 12 bit, T 14 bit
#define SHT21_CONFIG_RES_8      0x01        // RH 8 bit, T 12 bit
#define SHT21_CONFIG_RES_10     0x80        // RH 10 bit, T 13 bit
#define SHT21_CONFIG_RES_11     0x81        // RH 11 bit, T 11 bit
#define SHT21_CONFIG_BATT_M     0x40        // Battery status indicator
#define SHT21_CONFIG_BATT_GOOD  0x00
#define SHT21_CONFIG_BATT_LOW   0x40
#define SHT21_CONFIG_HEATER_M   0x04        // On chip heater for test and
                                            // diagnostics
#define SHT21_CONFIG_HEATER_DISABLE                                           \
                                0x00        // Heater off
#define SHT21_CONFIG_HEATER_ENABLE                                            \
                                0x04        // Heater on
#define SHT21_CONFIG_OTP_RELOAD_M                                             \
                                0x02        // OTP reload control; soft reset
                                            // is preferred
#define SHT21_CONFIG_OTP_RELOAD_ENABLE                                        \
                                0x00        // OTP reload enabled
#define SHT21_CONFIG_OTP_RELOAD_DISABLE                                       \
                                0x02        // OTP reload disabled
#define SHT21_CONFIG_BATT_S     6
#define SHT21_CONFIG_HEATER_S   2
#define SHT21_CONFIG_OTP_RELOAD_S                                             \
                                1
#define SHT21_CONFIG_RES_S      0

#endif // __SENSORLIB_HW_SHT21_H__
