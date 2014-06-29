Data Logger

This example application is a data logger.  It can be configured to collect
data from up to 10 data sources.  The possible data sources are:
- 4 analog inputs, 0-20V
- 9-axis I2C fusion data (3axis gyro, 3axis accelorometer, 3axis compass)
- internal and external temperature sensors
- processor current consumption

The data logger provides a menu navigation that is operated by the buttons
on the board (up, down, left, right, select).  The data logger
can be configured by using the menus.  The following items can be
configured:
- data sources to be logged
- sample rate
- storage location
- sleep modes
- clock

Using the data logger:

Use the CONFIG menu to configure the data logger.  The following choices
are provided:

- CHANNELS - enable specific channels of data that will be logged
- PERIOD - select the sample period
- STORAGE - select where the collected data will be stored:
    - FLASH - stored in the internal flash memory
    - USB - stored on a connected USB memory stick
    - HOST PC - transmitted to a host PC via USB OTG virtual serial port
    - NONE - the data will only be displayed and not stored
- SLEEP - select whether or not the board sleeps between samples.  Sleep
mode is allowed when storing to flash at with a period of 1 second or
longer.
- CLOCK - allows setting of internal time-of-day clock that is used for
time stamping of the sampled data

Use the START menu to start the data logger running.  It will begin
collecting and storing the data.  It will continue to collect data until
stopped by pressing the left button or select button.

While the data logger is collecting data and it is not configured to
sleep, a simple strip chart showing the collected data will appear on the
display.  If the data logger is configured to sleep, then no strip chart
will be shown.

If the data logger is storing to internal flash memory, it will overwrite
the oldest data.  If storing to a USB memory device it will store data
until the device is full.

The VIEW menu allows viewing the values of the data sources in numerical
format.  When viewed this way the data is not stored.

The SAVE menu allows saving data that was stored in internal flash memory
to a USB stick.  The data will be saved in a text file in CSV format.

The ERASE menu is used to erase the internal memory so more data can be
saved.

When the board running qs-logger is connected to a host PC via
the USB OTG connection for the first time, Windows will prompt for a device
driver for the board.
This can be found in /ti/TivaWare_C_Series-x.x/windows_drivers
assuming you installed the software in the default folder.

A companion Windows application, logger, can be found in the
/ti/TivaWare_C_Series-x.x/tools/bin directory.  When the data logger's
STORAGE option is set to "HOST PC" and the board is connected to a PC
via the USB OTG connection, captured data will be transfered back to the PC
using the virtual serial port that the EK board offers.  When the logger
application is run, it will search for the first connected board
and display any sample data received.  The application also offers the
option to log the data to a file on the PC.

Please note that this code will not run on a EK-LM4F232 board due to
hardware differences. If tried an error will be show on the screen.
For DK-EM4F232 code please see the examples/boards/ek-lm4f232 directory.

-------------------------------------------------------------------------------

Copyright (c) 2011-2014 Texas Instruments Incorporated.  All rights reserved.
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
