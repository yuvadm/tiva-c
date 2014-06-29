SD card using FAT file system

This example application demonstrates reading a file system from an SD
card.  It makes use of FatFs, a FAT file system driver.  It provides a
simple command console via a serial port for issuing commands to view and
navigate the file system on the SD card.

The first UART, which is connected to the USB debug virtual serial port on
the evaluation board, is configured for 115,200 bits per second, and 8-N-1
mode.  When the program is started a message will be printed to the
terminal.  Type ``help'' for command help.

For additional details about FatFs, see the following site:
http://elm-chan.org/fsw/ff/00index_e.html

-------------------------------------------------------------------------------

Copyright (c) 2011-2014 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 2.1.0.12573 of the EK-LM4F232 Firmware Package.
