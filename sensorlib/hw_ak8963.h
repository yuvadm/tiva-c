//*****************************************************************************
//
// hw_ak8963.h - Macros used when accessing the Asahi Kasei AK8963
//               magnetometer.
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

#ifndef __SENSORLIB_HW_AK8963_H__
#define __SENSORLIB_HW_AK8963_H__

//*****************************************************************************
//
// The following are defines for the AK8963 register addresses.
//
//*****************************************************************************
#define AK8963_O_WIA            0x00        // Device ID register
#define AK8963_O_INFO           0x01        // Information register
#define AK8963_O_ST1            0x02        // Status 1 register
#define AK8963_O_HXL            0x03        // X-axis LSB output register
#define AK8963_O_HXH            0x04        // X-axis MSB output register
#define AK8963_O_HYL            0x05        // Y-axis LSB output register
#define AK8963_O_HYH            0x06        // Y-axis MSB output register
#define AK8963_O_HZL            0x07        // Z-axis LSB output register
#define AK8963_O_HZH            0x08        // Z-axis MSB output register
#define AK8963_O_ST2            0x09        // Status 2 register
#define AK8963_O_CNTL           0x0A        // Control register
#define AK8963_O_CNTL2          0x0B        // Control 2 register
#define AK8963_O_ASTC           0x0C        // Self-test register
#define AK8963_O_I2CDIS         0x0F        // Disable I2C bus interface
#define AK8963_O_ASAX           0x10        // X-axis sensitivity register
#define AK8963_O_ASAY           0x11        // Y-axis sensitivity register
#define AK8963_O_ASAZ           0x12        // Z-axis sensitivity register

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_WIA register.
//
//*****************************************************************************
#define AK8963_WIA_M            0xFF        // Device ID
#define AK8963_WIA_AK8963       0x48        // AK8963
#define AK8963_WIA_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_INFO register.
//
//*****************************************************************************
#define AK8963_INFO_M           0xFF        // Device information value
#define AK8963_INFO_S           0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_ST1 register.
//
//*****************************************************************************
#define AK8963_ST1_DOR          0x02        // Data overrun
#define AK8963_ST1_DRDY         0x01        // Data ready

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_HXL register.
//
//*****************************************************************************
#define AK8963_HXL_M            0xFF        // Output data
#define AK8963_HXL_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_HXH register.
//
//*****************************************************************************
#define AK8963_HXH_M            0xFF        // Output data
#define AK8963_HXH_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_HYL register.
//
//*****************************************************************************
#define AK8963_HYL_M            0xFF        // Output data
#define AK8963_HYL_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_HYH register.
//
//*****************************************************************************
#define AK8963_HYH_M            0xFF        // Output data
#define AK8963_HYH_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_HZL register.
//
//*****************************************************************************
#define AK8963_HZL_M            0xFF        // Output data
#define AK8963_HZL_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_HZH register.
//
//*****************************************************************************
#define AK8963_HZH_M            0xFF        // Output data
#define AK8963_HZH_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_ST2 register.
//
//*****************************************************************************
#define AK8963_ST2_BITM_M       0x10        // Output bit setting
#define AK8963_ST2_BITM_14BIT   0x00        // 14-bit output
#define AK8963_ST2_BITM_16BIT   0x10        // 16-bit output
#define AK8963_ST2_HOFL         0x08        // Magnetic sensor overflow
#define AK8963_ST2_BITM_S       4

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_CNTL register.
//
//*****************************************************************************
#define AK8963_CNTL_BITM_M      0x10        // Output bit setting
#define AK8963_CNTL_BITM_14BIT  0x00        // 14-bit output
#define AK8963_CNTL_BITM_16BIT  0x10        // 16-bit output
#define AK8963_CNTL_MODE_M      0x0F        // Operation mode
#define AK8963_CNTL_MODE_POWER_DOWN                                           \
                                0x00        // Power-down mode
#define AK8963_CNTL_MODE_SINGLE 0x01        // Single measurement mode
#define AK8963_CNTL_MODE_CONT_1 0x02        // Continuous measurement mode 1
                                            // (8Hz)
#define AK8963_CNTL_MODE_EXT_TRIG                                             \
                                0x04        // External trigger measurement
                                            // mode
#define AK8963_CNTL_MODE_CONT_2 0x06        // Continuous measurement mode 2
                                            // (100Hz)
#define AK8963_CNTL_MODE_SELF_TEST                                            \
                                0x08        // Self-test mode
#define AK8963_CNTL_MODE_FUSE_ROM                                             \
                                0x0F        // Fuse ROM access mode
#define AK8963_CNTL_BITM_S      4
#define AK8963_CNTL_MODE_S      0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_CNTL2 register.
//
//*****************************************************************************
#define AK8963_CNTL2_SRST       0x01        // Register reset

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_ASTC register.
//
//*****************************************************************************
#define AK8963_ASTC_SELF        0x40        // Generate magnetic field for
                                            // self-test

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_I2CDIS
// register.
//
//*****************************************************************************
#define AK8963_I2CDIS_I2CDIS    0xFF        // Disable I2C bus interface

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_ASAX register.
//
//*****************************************************************************
#define AK8963_ASAX_M           0xFF        // X-axis sensitivity adjustment
#define AK8963_ASAX_S           0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_ASAY register.
//
//*****************************************************************************
#define AK8963_ASAY_M           0xFF        // Y-axis sensitivity adjustment
#define AK8963_ASAY_S           0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8963_O_ASAZ register.
//
//*****************************************************************************
#define AK8963_ASAZ_M           0xFF        // Z-axis sensitivity adjustment
#define AK8963_ASAZ_S           0

#endif // __SENSORLIB_HW_AK8963_H__
