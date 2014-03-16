Interrupts

This example application demonstrates the interrupt preemption and
tail-chaining capabilities of Cortex-M4 microprocessor and NVIC.  Nested
interrupts are synthesized when the interrupts have the same priority,
increasing priorities, and decreasing priorities.  With increasing
priorities, preemption will occur; in the other two cases tail-chaining
will occur.  The currently pending interrupts and the currently executing
interrupt will be displayed on the UART; GPIO pins B3, L1 and L0 (the
GPIO on jumper J27 on the left edge of the board) will be asserted upon
interrupt handler entry and de-asserted before interrupt handler exit so
that the off-to-on time can be observed with a scope or logic analyzer to
see the speed of tail-chaining (for the two cases where tail-chaining is
occurring).

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
