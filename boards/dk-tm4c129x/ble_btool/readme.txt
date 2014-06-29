Bluetooth Low Energy ``BTool''

This application connects the ICDI virtual UART0 with UART3, so that
the BTool application running on PC can control the CC2540/CC2541 through
the TM4C129X.

The CC254X device should be programmed with HostTestRelease(Network
Processor) application.  A hex file containing this application can be
found in the BLE stack 1.4.0 release under
C:\\...\\BLE-CC254x-1.4.0\\Accessories\\HexFiles\\
CC254X_SmartRF_HostTestRelease_All.hex. Please refer to the Bluetooth Low
Energy CC254X Development Kit User's Guide for information on how to load
the hex file to the CC254X. This User's Guide can be found in
http://www.ti.com/lit/ug/swru301a/swru301a.pdf

On the TM4C129X development board, make sure that jumpers PJ0 and PJ1
are connected to the "EM_UART" side to ensure that the UART3 TX and RX
signals are routed to the EM header.  UART3 is used to communicate between
the TM4C129X and the CC2540 device.

BTool is a PC application that allows a user to form a connection between
two BLE devices. For more details on BTool, please refer to the above
User's Guide. The installer for BTool is included in the BLE stack 1.4.0
release under C:\\...\\BLE-CC254x-1.4.0\\Accessories\\BTool\\


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
