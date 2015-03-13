#******************************************************************************
#
# Makefile - Rules for building the sensor library.
#
# Copyright (c) 2012-2014 Texas Instruments Incorporated.  All rights reserved.
# Software License Agreement
# 
# Texas Instruments (TI) is supplying this software for use solely and
# exclusively on TI's microcontroller products. The software is owned by
# TI and/or its suppliers, and is protected under applicable copyright
# laws. You may not combine this software with "viral" open-source
# software in order to form a larger program.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
# NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
# NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
# CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
# DAMAGES, FOR ANY REASON WHATSOEVER.
# 
# This is part of revision 2.1.0.12573 of the Tiva Firmware Development Package.
#
#******************************************************************************

#
# The base directory for TivaWare.
#
ROOT=..

#
# Include the common make definitions.
#
include ${ROOT}/makedefs

#
# Where to find header files that do not live in the source directory.
#
IPATH=..

#
# The default rule, which causes the sensor library to be built.
#
all: ${COMPILER}
all: ${COMPILER}/libsensor.a

#
# The rule to clean out all the build products.
#
clean:
	@rm -rf ${COMPILER} ${wildcard *~}

#
# The rule to create the target directory.
#
${COMPILER}:
	@mkdir -p ${COMPILER}

#
# Rules for building the sensor library.
#
${COMPILER}/libsensor.a: ${COMPILER}/ak8963.o
${COMPILER}/libsensor.a: ${COMPILER}/ak8975.o
${COMPILER}/libsensor.a: ${COMPILER}/bmp180.o
${COMPILER}/libsensor.a: ${COMPILER}/bq27510g3.o
${COMPILER}/libsensor.a: ${COMPILER}/cm3218.o
${COMPILER}/libsensor.a: ${COMPILER}/comp_dcm.o
${COMPILER}/libsensor.a: ${COMPILER}/i2cm_drv.o
${COMPILER}/libsensor.a: ${COMPILER}/isl29023.o
${COMPILER}/libsensor.a: ${COMPILER}/kxti9.o
${COMPILER}/libsensor.a: ${COMPILER}/l3gd20h.o
${COMPILER}/libsensor.a: ${COMPILER}/lsm303d.o
${COMPILER}/libsensor.a: ${COMPILER}/lsm303dlhc_accel.o
${COMPILER}/libsensor.a: ${COMPILER}/lsm303dlhc_mag.o
${COMPILER}/libsensor.a: ${COMPILER}/magneto.o
${COMPILER}/libsensor.a: ${COMPILER}/mpu6050.o
${COMPILER}/libsensor.a: ${COMPILER}/mpu9150.o
${COMPILER}/libsensor.a: ${COMPILER}/quaternion.o
${COMPILER}/libsensor.a: ${COMPILER}/sht21.o
${COMPILER}/libsensor.a: ${COMPILER}/tmp006.o
${COMPILER}/libsensor.a: ${COMPILER}/tmp100.o
${COMPILER}/libsensor.a: ${COMPILER}/vector.o

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
