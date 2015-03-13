//*****************************************************************************
//
// hw_ak8975.h - Macros used when accessing the Asahi Kasei AK8975
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

#ifndef __SENSORLIB_HW_AK8975_H__
#define __SENSORLIB_HW_AK8975_H__

//*****************************************************************************
//
// The following are defines for the AK8975 register addresses.
//
//*****************************************************************************
#define AK8975_O_WIA            0x00        // Device ID register
#define AK8975_O_INFO           0x01        // Information register
#define AK8975_O_ST1            0x02        // Status 1 register
#define AK8975_O_HXL            0x03        // X-axis LSB output register
#define AK8975_O_HXH            0x04        // X-axis MSB output register
#define AK8975_O_HYL            0x05        // Y-axis LSB output register
#define AK8975_O_HYH            0x06        // Y-axis MSB output register
#define AK8975_O_HZL            0x07        // Z-axis LSB output register
#define AK8975_O_HZH            0x08        // Z-axis MSB output register
#define AK8975_O_ST2            0x09        // Status 2 register
#define AK8975_O_CNTL           0x0A        // Control register
#define AK8975_O_ASTC           0x0C        // Self-test register
#define AK8975_O_ASAX           0x10        // X-axis sensitivity register
#define AK8975_O_ASAY           0x11        // Y-axis sensitivity register
#define AK8975_O_ASAZ           0x12        // Z-axis sensitivity register

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_WIA register.
//
//*****************************************************************************
#define AK8975_WIA_M            0xFF        // Device ID
#define AK8975_WIA_AK8975       0x48        // AK8975
#define AK8975_WIA_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_INFO register.
//
//*****************************************************************************
#define AK8975_INFO_M           0xFF        // Device information value
#define AK8975_INFO_S           0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_ST1 register.
//
//*****************************************************************************
#define AK8975_ST1_DRDY         0x01        // Data ready

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_HXL register.
//
//*****************************************************************************
#define AK8975_HXL_M            0xFF        // Output data
#define AK8975_HXL_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_HXH register.
//
//*****************************************************************************
#define AK8975_HXH_M            0xFF        // Output data
#define AK8975_HXH_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_HYL register.
//
//*****************************************************************************
#define AK8975_HYL_M            0xFF        // Output data
#define AK8975_HYL_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_HYH register.
//
//*****************************************************************************
#define AK8975_HYH_M            0xFF        // Output data
#define AK8975_HYH_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_HZL register.
//
//*****************************************************************************
#define AK8975_HZL_M            0xFF        // Output data
#define AK8975_HZL_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_HZH register.
//
//*****************************************************************************
#define AK8975_HZH_M            0xFF        // Output data
#define AK8975_HZH_S            0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_ST2 register.
//
//*****************************************************************************
#define AK8975_ST2_HOFL         0x08        // Magnetic sensor overflow
#define AK8975_ST2_DERR         0x04        // Data error

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_CNTL register.
//
//*****************************************************************************
#define AK8975_CNTL_MODE_M      0x0F        // Operation mode
#define AK8975_CNTL_MODE_POWER_DOWN                                           \
                                0x00        // Power-down mode
#define AK8975_CNTL_MODE_SINGLE 0x01        // Single measurement mode
#define AK8975_CNTL_MODE_SELF_TEST                                            \
                                0x08        // Self-test mode
#define AK8975_CNTL_MODE_FUSE_ROM                                             \
                                0x0F        // Fuse ROM access mode
#define AK8975_CNTL_MODE_S      0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_ASTC register.
//
//*****************************************************************************
#define AK8975_ASTC_SELF        0x40        // Generate magnetic field for
                                            // self-test

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_ASAX register.
//
//*****************************************************************************
#define AK8975_ASAX_M           0xFF        // X-axis sensitivity adjustment
#define AK8975_ASAX_S           0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_ASAY register.
//
//*****************************************************************************
#define AK8975_ASAY_M           0xFF        // Y-axis sensitivity adjustment
#define AK8975_ASAY_S           0

//*****************************************************************************
//
// The following are defines for the bit fields in the AK8975_O_ASAZ register.
//
//*****************************************************************************
#define AK8975_ASAZ_M           0xFF        // Z-axis sensitivity adjustment
#define AK8975_ASAZ_S           0

#endif // __SENSORLIB_HW_AK8975_H__
