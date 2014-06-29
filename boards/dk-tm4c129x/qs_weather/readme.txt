Quickstart Weather Application

This example application demonstrates the operation of the Tiva C series
evaluation kit as a weather reporting application.

The application supports updating weather information from Open Weather Map
weather provider(http://openweathermap.org/).  The application uses the
lwIP stack to obtain an address through DNS, resolve the address of the
Open Weather Map site and then build and handle all of the requests
necessary to access the weather information.  The application can also use
a web proxy, allows for a custom city to be added to the list of cities and
toggles temperature units from Celsius to Fahrenheit.  The application uses
gestures to navigate between various screens.  To open the settings screen
just press and drag down on any city screen.  To exit the setting screen
press and drag up and you are returned to the main city display.  To
navigate between cities, press and drag left or right and the new city
information is displayed.

For additional details about Open Weather Map refer to their web page at:
http://openweathermap.org/

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
