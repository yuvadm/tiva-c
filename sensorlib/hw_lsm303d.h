//*****************************************************************************
//
// hw_lsm303d.h - Macros used when accessing the ST LSM303D
//                accelerometer/magnetometer
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

#ifndef __SENSORLIB_HW_LSM303D_H__
#define __SENSORLIB_HW_LSM303D_H__

//*****************************************************************************
//
// The following are defines for the LSM303D register addresses
//
//*****************************************************************************
#define LSM303D_O_TEMP_OUT_LSB  0x5         // Temperature bits 7-0
#define LSM303D_O_TEMP_OUT_MSB  0x6         // Temperature bits 11-8
#define LSM303D_O_MAG_STATUS    0x7         // Magnetic status register
#define LSM303D_O_MAG_OUT_X_MSB 0x08        // X-axis MSB
#define LSM303D_O_MAG_OUT_X_LSB 0x09        // X-axis LSB
#define LSM303D_O_MAG_OUT_Y_MSB 0x0A        // Y-axis MSB
#define LSM303D_O_MAG_OUT_Y_LSB 0x0B        // Y-axis LSB
#define LSM303D_O_MAG_OUT_Z_MSB 0x0C        // Z-axis MSB
#define LSM303D_O_MAG_OUT_Z_LSB 0x0D        // Z-axis LSB
#define LSM303D_O_WHO_AM_I      0x0F        // ID register, constant 0x49
#define LSM303D_O_MAG_INT_CTRL  0x12        // magnetic control register
#define LSM303D_O_MAG_INT_SRC   0x13        // Interrupt source (status) for
                                            // magnetometer interrupt
#define LSM303D_O_MAG_THS_MSB   0x14        // Threshold MSB
#define LSM303D_O_MAG_THS_LSB   0x15        // Threshold LSB. Even though the
                                            // threshold is expressed in
                                            // absolute value, the device
                                            // detects both positive and
                                            // negative thresholds.
#define LSM303D_O_MAG_OFFSET_X_MSB                                            \
                                0x16        // MSB of Magnetic offset for
                                            // X-axis. The value is expressed
                                            // in 16-bit as 2s complement.
#define LSM303D_O_MAG_OFFSET_X_LSB                                            \
                                0x17        // LSB of Magnetic offset for
                                            // X-axis. The value is expressed
                                            // in 16-bit as 2s complement.
#define LSM303D_O_MAG_OFFSET_Y_MSB                                            \
                                0x18        // MSB of Magnetic offset for
                                            // Y-axis. The value is expressed
                                            // in 16-bit as 2s complement.
#define LSM303D_O_MAG_OFFSET_Y_LSB                                            \
                                0x19        // LSB of Magnetic offset for
                                            // Y-axis. The value is expressed
                                            // in 16-bit as 2s complement.
#define LSM303D_O_MAG_OFFSET_Z_MSB                                            \
                                0x1A        // MSB of Magnetic offset for
                                            // Z-axis. The value is expressed
                                            // in 16-bit as 2s complement.
#define LSM303D_O_MAG_OFFSET_Z_LSB                                            \
                                0x1B        // LSB of Magnetic offset for
                                            // Z-axis. The value is expressed
                                            // in 16-bit as 2s complement.
#define LSM303D_O_ACCEL_REFERENCE_X                                           \
                                0x1C        // Reference value for high-pass
                                            // filter for X-axis acceleration
                                            // data.
#define LSM303D_O_ACCEL_REFERENCE_Y                                           \
                                0x1D        // Reference value for high-pass
                                            // filter for Y-axis acceleration
                                            // data.
#define LSM303D_O_ACCEL_REFERENCE_Z                                           \
                                0x1E        // Reference value for high-pass
                                            // filter for Z-axis acceleration
                                            // data.
#define LSM303D_O_CTRL0         0x1F        // Accel control 0
#define LSM303D_O_CTRL1         0x20        // Control 1 - power settings
#define LSM303D_O_CTRL2         0x21        // Control 2
#define LSM303D_O_CTRL3         0x22        // Control 3
#define LSM303D_O_CTRL4         0x23        // Control 4
#define LSM303D_O_CTRL5         0x24        // Control 5
#define LSM303D_O_CTRL6         0x25        // Control 6
#define LSM303D_O_CTRL7         0x26        // Control 7
#define LSM303D_O_STATUS        0x27        // Status register
#define LSM303D_O_OUT_X_LSB     0x28        // X-axis LSB
#define LSM303D_O_OUT_X_MSB     0x29        // X-axis MSB
#define LSM303D_O_OUT_Y_LSB     0x2A        // Y-axis LSB
#define LSM303D_O_OUT_Y_MSB     0x2B        // Y-axis MSB
#define LSM303D_O_OUT_Z_LSB     0x2C        // Z-axis LSB
#define LSM303D_O_OUT_Z_MSB     0x2D        // Z-axis MSB
#define LSM303D_O_FIFO_CTRL     0x2E        // FIFO control
#define LSM303D_O_FIFO_SRC      0x2F        // FIFO_SRC register
#define LSM303D_O_INT1_CFG      0x30        // INT1 interrupt generation; this
                                            // register only writable after
                                            // boot
#define LSM303D_O_INT1_SRC      0x31        // interrupt source register (read
                                            // only)
#define LSM303D_O_INT1_THS      0x32        // Interrupt 1 threshold
#define LSM303D_O_INT1_DURATION 0x33        // INT1 duration register
#define LSM303D_O_INT2_CFG      0x34        // INT2 interrupt generation; this
                                            // register only writable after
                                            // boot
#define LSM303D_O_INT2_SRC      0x35        // INT2 source register (read only)
#define LSM303D_O_INT2_THS      0x36        // INT2 threshold
#define LSM303D_O_INT2_DURATION 0x37        // INT2 duration register
#define LSM303D_O_CLICK_CFG     0x38        // Click config A register
#define LSM303D_O_CLICK_SRC     0x39        // Click source A
#define LSM303D_O_CLICK_THS     0x3A        // click-click threshold
#define LSM303D_O_TIME_LIMIT    0x3B        // Time Limit A register
#define LSM303D_O_TIME_LATENCY  0x3C        // Time Latency A register; 1 LSB =
                                            // 1/ODR; TLA7 through TLA0 define
                                            // the time interval that starts
                                            // after the first click detection
                                            // where the click detection
                                            // procedure is disabled, in cases
                                            // where the device is configured
                                            // for double click detection
#define LSM303D_O_TIME_WINDOW   0x3D        // Time Window A register; 1 LSB =
                                            // 1/ODR; TW7 through TW0 define
                                            // the maximum interval of time
                                            // that can elapse after the end of
                                            // the latency interval in which
                                            // the click detection procedure
                                            // can start, in cases where the
                                            // device is configured for double
                                            // click detection
#define LSM303D_O_ACT_THS       0x3E        // Sleep to Wake, Return to Sleep
                                            // activation threshold 1LSb = 16mg
#define LSM303D_O_ACT_DUR       0x3F        // Sleep to Wake, Return to Sleep
                                            // duration DUR = (Act_DUR +
                                            // 1)*8/ODR

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_TEMP_OUT_LSB
// register.
//
//*****************************************************************************
#define LSM303D_TEMP_OUT_LSB_LSB_M                                            \
                                0xFF
#define LSM303D_TEMP_OUT_LSB_LSB_S                                            \
                                4

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_TEMP_OUT_MSB
// register.
//
//*****************************************************************************
#define LSM303D_TEMP_OUT_MSB_MSB_M                                            \
                                0x0F
#define LSM303D_TEMP_OUT_MSB_MSB_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_MAG_STATUS
// register.
//
//*****************************************************************************
#define LSM303D_MAG_STATUS_ZYXTMOR_M                                          \
                                0x80        // X,Y,Z axis and temp data overrun
#define LSM303D_MAG_STATUS_ZYXTMOR_TRUE                                       \
                                0x80
#define LSM303D_MAG_STATUS_ZMOR_M                                             \
                                0x40        // Z-axis mag data overrun
#define LSM303D_MAG_STATUS_ZMOR_TRUE                                          \
                                0x40
#define LSM303D_MAG_STATUS_YMOR_M                                             \
                                0x20        // Y-axis mag data overrun
#define LSM303D_MAG_STATUS_YMOR_TRUE                                          \
                                0x20
#define LSM303D_MAG_STATUS_XMOR_M                                             \
                                0x10        // X-axis mag data overrun
#define LSM303D_MAG_STATUS_XMOR_TRUE                                          \
                                0x10
#define LSM303D_MAG_STATUS_ZYXTMD_M                                           \
                                0x08        // X,Y,Z axis and temp data
                                            // available
#define LSM303D_MAG_STATUS_ZYXTMD_AVAIL                                       \
                                0x08
#define LSM303D_MAG_STATUS_ZMD_M                                              \
                                0x04        // New mag data available for Z
                                            // axis
#define LSM303D_MAG_STATUS_ZMD_AVAIL                                          \
                                0x04
#define LSM303D_MAG_STATUS_YMD_M                                              \
                                0x02        // New mag data available for Y
                                            // axis
#define LSM303D_MAG_STATUS_YMD_AVAIL                                          \
                                0x02
#define LSM303D_MAG_STATUS_XMD_M                                              \
                                0x01        // New mag data available for X
                                            // axis
#define LSM303D_MAG_STATUS_XMD_AVAIL                                          \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_MAG_INT_CTRL
// register.
//
//*****************************************************************************
#define LSM303D_MAG_INT_CTRL_XINT_M                                           \
                                0x80        // Enable interrupt recognition on
                                            // X-axis for magnetic data.
#define LSM303D_MAG_INT_CTRL_XINT_ENABLE                                      \
                                0x80
#define LSM303D_MAG_INT_CTRL_YINT_M                                           \
                                0x40        // Enable interrupt recognition on
                                            // Y-axis for magnetic data.
#define LSM303D_MAG_INT_CTRL_YINT_ENABLE                                      \
                                0x40
#define LSM303D_MAG_INT_CTRL_ZINT_M                                           \
                                0x20        // Enable interrupt recognition on
                                            // Z-axis for magnetic data.
#define LSM303D_MAG_INT_CTRL_ZINT_ENABLE                                      \
                                0x20
#define LSM303D_MAG_INT_CTRL_PINCFG_M                                         \
                                0x10        // interrupt pin drive
                                            // configuration
#define LSM303D_MAG_INT_CTRL_PINCFG_PUSHPULL                                  \
                                0x00
#define LSM303D_MAG_INT_CTRL_PINCFG_OPENDRAIN                                 \
                                0x10
#define LSM303D_MAG_INT_CTRL_POLARITY_M                                       \
                                0x08        // interrupt polarity
#define LSM303D_MAG_INT_CTRL_POLARITY_LOW                                     \
                                0x00
#define LSM303D_MAG_INT_CTRL_POLARITY_HIGH                                    \
                                0x08
#define LSM303D_MAG_INT_CTRL_LATCH_M                                          \
                                0x04        // latch interrupt request
#define LSM303D_MAG_INT_CTRL_LATCH_ENABLE                                     \
                                0x04
#define LSM303D_MAG_INT_CTRL_4D_M                                             \
                                0x02        // 4D detection on acceleration
                                            // data is enabled when 6D bit in
                                            // IG_CFG1
#define LSM303D_MAG_INT_CTRL_4D_ENABLE                                        \
                                0x02
#define LSM303D_MAG_INT_CTRL_MI_M                                             \
                                0x01        // Interrupt generation for
                                            // magnetic data
#define LSM303D_MAG_INT_CTRL_MI_ENABLE                                        \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_MAG_INT_SRC
// register.
//
//*****************************************************************************
#define LSM303D_MAG_INT_SRC_M_PTH_X_M                                         \
                                0x80        // Magnetic value on X-axis exceeds
                                            // the threshold on the positive
                                            // side.
#define LSM303D_MAG_INT_SRC_M_PTH_X_ACTIVE                                    \
                                0x80
#define LSM303D_MAG_INT_SRC_M_PTH_Y_M                                         \
                                0x40        // Magnetic value on Y-axis exceeds
                                            // the threshold on the positive
                                            // side.
#define LSM303D_MAG_INT_SRC_M_PTH_Y_ACTIVE                                    \
                                0x40
#define LSM303D_MAG_INT_SRC_M_PTH_Z_M                                         \
                                0x20        // Magnetic value on Z-axis exceeds
                                            // the threshold on the positive
                                            // side.
#define LSM303D_MAG_INT_SRC_M_PTH_Z_ACTIVE                                    \
                                0x20
#define LSM303D_MAG_INT_SRC_M_NTH_X_M                                         \
                                0x10        // Magnetic value on X-axis exceeds
                                            // the threshold on the negative
                                            // side.
#define LSM303D_MAG_INT_SRC_M_NTH_X_ACTIVE                                    \
                                0x10
#define LSM303D_MAG_INT_SRC_M_NTH_Y_M                                         \
                                0x08        // Magnetic value on Y-axis exceeds
                                            // the threshold on the negative
                                            // side.
#define LSM303D_MAG_INT_SRC_M_NTH_Y_ACTIVE                                    \
                                0x08
#define LSM303D_MAG_INT_SRC_M_NTH_Z_M                                         \
                                0x04        // Magnetic value on Z-axis exceeds
                                            // the threshold on the negative
                                            // side.
#define LSM303D_MAG_INT_SRC_M_NTH_Z_ACTIVE                                    \
                                0x04
#define LSM303D_MAG_INT_SRC_MROI_M                                            \
                                0x02        // Internal measurement range
                                            // overflow on magnetic value.
#define LSM303D_MAG_INT_SRC_MROI_ACTIVE                                       \
                                0x02
#define LSM303D_MAG_INT_SRC_MINT_M                                            \
                                0x01        // Magnetic interrupt event. The
                                            // magnetic field value exceeds the
                                            // threshold.
#define LSM303D_MAG_INT_SRC_MINT_ACTIVE                                       \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_CTRL0
// register.
//
//*****************************************************************************
#define LSM303D_CTRL0_BOOT_M    0x80        // Reboot memory content.
#define LSM303D_CTRL0_BOOT_REBOOT                                             \
                                0x80
#define LSM303D_CTRL0_FIFO_M    0x40        // FIFO enable.
#define LSM303D_CTRL0_FIFO_ENABLE                                             \
                                0x40
#define LSM303D_CTRL0_FTH_M     0x20        // FIFO programmable threshold
                                            // enable.
#define LSM303D_CTRL0_FTH_ENABLE                                              \
                                0x20
#define LSM303D_CTRL0_HPCLICK_M 0x04        // High-pass filter enabled for
                                            // click function.
#define LSM303D_CTRL0_HPCLICK_ENABLE                                          \
                                0x04
#define LSM303D_CTRL0_HPIS1_M   0x02        // High-pass filter enabled for
                                            // interrupt generator 1
#define LSM303D_CTRL0_HPIS1_ENABLE                                            \
                                0x02
#define LSM303D_CTRL0_HPIS2_M   0x01        // High-pass filter enabled for
                                            // interrupt generator 2
#define LSM303D_CTRL0_HPIS2_ENABLE                                            \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_CTRL1
// register.
//
//*****************************************************************************
#define LSM303D_CTRL1_AODR_M    0xF0        // accel output data rate selection
#define LSM303D_CTRL1_AODR_PD   0x00        // Power-down mode
#define LSM303D_CTRL1_AODR_3_125HZ                                            \
                                0x10
#define LSM303D_CTRL1_AODR_6_2HZ                                              \
                                0x20
#define LSM303D_CTRL1_AODR_12_5HZ                                             \
                                0x30
#define LSM303D_CTRL1_AODR_25HZ 0x40
#define LSM303D_CTRL1_AODR_50HZ 0x50
#define LSM303D_CTRL1_AODR_100HZ                                              \
                                0x60
#define LSM303D_CTRL1_AODR_200HZ                                              \
                                0x70
#define LSM303D_CTRL1_AODR_400HZ                                              \
                                0x80
#define LSM303D_CTRL1_AODR_800HZ                                              \
                                0x90
#define LSM303D_CTRL1_AODR_1600HZ                                             \
                                0xA0
#define LSM303D_CTRL1_BDU_M     0x08        // Block data update for
                                            // acceleration and magnetic data.
                                            // (0: continuous update; 1: output
                                            // registers not updated until MSB
                                            // and LSB have been read), default
                                            // continuous
#define LSM303D_CTRL1_BDU_CONTINUOUS                                          \
                                0x00
#define LSM303D_CTRL1_BDU_BLOCK 0x08
#define LSM303D_CTRL1_AXIS_M    0x7         // Axis power control
#define LSM303D_CTRL1_AXIS_Y_EN 0x01        // Y-axis enable
#define LSM303D_CTRL1_AXIS_X_EN 0x02        // X-axis enable
#define LSM303D_CTRL1_AXIS_Z_EN 0x04        // Z-axis enable
#define LSM303D_CTRL1_AODR_S    4
#define LSM303D_CTRL1_AXIS_S    0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_CTRL2
// register.
//
//*****************************************************************************
#define LSM303D_CTRL2_AFS_M     0x38        // Acceleration full-scale
                                            // selection.
#define LSM303D_CTRL2_AFS_2G    0x00        // +/- 2G sensitivity
#define LSM303D_CTRL2_AFS_4G    0x08        // +/- 4G sensitivity
#define LSM303D_CTRL2_AFS_6G    0x10        // +/- 6G sensitivity
#define LSM303D_CTRL2_AFS_8G    0x18        // +/- 8G sensitivity
#define LSM303D_CTRL2_AFS_16G   0x40        // +/- 16G sensitivity
#define LSM303D_CTRL2_ABW_M     0xC         // anti-alias filter bandwidth
#define LSM303D_CTRL2_ABW_773HZ 0x00
#define LSM303D_CTRL2_ABW_194HZ 0x40
#define LSM303D_CTRL2_ABW_362HZ 0x80
#define LSM303D_CTRL2_ABW_50HZ  0xC0
#define LSM303D_CTRL2_AST_M     0x02        // Acceleration self-test enable.
#define LSM303D_CTRL2_AST_ENABLE                                              \
                                0x02
#define LSM303D_CTRL2_SIM_M     0x01        // SPI Serial Interface mode
                                            // selection. (default: 4 wire)
#define LSM303D_CTRL2_SIM_4WIRE 0x00
#define LSM303D_CTRL2_SIM_3WIRE 0x01
#define LSM303D_CTRL2_ABW_S     6
#define LSM303D_CTRL2_AFS_S     3

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_CTRL3
// register.
//
//*****************************************************************************
#define LSM303D_CTRL3_INT1_BOOT_M                                             \
                                0x80        // Boot on INT1 enable
#define LSM303D_CTRL3_INT1_BOOT_EN                                            \
                                0x80
#define LSM303D_CTRL3_INT1_CLICK_M                                            \
                                0x40        // Click generator interrupt on
                                            // INT1.
#define LSM303D_CTRL3_INT1_CLICK_EN                                           \
                                0x40
#define LSM303D_CTRL3_INT1_IG1_M                                              \
                                0x20        // Inertial interrupt generator 1
                                            // on INT1
#define LSM303D_CTRL3_INT1_IG1_EN                                             \
                                0x20
#define LSM303D_CTRL3_INT1_IG2_M                                              \
                                0x10        // Inertial interrupt generator 2
                                            // on INT1
#define LSM303D_CTRL3_INT1_IG2_EN                                             \
                                0x10
#define LSM303D_CTRL3_INT1_IGM_M                                              \
                                0x08        // Magnetic interrupt generator on
                                            // INT1
#define LSM303D_CTRL3_INT1_IGM_EN                                             \
                                0x08
#define LSM303D_CTRL3_INT1_ACCEL_DRDY_M                                       \
                                0x04        // Accelerometer data-ready signal
                                            // on INT1
#define LSM303D_CTRL3_INT1_ACCEL_DRDY_EN                                      \
                                0x04
#define LSM303D_CTRL3_INT1_MAG_DRDY_M                                         \
                                0x02        // Magnetometer data-ready signal
                                            // on INT1.
#define LSM303D_CTRL3_INT1_MAG_DRDY_EN                                        \
                                0x02
#define LSM303D_CTRL3_INT1_EMPTY_M                                            \
                                0x01        // FIFO empty indication on INT1
#define LSM303D_CTRL3_INT1_EMPTY_EN                                           \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_CTRL4
// register.
//
//*****************************************************************************
#define LSM303D_CTRL4_INT2_CLICK_M                                            \
                                0x80        // Click generator interrupt on
                                            // INT2
#define LSM303D_CTRL4_INT2_CLICK_EN                                           \
                                0x80
#define LSM303D_CTRL4_INT2_IG1_M                                              \
                                0x40        // Inertial interrupt generator 1
                                            // on INT2
#define LSM303D_CTRL4_INT2_IG1_EN                                             \
                                0x40
#define LSM303D_CTRL4_INT2_IG2_M                                              \
                                0x20        // Inertial interrupt generator 2
                                            // on INT2
#define LSM303D_CTRL4_INT2_IG2_EN                                             \
                                0x20
#define LSM303D_CTRL4_INT2_IGM_M                                              \
                                0x10        // Magnetic interrupt generator on
                                            // INT2
#define LSM303D_CTRL4_INT2_IGM_EN                                             \
                                0x10
#define LSM303D_CTRL4_INT2_ACCEL_DRDY_M                                       \
                                0x08        // Accelerometer data-ready signal
                                            // on INT2
#define LSM303D_CTRL4_INT2_ACCEL_DRDY_EN                                      \
                                0x08
#define LSM303D_CTRL4_INT2_MAG_DRDY_M                                         \
                                0x04        // Magnetometer data-ready signal
                                            // on INT2
#define LSM303D_CTRL4_INT2_MAG_DRDY_EN                                        \
                                0x04
#define LSM303D_CTRL4_INT2_OVR_M                                              \
                                0x02        // FIFO overrun interrupt on INT2
#define LSM303D_CTRL4_INT2_OVR_EN                                             \
                                0x02
#define LSM303D_CTRL4_INT2_FTH_M                                              \
                                0x01        // FIFO threshold interrupt on INT2
#define LSM303D_CTRL4_INT2_FTH_EN                                             \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_CTRL5
// register.
//
//*****************************************************************************
#define LSM303D_CTRL5_TEMP_M    0x80        // Temperature sensor enable.
#define LSM303D_CTRL5_TEMP_EN   0x80
#define LSM303D_CTRL5_MRES_M    0x60        // Magnetic resolution selection
#define LSM303D_CTRL5_MRES_LOW  0x00        // low resolution
#define LSM303D_CTRL5_MRES_HIGH 0x60        // high resolution
#define LSM303D_CTRL5_MODR_M    0x1C        // mag output data rate selection
#define LSM303D_CTRL5_MODR_3_125HZ                                            \
                                0x00
#define LSM303D_CTRL5_MODR_6_2HZ                                              \
                                0x04
#define LSM303D_CTRL5_MODR_12_5HZ                                             \
                                0x08
#define LSM303D_CTRL5_MODR_25HZ 0x0C
#define LSM303D_CTRL5_MODR_50HZ 0x10
#define LSM303D_CTRL5_MODR_100HZ                                              \
                                0x14
#define LSM303D_CTRL5_LIR2_M    0x02        // latch interrupt request on int2
#define LSM303D_CTRL5_LIR2_EN   0x02
#define LSM303D_CTRL5_LIR1_M    0x01        // latch interrupt request on int1
#define LSM303D_CTRL5_LIR1_EN   0x01
#define LSM303D_CTRL5_MRES_S    5
#define LSM303D_CTRL5_MODR_S    2

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_CTRL6
// register.
//
//*****************************************************************************
#define LSM303D_CTRL6_MFS_M     0x60        // magnetic full scale select
#define LSM303D_CTRL6_MFS_2G    0x00        // +/- 2 gauss
#define LSM303D_CTRL6_MFS_4G    0x20        // +/- 4 gauss
#define LSM303D_CTRL6_MFS_8G    0x40        // +/- 8 gauss
#define LSM303D_CTRL6_MFS_12G   0x60        // +/- 16 gauss
#define LSM303D_CTRL6_MFS_S     5

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_CTRL7
// register.
//
//*****************************************************************************
#define LSM303D_CTRL7_AHPM_M    0xC0        // High-pass filter mode selection
                                            // for acceleration data
#define LSM303D_CTRL7_AHPM_NORMAL                                             \
                                0x00
#define LSM303D_CTRL7_AHPM_REFERENCE                                          \
                                0x40
#define LSM303D_CTRL7_AHPM_AUTORESET                                          \
                                0xC0
#define LSM303D_CTRL7_AFDS_M    0x20        // default: internal filter
                                            // bypassed
#define LSM303D_CTRL7_AFDS_BYPASSED                                           \
                                0x00
#define LSM303D_CTRL7_AFDS_FILTERED                                           \
                                0x20
#define LSM303D_CTRL7_TEMP_ONLY_M                                             \
                                0x10        // Temperature sensor only mode,
                                            // mag off
#define LSM303D_CTRL7_TEMP_ONLY_EN                                            \
                                0x10
#define LSM303D_CTRL7_MLP_M     0x04        // Magnetic data low-power mode. If
                                            // this bit is 1, the M_ODR [2:0]
                                            // is set to 3.125 Hz independently
                                            // from the MODR settings.
#define LSM303D_CTRL7_MLP_NORMAL                                              \
                                0x00
#define LSM303D_CTRL7_MLP_LOWPOWER                                            \
                                0x04
#define LSM303D_CTRL7_MD_M      0x3         // Magnetic sensor mode selection
#define LSM303D_CTRL7_MD_CONTINUOUS                                           \
                                0x00
#define LSM303D_CTRL7_MD_SINGLE 0x01
#define LSM303D_CTRL7_MD_POWERDOWN                                            \
                                0x02
#define LSM303D_CTRL7_AHPM_S    6
#define LSM303D_CTRL7_MD_S      0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_STATUS
// register.
//
//*****************************************************************************
#define LSM303D_STATUS_OR_M     0xF0        // Accel Axis data overrun
#define LSM303D_STATUS_OR_X     0x10        // Accel X-axis data overrun
#define LSM303D_STATUS_OR_Y     0x20        // Accel Y-axis data overrun
#define LSM303D_STATUS_OR_Z     0x40        // Accel Z-axis data overrun
#define LSM303D_STATUS_OR_ZYX   0x80        // Accel X, Y, and X data overrun
#define LSM303D_STATUS_DA_M     0xF         // Accel data available
#define LSM303D_STATUS_DA_X     0x01        // Accel X-axis data available
#define LSM303D_STATUS_DA_Y     0x02        // Accel Y-axis data available
#define LSM303D_STATUS_DA_Z     0x04        // Accel Z-axis data available
#define LSM303D_STATUS_DA_ZYX   0x08        // Accel X, Y, and X data available
#define LSM303D_STATUS_OR_S     4
#define LSM303D_STATUS_DA_S     0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_FIFO_CTRL
// register.
//
//*****************************************************************************
#define LSM303D_FIFO_CTRL_MODE_M                                              \
                                0xE0        // FIFO mode setting
#define LSM303D_FIFO_CTRL_MODE_BYPASS                                         \
                                0x00        // Bypass mode
#define LSM303D_FIFO_CTRL_MODE_FIFO                                           \
                                0x20        // FIFO mode
#define LSM303D_FIFO_CTRL_MODE_STREAM                                         \
                                0x40        // Stream mode
#define LSM303D_FIFO_CTRL_MODE_S2F                                            \
                                0x60        // Stream-to-FIFO mode
#define LSM303D_FIFO_CTRL_MODE_B2S                                            \
                                0x80        // Bypass-to-stream mode
#define LSM303D_FIFO_CTRL_THRESH_M                                            \
                                0x1F        // FIFO Threshold setting
#define LSM303D_FIFO_CTRL_MODE_S                                              \
                                5
#define LSM303D_FIFO_CTRL_THRESH_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_FIFO_SRC
// register.
//
//*****************************************************************************
#define LSM303D_FIFO_SRC_FTH_M  0x80        // FIFO threshold is greater than
                                            // or equal to level or less than
                                            // level
#define LSM303D_FIFO_SRC_FTH_LT 0x00
#define LSM303D_FIFO_SRC_FTH_GEQ                                              \
                                0x80
#define LSM303D_FIFO_SRC_OVRN_M 0x40        // overrun status bit
#define LSM303D_FIFO_SRC_OVRN_FILLED                                          \
                                0x40
#define LSM303D_FIFO_SRC_EMPTY_M                                              \
                                0x20        // FIFO empty
#define LSM303D_FIFO_SRC_EMPTY_EMPTY                                          \
                                0x20
#define LSM303D_FIFO_SRC_STORE_SAMPLES_M                                      \
                                0x1F        // FIFO stored data level of the
                                            // unread samples
#define LSM303D_FIFO_SRC_STORE_SAMPLES_S                                      \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_INT1_CFG
// register.
//
//*****************************************************************************
#define LSM303D_INT1_CFG_ANDOR_M                                              \
                                0x80        // AND/OR combination of Interrupt
                                            // events; default value: 0
#define LSM303D_INT1_CFG_ANDOR_OR                                             \
                                0x00
#define LSM303D_INT1_CFG_ANDOR_AND                                            \
                                0x80
#define LSM303D_INT1_CFG_6D_M   0x40        // 6-direction function enabled
#define LSM303D_INT1_CFG_6D_EN  0x40
#define LSM303D_INT1_CFG_ZHI_M  0x20        // enable interrupt generation on
                                            // Z-hi event
#define LSM303D_INT1_CFG_ZHI_EN 0x20
#define LSM303D_INT1_CFG_ZUPE_M 0x20        // enable interrupt generation on
                                            // Z-hi event
#define LSM303D_INT1_CFG_ZUPE_EN                                              \
                                0x20
#define LSM303D_INT1_CFG_ZDOWNE_M                                             \
                                0x10        // enable interrupt generation on
                                            // Z-low event
#define LSM303D_INT1_CFG_ZDOWNE_EN                                            \
                                0x10
#define LSM303D_INT1_CFG_ZLI_M  0x10        // enable interrupt generation on
                                            // Z-low event
#define LSM303D_INT1_CFG_ZLI_EN 0x10
#define LSM303D_INT1_CFG_YUPE_M 0x08        // enable interrupt generation on
                                            // Y-hi event
#define LSM303D_INT1_CFG_YUPE_EN                                              \
                                0x08
#define LSM303D_INT1_CFG_YHI_M  0x08        // enable interrupt generation on
                                            // Y-hi event
#define LSM303D_INT1_CFG_YHI_EN 0x08
#define LSM303D_INT1_CFG_YDOWNE_M                                             \
                                0x04        // enable interrupt generation on
                                            // Y-low event
#define LSM303D_INT1_CFG_YDOWNE_EN                                            \
                                0x04
#define LSM303D_INT1_CFG_YLI_M  0x04        // enable interrupt generation on
                                            // Y-low event
#define LSM303D_INT1_CFG_YLI_EN 0x04
#define LSM303D_INT1_CFG_XUPE_M 0x02        // enable interrupt generation on
                                            // X-hi event
#define LSM303D_INT1_CFG_XUPE_EN                                              \
                                0x02
#define LSM303D_INT1_CFG_XHI_M  0x02        // enable interrupt generation on
                                            // X-hi event
#define LSM303D_INT1_CFG_XHI_EN 0x02
#define LSM303D_INT1_CFG_XDOWNE_M                                             \
                                0x01        // enable interrupt generation on
                                            // X-low event
#define LSM303D_INT1_CFG_XDOWNE_EN                                            \
                                0x01
#define LSM303D_INT1_CFG_XLI_M  0x01        // enable interrupt generation on
                                            // X-low event
#define LSM303D_INT1_CFG_XLI_EN 0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_INT1_SRC
// register.
//
//*****************************************************************************
#define LSM303D_INT1_SRC_ZERO_M 0x80        // This bit must be zero
#define LSM303D_INT1_SRC_ZERO_ZERO                                            \
                                0x00
#define LSM303D_INT1_SRC_ZERO_INVALID                                         \
                                0x80
#define LSM303D_INT1_SRC_IA_M   0x40        // interrupt active
#define LSM303D_INT1_SRC_IA_ACTIVE                                            \
                                0x40
#define LSM303D_INT1_SRC_ZH_M   0x20        // Z-Hi event occurred
#define LSM303D_INT1_SRC_ZH_OCCURRED                                          \
                                0x20
#define LSM303D_INT1_SRC_ZL_M   0x10        // Z-Low event occurred
#define LSM303D_INT1_SRC_ZL_OCCURRED                                          \
                                0x10
#define LSM303D_INT1_SRC_YH_M   0x08        // Y-Hi event occurred
#define LSM303D_INT1_SRC_YH_OCCURRED                                          \
                                0x08
#define LSM303D_INT1_SRC_YL_M   0x04        // Y-Low event occurred
#define LSM303D_INT1_SRC_YL_OCCURRED                                          \
                                0x04
#define LSM303D_INT1_SRC_XH_M   0x02        // X-Hi event occurred
#define LSM303D_INT1_SRC_XH_OCCURRED                                          \
                                0x02
#define LSM303D_INT1_SRC_XL_M   0x01        // X-Low event occurred
#define LSM303D_INT1_SRC_XL_OCCURRED                                          \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_INT1_THS
// register.
//
//*****************************************************************************
#define LSM303D_INT1_THS_ZERO_M 0x80        // This bit must be zero
#define LSM303D_INT1_THS_ZERO_ZERO                                            \
                                0x00
#define LSM303D_INT1_THS_ZERO_INVALID                                         \
                                0x80
#define LSM303D_INT1_THS_THS_M  0x7F        // THS[6-0] Interrupt threshold,
                                            // default 0x0
#define LSM303D_INT1_THS_THS_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_INT1_DURATION
// register.
//
//*****************************************************************************
#define LSM303D_INT1_DURATION_ZERO_M                                          \
                                0x80        // This bit must be zero
#define LSM303D_INT1_DURATION_ZERO_ZERO                                       \
                                0x00
#define LSM303D_INT1_DURATION_ZERO_INVALID                                    \
                                0x80
#define LSM303D_INT1_DURATION_DURATION_M                                      \
                                0x7F        // Duration D[6-0]
#define LSM303D_INT1_DURATION_DURATION_S                                      \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_INT2_CFG
// register.
//
//*****************************************************************************
#define LSM303D_INT2_CFG_ANDOR_M                                              \
                                0x80        // AND/OR combination of Interrupt
                                            // events; default value: 0
#define LSM303D_INT2_CFG_ANDOR_OR                                             \
                                0x00
#define LSM303D_INT2_CFG_ANDOR_AND                                            \
                                0x80
#define LSM303D_INT2_CFG_6D_M   0x40        // 6-direction function enabled
#define LSM303D_INT2_CFG_6D_EN  0x40
#define LSM303D_INT2_CFG_ZHI_M  0x20        // enable interrupt generation on
                                            // Z-hi event
#define LSM303D_INT2_CFG_ZHI_EN 0x20
#define LSM303D_INT2_CFG_ZLI_M  0x10        // enable interrupt generation on
                                            // Z-low event
#define LSM303D_INT2_CFG_ZLI_EN 0x10
#define LSM303D_INT2_CFG_YHI_M  0x08        // enable interrupt generation on
                                            // Y-hi event
#define LSM303D_INT2_CFG_YHI_EN 0x08
#define LSM303D_INT2_CFG_YLI_M  0x04        // enable interrupt generation on
                                            // Y-low event
#define LSM303D_INT2_CFG_YLI_EN 0x04
#define LSM303D_INT2_CFG_XHI_M  0x02        // enable interrupt generation on
                                            // X-hi event
#define LSM303D_INT2_CFG_XHI_EN 0x02
#define LSM303D_INT2_CFG_XLI_M  0x01        // enable interrupt generation on
                                            // X-low event
#define LSM303D_INT2_CFG_XLI_EN 0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_INT2_SRC
// register.
//
//*****************************************************************************
#define LSM303D_INT2_SRC_ZERO_M 0x80        // This bit must be zero
#define LSM303D_INT2_SRC_ZERO_ZERO                                            \
                                0x00
#define LSM303D_INT2_SRC_ZERO_INVALID                                         \
                                0x80
#define LSM303D_INT2_SRC_IA_M   0x40        // interrupt active
#define LSM303D_INT2_SRC_IA_ACTIVE                                            \
                                0x40
#define LSM303D_INT2_SRC_ZH_M   0x20        // Z-Hi event occurred
#define LSM303D_INT2_SRC_ZH_OCCURRED                                          \
                                0x20
#define LSM303D_INT2_SRC_ZL_M   0x10        // Z-Low event occurred
#define LSM303D_INT2_SRC_ZL_OCCURRED                                          \
                                0x10
#define LSM303D_INT2_SRC_YH_M   0x08        // Y-Hi event occurred
#define LSM303D_INT2_SRC_YH_OCCURRED                                          \
                                0x08
#define LSM303D_INT2_SRC_YL_M   0x04        // Y-Low event occurred
#define LSM303D_INT2_SRC_YL_OCCURRED                                          \
                                0x04
#define LSM303D_INT2_SRC_XH_M   0x02        // X-Hi event occurred
#define LSM303D_INT2_SRC_XH_OCCURRED                                          \
                                0x02
#define LSM303D_INT2_SRC_XL_M   0x01        // X-Low event occurred
#define LSM303D_INT2_SRC_XL_OCCURRED                                          \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_INT2_THS
// register.
//
//*****************************************************************************
#define LSM303D_INT2_THS_ZERO_M 0x80        // This bit must be zero
#define LSM303D_INT2_THS_ZERO_ZERO                                            \
                                0x00
#define LSM303D_INT2_THS_ZERO_INVALID                                         \
                                0x80
#define LSM303D_INT2_THS_THS_M  0x7F        // THS[6-0] Interrupt threshold,
                                            // default 0x0
#define LSM303D_INT2_THS_THS_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_INT2_DURATION
// register.
//
//*****************************************************************************
#define LSM303D_INT2_DURATION_ZERO_M                                          \
                                0x80        // This bit must be zero
#define LSM303D_INT2_DURATION_ZERO_ZERO                                       \
                                0x00
#define LSM303D_INT2_DURATION_ZERO_INVALID                                    \
                                0x80
#define LSM303D_INT2_DURATION_DURATION_M                                      \
                                0x7F        // Duration D[6-0]
#define LSM303D_INT2_DURATION_DURATION_S                                      \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_CLICK_CFG
// register.
//
//*****************************************************************************
#define LSM303D_CLICK_CFG_ZD_M  0x20        // Enable interrupt double click on
                                            // Z axis
#define LSM303D_CLICK_CFG_ZD_DIS                                              \
                                0x00
#define LSM303D_CLICK_CFG_ZD_EN 0x20
#define LSM303D_CLICK_CFG_ZS_M  0x10        // Enable interrupt single click on
                                            // Z axis
#define LSM303D_CLICK_CFG_ZS_DIS                                              \
                                0x00
#define LSM303D_CLICK_CFG_ZS_EN 0x10
#define LSM303D_CLICK_CFG_YD_M  0x08        // Enable interrupt double click on
                                            // Y axis
#define LSM303D_CLICK_CFG_YD_DIS                                              \
                                0x00
#define LSM303D_CLICK_CFG_YD_EN 0x08
#define LSM303D_CLICK_CFG_YS_M  0x04        // Enable interrupt single click on
                                            // Y axis
#define LSM303D_CLICK_CFG_YS_DIS                                              \
                                0x00
#define LSM303D_CLICK_CFG_YS_EN 0x04
#define LSM303D_CLICK_CFG_XD_M  0x02        // Enable interrupt double click on
                                            // X axis
#define LSM303D_CLICK_CFG_XD_DIS                                              \
                                0x00
#define LSM303D_CLICK_CFG_XD_EN 0x02
#define LSM303D_CLICK_CFG_XS_M  0x01        // Enable interrupt single click on
                                            // X axis
#define LSM303D_CLICK_CFG_XS_DIS                                              \
                                0x00
#define LSM303D_CLICK_CFG_XS_EN 0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_CLICK_SRC
// register.
//
//*****************************************************************************
#define LSM303D_CLICK_SRC_IA_M  0x40        // Interrupt pending
#define LSM303D_CLICK_SRC_IA_NONE                                             \
                                0x00
#define LSM303D_CLICK_SRC_IA_PENDING                                          \
                                0x40
#define LSM303D_CLICK_SRC_DCLICK_M                                            \
                                0x20        // double click-click enable
#define LSM303D_CLICK_SRC_DCLICK_DIS                                          \
                                0x00
#define LSM303D_CLICK_SRC_DCLICK_EN                                           \
                                0x20
#define LSM303D_CLICK_SRC_SCLICK_M                                            \
                                0x10        // single click-click enable
#define LSM303D_CLICK_SRC_SCLICK_DIS                                          \
                                0x00
#define LSM303D_CLICK_SRC_SCLICK_EN                                           \
                                0x10
#define LSM303D_CLICK_SRC_SIGN_M                                              \
                                0x08        // click-click sign
#define LSM303D_CLICK_SRC_SIGN_POSITIVE                                       \
                                0x00
#define LSM303D_CLICK_SRC_SIGN_NEGATIVE                                       \
                                0x08
#define LSM303D_CLICK_SRC_AXIS_M                                              \
                                0x7         // Axis click detection
#define LSM303D_CLICK_SRC_AXIS_X                                              \
                                0x01        // X click-click detection
#define LSM303D_CLICK_SRC_AXIS_Y                                              \
                                0x02        // Y click-click detection
#define LSM303D_CLICK_SRC_AXIS_Z                                              \
                                0x04        // Z click-click detection
#define LSM303D_CLICK_SRC_AXIS_S                                              \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_CLICK_THS
// register.
//
//*****************************************************************************
#define LSM303D_CLICK_THS_THS_M 0x7F        // Threshold; 1 LSB = full-scale /
                                            // 128; THS6 through THS0 define
                                            // the threshold which is used by
                                            // the system to start the click
                                            // detection procedure; the
                                            // threshold value is expressed
                                            // over 7 bits as an unsigned
                                            // number
#define LSM303D_CLICK_THS_THS_S 0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_TIME_LIMIT
// register.
//
//*****************************************************************************
#define LSM303D_TIME_LIMIT_TLI_M                                              \
                                0x7F        // Time Limit; 1 LSB = 1/ODR; TLI7
                                            // through TLI0 define the maximum
                                            // time interval that can elapse
                                            // between the start of the click
                                            // detection procedure (the
                                            // acceleration on the selected
                                            // channel exceeds the programmed
                                            // threshold) and when the
                                            // acceleration goes back below the
                                            // threshold
#define LSM303D_TIME_LIMIT_TLI_S                                              \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_ACT_THS
// register.
//
//*****************************************************************************
#define LSM303D_ACT_THS_THRESHOLD_M                                           \
                                0x7F
#define LSM303D_ACT_THS_THRESHOLD_S                                           \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303D_O_ACT_DUR
// register.
//
//*****************************************************************************
#define LSM303D_ACT_DUR_THRESHOLD_M                                           \
                                0x7F
#define LSM303D_ACT_DUR_THRESHOLD_S                                           \
                                0

#endif // __SENSORLIB_HW_LSM303D_H__
