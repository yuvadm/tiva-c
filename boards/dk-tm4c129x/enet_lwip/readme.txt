Ethernet with lwIP

This example application demonstrates the operation of the Tiva
Ethernet controller using the lwIP TCP/IP Stack configured to operate as
an HTTP file server (web server).  DHCP is used to obtain an Ethernet
address.  If DHCP times out without obtaining an address, AUTOIP will be
used to obtain a link-local address.  The address that is selected will be
shown on the QVGA display.

The file system code will first check to see if an SD card has been plugged
into the microSD slot.  If so, all file requests from the web server will
be directed to the SD card.  Otherwise, a default set of pages served up
by an internal file system will be used.  Source files for the internal
file system image can be found in the ``fs'' directory.  If any of these
files are changed, the file system image (enet_fsdata.h) should be
rebuilt using the command:

    ../../../../tools/bin/makefsfile -i fs -o enet_fsdata.h -r -h -q

For additional details on lwIP, refer to the lwIP web page at:
http://savannah.nongnu.org/projects/lwip/

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
