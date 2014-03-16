Bit-Banding

This example application demonstrates the use of the bit-banding
capabilities of the Cortex-M4F microprocessor.  All of SRAM and all of the
peripherals reside within bit-band regions, meaning that bit-banding
operations can be applied to any of them.  In this example, a variable in
SRAM is set to a particular value one bit at a time using bit-banding
operations (it would be more efficient to do a single non-bit-banded write;
this simply demonstrates the operation of bit-banding).

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
