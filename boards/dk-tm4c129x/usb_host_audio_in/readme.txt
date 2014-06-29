USB host audio example application using a USB audio device for input
and PWM audio to the on board speaker for output.

This example application demonstrates streaming audio from a USB audio
device that supports recording an audio source at 48000 16 bit stereo.
The application starts recording audio from the USB audio device when
the "Record" button is pressed and stream it to the speaker on the board.
Because some audio devices require more power, you may need to use an
external 5 volt supply to provide enough power to the USB audio device.

The application can be recompiled to run using and external USB phy to
implement a high speed host using an external USB phy.  To use the external
phy the application must be built with \b USE_ULPI defined.  This disables
the internal phy and the connector on the DK-TM4C129X board and enables the
connections to the external ULPI phy pins on the DK-TM4C129X board.

-------------------------------------------------------------------------------

Copyright (c) 2010-2014 Texas Instruments Incorporated.  All rights reserved.
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
