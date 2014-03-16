Internet of Things Quickstart


This application records various information about user activity on the
board, and periodically reports it to a cloud server managed by Exosite. In
order to use all of the features of this application, you will need to have
an account with Exosite, and make sure that the device you are using is
registered to your Exosite profile with its original MAC address from the
factory.

If you do not yet have an Exosite account, you can create one at
http://ti.exosite.com. The web interface there will help guide you through
the account creation process. There is also information in the Quickstart
document that is shipped along with the EK-TM4C1294XL evaluation kit.

This application uses a command-line based interface through a virtual COM
port on UART 0, with the settings 115,200-8-N-1. This application also
requires a wired Ethernet connection with internet access to perform
cloud-connected activities.

Once the application is running you should be able to see program output
over the virtual COM port, and interact with the command-line. The command
line will allow you to see the information being sent to and from Exosite's
servers, change the state of LEDs, and play a game of tic-tac-toe. If you
have internet connectivity issues, need to find your MAC address, or need
to re-activate your EK-TM4C1294XL board with Exosite, the command line
interface also has options to support these operations. Type
'help' at the command prompt to see a list of available commands.

If your local internet connection requires the use of a proxy server, you
will need to enter a command over the virtual COM port terminal before the
device will be able to connect to Exosite. When prompted by the
application, type 'setproxy help' for information on how to configure the
proxy.  Alternatively, you may uncomment the define statements below for
"CUSTOM_PROXY" settings, fill in the correct information for your local
http proxy server, and recompile this example. This will permanently set
your proxy as the default connection point.


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
