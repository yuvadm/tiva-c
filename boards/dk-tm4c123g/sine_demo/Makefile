#******************************************************************************
#
# Makefile - Rules for building the sine_demo example.
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
# This is part of revision 2.1.0.12573 of the DK-TM4C123G Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=TM4C123GH6PGE

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

#
# Where to find header files that do not live in the source directory.
#
IPATH=..
IPATH+=../../../..

#
# The default rule, which causes the sine_demo example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/sine_demo.axf

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
# Rules for building the sine_demo example.
#
${COMPILER}/sine_demo.axf: ${COMPILER}/cfal96x64x16.o
${COMPILER}/sine_demo.axf: ${COMPILER}/sine_demo.o
${COMPILER}/sine_demo.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/sine_demo.axf: ${COMPILER}/stripchartwidget.o
${COMPILER}/sine_demo.axf: ${ROOT}/grlib/${COMPILER}/libgr.a
${COMPILER}/sine_demo.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/sine_demo.axf: sine_demo.ld
SCATTERgcc_sine_demo=sine_demo.ld
ENTRY_sine_demo=ResetISR
CFLAGSgcc=-DTARGET_IS_TM4C123_RB1

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
