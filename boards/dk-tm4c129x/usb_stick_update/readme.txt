USB Memory Stick Updater

This example application behaves the same way as a boot loader.  It resides
at the beginning of flash, and will read a binary file from a USB memory
stick and program it into another location in flash.  Once the user
application has been programmed into flash, this program will always start
the user application until requested to load a new application.

When this application starts, if there is a user application already in
flash (at \b APP_START_ADDRESS), then it will just run the user application.
It will attempt to load a new application from a USB memory stick under
the following conditions:

- no user application is present at \b APP_START_ADDRESS
- the user application has requested an update by transferring control
to the updater
- the user holds down the eval board push button when the board is reset

When this application is attempting to perform an update, it will wait
forever for a USB memory stick to be plugged in.  Once a USB memory stick
is found, it will search the root directory for a specific file name, which
is \e FIRMWARE.BIN by default.  This file must be a binary image of the
program you want to load (the .bin file), linked to run from the correct
address, at \b APP_START_ADDRESS.

The USB memory stick must be formatted as a FAT16 or FAT32 file system
(the normal case), and the binary file must be located in the root
directory.  Other files can exist on the memory stick but they will be
ignored.

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
