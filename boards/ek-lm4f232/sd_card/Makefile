#******************************************************************************
#
# Makefile - Rules for building the SD card example.
#
# Copyright (c) 2011-2014 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 2.1.0.12573 of the EK-LM4F232 Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=TM4C123GH6PGE

#
# The base directory for TivaWare.
#
ROOT=../../..

#
# Include the common make definitions.
#
include ${ROOT}/makedefs

#
# Where to find source files that do not live in this directory.
#
VPATH=../drivers
VPATH+=../../../third_party/fatfs/src/option
VPATH+=../../../third_party/fatfs/port
VPATH+=../../../third_party/fatfs/src
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=..
IPATH+=../../..
IPATH+=../../../third_party

#
# The default rule, which causes the SD card example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/sd_card.axf

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
# Rules for building the SD card example.
#
${COMPILER}/sd_card.axf: ${COMPILER}/cc932.o
${COMPILER}/sd_card.axf: ${COMPILER}/cfal96x64x16.o
${COMPILER}/sd_card.axf: ${COMPILER}/cmdline.o
${COMPILER}/sd_card.axf: ${COMPILER}/ff.o
${COMPILER}/sd_card.axf: ${COMPILER}/mmc-ek-lm4f232.o
${COMPILER}/sd_card.axf: ${COMPILER}/sd_card.o
${COMPILER}/sd_card.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/sd_card.axf: ${COMPILER}/uartstdio.o
${COMPILER}/sd_card.axf: ${ROOT}/grlib/${COMPILER}/libgr.a
${COMPILER}/sd_card.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/sd_card.axf: sd_card.ld
SCATTERgcc_sd_card=sd_card.ld
ENTRY_sd_card=ResetISR
CFLAGSgcc=-DTARGET_IS_TM4C123_RA1 -DENABLE_LFN

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
