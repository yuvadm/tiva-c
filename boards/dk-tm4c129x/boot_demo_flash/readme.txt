Flash-based Boot Loader Demo

An example to demonstrate the use of a flash-based boot loader.  At
startup, the application will configure the UART, USB and Ethernet
peripherals, and then branch to the boot loader to await the start of an
update.

This application is intended for use with flash-based ethernet boot
loader (boot_emac_flash).  Although it isn't neccessary to configure USB
and UART interface in this example since only ethernet flash-based boot
loader is provided, the code is there for completeness so that USB or
UART flash-based boot loader can be implemented if needed.
The link address for this application is set to 0x4000, the link address
has to be multiple of the flash erase block size(16KB=0x4000).
You may change this address to a 16KB boundary higher than the last
address occupied by the boot loader binary as long as you also rebuild the
boot loader itself after modifying its bl_config.h file to set
APP_START_ADDRESS to the same value.

The boot_demo_emac_flash application can be used along with this
application to easily demonstrate that the boot loader is actually updating
the on-chip flash.


-------------------------------------------------------------------------------

Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
Software License Agreement

Texas Instruments (TI) is supplying this software for use solely and
exclusively on TI's microcontroller products. The software is owned by
TI and/or its suppliers, and is protected under applicable copyright
laws. You may not combine this software with "viral" open-source
software in order to form a larger program.

THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
DAMAGES, FOR ANY REASON WHATSOEVER.

This is part of revision 2.1.0.12573 of the DK-TM4C129X Firmware Package.
