CAN Example

This example application utilizes CAN to send characters back and forth
between two boards. It uses the UART to read / write the characters to
the UART terminal. It also uses the graphical display on the board to show
the last character transmited / received. Error handling is also included.

CAN HARDWARE SETUP:

To use this example you will need to hook up two DK-TM4C123G boards
together in a CAN network. This involves hooking the CANH screw terminals
together and the CANL terminals together. In addition 120ohm termination
resistors will need to be added to the edges of the network between CANH
and CANL.  In the two board setup this means hooking a 120 ohm resistor
between CANH and CANL on both boards.

See diagram below for visual. '---' represents wire.

      CANH--+--------------------------+--CANH
            |                          |
           .-.                        .-.
           | |120ohm                  | |120ohm
           | |                        | |
           '-'                        '-'
            |                          |
      CANL--+--------------------------+--CANL

SOFTWARE SETUP:

Once the hardware connections are setup connect both boards to the computer
via the In-Circuit Debug Interface USB port next to the graphical display.
Attach a UART terminal to each board configured 115,200 baud, 8-n-1 mode.

Anything you type into one terminal will show up in the other terminal and
vice versa. The last character sent / received will also be displayed on
the graphical display on the board.

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

This is part of revision 2.1.0.12573 of the DK-TM4C123G Firmware Package.
