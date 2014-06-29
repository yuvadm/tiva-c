USB HUB Host example(usb_host_hub)</h1>

This example application demonstrates how to support a USB keyboard
and a USB mass storage with a USB Hub.  The application emulates a very
simple console with the USB keyboard used for input.  The application
requires that the mass storage device is also inserted or the console will
generate errors when accessing the file system.  The console supports the
following commands: "ls", "cat", "pwd", "cd" and "help".  The "ls" command
will provide a listing of the files in the current directory.  The "cat"
command can be used to print the contents of a file to the screen.  The
"pwd" command displays the current working directory.  The "cd" command
allows the application to move to a new directory.  The cd command is
simplified and only supports "cd .." but not directory changes like
"cd ../somedir".  The "help" command has other aliases that are displayed
when the "help" command is issued.

Any keyboard that supports the USB HID BIOS protocol should work with this
demo application.

The application can be recompiled to run using and external USB phy to
implement a high speed host using an external USB phy.  To use the external
phy the application must be built with \b USE_ULPI defined.  This disables
the internal phy and the connector on the DK-TM4C129X board and enables the
connections to the external ULPI phy pins on the DK-TM4C129X board.

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
