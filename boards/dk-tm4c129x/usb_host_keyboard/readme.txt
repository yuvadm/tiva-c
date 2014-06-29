USB Host keyboard example(usb_host_keyboard)</h1>

This example application demonstrates how to support a USB keyboard using
the DK-TM4C129X development board.  This application supports only a
standard keyboard HID device, but can report on the types of other devices
that are connected without having the ability to access them.  Key presses
are shown on the display as well as the caps-lock, scroll-lock, and
num-lock states of the keyboard.  The bottom left status bar reports the
type of device attached.  The user interface for the application is
handled in the keyboard_ui.c file while the usb_host_keyboard.c file
handles start up and the USB interface.

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
