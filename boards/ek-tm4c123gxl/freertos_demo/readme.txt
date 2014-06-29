FreeRTOS Example

This application demonstrates the use of FreeRTOS on Launchpad.

The application blinks the user-selected LED at a user-selected frequency.
To select the LED press the left button and to select the frequency press
the right button.  The UART outputs the application status at 115,200 baud,
8-n-1 mode.

This application utilizes FreeRTOS to perform the tasks in a concurrent
fashion.  The following tasks are created:

- An LED task, which blinks the user-selected on-board LED at a
  user-selected rate (changed via the buttons).

- A Switch task, which monitors the buttons pressed and passes the
  information to LED task.

In addition to the tasks, this application also uses the following FreeRTOS
resources:

- A Queue to enable information transfer between tasks.

- A Semaphore to guard the resource, UART, from access by multiple tasks at
  the same time.

- A non-blocking FreeRTOS Delay to put the tasks in blocked state when they
  have nothing to do.

For additional details on FreeRTOS, refer to the FreeRTOS web page at:
http://www.freertos.org/

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
