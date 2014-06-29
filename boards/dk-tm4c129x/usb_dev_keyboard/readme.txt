USB HID Keyboard Device

This example application turns the evaluation board into a USB keyboard
supporting the Human Interface Device class.  The color LCD display shows a
virtual keyboard and taps on the touchscreen will send appropriate key
usage codes back to the USB host. Modifier keys (Shift, Ctrl and Alt) are
``sticky'' and tapping them toggles their state. The board status LED is
used to indicate the current Caps Lock state and is updated in response to
pressing the ``Caps'' key on the virtual keyboard or any other keyboard
attached to the same USB host system.

The device implemented by this application also supports USB remote wakeup
allowing it to request the host to reactivate a suspended bus.  If the bus
is suspended (as indicated on the application display), touching the
display will request a remote wakeup assuming the host has not
specifically disabled such requests.

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
