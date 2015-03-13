//*****************************************************************************
//
// hw_isl29023.h - Macros used when accessing the Intersil ISL29023 ambient
//                 light sensor
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

#ifndef __SENSORLIB_HW_ISL29023_H__
#define __SENSORLIB_HW_ISL29023_H__

//*****************************************************************************
//
// The following are defines for the ISL29023 Register Addresses
//
//*****************************************************************************
#define ISL29023_O_CMD_I        0x00        // ISL29023 command register one
#define ISL29023_O_CMD_II       0x01
#define ISL29023_O_DATA_OUT_LSB 0x02        // Least significant byte of data
#define ISL29023_O_DATA_OUT_MSB 0x03        // Most significant byte of data
#define ISL29023_O_INT_LT_LSB   0x04        // Interrupt lower threshold least
                                            // significant byte.
#define ISL29023_O_INT_LT_MSB   0x05        // Interrupt lower threshold most
                                            // significant byte.
#define ISL29023_O_INT_HT_LSB   0x06        // Interrupt high threshold least
                                            // significant byte.
#define ISL29023_O_INT_HT_MSB   0x07        // Interrupt high threshold most
                                            // signficant byte

//*****************************************************************************
//
// The following are defines for the bit fields in the ISL29023_O_CMD_I
// register.
//
//*****************************************************************************
#define ISL29023_CMD_I_OP_MODE_M                                              \
                                0xE0        // Operation Mode
#define ISL29023_CMD_I_OP_MODE_POWER_DOWN                                     \
                                0x00        // Power Down the device (Default)
#define ISL29023_CMD_I_OP_MODE_RESERVED_3                                     \
                                0x0E        // RESERVED
#define ISL29023_CMD_I_OP_MODE_ALS_LOW                                        \
                                0x20        // Measure ALS once per integration
                                            // cyle
#define ISL29023_CMD_I_OP_MODE_IR_ONCE                                        \
                                0x40        // Measure IR once
#define ISL29023_CMD_I_OP_MODE_RESERVED_1                                     \
                                0x60        // RESERVED
#define ISL29023_CMD_I_OP_MODE_RESERVED_2                                     \
                                0x80        // RESERVED
#define ISL29023_CMD_I_OP_MODE_ALS_CONT                                       \
                                0xA0        // Measure ambient light sensor
                                            // continuously.
#define ISL29023_CMD_I_OP_MODE_IR_CONT                                        \
                                0xC         // Measure infrared sensor
                                            // continuously
#define ISL29023_CMD_I_INT_FLAG_M                                             \
                                0x04        // Interrupt flag
#define ISL29023_CMD_I_INT_FLAG 0x04        // Interrupt flag
#define ISL29023_CMD_I_INT_PERSIST_M                                          \
                                0x03        // Consecutive measurements outside
                                            // threshold before interrupt
#define ISL29023_CMD_I_INT_PERSIST_1                                          \
                                0x00        // Interrupt on first cycle outside
                                            // threshold
#define ISL29023_CMD_I_INT_PERSIST_4                                          \
                                0x01        // Interrupt on fourth cycle
                                            // oustside threshold
#define ISL29023_CMD_I_INT_PERSIST_8                                          \
                                0x02        // Interrupt on eigth cycle outside
                                            // threshold
#define ISL29023_CMD_I_INT_PERSIST_16                                         \
                                0x03        // Interrupt on sixteenth cycle
                                            // outside threshold
#define ISL29023_CMD_I_OP_MODE_S                                              \
                                5
#define ISL29023_CMD_I_INT_FLAG_S                                             \
                                2
#define ISL_29023_CMD_I_INT_PERSIST_S                                         \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the ISL29023_O_CMD_II
// register.
//
//*****************************************************************************
#define ISL29023_CMD_II_ADC_RES_M                                             \
                                0x0C        // ADC resolution setting
#define ISL29023_CMD_II_ADC_RES_16                                            \
                                0x00        // 16 bit resolution
#define ISL29023_CMD_II_ADC_RES_12                                            \
                                0x04        // 12 bit resolution
#define ISL29023_CMD_II_ADC_RES_8                                             \
                                0x08        // 8 bit resolution
#define ISL29023_CMD_II_ADC_RES_4                                             \
                                0x0C        // 4 bit resolution
#define ISL29023_CMD_II_RANGE_M 0x03        // Sensor Range Setting in Lux
#define ISL29023_CMD_II_RANGE_1K                                              \
                                0x00        // 1000 lux range
#define ISL29023_CMD_II_RANGE_4K                                              \
                                0x01        // 4000 lux range
#define ISL29023_CMD_II_RANGE_16K                                             \
                                0x02        // 16000 lux range
#define ISL29023_CMD_II_RANGE_64K                                             \
                                0x03        // 64000 lux range
#define ISL29023_CMD_II_ADC_RES_S                                             \
                                2
#define ISL29023_CMD_II_RANGE_S 0

#endif // __SENSORLIB_HW_ISL29023_H__
