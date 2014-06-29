#******************************************************************************
#
# Makefile - Rules for building the USB composite device keyboard mouse example.
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
# The default rule, which causes the USB composite device keyboard mouse example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/usb_dev_chid.axf

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
# Rules for building the USB composite device keyboard mouse example.
#
${COMPILER}/usb_dev_chid.axf: ${COMPILER}/frame.o
${COMPILER}/usb_dev_chid.axf: ${COMPILER}/kentec320x240x16_ssd2119.o
${COMPILER}/usb_dev_chid.axf: ${COMPILER}/pinout.o
${COMPILER}/usb_dev_chid.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/usb_dev_chid.axf: ${COMPILER}/touch.o
${COMPILER}/usb_dev_chid.axf: ${COMPILER}/ui.o
${COMPILER}/usb_dev_chid.axf: ${COMPILER}/usb_dev_chid.o
${COMPILER}/usb_dev_chid.axf: ${COMPILER}/usb_keyboard.o
${COMPILER}/usb_dev_chid.axf: ${COMPILER}/usb_mouse.o
${COMPILER}/usb_dev_chid.axf: ${COMPILER}/usb_structs.o
${COMPILER}/usb_dev_chid.axf: ${ROOT}/usblib/${COMPILER}/libusb.a
${COMPILER}/usb_dev_chid.axf: ${ROOT}/grlib/${COMPILER}/libgr.a
${COMPILER}/usb_dev_chid.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/usb_dev_chid.axf: usb_dev_chid.ld
SCATTERgcc_usb_dev_chid=usb_dev_chid.ld
ENTRY_usb_dev_chid=ResetISR
CFLAGSgcc=-DTARGET_IS_TM4C129_RA0 -DUART_BUFFERED

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
