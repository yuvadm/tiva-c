Ethernet with uIP

This example application demonstrates the operation of the Tiva C Series
Ethernet controller using the uIP TCP/IP Stack.  DHCP is used to obtain
an Ethernet address.  A basic web site is served over the Ethernet port.
The web site displays a few lines of text, and a counter that increments
each time the page is sent.

UART0, connected to the ICDI virtual COM port and running at 115,200,
8-N-1, is used to display messages from this application.

For additional details on uIP, refer to the uIP web page at:
http://www.sics.se/~adam/uip/

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
