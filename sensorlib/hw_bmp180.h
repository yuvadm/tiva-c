//*****************************************************************************
//
// hw_bmp180.h - Macros used when accessing the Bosch BMP180 barometric
//               pressure sensor.
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

#ifndef __SENSORLIB_HW_BMP180_H__
#define __SENSORLIB_HW_BMP180_H__

//*****************************************************************************
//
// The following are defines for the BMP180 register addresses.
//
//*****************************************************************************
#define BMP180_O_AC1_MSB        0xAA        // AC1 MSB register
#define BMP180_O_AC1_LSB        0xAB        // AC1 LSB register
#define BMP180_O_AC2_MSB        0xAC        // AC2 MSB register
#define BMP180_O_AC2_LSB        0xAD        // AC2 LSB register
#define BMP180_O_AC3_MSB        0xAE        // AC3 MSB register
#define BMP180_O_AC3_LSB        0xAF        // AC3 LSB register
#define BMP180_O_AC4_MSB        0xB0        // AC4 MSB register
#define BMP180_O_AC4_LSB        0xB1        // AC4 LSB register
#define BMP180_O_AC5_MSB        0xB2        // AC5 MSB register
#define BMP180_O_AC5_LSB        0xB3        // AC5 LSB register
#define BMP180_O_AC6_MSB        0xB4        // AC6 MSB register
#define BMP180_O_AC6_LSB        0xB5        // AC6 LSB register
#define BMP180_O_B1_MSB         0xB6        // B1 MSB register
#define BMP180_O_B1_LSB         0xB7        // B1 LSB register
#define BMP180_O_B2_MSB         0xB8        // B2 MSB register
#define BMP180_O_B2_LSB         0xB9        // B2 LSB register
#define BMP180_O_MB_MSB         0xBA        // MB MSB register
#define BMP180_O_MB_LSB         0xBB        // MB LSB register
#define BMP180_O_MC_MSB         0xBC        // MC MSB register
#define BMP180_O_MC_LSB         0xBD        // MC LSB register
#define BMP180_O_MD_MSB         0xBE        // MD MSB register
#define BMP180_O_MD_LSB         0xBF        // MD LSB register
#define BMP180_O_ID             0xD0        // Device ID register
#define BMP180_O_SOFT_RESET     0xE0        // Soft reset register
#define BMP180_O_CTRL_MEAS      0xF4        // Measurement control register
#define BMP180_O_OUT_MSB        0xF6        // ADC data MSB register
#define BMP180_O_OUT_LSB        0xF7        // ADC data LSB register
#define BMP180_O_OUT_XLSB       0xF8        // ADC data XLSB register

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_AC1_MSB
// register.
//
//*****************************************************************************
#define BMP180_AC1_MSB_M        0xFF        // MSB of AC1 calibration
                                            // coefficient
#define BMP180_AC1_MSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_AC1_LSB
// register.
//
//*****************************************************************************
#define BMP180_AC1_LSB_M        0xFF        // LSB of AC1 calibration
                                            // coefficient
#define BMP180_AC1_LSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_AC2_MSB
// register.
//
//*****************************************************************************
#define BMP180_AC2_MSB_M        0xFF        // MSB of AC2 calibration
                                            // coefficient
#define BMP180_AC2_MSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_AC2_LSB
// register.
//
//*****************************************************************************
#define BMP180_AC2_LSB_M        0xFF        // LSB of AC2 calibration
                                            // coefficient
#define BMP180_AC2_LSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_AC3_MSB
// register.
//
//*****************************************************************************
#define BMP180_AC3_MSB_M        0xFF        // MSB of AC3 calibration
                                            // coefficient
#define BMP180_AC3_MSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_AC3_LSB
// register.
//
//*****************************************************************************
#define BMP180_AC3_LSB_M        0xFF        // LSB of AC3 calibration
                                            // coefficient
#define BMP180_AC3_LSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_AC4_MSB
// register.
//
//*****************************************************************************
#define BMP180_AC4_MSB_M        0xFF        // MSB of AC4 calibration
                                            // coefficient
#define BMP180_AC4_MSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_AC4_LSB
// register.
//
//*****************************************************************************
#define BMP180_AC4_LSB_M        0xFF        // LSB of AC4 calibration
                                            // coefficient
#define BMP180_AC4_LSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_AC5_MSB
// register.
//
//*****************************************************************************
#define BMP180_AC5_MSB_M        0xFF        // MSB of AC5 calibration
                                            // coefficient
#define BMP180_AC5_MSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_AC5_LSB
// register.
//
//*****************************************************************************
#define BMP180_AC5_LSB_M        0xFF        // LSB of AC5 calibration
                                            // coefficient
#define BMP180_AC5_LSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_AC6_MSB
// register.
//
//*****************************************************************************
#define BMP180_AC6_MSB_M        0xFF        // MSB of AC6 calibration
                                            // coefficient
#define BMP180_AC6_MSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_AC6_LSB
// register.
//
//*****************************************************************************
#define BMP180_AC6_LSB_M        0xFF        // LSB of AC6 calibration
                                            // coefficient
#define BMP180_AC6_LSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_B1_MSB
// register.
//
//*****************************************************************************
#define BMP180_B1_MSB_M         0xFF        // MSB of B1 calibration
                                            // coefficient
#define BMP180_B1_MSB_S         0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_B1_LSB
// register.
//
//*****************************************************************************
#define BMP180_B1_LSB_M         0xFF        // LSB of B1 calibration
                                            // coefficient
#define BMP180_B1_LSB_S         0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_B2_MSB
// register.
//
//*****************************************************************************
#define BMP180_B2_MSB_M         0xFF        // MSB of B2 calibration
                                            // coefficient
#define BMP180_B2_MSB_S         0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_B2_LSB
// register.
//
//*****************************************************************************
#define BMP180_B2_LSB_M         0xFF        // LSB of B2 calibration
                                            // coefficient
#define BMP180_B2_LSB_S         0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_MB_MSB
// register.
//
//*****************************************************************************
#define BMP180_MB_MSB_M         0xFF        // MSB of MB calibration
                                            // coefficient
#define BMP180_MB_MSB_S         0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_MB_LSB
// register.
//
//*****************************************************************************
#define BMP180_MB_LSB_M         0xFF        // LSB of MB calibration
                                            // coefficient
#define BMP180_MB_LSB_S         0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_MC_MSB
// register.
//
//*****************************************************************************
#define BMP180_MC_MSB_M         0xFF        // MSB of MC calibration
                                            // coefficient
#define BMP180_MC_MSB_S         0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_MC_LSB
// register.
//
//*****************************************************************************
#define BMP180_MC_LSB_M         0xFF        // LSB of MC calibration
                                            // coefficient
#define BMP180_MC_LSB_S         0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_MD_MSB
// register.
//
//*****************************************************************************
#define BMP180_MD_MSB_M         0xFF        // MSB of MD calibration
                                            // coefficient
#define BMP180_MD_MSB_S         0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_MD_LSB
// register.
//
//*****************************************************************************
#define BMP180_MD_LSB_M         0xFF        // LSB of MD calibration
                                            // coefficient
#define BMP180_MD_LSB_S         0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_ID register.
//
//*****************************************************************************
#define BMP180_ID_M             0xFF        // Device ID
#define BMP180_ID_BMP180        0x55        // BMP180
#define BMP180_ID_S             0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_SOFT_RESET
// register.
//
//*****************************************************************************
#define BMP180_SOFT_RESET_M     0xFF        // Soft reset value
#define BMP180_SOFT_RESET_VALUE 0xB6        // Request a soft reset
#define BMP180_SOFT_RESET_S     0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_CTRL_MEAS
// register.
//
//*****************************************************************************
#define BMP180_CTRL_MEAS_OSS_M  0xC0        // Oversampling ratio
#define BMP180_CTRL_MEAS_OSS_1  0x00        // Single sampling
#define BMP180_CTRL_MEAS_OSS_2  0x40        // 2x oversampling
#define BMP180_CTRL_MEAS_OSS_4  0x80        // 4x oversampling
#define BMP180_CTRL_MEAS_OSS_8  0xC0        // 8x oversampling
#define BMP180_CTRL_MEAS_SCO    0x20        // Start of conversion
#define BMP180_CTRL_MEAS_M      0x1F        // Measurement control
#define BMP180_CTRL_MEAS_TEMPERATURE                                          \
                                0x0E        // Temperature measurement
#define BMP180_CTRL_MEAS_PRESSURE                                             \
                                0x14        // Pressure measurement
#define BMP180_CTRL_MEAS_OSS_S  6
#define BMP180_CTRL_MEAS_S      0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_OUT_MSB
// register.
//
//*****************************************************************************
#define BMP180_OUT_MSB_M        0xFF        // Bits [20:13] of the ADC data
#define BMP180_OUT_MSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_OUT_LSB
// register.
//
//*****************************************************************************
#define BMP180_OUT_LSB_M        0xFF        // Bits [12:5] of the ADC data
#define BMP180_OUT_LSB_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the BMP180_O_OUT_XLSB
// register.
//
//*****************************************************************************
#define BMP180_OUT_XLSB_M       0xF8        // Bits [4:0] of the ADC data
#define BMP180_OUT_XLSB_S       3

#endif // __SENSORLIB_HW_BMP180_H__
