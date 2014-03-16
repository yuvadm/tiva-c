//*****************************************************************************
//
// softuart.c - Driver for the SoftUART.
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
//! \addtogroup softuart_api
//! @{
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/uart.h"
#include "utils/softuart.h"

//*****************************************************************************
//
// The states in the SoftUART transmit state machine.  The code depends upon
// the fact that the value of TXSTATE_DATA_n is n + 1, and that TXSTATE_DATA_0
// is 1.
//
//*****************************************************************************
#define SOFTUART_TXSTATE_IDLE   0
#define SOFTUART_TXSTATE_DATA_0 1
#define SOFTUART_TXSTATE_DATA_1 2
#define SOFTUART_TXSTATE_DATA_2 3
#define SOFTUART_TXSTATE_DATA_3 4
#define SOFTUART_TXSTATE_DATA_4 5
#define SOFTUART_TXSTATE_DATA_5 6
#define SOFTUART_TXSTATE_DATA_6 7
#define SOFTUART_TXSTATE_DATA_7 8
#define SOFTUART_TXSTATE_START  9
#define SOFTUART_TXSTATE_PARITY 10
#define SOFTUART_TXSTATE_STOP_0 11
#define SOFTUART_TXSTATE_STOP_1 12
#define SOFTUART_TXSTATE_BREAK  13

//*****************************************************************************
//
// The states of the SoftUART receive state machine.  The code depends upon the
// the fact that the value of RXSTATE_DATA_n is n, and that RXSTATE_DATA_0 is
// 0.
//
//*****************************************************************************
#define SOFTUART_RXSTATE_DATA_0 0
#define SOFTUART_RXSTATE_DATA_1 1
#define SOFTUART_RXSTATE_DATA_2 2
#define SOFTUART_RXSTATE_DATA_3 3
#define SOFTUART_RXSTATE_DATA_4 4
#define SOFTUART_RXSTATE_DATA_5 5
#define SOFTUART_RXSTATE_DATA_6 6
#define SOFTUART_RXSTATE_DATA_7 7
#define SOFTUART_RXSTATE_IDLE   8
#define SOFTUART_RXSTATE_PARITY 9
#define SOFTUART_RXSTATE_STOP_0 10
#define SOFTUART_RXSTATE_STOP_1 11
#define SOFTUART_RXSTATE_BREAK  12
#define SOFTUART_RXSTATE_DELAY  13

//*****************************************************************************
//
// The flags in the SoftUART ui8Flags structure member.
//
//*****************************************************************************
#define SOFTUART_FLAG_ENABLE    0x01
#define SOFTUART_FLAG_TXBREAK   0x02

//*****************************************************************************
//
// The flags in the SoftUART ui8RxFlags structure member.
//
//*****************************************************************************
#define SOFTUART_RXFLAG_OE      0x08
#define SOFTUART_RXFLAG_BE      0x04
#define SOFTUART_RXFLAG_PE      0x02
#define SOFTUART_RXFLAG_FE      0x01

//*****************************************************************************
//
// Additional internal configuration stored in the SoftUART ui16Config
// structure member.
//
//*****************************************************************************
#define SOFTUART_CONFIG_BASE_M  0x00ff
#define SOFTUART_CONFIG_EXT_M   0xff00
#define SOFTUART_CONFIG_TXLVL_M 0x0700
#define SOFTUART_CONFIG_TXLVL_1 0x0000
#define SOFTUART_CONFIG_TXLVL_2 0x0100
#define SOFTUART_CONFIG_TXLVL_4 0x0200
#define SOFTUART_CONFIG_TXLVL_6 0x0300
#define SOFTUART_CONFIG_TXLVL_7 0x0400
#define SOFTUART_CONFIG_RXLVL_M 0x3800
#define SOFTUART_CONFIG_RXLVL_1 0x0000
#define SOFTUART_CONFIG_RXLVL_2 0x0800
#define SOFTUART_CONFIG_RXLVL_4 0x1000
#define SOFTUART_CONFIG_RXLVL_6 0x1800
#define SOFTUART_CONFIG_RXLVL_7 0x2000

//*****************************************************************************
//
// The odd parity of each possible data byte.  The odd parity of N can be found
// by looking at bit N % 32 of word N / 32.
//
//*****************************************************************************
static uint32_t g_pui32ParityOdd[] =
{
    0x69969669, 0x96696996, 0x96696996, 0x69969669,
    0x96696996, 0x69969669, 0x69969669, 0x96696996
};

//*****************************************************************************
//
//! Initializes the SoftUART module.
//!
//! \param psUART specifies the soft UART data structure.
//!
//! This function initializes the data structure for the SoftUART module,
//! putting it into the default configuration.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTInit(tSoftUART *psUART)
{
    //
    // Clear the SoftUART data structure.
    //
    memset(psUART, 0, sizeof(tSoftUART));

    //
    // Set the default transmit and receive buffer interrupt level.
    //
    psUART->ui16Config = SOFTUART_CONFIG_TXLVL_4 | SOFTUART_CONFIG_RXLVL_4;
}

//*****************************************************************************
//
//! Sets the configuration of a SoftUART module.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param ui32Config is the data format for the port (number of data bits,
//! number of stop bits, and parity).
//!
//! This function configures the SoftUART for operation in the specified data
//! format, as specified in the \e ui32Config parameter.
//!
//! The \e ui32Config parameter is the logical OR of three values: the number
//! of data bits, the number of stop bits, and the parity.
//! \b SOFTUART_CONFIG_WLEN_8, \b SOFTUART_CONFIG_WLEN_7,
//! \b SOFTUART_CONFIG_WLEN_6, and \b SOFTUART_CONFIG_WLEN_5 select from eight
//! to five data bits per byte (respectively).  \b SOFTUART_CONFIG_STOP_ONE and
//! \b SOFTUART_CONFIG_STOP_TWO select one or two stop bits (respectively).
//! \b SOFTUART_CONFIG_PAR_NONE, \b SOFTUART_CONFIG_PAR_EVEN,
//! \b SOFTUART_CONFIG_PAR_ODD, \b SOFTUART_CONFIG_PAR_ONE, and
//! \b SOFTUART_CONFIG_PAR_ZERO select the parity mode (no parity bit, even
//! parity bit, odd parity bit, parity bit always one, and parity bit always
//! zero, respectively).
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTConfigSet(tSoftUART *psUART, uint32_t ui32Config)
{
    //
    // See if a GPIO pin has been set for Tx.
    //
    if(psUART->ui32TxGPIO != 0)
    {
        //
        // Configure the Tx pin.
        //
        MAP_GPIOPinTypeGPIOOutput(psUART->ui32TxGPIO & 0xfffff000,
                                  (psUART->ui32TxGPIO & 0x00000fff) >> 2);

        //
        // Set the Tx pin high.
        //
        HWREG(psUART->ui32TxGPIO) = 255;
    }

    //
    // See if a GPIO pin has been set for Rx.
    //
    if(psUART->ui32RxGPIOPort != 0)
    {
        //
        // Configure the Rx pin.
        //
        MAP_GPIOPinTypeGPIOInput(psUART->ui32RxGPIOPort, psUART->ui8RxPin);

        //
        // Set the Rx pin to generate an interrupt on the next falling edge.
        //
        MAP_GPIOIntTypeSet(psUART->ui32RxGPIOPort, psUART->ui8RxPin,
                           GPIO_FALLING_EDGE);

        //
        // Enable the Rx pin interrupt.
        //
        GPIOIntClear(psUART->ui32RxGPIOPort, psUART->ui8RxPin);
        GPIOIntEnable(psUART->ui32RxGPIOPort, psUART->ui8RxPin);
    }

    //
    // Make sure that the transmit and receive buffers are empty.
    //
    psUART->ui16TxBufferRead = 0;
    psUART->ui16TxBufferWrite = 0;
    psUART->ui16RxBufferRead = 0;
    psUART->ui16RxBufferWrite = 0;

    //
    // Save the data format.
    //
    psUART->ui16Config = ((psUART->ui16Config & SOFTUART_CONFIG_EXT_M) |
                          (ui32Config & SOFTUART_CONFIG_BASE_M));

    //
    // Enable the SoftUART module.
    //
    psUART->ui8Flags |= SOFTUART_FLAG_ENABLE;

    //
    // The next value to be written to the Tx pin is one since the SoftUART is
    // idle.
    //
    psUART->ui8TxNext = 255;

    //
    // Start the SoftUART state machines in the idle state.
    //
    psUART->ui8TxState = SOFTUART_TXSTATE_IDLE;
    psUART->ui8RxState = SOFTUART_RXSTATE_IDLE;
}

//*****************************************************************************
//
//! Performs the periodic update of the SoftUART transmitter.
//!
//! \param psUART specifies the SoftUART data structure.
//!
//! This function performs the periodic, time-based updates to the SoftUART
//! transmitter.  The transmission of data from the SoftUART is performed by
//! the state machine in this function.
//!
//! This function must be called at the desired SoftUART baud rate.  For
//! example, to run the SoftUART at 115,200 baud, this function must be called
//! at a 115,200 Hz rate.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTTxTimerTick(tSoftUART *psUART)
{
    uint32_t ui32Temp;

    //
    // Write the next value to the Tx data line.  This value was computed on
    // the previous timer tick, which helps to reduce the jitter on the Tx
    // edges (which is important since a UART connection does not contain a
    // clock signal).
    //
    HWREG(psUART->ui32TxGPIO) = psUART->ui8TxNext;

    //
    // Determine the current state of the state machine.
    //
    switch(psUART->ui8TxState)
    {
        //
        // The state machine is idle.
        //
        case SOFTUART_TXSTATE_IDLE:
        {
            //
            // See if the SoftUART module is enabled.
            //
            if(!(psUART->ui8Flags & SOFTUART_FLAG_ENABLE))
            {
                //
                // The SoftUART module is not enabled, so do nothing and stay
                // in the idle state.
                //
                break;
            }

            //
            // See if the break signal should be asserted.
            //
            else if(psUART->ui8Flags & SOFTUART_FLAG_TXBREAK)
            {
                //
                // The data line should be driven low while in the break state.
                //
                psUART->ui8TxNext = 0;

                //
                // Move to the break state.
                //
                psUART->ui8TxState = SOFTUART_TXSTATE_BREAK;
            }

            //
            // Otherwise, see if there is data in the transmit buffer.
            //
            else if(psUART->ui16TxBufferRead != psUART->ui16TxBufferWrite)
            {
                //
                // The data line should be driven low to indicate a start bit.
                //
                psUART->ui8TxNext = 0;

                //
                // Move to the start bit state.
                //
                psUART->ui8TxState = SOFTUART_TXSTATE_START;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the start bit state.
        //
        case SOFTUART_TXSTATE_START:
        {
            //
            // Get the next byte to be transmitted.
            //
            psUART->ui8TxData = psUART->pui8TxBuffer[psUART->ui16TxBufferRead];

            //
            // The next value to be written to the data line is the LSB of the
            // next data byte.
            //
            psUART->ui8TxNext = (psUART->ui8TxData & 1) ? 255 : 0;

            //
            // Move to the data bit 0 state.
            //
            psUART->ui8TxState = SOFTUART_TXSTATE_DATA_0;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In each of these states, a bit of the data byte must be output.
        // This depends upon TXSTATE_DATA_n and TXSTATE_DATA_(n+1) being
        // consecutively numbered.
        //
        case SOFTUART_TXSTATE_DATA_0:
        case SOFTUART_TXSTATE_DATA_1:
        case SOFTUART_TXSTATE_DATA_2:
        case SOFTUART_TXSTATE_DATA_3:
        {
            //
            // The next value to be written to the data line is the next bit of
            // the data byte.
            //
            psUART->ui8TxNext =
                (psUART->ui8TxData & (1 << psUART->ui8TxState)) ? 255 : 0;

            //
            // Advance to the next state.
            //
            psUART->ui8TxState++;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In each of these states, a bit of the data byte must be output.
        // Additionally, based on the configuration of the SoftUART, this bit
        // might be the last data bit of the data byte.  This depends upon
        // TXSTATE_DATA_n and TXSTATE_DATA_(n+1) being consecutively numbered.
        //
        case SOFTUART_TXSTATE_DATA_4:
        case SOFTUART_TXSTATE_DATA_5:
        case SOFTUART_TXSTATE_DATA_6:
        case SOFTUART_TXSTATE_DATA_7:
        {
            //
            // See if the bit that was just transferred is the last bit of the
            // data byte (based on the configuration of the SoftUART).
            //
            if(((psUART->ui16Config & SOFTUART_CONFIG_WLEN_MASK) >>
                SOFTUART_CONFIG_WLEN_S) ==
               (psUART->ui8TxState - SOFTUART_TXSTATE_DATA_4))
            {
                //
                // See if parity is enabled.
                //
                if((psUART->ui16Config & SOFTUART_CONFIG_PAR_MASK) !=
                   SOFTUART_CONFIG_PAR_NONE)
                {
                    //
                    // See if the parity is set to one.
                    //
                    if((psUART->ui16Config & SOFTUART_CONFIG_PAR_MASK) ==
                       SOFTUART_CONFIG_PAR_ONE)
                    {
                        //
                        // The next value to be written to the data line is
                        // one.
                        //
                        psUART->ui8TxNext = 255;
                    }

                    //
                    // Otherwise, see if the parity is set to zero.
                    //
                    else if((psUART->ui16Config & SOFTUART_CONFIG_PAR_MASK) ==
                            SOFTUART_CONFIG_PAR_ZERO)
                    {
                        //
                        // The next value to be written to the data line is
                        // zero.
                        //
                        psUART->ui8TxNext = 0;
                    }

                    //
                    // Otherwise, there is either even or odd parity.
                    //
                    else
                    {
                        //
                        // Find the odd parity for the data byte.
                        //
                        psUART->ui8TxNext =
                            ((g_pui32ParityOdd[psUART->ui8TxData >> 5] &
                              (1 << (psUART->ui8TxData & 31))) ? 255 : 0);

                        //
                        // If the parity is set to even, then invert the
                        // parity just computed (making it even parity).
                        //
                        if((psUART->ui16Config & SOFTUART_CONFIG_PAR_MASK) ==
                           SOFTUART_CONFIG_PAR_EVEN)
                        {
                            psUART->ui8TxNext ^= 255;
                        }
                    }

                    //
                    // Advance to the parity state.
                    //
                    psUART->ui8TxState = SOFTUART_TXSTATE_PARITY;
                }

                //
                // Parity is not enabled.
                //
                else
                {
                    //
                    // The next value to write to the data line is the stop
                    // bit.
                    //
                    psUART->ui8TxNext = 255;

                    //
                    // See if there are one or two stop bits.
                    //
                    if((psUART->ui16Config & SOFTUART_CONFIG_STOP_MASK) ==
                       SOFTUART_CONFIG_STOP_TWO)
                    {
                        //
                        // Advance to the two stop bits state.
                        //
                        psUART->ui8TxState = SOFTUART_TXSTATE_STOP_0;
                    }
                    else
                    {
                        //
                        // Advance to the one stop bit state.
                        //
                        psUART->ui8TxState = SOFTUART_TXSTATE_STOP_1;
                    }
                }
            }

            //
            // Otherwise, there are more data bits to transfer.
            //
            else
            {
                //
                // The next value to be written to the data line is the next
                // bit of the data byte.
                //
                psUART->ui8TxNext =
                    (psUART->ui8TxData & (1 << psUART->ui8TxState)) ? 255 : 0;

                //
                // Advance to the next state.
                //
                psUART->ui8TxState++;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the parity bit state.
        //
        case SOFTUART_TXSTATE_PARITY:
        {
            //
            // The next value to write to the data line is the stop bit.
            //
            psUART->ui8TxNext = 255;

            //
            // See if there are one or two stop bits.
            //
            if((psUART->ui16Config & SOFTUART_CONFIG_STOP_MASK) ==
               SOFTUART_CONFIG_STOP_TWO)
            {
                //
                // Advance to the two stop bits state.
                //
                psUART->ui8TxState = SOFTUART_TXSTATE_STOP_0;
            }
            else
            {
                //
                // Advance to the one stop bit state.
                //
                psUART->ui8TxState = SOFTUART_TXSTATE_STOP_1;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the two stop bits state.
        //
        case SOFTUART_TXSTATE_STOP_0:
        {
            //
            // Advance to the one stop bit state.
            //
            psUART->ui8TxState = SOFTUART_TXSTATE_STOP_1;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the one stop bit state.
        //
        case SOFTUART_TXSTATE_STOP_1:
        {
            //
            // The data byte has been completely transferred, so advance the
            // read pointer.
            //
            psUART->ui16TxBufferRead++;
            if(psUART->ui16TxBufferRead == psUART->ui16TxBufferLen)
            {
                psUART->ui16TxBufferRead = 0;
            }

            //
            // Determine the number of characters in the transmit buffer.
            //
            if(psUART->ui16TxBufferRead > psUART->ui16TxBufferWrite)
            {
                ui32Temp = (psUART->ui16TxBufferLen -
                            (psUART->ui16TxBufferRead -
                             psUART->ui16TxBufferWrite));
            }
            else
            {
                ui32Temp = (psUART->ui16TxBufferWrite -
                            psUART->ui16TxBufferRead);
            }

            //
            // If the transmit buffer fullness just crossed the programmed
            // level, generate a transmit "interrupt".
            //
            if(ui32Temp == psUART->ui16TxBufferLevel)
            {
                psUART->ui16IntStatus |= SOFTUART_INT_TX;
            }

            //
            // See if the SoftUART module is enabled.
            //
            if(!(psUART->ui8Flags & SOFTUART_FLAG_ENABLE))
            {
                //
                // The SoftUART module is not enabled, so do advance to the
                // idle state.
                //
                psUART->ui8TxState = SOFTUART_TXSTATE_IDLE;
            }

            //
            // See if the break signal should be asserted.
            //
            else if(psUART->ui8Flags & SOFTUART_FLAG_TXBREAK)
            {
                //
                // The data line should be driven low while in the break state.
                //
                psUART->ui8TxNext = 0;

                //
                // Move to the break state.
                //
                psUART->ui8TxState = SOFTUART_TXSTATE_BREAK;
            }

            //
            // Otherwise, see if there is data in the transmit buffer.
            //
            else if(psUART->ui16TxBufferRead != psUART->ui16TxBufferWrite)
            {
                //
                // The data line should be driven low to indicate a start bit.
                //
                psUART->ui8TxNext = 0;

                //
                // Move to the start bit state.
                //
                psUART->ui8TxState = SOFTUART_TXSTATE_START;
            }

            //
            // Otherwise, there is nothing to do.
            //
            else
            {
                //
                // Assert the end of transmission "interrupt".
                //
                psUART->ui16IntStatus |= SOFTUART_INT_EOT;

                //
                // Advance to the idle state.
                //
                psUART->ui8TxState = SOFTUART_TXSTATE_IDLE;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the break state.
        //
        case SOFTUART_TXSTATE_BREAK:
        {
            //
            // See if the break should be deasserted.
            //
            if(!(psUART->ui8Flags & SOFTUART_FLAG_ENABLE) ||
               !(psUART->ui8Flags & SOFTUART_FLAG_TXBREAK))
            {
                //
                // The data line should be driven high to indicate it is idle.
                //
                psUART->ui8TxNext = 255;

                //
                // Advance to the idle state.
                //
                psUART->ui8TxState = SOFTUART_TXSTATE_IDLE;
            }

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
    // the UART peripheral.
    //
    while(((psUART->ui16IntStatus & psUART->ui16IntMask) != 0) &&
          (psUART->pfnIntCallback != 0))
    {
        //
        // Call the callback function.
        //
        psUART->pfnIntCallback();
    }
}

//*****************************************************************************
//
//! Handles the assertion of the receive ``interrupt''.
//!
//! \param psUART specifies the SoftUART data structure.
//!
//! This function is used to determine when to assert the receive ``interrupt''
//! as a result of writing data into the receive buffer (when characters are
//! received from the Rx pin).
//!
//! \return None.
//
//*****************************************************************************
static void
SoftUARTRxWriteInt(tSoftUART *psUART)
{
    uint32_t ui32Temp;

    //
    // Determine the number of characters in the receive buffer.
    //
    if(psUART->ui16RxBufferWrite > psUART->ui16RxBufferRead)
    {
        ui32Temp = psUART->ui16RxBufferWrite - psUART->ui16RxBufferRead;
    }
    else
    {
        ui32Temp = (psUART->ui16RxBufferLen + psUART->ui16RxBufferWrite -
                    psUART->ui16RxBufferRead);
    }

    //
    // If the receive buffer fullness just crossed the programmed level,
    // generate a receive "interrupt".
    //
    if(ui32Temp == psUART->ui16RxBufferLevel)
    {
        psUART->ui16IntStatus |= SOFTUART_INT_RX;
    }
}

//*****************************************************************************
//
//! Performs the periodic update of the SoftUART receiver.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param bEdgeInt should be \b true if this function is being called because
//! of a GPIO edge interrupt and \b false if it is being called because of a
//! timer interrupt.
//!
//! This function performs the periodic, time-based updates to the SoftUART
//! receiver.  The reception of data to the SoftUART is performed by the state
//! machine in this function.
//!
//! This function must be called by the GPIO interrupt handler, and then
//! periodically at the desired SoftUART baud rate.  For example, to run the
//! SoftUART at 115,200 baud, this function must be called at a 115,200 Hz
//! rate.
//!
//! \return Returns \b SOFTUART_RXTIMER_NOP if the receive timer should
//! continue to operate or \b SOFTUART_RXTIMER_END if it should be stopped.
//
//*****************************************************************************
uint32_t
SoftUARTRxTick(tSoftUART *psUART, bool bEdgeInt)
{
    uint32_t ui32PinState, ui32Temp, ui32Ret;

    //
    // Read the current state of the Rx data line.
    //
    ui32PinState = MAP_GPIOPinRead(psUART->ui32RxGPIOPort, psUART->ui8RxPin);

    //
    // The default return code inidicates that the receive timer does not need
    // to be stopped.
    //
    ui32Ret = SOFTUART_RXTIMER_NOP;

    //
    // See if this is an edge interrupt while delaying for the receive timeout
    // interrupt.
    //
    if(bEdgeInt && (psUART->ui8RxState == SOFTUART_RXSTATE_DELAY))
    {
        //
        // The receive timeout has been cancelled since the next character has
        // started, so go to the idle state.
        //
        psUART->ui8RxState = SOFTUART_RXSTATE_IDLE;
    }

    //
    // Determine the current state of the state machine.
    //
    switch(psUART->ui8RxState)
    {
        //
        // The state machine is idle.
        //
        case SOFTUART_RXSTATE_IDLE:
        {
            //
            // The falling edge of the start bit was just sampled, so disable
            // the GPIO edge interrupt since the remainder of the character
            // will be read using a timer tick.
            //
            GPIOIntClear(psUART->ui32RxGPIOPort, psUART->ui8RxPin);
            GPIOIntDisable(psUART->ui32RxGPIOPort, psUART->ui8RxPin);

            //
            // Clear the receive data buffer.
            //
            psUART->ui8RxData = 0;

            //
            // Clear all reception errors other than overrun (which is cleared
            // only when the first character after the overrun is written into
            // the receive buffer), and set the break error (which is cleared
            // if any non-zero bits are read during this character).
            //
            psUART->ui8RxFlags = ((psUART->ui8RxFlags & SOFTUART_RXFLAG_OE) |
                                  SOFTUART_RXFLAG_BE);

            //
            // Advance to the first data bit state.
            //
            psUART->ui8RxState = SOFTUART_RXSTATE_DATA_0;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In each of these states, a bit of the data byte is read.  This
        // depends upon RXSTATE_DATA_n and RXSTATE_DATA_(n+1) being
        // consecutively numbered.
        //
        case SOFTUART_RXSTATE_DATA_0:
        case SOFTUART_RXSTATE_DATA_1:
        case SOFTUART_RXSTATE_DATA_2:
        case SOFTUART_RXSTATE_DATA_3:
        {
            //
            // See if the Rx pin is high.
            //
            if(ui32PinState != 0)
            {
                //
                // Set this bit of the received character.
                //
                psUART->ui8RxData |= 1 << psUART->ui8RxState;

                //
                // Clear the break error since a non-zero bit was received.
                //
                psUART->ui8RxFlags &= ~(SOFTUART_RXFLAG_BE);
            }

            //
            // Advance to the next state.
            //
            psUART->ui8RxState++;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In each of these states, a bit of the data byte is read.
        // Additionally, based on the configuration of the SoftUART, this bit
        // might be the last bit of the data byte.  This depends upon
        // RXSTATE_DATA_n and RXSTATE_DATA_(n+1) being consecutively numbered.
        //
        case SOFTUART_RXSTATE_DATA_4:
        case SOFTUART_RXSTATE_DATA_5:
        case SOFTUART_RXSTATE_DATA_6:
        case SOFTUART_RXSTATE_DATA_7:
        {
            //
            // See if the Rx pin is high.
            //
            if(ui32PinState != 0)
            {
                //
                // Set this bit of the received character.
                //
                psUART->ui8RxData |= 1 << psUART->ui8RxState;

                //
                // Clear the break error since a non-zero bit was received.
                //
                psUART->ui8RxFlags &= ~(SOFTUART_RXFLAG_BE);
            }

            //
            // See if the bit that was just transferred is the last bit of the
            // data byte (based on the configuration of the SoftUART).
            //
            if(((psUART->ui16Config & SOFTUART_CONFIG_WLEN_MASK) >>
                SOFTUART_CONFIG_WLEN_S) ==
               (psUART->ui8RxState - SOFTUART_RXSTATE_DATA_4))
            {
                //
                // See if parity is enabled.
                //
                if((psUART->ui16Config & SOFTUART_CONFIG_PAR_MASK) !=
                   SOFTUART_CONFIG_PAR_NONE)
                {
                    //
                    // Advance to the parity state.
                    //
                    psUART->ui8RxState = SOFTUART_RXSTATE_PARITY;
                }

                //
                // Otherwise, see if there are one or two stop bits.
                //
                else if((psUART->ui16Config & SOFTUART_CONFIG_STOP_MASK) ==
                        SOFTUART_CONFIG_STOP_TWO)
                {
                    //
                    // Advance to the two stop bits state.
                    //
                    psUART->ui8RxState = SOFTUART_RXSTATE_STOP_0;
                }

                //
                // Otherwise, advance to the one stop bit state.
                //
                else
                {
                    psUART->ui8RxState = SOFTUART_RXSTATE_STOP_1;
                }
            }

            //
            // Otherwise, there are more bits to receive.
            //
            else
            {
                //
                // Advance to the next state.
                //
                psUART->ui8RxState++;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the parity bit state.
        //
        case SOFTUART_RXSTATE_PARITY:
        {
            //
            // See if the parity is set to one.
            //
            if((psUART->ui16Config & SOFTUART_CONFIG_PAR_MASK) ==
               SOFTUART_CONFIG_PAR_ONE)
            {
                //
                // Set the expected parity to one.
                //
                ui32Temp = psUART->ui8RxPin;
            }

            //
            // Otherwise, see if the parity is set to zero.
            //
            else if((psUART->ui16Config & SOFTUART_CONFIG_PAR_MASK) ==
                    SOFTUART_CONFIG_PAR_ZERO)
            {
                //
                // Set the expected parity to zero.
                //
                ui32Temp = 0;
            }

            //
            // Otherwise, there is either even or odd parity.
            //
            else
            {
                //
                // Find the odd parity for the data byte.
                //
                ui32Temp = ((g_pui32ParityOdd[psUART->ui8RxData >> 5] &
                             (1 << (psUART->ui8RxData & 31))) ?
                            psUART->ui8RxPin : 0);

                //
                // If the parity is set to even, then invert the parity just
                // computed (making it even parity).
                //
                if((psUART->ui16Config & SOFTUART_CONFIG_PAR_MASK) ==
                   SOFTUART_CONFIG_PAR_EVEN)
                {
                    ui32Temp ^= psUART->ui8RxPin;
                }
            }

            //
            // See if the pin state matches the expected parity.
            //
            if(ui32PinState != ui32Temp)
            {
                //
                // The parity does not match, so set the parity error flag.
                //
                psUART->ui8RxFlags |= SOFTUART_RXFLAG_PE;
            }

            //
            // See if the Rx pin is high.
            //
            if(ui32PinState != 0)
            {
                //
                // Clear the break error since a non-zero bit was received.
                //
                psUART->ui8RxFlags &= ~(SOFTUART_RXFLAG_BE);
            }

            //
            // See if there are one or two stop bits.
            //
            if((psUART->ui16Config & SOFTUART_CONFIG_STOP_MASK) ==
               SOFTUART_CONFIG_STOP_TWO)
            {
                //
                // Advance to the two stop bits state.
                //
                psUART->ui8RxState = SOFTUART_RXSTATE_STOP_0;
            }
            else
            {
                //
                // Advance to the one stop bit state.
                //
                psUART->ui8RxState = SOFTUART_RXSTATE_STOP_1;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the two stop bits state.
        //
        case SOFTUART_RXSTATE_STOP_0:
        {
            //
            // See if the Rx pin is low.
            //
            if(ui32PinState == 0)
            {
                //
                // Since the Rx pin is low, there is a framing error.
                //
                psUART->ui8RxFlags |= SOFTUART_RXFLAG_FE;
            }
            else
            {
                //
                // Clear the break error since a non-zero bit was received.
                //
                psUART->ui8RxFlags &= ~(SOFTUART_RXFLAG_BE);
            }

            //
            // Advance to the one stop bit state.
            //
            psUART->ui8RxState = SOFTUART_RXSTATE_STOP_1;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the one stop bit state.
        //
        case SOFTUART_RXSTATE_STOP_1:
        {
            //
            // See if the Rx pin is low.
            //
            if(ui32PinState == 0)
            {
                //
                // Since the Rx pin is low, there is a framing error.
                //
                psUART->ui8RxFlags |= SOFTUART_RXFLAG_FE;
            }
            else
            {
                //
                // Clear the break error since a non-zero bit was received.
                //
                psUART->ui8RxFlags &= ~(SOFTUART_RXFLAG_BE);
            }

            //
            // See if the break error is still asserted (meaning that every bit
            // received was zero).
            //
            if(psUART->ui8RxFlags & SOFTUART_RXFLAG_BE)
            {
                //
                // Since every bit was zero, advance to the break state.
                //
                psUART->ui8RxState = SOFTUART_RXSTATE_BREAK;

                //
                // This state has been handled.
                //
                break;
            }

            //
            // Compute the value of the write pointer advanced by one.
            //
            ui32Temp = psUART->ui16RxBufferWrite + 1;
            if(ui32Temp == psUART->ui16RxBufferLen)
            {
                ui32Temp = 0;
            }

            //
            // See if there is space in the receive buffer.
            //
            if(ui32Temp == psUART->ui16RxBufferRead)
            {
                //
                // Set the overrun error flag.  This will remain set until a
                // new character can be placed into the receive buffer, which
                // will then be given this status.
                //
                psUART->ui8RxFlags |= SOFTUART_RXFLAG_OE;

                //
                // Set the receive overrun "interrupt" and status if it is not
                // already set.
                //
                if(!(psUART->ui8RxStatus & SOFTUART_RXERROR_OVERRUN))
                {
                    psUART->ui8RxStatus |= SOFTUART_RXERROR_OVERRUN;
                    psUART->ui16IntStatus |= SOFTUART_INT_OE;
                }
            }

            //
            // Otherwise, there is space in the receive buffer.
            //
            else
            {
                //
                // Write this data byte, along with the receive flags, into the
                // receive buffer.
                //
                psUART->pui16RxBuffer[psUART->ui16RxBufferWrite] =
                    psUART->ui8RxData | (psUART->ui8RxFlags << 8);

                //
                // Advance the write pointer.
                //
                psUART->ui16RxBufferWrite = ui32Temp;

                //
                // Clear the receive flags, most importantly the overrun flag
                // since it was just written into the receive buffer.
                //
                psUART->ui8RxFlags = 0;

                //
                // Assert the receive "interrupt" if appropriate.
                //
                SoftUARTRxWriteInt(psUART);
            }

            //
            // See if this character had a parity error.
            //
            if(psUART->ui8RxFlags & SOFTUART_RXFLAG_PE)
            {
                //
                // Assert the parity error "interrupt".
                //
                psUART->ui16IntStatus |= SOFTUART_INT_PE;
            }

            //
            // See if this character had a framing error.
            //
            if(psUART->ui8RxFlags & SOFTUART_RXFLAG_FE)
            {
                //
                // Assert the framing error "interrupt".
                //
                psUART->ui16IntStatus |= SOFTUART_INT_FE;
            }

            //
            // Enable the falling edge interrupt on the Rx pin so that the next
            // start bit can be detected.
            //
            GPIOIntClear(psUART->ui32RxGPIOPort, psUART->ui8RxPin);
            GPIOIntEnable(psUART->ui32RxGPIOPort, psUART->ui8RxPin);

            //
            // Advance to the receive timeout delay state.
            //
            psUART->ui8RxData = 0;
            psUART->ui8RxState = SOFTUART_RXSTATE_DELAY;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the break state.
        //
        case SOFTUART_RXSTATE_BREAK:
        {
            //
            // See if the Rx pin is high.
            //
            if(ui32PinState != 0)
            {
                //
                // Clear the break error since a non-zero bit was received.
                //
                psUART->ui8RxFlags &= ~(SOFTUART_RXFLAG_BE);
            }

            //
            // Compute the value of the write pointer advanced by one.
            //
            ui32Temp = psUART->ui16RxBufferWrite + 1;
            if(ui32Temp == psUART->ui16RxBufferLen)
            {
                ui32Temp = 0;
            }

            //
            // See if there is space in the receive buffer.
            //
            if(ui32Temp == psUART->ui16RxBufferRead)
            {
                //
                // Set the overrun error flag.  This will remain set until a
                // new character can be placed into the receive buffer, which
                // will then be given this status.
                //
                psUART->ui8RxFlags |= SOFTUART_RXFLAG_OE;

                //
                // Set the receive overrun "interrupt" and status if it is not
                // already set.
                //
                if(!(psUART->ui8RxStatus & SOFTUART_RXERROR_OVERRUN))
                {
                    psUART->ui8RxStatus |= SOFTUART_RXERROR_OVERRUN;
                    psUART->ui16IntStatus |= SOFTUART_INT_OE;
                }
            }

            //
            // Otherwise, there is space in the receive buffer.
            //
            else
            {
                //
                // Write this data byte, along with the receive flags, into the
                // receive buffer.
                //
                psUART->pui16RxBuffer[psUART->ui16RxBufferWrite] =
                    psUART->ui8RxData | (psUART->ui8RxFlags << 8);

                //
                // Advance the write pointer.
                //
                psUART->ui16RxBufferWrite = ui32Temp;

                //
                // Clear the receive flags, most importantly the overrun flag
                // since it was just written into the receive buffer.
                //
                psUART->ui8RxFlags = 0;

                //
                // Assert the receive "interrupt" if appropriate.
                //
                SoftUARTRxWriteInt(psUART);
            }

            //
            // See if this was a break error.
            //
            if(psUART->ui8RxFlags & SOFTUART_RXFLAG_BE)
            {
                //
                // Assert the break error "interrupt".
                //
                psUART->ui16IntStatus |= SOFTUART_INT_BE;
            }

            //
            // See if this character had a parity error.
            //
            if(psUART->ui8RxFlags & SOFTUART_RXFLAG_PE)
            {
                //
                // Assert the parity error "interrupt".
                //
                psUART->ui16IntStatus |= SOFTUART_INT_PE;
            }

            //
            // Assert the framing error "interrupt".
            //
            psUART->ui16IntStatus |= SOFTUART_INT_FE;

            //
            // Enable the falling edge interrupt on the Rx pin so that the next
            // start bit can be detected.
            //
            GPIOIntClear(psUART->ui32RxGPIOPort, psUART->ui8RxPin);
            GPIOIntEnable(psUART->ui32RxGPIOPort, psUART->ui8RxPin);

            //
            // Advance to the receive timeout delay state.
            //
            psUART->ui8RxData = 0;
            psUART->ui8RxState = SOFTUART_RXSTATE_DELAY;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the receive timeout delay state.
        //
        case SOFTUART_RXSTATE_DELAY:
        {
            //
            // See if the receive timeout has expired.
            //
            if(psUART->ui8RxData++ == 32)
            {
                //
                // Assert the receive timeout "interrupt".
                //
                psUART->ui16IntStatus |= SOFTUART_INT_RT;

                //
                // Tell the caller that the receive timer can be disabled.
                //
                ui32Ret = SOFTUART_RXTIMER_END;
            }

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
    // the UART peripheral.
    //
    while(((psUART->ui16IntStatus & psUART->ui16IntMask) != 0) &&
          (psUART->pfnIntCallback != 0))
    {
        //
        // Call the callback function.
        //
        psUART->pfnIntCallback();
    }

    //
    // Return to the caller.
    //
    return(ui32Ret);
}

//*****************************************************************************
//
//! Sets the type of parity.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param ui32Parity specifies the type of parity to use.
//!
//! Sets the type of parity to use for transmitting and expect when receiving.
//! The \e ui32Parity parameter must be one of \b SOFTUART_CONFIG_PAR_NONE,
//! \b SOFTUART_CONFIG_PAR_EVEN, \b SOFTUART_CONFIG_PAR_ODD,
//! \b SOFTUART_CONFIG_PAR_ONE, or \b SOFTUART_CONFIG_PAR_ZERO.  The last two
//! allow direct control of the parity bit; it is always either one or zero
//! based on the mode.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTParityModeSet(tSoftUART *psUART, uint32_t ui32Parity)
{
    //
    // Check the arguments.
    //
    ASSERT((ui32Parity == SOFTUART_CONFIG_PAR_NONE) ||
           (ui32Parity == SOFTUART_CONFIG_PAR_EVEN) ||
           (ui32Parity == SOFTUART_CONFIG_PAR_ODD) ||
           (ui32Parity == SOFTUART_CONFIG_PAR_ONE) ||
           (ui32Parity == SOFTUART_CONFIG_PAR_ZERO));

    //
    // Set the parity mode.
    //
    psUART->ui16Config =
        (psUART->ui16Config & SOFTUART_CONFIG_PAR_MASK) | ui32Parity;
}

//*****************************************************************************
//
//! Gets the type of parity currently being used.
//!
//! \param psUART specifies the SoftUART data structure.
//!
//! This function gets the type of parity used for transmitting data and
//! expected when receiving data.
//!
//! \return Returns the current parity settings, specified as one of
//! \b SOFTUART_CONFIG_PAR_NONE, \b SOFTUART_CONFIG_PAR_EVEN,
//! \b SOFTUART_CONFIG_PAR_ODD, \b SOFTUART_CONFIG_PAR_ONE, or
//! \b SOFTUART_CONFIG_PAR_ZERO.
//
//*****************************************************************************
uint32_t
SoftUARTParityModeGet(tSoftUART *psUART)
{
    //
    // Return the current parity setting.
    //
    return(psUART->ui16Config & SOFTUART_CONFIG_PAR_MASK);
}

//*****************************************************************************
//
//! Sets the transmit ``interrupt'' buffer level.
//!
//! \param psUART specifies the soft UART data structure.
//!
//! This function computes the transmit buffer level at which the transmit
//! ``interrupt'' is generated.
//!
//! \return None.
//
//*****************************************************************************
static void
SoftUARTTxLevelSet(tSoftUART *psUART)
{
    //
    // Determine the transmit buffer "interrupt" fullness setting.
    //
    switch(psUART->ui16Config & SOFTUART_CONFIG_TXLVL_M)
    {
        //
        // The transmit "interrupt" should be generated when the buffer is 1/8
        // full.
        //
        case SOFTUART_CONFIG_TXLVL_1:
        {
            //
            // Set the transmit buffer level to 1/8 of the buffer length.
            //
            psUART->ui16TxBufferLevel = psUART->ui16TxBufferLen / 8;

            //
            // This setting has been handled.
            //
            break;
        }

        //
        // The transmit "interrupt" should be generated when the buffer is 1/4
        // (2/8) full.
        //
        case SOFTUART_CONFIG_TXLVL_2:
        {
            //
            // Set the transmit buffer level to 1/4 of the buffer length.
            //
            psUART->ui16TxBufferLevel = psUART->ui16TxBufferLen / 4;

            //
            // This setting has been handled.
            //
            break;
        }

        //
        // The transmit "interrupt" should be generated when the buffer is 1/2
        // (4/8) full.
        //
        case SOFTUART_CONFIG_TXLVL_4:
        {
            //
            // Set the transmit buffer level to 1/2 of the buffer length.
            //
            psUART->ui16TxBufferLevel = psUART->ui16TxBufferLen / 2;

            //
            // This setting has been handled.
            //
            break;
        }

        //
        // The transmit "interrupt" should be generated when the buffer is 3/4
        // (6/8) full.
        //
        case SOFTUART_CONFIG_TXLVL_6:
        {
            //
            // Set the transmit buffer level to 3/4 of the buffer length.
            //
            psUART->ui16TxBufferLevel = (psUART->ui16TxBufferLen * 3) / 4;

            //
            // This setting has been handled.
            //
            break;
        }

        //
        // The transmit "interrupt" should be generated when the buffer is 7/8
        // full.
        //
        case SOFTUART_CONFIG_TXLVL_7:
        {
            //
            // Set the transmit buffer level to 7/8 of the buffer length.
            //
            psUART->ui16TxBufferLevel = (psUART->ui16TxBufferLen * 7) / 8;

            //
            // This setting has been handled.
            //
            break;
        }
    }
}

//*****************************************************************************
//
//! Sets the receive ``interrupt'' buffer level.
//!
//! \param psUART specifies the soft UART data structure.
//!
//! This function computes the receive buffer level at which the receive
//! ``interrupt'' is generated.
//!
//! \return None.
//
//*****************************************************************************
static void
SoftUARTRxLevelSet(tSoftUART *psUART)
{
    //
    // Determine the receive buffer "interrupt" fullness setting.
    //
    switch(psUART->ui16Config & SOFTUART_CONFIG_RXLVL_M)
    {
        //
        // The receive "interrupt" should be generated when the buffer is 1/8
        // full.
        //
        case SOFTUART_CONFIG_RXLVL_1:
        {
            //
            // Set the receive buffer level to 1/8 of the buffer length.
            //
            psUART->ui16RxBufferLevel = psUART->ui16RxBufferLen / 8;

            //
            // This setting has been handled.
            //
            break;
        }

        //
        // The receive "interrupt" should be generated when the buffer is 1/4
        // (2/8) full.
        //
        case SOFTUART_CONFIG_RXLVL_2:
        {
            //
            // Set the receive buffer level to 1/4 of the buffer length.
            //
            psUART->ui16RxBufferLevel = psUART->ui16RxBufferLen / 4;

            //
            // This setting has been handled.
            //
            break;
        }

        //
        // The receive "interrupt" should be generated when the buffer is 1/2
        // (4/8) full.
        //
        case SOFTUART_CONFIG_RXLVL_4:
        {
            //
            // Set the receive buffer level to 1/2 of the buffer length.
            //
            psUART->ui16RxBufferLevel = psUART->ui16RxBufferLen / 2;

            //
            // This setting has been handled.
            //
            break;
        }

        //
        // The receive "interrupt" should be generated when the buffer is 3/4
        // (6/8) full.
        //
        case SOFTUART_CONFIG_RXLVL_6:
        {
            //
            // Set the receive buffer level to 3/4 of the buffer length.
            //
            psUART->ui16RxBufferLevel = (psUART->ui16RxBufferLen * 3) / 4;

            //
            // This setting has been handled.
            //
            break;
        }

        //
        // The receive "interrupt" should be generated when the buffer is 7/8
        // full.
        //
        case SOFTUART_CONFIG_RXLVL_7:
        {
            //
            // Set the receive buffer level to 7/8 of the buffer length.
            //
            psUART->ui16RxBufferLevel = (psUART->ui16RxBufferLen * 7) / 8;

            //
            // This setting has been handled.
            //
            break;
        }
    }
}

//*****************************************************************************
//
//! Sets the buffer level at which ``interrupts'' are generated.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param ui32TxLevel is the transmit buffer ``interrupt'' level, specified as
//! one of \b UART_FIFO_TX1_8, \b UART_FIFO_TX2_8, \b UART_FIFO_TX4_8,
//! \b UART_FIFO_TX6_8, or \b UART_FIFO_TX7_8.
//! \param ui32RxLevel is the receive buffer ``interrupt'' level, specified as
//! one of \b UART_FIFO_RX1_8, \b UART_FIFO_RX2_8, \b UART_FIFO_RX4_8,
//! \b UART_FIFO_RX6_8, or \b UART_FIFO_RX7_8.
//!
//! This function sets the buffer level at which transmit and receive
//! ``interrupts'' are generated.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTFIFOLevelSet(tSoftUART *psUART, uint32_t ui32TxLevel,
                     uint32_t ui32RxLevel)
{
    //
    // Check the arguments.
    //
    ASSERT((ui32TxLevel == SOFTUART_FIFO_TX1_8) ||
           (ui32TxLevel == SOFTUART_FIFO_TX2_8) ||
           (ui32TxLevel == SOFTUART_FIFO_TX4_8) ||
           (ui32TxLevel == SOFTUART_FIFO_TX6_8) ||
           (ui32TxLevel == SOFTUART_FIFO_TX7_8));
    ASSERT((ui32RxLevel == SOFTUART_FIFO_RX1_8) ||
           (ui32RxLevel == SOFTUART_FIFO_RX2_8) ||
           (ui32RxLevel == SOFTUART_FIFO_RX4_8) ||
           (ui32RxLevel == SOFTUART_FIFO_RX6_8) ||
           (ui32RxLevel == SOFTUART_FIFO_RX7_8));

    //
    // Save the buffer "interrupt" levels.
    //
    psUART->ui16Config = ((psUART->ui16Config & SOFTUART_CONFIG_BASE_M) |
                          ((ui32TxLevel | ui32RxLevel) << 8));

    //
    // Compute the new buffer "interrupt" levels.
    //
    SoftUARTTxLevelSet(psUART);
    SoftUARTRxLevelSet(psUART);
}

//*****************************************************************************
//
//! Gets the buffer level at which ``interrupts'' are generated.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param pui32TxLevel is a pointer to storage for the transmit buffer level,
//! returned as one of \b UART_FIFO_TX1_8, \b UART_FIFO_TX2_8,
//! \b UART_FIFO_TX4_8, \b UART_FIFO_TX6_8, or \b UART_FIFO_TX7_8.
//! \param pui32RxLevel is a pointer to storage for the receive buffer level,
//! returned as one of \b UART_FIFO_RX1_8, \b UART_FIFO_RX2_8,
//! \b UART_FIFO_RX4_8, \b UART_FIFO_RX6_8, or \b UART_FIFO_RX7_8.
//!
//! This function gets the buffer level at which transmit and receive
//! ``interrupts'' are generated.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTFIFOLevelGet(tSoftUART *psUART, uint32_t *pui32TxLevel,
                     uint32_t *pui32RxLevel)
{
    //
    // Extract the transmit and receive buffer levels.
    //
    *pui32TxLevel = (psUART->ui16Config & SOFTUART_CONFIG_TXLVL_M) >> 8;
    *pui32RxLevel = (psUART->ui16Config & SOFTUART_CONFIG_RXLVL_M) >> 8;
}

//*****************************************************************************
//
//! Gets the current configuration of a UART.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param pui32Config is a pointer to storage for the data format.
//!
//! Returns the data format of the SoftUART.  The data format returned in
//! \e pui32Config is enumerated the same as the \e ui32Config parameter of
//! SoftUARTConfigSet().
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTConfigGet(tSoftUART *psUART, uint32_t *pui32Config)
{
    //
    // Get the data format.
    //
    *pui32Config = psUART->ui16Config & SOFTUART_CONFIG_BASE_M;
}

//*****************************************************************************
//
//! Enables the SoftUART.
//!
//! \param psUART specifies the SoftUART data structure.
//!
//! This function enables the SoftUART, allowing data to be transmitted and
//! received.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTEnable(tSoftUART *psUART)
{
    //
    // Enable the SoftUART.
    //
    psUART->ui8Flags |= SOFTUART_FLAG_ENABLE;
}

//*****************************************************************************
//
//! Disables the SoftUART.
//!
//! \param psUART specifies the SoftUART data structure.
//!
//! This function disables the SoftUART after waiting for it to become idle.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTDisable(tSoftUART *psUART)
{
    //
    // Wait for end of TX.
    //
    while(SoftUARTBusy(psUART))
    {
    }

    //
    // Disable the SoftUART.
    //
    psUART->ui8Flags &= ~(SOFTUART_FLAG_ENABLE);
}

//*****************************************************************************
//
//! Determines if there are any characters in the receive buffer.
//!
//! \param psUART specifies the SoftUART data structure.
//!
//! This function returns a flag indicating whether or not there is data
//! available in the receive buffer.
//!
//! \return Returns \b true if there is data in the receive buffer or \b false
//! if there is no data in the receive buffer.
//
//*****************************************************************************
bool
SoftUARTCharsAvail(tSoftUART *psUART)
{
    //
    // Return the availability of characters.
    //
    return((psUART->ui16RxBufferRead == psUART->ui16RxBufferWrite) ? false :
           true);
}

//*****************************************************************************
//
//! Determines if there is any space in the transmit buffer.
//!
//! \param psUART specifies the SoftUART data structure.
//!
//! This function returns a flag indicating whether or not there is space
//! available in the transmit buffer.
//!
//! \return Returns \b true if there is space available in the transmit buffer
//! or \b false if there is no space available in the transmit buffer.
//
//*****************************************************************************
bool
SoftUARTSpaceAvail(tSoftUART *psUART)
{
    uint16_t ui16Temp;

    //
    // Determine the values of the write pointer once incremented.
    //
    ui16Temp = psUART->ui16TxBufferWrite + 1;
    if(ui16Temp == psUART->ui16TxBufferLen)
    {
        ui16Temp = 0;
    }

    //
    // Return the availability of space.
    //
    return((psUART->ui16TxBufferRead == ui16Temp) ? false : true);
}

//*****************************************************************************
//
//! Handles the deassertion of the receive ``interrupts''.
//!
//! \param psUART specifies the SoftUART data structure.
//!
//! This function is used to determine when to deassert the receive
//! ``interrupt'' as a result of reading data from the receive buffer.
//!
//! \return None.
//
//*****************************************************************************
static void
SoftUARTRxReadInt(tSoftUART *psUART)
{
    uint32_t ui32Temp;

    //
    // Determine the number of characters in the receive buffer.
    //
    if(psUART->ui16RxBufferWrite > psUART->ui16RxBufferRead)
    {
        ui32Temp = psUART->ui16RxBufferWrite - psUART->ui16RxBufferRead;
    }
    else
    {
        ui32Temp = (psUART->ui16RxBufferLen + psUART->ui16RxBufferWrite -
                    psUART->ui16RxBufferRead);
    }

    //
    // See if the number of characters in the receive buffer have dropped below
    // the receive trigger level.
    //
    if(ui32Temp < psUART->ui16RxBufferLevel)
    {
        //
        // Deassert the receive "interrupt".
        //
        psUART->ui16IntStatus &= ~(SOFTUART_INT_RX);
    }

    //
    // See if the receive buffer is now empty.
    //
    if(ui32Temp == 0)
    {
        //
        // Deassert the receive timeout "interrupt".
        //
        psUART->ui16IntStatus &= ~(SOFTUART_INT_RT);
    }
}

//*****************************************************************************
//
//! Receives a character from the specified port.
//!
//! \param psUART specifies the SoftUART data structure.
//!
//! Gets a character from the receive buffer for the specified port.
//!
//! \return Returns the character read from the specified port, cast as a
//! \e int32_t.  A \b -1 is returned if there are no characters present in the
//! receive buffer.  The SoftUARTCharsAvail() function should be called before
//! attempting to call this function.
//
//*****************************************************************************
int32_t
SoftUARTCharGetNonBlocking(tSoftUART *psUART)
{
    int32_t i32Temp;

    //
    // See if there are any characters in the receive buffer.
    //
    if(psUART->ui16RxBufferRead != psUART->ui16RxBufferWrite)
    {
        //
        // Read the next character.
        //
        i32Temp = psUART->pui16RxBuffer[psUART->ui16RxBufferRead];
        psUART->ui16RxBufferRead++;
        if(psUART->ui16RxBufferRead == psUART->ui16RxBufferLen)
        {
            psUART->ui16RxBufferRead = 0;
        }

        //
        // Deassert the receive "interrupt(s)" if appropriate.
        //
        SoftUARTRxReadInt(psUART);

        //
        // Set the receive status to match this character.
        //
        psUART->ui8RxStatus =
            ((psUART->ui8RxStatus & SOFTUART_RXERROR_OVERRUN) |
             ((i32Temp >> 8) & ~(SOFTUART_RXERROR_OVERRUN)));

        //
        // Return this character.
        //
        return(i32Temp);
    }
    else
    {
        //
        // There are no characters, so return a failure.
        //
        return(-1);
    }
}

//*****************************************************************************
//
//! Waits for a character from the specified port.
//!
//! \param psUART specifies the SoftUART data structure.
//!
//! Gets a character from the receive buffer for the specified port.  If there
//! are no characters available, this function waits until a character is
//! received before returning.
//!
//! \return Returns the character read from the specified port, cast as a
//! \e int32_t.
//
//*****************************************************************************
int32_t
SoftUARTCharGet(tSoftUART *psUART)
{
    int32_t i32Temp;

    //
    // Wait until a int8_t is available.
    //
    while(psUART->ui16RxBufferRead ==
          *(volatile uint16_t *)(&(psUART->ui16RxBufferWrite)))
    {
    }

    //
    // Read the next character.
    //
    i32Temp = psUART->pui16RxBuffer[psUART->ui16RxBufferRead];
    psUART->ui16RxBufferRead++;
    if(psUART->ui16RxBufferRead == psUART->ui16RxBufferLen)
    {
        psUART->ui16RxBufferRead = 0;
    }

    //
    // Deassert the receive "interrupt(s)" if appropriate.
    //
    SoftUARTRxReadInt(psUART);

    //
    // Set the receive status to match this character.
    //
    psUART->ui8RxStatus = ((psUART->ui8RxStatus & SOFTUART_RXERROR_OVERRUN) |
                           ((i32Temp >> 8) & ~(SOFTUART_RXERROR_OVERRUN)));

    //
    // Return this character.
    //
    return(i32Temp);
}

//*****************************************************************************
//
//! Sends a character to the specified port.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param ui8Data is the character to be transmitted.
//!
//! Writes the character \e ui8Data to the transmit buffer for the specified
//! port.  This function does not block, so if there is no space available,
//! then a \b false is returned, and the application must retry the function
//! later.
//!
//! \return Returns \b true if the character was successfully placed in the
//! transmit buffer or \b false if there was no space available in the
//! transmit buffer.
//
//*****************************************************************************
bool
SoftUARTCharPutNonBlocking(tSoftUART *psUART, uint8_t ui8Data)
{
    uint16_t ui16Temp;

    //
    // Determine the values of the write pointer once incremented.
    //
    ui16Temp = psUART->ui16TxBufferWrite + 1;
    if(ui16Temp == psUART->ui16TxBufferLen)
    {
        ui16Temp = 0;
    }

    //
    // See if there is space in the transmit buffer.
    //
    if(ui16Temp != psUART->ui16TxBufferRead)
    {
        //
        // Write this character to the transmit buffer.
        //
        psUART->pui8TxBuffer[psUART->ui16TxBufferWrite] = ui8Data;
        psUART->ui16TxBufferWrite = ui16Temp;

        //
        // Success.
        //
        return(true);
    }
    else
    {
        //
        // There is no space in the transmit buffer, so return a failure.
        //
        return(false);
    }
}

//*****************************************************************************
//
//! Waits to send a character from the specified port.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param ui8Data is the character to be transmitted.
//!
//! Sends the character \e ui8Data to the transmit buffer for the specified
//! port.  If there is no space available in the transmit buffer, this function
//! waits until there is space available before returning.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTCharPut(tSoftUART *psUART, uint8_t ui8Data)
{
    uint16_t ui16Temp;

    //
    // Wait until space is available.
    //
    ui16Temp = psUART->ui16TxBufferWrite + 1;
    if(ui16Temp == psUART->ui16TxBufferLen)
    {
        ui16Temp = 0;
    }
    while(ui16Temp == *(volatile uint16_t *)(&(psUART->ui16TxBufferRead)))
    {
    }

    //
    // Send the int8_t.
    //
    psUART->pui8TxBuffer[psUART->ui16TxBufferWrite] = ui8Data;
    psUART->ui16TxBufferWrite = ui16Temp;
}

//*****************************************************************************
//
//! Causes a BREAK to be sent.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param bBreakState controls the output level.
//!
//! Calling this function with \e bBreakState set to \b true asserts a break
//! condition on the SoftUART.  Calling this function with \e bBreakState set
//! to \b false removes the break condition.  For proper transmission of a
//! break command, the break must be asserted for at least two complete frames.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTBreakCtl(tSoftUART *psUART, bool bBreakState)
{
    //
    // Set the break condition as requested.
    //
    if(bBreakState)
    {
        psUART->ui8Flags |= SOFTUART_FLAG_TXBREAK;
    }
    else
    {
        psUART->ui8Flags &= ~(SOFTUART_FLAG_TXBREAK);
    }
}

//*****************************************************************************
//
//! Determines whether the UART transmitter is busy or not.
//!
//! \param psUART specifies the SoftUART data structure.
//!
//! Allows the caller to determine whether all transmitted bytes have cleared
//! the transmitter hardware.  If \b false is returned, the transmit buffer is
//! empty and all bits of the last transmitted character, including all stop
//! bits, have left the hardware shift register.
//!
//! \return Returns \b true if the UART is transmitting or \b false if all
//! transmissions are complete.
//
//*****************************************************************************
bool
SoftUARTBusy(tSoftUART *psUART)
{
    //
    // Determine if the UART is busy.
    //
    return(((psUART->ui8TxState == SOFTUART_TXSTATE_IDLE) &&
            (((psUART->ui8Flags & SOFTUART_FLAG_ENABLE) == 0) ||
             (psUART->ui16TxBufferRead == psUART->ui16TxBufferWrite))) ?
           false : true);
}

//*****************************************************************************
//
//! Enables individual SoftUART ``interrupt'' sources.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param ui32IntFlags is the bit mask of the ``interrupt'' sources to be
//! enabled.
//!
//! Enables the indicated SoftUART ``interrupt'' sources.  Only the sources
//! that are enabled can be reflected to the SoftUART callback.
//!
//! The \e ui32IntFlags parameter is the logical OR of any of the following:
//!
//! - \b SOFTUART_INT_OE - Overrun Error ``interrupt''
//! - \b SOFTUART_INT_BE - Break Error ``interrupt''
//! - \b SOFTUART_INT_PE - Parity Error ``interrupt''
//! - \b SOFTUART_INT_FE - Framing Error ``interrupt''
//! - \b SOFTUART_INT_RT - Receive Timeout ``interrupt''
//! - \b SOFTUART_INT_TX - Transmit ``interrupt''
//! - \b SOFTUART_INT_RX - Receive ``interrupt''
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTIntEnable(tSoftUART *psUART, uint32_t ui32IntFlags)
{
    //
    // Enable the specified interrupts.
    //
    psUART->ui16IntMask |= ui32IntFlags;
}

//*****************************************************************************
//
//! Disables individual SoftUART ``interrupt'' sources.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param ui32IntFlags is the bit mask of the ``interrupt'' sources to be
//! disabled.
//!
//! Disables the indicated SoftUART ``interrupt'' sources.  Only the sources
//! that are enabled can be reflected to the SoftUART callback.
//!
//! The \e ui32IntFlags parameter has the same definition as the
//! \e ui32IntFlags parameter to SoftUARTIntEnable().
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTIntDisable(tSoftUART *psUART, uint32_t ui32IntFlags)
{
    //
    // Disable the specified interrupts.
    //
    psUART->ui16IntMask &= ~(ui32IntFlags);
}

//*****************************************************************************
//
//! Gets the current SoftUART ``interrupt'' status.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param bMasked is \b false if the raw ``interrupt'' status is required and
//! \b true if the masked ``interrupt'' status is required.
//!
//! This returns the ``interrupt'' status for the SoftUART.  Either the raw
//! ``interrupt'' status or the status of ``interrupts'' that are allowed to
//! reflect to the SoftUART callback can be returned.
//!
//! \return Returns the current ``interrupt'' status, enumerated as a bit field
//! of values described in SoftUARTIntEnable().
//
//*****************************************************************************
uint32_t
SoftUARTIntStatus(tSoftUART *psUART, bool bMasked)
{
    //
    // Return either the interrupt status or the raw interrupt status as
    // requested.
    //
    if(bMasked)
    {
        return(psUART->ui16IntStatus & psUART->ui16IntMask);
    }
    else
    {
        return(psUART->ui16IntStatus);
    }
}

//*****************************************************************************
//
//! Clears SoftUART ``interrupt'' sources.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param ui32IntFlags is a bit mask of the ``interrupt'' sources to be
//! cleared.
//!
//! The specified SoftUART ``interrupt'' sources are cleared, so that they no
//! longer assert.  This function must be called in the callback function to
//! keep the ``interrupt'' from being recognized again immediately upon exit.
//!
//! The \e ui32IntFlags parameter has the same definition as the
//! \e ui32IntFlags parameter to SoftUARTIntEnable().
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTIntClear(tSoftUART *psUART, uint32_t ui32IntFlags)
{
    //
    // Clear the requested interrupt sources.
    //
    psUART->ui16IntStatus &= ~(ui32IntFlags);
}

//*****************************************************************************
//
//! Gets current receiver errors.
//!
//! \param psUART specifies the SoftUART data structure.
//!
//! This function returns the current state of each of the 4 receiver error
//! sources.  The returned errors are equivalent to the four error bits
//! returned via the previous call to SoftUARTCharGet() or
//! SoftUARTCharGetNonBlocking() with the exception that the overrun error is
//! set immediately when the overrun occurs rather than when a character is
//! next read.
//!
//! \return Returns a logical OR combination of the receiver error flags,
//! \b SOFTUART_RXERROR_FRAMING, \b SOFTUART_RXERROR_PARITY,
//! \b SOFTUART_RXERROR_BREAK and \b SOFTUART_RXERROR_OVERRUN.
//
//*****************************************************************************
uint32_t
SoftUARTRxErrorGet(tSoftUART *psUART)
{
    //
    // Return the current value of the receive status.
    //
    return(psUART->ui8RxStatus);
}

//*****************************************************************************
//
//! Clears all reported receiver errors.
//!
//! \param psUART specifies the SoftUART data structure.
//!
//! This function is used to clear all receiver error conditions reported via
//! SoftUARTRxErrorGet().  If using the overrun, framing error, parity error or
//! break interrupts, this function must be called after clearing the interrupt
//! to ensure that later errors of the same type trigger another interrupt.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTRxErrorClear(tSoftUART *psUART)
{
    //
    // Clear any receive error status.
    //
    psUART->ui8RxStatus = 0;
}

//*****************************************************************************
//
//! Sets the callback used by the SoftUART module.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param pfnCallback is a pointer to the callback function.
//!
//! This function sets the address of the callback function that is called when
//! there is an ``interrupt'' produced by the SoftUART module.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTCallbackSet(tSoftUART *psUART, void (*pfnCallback)(void))
{
    //
    // Save the callback function address.
    //
    psUART->pfnIntCallback = pfnCallback;
}

//*****************************************************************************
//
//! Sets the GPIO pin to be used as the SoftUART Tx signal.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param ui32Base is the base address of the GPIO module.
//! \param ui8Pin is the bit-packed representation of the pin to use.
//!
//! This function sets the GPIO pin that is used when the SoftUART must assert
//! the Tx signal.
//!
//! The pin is specified using a bit-packed byte, where bit 0 of the byte
//! represents GPIO port pin 0, bit 1 represents GPIO port pin 1, and so on.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTTxGPIOSet(tSoftUART *psUART, uint32_t ui32Base, uint8_t ui8Pin)
{
    //
    // Save the base address and pin for the Tx signal.
    //
    if(ui32Base == 0)
    {
        psUART->ui32TxGPIO = 0;
    }
    else
    {
        psUART->ui32TxGPIO = ui32Base + (ui8Pin << 2);
    }
}

//*****************************************************************************
//
//! Sets the GPIO pin to be used as the SoftUART Rx signal.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param ui32Base is the base address of the GPIO module.
//! \param ui8Pin is the bit-packed representation of the pin to use.
//!
//! This function sets the GPIO pin that is used when the SoftUART must sample
//! the Rx signal.  If there is not a GPIO pin allocated for Rx, the SoftUART
//! module will not read data from the slave device.
//!
//! The pin is specified using a bit-packed byte, where bit 0 of the byte
//! represents GPIO port pin 0, bit 1 represents GPIO port pin 1, and so on.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTRxGPIOSet(tSoftUART *psUART, uint32_t ui32Base, uint8_t ui8Pin)
{
    //
    // Save the base address and pin for the Rx signal.
    //
    if(ui32Base == 0)
    {
        psUART->ui32RxGPIOPort = 0;
        psUART->ui8RxPin = 0;
    }
    else
    {
        psUART->ui32RxGPIOPort = ui32Base;
        psUART->ui8RxPin = ui8Pin;
    }
}

//*****************************************************************************
//
//! Sets the transmit buffer for a SoftUART module.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param pui8TxBuffer is the address of the transmit buffer.
//! \param ui16Len is the size, in 8-bit bytes, of the transmit buffer.
//!
//! This function sets the address and size of the transmit buffer.  It also
//! resets the read and write pointers, marking the transmit buffer as empty.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTTxBufferSet(tSoftUART *psUART, uint8_t *pui8TxBuffer, uint16_t ui16Len)
{
    //
    // Save the transmit buffer address and length.
    //
    psUART->pui8TxBuffer = pui8TxBuffer;
    psUART->ui16TxBufferLen = ui16Len;

    //
    // Reset the transmit buffer read and write pointers.
    //
    psUART->ui16TxBufferRead = 0;
    psUART->ui16TxBufferWrite = 0;

    //
    // Compute the new buffer "interrupt" level.
    //
    SoftUARTTxLevelSet(psUART);
}

//*****************************************************************************
//
//! Sets the receive buffer for a SoftUART module.
//!
//! \param psUART specifies the SoftUART data structure.
//! \param pui16RxBuffer is the address of the receive buffer.
//! \param ui16Len is the size, in 16-bit half-words, of the receive buffer.
//!
//! This function sets the address and size of the receive buffer.  It also
//! resets the read and write pointers, marking the receive buffer as empty.
//!
//! \return None.
//
//*****************************************************************************
void
SoftUARTRxBufferSet(tSoftUART *psUART, uint16_t *pui16RxBuffer,
                    uint16_t ui16Len)
{
    //
    // Save the receive buffer address and length.
    //
    psUART->pui16RxBuffer = pui16RxBuffer;
    psUART->ui16RxBufferLen = ui16Len;

    //
    // Reset the receive read and write pointers.
    //
    psUART->ui16RxBufferRead = 0;
    psUART->ui16RxBufferWrite = 0;

    //
    // Compute the new buffer "interrupt" level.
    //
    SoftUARTRxLevelSet(psUART);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
