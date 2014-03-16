//*****************************************************************************
//
// softi2c.c - Driver for the SoftI2C.
//
// Copyright (c) 2010-2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.0.12573 of the Tiva Utility Library.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup softi2c_api
//! @{
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "utils/softi2c.h"

//*****************************************************************************
//
// The states in the SoftI2C state machine.  The code depends upon the fact
// that the value of STATE_X1 is exactly one greater than STATE_X0 (for any
// value of X and for any following digit)...however there is no dependence on
// the values of STATE_Xn and STATE_Yn.
//
//*****************************************************************************
#define SOFTI2C_STATE_IDLE      0
#define SOFTI2C_STATE_START0    1
#define SOFTI2C_STATE_START1    2
#define SOFTI2C_STATE_START2    3
#define SOFTI2C_STATE_START3    4
#define SOFTI2C_STATE_START4    5
#define SOFTI2C_STATE_START5    6
#define SOFTI2C_STATE_START6    7
#define SOFTI2C_STATE_START7    8
#define SOFTI2C_STATE_ADDR0     9
#define SOFTI2C_STATE_ADDR1     10
#define SOFTI2C_STATE_ADDR2     11
#define SOFTI2C_STATE_ADDR3     12
#define SOFTI2C_STATE_SEND0     13
#define SOFTI2C_STATE_SEND1     14
#define SOFTI2C_STATE_SEND2     15
#define SOFTI2C_STATE_SEND3     16
#define SOFTI2C_STATE_RECV0     17
#define SOFTI2C_STATE_RECV1     18
#define SOFTI2C_STATE_RECV2     19
#define SOFTI2C_STATE_RECV3     20
#define SOFTI2C_STATE_STOP0     21
#define SOFTI2C_STATE_STOP1     22
#define SOFTI2C_STATE_STOP2     23
#define SOFTI2C_STATE_STOP3     24
#define SOFTI2C_STATE_STOP4     25

//*****************************************************************************
//
// The flags in the SoftI2C ui8Flags structure member.  The first four flags,
// RUN, START, STOP, and ACK, must match with the definitions of the
// SOFTI2C_CMD_* commands in softi2c.h.
//
//*****************************************************************************
#define SOFTI2C_FLAG_RUN        0
#define SOFTI2C_FLAG_START      1
#define SOFTI2C_FLAG_STOP       2
#define SOFTI2C_FLAG_ACK        3
#define SOFTI2C_FLAG_ADDR_ACK   5
#define SOFTI2C_FLAG_DATA_ACK   6
#define SOFTI2C_FLAG_RECEIVE    7

//*****************************************************************************
//
//! Performs the periodic update of the SoftI2C module.
//!
//! \param psI2C specifies the SoftI2C data structure.
//!
//! This function performs the periodic, time-based updates to the SoftI2C
//! module.  The transmission and reception of data over the SoftI2C link is
//! performed by the state machine in this function.
//!
//! This function must be called at four times the desired SoftI2C clock rate.
//! For example, to run the SoftI2C clock at 10 KHz, this function must be
//! called at a 40 KHz rate.
//!
//! \return None.
//
//*****************************************************************************
void
SoftI2CTimerTick(tSoftI2C *psI2C)
{
    //
    // Determine the current state of the state machine.
    //
    switch(psI2C->ui8State)
    {
        //
        // The state machine is idle.
        //
        case SOFTI2C_STATE_IDLE:
        {
            //
            // See if the START flag is set, indicating that a start condition
            // should be generated.
            //
            if(HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_START) == 1)
            {
                //
                // Based on the current state of the SCL and SDA pins, pick the
                // appropriate place within the state machine to begin the
                // start/repeated-start signalling.
                //
                if(HWREG(psI2C->ui32SCLGPIO) != 0)
                {
                    psI2C->ui8State = SOFTI2C_STATE_START4;
                }
                else if(HWREG(psI2C->ui32SDAGPIO) == 0)
                {
                    psI2C->ui8State = SOFTI2C_STATE_START0;
                }
                else
                {
                    psI2C->ui8State = SOFTI2C_STATE_START2;
                }
            }

            //
            // Otherwise, see if the RUN flag is set, indicating that a data
            // byte should be transferred.
            //
            else if(HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_RUN) == 1)
            {
                //
                // Start the transfer from the first bit.
                //
                psI2C->ui8CurrentBit = 0;

                //
                // See if a byte should be sent or received.
                //
                if(HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_RECEIVE) == 0)
                {
                    //
                    // A byte should be sent.
                    //
                    psI2C->ui8State = SOFTI2C_STATE_SEND0;
                }
                else
                {
                    //
                    // A byte should be received.  Clear out the receive data
                    // buffer in preparation for receiving the new byte.
                    //
                    psI2C->ui8Data = 0;
                    psI2C->ui8State = SOFTI2C_STATE_RECV0;
                }
            }

            //
            // Otherwise, see if the STOP flag is set, indicating that a stop
            // condition should be generated.
            //
            else if(HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_STOP) == 1)
            {
                //
                // Generate a stop condition.
                //
                psI2C->ui8State = SOFTI2C_STATE_STOP0;
            }

            //
            // See if the SoftI2C state machine has left the idle state.
            //
            if(psI2C->ui8State != SOFTI2C_STATE_IDLE)
            {
                //
                // The address and data ACK error flags should be cleared; they
                // will be set if appropriate while the current command is
                // being executed.
                //
                HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_ADDR_ACK) = 0;
                HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_DATA_ACK) = 0;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The beginning of the start condition sequence when SDA and SCL are
        // low.  SDA must be driven high prior to driving SCL high so that a
        // repeated-start is generated, instead of a stop then start.
        //
        case SOFTI2C_STATE_START0:
        {
            //
            // Set SDA high.
            //
            HWREG(psI2C->ui32SDAGPIO) = 255;

            //
            // Advance to the next state.
            //
            psI2C->ui8State = SOFTI2C_STATE_START1;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // Each of these states exists only to provide some timing delay in
        // order to conform with the signalling requirements of I2C.  This
        // depends upon STATE_Xn and STATE_X(n+1) being consecutively numbered.
        //
        case SOFTI2C_STATE_START1:
        case SOFTI2C_STATE_START3:
        case SOFTI2C_STATE_START5:
        case SOFTI2C_STATE_STOP1:
        case SOFTI2C_STATE_STOP3:
        {
            //
            // Advance to the next state.
            //
            psI2C->ui8State++;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In each of these states, SCL must be driven high.  This depends upon
        // STATE_Xn and STATE_X(n+1) being consecutively numbered.
        //
        case SOFTI2C_STATE_START2:
        case SOFTI2C_STATE_ADDR1:
        case SOFTI2C_STATE_SEND1:
        case SOFTI2C_STATE_RECV1:
        case SOFTI2C_STATE_STOP2:
        {
            //
            // Set SCL high.
            //
            HWREG(psI2C->ui32SCLGPIO) = 255;

            //
            // Advance to the next state.
            //
            psI2C->ui8State++;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In each of these states, SDA must be driven low.  This depends upon
        // STATE_Xn and STATE_X(n+1) being consecutively numbered.
        //
        case SOFTI2C_STATE_START4:
        case SOFTI2C_STATE_STOP0:
        {
            //
            // Set SDA low.
            //
            HWREG(psI2C->ui32SDAGPIO) = 0;

            //
            // Advance to the next state.
            //
            psI2C->ui8State++;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In this state, SCL must be driven low.
        //
        case SOFTI2C_STATE_START6:
        {
            //
            // Set SCL low.
            //
            HWREG(psI2C->ui32SCLGPIO) = 0;

            //
            // Advance to the next state.
            //
            psI2C->ui8State = SOFTI2C_STATE_START7;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In this state, the start condition has been generated.
        //
        case SOFTI2C_STATE_START7:
        {
            //
            // Start with the first bit of the address.
            //
            psI2C->ui8CurrentBit = 0;

            //
            // Advance to the address output state.
            //
            psI2C->ui8State = SOFTI2C_STATE_ADDR0;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In this state, the next bit of the slave address must be sent.
        //
        case SOFTI2C_STATE_ADDR0:
        {
            //
            // See if this is one of the first seven bits of the address phase.
            //
            if(psI2C->ui8CurrentBit < 7)
            {
                //
                // Write the next bit of the slave address to SDA.
                //
                HWREG(psI2C->ui32SDAGPIO) =
                    ((psI2C->ui8SlaveAddr &
                      (1 << (6 - psI2C->ui8CurrentBit))) ? 255 : 0);
            }

            //
            // Otherwise, see if this is the eight bit of the address phase
            // (which is the read/not write bit).
            //
            else if(psI2C->ui8CurrentBit == 7)
            {
                //
                // Write the read/not write bit to SDA.
                //
                HWREG(psI2C->ui32SDAGPIO) =
                    (HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_RECEIVE) ?
                     255 : 0);
            }

            //
            // Otherwise, this is the ninth bit of the address phase (in other
            // words, the ACK bit).
            //
            else
            {
                //
                // Change the SDA GPIO into an input so that the ACK or NAK
                // provided by the slave can be read.
                //
                MAP_GPIODirModeSet(psI2C->ui32SDAGPIO & 0xfffff000,
                                   (psI2C->ui32SDAGPIO & 0x00000fff) >> 2,
                                   GPIO_DIR_MODE_IN);
            }

            //
            // Advance to the next state.
            //
            psI2C->ui8State = SOFTI2C_STATE_ADDR1;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In each of these states, wait until SCL has gone high (it has been
        // released by the SoftI2C master, but may be held low by the slave).
        // This depends upon STATE_Xn and STATE_X(n+1) being consecutively
        // numbered.
        //
        case SOFTI2C_STATE_ADDR2:
        case SOFTI2C_STATE_SEND2:
        case SOFTI2C_STATE_RECV2:
        {
            //
            // See if SCL has gone high.
            //
            if(HWREG(psI2C->ui32SCLGPIO) != 0)
            {
                //
                // Advance to the next state now that SCL has gone high.
                //
                psI2C->ui8State++;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In this state, SCL must be driven low.  If on the ninth bit of the
        // address transfer, the ACK/NAK status is read from the slave.
        //
        case SOFTI2C_STATE_ADDR3:
        {
            //
            // See if this is the ninth bit of the address phase (in other
            // words, the ACK bit).
            //
            if(psI2C->ui8CurrentBit == 8)
            {
                //
                // See if the SDA line is high.
                //
                if(HWREG(psI2C->ui32SDAGPIO) != 0)
                {
                    //
                    // Since the SDA line is high, the address byte has not
                    // been ACKed by any slave.
                    //
                    HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_ADDR_ACK) = 1;
                }

                //
                // Change the SDA GPIO back into an output.
                //
                MAP_GPIODirModeSet(psI2C->ui32SDAGPIO & 0xfffff000,
                                   (psI2C->ui32SDAGPIO & 0x00000fff) >> 2,
                                   GPIO_DIR_MODE_OUT);

                //
                // The start phase (start or repeated-start, plus the address
                // byte) have completed, so clear the START flag.
                //
                HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_START) = 0;

                //
                // See if the RUN flag is set, indicating that a data byte
                // should be transferred as well.
                //
                if(HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_RUN) == 1)
                {
                    //
                    // Reset the current bit to zero for the start of the data
                    // phase.
                    //
                    psI2C->ui8CurrentBit = 0;

                    //
                    // See if the data byte is being sent or received.
                    //
                    if(HWREGBITB(&(psI2C->ui8Flags),
                                 SOFTI2C_FLAG_RECEIVE) == 0)
                    {
                        //
                        // The data byte is being sent, so advance to the data
                        // send state.
                        //
                        psI2C->ui8State = SOFTI2C_STATE_SEND0;
                    }
                    else
                    {
                        //
                        // The data byte is being received, so clear the data
                        // buffer and advance to the data receive state.
                        //
                        psI2C->ui8Data = 0;
                        psI2C->ui8State = SOFTI2C_STATE_RECV0;
                    }
                }

                //
                // Otherwise, see if the STOP flag is set, indicating that a
                // stop condition should be generated.
                //
                else if(HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_STOP) == 1)
                {
                    //
                    // Advance to the stop state.
                    //
                    psI2C->ui8State = SOFTI2C_STATE_STOP0;
                }

                //
                // Otherwise, go to the idle state.
                //
                else
                {
                    //
                    // Since the requested operations have completed, set the
                    // SoftI2C ``interrupt''.
                    //
                    psI2C->ui8IntStatus = 1;

                    //
                    // Advance to the idle state.
                    //
                    psI2C->ui8State = SOFTI2C_STATE_IDLE;
                }
            }

            //
            // Otherwise, the next bit of the address should be transferred.
            //
            else
            {
                //
                // Increment the bit count.
                //
                psI2C->ui8CurrentBit++;

                //
                // Advance to the address tranfer state.
                //
                psI2C->ui8State = SOFTI2C_STATE_ADDR0;
            }

            //
            // Set SCL low.
            //
            HWREG(psI2C->ui32SCLGPIO) = 0;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In this state, the next bit of the data byte must be sent.
        //
        case SOFTI2C_STATE_SEND0:
        {
            //
            // See if this is one of the first eight bits of the data phase.
            //
            if(psI2C->ui8CurrentBit < 8)
            {
                //
                // Write the next bit of the data byte to SDA.
                //
                HWREG(psI2C->ui32SDAGPIO) =
                    ((psI2C->ui8Data &
                      (1 << (7 - psI2C->ui8CurrentBit))) ? 255 : 0);
            }

            //
            // Otherwise, this is the ninth bit of the data phase (in other
            // words, the ACK bit).
            //
            else
            {
                //
                // Change the SDA GPIO into an input so that the ACK or NAK
                // provided by the slave can be read.
                //
                MAP_GPIODirModeSet(psI2C->ui32SDAGPIO & 0xfffff000,
                                   (psI2C->ui32SDAGPIO & 0x00000fff) >> 2,
                                   GPIO_DIR_MODE_IN);
            }

            //
            // Advance to the next state.
            //
            psI2C->ui8State = SOFTI2C_STATE_SEND1;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In this state, SCL must be driven low.  If on the ninth bit of the
        // data transfer, the ACK/NAK status is read from the slave.
        //
        case SOFTI2C_STATE_SEND3:
        {
            //
            // See if this is the ninth bit of the data phase (in other words,
            // the ACK bit).
            //
            if(psI2C->ui8CurrentBit == 8)
            {
                //
                // See if the SDA line is high.
                //
                if(HWREG(psI2C->ui32SDAGPIO) != 0)
                {
                    //
                    // Since the SDA line is high, the data byte has not been
                    // ACKed by the slave.
                    //
                    HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_DATA_ACK) = 1;
                }

                //
                // Change the SDA GPIO back into an output.
                //
                MAP_GPIODirModeSet(psI2C->ui32SDAGPIO & 0xfffff000,
                                   (psI2C->ui32SDAGPIO & 0x00000fff) >> 2,
                                   GPIO_DIR_MODE_OUT);

                //
                // The data phase has completed, so clear the RUN flag.
                //
                HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_RUN) = 0;

                //
                // See if the STOP flag is set, indicating that a stop
                // condition should be generated.
                //
                if(HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_STOP) == 1)
                {
                    //
                    // Advance to the stop state.
                    //
                    psI2C->ui8State = SOFTI2C_STATE_STOP0;
                }

                //
                // Otherwise, go to the idle state.
                //
                else
                {
                    //
                    // Since the requested operations have completed, set the
                    // SoftI2C ``interrupt''.
                    //
                    psI2C->ui8IntStatus = 1;

                    //
                    // Advance to the idle state.
                    //
                    psI2C->ui8State = SOFTI2C_STATE_IDLE;
                }
            }

            //
            // Otherwise, the next bit of the data should be transferred.
            //
            else
            {
                //
                // Increment the bit count.
                //
                psI2C->ui8CurrentBit++;

                //
                // Advance to the data transmit state.
                //
                psI2C->ui8State = SOFTI2C_STATE_SEND0;
            }

            //
            // Set SCL low.
            //
            HWREG(psI2C->ui32SCLGPIO) = 0;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In this state, the next bit of the data byte must be received.
        //
        case SOFTI2C_STATE_RECV0:
        {
            //
            // See if this is the first bit of the data phase.
            //
            if(psI2C->ui8CurrentBit == 0)
            {
                //
                // Change the SDA GPIO into an input so that the data provided
                // by the slave can be read.
                //
                MAP_GPIODirModeSet(psI2C->ui32SDAGPIO & 0xfffff000,
                                   (psI2C->ui32SDAGPIO & 0x00000fff) >> 2,
                                   GPIO_DIR_MODE_IN);
            }

            //
            // Otherwise, see if this is the ninth bit of the data phase (in
            // other words, the ACK bit).
            //
            else if(psI2C->ui8CurrentBit == 8)
            {
                //
                // Change the SDA GPIO into an output so that the ACK bit can
                // be driven to the slave.
                //
                MAP_GPIODirModeSet(psI2C->ui32SDAGPIO & 0xfffff000,
                                   (psI2C->ui32SDAGPIO & 0x00000fff) >> 2,
                                   GPIO_DIR_MODE_OUT);

                //
                // See if this byte should be ACKed or NAKed.
                //
                if(HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_ACK) == 1)
                {
                    //
                    // Drive SDA low to ACK the data byte.
                    //
                    HWREG(psI2C->ui32SDAGPIO) = 0;
                }
                else
                {
                    //
                    // Allow SDA to get pulled high to NAK the data byte.
                    //
                    HWREG(psI2C->ui32SDAGPIO) = 255;
                }
            }

            //
            // Advance to the next state.
            //
            psI2C->ui8State = SOFTI2C_STATE_RECV1;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In this state, SCL must be driven low.  For the first eight bits of
        // the data transfer, the data bits are read from the slave.
        //
        case SOFTI2C_STATE_RECV3:
        {
            //
            // See if this is the ninth bit of the data phase (in other words,
            // the ACK bit).
            //
            if(psI2C->ui8CurrentBit == 8)
            {
                //
                // The data phase has completed, so clear the RUN flag.
                //
                HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_RUN) = 0;

                //
                // See if the STOP flag is set, indicating that a stop
                // condition should be generated.
                //
                if(HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_STOP) == 1)
                {
                    //
                    // Advance to the stop state.
                    //
                    psI2C->ui8State = SOFTI2C_STATE_STOP0;
                }

                //
                // Otherwise, go to the idle state.
                //
                else
                {
                    //
                    // Since the requested operations have completed, set the
                    // SoftI2C ``interrupt''.
                    //
                    psI2C->ui8IntStatus = 1;

                    //
                    // Advance to the idle state.
                    //
                    psI2C->ui8State = SOFTI2C_STATE_IDLE;
                }
            }

            //
            // Otherwise, the next bit of the data should be transferred.
            //
            else
            {
                //
                // Read the next bit of data from the SDA line.
                //
                psI2C->ui8Data |= (HWREG(psI2C->ui32SDAGPIO) ?
                                   (1 << (7 - psI2C->ui8CurrentBit)) : 0);

                //
                // Increment the bit count.
                //
                psI2C->ui8CurrentBit++;

                //
                // Advance to the data receive state.
                //
                psI2C->ui8State = SOFTI2C_STATE_RECV0;
            }

            //
            // Set SCL low.
            //
            HWREG(psI2C->ui32SCLGPIO) = 0;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In this state, SDA must be driven high to create the stop condition.
        //
        case SOFTI2C_STATE_STOP4:
        {
            //
            // Set SDA high to create the stop condition.
            //
            HWREG(psI2C->ui32SDAGPIO) = 255;

            //
            // The stop condition has been generated, so clear the STOP flag.
            //
            HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_STOP) = 0;

            //
            // Since the requested operations have completed, set the SoftI2C
            // ``interrupt''.
            //
            psI2C->ui8IntStatus = 1;

            //
            // Advance to the idle state.
            //
            psI2C->ui8State = SOFTI2C_STATE_IDLE;

            //
            // This state has been handled.
            //
            break;
        }
    }

    //
    // Call the "interrupt" callback while there are enabled "interrupts"
    // asserted.  By calling in a loop until the "interrupts" are no longer
    // asserted, this mimics the behavior of a real hardware implementation of
    // the I2C peripheral.
    //
    while(((psI2C->ui8IntStatus & psI2C->ui8IntMask) != 0) &&
          (psI2C->pfnIntCallback != 0))
    {
        //
        // Call the callback function.
        //
        psI2C->pfnIntCallback();
    }
}

//*****************************************************************************
//
//! Initializes the SoftI2C module.
//!
//! \param psI2C specifies the SoftI2C data structure.
//!
//! This function initializes operation of the SoftI2C module.  After
//! successful initialization of the SoftI2C module, the software I2C bus is in
//! the idle state.
//!
//! \return None.
//
//*****************************************************************************
void
SoftI2CInit(tSoftI2C *psI2C)
{
    //
    // Configure the SCL pin.
    //
    MAP_GPIODirModeSet(psI2C->ui32SCLGPIO & 0xfffff000,
                       (psI2C->ui32SCLGPIO & 0x00000fff) >> 2,
                       GPIO_DIR_MODE_OUT);
    MAP_GPIOPadConfigSet(psI2C->ui32SCLGPIO & 0xfffff000,
                         (psI2C->ui32SCLGPIO & 0x00000fff) >> 2,
                         GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_OD);

    //
    // Set the SCL pin high.
    //
    HWREG(psI2C->ui32SCLGPIO) = 255;

    //
    // Configure the SDA pin.
    //
    MAP_GPIODirModeSet(psI2C->ui32SDAGPIO & 0xfffff000,
                       (psI2C->ui32SDAGPIO & 0x00000fff) >> 2,
                       GPIO_DIR_MODE_OUT);
    MAP_GPIOPadConfigSet(psI2C->ui32SDAGPIO & 0xfffff000,
                         (psI2C->ui32SDAGPIO & 0x00000fff) >> 2,
                         GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_OD);

    //
    // Set the SDA pin high.
    //
    HWREG(psI2C->ui32SDAGPIO) = 255;

    //
    // The ``interrupt'' is not asserted at the start.
    //
    psI2C->ui8IntStatus = 0;

    //
    // There are no flags at the start.
    //
    psI2C->ui8Flags = 0;

    //
    // Start the SoftI2C state machine in the idle state.
    //
    psI2C->ui8State = SOFTI2C_STATE_IDLE;
}

//*****************************************************************************
//
//! Sets the callback used by the SoftI2C module.
//!
//! \param psI2C specifies the SoftI2C data structure.
//! \param pfnCallback is a pointer to the callback function.
//!
//! This function sets the address of the callback function that is called when
//! there is an ``interrupt'' produced by the SoftI2C module.
//!
//! \return None.
//
//*****************************************************************************
void
SoftI2CCallbackSet(tSoftI2C *psI2C, void (*pfnCallback)(void))
{
    //
    // Save the callback function address.
    //
    psI2C->pfnIntCallback = pfnCallback;
}

//*****************************************************************************
//
//! Sets the GPIO pin to be used as the SoftI2C SCL signal.
//!
//! \param psI2C specifies the SoftI2C data structure.
//! \param ui32Base is the base address of the GPIO module.
//! \param ui8Pin is the bit-packed representation of the pin to use.
//!
//! This function sets the GPIO pin that is used for the SoftI2C SCL signal.
//!
//! The pin is specified using a bit-packed byte, where bit 0 of the byte
//! represents GPIO port pin 0, bit 1 represents GPIO port pin 1, and so on.
//!
//! \return None.
//
//*****************************************************************************
void
SoftI2CSCLGPIOSet(tSoftI2C *psI2C, uint32_t ui32Base, uint8_t ui8Pin)
{
    //
    // Save the base address and pin for the SCL signal.
    //
    psI2C->ui32SCLGPIO = ui32Base + (ui8Pin << 2);
}

//*****************************************************************************
//
//! Sets the GPIO pin to be used as the SoftI2C SDA signal.
//!
//! \param psI2C specifies the SoftI2C data structure.
//! \param ui32Base is the base address of the GPIO module.
//! \param ui8Pin is the bit-packed representation of the pin to use.
//!
//! This function sets the GPIO pin that is used for the SoftI2C SDA signal.
//!
//! The pin is specified using a bit-packed byte, where bit 0 of the byte
//! represents GPIO port pin 0, bit 1 represents GPIO port pin 1, and so on.
//!
//! \return None.
//
//*****************************************************************************
void
SoftI2CSDAGPIOSet(tSoftI2C *psI2C, uint32_t ui32Base, uint8_t ui8Pin)
{
    //
    // Save the base address and pin for the SDA signal.
    //
    psI2C->ui32SDAGPIO = ui32Base + (ui8Pin << 2);
}

//*****************************************************************************
//
//! Enables the SoftI2C ``interrupt''.
//!
//! \param psI2C specifies the SoftI2C data structure.
//!
//! Enables the SoftI2C ``interrupt'' source.
//!
//! \return None.
//
//*****************************************************************************
void
SoftI2CIntEnable(tSoftI2C *psI2C)
{
    //
    // Enable the master interrupt.
    //
    psI2C->ui8IntMask = 1;
}

//*****************************************************************************
//
//! Disables the SoftI2C ``interrupt''.
//!
//! \param psI2C specifies the SoftI2C data structure.
//!
//! Disables the SoftI2C ``interrupt'' source.
//!
//! \return None.
//
//*****************************************************************************
void
SoftI2CIntDisable(tSoftI2C *psI2C)
{
    //
    // Disable the master interrupt.
    //
    psI2C->ui8IntMask = 0;
}

//*****************************************************************************
//
//! Gets the current SoftI2C ``interrupt'' status.
//!
//! \param psI2C specifies the SoftI2C data structure.
//! \param bMasked is \b false if the raw ``interrupt'' status is requested and
//! \b true if the masked ``interrupt'' status is requested.
//!
//! This returns the ``interrupt'' status for the SoftI2C module.  Either the
//! raw ``interrupt'' status or the status of ``interrupts'' that are allowed
//! to reflect to the processor can be returned.
//!
//! \return The current interrupt status, returned as \b true if active
//! or \b false if not active.
//
//*****************************************************************************
bool
SoftI2CIntStatus(tSoftI2C *psI2C, bool bMasked)
{
    //
    // Return either the interrupt status or the raw interrupt status as
    // requested.
    //
    if(bMasked)
    {
        return((psI2C->ui8IntStatus & psI2C->ui8IntMask) ? true : false);
    }
    else
    {
        return(psI2C->ui8IntStatus ? true : false);
    }
}

//*****************************************************************************
//
//! Clears the SoftI2C ``interrupt''.
//!
//! \param psI2C specifies the SoftI2C data structure.
//!
//! The SoftI2C ``interrupt'' source is cleared, so that it no longer asserts.
//! This function must be called in the ``interrupt'' handler to keep it from
//! being called again immediately on exit.
//!
//! \return None.
//
//*****************************************************************************
void
SoftI2CIntClear(tSoftI2C *psI2C)
{
    //
    // Clear the SoftI2C interrupt source.
    //
    psI2C->ui8IntStatus = 0;
}

//*****************************************************************************
//
//! Sets the address that the SoftI2C module places on the bus.
//!
//! \param psI2C specifies the SoftI2C data structure.
//! \param ui8SlaveAddr 7-bit slave address
//! \param bReceive flag indicating the type of communication with the slave.
//!
//! This function sets the address that the SoftI2C module places on the bus
//! when initiating a transaction.  When the \e bReceive parameter is set to
//! \b true, the address indicates that the SoftI2C moudle is initiating a read
//! from the slave; otherwise the address indicates that the SoftI2C module is
//! initiating a write to the slave.
//!
//! \return None.
//
//*****************************************************************************
void
SoftI2CSlaveAddrSet(tSoftI2C *psI2C, uint8_t ui8SlaveAddr,
                    bool bReceive)
{
    //
    // Check the arguments.
    //
    ASSERT(!(ui8SlaveAddr & 0x80));

    //
    // Set the address of the slave with which the master will communicate.
    //
    psI2C->ui8SlaveAddr = ui8SlaveAddr;

    //
    // Set a flag to indicate if this is a transmit or receive.
    //
    if(bReceive)
    {
        HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_RECEIVE) = 1;
    }
    else
    {
        HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_RECEIVE) = 0;
    }
}

//*****************************************************************************
//
//! Indicates whether or not the SoftI2C module is busy.
//!
//! \param psI2C specifies the SoftI2C data structure.
//!
//! This function returns an indication of whether or not the SoftI2C module is
//! busy transmitting or receiving data.
//!
//! \return Returns \b true if the SoftI2C module is busy; otherwise, returns
//! \b false.
//
//*****************************************************************************
bool
SoftI2CBusy(tSoftI2C *psI2C)
{
    //
    // Return the busy status.
    //
    if(psI2C->ui8State != SOFTI2C_STATE_IDLE)
    {
        return(true);
    }
    else
    {
        return(false);
    }
}

//*****************************************************************************
//
//! Controls the state of the SoftI2C module.
//!
//! \param psI2C specifies the SoftI2C data structure.
//! \param ui32Cmd command to be issued to the SoftI2C module.
//!
//! This function is used to control the state of the SoftI2C module send and
//! receive operations.  The \e ui8Cmd parameter can be one of the following
//! values:
//!
//! - \b SOFTI2C_CMD_SINGLE_SEND
//! - \b SOFTI2C_CMD_SINGLE_RECEIVE
//! - \b SOFTI2C_CMD_BURST_SEND_START
//! - \b SOFTI2C_CMD_BURST_SEND_CONT
//! - \b SOFTI2C_CMD_BURST_SEND_FINISH
//! - \b SOFTI2C_CMD_BURST_SEND_ERROR_STOP
//! - \b SOFTI2C_CMD_BURST_RECEIVE_START
//! - \b SOFTI2C_CMD_BURST_RECEIVE_CONT
//! - \b SOFTI2C_CMD_BURST_RECEIVE_FINISH
//! - \b SOFTI2C_CMD_BURST_RECEIVE_ERROR_STOP
//!
//! \return None.
//
//*****************************************************************************
void
SoftI2CControl(tSoftI2C *psI2C, uint32_t ui32Cmd)
{
    //
    // Check the arguments.
    //
    ASSERT((ui32Cmd == SOFTI2C_CMD_SINGLE_SEND) ||
           (ui32Cmd == SOFTI2C_CMD_SINGLE_RECEIVE) ||
           (ui32Cmd == SOFTI2C_CMD_BURST_SEND_START) ||
           (ui32Cmd == SOFTI2C_CMD_BURST_SEND_CONT) ||
           (ui32Cmd == SOFTI2C_CMD_BURST_SEND_FINISH) ||
           (ui32Cmd == SOFTI2C_CMD_BURST_SEND_ERROR_STOP) ||
           (ui32Cmd == SOFTI2C_CMD_BURST_RECEIVE_START) ||
           (ui32Cmd == SOFTI2C_CMD_BURST_RECEIVE_CONT) ||
           (ui32Cmd == SOFTI2C_CMD_BURST_RECEIVE_FINISH) ||
           (ui32Cmd == SOFTI2C_CMD_BURST_RECEIVE_ERROR_STOP));

    //
    // Send the command.
    //
    psI2C->ui8Flags = (psI2C->ui8Flags & 0xf0) | ui32Cmd;
}

//*****************************************************************************
//
//! Gets the error status of the SoftI2C module.
//!
//! \param psI2C specifies the SoftI2C data structure.
//!
//! This function is used to obtain the error status of the SoftI2C module send
//! and receive operations.
//!
//! \return Returns the error status, as one of \b SOFTI2C_ERR_NONE,
//! \b SOFTI2C_ERR_ADDR_ACK, or \b SOFTI2C_ERR_DATA_ACK.
//
//*****************************************************************************
uint32_t
SoftI2CErr(tSoftI2C *psI2C)
{
    //
    // If the SoftI2C is busy, there is no error to report.
    //
    if(psI2C->ui8State != SOFTI2C_STATE_IDLE)
    {
        return(SOFTI2C_ERR_NONE);
    }

    //
    // Return any errors that may have occurred.
    //
    return((HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_ADDR_ACK) ?
            SOFTI2C_ERR_ADDR_ACK : 0) |
           (HWREGBITB(&(psI2C->ui8Flags), SOFTI2C_FLAG_DATA_ACK) ?
            SOFTI2C_ERR_DATA_ACK : 0));
}

//*****************************************************************************
//
//! Transmits a byte from the SoftI2C module.
//!
//! \param psI2C specifies the SoftI2C data structure.
//! \param ui8Data data to be transmitted from the SoftI2C module.
//!
//! This function places the supplied data into SoftI2C module in preparation
//! for being transmitted via an appropriate call to SoftI2CControl().
//!
//! \return None.
//
//*****************************************************************************
void
SoftI2CDataPut(tSoftI2C *psI2C, uint8_t ui8Data)
{
    //
    // Write the byte.
    //
    psI2C->ui8Data = ui8Data;
}

//*****************************************************************************
//
//! Receives a byte that has been sent to the SoftI2C module.
//!
//! \param psI2C specifies the SoftI2C data structure.
//!
//! This function reads a byte of data from the SoftI2C module that was
//! received as a result of an appropriate call to SoftI2CControl().
//!
//! \return Returns the byte received by the SoftI2C module, cast as an
//! uint32_t.
//
//*****************************************************************************
uint32_t
SoftI2CDataGet(tSoftI2C *psI2C)
{
    //
    // Read a byte.
    //
    return(psI2C->ui8Data);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
