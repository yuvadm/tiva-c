Sleep Modes(sleep_modes)</h1>

This example demonstrates the different power modes available on the Tiva
C Series devices. The user button (USR-SW1) is used to cycle through the
different power modes.
The SRAM, Flash, and LDO are all configured to a lower power setting for
the different modes.

A timer is configured to toggle an LED in an ISR in both Run and Sleep
mode.
In Deep-Sleep the PWM is used to toggle the same LED in hardware. The three
remaining LEDs are used to indicate the current power mode.

        LED key in addition to the toggling LED:
            3 LEDs on - Run Mode
            2 LEDs on - Sleep Mode
            1 LED on - Deep-Sleep Mode

UART0, connected to the Virtual Serial Port and running at 115,200, 8-N-1,
is used to display messages from this application.

-------------------------------------------------------------------------------

Copyright (c) 2014 Texas Instruments Incorporated.  All rights reserved.
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
