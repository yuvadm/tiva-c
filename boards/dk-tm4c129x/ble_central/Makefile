#******************************************************************************
#
# Makefile - Rules for building the ble central example.
#
# Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 2.1.0.12573 of the DK-TM4C129X Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=TM4C129XNCZAD

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
VPATH=../drivers
VPATH+=../../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=..
IPATH+=../../../..

#
# The default rule, which causes the ble central example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/ble_central.axf

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
# Rules for building the ble central example.
#
${COMPILER}/ble_central.axf: ${COMPILER}/ble_central.o
${COMPILER}/ble_central.axf: ${COMPILER}/frame.o
${COMPILER}/ble_central.axf: ${COMPILER}/gap.o
${COMPILER}/ble_central.axf: ${COMPILER}/kentec320x240x16_ssd2119.o
${COMPILER}/ble_central.axf: ${COMPILER}/pinout.o
${COMPILER}/ble_central.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/ble_central.axf: ${COMPILER}/touch.o
${COMPILER}/ble_central.axf: ${COMPILER}/uartstdio.o
${COMPILER}/ble_central.axf: ${COMPILER}/ustdlib.o
${COMPILER}/ble_central.axf: ${ROOT}/grlib/${COMPILER}/libgr.a
${COMPILER}/ble_central.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/ble_central.axf: ble_central.ld
SCATTERgcc_ble_central=ble_central.ld
ENTRY_ble_central=ResetISR
CFLAGSgcc=-DTARGET_IS_TM4C129_RA0

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
