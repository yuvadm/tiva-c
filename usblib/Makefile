#******************************************************************************
#
# Makefile - Rules for building the USB library.
#
# Copyright (c) 2008-2014 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 2.1.0.12573 of the Tiva USB Library.
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
# Where to find source files that do not live in this directory.
#
VPATH=./device
VPATH+=./host

#
# Where to find header files that do not live in the source directory.
#
IPATH=..

#
# The default rule, which causes the USB library to be built.
#
all: ${COMPILER}
all: ${COMPILER}/libusb.a

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
# Rules for building the USB library.
#
${COMPILER}/libusb.a: ${COMPILER}/usbbuffer.o
${COMPILER}/libusb.a: ${COMPILER}/usbdaudio.o
${COMPILER}/libusb.a: ${COMPILER}/usbdbulk.o
${COMPILER}/libusb.a: ${COMPILER}/usbdcdc.o
${COMPILER}/libusb.a: ${COMPILER}/usbdcdesc.o
${COMPILER}/libusb.a: ${COMPILER}/usbdcomp.o
${COMPILER}/libusb.a: ${COMPILER}/usbdconfig.o
${COMPILER}/libusb.a: ${COMPILER}/usbddfu-rt.o
${COMPILER}/libusb.a: ${COMPILER}/usbdenum.o
${COMPILER}/libusb.a: ${COMPILER}/usbdesc.o
${COMPILER}/libusb.a: ${COMPILER}/usbdhandler.o
${COMPILER}/libusb.a: ${COMPILER}/usbdhid.o
${COMPILER}/libusb.a: ${COMPILER}/usbdhidgamepad.o
${COMPILER}/libusb.a: ${COMPILER}/usbdhidkeyb.o
${COMPILER}/libusb.a: ${COMPILER}/usbdhidmouse.o
${COMPILER}/libusb.a: ${COMPILER}/usbdma.o
${COMPILER}/libusb.a: ${COMPILER}/usbdmsc.o
${COMPILER}/libusb.a: ${COMPILER}/usbhaudio.o
${COMPILER}/libusb.a: ${COMPILER}/usbhhid.o
${COMPILER}/libusb.a: ${COMPILER}/usbhhidkeyboard.o
${COMPILER}/libusb.a: ${COMPILER}/usbhhidmouse.o
${COMPILER}/libusb.a: ${COMPILER}/usbhhub.o
${COMPILER}/libusb.a: ${COMPILER}/usbhmsc.o
${COMPILER}/libusb.a: ${COMPILER}/usbhostenum.o
${COMPILER}/libusb.a: ${COMPILER}/usbhscsi.o
${COMPILER}/libusb.a: ${COMPILER}/usbkeyboardmap.o
${COMPILER}/libusb.a: ${COMPILER}/usbmode.o
${COMPILER}/libusb.a: ${COMPILER}/usbringbuf.o
${COMPILER}/libusb.a: ${COMPILER}/usbtick.o
${COMPILER}/libusb.a: ${COMPILER}/usbulpi.o

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
