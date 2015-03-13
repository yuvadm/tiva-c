#******************************************************************************
#
# Makefile - Rules for building the graphics library.
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
# This is part of revision 2.1.0.12573 of the Tiva Graphics Library.
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
VPATH=fonts

#
# Where to find header files that do not live in the source directory.
#
IPATH=..

#
# The default rule, which causes the graphics library to be built.
#
all: ${COMPILER}
all: ${COMPILER}/libgr.a

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
# Rules for building the graphics library.
#
${COMPILER}/libgr.a: ${COMPILER}/canvas.o
${COMPILER}/libgr.a: ${COMPILER}/charmap.o
${COMPILER}/libgr.a: ${COMPILER}/checkbox.o
${COMPILER}/libgr.a: ${COMPILER}/circle.o
${COMPILER}/libgr.a: ${COMPILER}/container.o
${COMPILER}/libgr.a: ${COMPILER}/context.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm12.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm12b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm12i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm14.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm14b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm14i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm16.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm16b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm16i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm18.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm18b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm18i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm20.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm20b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm20i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm22.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm22b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm22i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm24.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm24b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm24i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm26.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm26b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm26i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm28.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm28b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm28i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm30.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm30b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm30i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm32.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm32b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm32i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm34.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm34b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm34i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm36.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm36b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm36i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm38.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm38b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm38i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm40.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm40b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm40i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm42.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm42b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm42i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm44.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm44b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm44i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm46.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm46b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm46i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm48.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm48b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcm48i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc12.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc14.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc16.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc18.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc20.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc22.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc24.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc26.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc28.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc30.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc32.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc34.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc36.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc38.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc40.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc42.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc44.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc46.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmsc48.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss12.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss12b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss12i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss14.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss14b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss14i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss16.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss16b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss16i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss18.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss18b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss18i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss20.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss20b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss20i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss22.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss22b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss22i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss24.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss24b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss24i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss26.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss26b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss26i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss28.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss28b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss28i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss30.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss30b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss30i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss32.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss32b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss32i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss34.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss34b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss34i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss36.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss36b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss36i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss38.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss38b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss38i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss40.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss40b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss40i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss42.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss42b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss42i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss44.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss44b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss44i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss46.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss46b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss46i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss48.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss48b.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmss48i.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt12.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt14.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt16.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt18.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt20.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt22.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt24.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt26.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt28.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt30.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt32.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt34.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt36.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt38.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt40.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt42.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt44.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt46.o
${COMPILER}/libgr.a: ${COMPILER}/fontcmtt48.o
${COMPILER}/libgr.a: ${COMPILER}/fontfixed6x8.o
${COMPILER}/libgr.a: ${COMPILER}/image.o
${COMPILER}/libgr.a: ${COMPILER}/imgbutton.o
${COMPILER}/libgr.a: ${COMPILER}/keyboard.o
${COMPILER}/libgr.a: ${COMPILER}/line.o
${COMPILER}/libgr.a: ${COMPILER}/listbox.o
${COMPILER}/libgr.a: ${COMPILER}/offscr1bpp.o
${COMPILER}/libgr.a: ${COMPILER}/offscr4bpp.o
${COMPILER}/libgr.a: ${COMPILER}/offscr8bpp.o
${COMPILER}/libgr.a: ${COMPILER}/pushbutton.o
${COMPILER}/libgr.a: ${COMPILER}/radiobutton.o
${COMPILER}/libgr.a: ${COMPILER}/rectangle.o
${COMPILER}/libgr.a: ${COMPILER}/slider.o
${COMPILER}/libgr.a: ${COMPILER}/string.o
${COMPILER}/libgr.a: ${COMPILER}/widget.o

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
