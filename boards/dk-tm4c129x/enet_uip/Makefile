#******************************************************************************
#
# Makefile - Rules for building the Ethernet with uIP.
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
ROOT=../../..

#
# Include the common make definitions.
#
include ${ROOT}/makedefs

#
# Where to find source files that do not live in this directory.
#
VPATH=.
VPATH+=../../../third_party/uip-1.0/apps/dhcpc
VPATH+=../drivers
VPATH+=../../../third_party/uip-1.0/apps/httpd
VPATH+=../../../third_party/uip-1.0/uip
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=..
IPATH+=../../..
IPATH+=../../../third_party/uip-1.0/apps
IPATH+=../../../third_party/uip-1.0/uip
IPATH+=../../../third_party/uip-1.0
IPATH+=../../../utils

#
# The default rule, which causes the Ethernet with uIP to be built.
#
all: ${COMPILER}
all: ${COMPILER}/enet_uip.axf

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
# Rules for building the Ethernet with uIP.
#
${COMPILER}/enet_uip.axf: ${COMPILER}/dhcpc.o
${COMPILER}/enet_uip.axf: ${COMPILER}/enet_uip.o
${COMPILER}/enet_uip.axf: ${COMPILER}/frame.o
${COMPILER}/enet_uip.axf: ${COMPILER}/httpd.o
${COMPILER}/enet_uip.axf: ${COMPILER}/kentec320x240x16_ssd2119.o
${COMPILER}/enet_uip.axf: ${COMPILER}/pinout.o
${COMPILER}/enet_uip.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/enet_uip.axf: ${COMPILER}/uartstdio.o
${COMPILER}/enet_uip.axf: ${COMPILER}/uip.o
${COMPILER}/enet_uip.axf: ${COMPILER}/uip_arp.o
${COMPILER}/enet_uip.axf: ${COMPILER}/uip_timer.o
${COMPILER}/enet_uip.axf: ${COMPILER}/ustdlib.o
${COMPILER}/enet_uip.axf: ${ROOT}/grlib/${COMPILER}/libgr.a
${COMPILER}/enet_uip.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/enet_uip.axf: enet_uip.ld
SCATTERgcc_enet_uip=enet_uip.ld
ENTRY_enet_uip=ResetISR
CFLAGSgcc=-DTARGET_IS_TM4C129_RA0 -DDEBUG_OUTPUT -DUIP_OFFLOAD_ICMP_CHKSUM

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
