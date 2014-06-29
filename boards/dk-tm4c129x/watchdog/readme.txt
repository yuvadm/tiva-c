Watchdog

This example application demonstrates the use of the watchdog as a simple
heartbeat for the system.  If a watchdog is not periodically fed, it will
reset the system.  The GREEN LED will blink once every second to show
every time watchdog 0 is being fed, the Amber LED will blink once every
second to indicate watchdog 1 is being fed. To stop the watchdog being fed
and, hence, cause a system reset, tap the left screen to starve the
watchdog 0, and right screen to starve the watchdog 1.

The counters on the screen show the number of interrupts that each watchdog
served; the count wraps at 255.  Since the two watchdogs run in different
clock domains, the counters will get out of sync over time.

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
