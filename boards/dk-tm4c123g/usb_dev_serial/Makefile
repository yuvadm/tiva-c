#******************************************************************************
#
# Makefile - Rules for building the USB device serial example.
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
VPATH+=../../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=..
IPATH+=../../../..

#
# The default rule, which causes the USB device serial example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/usb_dev_serial.axf

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
# Rules for building the USB device serial example.
#
${COMPILER}/usb_dev_serial.axf: ${COMPILER}/cfal96x64x16.o
${COMPILER}/usb_dev_serial.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/usb_dev_serial.axf: ${COMPILER}/uartstdio.o
${COMPILER}/usb_dev_serial.axf: ${COMPILER}/usb_dev_serial.o
${COMPILER}/usb_dev_serial.axf: ${COMPILER}/usb_serial_structs.o
${COMPILER}/usb_dev_serial.axf: ${COMPILER}/ustdlib.o
${COMPILER}/usb_dev_serial.axf: ${ROOT}/usblib/${COMPILER}/libusb.a
${COMPILER}/usb_dev_serial.axf: ${ROOT}/grlib/${COMPILER}/libgr.a
${COMPILER}/usb_dev_serial.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/usb_dev_serial.axf: usb_dev_serial.ld
SCATTERgcc_usb_dev_serial=usb_dev_serial.ld
ENTRY_usb_dev_serial=ResetISR
CFLAGSgcc=-DTARGET_IS_TM4C123_RB1 -DUART_BUFFERED

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
