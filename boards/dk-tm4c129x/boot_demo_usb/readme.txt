Boot Loader USB Demo

This example application is used in conjunction with the USB boot loader in
ROM and turns the development board into a composite device supporting a
mouse via the Human Interface Device class and also publishing runtime
Device Firmware Upgrade (DFU) capability.  Dragging a finger or stylus over
the touchscreen translates into mouse movement and presses on marked areas
at the bottom of the screen indicate mouse button press.  This input is
used to generate messages in HID reports sent to the USB host allowing the
development board to control the mouse pointer on the host system.

Since the device also publishes a DFU interface, host software such as the
dfuprog tool can determine that the device is capable of receiving software
updates over USB.  The runtime DFU protocol allows such tools to signal the
device to switch into DFU mode and prepare to receive a new software image.

Runtime DFU functionality requires only that the device listen for a
particular request (DETACH) from the host and, when this is received,
transfer control to the USB boot loader via the normal means to reenumerate
as a pure DFU device capable of uploading and downloading firmware images.

Windows device drivers for both the runtime and DFU mode of operation can
be found in <tt>C:/TI/TivaWare_C_Series-x.x/windows_drivers</tt> assuming
you installed TivaWare in the default directory.

To illustrate runtime DFU capability, use the <tt>dfuprog</tt> tool which
is part of the Tiva Windows USB Examples package (SW-USB-win-xxxx.msi)
Assuming this package is installed in the default location, the
<tt>dfuprog</tt> executable can be found in the
<tt>C:/Program Files/Texas Instruments/Tiva/usb_examples</tt> or 
<tt>C:/Program Files (x86)/Texas Instruments/Tiva/usb_examples</tt>
directory.

With the device connected to your PC and the device driver installed, enter
the following command to enumerate DFU devices:

<tt>dfuprog -e</tt>

This will list all DFU-capable devices found and you should see that you
have one or two devices available which are in ``Runtime'' mode.

If you see two devices, it is strongly recommended that you disconnect
ICDI debug port from the PC, and power the board either with a 5V external
power brick or any usb wall charger which is not plugged in your pc.
This way, your PC is connected to the board only through USB OTG port. 
The reason for this is that the ICDI chip on the board is DFU-capable
device as well as TM4C129X, if not careful, the firmware on the ICDI
chip could be accidently erased which can not restored easily. As a
result, debug capabilities would be lost!

If IDCI debug port is disconnected from your PC, you should see only one
device from above command, and its index should be 0, and should be named
as ``Mouse with Device Firmware Upgrade''. 
If for any reason that you cannot provide the power to the board without
connecting ICDI debug port to your PC, the above command should show two
devices, the second device is probably named as
``In-Circuit Debug interface'', and we need to be careful not to update
the firmware on that device. So please take careful note of the index for
the device ``Mouse with Device Firmware Upgrade'', it could be 0 or 1, we
will need this index number for the following command. 
Entering the following command will switch this device into DFU mode and
leave it ready to receive a new firmware image:

<tt>dfuprog -i index -m</tt>

After entering this command, you should notice that the device disconnects
from the USB bus and reconnects again.  Running ``<tt>dfuprog -e</tt>'' a
second time will show that the device is now in DFU mode and ready to
receive downloads.  At this point, either LM Flash Programmer or dfuprog
may be used to send a new application binary to the device.

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
