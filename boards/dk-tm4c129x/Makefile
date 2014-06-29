#******************************************************************************
#
# Makefile - Rules for building the dk-tm4c129x examples.
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
# A list of the directories containing the examples.
#
DIRS=aes_cbc_decrypt      \
     aes_cbc_encrypt      \
     aes_ccm_decrypt      \
     aes_ccm_encrypt      \
     aes_cmac             \
     aes_ecb_decrypt      \
     aes_ecb_encrypt      \
     aes_gcm_decrypt      \
     aes_gcm_encrypt      \
     bitband              \
     ble_btool            \
     ble_central          \
     blinky               \
     boot_demo_uart       \
     boot_demo_usb        \
     boot_demo_emac_flash \
     boot_demo_flash      \
     boot_emac_flash      \
     calibrate            \
     crc32                \
     enet_io              \
     enet_lwip            \
     enet_uip             \
     fontview             \
     freertos_demo        \
     gpio_jtag            \
     grlib_demo           \
     grlib_driver_test    \
     hello                \
     hello_widget         \
     hibernate            \
     interrupts           \
     lang_demo            \
     mpu_fault            \
     qs_weather           \
     scribble             \
     sd_card              \
     sha1_hash            \
     sha1_hmac            \
     synth                \
     tamper               \
     tdes_cbc_decrypt     \
     tdes_cbc_encrypt     \
     timers               \
     uart_echo            \
     udma_demo            \
     usb_dev_bulk         \
     usb_dev_chid         \
     usb_dev_gamepad      \
     usb_dev_keyboard     \
     usb_dev_msc          \
     usb_dev_serial       \
     usb_host_audio       \
     usb_host_audio_in    \
     usb_host_hub         \
     usb_host_keyboard    \
     usb_host_mouse       \
     usb_host_msc         \
     usb_otg_mouse        \
     usb_stick_demo       \
     usb_stick_update     \
     watchdog            

#
# The default rule, which causes the examples to be built.
#
all:
	@for i in ${DIRS};              \
	 do                             \
	     make -C $${i} || exit $$?; \
	 done

#
# The rule to clean out all the build products.
#
clean:
	@rm -rf ${wildcard *~} __dummy__
	@-for i in ${DIRS};        \
	  do                       \
	      make -C $${i} clean; \
	  done
