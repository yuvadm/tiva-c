USB HID Keyboard Device

This example application turns the evaluation board into a USB keyboard
supporting the Human Interface Device class.  When the push button is
pressed, a sequence of key presses is simulated to type a string.  Care
should be taken to ensure that the active window can safely receive the
text; enter is not pressed at any point so no actions are attempted by the
host if a terminal window is used (for example).  The status LED is used to
indicate the current Caps Lock state and is updated in response to any
other keyboard attached to the same USB host system.

The device implemented by this application also supports USB remote wakeup
allowing it to request the host to reactivate a suspended bus.  If the bus
is suspended (as indicated on the application display), pressing the
push button will request a remote wakeup assuming the host has not
specifically disabled such requests.


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
