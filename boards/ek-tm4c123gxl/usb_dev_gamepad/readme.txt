USB HID Gamepad Device

This example application turns the evaluation board into USB game pad
device using the Human Interface Device gamepad class.  The buttons on the
board are reported as buttons 1 and 2.  The X, Y, and Z coordinates are
reported using the ADC input on GPIO port E pins 1, 2, and 3.  The X input
is on PE3, the Y input is on PE2 and the Z input is on PE1.  These are
not connected to any real input so the values simply read whatever is on
the pins.  To get valid values the pins should have voltage that range
from VDDA(3V) to 0V.  The blue LED on PF5 is used to indicate gamepad
activity to the host and blinks when there is USB bus activity.

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

This is part of revision 2.1.0.12573 of the EK-TM4C123GXL Firmware Package.
