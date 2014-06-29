USB HID Keyboard Host

This example application demonstrates how to support a USB keyboard
attached to the evaluation kit board.  The display will show if a keyboard
is currently connected and the current state of the Caps Lock key on the
keyboard that is connected on the bottom status area of the screen.
Pressing any keys on the keyboard will cause them to be printed on the
screen and to be sent out the UART at 115200 baud with no parity, 8 bits
and 1 stop bit.  Any keyboard that supports the USB HID BIOS protocol
should work with this demo application.

-------------------------------------------------------------------------------

Copyright (c) 2012-2014 Texas Instruments Incorporated.  All rights reserved.
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
