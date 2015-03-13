//*****************************************************************************
//
// hw_mpu6050.h - Macros used when accessing the Invensense MPU6050
//                accelerometer/gyroscope/magnetometer.
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

#ifndef __SENSORLIB_HW_MPU6050_H__
#define __SENSORLIB_HW_MPU6050_H__

//*****************************************************************************
//
// The following are defines for the MPU6050 register addresses.
//
//*****************************************************************************
#define MPU6050_O_SELF_TEST_X   0x0D        // Self test X register
#define MPU6050_O_SELF_TEST_Y   0x0E        // Self test Y register
#define MPU6050_O_SELF_TEST_Z   0x0F        // Self test Z register
#define MPU6050_O_SELF_TEST_A   0x10        // Self test A register
#define MPU6050_O_SMPLRT_DIV    0x19        // Sample rate divider register
#define MPU6050_O_CONFIG        0x1A        // Configuration register
#define MPU6050_O_GYRO_CONFIG   0x1B        // Gyro configuration register
#define MPU6050_O_ACCEL_CONFIG  0x1C        // Accelerometer configuration
                                            // register
#define MPU6050_O_MOT_THR       0x1F        // Motion detection threshold
                                            // register
#define MPU6050_O_FIFO_EN       0x23        // FIFO enable register
#define MPU6050_O_I2C_MST_CTRL  0x24        // I2C master control register
#define MPU6050_O_I2C_SLV0_ADDR 0x25        // I2C slave 0 address register
#define MPU6050_O_I2C_SLV0_REG  0x26        // I2C slave 0 register number
                                            // register
#define MPU6050_O_I2C_SLV0_CTRL 0x27        // I2C slave 0 control register
#define MPU6050_O_I2C_SLV1_ADDR 0x28        // I2C slave 1 address register
#define MPU6050_O_I2C_SLV1_REG  0x29        // I2C slave 1 register number
                                            // register
#define MPU6050_O_I2C_SLV1_CTRL 0x2A        // I2C slave 1 control register
#define MPU6050_O_I2C_SLV2_ADDR 0x2B        // I2C slave 2 address register
#define MPU6050_O_I2C_SLV2_REG  0x2C        // I2C slave 2 register number
                                            // register
#define MPU6050_O_I2C_SLV2_CTRL 0x2D        // I2C slave 2 control register
#define MPU6050_O_I2C_SLV3_ADDR 0x2E        // I2C slave 3 address register
#define MPU6050_O_I2C_SLV3_REG  0x2F        // I2C slave 3 register number
                                            // register
#define MPU6050_O_I2C_SLV3_CTRL 0x30        // I2C slave 3 control register
#define MPU6050_O_I2C_SLV4_ADDR 0x31        // I2C slave 4 address register
#define MPU6050_O_I2C_SLV4_REG  0x32        // I2C slave 4 register number
                                            // register
#define MPU6050_O_I2C_SLV4_DO   0x33        // I2C slave 4 output data register
#define MPU6050_O_I2C_SLV4_CTRL 0x34        // I2C slave 4 control register
#define MPU6050_O_I2C_SLV4_DI   0x35        // I2C slave 4 input data register
#define MPU6050_O_I2C_MST_STATUS                                              \
                                0x36        // I2C master status register
#define MPU6050_O_INT_PIN_CFG   0x37        // INT pin configuration register
#define MPU6050_O_INT_ENABLE    0x38        // Interrupt enable register
#define MPU6050_O_INT_STATUS    0x3A        // Interrupt status register
#define MPU6050_O_ACCEL_XOUT_H  0x3B        // X-axis acceleration data MSB
                                            // register
#define MPU6050_O_ACCEL_XOUT_L  0x3C        // X-axis acceleration data LSB
                                            // register
#define MPU6050_O_ACCEL_YOUT_H  0x3D        // Y-axis acceleration data MSB
                                            // register
#define MPU6050_O_ACCEL_YOUT_L  0x3E        // Y-axis accelearation data LSB
                                            // register
#define MPU6050_O_ACCEL_ZOUT_H  0x3F        // Z-axis acceleration data MSB
                                            // register
#define MPU6050_O_ACCEL_ZOUT_L  0x40        // Z-axis acceleration data LSB
                                            // register
#define MPU6050_O_TEMP_OUT_H    0x41        // Temperature data MSB register
#define MPU6050_O_TEMP_OUT_L    0x42        // Temperature data LSB register
#define MPU6050_O_GYRO_XOUT_H   0x43        // X-axis gyro data MSB register
#define MPU6050_O_GYRO_XOUT_L   0x44        // X-axis gyro data LSB register
#define MPU6050_O_GYRO_YOUT_H   0x45        // Y-axis gyro data MSB register
#define MPU6050_O_GYRO_YOUT_L   0x46        // Y-axis gyro data LSB register
#define MPU6050_O_GYRO_ZOUT_H   0x47        // Z-axis gyro data MSB register
#define MPU6050_O_GYRO_ZOUT_L   0x48        // Z-axis gyro data LSB register
#define MPU6050_O_EXT_SENS_DATA_00                                            \
                                0x49        // External sensor data 0 register
#define MPU6050_O_EXT_SENS_DATA_01                                            \
                                0x4A        // External sensor data 1 register
#define MPU6050_O_EXT_SENS_DATA_02                                            \
                                0x4B        // External sensor data 2 register
#define MPU6050_O_EXT_SENS_DATA_03                                            \
                                0x4C        // External sensor data 3 register
#define MPU6050_O_EXT_SENS_DATA_04                                            \
                                0x4D        // External sensor data 4 register
#define MPU6050_O_EXT_SENS_DATA_05                                            \
                                0x4E        // External sensor data 5 register
#define MPU6050_O_EXT_SENS_DATA_06                                            \
                                0x4F        // External sensor data 6 register
#define MPU6050_O_EXT_SENS_DATA_07                                            \
                                0x50        // External sensor data 7 register
#define MPU6050_O_EXT_SENS_DATA_08                                            \
                                0x51        // External sensor data 8 register
#define MPU6050_O_EXT_SENS_DATA_09                                            \
                                0x52        // External sensor data 9 register
#define MPU6050_O_EXT_SENS_DATA_10                                            \
                                0x53        // External sensor data 10 register
#define MPU6050_O_EXT_SENS_DATA_11                                            \
                                0x54        // External sensor data 11 register
#define MPU6050_O_EXT_SENS_DATA_12                                            \
                                0x55        // External sensor data 12 register
#define MPU6050_O_EXT_SENS_DATA_13                                            \
                                0x56        // External sensor data 13 register
#define MPU6050_O_EXT_SENS_DATA_14                                            \
                                0x57        // External sensor data 14 register
#define MPU6050_O_EXT_SENS_DATA_15                                            \
                                0x58        // External sensor data 15 register
#define MPU6050_O_EXT_SENS_DATA_16                                            \
                                0x59        // External sensor data 16 register
#define MPU6050_O_EXT_SENS_DATA_17                                            \
                                0x5A        // External sensor data 17 register
#define MPU6050_O_EXT_SENS_DATA_18                                            \
                                0x5B        // External sensor data 18 register
#define MPU6050_O_EXT_SENS_DATA_19                                            \
                                0x5C        // External sensor data 19 register
#define MPU6050_O_EXT_SENS_DATA_20                                            \
                                0x5D        // External sensor data 20 register
#define MPU6050_O_EXT_SENS_DATA_21                                            \
                                0x5E        // External sensor data 21 register
#define MPU6050_O_EXT_SENS_DATA_22                                            \
                                0x5F        // External sensor data 22 register
#define MPU6050_O_EXT_SENS_DATA_23                                            \
                                0x60        // External sensor data 23 register
#define MPU6050_O_I2C_SLV0_DO   0x63        // I2C slave 0 output data register
#define MPU6050_O_I2C_SLV1_DO   0x64        // I2C slave 1 output data register
#define MPU6050_O_I2C_SLV2_DO   0x65        // I2C slave 2 output data register
#define MPU6050_O_I2C_SLV3_DO   0x66        // I2C slave 3 output data register
#define MPU6050_O_I2C_MST_DELAY_CTRL                                          \
                                0x67        // I2C master delay control
                                            // register
#define MPU6050_O_SIGNAL_PATH_RESET                                           \
                                0x68        // Signal path reset register
#define MPU6050_O_MOT_DETECT_CTRL                                             \
                                0x69        // Motion detection control
                                            // register
#define MPU6050_O_USER_CTRL     0x6A        // User control register
#define MPU6050_O_PWR_MGMT_1    0x6B        // Power management 1 register
#define MPU6050_O_PWR_MGMT_2    0x6C        // Power management 2 register
#define MPU6050_O_FIFO_COUNTH   0x72        // FIFO count MSB register
#define MPU6050_O_FIFO_COUNTL   0x73        // FIFO count LSB register
#define MPU6050_O_FIFO_R_W      0x74        // FIFO read write register
#define MPU6050_O_WHO_AM_I      0x75        // Who am I register

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_SELF_TEST_X
// register.
//
//*****************************************************************************
#define MPU6050_SELF_TEST_X_XA_TEST_M                                         \
                                0xE0        // Accelerometer XA_TEST[4:2]
#define MPU6050_SELF_TEST_X_XG_TEST_M                                         \
                                0x1F        // Gyro XG_TEST[4:0]
#define MPU6050_SELF_TEST_X_XA_TEST_S                                         \
                                5
#define MPU6050_SELF_TEST_X_XG_TEST_S                                         \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_SELF_TEST_Y
// register.
//
//*****************************************************************************
#define MPU6050_SELF_TEST_Y_YA_TEST_M                                         \
                                0xE0        // Accelerometer YA_TEST[4:2]
#define MPU6050_SELF_TEST_Y_YG_TEST_M                                         \
                                0x1F        // Gyro YG_TEST[4:0]
#define MPU6050_SELF_TEST_Y_YA_TEST_S                                         \
                                5
#define MPU6050_SELF_TEST_Y_YG_TEST_S                                         \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_SELF_TEST_Z
// register.
//
//*****************************************************************************
#define MPU6050_SELF_TEST_Z_ZA_TEST_M                                         \
                                0xE0        // Accelerometer ZA_TEST[4:2]
#define MPU6050_SELF_TEST_Z_ZG_TEST_M                                         \
                                0x1F        // Gyro ZG_TEST[4:0]
#define MPU6050_SELF_TEST_Z_ZA_TEST_S                                         \
                                5
#define MPU6050_SELF_TEST_Z_ZG_TEST_S                                         \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_SELF_TEST_A
// register.
//
//*****************************************************************************
#define MPU6050_SELF_TEST_A_XA_TEST_M                                         \
                                0x30        // Accelerometer XA_TEST[1:0]
#define MPU6050_SELF_TEST_A_YA_TEST_M                                         \
                                0x0C        // Accelerometer YA_TEST[1:0]
#define MPU6050_SELF_TEST_A_ZA_TEST_M                                         \
                                0x03        // Accelerometer ZA_TEST[1:0]
#define MPU6050_SELF_TEST_A_XA_TEST_S                                         \
                                4
#define MPU6050_SELF_TEST_A_YA_TEST_S                                         \
                                2
#define MPU6050_SELF_TEST_A_ZA_TEST_S                                         \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_SMPLRT_DIV
// register.
//
//*****************************************************************************
#define MPU6050_SMPLRT_DIV_M    0xFF        // Gyro output rate divider
#define MPU6050_SMPLRT_DIV_S    0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_CONFIG
// register.
//
//*****************************************************************************
#define MPU6050_CONFIG_EXT_SYNC_SET_M                                         \
                                0x38        // FSYNC pin sample location
#define MPU6050_CONFIG_EXT_SYNC_SET_DIS                                       \
                                0x00        // FSYNC input disabled
#define MPU6050_CONFIG_EXT_SYNC_SET_TEMP_OUT_L                                \
                                0x08        // FSYNC on TEMP_OUT_L[0]
#define MPU6050_CONFIG_EXT_SYNC_SET_GYRO_XOUT_L                               \
                                0x10        // FSYNC on GYRO_XOUT_L[0]
#define MPU6050_CONFIG_EXT_SYNC_SET_GYRO_YOUT_L                               \
                                0x18        // FSYNC on GYRO_YOUT_L[0]
#define MPU6050_CONFIG_EXT_SYNC_SET_GYRO_ZOUT_L                               \
                                0x20        // FSYNC on GYRO_ZOUT_L[0]
#define MPU6050_CONFIG_EXT_SYNC_SET_ACCEL_XOUT_L                              \
                                0x28        // FSYNC on ACCEL_XOUT_L[0]
#define MPU6050_CONFIG_EXT_SYNC_SET_ACCEL_YOUT_L                              \
                                0x30        // FSYNC on ACCEL_YOUT_L[0]
#define MPU6050_CONFIG_EXT_SYNC_SET_ACCEL_ZOUT_L                              \
                                0x38        // FSYNC on ACCEL_ZOUT_L[0]
#define MPU6050_CONFIG_DLPF_CFG_M                                             \
                                0x07        // Digital low-pass filter
                                            // configuration
#define MPU6050_CONFIG_DLPF_CFG_260_256                                       \
                                0x00        // 260 Hz accelerometer bandwidth,
                                            // 256 Hz gyro bandwidth
#define MPU6050_CONFIG_DLPF_CFG_184_188                                       \
                                0x01        // 184 Hz accelerometer bandwidth,
                                            // 188 Hz gyro bandwidth
#define MPU6050_CONFIG_DLPF_CFG_94_98                                         \
                                0x02        // 94 Hz accelerometer bandwidth,
                                            // 98 Hz gyro bandwidth
#define MPU6050_CONFIG_DLPF_CFG_44_42                                         \
                                0x03        // 44 Hz accelerometer bandwidth,
                                            // 42 Hz gyro bandwidth
#define MPU6050_CONFIG_DLPF_CFG_21_20                                         \
                                0x04        // 21 Hz accelerometer bandwidth,
                                            // 20 Hz gyro bandwidth
#define MPU6050_CONFIG_DLPF_CFG_10_10                                         \
                                0x05        // 10 Hz accelerometer bandwidth,
                                            // 10 Hz gyro bandwidth
#define MPU6050_CONFIG_DLPF_CFG_5_5                                           \
                                0x06        // 5 Hz accelerometer bandwidth, 5
                                            // Hz gyro bandwidth
#define MPU6050_CONFIG_EXT_SYNC_SET_S                                         \
                                3
#define MPU6050_CONFIG_DLPF_CFG_S                                             \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_GYRO_CONFIG
// register.
//
//*****************************************************************************
#define MPU6050_GYRO_CONFIG_FS_SEL_M                                          \
                                0x18        // Gyro full-scale range
#define MPU6050_GYRO_CONFIG_FS_SEL_250                                        \
                                0x00        // Gyro full-scale range +/- 250
                                            // degrees/sec
#define MPU6050_GYRO_CONFIG_FS_SEL_500                                        \
                                0x08        // Gyro full-scale range +/- 500
                                            // degrees/sec
#define MPU6050_GYRO_CONFIG_FS_SEL_1000                                       \
                                0x10        // Gyro full-scale range +/- 1000
                                            // degrees/sec
#define MPU6050_GYRO_CONFIG_FS_SEL_2000                                       \
                                0x18        // Gyro full-scale range +/- 2000
                                            // degrees/sec
#define MPU6050_GYRO_CONFIG_FS_SEL_S                                          \
                                3

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_ACCEL_CONFIG
// register.
//
//*****************************************************************************
#define MPU6050_ACCEL_CONFIG_XA_ST                                            \
                                0x80        // X-axis accelerometer self-test
                                            // enable
#define MPU6050_ACCEL_CONFIG_YA_ST                                            \
                                0x40        // Y-axis accelerometer self-test
                                            // enable
#define MPU6050_ACCEL_CONFIG_ZA_ST                                            \
                                0x20        // Z-axis accelerometer self-test
                                            // enable
#define MPU6050_ACCEL_CONFIG_AFS_SEL_M                                        \
                                0x18        // Accelerometer full-scale range
#define MPU6050_ACCEL_CONFIG_AFS_SEL_2G                                       \
                                0x00        // Accelerometer full-scale range 2
                                            // g
#define MPU6050_ACCEL_CONFIG_AFS_SEL_4G                                       \
                                0x08        // Accelerometer full-scale range 4
                                            // g
#define MPU6050_ACCEL_CONFIG_AFS_SEL_8G                                       \
                                0x10        // Accelerometer full-scale range 8
                                            // g
#define MPU6050_ACCEL_CONFIG_AFS_SEL_16G                                      \
                                0x18        // Accelerometer full-scale range
                                            // 16 g
#define MPU6050_ACCEL_CONFIG_AFS_SEL_S                                        \
                                3

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_MOT_THR
// register.
//
//*****************************************************************************
#define MPU6050_MOT_THR_M       0xFF        // Motion detection threshold value
#define MPU6050_MOT_THR_S       0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_FIFO_EN
// register.
//
//*****************************************************************************
#define MPU6050_FIFO_EN_TEMP    0x80        // Temperature sensor FIFO enable
#define MPU6050_FIFO_EN_XG      0x40        // X-axis gyro FIFO enable
#define MPU6050_FIFO_EN_YG      0x20        // Y-axis gyro FIFO enable
#define MPU6050_FIFO_EN_ZG      0x10        // Z-axis gyro FIFO enable
#define MPU6050_FIFO_EN_ACCEL   0x08        // Accelerometer FIFO enable
#define MPU6050_FIFO_EN_SLV2    0x04        // Slave 2 FIFO enable
#define MPU6050_FIFO_EN_SLV1    0x02        // Slave 1 FIFO enable
#define MPU6050_FIFO_EN_SLV0    0x01        // Slave 0 FIFO enable

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_MST_CTRL
// register.
//
//*****************************************************************************
#define MPU6050_I2C_MST_CTRL_MULT_MST_EN                                      \
                                0x80        // Multi-master enable
#define MPU6050_I2C_MST_CTRL_WAIT_FOR_ES                                      \
                                0x40        // Wait for external sensor data
#define MPU6050_I2C_MST_CTRL_SLV3_FIFO_EN                                     \
                                0x20        // Slave 3 FIFO enable
#define MPU6050_I2C_MST_CTRL_I2C_MST_P_NSR                                    \
                                0x10        // No repeated start conditions
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_M                                    \
                                0x0F        // I2C master clock speed
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_348                                  \
                                0x00        // 348 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_333                                  \
                                0x01        // 333 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_320                                  \
                                0x02        // 320 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_308                                  \
                                0x03        // 308 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_296                                  \
                                0x04        // 296 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_286                                  \
                                0x05        // 286 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_276                                  \
                                0x06        // 276 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_267                                  \
                                0x07        // 267 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_258                                  \
                                0x08        // 258 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_500                                  \
                                0x09        // 500 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_471                                  \
                                0x0A        // 471 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_444                                  \
                                0x0B        // 444 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_421                                  \
                                0x0C        // 421 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_400                                  \
                                0x0D        // 400 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_381                                  \
                                0x0E        // 381 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_364                                  \
                                0x0F        // 364 kHz I2C master clock
#define MPU6050_I2C_MST_CTRL_I2C_MST_CLK_S                                    \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV0_ADDR
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV0_ADDR_RW                                              \
                                0x80        // Read/not write
#define MPU6050_I2C_SLV0_ADDR_M 0x7F        // Slave address
#define MPU6050_I2C_SLV0_ADDR_S 0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV0_REG
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV0_REG_M  0xFF        // Slave register number
#define MPU6050_I2C_SLV0_REG_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV0_CTRL
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV0_CTRL_EN                                              \
                                0x80        // Enable slave
#define MPU6050_I2C_SLV0_CTRL_BYTE_SW                                         \
                                0x40        // Byte-swap word pairs
#define MPU6050_I2C_SLV0_CTRL_REG_DIS                                         \
                                0x20        // Disable register number transfer
#define MPU6050_I2C_SLV0_CTRL_GRP                                             \
                                0x10        // Word pair grouping
#define MPU6050_I2C_SLV0_CTRL_LEN_M                                           \
                                0x0F        // Number of bytes to transfer
#define MPU6050_I2C_SLV0_CTRL_LEN_S                                           \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV1_ADDR
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV1_ADDR_RW                                              \
                                0x80        // Read/not write
#define MPU6050_I2C_SLV1_ADDR_M 0x7F        // Slave address
#define MPU6050_I2C_SLV1_ADDR_S 0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV1_REG
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV1_REG_M  0xFF        // Slave register number
#define MPU6050_I2C_SLV1_REG_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV1_CTRL
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV1_CTRL_EN                                              \
                                0x80        // Enable slave
#define MPU6050_I2C_SLV1_CTRL_BYTE_SW                                         \
                                0x40        // Byte-swap word pairs
#define MPU6050_I2C_SLV1_CTRL_REG_DIS                                         \
                                0x20        // Disable register number transfer
#define MPU6050_I2C_SLV1_CTRL_GRP                                             \
                                0x10        // Word pair grouping
#define MPU6050_I2C_SLV1_CTRL_LEN_M                                           \
                                0x0F        // Number of bytes to transfer
#define MPU6050_I2C_SLV1_CTRL_LEN_S                                           \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV2_ADDR
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV2_ADDR_RW                                              \
                                0x80        // Read/not write
#define MPU6050_I2C_SLV2_ADDR_M 0x7F        // Slave address
#define MPU6050_I2C_SLV2_ADDR_S 0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV2_REG
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV2_REG_M  0xFF        // Slave register number
#define MPU6050_I2C_SLV2_REG_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV2_CTRL
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV2_CTRL_EN                                              \
                                0x80        // Enable slave
#define MPU6050_I2C_SLV2_CTRL_BYTE_SW                                         \
                                0x40        // Byte-swap word pairs
#define MPU6050_I2C_SLV2_CTRL_REG_DIS                                         \
                                0x20        // Disable register number transfer
#define MPU6050_I2C_SLV2_CTRL_GRP                                             \
                                0x10        // Word pair grouping
#define MPU6050_I2C_SLV2_CTRL_LEN_M                                           \
                                0x0F        // Number of bytes to transfer
#define MPU6050_I2C_SLV2_CTRL_LEN_S                                           \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV3_ADDR
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV3_ADDR_RW                                              \
                                0x80        // Read/not write
#define MPU6050_I2C_SLV3_ADDR_M 0x7F        // Slave address
#define MPU6050_I2C_SLV3_ADDR_S 0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV3_REG
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV3_REG_M  0xFF        // Slave register number
#define MPU6050_I2C_SLV3_REG_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV3_CTRL
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV3_CTRL_EN                                              \
                                0x80        // Enable slave
#define MPU6050_I2C_SLV3_CTRL_BYTE_SW                                         \
                                0x40        // Byte-swap word pairs
#define MPU6050_I2C_SLV3_CTRL_REG_DIS                                         \
                                0x20        // Disable register number transfer
#define MPU6050_I2C_SLV3_CTRL_GRP                                             \
                                0x10        // Word pair grouping
#define MPU6050_I2C_SLV3_CTRL_LEN_M                                           \
                                0x0F        // Number of bytes to transfer
#define MPU6050_I2C_SLV3_CTRL_LEN_S                                           \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV4_ADDR
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV4_ADDR_RW                                              \
                                0x80        // Read/not write
#define MPU6050_I2C_SLV4_ADDR_M 0x7F        // Slave address
#define MPU6050_I2C_SLV4_ADDR_S 0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV4_REG
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV4_REG_M  0xFF        // Slave register number
#define MPU6050_I2C_SLV4_REG_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV4_CTRL
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV4_CTRL_EN                                              \
                                0x80        // Enable slave
#define MPU6050_I2C_SLV4_CTRL_INT_EN                                          \
                                0x40        // Interrupt enable
#define MPU6050_I2C_SLV4_CTRL_REG_DIS                                         \
                                0x20        // Disable register number transfer
#define MPU6050_I2C_SLV4_CTRL_I2C_MST_DLY_M                                   \
                                0x1F        // Slave access delay
#define MPU6050_I2C_SLV4_CTRL_I2C_MST_DLY_S                                   \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV4_DI
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV4_DI_M   0xFF        // Input data
#define MPU6050_I2C_SLV4_DI_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_MST_STATUS
// register.
//
//*****************************************************************************
#define MPU6050_I2C_MST_STATUS_PASS_THROUGH                                   \
                                0x80        // Pass through FSYNC interrupt
                                            // status
#define MPU6050_I2C_MST_STATUS_I2C_SLV4_DONE                                  \
                                0x40        // I2C slave 4 completion status
#define MPU6050_I2C_MST_STATUS_I2C_LOST_ARB                                   \
                                0x20        // I2C arbitration lost status
#define MPU6050_I2C_MST_STATUS_I2C_SLV4_NACK                                  \
                                0x10        // I2C slave 4 NACK status
#define MPU6050_I2C_MST_STATUS_I2C_SLV3_NACK                                  \
                                0x08        // I2C slave 3 NACK status
#define MPU6050_I2C_MST_STATUS_I2C_SLV2_NACK                                  \
                                0x04        // I2C slave 2 NACK status
#define MPU6050_I2C_MST_STATUS_I2C_SLV1_NACK                                  \
                                0x02        // I2C slave 1 NACK status
#define MPU6050_I2C_MST_STATUS_I2C_SLV0_NACK                                  \
                                0x01        // I2C slave 0 NACK status

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_INT_PIN_CFG
// register.
//
//*****************************************************************************
#define MPU6050_INT_PIN_CFG_INT_LEVEL                                         \
                                0x80        // INT pin active low
#define MPU6050_INT_PIN_CFG_INT_OPEN                                          \
                                0x40        // INT pin open-drain
#define MPU6050_INT_PIN_CFG_LATCH_INT_EN                                      \
                                0x20        // Latch INT pin output
#define MPU6050_INT_PIN_CFG_INT_RD_CLEAR                                      \
                                0x10        // Interrupt clear on any read
#define MPU6050_INT_PIN_CFG_FSYNC_INT_LEVEL                                   \
                                0x08        // FSYNC pin active low
#define MPU6050_INT_PIN_CFG_FSYNC_INT_EN                                      \
                                0x04        // FSYNC pin interrupt enable
#define MPU6050_INT_PIN_CFG_I2C_BYPASS_EN                                     \
                                0x02        // I2C bypass enable

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_INT_ENABLE
// register.
//
//*****************************************************************************
#define MPU6050_INT_ENABLE_MOT_EN                                             \
                                0x40        // Motion detection interrupt
                                            // enable
#define MPU6050_INT_ENABLE_FIFO_OFLOW_EN                                      \
                                0x10        // FIFO overflow interrupt enable
#define MPU6050_INT_ENABLE_I2C_MST_INT_EN                                     \
                                0x08        // I2C master interrupt enable
#define MPU6050_INT_ENABLE_DATA_RDY_EN                                        \
                                0x01        // Data ready interrupt enable

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_INT_STATUS
// register.
//
//*****************************************************************************
#define MPU6050_INT_STATUS_MOT_INT                                            \
                                0x40        // Motion detection interrupt
                                            // status
#define MPU6050_INT_STATUS_FIFO_OFLOW_INT                                     \
                                0x10        // FIFO overflow interrupt status
#define MPU6050_INT_STATUS_I2C_MST_INT                                        \
                                0x08        // I2C master interrupt status
#define MPU6050_INT_STATUS_DATA_RDY_INT                                       \
                                0x01        // Data ready interrupt status

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_ACCEL_XOUT_H
// register.
//
//*****************************************************************************
#define MPU6050_ACCEL_XOUT_H_M  0xFF        // Bits [15:8] of X-axis
                                            // acceleration data
#define MPU6050_ACCEL_XOUT_H_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_ACCEL_XOUT_L
// register.
//
//*****************************************************************************
#define MPU6050_ACCEL_XOUT_L_M  0xFF        // Bits [7:0] of X-axis
                                            // acceleration data
#define MPU6050_ACCEL_XOUT_L_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_ACCEL_YOUT_H
// register.
//
//*****************************************************************************
#define MPU6050_ACCEL_YOUT_H_M  0xFF        // Bits [15:8] of Y-axis
                                            // acceleration data
#define MPU6050_ACCEL_YOUT_H_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_ACCEL_YOUT_L
// register.
//
//*****************************************************************************
#define MPU6050_ACCEL_YOUT_L_M  0xFF        // Bits [7:0] of Y-axis
                                            // acceleration data
#define MPU6050_ACCEL_YOUT_L_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_ACCEL_ZOUT_H
// register.
//
//*****************************************************************************
#define MPU6050_ACCEL_ZOUT_H_M  0xFF        // Bits [15:8] of Z-axis
                                            // acceleration data
#define MPU6050_ACCEL_ZOUT_H_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_ACCEL_ZOUT_L
// register.
//
//*****************************************************************************
#define MPU6050_ACCEL_ZOUT_L_M  0xFF        // Bits [7:0] of Z-axis
                                            // acceleration data
#define MPU6050_ACCEL_ZOUT_L_S  0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_TEMP_OUT_H
// register.
//
//*****************************************************************************
#define MPU6050_ACCEL_TEMP_OUT_H_M                                            \
                                0xFF        // Bits [15:8] of temperature data
#define MPU6050_ACCEL_TEMP_OUT_H_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_TEMP_OUT_L
// register.
//
//*****************************************************************************
#define MPU6050_ACCEL_TEMP_OUT_L_M                                            \
                                0xFF        // Bits [7:0] of temperature data
#define MPU6050_ACCEL_TEMP_OUT_L_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_GYRO_XOUT_H
// register.
//
//*****************************************************************************
#define MPU6050_GYRO_XOUT_H_M   0xFF        // Bits [15:8] of X-axis gyro data
#define MPU6050_GYRO_XOUT_H_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_GYRO_XOUT_L
// register.
//
//*****************************************************************************
#define MPU6050_GYRO_XOUT_L_M   0xFF        // Bits [7:0] of X-axis gyro data
#define MPU6050_GYRO_XOUT_L_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_GYRO_YOUT_H
// register.
//
//*****************************************************************************
#define MPU6050_GYRO_YOUT_H_M   0xFF        // Bits [15:8] of Y-axis gyro data
#define MPU6050_GYRO_YOUT_H_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_GYRO_YOUT_L
// register.
//
//*****************************************************************************
#define MPU6050_GYRO_YOUT_L_M   0xFF        // Bits [7:0] of Y-axis gyro data
#define MPU6050_GYRO_YOUT_L_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_GYRO_ZOUT_H
// register.
//
//*****************************************************************************
#define MPU6050_GYRO_ZOUT_H_M   0xFF        // Bits [15:8] of Z-axis gyro data
#define MPU6050_GYRO_ZOUT_H_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_GYRO_ZOUT_L
// register.
//
//*****************************************************************************
#define MPU6050_GYRO_ZOUT_L_M   0xFF        // Bits [7:0] of Z-axis gyro data
#define MPU6050_GYRO_ZOUT_L_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_00 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_00_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_00_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_01 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_01_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_01_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_02 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_02_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_02_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_03 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_03_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_03_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_04 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_04_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_04_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_05 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_05_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_05_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_06 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_06_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_06_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_07 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_07_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_07_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_08 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_08_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_08_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_09 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_09_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_09_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_10 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_10_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_10_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_11 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_11_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_11_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_12 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_12_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_12_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_13 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_13_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_13_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_14 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_14_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_14_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_15 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_15_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_15_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_16 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_16_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_16_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_17 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_17_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_17_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_18 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_18_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_18_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_19 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_19_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_19_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_20 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_20_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_20_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_21 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_21_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_21_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_22 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_22_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_22_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_EXT_SENS_DATA_23 register.
//
//*****************************************************************************
#define MPU6050_EXT_SENS_DATA_23_M                                            \
                                0xFF        // External sensor data
#define MPU6050_EXT_SENS_DATA_23_S                                            \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV0_DO
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV0_DO_M   0xFF        // Output data
#define MPU6050_I2C_SLV0_DO_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV1_DO
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV1_DO_M   0xFF        // Output data
#define MPU6050_I2C_SLV1_DO_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV2_DO
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV2_DO_M   0xFF        // Output data
#define MPU6050_I2C_SLV2_DO_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_I2C_SLV3_DO
// register.
//
//*****************************************************************************
#define MPU6050_I2C_SLV3_DO_M   0xFF        // Output data
#define MPU6050_I2C_SLV3_DO_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_I2C_MST_DELAY_CTRL register.
//
//*****************************************************************************
#define MPU6050_I2C_MST_DELAY_CTRL_DELAY_ES_SHADOW                            \
                                0x80        // Delay external sensor data
#define MPU6050_I2C_MST_DELAY_CTRL_I2C_SLV4_DLY_EN                            \
                                0x10        // I2C slave 4 delay enable
#define MPU6050_I2C_MST_DELAY_CTRL_I2C_SLV3_DLY_EN                            \
                                0x08        // I2C slave 3 delay enable
#define MPU6050_I2C_MST_DELAY_CTRL_I2C_SLV2_DLY_EN                            \
                                0x04        // I2C slave 2 delay enable
#define MPU6050_I2C_MST_DELAY_CTRL_I2C_SLV1_DLY_EN                            \
                                0x02        // I2C slave 1 delay enable
#define MPU6050_I2C_MST_DELAY_CTRL_I2C_SLV0_DLY_EN                            \
                                0x01        // I2C slave 0 delay enable

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_SIGNAL_PATH_RESET register.
//
//*****************************************************************************
#define MPU6050_SIGNAL_PATH_RESET_GYRO                                        \
                                0x04        // Reset gyro
#define MPU6050_SIGNAL_PATH_RESET_ACCEL                                       \
                                0x02        // Reset accelerometer
#define MPU6050_SIGNAL_PATH_RESET_TEMP                                        \
                                0x01        // Reset temperature sensor

//*****************************************************************************
//
// The following are defines for the bit fields in the
// MPU6050_O_MOT_DETECT_CTRL register.
//
//*****************************************************************************
#define MPU6050_MOT_DETECT_CTRL_ACCEL_ON_DELAY_M                              \
                                0x30        // Accelerometer wake-up delay
#define MPU6050_MOT_DETECT_CTRL_ACCEL_ON_DELAY_4MS                            \
                                0x00        // Delay 4 ms
#define MPU6050_MOT_DETECT_CTRL_ACCEL_ON_DELAY_5MS                            \
                                0x10        // Delay 5 ms
#define MPU6050_MOT_DETECT_CTRL_ACCEL_ON_DELAY_6MS                            \
                                0x20        // Delay 6 ms
#define MPU6050_MOT_DETECT_CTRL_ACCEL_ON_DELAY_7MS                            \
                                0x30        // Delay 7 ms
#define MPU6050_MOT_DETECT_CTRL_ACCEL_ON_DELAY_S                              \
                                4

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_USER_CTRL
// register.
//
//*****************************************************************************
#define MPU6050_USER_CTRL_FIFO_EN                                             \
                                0x40        // FIFO enable
#define MPU6050_USER_CTRL_I2C_MST_EN                                          \
                                0x20        // I2C master mode enable
#define MPU6050_USER_CTRL_I2C_IF_DIS                                          \
                                0x10        // Write as zero
#define MPU6050_USER_CTRL_FIFO_RESET                                          \
                                0x04        // Reset FIFO buffer
#define MPU6050_USER_CTRL_I2C_MST_RESET                                       \
                                0x02        // Reset I2C master
#define MPU6050_USER_CTRL_SIG_COND_RESET                                      \
                                0x01        // Reset all sensors

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_PWR_MGMT_1
// register.
//
//*****************************************************************************
#define MPU6050_PWR_MGMT_1_DEVICE_RESET                                       \
                                0x80        // Device reset
#define MPU6050_PWR_MGMT_1_SLEEP                                              \
                                0x40        // Enter sleep mode
#define MPU6050_PWR_MGMT_1_CYCLE                                              \
                                0x20        // Enable automatic sleep
#define MPU6050_PWR_MGMT_1_TEMP_DIS                                           \
                                0x08        // Disable temperature sensor
#define MPU6050_PWR_MGMT_1_CLKSEL_M                                           \
                                0x07        // Clock source select
#define MPU6050_PWR_MGMT_1_CLKSEL_INT                                         \
                                0x00        // Internal 8 MHz oscillator
#define MPU6050_PWR_MGMT_1_CLKSEL_XG                                          \
                                0x01        // PLL with X-axis gyro reference
#define MPU6050_PWR_MGMT_1_CLKSEL_YG                                          \
                                0x02        // PLL with Y-axis gyro reference
#define MPU6050_PWR_MGMT_1_CLKSEL_ZG                                          \
                                0x03        // PLL with Z-axis gyro reference
#define MPU6050_PWR_MGMT_1_CLKSEL_EXT32K                                      \
                                0x04        // PLL with external 32.768 kHz
                                            // reference
#define MPU6050_PWR_MGMT_1_CLKSEL_EXT19M                                      \
                                0x05        // PLL with external 19.2 MHz
                                            // reference
#define MPU6050_PWR_MGMT_1_CLKSEL_STOP                                        \
                                0x07        // Clock disable
#define MPU6050_PWR_MGMT_1_CLKSEL_S                                           \
                                0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_PWR_MGMT_2
// register.
//
//*****************************************************************************
#define MPU6050_PWR_MGMT_2_LP_WAKE_CTRL_M                                     \
                                0xC0        // Wake-up frequency
#define MPU6050_PWR_MGMT_2_LP_WAKE_CTRL_1_25                                  \
                                0x00        // Wake-up at 1.25 Hz
#define MPU6050_PWR_MGMT_2_LP_WAKE_CTRL_5                                     \
                                0x40        // Wake-up at 5 Hz
#define MPU6050_PWR_MGMT_2_LP_WAKE_CTRL_20                                    \
                                0x80        // Wake-up at 20 Hz
#define MPU6050_PWR_MGMT_2_LP_WAKE_CTRL_40                                    \
                                0xC0        // Wake-up at 40 Hz
#define MPU6050_PWR_MGMT_2_STBY_XA                                            \
                                0x20        // Put X-axis accelerometer into
                                            // standby mode
#define MPU6050_PWR_MGMT_2_STBY_YA                                            \
                                0x10        // Put Y-axis accelerometer into
                                            // standby mode
#define MPU6050_PWR_MGMT_2_STBY_ZA                                            \
                                0x08        // Put Z-axis accelerometer into
                                            // standby mode
#define MPU6050_PWR_MGMT_2_STBY_XG                                            \
                                0x04        // Put X-axis gyro into standby
                                            // mode
#define MPU6050_PWR_MGMT_2_STBY_YG                                            \
                                0x02        // Put Y-axis gyro into standby
                                            // mode
#define MPU6050_PWR_MGMT_2_STBY_ZG                                            \
                                0x01        // Put Z-axis gyro into standby
                                            // mode
#define MPU6050_PWR_MGMT_2_LP_WAKE_CTRL_S                                     \
                                6

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_FIFO_COUNTH
// register.
//
//*****************************************************************************
#define MPU6050_FIFO_COUNTH_M   0xFF        // FIFO count [15:8]
#define MPU6050_FIFO_COUNTH_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_FIFO_COUNTL
// register.
//
//*****************************************************************************
#define MPU6050_FIFO_COUNTL_M   0xFF        // FIFO count [7:0]
#define MPU6050_FIFO_COUNTL_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_FIFO_R_W
// register.
//
//*****************************************************************************
#define MPU6050_FIFO_R_W_M      0xFF        // FIFO data
#define MPU6050_FIFO_R_W_S      0

//*****************************************************************************
//
// The following are defines for the bit fields in the MPU6050_O_WHO_AM_I
// register.
//
//*****************************************************************************
#define MPU6050_WHO_AM_I_M      0x7E        // I2C address
#define MPU6050_WHO_AM_I_MPU6050                                              \
                                0x68        // MPU6050
#define MPU6050_WHO_AM_I_S      1

#endif // __SENSORLIB_HW_MPU6050_H__
