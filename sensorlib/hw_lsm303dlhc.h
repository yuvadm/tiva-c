//*****************************************************************************
//
// hw_lsm303dlhc.h - Macros used when accessing the ST LSM303DLHC accel/mag
//                   combo
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

#ifndef __SENSORLIB_HW_LSM303DLHC_H__
#define __SENSORLIB_HW_LSM303DLHC_H__

//*****************************************************************************
//
// The following are defines for the LSM303DLHC register addresses
//
//*****************************************************************************
#define LSM303DLHC_O_MAG_CRA    0x0         // Magnetometer control
#define LSM303DLHC_O_MAG_CRB    0x01        // Gain configuration
#define LSM303DLHC_O_MAG_MR     0x02        // Mode configuration
#define LSM303DLHC_O_MAG_OUT_X_MSB                                            \
                                0x03        // X-axis MSB
#define LSM303DLHC_O_MAG_OUT_X_LSB                                            \
                                0x04        // X-axis LSB
#define LSM303DLHC_O_MAG_OUT_Y_MSB                                            \
                                0x05        // Y-axis MSB
#define LSM303DLHC_O_MAG_OUT_Y_LSB                                            \
                                0x06        // Y-axis LSB
#define LSM303DLHC_O_MAG_OUT_Z_MSB                                            \
                                0x07        // Z-axis MSB
#define LSM303DLHC_O_MAG_OUT_Z_LSB                                            \
                                0x08        // Z-axis LSB
#define LSM303DLHC_O_MAG_SR     0x09        // Status register
#define LSM303DLHC_O_MAG_IRA    0x0A
#define LSM303DLHC_O_MAG_IRB    0x0B
#define LSM303DLHC_O_MAG_IRC    0x0C
#define LSM303DLHC_O_CTRL1      0x20        // Control 1 - power settings
#define LSM303DLHC_O_CTRL2      0x21        // Control 2
#define LSM303DLHC_O_CTRL3      0x22        // Control 3
#define LSM303DLHC_O_CTRL4      0x23        // Control 4
#define LSM303DLHC_O_CTRL5      0x24        // Control 5
#define LSM303DLHC_O_CTRL6      0x25        // Control 6
#define LSM303DLHC_O_REFERENCE  0x26        // Reference/Datacapture_A
#define LSM303DLHC_O_STATUS     0x27        // Status register
#define LSM303DLHC_O_OUT_X_LSB  0x28        // X-axis LSB
#define LSM303DLHC_O_OUT_X_MSB  0x29        // X-axis MSB
#define LSM303DLHC_O_OUT_Y_LSB  0x2A        // Y-axis LSB
#define LSM303DLHC_O_OUT_Y_MSB  0x2B        // Y-axis MSB
#define LSM303DLHC_O_OUT_Z_LSB  0x2C        // Z-axis LSB
#define LSM303DLHC_O_OUT_Z_MSB  0x2D        // Z-axis MSB
#define LSM303DLHC_O_FIFO_CTRL  0x2E        // FIFO control
#define LSM303DLHC_O_FIFO_SRC   0x2F        // FIFO_SRC register
#define LSM303DLHC_O_INT1_CFG_A 0x30        // INT1 interrupt generation; this
                                            // register only writable after
                                            // boot
#define LSM303DLHC_O_INT1_SRC_A 0x31        // interrupt source register (read
                                            // only)
#define LSM303DLHC_O_MAG_TEMP_OUT_MSB                                         \
                                0x31        // Temperature bits 11-4
#define LSM303DLHC_O_MAG_TEMP_OUT_LSB                                         \
                                0x32        // Temperature bits 3-0
#define LSM303DLHC_O_INT1_THS_A 0x32        // Interrupt 1 threshold
#define LSM303DLHC_O_INT1_DURATION_A                                          \
                                0x33        // INT1 duration register
#define LSM303DLHC_O_INT2_CFG_A 0x34        // INT2 interrupt generation; this
                                            // register only writable after
                                            // boot
#define LSM303DLHC_O_INT2_SRC_A 0x35        // INT2 source register (read only)
#define LSM303DLHC_O_INT2_THS_A 0x36        // INT2 threshold
#define LSM303DLHC_O_INT2_DURATION_A                                          \
                                0x37        // INT2 duration register
#define LSM303DLHC_O_CLICK_CFG_A                                              \
                                0x38        // Click config A register
#define LSM303DLHC_O_CLICK_SRC_A                                              \
                                0x39        // Click source A
#define LSM303DLHC_O_CLICK_THS_A                                              \
                                0x3A        // click-click threshold
#define LSM303DLHC_O_TIME_LIMIT_A                                             \
                                0x3B        // Time Limit A register
#define LSM303DLHC_O_TIME_LATENCY_A                                           \
                                0x3C        // Time Latency A register; 1 LSB =
                                            // 1/ODR; TLA7 through TLA0 define
                                            // the time interval that starts
                                            // after the first click detection
                                            // where the click detection
                                            // procedure is disabled, in cases
                                            // where the device is configured
                                            // for double click detection
#define LSM303DLHC_O_TIME_WINDOW_A                                            \
                                0x3D        // Time Window A register; 1 LSB =
                                            // 1/ODR; TW7 through TW0 define
                                            // the maximum interval of time
                                            // that can elapse after the end of
                                            // the latency interval in which
                                            // the click detection procedure
                                            // can start, in cases where the
                                            // device is configured for double
                                            // click detection

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_MAG_CRA
// register.
//
//*****************************************************************************
#define LSM303DLHC_MAG_CRA_TEMP_M                                             \
                                0x80        // Temperature sensor enable
#define LSM303DLHC_MAG_CRA_TEMP_DIS                                           \
                                0x00
#define LSM303DLHC_MAG_CRA_TEMP_EN                                            \
                                0x80
#define LSM303DLHC_MAG_CRA_DO_M 0x1C        // Data output rate
#define LSM303DLHC_MAG_CRA_DO_0_75HZ                                          \
                                0x00        // 0.75 Hz
#define LSM303DLHC_MAG_CRA_DO_1_5HZ                                           \
                                0x04        // 1.5 Hz
#define LSM303DLHC_MAG_CRA_DO_3_0HZ                                           \
                                0x08        // 3.0 Hz
#define LSM303DLHC_MAG_CRA_DO_7_5HZ                                           \
                                0x0C        // 7.5 Hz
#define LSM303DLHC_MAG_CRA_DO_15HZ                                            \
                                0x10        // 15 Hz
#define LSM303DLHC_MAG_CRA_DO_30HZ                                            \
                                0x14        // 30 Hz
#define LSM303DLHC_MAG_CRA_DO_75HZ                                            \
                                0x18        // 75 Hz
#define LSM303DLHC_MAG_CRA_DO_220HZ                                           \
                                0x1C        // 220 Hz
#define LSM303DLHC_MAG_CRA_DO_S 2

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_MAG_CRB
// register.
//
//*****************************************************************************
#define LSM303DLHC_MAG_CRB_GAIN_M                                             \
                                0xE0        // Gain selection
#define LSM303DLHC_MAG_CRB_GAIN_1_3GAUSS                                      \
                                0x20        // +/- 1.3 gauss, 1100 LSB/gauss
#define LSM303DLHC_MAG_CRB_GAIN_1_9GAUSS                                      \
                                0x40        // +/- 1.9 gauss, 855 LSB/gauss
#define LSM303DLHC_MAG_CRB_GAIN_2_5GAUSS                                      \
                                0x60        // +/- 2.5 gauss, 670 LSB/gauss
#define LSM303DLHC_MAG_CRB_GAIN_4_0GAUSS                                      \
                                0x80        // +/- 4.0 gauss, 450 LSB/guass
#define LSM303DLHC_MAG_CRB_GAIN_4_7GAUSS                                      \
                                0xA0        // +/- 4.7 gauss, 400 LSB/gauss
#define LSM303DLHC_MAG_CRB_GAIN_5_6GAUSS                                      \
                                0xC0        // +/- 5.6 gauss, 330 LSB/gauss
#define LSM303DLHC_MAG_CRB_GAIN_8_1GAUSS                                      \
                                0xE0        // +/- 8.1 gauss, 230 LSB/gauss
#define LSM303DLHC_MAG_CRB_GAIN_S                                             \
                                5

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_MAG_MR
// register.
//
//*****************************************************************************
#define LSM303DLHC_MAG_MR_MODE_M                                              \
                                0x3         // Mode select bits
#define LSM303DLHC_MAG_MR_MODE_CONTINUOUS                                     \
                                0x00        // Continuous conversion mode
#define LSM303DLHC_MAG_MR_MODE_SINGLE                                         \
                                0x01        // Single conversion mode
#define LSM303DLHC_MAG_MR_MODE_SLEEP                                          \
                                0x02        // Sleep mode
#define LSM303DLHC_MAG_MR_MODE_S                                              \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_MAG_SR
// register.
//
//*****************************************************************************
#define LSM303DLHC_MAG_SR_LOCK_M                                              \
                                0x02        // Data output register lock; once
                                            // a new set of measurements is
                                            // available, this bit is set when
                                            // the first magnetic field data
                                            // register has been read
#define LSM303DLHC_MAG_SR_LOCK_LOCKED                                         \
                                0x02
#define LSM303DLHC_MAG_SR_DATA_M                                              \
                                0x01        // Data ready bit; this bit is set
                                            // when a new set of measures are
                                            // available
#define LSM303DLHC_MAG_SR_DATA_READ                                           \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_MAG_IRA
// register.
//
//*****************************************************************************
#define LSM303DLHC_MAG_IRA_CONSTANT_M                                         \
                                0xFF
#define LSM303DLHC_MAG_IRA_CONSTANT_VAL                                       \
                                0x48
#define LSM303DLHC_MAG_IRA_CONSTANT_S                                         \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_MAG_IRB
// register.
//
//*****************************************************************************
#define LSM303DLHC_MAG_IRB_CONSTANT_M                                         \
                                0xFF
#define LSM303DLHC_MAG_IRB_CONSTANT_VAL                                       \
                                0x34
#define LSM303DLHC_MAG_IRB_CONSTANT_S                                         \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_MAG_IRC
// register.
//
//*****************************************************************************
#define LSM303DLHC_MAG_IRC_CONSTANT_M                                         \
                                0xFF
#define LSM303DLHC_MAG_IRC_CONSTANT_VAL                                       \
                                0x33
#define LSM303DLHC_MAG_IRC_CONSTANT_S                                         \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_CTRL1
// register.
//
//*****************************************************************************
#define LSM303DLHC_CTRL1_ODR_M  0xF0        // data rate selection
#define LSM303DLHC_CTRL1_ODR_PD 0x00        // Power-down mode
#define LSM303DLHC_CTRL1_ODR_1HZ                                              \
                                0x10
#define LSM303DLHC_CTRL1_ODR_10HZ                                             \
                                0x20
#define LSM303DLHC_CTRL1_ODR_25HZ                                             \
                                0x30
#define LSM303DLHC_CTRL1_ODR_50HZ                                             \
                                0x40
#define LSM303DLHC_CTRL1_ODR_100HZ                                            \
                                0x50
#define LSM303DLHC_CTRL1_ODR_200HZ                                            \
                                0x60
#define LSM303DLHC_CTRL1_ODR_400HZ                                            \
                                0x70
#define LSM303DLHC_CTRL1_ODR_1620HZ                                           \
                                0x80        // Low power mode
#define LSM303DLHC_CTRL1_ODR_5376HZ                                           \
                                0x90        // 1.344KHz normal, 5.376KHz low
                                            // power
#define LSM303DLHC_CTRL1_POWER_M                                              \
                                0x08        // Power control
#define LSM303DLHC_CTRL1_POWER_LOWPOW                                         \
                                0x00
#define LSM303DLHC_CTRL1_POWER_NORMAL                                         \
                                0x08
#define LSM303DLHC_CTRL1_AXIS_M 0x7         // Axis power control
#define LSM303DLHC_CTRL1_AXIS_Y_EN                                            \
                                0x01        // Y-axis enable
#define LSM303DLHC_CTRL1_AXIS_X_EN                                            \
                                0x02        // X-axis enable
#define LSM303DLHC_CTRL1_AXIS_Z_EN                                            \
                                0x04        // Z-axis enable
#define LSM303DLHC_CTRL1_ODR_S  4
#define LSM303DLHC_CTRL1_AXIS_S 0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_CTRL2
// register.
//
//*****************************************************************************
#define LSM303DLHC_CTRL2_HPMODE_M                                             \
                                0xC0        // high pass filter mode selection
#define LSM303DLHC_CTRL2_HPMODE_NORMAL_RESET                                  \
                                0x00        // normal mode (reset reading
                                            // HP_RESET_FILTER)
#define LSM303DLHC_CTRL2_HPMODE_REFERENCE                                     \
                                0x40        // reference signal for filtering
#define LSM303DLHC_CTRL2_HPMODE_NORMAL                                        \
                                0x80        // Normal mode
#define LSM303DLHC_CTRL2_HPMODE_AUTORESET                                     \
                                0xC0        // autoreset on interrupt event
#define LSM303DLHC_CTRL2_HPCUTOFF_M                                           \
                                0x30        // high pass filter cut-off
                                            // frequency selection
#define LSM303DLHC_CTRL2_FDS_M  0x08        // Filtered data selection
#define LSM303DLHC_CTRL2_FDS_BYPASSED                                         \
                                0x00
#define LSM303DLHC_CTRL2_FDS_FILTERED                                         \
                                0x08
#define LSM303DLHC_CTRL2_HPCLICK_M                                            \
                                0x04        // High pass filter enabled for
                                            // CLICK function
#define LSM303DLHC_CTRL2_HPCLICK_BYPASSED                                     \
                                0x00
#define LSM303DLHC_CTRL2_HPCLICK_FILTERED                                     \
                                0x04
#define LSM303DLHC_CTRL2_HPIS2_M                                              \
                                0x02        // High pass filter enabled for AOI
                                            // function on Interrupt 2
#define LSM303DLHC_CTRL2_HPIS2_BYPASSED                                       \
                                0x00
#define LSM303DLHC_CTRL2_HPIS2_FILTERED                                       \
                                0x02
#define LSM303DLHC_CTRL2_HPIS1_M                                              \
                                0x01        // High pass filter enabled for AOI
                                            // function on Interrupt 1
#define LSM303DLHC_CTRL2_HPIS1_BYPASSED                                       \
                                0x00
#define LSM303DLHC_CTRL2_HPIS1_FILTERED                                       \
                                0x01
#define LSM303DLHC_CTRL2_HPMODE_S                                             \
                                6
#define LSM303DLHC_CTRL2_HPCUTOFF_S                                           \
                                4

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_CTRL3
// register.
//
//*****************************************************************************
#define LSM303DLHC_CTRL3_I1CLICK_M                                            \
                                0x80        // CLICK on INT1
#define LSM303DLHC_CTRL3_I1CLICK_DIS                                          \
                                0x00
#define LSM303DLHC_CTRL3_I1CLICK_EN                                           \
                                0x80
#define LSM303DLHC_CTRL3_I1AOI1_M                                             \
                                0x40        // AOI1 on INT1
#define LSM303DLHC_CTRL3_I1AOI1_DIS                                           \
                                0x00
#define LSM303DLHC_CTRL3_I1AOI1_EN                                            \
                                0x40
#define LSM303DLHC_CTRL3_I1AOI2_M                                             \
                                0x20        // AOI2 on INT1
#define LSM303DLHC_CTRL3_I1AOI2_DIS                                           \
                                0x00
#define LSM303DLHC_CTRL3_I1AOI2_EN                                            \
                                0x20
#define LSM303DLHC_CTRL3_I1DRDY1_M                                            \
                                0x10        // DRDY1 on INT1
#define LSM303DLHC_CTRL3_I1DRDY1_DIS                                          \
                                0x00
#define LSM303DLHC_CTRL3_I1DRDY1_EN                                           \
                                0x10
#define LSM303DLHC_CTRL3_I1DRDY2_M                                            \
                                0x08        // DRDY2 on INT1
#define LSM303DLHC_CTRL3_I1DRDY2_DIS                                          \
                                0x00
#define LSM303DLHC_CTRL3_I1DRDY2_EN                                           \
                                0x08
#define LSM303DLHC_CTRL3_I1WTM_M                                              \
                                0x04        // FIFO watermark interrupt on INT1
#define LSM303DLHC_CTRL3_I1WTM_DIS                                            \
                                0x00
#define LSM303DLHC_CTRL3_I1WTM_EN                                             \
                                0x04
#define LSM303DLHC_CTRL3_I1OVERRUN_M                                          \
                                0x02        // FIFO overrun interrupt on INT1
#define LSM303DLHC_CTRL3_I1OVERRUN_DIS                                        \
                                0x00
#define LSM303DLHC_CTRL3_I1OVERRUN_EN                                         \
                                0x02

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_CTRL4
// register.
//
//*****************************************************************************
#define LSM303DLHC_CTRL4_BDU_M  0x80        // Block data update; default
                                            // value: 0 (0: continuous update;
                                            // 1: output registers not updated
                                            // until MSB and LSB reading)
#define LSM303DLHC_CTRL4_BDU_CONTINUOUS                                       \
                                0x00
#define LSM303DLHC_CTRL4_BDU_MSBLSB                                           \
                                0x80
#define LSM303DLHC_CTRL4_ENDIAN_M                                             \
                                0x40        // Endian selection
#define LSM303DLHC_CTRL4_ENDIAN_LITTLE                                        \
                                0x00
#define LSM303DLHC_CTRL4_ENDIAN_BIG                                           \
                                0x40
#define LSM303DLHC_CTRL4_FS_M   0x30        // full scale selection; default
                                            // value: 0
#define LSM303DLHC_CTRL4_FS_2G  0x00
#define LSM303DLHC_CTRL4_FS_4G  0x10
#define LSM303DLHC_CTRL4_FS_8G  0x20
#define LSM303DLHC_CTRL4_FS_16G 0x30
#define LSM303DLHC_CTRL4_RESOLUTION_M                                         \
                                0x08        // resolution output mode
#define LSM303DLHC_CTRL4_RESOLUTION_LOW                                       \
                                0x00
#define LSM303DLHC_CTRL4_RESOLUTION_HIGH                                      \
                                0x08
#define LSM303DLHC_CTRL4_SIM_M  0x01        // SPI Serial Interface Mode
                                            // selection (default = 0, 4-wire)
#define LSM303DLHC_CTRL4_SIM_4WIRE                                            \
                                0x00
#define LSM303DLHC_CTRL4_SIM_3WIRE                                            \
                                0x01
#define LSM303DLHC_CTRL4_FS_S   4

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_CTRL5
// register.
//
//*****************************************************************************
#define LSM303DLHC_CTRL5_REBOOTCTL_M                                          \
                                0x80        // Reboot memory conent
#define LSM303DLHC_CTRL5_REBOOTCTL_NORMAL                                     \
                                0x00
#define LSM303DLHC_CTRL5_REBOOTCTL_REBOOT                                     \
                                0x80
#define LSM303DLHC_CTRL5_FIFO_M 0x40        // FIFO enable
#define LSM303DLHC_CTRL5_FIFO_DIS                                             \
                                0x00
#define LSM303DLHC_CTRL5_FIFO_EN                                              \
                                0x40
#define LSM303DLHC_CTRL5_LIR_INT1_M                                           \
                                0x08        // Latch interrupt on INT1
#define LSM303DLHC_CTRL5_LIR_INT1_DIS                                         \
                                0x00
#define LSM303DLHC_CTRL5_LIR_INT1_EN                                          \
                                0x08
#define LSM303DLHC_CTRL5_D4D_INT1_M                                           \
                                0x04        // 4D Int enable on INT1
#define LSM303DLHC_CTRL5_D4D_INT1_DIS                                         \
                                0x00
#define LSM303DLHC_CTRL5_D4D_INT1_EN                                          \
                                0x04
#define LSM303DLHC_CTRL5_LIR_INT2_M                                           \
                                0x02        // Latch interrupt request on
                                            // INT2_SRC register, with INT2_SRC
                                            // register cleared by reading
                                            // INT2_SRC itself; default value:
                                            // 0
#define LSM303DLHC_CTRL5_LIR_INT2_DIS                                         \
                                0x00
#define LSM303DLHC_CTRL5_LIR_INT2_EN                                          \
                                0x02
#define LSM303DLHC_CTRL5_D4D_INT2_M                                           \
                                0x01        // 4D enable: 4D detection is
                                            // enabled on INT2 when 6D bit on
                                            // INT2_CFG is set to 1
#define LSM303DLHC_CTRL5_D4D_INT2_DIS                                         \
                                0x00
#define LSM303DLHC_CTRL5_D4D_INT2_EN                                          \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_CTRL6
// register.
//
//*****************************************************************************
#define LSM303DLHC_CTRL6_I2_CLICK_M                                           \
                                0x80        // CLICK interrupt on PAD2
#define LSM303DLHC_CTRL6_I2_CLICK_DIS                                         \
                                0x00
#define LSM303DLHC_CTRL6_I2_CLICK_EN                                          \
                                0x80
#define LSM303DLHC_CTRL6_I2_INT1_M                                            \
                                0x40        // Interrupt 1 on PAD2; default
                                            // value 0
#define LSM303DLHC_CTRL6_I2_INT1_DIS                                          \
                                0x00
#define LSM303DLHC_CTRL6_I2_INT1_EN                                           \
                                0x40
#define LSM303DLHC_CTRL6_I2_INT2_M                                            \
                                0x20        // Interrupt 2 on PAD2; default
                                            // value 0
#define LSM303DLHC_CTRL6_I2_INT2_DIS                                          \
                                0x00
#define LSM303DLHC_CTRL6_I2_INT2_EN                                           \
                                0x20
#define LSM303DLHC_CTRL6_BOOT_I2_M                                            \
                                0x10        // Reboot memory content on PAD2;
                                            // default value: 0
#define LSM303DLHC_CTRL6_BOOT_I2_DIS                                          \
                                0x00
#define LSM303DLHC_CTRL6_BOOT_I2_EN                                           \
                                0x10
#define LSM303DLHC_CTRL6_P2_ACT_M                                             \
                                0x08        // active function status on PAD2
#define LSM303DLHC_CTRL6_P2_ACT_DIS                                           \
                                0x00
#define LSM303DLHC_CTRL6_P2_ACT_EN                                            \
                                0x08
#define LSM303DLHC_CTRL6_H_LACTIVE_M                                          \
                                0x02        // interrupt active configuration
                                            // on INT; default value 0 (0:
                                            // high; 1:low)
#define LSM303DLHC_CTRL6_H_LACTIVE_HI                                         \
                                0x00
#define LSM303DLHC_CTRL6_H_LACTIVE_LOW                                        \
                                0x02

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_STATUS
// register.
//
//*****************************************************************************
#define LSM303DLHC_STATUS_OR_M  0xF0        // Axis data overrun
#define LSM303DLHC_STATUS_OR_X  0x10        // X-axis data overrun
#define LSM303DLHC_STATUS_OR_Y  0x20        // Y-axis data overrun
#define LSM303DLHC_STATUS_OR_Z  0x40        // Z-axis data overrun
#define LSM303DLHC_STATUS_OR_ZYX                                              \
                                0x80        // X, Y, and X data overrun
#define LSM303DLHC_STATUS_DA_M  0xF         // Axis data available
#define LSM303DLHC_STATUS_DA_X  0x01        // X-axis data available
#define LSM303DLHC_STATUS_DA_Y  0x02        // Y-axis data available
#define LSM303DLHC_STATUS_DA_Z  0x04        // Z-axis data available
#define LSM303DLHC_STATUS_DA_ZYX                                              \
                                0x08        // X, Y, and X data available
#define LSM303DLHC_STATUS_OR_S  4
#define LSM303DLHC_STATUS_DA_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_FIFO_CTRL
// register.
//
//*****************************************************************************
#define LSM303DLHC_FIFO_CTRL_TR_M                                             \
                                0x20
#define LSM303DLHC_FIFO_CTRL_TR_INT1                                          \
                                0x00
#define LSM303DLHC_FIFO_CTRL_TR_INT2                                          \
                                0x20
#define LSM303DLHC_FIFO_CTRL_THRESH_M                                         \
                                0x1F        // FIFO Threshold setting
#define LSM303DLHC_FIFO_CTRL_MODE_M                                           \
                                0xC         // FIFO mode setting
#define LSM303DLHC_FIFO_CTRL_MODE_BYPASS                                      \
                                0x00        // Bypass mode
#define LSM303DLHC_FIFO_CTRL_MODE_FIFO                                        \
                                0x40        // FIFO mode
#define LSM303DLHC_FIFO_CTRL_MODE_STREAM                                      \
                                0x80        // Stream mode
#define LSM303DLHC_FIFO_CTRL_MODE_TRIGGER                                     \
                                0xC0        // Trigger mode
#define LSM303DLHC_FIFO_CTRL_MODE_S                                           \
                                6
#define LSM303DLHC_FIFO_CTRL_THRESH_S                                         \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_FIFO_SRC
// register.
//
//*****************************************************************************
#define LSM303DLHC_FIFO_SRC_FTH_M                                             \
                                0x80        // FIFO threshold is greater than
                                            // or equal to level or less than
                                            // level
#define LSM303DLHC_FIFO_SRC_FTH_LT                                            \
                                0x00
#define LSM303DLHC_FIFO_SRC_FTH_GEQ                                           \
                                0x80
#define LSM303DLHC_FIFO_SRC_OVRN_M                                            \
                                0x40        // overrun status bit
#define LSM303DLHC_FIFO_SRC_OVRN_FILLED                                       \
                                0x40
#define LSM303DLHC_FIFO_SRC_EMPTY_M                                           \
                                0x20        // FIFO empty
#define LSM303DLHC_FIFO_SRC_EMPTY_EMPTY                                       \
                                0x20
#define LSM303DLHC_FIFO_SRC_STORE_SAMPLES_M                                   \
                                0x1F        // FIFO stored data level of the
                                            // unread samples
#define LSM303DLHC_FIFO_SRC_STORE_SAMPLES_S                                   \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_INT1_CFG_A
// register.
//
//*****************************************************************************
#define LSM303DLHC_INT1_CFG_A_ANDOR_M                                         \
                                0x80        // AND/OR combination of Interrupt
                                            // events; default value: 0
#define LSM303DLHC_INT1_CFG_A_ANDOR_OR                                        \
                                0x00
#define LSM303DLHC_INT1_CFG_A_ANDOR_AND                                       \
                                0x80
#define LSM303DLHC_INT1_CFG_A_6D_M                                            \
                                0x40        // 6-direction function enabled
#define LSM303DLHC_INT1_CFG_A_6D_EN                                           \
                                0x40
#define LSM303DLHC_INT1_CFG_A_ZHI_M                                           \
                                0x20        // enable interrupt generation on
                                            // Z-hi event
#define LSM303DLHC_INT1_CFG_A_ZHI_EN                                          \
                                0x20
#define LSM303DLHC_INT1_CFG_A_ZUPE_M                                          \
                                0x20        // enable interrupt generation on
                                            // Z-hi event
#define LSM303DLHC_INT1_CFG_A_ZUPE_EN                                         \
                                0x20
#define LSM303DLHC_INT1_CFG_A_ZDOWNE_M                                        \
                                0x10        // enable interrupt generation on
                                            // Z-low event
#define LSM303DLHC_INT1_CFG_A_ZDOWNE_EN                                       \
                                0x10
#define LSM303DLHC_INT1_CFG_A_ZLI_M                                           \
                                0x10        // enable interrupt generation on
                                            // Z-low event
#define LSM303DLHC_INT1_CFG_A_ZLI_EN                                          \
                                0x10
#define LSM303DLHC_INT1_CFG_A_YUPE_M                                          \
                                0x08        // enable interrupt generation on
                                            // Y-hi event
#define LSM303DLHC_INT1_CFG_A_YUPE_EN                                         \
                                0x08
#define LSM303DLHC_INT1_CFG_A_YHI_M                                           \
                                0x08        // enable interrupt generation on
                                            // Y-hi event
#define LSM303DLHC_INT1_CFG_A_YHI_EN                                          \
                                0x08
#define LSM303DLHC_INT1_CFG_A_YDOWNE_M                                        \
                                0x04        // enable interrupt generation on
                                            // Y-low event
#define LSM303DLHC_INT1_CFG_A_YDOWNE_EN                                       \
                                0x04
#define LSM303DLHC_INT1_CFG_A_YLI_M                                           \
                                0x04        // enable interrupt generation on
                                            // Y-low event
#define LSM303DLHC_INT1_CFG_A_YLI_EN                                          \
                                0x04
#define LSM303DLHC_INT1_CFG_A_XHI_M                                           \
                                0x02        // enable interrupt generation on
                                            // X-hi event
#define LSM303DLHC_INT1_CFG_A_XHI_EN                                          \
                                0x02
#define LSM303DLHC_INT1_CFG_A_XUPE_M                                          \
                                0x02        // enable interrupt generation on
                                            // X-hi event
#define LSM303DLHC_INT1_CFG_A_XUPE_EN                                         \
                                0x02
#define LSM303DLHC_INT1_CFG_A_XLI_M                                           \
                                0x01        // enable interrupt generation on
                                            // X-low event
#define LSM303DLHC_INT1_CFG_A_XLI_EN                                          \
                                0x01
#define LSM303DLHC_INT1_CFG_A_XDOWNE_M                                        \
                                0x01        // enable interrupt generation on
                                            // X-low event
#define LSM303DLHC_INT1_CFG_A_XDOWNE_EN                                       \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_INT1_SRC_A
// register.
//
//*****************************************************************************
#define LSM303DLHC_INT1_SRC_A_ZERO_M                                          \
                                0x80        // This bit must be zero
#define LSM303DLHC_INT1_SRC_A_ZERO_ZERO                                       \
                                0x00
#define LSM303DLHC_INT1_SRC_A_ZERO_INVALID                                    \
                                0x80
#define LSM303DLHC_INT1_SRC_A_IA_M                                            \
                                0x40        // interrupt active
#define LSM303DLHC_INT1_SRC_A_IA_ACTIVE                                       \
                                0x40
#define LSM303DLHC_INT1_SRC_A_ZH_M                                            \
                                0x20        // Z-Hi event occurred
#define LSM303DLHC_INT1_SRC_A_ZH_OCCURRED                                     \
                                0x20
#define LSM303DLHC_INT1_SRC_A_ZL_M                                            \
                                0x10        // Z-Low event occurred
#define LSM303DLHC_INT1_SRC_A_ZL_OCCURRED                                     \
                                0x10
#define LSM303DLHC_INT1_SRC_A_YH_M                                            \
                                0x08        // Y-Hi event occurred
#define LSM303DLHC_INT1_SRC_A_YH_OCCURRED                                     \
                                0x08
#define LSM303DLHC_INT1_SRC_A_YL_M                                            \
                                0x04        // Y-Low event occurred
#define LSM303DLHC_INT1_SRC_A_YL_OCCURRED                                     \
                                0x04
#define LSM303DLHC_INT1_SRC_A_XH_M                                            \
                                0x02        // X-Hi event occurred
#define LSM303DLHC_INT1_SRC_A_XH_OCCURRED                                     \
                                0x02
#define LSM303DLHC_INT1_SRC_A_XL_M                                            \
                                0x01        // X-Low event occurred
#define LSM303DLHC_INT1_SRC_A_XL_OCCURRED                                     \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the
// LSM303DLHC_O_MAG_TEMP_OUT_MSB register.
//
//*****************************************************************************
#define LSM303DLHC_MAG_TEMP_OUT_MSB_MSB_M                                     \
                                0xFF
#define LSM303DLHC_MAG_TEMP_OUT_MSB_MSB_S                                     \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// LSM303DLHC_O_MAG_TEMP_OUT_LSB register.
//
//*****************************************************************************
#define LSM303DLHC_MAG_TEMP_OUT_LSB_LSB_M                                     \
                                0xF0
#define LSM303DLHC_MAG_TEMP_OUT_LSB_LSB_S                                     \
                                4

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_INT1_THS_A
// register.
//
//*****************************************************************************
#define LSM303DLHC_INT1_THS_A_ZERO_M                                          \
                                0x80        // This bit must be zero
#define LSM303DLHC_INT1_THS_A_ZERO_ZERO                                       \
                                0x00
#define LSM303DLHC_INT1_THS_A_ZERO_INVALID                                    \
                                0x80
#define LSM303DLHC_INT1_THS_A_THS_M                                           \
                                0x7F        // THS[6-0] Interrupt threshold,
                                            // default 0x0
#define LSM303DLHC_INT1_THS_A_THS_S                                           \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// LSM303DLHC_O_INT1_DURATION_A register.
//
//*****************************************************************************
#define LSM303DLHC_INT1_DURATION_A_ZERO_M                                     \
                                0x80        // This bit must be zero
#define LSM303DLHC_INT1_DURATION_A_ZERO_ZERO                                  \
                                0x00
#define LSM303DLHC_INT1_DURATION_A_ZERO_INVALID                               \
                                0x80
#define LSM303DLHC_INT1_DURATION_A_DURATION_M                                 \
                                0x7F        // Duration D[6-0]
#define LSM303DLHC_INT1_DURATION_A_DURATION_S                                 \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_INT2_CFG_A
// register.
//
//*****************************************************************************
#define LSM303DLHC_INT2_CFG_A_ANDOR_M                                         \
                                0x80        // AND/OR combination of Interrupt
                                            // events; default value: 0
#define LSM303DLHC_INT2_CFG_A_ANDOR_OR                                        \
                                0x00
#define LSM303DLHC_INT2_CFG_A_ANDOR_AND                                       \
                                0x80
#define LSM303DLHC_INT2_CFG_A_6D_M                                            \
                                0x40        // 6-direction function enabled
#define LSM303DLHC_INT2_CFG_A_6D_EN                                           \
                                0x40
#define LSM303DLHC_INT2_CFG_A_ZHI_M                                           \
                                0x20        // enable interrupt generation on
                                            // Z-hi event
#define LSM303DLHC_INT2_CFG_A_ZHI_EN                                          \
                                0x20
#define LSM303DLHC_INT2_CFG_A_ZLI_M                                           \
                                0x10        // enable interrupt generation on
                                            // Z-low event
#define LSM303DLHC_INT2_CFG_A_ZLI_EN                                          \
                                0x10
#define LSM303DLHC_INT2_CFG_A_YHI_M                                           \
                                0x08        // enable interrupt generation on
                                            // Y-hi event
#define LSM303DLHC_INT2_CFG_A_YHI_EN                                          \
                                0x08
#define LSM303DLHC_INT2_CFG_A_YLI_M                                           \
                                0x04        // enable interrupt generation on
                                            // Y-low event
#define LSM303DLHC_INT2_CFG_A_YLI_EN                                          \
                                0x04
#define LSM303DLHC_INT2_CFG_A_XHI_M                                           \
                                0x02        // enable interrupt generation on
                                            // X-hi event
#define LSM303DLHC_INT2_CFG_A_XHI_EN                                          \
                                0x02
#define LSM303DLHC_INT2_CFG_A_XLI_M                                           \
                                0x01        // enable interrupt generation on
                                            // X-low event
#define LSM303DLHC_INT2_CFG_A_XLI_EN                                          \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_INT2_SRC_A
// register.
//
//*****************************************************************************
#define LSM303DLHC_INT2_SRC_A_ZERO_M                                          \
                                0x80        // This bit must be zero
#define LSM303DLHC_INT2_SRC_A_ZERO_ZERO                                       \
                                0x00
#define LSM303DLHC_INT2_SRC_A_ZERO_INVALID                                    \
                                0x80
#define LSM303DLHC_INT2_SRC_A_IA_M                                            \
                                0x40        // interrupt active
#define LSM303DLHC_INT2_SRC_A_IA_ACTIVE                                       \
                                0x40
#define LSM303DLHC_INT2_SRC_A_ZH_M                                            \
                                0x20        // Z-Hi event occurred
#define LSM303DLHC_INT2_SRC_A_ZH_OCCURRED                                     \
                                0x20
#define LSM303DLHC_INT2_SRC_A_ZL_M                                            \
                                0x10        // Z-Low event occurred
#define LSM303DLHC_INT2_SRC_A_ZL_OCCURRED                                     \
                                0x10
#define LSM303DLHC_INT2_SRC_A_YH_M                                            \
                                0x08        // Y-Hi event occurred
#define LSM303DLHC_INT2_SRC_A_YH_OCCURRED                                     \
                                0x08
#define LSM303DLHC_INT2_SRC_A_YL_M                                            \
                                0x04        // Y-Low event occurred
#define LSM303DLHC_INT2_SRC_A_YL_OCCURRED                                     \
                                0x04
#define LSM303DLHC_INT2_SRC_A_XH_M                                            \
                                0x02        // X-Hi event occurred
#define LSM303DLHC_INT2_SRC_A_XH_OCCURRED                                     \
                                0x02
#define LSM303DLHC_INT2_SRC_A_XL_M                                            \
                                0x01        // X-Low event occurred
#define LSM303DLHC_INT2_SRC_A_XL_OCCURRED                                     \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_INT2_THS_A
// register.
//
//*****************************************************************************
#define LSM303DLHC_INT2_THS_A_ZERO_M                                          \
                                0x80        // This bit must be zero
#define LSM303DLHC_INT2_THS_A_ZERO_ZERO                                       \
                                0x00
#define LSM303DLHC_INT2_THS_A_ZERO_INVALID                                    \
                                0x80
#define LSM303DLHC_INT2_THS_A_THS_M                                           \
                                0x7F        // THS[6-0] Interrupt threshold,
                                            // default 0x0
#define LSM303DLHC_INT2_THS_A_THS_S                                           \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// LSM303DLHC_O_INT2_DURATION_A register.
//
//*****************************************************************************
#define LSM303DLHC_INT2_DURATION_A_ZERO_M                                     \
                                0x80        // This bit must be zero
#define LSM303DLHC_INT2_DURATION_A_ZERO_ZERO                                  \
                                0x00
#define LSM303DLHC_INT2_DURATION_A_ZERO_INVALID                               \
                                0x80
#define LSM303DLHC_INT2_DURATION_A_DURATION_M                                 \
                                0x7F        // Duration D[6-0]
#define LSM303DLHC_INT2_DURATION_A_DURATION_S                                 \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_CLICK_CFG_A
// register.
//
//*****************************************************************************
#define LSM303DLHC_CLICK_CFG_A_ZD_M                                           \
                                0x20        // Enable interrupt double click on
                                            // Z axis
#define LSM303DLHC_CLICK_CFG_A_ZD_DIS                                         \
                                0x00
#define LSM303DLHC_CLICK_CFG_A_ZD_EN                                          \
                                0x20
#define LSM303DLHC_CLICK_CFG_A_ZS_M                                           \
                                0x10        // Enable interrupt single click on
                                            // Z axis
#define LSM303DLHC_CLICK_CFG_A_ZS_DIS                                         \
                                0x00
#define LSM303DLHC_CLICK_CFG_A_ZS_EN                                          \
                                0x10
#define LSM303DLHC_CLICK_CFG_A_YD_M                                           \
                                0x08        // Enable interrupt double click on
                                            // Y axis
#define LSM303DLHC_CLICK_CFG_A_YD_DIS                                         \
                                0x00
#define LSM303DLHC_CLICK_CFG_A_YD_EN                                          \
                                0x08
#define LSM303DLHC_CLICK_CFG_A_YS_M                                           \
                                0x04        // Enable interrupt single click on
                                            // Y axis
#define LSM303DLHC_CLICK_CFG_A_YS_DIS                                         \
                                0x00
#define LSM303DLHC_CLICK_CFG_A_YS_EN                                          \
                                0x04
#define LSM303DLHC_CLICK_CFG_A_XD_M                                           \
                                0x02        // Enable interrupt double click on
                                            // X axis
#define LSM303DLHC_CLICK_CFG_A_XD_DIS                                         \
                                0x00
#define LSM303DLHC_CLICK_CFG_A_XD_EN                                          \
                                0x02
#define LSM303DLHC_CLICK_CFG_A_XS_M                                           \
                                0x01        // Enable interrupt single click on
                                            // X axis
#define LSM303DLHC_CLICK_CFG_A_XS_DIS                                         \
                                0x00
#define LSM303DLHC_CLICK_CFG_A_XS_EN                                          \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_CLICK_SRC_A
// register.
//
//*****************************************************************************
#define LSM303DLHC_CLICK_SRC_A_IA_M                                           \
                                0x40        // Interrupt pending
#define LSM303DLHC_CLICK_SRC_A_IA_NONE                                        \
                                0x00
#define LSM303DLHC_CLICK_SRC_A_IA_PENDING                                     \
                                0x40
#define LSM303DLHC_CLICK_SRC_A_DCLICK_M                                       \
                                0x20        // double click-click enable
#define LSM303DLHC_CLICK_SRC_A_DCLICK_DIS                                     \
                                0x00
#define LSM303DLHC_CLICK_SRC_A_DCLICK_EN                                      \
                                0x20
#define LSM303DLHC_CLICK_SRC_A_SCLICK_M                                       \
                                0x10        // single click-click enable
#define LSM303DLHC_CLICK_SRC_A_SCLICK_DIS                                     \
                                0x00
#define LSM303DLHC_CLICK_SRC_A_SCLICK_EN                                      \
                                0x10
#define LSM303DLHC_CLICK_SRC_A_SIGN_M                                         \
                                0x08        // click-click sign
#define LSM303DLHC_CLICK_SRC_A_SIGN_POSITIVE                                  \
                                0x00
#define LSM303DLHC_CLICK_SRC_A_SIGN_NEGATIVE                                  \
                                0x08
#define LSM303DLHC_CLICK_SRC_A_AXIS_M                                         \
                                0x7         // Axis click detection
#define LSM303DLHC_CLICK_SRC_A_AXIS_X                                         \
                                0x01        // X click-click detection
#define LSM303DLHC_CLICK_SRC_A_AXIS_Y                                         \
                                0x02        // Y click-click detection
#define LSM303DLHC_CLICK_SRC_A_AXIS_Z                                         \
                                0x04        // Z click-click detection
#define LSM303DLHC_CLICK_SRC_A_AXIS_S                                         \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the LSM303DLHC_O_CLICK_THS_A
// register.
//
//*****************************************************************************
#define LSM303DLHC_CLICK_THS_A_THS_M                                          \
                                0x7F        // Threshold; 1 LSB = full-scale /
                                            // 128; THS6 through THS0 define
                                            // the threshold which is used by
                                            // the system to start the click
                                            // detection procedure; the
                                            // threshold value is expressed
                                            // over 7 bits as an unsigned
                                            // number
#define LSM303DLHC_CLICK_THS_A_THS_S                                          \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// LSM303DLHC_O_TIME_LIMIT_A register.
//
//*****************************************************************************
#define LSM303DLHC_TIME_LIMIT_A_TLI_M                                         \
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
#define LSM303DLHC_TIME_LIMIT_A_TLI_S                                         \
                                0

#endif // __SENSORLIB_HW_LSM303DLHC_H__
