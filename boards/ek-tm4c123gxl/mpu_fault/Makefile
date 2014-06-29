#******************************************************************************
#
# Makefile - Rules for building the MPU example.
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
# This is part of revision 2.1.0.12573 of the EK-TM4C123GXL Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=TM4C123GH6PM

#
# The base directory for TivaWare.
#
ROOT=../../../..

#
# Include the common make definitions.
#
include ${ROOT}/makedefs

#
# Where to find source files that do not live in this directory.
#
VPATH=../../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=../../../..
IPATH+=../../../..

#
# The default rule, which causes the MPU example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/mpu_fault.axf

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
# Rules for building the MPU example.
#
${COMPILER}/mpu_fault.axf: ${COMPILER}/mpu_fault.o
${COMPILER}/mpu_fault.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/mpu_fault.axf: ${COMPILER}/uartstdio.o
${COMPILER}/mpu_fault.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/mpu_fault.axf: mpu_fault.ld
SCATTERgcc_mpu_fault=mpu_fault.ld
ENTRY_mpu_fault=ResetISR
CFLAGSgcc=-DTARGET_IS_TM4C123_RB1

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
