USB Mass Storage Class Host Example

This example application demonstrates reading a file system from
a USB flash disk.  It makes use of FatFs, a FAT file system driver.  It
provides a simple widget-based interface on the display for viewing and
navigating the file system on the flash disk.

For additional details about FatFs, see the following site:
http://elm-chan.org/fsw/ff/00index_e.html

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
