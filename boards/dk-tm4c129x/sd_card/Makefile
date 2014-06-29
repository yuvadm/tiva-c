#******************************************************************************
#
# Makefile - Rules for building the SD card example.
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
VPATH+=../../../../third_party/fatfs/src/option
VPATH+=../../../../third_party/fatfs/port
VPATH+=../../../../third_party/fatfs/src
VPATH+=../../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=..
IPATH+=../../../..
IPATH+=../../../../third_party

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
${COMPILER}/sd_card.axf: ${COMPILER}/cmdline.o
${COMPILER}/sd_card.axf: ${COMPILER}/ff.o
${COMPILER}/sd_card.axf: ${COMPILER}/frame.o
${COMPILER}/sd_card.axf: ${COMPILER}/kentec320x240x16_ssd2119.o
${COMPILER}/sd_card.axf: ${COMPILER}/mmc-dk-tm4c129x.o
${COMPILER}/sd_card.axf: ${COMPILER}/pinout.o
${COMPILER}/sd_card.axf: ${COMPILER}/sd_card.o
${COMPILER}/sd_card.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/sd_card.axf: ${COMPILER}/touch.o
${COMPILER}/sd_card.axf: ${COMPILER}/uartstdio.o
${COMPILER}/sd_card.axf: ${COMPILER}/ustdlib.o
${COMPILER}/sd_card.axf: ${ROOT}/grlib/${COMPILER}/libgr.a
${COMPILER}/sd_card.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/sd_card.axf: sd_card.ld
SCATTERgcc_sd_card=sd_card.ld
ENTRY_sd_card=ResetISR
CFLAGSgcc=-DTARGET_IS_TM4C129_RA0 -DENABLE_LFN -DUART_BUFFERED

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
