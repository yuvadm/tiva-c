//*****************************************************************************
//
// hw_kxti9.h - Macros used when accessing the Kionix KXTI9 accelerometer.
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

#ifndef __SENSORLIB_HW_KXTI9_H__
#define __SENSORLIB_HW_KXTI9_H__

//*****************************************************************************
//
// The following are defines for the KXTI9 register addresses.
//
//*****************************************************************************
#define KXTI9_O_XOUT_HFP_L      0x00        // X-axis high-pass data LSB
                                            // register
#define KXTI9_O_XOUT_HFP_H      0x01        // X-axis high-pass data MSB
                                            // register
#define KXTI9_O_YOUT_HFP_L      0x02        // Y-axis high-pass data LSB
                                            // register
#define KXTI9_O_YOUT_HFP_H      0x03        // Y-axis high-pass data MSB
                                            // register
#define KXTI9_O_ZOUT_HFP_L      0x04        // Z-axis high-pass data LSB
                                            // register
#define KXTI9_O_ZOUT_HFP_H      0x05        // Z-axis high-pass data MSB
                                            // register
#define KXTI9_O_XOUT_L          0x06        // X-axis data LSB register
#define KXTI9_O_XOUT_H          0x07        // X-axis data MSB register
#define KXTI9_O_YOUT_L          0x08        // Y-axis data LSB register
#define KXTI9_O_YOUT_H          0x09        // Y-axis data MSB register
#define KXTI9_O_ZOUT_L          0x0A        // Z-axis data LSB register
#define KXTI9_O_ZOUT_H          0x0B        // Z-axis data MSB register
#define KXTI9_O_DCST_RESP       0x0C        // Digital Control Status register
#define KXTI9_O_WHO_AM_I        0x0F        // Who Am I register
#define KXTI9_O_TILT_POS_CUR    0x10        // Current tilt position register
#define KXTI9_O_TILT_POS_PRE    0x11        // Previous tilt position register
#define KXTI9_O_INT_SRC1        0x15        // Interrupt source register 1
#define KXTI9_O_INT_SRC2        0x16        // Interrupt source register2
#define KXTI9_O_STATUS          0x18        // Status register
#define KXTI9_O_INT_REL         0x1A        // Interrupt clear/release register
#define KXTI9_O_CTRL1           0x1B        // Control register 1
#define KXTI9_O_CTRL2           0x1C        // Control register 2
#define KXTI9_O_CTRL3           0x1D        // Control register 3
#define KXTI9_O_INT_CTRL1       0x1E        // Interrupt control register 1
#define KXTI9_O_INT_CTRL2       0x1F        // Interrupt control register 2
#define KXTI9_O_INT_CTRL3       0x20        // Interrupt control register 3
#define KXTI9_O_DATA_CTRL       0x21        // Data control register
#define KXTI9_O_TILT_TIMER      0x28        // Tilt timer register
#define KXTI9_O_WUF_TIMER       0x29        // Wake-up timer register
#define KXTI9_O_TDT_TIMER       0x2B        // TDT timer register
#define KXTI9_O_TDT_H_THRESH    0x2C        // Jerk threshold high register
#define KXTI9_O_TDT_L_THRESH    0x2D        // Jerk threshold low register
#define KXTI9_O_TDT_TAP_TIMER   0x2E        // TDT tap timer register
#define KXTI9_O_TDT_TOTAL_TIMER 0x2F        // TDT double tap timer register
#define KXTI9_O_TDT_LATENCY_TIMER                                             \
                                0x30        // TDT tap latency timer register
#define KXTI9_O_TDT_WINDOW_TIMER                                              \
                                0x31        // TDT tap window timer register
#define KXTI9_O_BUF_CTRL1       0x32        // Buffer control 1 register
#define KXTI9_O_BUF_CTRL2       0x33        // Buffer control 2 register
#define KXTI9_O_STATUS1         0x34        // Buffer status 1 register
#define KXTI9_O_STATUS2         0x35        // Buffer status 2 register
#define KXTI9_O_BUF_CLEAR       0x36        // Buffer status clear register
#define KXTI9_O_SELF_TEST       0x3A        // Self-test register
#define KXTI9_O_WUF_THRESH      0x5A        // Wake-up threshold register
#define KXTI9_O_TILT_ANGLE      0x5C        // Tilt angle register
#define KXTI9_O_HYST_SET        0x5F        // Hysteresis set register
#define KXTI9_O_BUF_READ        0x7F        // Buffer read register

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_XOUT_HPF_L
// register.
//
//*****************************************************************************
#define KXTI9_XOUT_HPF_L_M      0xF0        // Bits [3:0] of high-pass filtered
                                            // X-axis data
#define KXTI9_XOUT_HPF_L_S      4

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_XOUT_HPF_H
// register.
//
//*****************************************************************************
#define KXTI9_XOUT_HPF_H_M      0xFF        // Bits [11:4] of high-pass
                                            // filtered X-axis data
#define KXTI9_XOUT_HPF_H_S      0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_YOUT_HPF_L
// register.
//
//*****************************************************************************
#define KXTI9_YOUT_HPF_L_M      0xF0        // Bits [3:0] of high-pass filtered
                                            // Y-axis data
#define KXTI9_YOUT_HPF_L_S      4

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_YOUT_HPF_H
// register.
//
//*****************************************************************************
#define KXTI9_YOUT_HPF_H_M      0xFF        // Bits [11:4] of high-pass
                                            // filtered Y-axis data
#define KXTI9_YOUT_HPF_H_S      0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_ZOUT_HPF_L
// register.
//
//*****************************************************************************
#define KXTI9_ZOUT_HPF_L_M      0xF0        // Bits [3:0] of high-pass filtered
                                            // Z-axis data
#define KXTI9_ZOUT_HPF_L_S      4

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_ZOUT_HPF_H
// register.
//
//*****************************************************************************
#define KXTI9_ZOUT_HPF_H_M      0xFF        // Bits [11:4] of high-pass
                                            // filtered Z-axis data
#define KXTI9_ZOUT_HPF_H_S      0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_XOUT_L register.
//
//*****************************************************************************
#define KXTI9_XOUT_L_M          0xF0        // Bits [3:0] of X-axis data
#define KXTI9_XOUT_L_S          4

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_XOUT_H register.
//
//*****************************************************************************
#define KXTI9_XOUT_H_M          0xFF        // Bits [11:4] of X-axis data
#define KXTI9_XOUT_H_S          0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_YOUT_L register.
//
//*****************************************************************************
#define KXTI9_YOUT_L_M          0xF0        // Bits [3:0] of Y-axis data
#define KXTI9_YOUT_L_S          4

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_YOUT_H register.
//
//*****************************************************************************
#define KXTI9_YOUT_H_M          0xFF        // Bits [11:4] of Y-axis data
#define KXTI9_YOUT_H_S          0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_ZOUT_L register.
//
//*****************************************************************************
#define KXTI9_ZOUT_L_M          0xF0        // Bits [3:0] of Z-axis data
#define KXTI9_ZOUT_L_S          4

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_ZOUT_H register.
//
//*****************************************************************************
#define KXTI9_ZOUT_H_M          0xFF        // Bits [11:4] of Z-axis data
#define KXTI9_ZOUT_H_S          0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_DCST_RESP
// register.
//
//*****************************************************************************
#define KXTI9_DCST_RESP_M       0xFF        // Check field
#define KXTI9_DCST_RESP_DEF     0x55        // Default response
#define KXTI9_DCST_RESP_INIT    0xAA        // Post-initialization response
#define KXTI9_DCST_RESP_S       0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_WHO_AM_I
// register.
//
//*****************************************************************************
#define KXTI9_DCST_RESP_M       0xFF        // Identification field
#define KXTI9_WHO_AM_I_KXTI9    0x04        // KXTI9
#define KXTI9_WHO_AM_I_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_TILT_POS_CUR
// register.
//
//*****************************************************************************
#define KXTI9_TILT_POS_CUR_LE   0x20        // Left state (X-)
#define KXTI9_TILT_POS_CUR_RI   0x10        // Right state (X+)
#define KXTI9_TILT_POS_CUR_DO   0x08        // Down state (Y-)
#define KXTI9_TILT_POS_CUR_UP   0x04        // Up state (Y+)
#define KXTI9_TILT_POS_CUR_FD   0x02        // Face-down state (Z-)
#define KXTI9_TILT_POS_CUR_FU   0x01        // Face-up state (Z+)

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_TILT_POS_PRE
// register.
//
//*****************************************************************************
#define KXTI9_TILT_POS_PRE_LE   0x20        // Left state (X-)
#define KXTI9_TILT_POS_PRE_RI   0x10        // Right state (X+)
#define KXTI9_TILT_POS_PRE_DO   0x08        // Down state (Y-)
#define KXTI9_TILT_POS_PRE_UP   0x04        // Up state (Y+)
#define KXTI9_TILT_POS_PRE_FD   0x02        // Face-down state (Z-)
#define KXTI9_TILT_POS_PRE_FU   0x01        // Face-up state (Z+)

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_INT_SRC1
// register.
//
//*****************************************************************************
#define KXTI9_INT_SRC1_TLE      0x20        // X negative (X-) reported
#define KXTI9_INT_SRC1_TRI      0x10        // X positive (X+) reported
#define KXTI9_INT_SRC1_TDO      0x08        // Y negative (Y-) reported
#define KXTI9_INT_SRC1_TUP      0x04        // Y positive (Y+) reported
#define KXTI9_INT_SRC1_TFD      0x02        // Z negative (Z-) reported
#define KXTI9_INT_SRC1_TFU      0x01        // Z positive (Z+) reported

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_INT_SRC2
// register.
//
//*****************************************************************************
#define KXTI9_INT_SRC2_WMI      0x20        // Buffer sample threshold reached
#define KXTI9_INT_SRC2_DRDY     0x10        // New accel data ready
#define KXTI9_INT_SRC2_TDTS_M   0x0C        // Tap event detected
#define KXTI9_INT_SRC2_TDTS_NONE                                              \
                                0x00        // No tap event
#define KXTI9_INT_SRC2_TDTS_SINGLE                                            \
                                0x01        // Single tap event
#define KXTI9_INT_SRC2_TDTS_DOUBLE                                            \
                                0x02        // Double tap event
#define KXTI9_INT_SRC2_TDTS_DIRECTIONAL                                       \
                                0x03        // Double tap event
#define KXTI9_INT_SRC2_WUFS     0x02        // Wake-up
#define KXTI9_INT_SRC2_TPS      0x01        // Tilt position change
#define KXTI9_INT_SRC2_S        2

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_STATUS register.
//
//*****************************************************************************
#define KXTI9_STATUS_INT        0x10        // Interrupt event

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_INT_REL
// register.
//
//*****************************************************************************
#define KXTI9_INT_REL_M         0xFF        // Data is unpredictable
#define KXTI9_INT_REL_S         0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_CTRL1 register.
//
//*****************************************************************************
#define KXTI9_CTRL1_PC1         0x80        // Operating mode
#define KXTI9_CTRL1_RES         0x40        // Performance (resolution) mode
#define KXTI9_CTRL1_DRDYE       0x20        // New data interrupt enable
#define KXTI9_CTRL1_GSEL_M      0x18        // Acceleration range
#define KXTI9_CTRL1_GSEL_2G     0x00        // +/-2g
#define KXTI9_CTRL1_GSEL_4G     0x01        // +/-4g
#define KXTI9_CTRL1_GSEL_8G     0x02        // +/-8g
#define KXTI9_CTRL1_TDTE        0x04        // Directional tap enable
#define KXTI9_CTRL1_WUFE        0x02        // Wake-up enable
#define KXTI9_CTRL1_TPE         0x01        // Tilt position enable
#define KXTI9_CTRL1_GSEL_S      3

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_CTRL2 register.
//
//*****************************************************************************
#define KXTI9_CTRL2_OTDTH       0x80        // Output data rate selection for
                                            // directional tap
#define KXTI9_CTRL2_LEM         0x20        // Left state tilt enable
#define KXTI9_CTRL2_RIM         0x10        // Right state tilt enable
#define KXTI9_CTRL2_DOM         0x08        // Down state tilt enable
#define KXTI9_CTRL2_UPM         0x04        // Up state tilt enable
#define KXTI9_CTRL2_FDM         0x02        // Face-down state tilt enable
#define KXTI9_CTRL2_FUM         0x01        // Face-up state tilt enable

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_CTRL3 register.
//
//*****************************************************************************
#define KXTI9_CTRL3_SRST        0x80        // Initiate software reset
#define KXTI9_CTRL3_OTP_M       0x60        // Output data rate for tilt
                                            // position
#define KXTI9_CTRL3_OTP_1_6HZ   0x00        // Data rate is 1.6Hz
#define KXTI9_CTRL3_OTP_6_3HZ   0x01        // Data rate is 6.3Hz
#define KXTI9_CTRL3_OTP_12_5HZ  0x02        // Data rate is 12.5Hz
#define KXTI9_CTRL3_OTP_50HZ    0x03        // Data rate is 50Hz
#define KXTI9_CTRL3_OWUF_M      0x60        // Output data rate for motion
                                            // detection and high-pass outputs
#define KXTI9_CTRL3_OWUF_25HZ   0x00        // Data rate is 25Hz
#define KXTI9_CTRL3_OWUF_50HZ   0x01        // Data rate is 50Hz
#define KXTI9_CTRL3_OWUF_100HZ  0x02        // Data rate is 100Hz
#define KXTI9_CTRL3_OWUF_200HZ  0x03        // Data rate is 200Hz
#define KXTI9_CTRL3_DCST        0x10        // Digital communication self-test
#define KXTI9_CTRL3_OTDT_M      0x0C        // Encoding values change based on
                                            // value of OTDTH bit
#define KXTI9_CTRL3_OTDT_0      0x00        // Encoding 0 is 50Hz or 12.5Hz
#define KXTI9_CTRL3_OTDT_1      0x01        // Encoding 1 is 100Hz or 25Hz
#define KXTI9_CTRL3_OTDT_2      0x02        // Encoding 2 is 200Hz or 800Hz
#define KXTI9_CTRL3_OTDT_3      0x03        // Encoding 3 is 400Hz or 1600Hz
#define KXTI9_CTRL3_OTP_S       5
#define KXTI9_CTRL3_OTDT_S      2
#define KXTI9_CTRL3_OWUF_S      0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_INT_CTRL1
// register.
//
//*****************************************************************************
#define KXTI9_INT_CTRL1_IEN     0x20        // Interrupt pin enable
#define KXTI9_INT_CTRL1_IEA     0x10        // Interrupt pin polarity
#define KXTI9_INT_CTRL1_IEL     0x08        // Interrupt pin response
#define KXTI9_INT_CTRL1_IEU     0x04        // Interrupt pin alternative
                                            // response

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_INT_CTRL2
// register.
//
//*****************************************************************************
#define KXTI9_INT_CTRL2_XBW     0x80        // X-axis motion interrupt enable
#define KXTI9_INT_CTRL2_YBW     0x40        // Y-axis motion interrupt enable
#define KXTI9_INT_CTRL2_ZBW     0x20        // Z-axis motion interrupt enable

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_INT_CTRL3
// register.
//
//*****************************************************************************
#define KXTI9_INT_CTRL3_TMEN    0x40        // Tap masking scheme enable
#define KXTI9_INT_CTRL3_TLEM    0x20        // X negative interrupt enable
#define KXTI9_INT_CTRL3_TRIM    0x10        // X positive interrupt enable
#define KXTI9_INT_CTRL3_TDOM    0x08        // Y negative interrupt enable
#define KXTI9_INT_CTRL3_TUPM    0x04        // Y positive interrupt enable
#define KXTI9_INT_CTRL3_TFDM    0x02        // Z negative interrupt enable
#define KXTI9_INT_CTRL3_TFUM    0x01        // Z positive interrupt enable

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_DATA_CTRL
// register.
//
//*****************************************************************************
#define KXTI9_DATA_CTRL_HPFRO_M 0x30        // High-pass filter roll-off
                                            // frequency
#define KXTI9_DATA_CTRL_HPFRO_50HZ                                            \
                                0x00        // Roll-off at 50Hz
#define KXTI9_DATA_CTRL_HPFRO_100HZ                                           \
                                0x01        // Roll-off at 100Hz
#define KXTI9_DATA_CTRL_HPFRO_200HZ                                           \
                                0x02        // Roll-off at 200Hz
#define KXTI9_DATA_CTRL_HPFRO_400HZ                                           \
                                0x03        // Roll-off at 400Hz
#define KXTI9_DATA_CTRL_OSA_M   0x07        // Output data rate and roll-off
                                            // for low-pass filter outputs
#define KXTI9_DATA_CTRL_OSA_12_5HZ                                            \
                                0x00        // 12.5Hz data rate, 6.25Hz LPF
                                            // roll-off
#define KXTI9_DATA_CTRL_OSA_25HZ                                              \
                                0x01        // 25Hz data rate, 12.5Hz LPF
                                            // roll-off
#define KXTI9_DATA_CTRL_OSA_50HZ                                              \
                                0x02        // 50Hz data rate, 25Hz LPF
                                            // roll-off
#define KXTI9_DATA_CTRL_OSA_100HZ                                             \
                                0x03        // 100Hz data rate, 50Hz LPF
                                            // roll-off
#define KXTI9_DATA_CTRL_OSA_200HZ                                             \
                                0x04        // 200Hz data rate, 100Hz LPF
                                            // roll-off
#define KXTI9_DATA_CTRL_OSA_400HZ                                             \
                                0x05        // 400Hz data rate, 200Hz LPF
                                            // roll-off
#define KXTI9_DATA_CTRL_OSA_800HZ                                             \
                                0x06        // 800Hz data rate, 400Hz LPF
                                            // roll-off
#define KXTI9_DATA_CTRL_HPFRO_S 4
#define KXTI9_DATA_CTRL_OSA_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_TILT_TIMER
// register.
//
//*****************************************************************************
#define KXTI9_TILT_TIMER_M      0xFF        // Initial timer count
#define KXTI9_TILT_TIMER_S      0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_WUF_TIMER
// register.
//
//*****************************************************************************
#define KXTI9_WUF_TIMER_M       0xFF        // Initial timer count
#define KXTI9_WUF_TIMER_S       0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_TDT_TIMER
// register.
//
//*****************************************************************************
#define KXTI9_TDT_TIMER_M       0xFF        // Initial timer count
#define KXTI9_TDT_TIMER_S       0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_TDT_H_THRESH
// register.
//
//*****************************************************************************
#define KXTI9_TDT_H_THRESH_M    0xFF        // Threshold high value
#define KXTI9_TDT_H_THRESH_S    0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_TDT_L_THRESH
// register.
//
//*****************************************************************************
#define KXTI9_TDT_L_THRESH_M    0xFF        // Threshold low value
#define KXTI9_TDT_L_THRESH_S    0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_TDT_TAP_TIMER
// register.
//
//*****************************************************************************
#define KXTI9_TDT_TAP_TIMER_M   0xFF        // Tap event counter
#define KXTI9_TDT_TAP_TIMER_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_TDT_TOTAL_TIMER
// register.
//
//*****************************************************************************
#define KXTI9_TDT_TOTAL_TIMER_M 0xFF        // Double tap event counter
#define KXTI9_TDT_TOTAL_TIMER_S 0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// KXTI9_O_TDT_LATENCY_TIMER register.
//
//*****************************************************************************
#define KXTI9_TDT_LATENCY_TIMER_M                                             \
                                0xFF        // Tap event latency counter
#define KXTI9_TDT_LATENCY_TIMER_S                                             \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_TDT_WINDOW_TIMER
// register.
//
//*****************************************************************************
#define KXTI9_TDT_WINDOW_TIMER_M                                              \
                                0xFF        // Tap event window counter
#define KXTI9_TDT_WINDOW_TIMER_S                                              \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_BUF_CTRL1
// register.
//
//*****************************************************************************
#define KXTI9_BUF_CTRL1_M       0x7F        // Buffer sample threshold
#define KXTI9_BUF_CTRL1_S       0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_BUF_CTRL2
// register.
//
//*****************************************************************************
#define KXTI9_BUF_CTRL2_BUFE    0x80        // Buffer enable
#define KXTI9_BUF_CTRL2_BUF_RES 0x40        // Buffer resolution
#define KXTI9_BUF_CTRL2_BUF_M_M 0x03        // Buffer mode
#define KXTI9_BUF_CTRL2_BUF_M_FIFO                                            \
                                0x00        // FIFO mode
#define KXTI9_BUF_CTRL2_BUF_M_STREAM                                          \
                                0x01        // Stream mode
#define KXTI9_BUF_CTRL2_BUF_M_TRIG                                            \
                                0x02        // Trigger mode
#define KXTI9_BUF_CTRL2_BUF_M_FILO                                            \
                                0x03        // FILO mode
#define KXTI9_BUF_CTRL2_BUF_M_S 0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_STATUS1
// register.
//
//*****************************************************************************
#define KXTI9_STATUS1_SMP_LEV_M 0xFF        // Number of bytes in sample buffer
#define KXTI9_STATUS1_SMP_LEV_S 0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_STATUS2
// register.
//
//*****************************************************************************
#define KXTI9_STATUS2_BUF_TRIG  0x80        // Trigger mode status

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_BUF_CLEAR
// register.
//
//*****************************************************************************
#define KXTI9_BUF_CLEAR_M       0xFF        // Data is unpredictable
#define KXTI9_BUF_CLEAR_S       0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_SELF_TEST
// register.
//
//*****************************************************************************
#define KXTI9_SELF_TEST_M       0xFF        // Writing 0xCA enables MEMS
                                            // self-test
#define KXTI9_SELF_TEST_S       0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_WUF_THRESH
// register.
//
//*****************************************************************************
#define KXTI9_WUF_THRESH_M      0xFF        // Acceleration wake-up threshold
#define KXTI9_WUF_THRESH_S      0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_TILT_ANGLE
// register.
//
//*****************************************************************************
#define KXTI9_TILT_ANGLE_M      0xFF        // Tilt angle threshold
#define KXTI9_TILT_ANGLE_S      0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_HYST_SET
// register.
//
//*****************************************************************************
#define KXTI9_HYST_SET_RES_M    0xE0        // Factory set value - do not
                                            // change
#define KXTI9_HYST_SET_HYST_M   0x1F        // Hysteresis angle
#define KXTI9_HYST_SET_RES_S    5
#define KXTI9_HYST_SET_HYST_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the KXTI9_O_BUF_READ
// register.
//
//*****************************************************************************
#define KXTI9_BUF_READ_M        0xFF        // Read data from buffer
#define KXTI9_BUF_READ_S        0

#endif // __SENSORLIB_HW_KXTI9_H__
