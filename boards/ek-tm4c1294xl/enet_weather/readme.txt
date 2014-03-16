Ethernet Weather Application

This example application demonstrates the operation of the Tiva C series
evaluation kit as a weather reporting application.

The application supports updating weather information from Open Weather Map
weather provider(http://openweathermap.org/).  The application uses the
lwIP stack to obtain an address through DNS, resolve the address of the
Open Weather Map site and then build and handle all of the requests
necessary to access the weather information.  The application can also use
a web proxy, allows for a custom city to be added to the list of cities and
toggles temperature units from Celsius to Fahrenheit.  The application
scrolls through the city list at a fixed interval when there is a valid
IP address and displays this information over the UART.

The application will print the city name, status, humidity, current temp
and the high/low. In addition the application will display what city it
is currently updating. Once the app has scrolled through the cities a
a defined amount of times, it will attempt to update the information.

UART0, connected to the Virtual Serial Port and running at 115,200, 8-N-1,
is used to display messages from this application.

/ For additional details about Open Weather Map refer to their web page at:
http://openweathermap.org/

For additional details on lwIP, refer to the lwIP web page at:
http://savannah.nongnu.org/projects/lwip/

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
