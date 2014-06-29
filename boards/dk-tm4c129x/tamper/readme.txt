Tamper

An example to demonstrate the use of tamper function in Hibernate module.
The user can ground any of these four GPIO pins(PM4, PM5, PM6, PM7 on
J28 and J30 headers on the development kit) to manually
triggle tamper events(s). The red indicators on the top of display should
reflect on which pin has triggered a tamper event.
The event along with the time stamp will be printed on the display.
The user can put the system in hibernation by pressing the HIB button.
The system should wake when the user either press RESET button,
or ground any of the four pins to trigger tamper event(s). When the
system boots up, the display should show whether the system woke
from hibernation or booted up from POR in which case a description of
howto instruction is printed on the display.
The RTC clock is displayed on the bottom of the display, the clock starts
from August 1st, 2013 at midnight when the app starts. The date and time
can be changed by pressing the CLOCK button. The clock is update every
second using hibernate calendar match interrupt. When the system
is in hibernation, the clock update on the display is paused, it resumes
once the system wakes up from hibernation.

WARNING: XOSC failure is implemented in this example code, care must be
taken to ensure that the XOSCn pin(Y3) is properly grounded in order to
safely generate the external oscillator failure without damaging the
external oscillator. XOSCFAIL can be triggered as a tamper event,
as well as wakeup event from hibernation.

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
