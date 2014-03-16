Hibernate Example

An example to demonstrate the use of the Hibernation module.  The user
can put the microcontroller in hibernation by typing 'hib' in the terminal
and pressing ENTER or by pressing USR_SW1 on the board.  The
microcontroller will then wake on its own after 5 seconds, or immediately
if the user presses the RESET button.  The External WAKE button, external
WAKE pins, and GPIO (PK6) wake sources can also be used to wake
immediately from hibernation.  The following wiring enables the use of
these pins as wake sources.
    WAKE on breadboard connection header (X11-95) to GND
    PK6 on BoosterPack 2 (X7-17) to GND
    PK6 on breadboard connection header (X11-63) to GND

The program keeps a count of the number of times it has entered
hibernation.  The value of the counter is stored in the battery-backed
memory of the Hibernation module so that it can be retrieved when the
microcontroller wakes.  The program displays the wall time and date by
making use of the calendar function of the Hibernate module.  User can
modify the date and time if so desired.

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

This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
