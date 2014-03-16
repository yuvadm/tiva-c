USB Composite Serial Device

This example application turns the evaluation kit into a multiple virtual
serial ports when connected to the USB host system.  The application
supports the USB Communication Device Class, Abstract Control Model to
redirect UART0 traffic to and from the USB host system.  For this example,
the evaluation kit will enumerate as a composite device with two virtual
serial ports. Including the physical UART0 connection with the ICDI, this
means that three independent virtual serial ports will be visible to the
USB host.

The first virtual serial port will echo data to the physical UART0 port on
the device which is connected to the virtual serial port on the ICDI device
on this board. The physical UART0 will also echo onto the first virtual
serial device provided by the Stellaris controller.

The second Stellaris virtual serial port will provide a console that can
echo data to both the ICDI virtual serial port and the first Stellaris
virtual serial port.  It will also allow turning on, off or toggling the
boards led status.  Typing a "?" and pressing return should echo a list of
commands to the terminal, since this board can show up as possibly three
individual virtual serial devices.

Assuming you installed TivaWare in the default directory, a driver
information (INF) file for use with Windows XP, Windows Vista and Windows7
can be found in C:/TivaWare_C_Series-x.x/windows_drivers. For Windows 2000,
the required INF file is in C:/TivaWare_C_Series-x.x/windows_drivers/win2K.

-------------------------------------------------------------------------------

Copyright (c) 2010-2014 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
