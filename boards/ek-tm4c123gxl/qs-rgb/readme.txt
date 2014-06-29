EK-TM4C123GXL Quickstart Application

A demonstration of the Tiva C Series LaunchPad (EK-TM4C123GXL)
capabilities.

Press and/or hold the left button to traverse towards the red end of the
ROYGBIV color spectrum.  Press and/or hold the right button to traverse
toward the violet end of the ROYGBIV color spectrum.

If no input is received for 5 seconds, the application will start
automatically changing the color displayed.

Press and hold both left and right buttons for 3 seconds to enter
hibernation.  During hibernation, the last color displayed will blink
for 0.5 seconds every 3 seconds.

The system can also be controlled via a command line provided via the UART.
Configure your host terminal emulator for 115200, 8-N-1 to access this
feature.

- Command 'help' generates a list of commands and helpful information.
- Command 'hib' will place the device into hibernation mode.
- Command 'rand' will initiate the pseudo-random color sequence.
- Command 'intensity' followed by a number between 0 and 100 will set the
brightness of the LED as a percentage of maximum brightness.
- Command 'rgb' followed by a six character hex value will set the color.
For example 'rgb FF0000' will produce a red color.

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

This is part of revision 2.1.0.12573 of the EK-TM4C123GXL Firmware Package.
