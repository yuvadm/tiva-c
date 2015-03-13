//*****************************************************************************
//
// hw_l3gd20h.h - Macros used when accessing the ST L3GD20H gyroscope
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

#ifndef __SENSORLIB_HW_L3GD20H_H__
#define __SENSORLIB_HW_L3GD20H_H__

//*****************************************************************************
//
// The following are defines for the L3GD20H register addresses
//
//*****************************************************************************
#define L3GD20H_O_WHOAMI        0x0F        // Device idenfitication register
#define L3GD20H_O_CTRL1         0x20        // Control 1 - power settings
#define L3GD20H_O_CTRL2         0x21        // control 2
#define L3GD20H_O_CTRL3         0x22        // Control 3
#define L3GD20H_O_CTRL4         0x23        // Control 4
#define L3GD20H_O_CTRL5         0x24        // Control 5
#define L3GD20H_O_REFERENCE     0x25        // Reference register
#define L3GD20H_O_OUT_TEMP      0x26        // Temperature data; -1LSB/deg,
                                            // two's complement
#define L3GD20H_O_STATUS        0x27        // Status register
#define L3GD20H_O_OUT_X_LSB     0x28        // X-axis LSB
#define L3GD20H_O_OUT_X_MSB     0x29        // X-axis MSB
#define L3GD20H_O_OUT_Y_LSB     0x2A        // Y-axis LSB
#define L3GD20H_O_OUT_Y_MSB     0x2B        // Y-axis MSB
#define L3GD20H_O_OUT_Z_LSB     0x2C        // Z-axis LSB
#define L3GD20H_O_OUT_Z_MSB     0x2D        // Z-axis MSB
#define L3GD20H_O_FIFO_CTRL     0x2E        // FIFO control
#define L3GD20H_O_FIFO_SRC      0x2F        // FIFO_SRC register
#define L3GD20H_O_IG_CFG        0x30        // Interrupt generation control
#define L3GD20H_O_IG_SRC        0x31        // interrupt source register (read
                                            // only)
#define L3GD20H_O_IG_THS_XH     0x32        // Hi-X threshold
#define L3GD20H_O_IG_THS_XL     0x33        // Lo-X threshold
#define L3GD20H_O_IG_THS_TH     0x34        // Hi-Y threshold
#define L3GD20H_O_IG_THS_TL     0x35        // Lo-Y threshold
#define L3GD20H_O_IG_THS_ZH     0x36        // Hi-Z threshold
#define L3GD20H_O_IG_THS_ZL     0x37        // Lo-X threshold
#define L3GD20H_O_IG_DURATION   0x38        // Interrupt generation duration
                                            // register
#define L3GD20H_O_LOW_ODR       0x39        // Low-speed output data rate (ODR)

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_CTRL1
// register.
//
//*****************************************************************************
#define L3GD20H_CTRL1_DR_M      0xC0        // ODR select, x8 if Low_ODR = 0
#define L3GD20H_CTRL1_DR_12_5_HZ                                              \
                                0x00        // 12.5Hz or 100Hz
#define L3GD20H_CTRL1_DR_25_HZ  0x40        // 25Hz or 200Hz
#define L3GD20H_CTRL1_DR_50_HZ  0x80        // 50Hz or 400Hz
#define L3GD20H_CTRL1_DR_800_HZ 0xC0        // 800Hz of Low_ODR=0, or 50Hz
                                            // otherwise
#define L3GD20H_CTRL1_BW_M      0x30        // Bandwidth select
#define L3GD20H_CTRL1_POWER_M   0x08        // Power control
#define L3GD20H_CTRL1_POWER_LOWPOW                                            \
                                0x00
#define L3GD20H_CTRL1_POWER_NORMAL                                            \
                                0x08
#define L3GD20H_CTRL1_AXIS_M    0x7         // Axis power control
#define L3GD20H_CTRL1_AXIS_Y_EN 0x01        // Y-axis enable
#define L3GD20H_CTRL1_AXIS_X_EN 0x02        // X-axis enable
#define L3GD20H_CTRL1_AXIS_Z_EN 0x04        // Z-axis enable
#define L3GD20H_CTRL1_DR_S      6
#define L3GD20H_CTRL1_BW_S      4
#define L3GD20H_CTRL1_AXIS_S    0

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_CTRL2
// register.
//
//*****************************************************************************
#define L3GD20H_CTRL2_EXTREN_M  0x80        // edge sensitive
#define L3GD20H_CTRL2_EXTREN_DIS                                              \
                                0x00
#define L3GD20H_CTRL2_EXTREN_EN 0x80
#define L3GD20H_CTRL2_LVLEN_M   0x40        // level sensitive
#define L3GD20H_CTRL2_LVLEN_DIS 0x00
#define L3GD20H_CTRL2_LVLEN_EN  0x40
#define L3GD20H_CTRL2_HPM_M     0x30        // high pass filter mode selection
#define L3GD20H_CTRL2_HPCF_M    0x0F        // high pass cutoff frequency
                                            // selection
#define L3GD20H_CTRL2_HPM_S     4
#define L3GD20H_CTRL2_HPCF_S    0

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_CTRL3
// register.
//
//*****************************************************************************
#define L3GD20H_CTRL3_INT1_IG_M 0x80        // Interrupt enable on INT1 pin
#define L3GD20H_CTRL3_INT1_IG_DIS                                             \
                                0x00
#define L3GD20H_CTRL3_INT1_IG_EN                                              \
                                0x80
#define L3GD20H_CTRL3_INT1_BOOT_M                                             \
                                0x40        // Boot status available on INT1
                                            // pin.
#define L3GD20H_CTRL3_INT1_BOOT_DIS                                           \
                                0x00
#define L3GD20H_CTRL3_INT1_BOOT_EN                                            \
                                0x40
#define L3GD20H_CTRL3_H_LACTIVE_M                                             \
                                0x20        // Interrupt active configuration
                                            // on INT; default value: 0 (0:
                                            // high; 1:low)
#define L3GD20H_CTRL3_H_LACTIVE_HI                                            \
                                0x00
#define L3GD20H_CTRL3_H_LACTIVE_LOW                                           \
                                0x20
#define L3GD20H_CTRL3_DRIVE_TYPE_M                                            \
                                0x10        // Push- Pull / Open drain; default
                                            // value: 0 (0: push-pull; 1: open
                                            // drain)
#define L3GD20H_CTRL3_DRIVE_TYPE_PP                                           \
                                0x00
#define L3GD20H_CTRL3_DRIVE_TYPE_OD                                           \
                                0x10
#define L3GD20H_CTRL3_INT2_DRDY_M                                             \
                                0x08        // Date Ready on DRDY/INT2 pin;
                                            // default value: 0
#define L3GD20H_CTRL3_INT2_DRDY_DIS                                           \
                                0x00
#define L3GD20H_CTRL3_INT2_DRDY_EN                                            \
                                0x08
#define L3GD20H_CTRL3_INT2_FTH_M                                              \
                                0x04        // FIFO Threshold interrupt on
                                            // DRDY/INT2 pin; default value: 0
#define L3GD20H_CTRL3_INT2_FTH_DIS                                            \
                                0x00
#define L3GD20H_CTRL3_INT2_FTH_EN                                             \
                                0x04
#define L3GD20H_CTRL3_INT2_ORUN_M                                             \
                                0x02        // FIFO Overrun interrupt on
                                            // DRDY/INT2 pin; default value: 0
#define L3GD20H_CTRL3_INT2_ORUN_DIS                                           \
                                0x00
#define L3GD20H_CTRL3_INT2_ORUN_EN                                            \
                                0x02
#define L3GD20H_CTRL3_INT2_EMPTY_M                                            \
                                0x01        // FIFO Empty interrupt on
                                            // DRDY/INT2 pin; default value: 0
#define L3GD20H_CTRL3_INT2_EMPTY_DIS                                          \
                                0x00
#define L3GD20H_CTRL3_INT2_EMPTY_EN                                           \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_CTRL4
// register.
//
//*****************************************************************************
#define L3GD20H_CTRL4_BDU_M     0x80        // Block data update; default
                                            // value: 0 (0: continuous update;
                                            // 1: output registers not updated
                                            // until MSB and LSB reading)
#define L3GD20H_CTRL4_BDU_CONTINUOUS                                          \
                                0x00
#define L3GD20H_CTRL4_BDU_MSBLSB                                              \
                                0x80
#define L3GD20H_CTRL4_ENDIAN_M  0x40        // Endian selection
#define L3GD20H_CTRL4_ENDIAN_LITTLE                                           \
                                0x00
#define L3GD20H_CTRL4_ENDIAN_BIG                                              \
                                0x40
#define L3GD20H_CTRL4_FS_M      0x30        // full scale selection; default
                                            // value: 0
#define L3GD20H_CTRL4_FS_245DPS 0x00        // 245 degrees per second
#define L3GD20H_CTRL4_FS_500DPS 0x10        // 500 degrees per second
#define L3GD20H_CTRL4_FS_2000DPS                                              \
                                0x30        // 2000 degrees per second
#define L3GD20H_CTRL4_IMPEN_M   0x08        // level sensitive latched enable;
                                            // default value: 0
#define L3GD20H_CTRL4_IMPEN_LVL_DIS                                           \
                                0x00
#define L3GD20H_CTRL4_IMPEN_LVL_EN                                            \
                                0x08
#define L3GD20H_CTRL4_SELFTEST_M                                              \
                                0x06        // self-test mode
#define L3GD20H_CTRL4_SELFTEST_NORMAL                                         \
                                0x00        // Normal mode (not self test,
                                            // default)
#define L3GD20H_CTRL4_SELFTEST_MODE0                                          \
                                0x02        // Self-test mode 0 (+)
#define L3GD20H_CTRL4_SELFTEST_MODE1                                          \
                                0x06        // Self-test mode 1 (-)
#define L3GD20H_CTRL4_SIM_M     0x01        // SPI Serial Interface Mode
                                            // selection (default = 0, 4-wire)
#define L3GD20H_CTRL4_SIM_4WIRE 0x00
#define L3GD20H_CTRL4_SIM_3WIRE 0x01
#define L3GD20H_CTRL4_FS_S      4
#define L3GD20H_CTRL4_SELFTEST_S                                              \
                                1

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_CTRL5
// register.
//
//*****************************************************************************
#define L3GD20H_CTRL5_REBOOTCTL_M                                             \
                                0x80        // Reboot memory conent
#define L3GD20H_CTRL5_REBOOTCTL_NORMAL                                        \
                                0x00
#define L3GD20H_CTRL5_REBOOTCTL_REBOOT                                        \
                                0x80
#define L3GD20H_CTRL5_FIFOCTL_M 0x40        // FIFO control
#define L3GD20H_CTRL5_FIFOCTL_DIS                                             \
                                0x00
#define L3GD20H_CTRL5_FIFOCTL_EN                                              \
                                0x40
#define L3GD20H_CTRL5_STOPONFTH_M                                             \
                                0x20        // Sensing chain FIFO stop values
                                            // memorization at FIFO Threshold;
                                            // default value: 0
#define L3GD20H_CTRL5_STOPONFTH_UNLIMITED                                     \
                                0x00
#define L3GD20H_CTRL5_STOPONFTH_THRESH_LIMITED                                \
                                0x20
#define L3GD20H_CTRL5_HPEN_M    0x10        // high pass filtern enable
                                            // (default 0)
#define L3GD20H_CTRL5_HPEN_DIS  0x00
#define L3GD20H_CTRL5_HPEN_EN   0x10
#define L3GD20H_CTRL5_IG_SEL_M  0x0C        // interrupt generator section
                                            // configuration
#define L3GD20H_CTRL5_OUT_SEL_M 0x03        // out selection configuration
#define L3GD20H_CTRL5_IG_SEL_S  2
#define L3GD20H_CTRL5_OUT_SEL_S 0

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_STATUS
// register.
//
//*****************************************************************************
#define L3GD20H_STATUS_OR_M     0xF0        // Axis data overrun
#define L3GD20H_STATUS_OR_X     0x10        // X-axis data overrun
#define L3GD20H_STATUS_OR_Y     0x20        // Y-axis data overrun
#define L3GD20H_STATUS_OR_Z     0x40        // Z-axis data overrun
#define L3GD20H_STATUS_OR_ZYX   0x80        // X, Y, and X data overrun
#define L3GD20H_STATUS_DA_M     0xF         // Axis data available
#define L3GD20H_STATUS_DA_X     0x01        // X-axis data available
#define L3GD20H_STATUS_DA_Y     0x02        // Y-axis data available
#define L3GD20H_STATUS_DA_Z     0x04        // Z-axis data available
#define L3GD20H_STATUS_DA_ZYX   0x08        // X, Y, and X data available
#define L3GD20H_STATUS_OR_S     4
#define L3GD20H_STATUS_DA_S     0

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_FIFO_CTRL
// register.
//
//*****************************************************************************
#define L3GD20H_FIFO_CTRL_THRESH_M                                            \
                                0x1F        // FIFO Threshold setting
#define L3GD20H_FIFO_CTRL_MODE_M                                              \
                                0xE         // FIFO mode setting
#define L3GD20H_FIFO_CTRL_MODE_S                                              \
                                5
#define L3GD20H_FIFO_CTRL_THRESH_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_FIFO_SRC
// register.
//
//*****************************************************************************
#define L3GD20H_FIFO_SRC_FTH_M  0x80        // FIFO threshold is greater than
                                            // or equal to level or less than
                                            // level
#define L3GD20H_FIFO_SRC_FTH_LT 0x00
#define L3GD20H_FIFO_SRC_FTH_GEQ                                              \
                                0x80
#define L3GD20H_FIFO_SRC_OVRN_M 0x40        // overrun status bit
#define L3GD20H_FIFO_SRC_OVRN_FILLED                                          \
                                0x40
#define L3GD20H_FIFO_SRC_EMPTY_M                                              \
                                0x20        // FIFO empty
#define L3GD20H_FIFO_SRC_EMPTY_EMPTY                                          \
                                0x20
#define L3GD20H_FIFO_SRC_STORE_SAMPLES_M                                      \
                                0x1F        // FIFO stored data level of the
                                            // unread samples
#define L3GD20H_FIFO_SRC_STORE_SAMPLES_S                                      \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_IG_CFG
// register.
//
//*****************************************************************************
#define L3GD20H_IG_CFG_ANDOR_M  0x80        // AND/OR combination of Interrupt
                                            // events; default value: 0
#define L3GD20H_IG_CFG_ANDOR_OR 0x00
#define L3GD20H_IG_CFG_ANDOR_AND                                              \
                                0x80
#define L3GD20H_IG_CFG_LIR_M    0x40        // Latch Interrupt Request; default
                                            // value: 0
#define L3GD20H_IG_CFG_LIR_LATCHED                                            \
                                0x40
#define L3GD20H_IG_CFG_ZHI_M    0x20        // enable interrupt generation on
                                            // Z-hi event
#define L3GD20H_IG_CFG_ZHI_EN   0x20
#define L3GD20H_IG_CFG_ZLI_M    0x10        // enable interrupt generation on
                                            // Z-low event
#define L3GD20H_IG_CFG_ZLI_EN   0x10
#define L3GD20H_IG_CFG_YHI_M    0x08        // enable interrupt generation on
                                            // Y-hi event
#define L3GD20H_IG_CFG_YHI_EN   0x08
#define L3GD20H_IG_CFG_YLI_M    0x04        // enable interrupt generation on
                                            // Y-low event
#define L3GD20H_IG_CFG_YLI_EN   0x04
#define L3GD20H_IG_CFG_XHI_M    0x02        // enable interrupt generation on
                                            // X-hi event
#define L3GD20H_IG_CFG_XHI_EN   0x02
#define L3GD20H_IG_CFG_XLI_M    0x01        // enable interrupt generation on
                                            // X-low event
#define L3GD20H_IG_CFG_XLI_EN   0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_IG_SRC
// register.
//
//*****************************************************************************
#define L3GD20H_IG_SRC_IA_M     0x40        // interrupt active
#define L3GD20H_IG_SRC_IA_ACTIVE                                              \
                                0x40
#define L3GD20H_IG_SRC_ZH_M     0x20        // Z-Hi event occurred
#define L3GD20H_IG_SRC_ZH_OCCURRED                                            \
                                0x20
#define L3GD20H_IG_SRC_ZL_M     0x10        // Z-Low event occurred
#define L3GD20H_IG_SRC_ZL_OCCURRED                                            \
                                0x10
#define L3GD20H_IG_SRC_YH_M     0x08        // Y-Hi event occurred
#define L3GD20H_IG_SRC_YH_OCCURRED                                            \
                                0x08
#define L3GD20H_IG_SRC_YL_M     0x04        // Y-Low event occurred
#define L3GD20H_IG_SRC_YL_OCCURRED                                            \
                                0x04
#define L3GD20H_IG_SRC_XH_M     0x02        // X-Hi event occurred
#define L3GD20H_IG_SRC_XH_OCCURRED                                            \
                                0x02
#define L3GD20H_IG_SRC_XL_M     0x01        // X-Low event occurred
#define L3GD20H_IG_SRC_XL_OCCURRED                                            \
                                0x01

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_IG_THS_XH
// register.
//
//*****************************************************************************
#define L3GD20H_IG_THS_XH_DCRM_M                                              \
                                0x80        // interrupt generation counter
                                            // mode
#define L3GD20H_IG_THS_XH_DCRM_RESET                                          \
                                0x00
#define L3GD20H_IG_THS_XH_DCRM_DECREMENT                                      \
                                0x80
#define L3GD20H_IG_THS_XH_THSX_M                                              \
                                0x7F        // THSX[14-8], default 0x0
#define L3GD20H_IG_THS_XH_THSX_S                                              \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_IG_THS_TH
// register.
//
//*****************************************************************************
#define L3GD20H_IG_THS_TH_THSY_M                                              \
                                0x7F        // THSY[14-8], default 0x0
#define L3GD20H_IG_THS_TH_THSY_S                                              \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_IG_THS_ZH
// register.
//
//*****************************************************************************
#define L3GD20H_IG_THS_ZH_THSX_M                                              \
                                0x7F        // THSZ[14-8], default 0x0
#define L3GD20H_IG_THS_ZH_THSX_S                                              \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_IG_DURATION
// register.
//
//*****************************************************************************
#define L3GD20H_IG_DURATION_WAIT_M                                            \
                                0x80        // If WAIT is enabled then DURATION
                                            // samples must occur before
                                            // asserting the interrupt.
#define L3GD20H_IG_DURATION_WAIT_DIS                                          \
                                0x00
#define L3GD20H_IG_DURATION_WAIT_EN                                           \
                                0x80
#define L3GD20H_IG_DURATION_DURATION_M                                        \
                                0x7F        // Duration D[6-0]
#define L3GD20H_IG_DURATION_DURATION_S                                        \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the L3GD20H_O_LOW_ODR
// register.
//
//*****************************************************************************
#define L3GD20H_LOW_ODR_DRDY_HL_M                                             \
                                0x20        // DRDY/INT2 active level
#define L3GD20H_LOW_ODR_DRDY_HL_ACTIVE_LOW                                    \
                                0x00
#define L3GD20H_LOW_ODR_DRDY_HL_ACTIVE_HIGH                                   \
                                0x20
#define L3GD20H_LOW_ODR_I2C_DISABLE_M                                         \
                                0x08        // disable I2C interface
#define L3GD20H_LOW_ODR_I2C_DISABLE_BOTH                                      \
                                0x00
#define L3GD20H_LOW_ODR_I2C_DISABLE_SPI_ONLY                                  \
                                0x08
#define L3GD20H_LOW_ODR_SWRESET_M                                             \
                                0x04        // software reset
#define L3GD20H_LOW_ODR_SWRESET_NORMAL                                        \
                                0x00
#define L3GD20H_LOW_ODR_SWRESET_RESET                                         \
                                0x04
#define L3GD20H_LOW_ODR_DATARATE_M                                            \
                                0x01        // Low-speed data rate enable;
                                            // default value: 0
#define L3GD20H_LOW_ODR_DATARATE_HIGH                                         \
                                0x00
#define L3GD20H_LOW_ODR_DATARATE_LOW                                          \
                                0x01

#endif // __SENSORLIB_HW_L3GD20H_H__
