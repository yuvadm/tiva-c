#******************************************************************************
#
# Makefile - Rules for building the Sample Ethernet Weather Application using lwIP 1.4.1.
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
# This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=TM4C1294NCPDT

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
VPATH+=../../../../third_party/lwip-1.4.1/apps/httpserver_raw
VPATH+=../../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=..
IPATH+=../../../..
IPATH+=../../../../third_party/lwip-1.4.1/apps
IPATH+=../../../../third_party/lwip-1.4.1/ports/tiva-tm4c129/include
IPATH+=../../../../third_party/lwip-1.4.1/src/include
IPATH+=../../../../third_party/lwip-1.4.1/src/include/ipv4
IPATH+=../../../../third_party

#
# The default rule, which causes the Sample Ethernet Weather Application using lwIP 1.4.1 to be built.
#
all: ${COMPILER}
all: ${COMPILER}/enet_weather.axf

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
# Rules for building the Sample Ethernet Weather Application using lwIP 1.4.1.
#
${COMPILER}/enet_weather.axf: ${COMPILER}/cmdline.o
${COMPILER}/enet_weather.axf: ${COMPILER}/commands.o
${COMPILER}/enet_weather.axf: ${COMPILER}/enet_weather.o
${COMPILER}/enet_weather.axf: ${COMPILER}/eth_client.o
${COMPILER}/enet_weather.axf: ${COMPILER}/flash_pb.o
${COMPILER}/enet_weather.axf: ${COMPILER}/http.o
${COMPILER}/enet_weather.axf: ${COMPILER}/images.o
${COMPILER}/enet_weather.axf: ${COMPILER}/json.o
${COMPILER}/enet_weather.axf: ${COMPILER}/locator.o
${COMPILER}/enet_weather.axf: ${COMPILER}/lwiplib.o
${COMPILER}/enet_weather.axf: ${COMPILER}/pinout.o
${COMPILER}/enet_weather.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/enet_weather.axf: ${COMPILER}/uartstdio.o
${COMPILER}/enet_weather.axf: ${COMPILER}/ustdlib.o
${COMPILER}/enet_weather.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/enet_weather.axf: enet_weather.ld
SCATTERgcc_enet_weather=enet_weather.ld
ENTRY_enet_weather=ResetISR
CFLAGSgcc=-DTARGET_IS_TM4C129_RA0 -DLWIP_1_4_1 -DUSE_HTTP_1_0 -DUART_BUFFERED

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
