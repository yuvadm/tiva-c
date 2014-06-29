USB Serial Device

This example application turns the development kit into a virtual serial
port when connected to the USB host system.  The application supports the
USB Communication Device Class, Abstract Control Model to redirect UART0
traffic to and from the USB host system.

The application can be recompiled to run using and external USB phy to
implement a high speed device using an external USB phy.  To use the
external phy the application must be built with \b USE_ULPI defined.  This
disables the internal phy and the connector on the DK-TM4C129X board and
enables the connections to the external ULPI phy pins on the DK-TM4C129X
board.

Assuming you installed TivaWare in the default directory, a
driver information (INF) file for use with Windows XP, Windows Vista and
Windows7 can be found in C:/ti/TivaWare-for-C-Series/windows_drivers.
For Windows 2000, the required INF file is in
C:/ti/TivaWare-for-C-Series/windows_drivers/win2K.

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
