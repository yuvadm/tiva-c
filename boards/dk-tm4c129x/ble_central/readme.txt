BLE Central Device Demonstration

This application provides a demonstration use of Bluetooth Low Energy
central device by utilizing TI’s BLE CC2540 Evaluation Module and
SesorTags.

By connecting a CC2540 EM board to the EM header on the TM4C129X development
board, the TM4C129X can communicate with the CC2540 by means of
vendor-specific HCI commands using the UART interface. This application can
discover up to three SensorTag devices, it can connect to any one of them,
perform pairing and bonding, read some sensor data and RSSI data from the
slave, and display the information on the LCD display.

This application can discover any BLE device although it may not be
able to make a connection to a device other than a SensorTag/CC2540 as the
bonding process will likely fail due to the use of a default passcode.
We have tested this application with SensorTag and CC2540 Development
boards (SmartRF05EB + CC2540 EM) with the SimpleBLEPeripheral
sample application programmed. It can successfully bond with both boards
since the same default passcode is expected. Both SensorTag and CC2540
devices have BLE Stack 1.4.0 release code programmed.

CC2540 device should be programmed with the HostTestRelease
(Network Processor) application. A hex file containing the this application
can be found in the BLE stack 1.4.0 release under
C:\\...\\BLE-CC254x-1.4.0\\Accessories\\HexFiles\\
CC2540_SmartRF_HostTestRelease_All.hex. Please refer to the Bluetooth Low
Energy CC2540 Development Kit User's Guide for information on how to load
the hex file to the CC2540. This User's Guide can be found in
http://www.ti.com/lit/ug/swru301a/swru301a.pdf

On the TM4C129X development board, make sure that jumpers PJ0 and PJ1
are connected to the "EM_UART" side which allows UART3 TX and RX signals to
be routed to the EM header. UART3 is used as the communication channel
between the CC2540 device and the TM4C129X device.

Once the application starts, it will verify the serial connection by
sending the CC2540 device a vendor-specific HCI command, and waiting for the
expected responses within a short time period. Once the physical connection
between TM4C129X and CC2540 is verified, the device will automatically
start to discover BLE peripheral device. If no devices are found within 20
seconds, the application will timeout and display "No Device Found",
otherwise the discovered device names will be shown on the display.
Touching any of the device names will start the process of establishing a
connection with that device.  This sample application always tries to make
a secure connection by pairing the device with default passcode "00000".
Upon successfully linking and pairing, application will start querying
sensor data, including IR temperature, ambient temperature, humidity and
RSSI.

Whe run inside, IR and ambient temperature should typically be in the low
20s (Celsius).  You can place the SensorTag near a hot object (such as a
cup of coffee) to verify that the IR temperature increases. You can move
the SensorTag further from the TM4C129X development board to verify that
its RSSI reading will decrease.

At any time after the connection is established, you can touch the
"disconnect" button on the bottom of the screen to terminate the
connection with the peripheral device.

In order to make the SensorTag discoverable by a central device, the
SensorTag needs to be in the discovery mode. The LED in the middle of the
board will blink periodically if the SensorTag is in discovery mode. If the
LED is not blinking, pressing the side button on the SensorTag should
place it in discovery mode.  Once it is connected to a central device,
the LED should be off, pressing the side button while it is connected will
terminate the connection and put the SensorTag in discovery mode again.
For more information on SensorTag, please visit
http://processors.wiki.ti.com/index.php/Bluetooth_SensorTag

Every HCI command and event are output to the UART console for
debugging purpose. The UART terminal should be configured in 115,200 baud,
8-n-1 mode.


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
