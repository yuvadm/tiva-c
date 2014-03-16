//*****************************************************************************
//
// softssi.c - Driver for the SoftSSI.
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
//! \addtogroup softssi_api
//! @{
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "utils/softssi.h"

//*****************************************************************************
//
// The states in the SoftSSI state machine.
//
//*****************************************************************************
#define SOFTSSI_STATE_IDLE      0
#define SOFTSSI_STATE_START     1
#define SOFTSSI_STATE_IN        2
#define SOFTSSI_STATE_OUT       3
#define SOFTSSI_STATE_STOP1     4
#define SOFTSSI_STATE_STOP2     5

//*****************************************************************************
//
// The flags in the SoftSSI ui8Flags structure member.
//
//*****************************************************************************
#define SOFTSSI_FLAG_ENABLE     0x80
#define SOFTSSI_FLAG_SPH        0x02
#define SOFTSSI_FLAG_SPO        0x01

//*****************************************************************************
//
//! Sets the configuration of a SoftSSI module.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param ui8Protocol specifes the data transfer protocol.
//! \param ui8Bits specifies the number of bits transferred per frame.
//!
//! This function configures the data format of a SoftSSI module.  The
//! \e ui8Protocol parameter can be one of the following values:
//! \b SOFTSSI_FRF_MOTO_MODE_0, \b SOFTSSI_FRF_MOTO_MODE_1,
//! \b SOFTSSI_FRF_MOTO_MODE_2, or \b SOFTSSI_FRF_MOTO_MODE_3.  These frame
//! formats imply the following polarity and phase configurations:
//!
//! <pre>
//! Polarity Phase         Mode
//!   0       0   SOFTSSI_FRF_MOTO_MODE_0
//!   0       1   SOFTSSI_FRF_MOTO_MODE_1
//!   1       0   SOFTSSI_FRF_MOTO_MODE_2
//!   1       1   SOFTSSI_FRF_MOTO_MODE_3
//! </pre>
//!
//! The \e ui8Bits parameter defines the width of the data transfers, and can
//! be a value between 4 and 16, inclusive.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSIConfigSet(tSoftSSI *psSSI, uint8_t ui8Protocol,
                 uint8_t ui8Bits)
{
    //
    // See if a GPIO pin has been set for Fss.
    //
    if(psSSI->ui32FssGPIO != 0)
    {
        //
        // Configure the Fss pin.
        //
        MAP_GPIOPinTypeGPIOOutput(psSSI->ui32FssGPIO & 0xfffff000,
                                  (psSSI->ui32FssGPIO & 0x00000fff) >> 2);

        //
        // Set the Fss pin high.
        //
        HWREG(psSSI->ui32FssGPIO) = 255;
    }

    //
    // Configure the Clk pin.
    //
    MAP_GPIOPinTypeGPIOOutput(psSSI->ui32ClkGPIO & 0xfffff000,
                              (psSSI->ui32ClkGPIO & 0x00000fff) >> 2);

    //
    // Set the Clk pin high or low based on the configured clock polarity.
    //
    if((ui8Protocol & SOFTSSI_FLAG_SPO) == 0)
    {
        HWREG(psSSI->ui32ClkGPIO) = 0;
    }
    else
    {
        HWREG(psSSI->ui32ClkGPIO) = 255;
    }

    //
    // Configure the Tx pin and set it low.
    //
    MAP_GPIOPinTypeGPIOOutput(psSSI->ui32TxGPIO & 0xfffff000,
                              (psSSI->ui32TxGPIO & 0x00000fff) >> 2);
    HWREG(psSSI->ui32TxGPIO) = 0;

    //
    // See if a GPIO pin has been set for Rx.
    //
    if(psSSI->ui32RxGPIO != 0)
    {
        //
        // Configure the Rx pin.
        //
        MAP_GPIOPinTypeGPIOInput(psSSI->ui32RxGPIO & 0xfffff000,
                                 (psSSI->ui32RxGPIO & 0x00000fff) >> 2);
    }

    //
    // Make sure that the transmit and receive FIFOs are empty.
    //
    psSSI->ui16TxBufferRead = 0;
    psSSI->ui16TxBufferWrite = 0;
    psSSI->ui16RxBufferRead = 0;
    psSSI->ui16RxBufferWrite = 0;

    //
    // Save the frame protocol.
    //
    psSSI->ui8Flags = ui8Protocol;

    //
    // Save the number of data bits.
    //
    psSSI->ui8Bits = ui8Bits;

    //
    // Since the FIFOs are empty, the transmit FIFO "interrupt" is asserted.
    //
    psSSI->ui8IntStatus = SOFTSSI_TXFF;

    //
    // Reset the idle counter.
    //
    psSSI->ui8IdleCount = 0;

    //
    // Disable the SoftSSI module.
    //
    psSSI->ui8Flags &= ~(SOFTSSI_FLAG_ENABLE);

    //
    // Start the SoftSSI state machine in the idle state.
    //
    psSSI->ui8State = SOFTSSI_STATE_IDLE;
}

//*****************************************************************************
//
//! Handles the assertion/deassertion of the transmit FIFO ``interrupt''.
//!
//! \param psSSI specifies the SoftSSI data structure.
//!
//! This function is used to determine when to assert or deassert the transmit
//! FIFO ``interrupt''.
//!
//! \return None.
//
//*****************************************************************************
static void
SoftSSITxInt(tSoftSSI *psSSI)
{
    uint16_t ui16Temp;

    //
    // Determine the number of words left in the transmit FIFO.
    //
    if(psSSI->ui16TxBufferRead > psSSI->ui16TxBufferWrite)
    {
        ui16Temp = (psSSI->ui16TxBufferLen + psSSI->ui16TxBufferWrite -
                    psSSI->ui16TxBufferRead);
    }
    else
    {
        ui16Temp = psSSI->ui16TxBufferWrite - psSSI->ui16TxBufferRead;
    }

    //
    // If the transmit FIFO is now half full or less, generate a transmit FIFO
    // "interrupt".  Otherwise, clear the transmit FIFO "interrupt".
    //
    if(ui16Temp <= (psSSI->ui16TxBufferLen / 2))
    {
        psSSI->ui8IntStatus |= SOFTSSI_TXFF;
    }
    else
    {
        psSSI->ui8IntStatus &= ~(SOFTSSI_TXFF);
    }
}

//*****************************************************************************
//
//! Handles the assertion/deassertion of the receive FIFO ``interrupt''.
//!
//! \param psSSI specifies the SoftSSI data structure.
//!
//! This function is used to determine when to assert or deassert the receive
//! FIFO ``interrupt''.
//!
//! \return None.
//
//*****************************************************************************
static void
SoftSSIRxInt(tSoftSSI *psSSI)
{
    uint16_t ui16Temp;

    //
    // Determine the number of words in the receive FIFO.
    //
    if(psSSI->ui16RxBufferRead > psSSI->ui16RxBufferWrite)
    {
        ui16Temp = (psSSI->ui16RxBufferLen + psSSI->ui16RxBufferWrite -
                    psSSI->ui16RxBufferRead);
    }
    else
    {
        ui16Temp = psSSI->ui16RxBufferWrite - psSSI->ui16RxBufferRead;
    }

    //
    // If the receive FIFO is now half full or more, generate a receive FIFO
    // "interrupt".  Otherwise, clear the receive FIFO "interrupt".
    //
    if(ui16Temp >= (psSSI->ui16RxBufferLen / 2))
    {
        psSSI->ui8IntStatus |= SOFTSSI_RXFF;
    }
    else
    {
        psSSI->ui8IntStatus &= ~(SOFTSSI_RXFF);
    }
}

//*****************************************************************************
//
//! Performs the periodic update of the SoftSSI module.
//!
//! \param psSSI specifies the SoftSSI data structure.
//!
//! This function performs the periodic, time-based updates to the SoftSSI
//! module.  The transmission and reception of data over the SoftSSI link is
//! performed by the state machine in this function.
//!
//! This function must be called at twice the desired SoftSSI clock rate.  For
//! example, to run the SoftSSI clock at 10 KHz, this function must be called
//! at a 20 KHz rate.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSITimerTick(tSoftSSI *psSSI)
{
    uint16_t ui16Temp;

    //
    // Determine the current state of the state machine.
    //
    switch(psSSI->ui8State)
    {
        //
        // The state machine is idle.
        //
        case SOFTSSI_STATE_IDLE:
        {
            //
            // See if the SoftSSI module is enabled and there is data in the
            // transmit FIFO.
            //
            if(((psSSI->ui8Flags & SOFTSSI_FLAG_ENABLE) != 0) &&
               (psSSI->ui16TxBufferRead != psSSI->ui16TxBufferWrite))
            {
                //
                // Assert the Fss signal if it is configured.
                //
                if(psSSI->ui32FssGPIO != 0)
                {
                    HWREG(psSSI->ui32FssGPIO) = 0;
                }

                //
                // Move to the start state.
                //
                psSSI->ui8State = SOFTSSI_STATE_START;
            }

            //
            // Otherwise, see if there is data in the receive FIFO.
            //
            else if((psSSI->ui16RxBufferRead != psSSI->ui16RxBufferWrite) &&
                    (psSSI->ui8IdleCount != 64))
            {
                //
                // Increment the idle counter.
                //
                psSSI->ui8IdleCount++;

                //
                // See if the idle counter has become large enough to trigger
                // a timeout "interrupt".
                //
                if(psSSI->ui8IdleCount == 64)
                {
                    //
                    // Trigger the receive timeout "interrupt".
                    //
                    psSSI->ui8IntStatus |= SOFTSSI_RXTO;
                }
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The start machine is in the transfer start state.
        //
        case SOFTSSI_STATE_START:
        {
            //
            // Get the next word to transfer from the transmit FIFO.
            //
            psSSI->ui16TxData =
                (psSSI->pui16TxBuffer[psSSI->ui16TxBufferRead] <<
                 (16 - psSSI->ui8Bits));

            //
            // Initialize the receive buffer to zero.
            //
            psSSI->ui16RxData = 0;

            //
            // Initialize the count of bits tranferred.
            //
            psSSI->ui8CurrentBit = 0;

            //
            // Write the first bit of the transmit word to the Tx pin.
            //
            HWREG(psSSI->ui32TxGPIO) =
                (psSSI->ui16TxData & 0x8000) ? 255 : 0;

            //
            // Shift to the next bit of the transmit word.
            //
            psSSI->ui16TxData <<= 1;

            //
            // If in SPI mode 1 or 3, then the Clk signal needs to be toggled.
            //
            if((psSSI->ui8Flags & SOFTSSI_FLAG_SPH) != 0)
            {
                HWREG(psSSI->ui32ClkGPIO) ^= 255;
            }

            //
            // Move to the data input state.
            //
            psSSI->ui8State = SOFTSSI_STATE_IN;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the data input state.
        //
        case SOFTSSI_STATE_IN:
        {
            //
            // Read the next bit from the Rx signal if it is configured.
            //
            if(psSSI->ui32RxGPIO != 0)
            {
                psSSI->ui16RxData = ((psSSI->ui16RxData << 1) |
                                     (HWREG(psSSI->ui32RxGPIO) ? 1 : 0));
            }

            //
            // Toggle the Clk signal.
            //
            HWREG(psSSI->ui32ClkGPIO) ^= 255;

            //
            // Increment the number of bits transferred.
            //
            psSSI->ui8CurrentBit++;

            //
            // See if the entire word has been transferred.
            //
            if(psSSI->ui8CurrentBit != psSSI->ui8Bits)
            {
                //
                // There are more bits to transfer, so move to the data output
                // state.
                //
                psSSI->ui8State = SOFTSSI_STATE_OUT;
            }
            else
            {
                //
                // Increment the transmit read pointer, removing the word that
                // was just transferred from the transmit FIFO.
                //
                psSSI->ui16TxBufferRead++;
                if(psSSI->ui16TxBufferRead == psSSI->ui16TxBufferLen)
                {
                    psSSI->ui16TxBufferRead = 0;
                }

                //
                // See if a transmit FIFO "interrupt" needs to be asserted.
                //
                SoftSSITxInt(psSSI);

                //
                // Determine the new value for the receive FIFO write pointer.
                //
                ui16Temp = psSSI->ui16RxBufferWrite + 1;
                if(ui16Temp >= psSSI->ui16RxBufferLen)
                {
                    ui16Temp = 0;
                }

                //
                // See if there is space in the receive FIFO for the word that
                // was just received.
                //
                if(ui16Temp == psSSI->ui16RxBufferRead)
                {
                    //
                    // The receive FIFO is full, so generate a receive FIFO
                    // overrun "interrupt".
                    //
                    psSSI->ui8IntStatus |= SOFTSSI_RXOR;
                }
                else
                {
                    //
                    // Store the new word into the receive FIFO.
                    //
                    psSSI->pui16RxBuffer[psSSI->ui16RxBufferWrite] =
                        psSSI->ui16RxData;

                    //
                    // Save the new receive FIFO write pointer.
                    //
                    psSSI->ui16RxBufferWrite = ui16Temp;

                    //
                    // See if a receive FIFO "interrupt" needs to be asserted.
                    //
                    SoftSSIRxInt(psSSI);
                }

                //
                // See if the next word should be transmitted immediately.
                // This will occur when there is data in the transmit FIFO, the
                // SoftSSI module is enabled, and the SoftSSI module is in SPI
                // mode 1 or 3.
                //
                if(((psSSI->ui8Flags & SOFTSSI_FLAG_ENABLE) != 0) &&
                   ((psSSI->ui8Flags & SOFTSSI_FLAG_SPH) != 0) &&
                   (psSSI->ui16TxBufferRead != psSSI->ui16TxBufferWrite))
                {
                    //
                    // Get the next word to transfer from the transmit FIFO.
                    //
                    psSSI->ui16TxData =
                        (psSSI->pui16TxBuffer[psSSI->ui16TxBufferRead] <<
                         (16 - psSSI->ui8Bits));

                    //
                    // Initialize the receive buffer to zero.
                    //
                    psSSI->ui16RxData = 0;

                    //
                    // Initialize the count of bits tranferred.
                    //
                    psSSI->ui8CurrentBit = 0;

                    //
                    // Move to the data output state.
                    //
                    psSSI->ui8State = SOFTSSI_STATE_OUT;
                }
                else
                {
                    //
                    // The next word should not be transmitted immediately, so
                    // move to the first step of the stop state.
                    //
                    psSSI->ui8State = SOFTSSI_STATE_STOP1;
                }
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the data output state.
        //
        case SOFTSSI_STATE_OUT:
        {
            //
            // Write the next bit of the transmit word to the Tx pin.
            //
            HWREG(psSSI->ui32TxGPIO) = (psSSI->ui16TxData & 0x8000) ? 255 : 0;

            //
            // Toggle the Clk signal.
            //
            HWREG(psSSI->ui32ClkGPIO) ^= 255;

            //
            // Shift to the next bit of the transmit word.
            //
            psSSI->ui16TxData <<= 1;

            //
            // Move to the data input state.
            //
            psSSI->ui8State = SOFTSSI_STATE_IN;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the first step of the stop state.
        //
        case SOFTSSI_STATE_STOP1:
        {
            //
            // Set the Tx pin low.
            //
            HWREG(psSSI->ui32TxGPIO) = 0;

            //
            // If in SPI mode 1 or 3, then the Clk signal needs to be toggled.
            //
            if((psSSI->ui8Flags & SOFTSSI_FLAG_SPH) == 0)
            {
                HWREG(psSSI->ui32ClkGPIO) ^= 255;
            }

            //
            // Move to the second step of the stop state.
            //
            psSSI->ui8State = SOFTSSI_STATE_STOP2;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The state machine is in the second step of the stop state.
        //
        case SOFTSSI_STATE_STOP2:
        {
            //
            // Deassert the Fss signal if it is configured.
            //
            if(psSSI->ui32FssGPIO != 0)
            {
                HWREG(psSSI->ui32FssGPIO) = 255;
            }

            //
            // Move to the idle state.
            //
            psSSI->ui8State = SOFTSSI_STATE_IDLE;

            //
            // Reset the idle counter.
            //
            psSSI->ui8IdleCount = 0;

            //
            // See if the end of transfer "interrupt" should be generated.
            //
            if(psSSI->ui16TxBufferRead == psSSI->ui16TxBufferWrite)
            {
                psSSI->ui8IntStatus |= SOFTSSI_TXEOT;
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
    // the SSI peripheral.
    //
    while(((psSSI->ui8IntStatus & psSSI->ui8IntMask) != 0) &&
          (psSSI->pfnIntCallback != 0))
    {
        //
        // Call the callback function.
        //
        psSSI->pfnIntCallback();
    }
}

//*****************************************************************************
//
//! Enables the SoftSSI module.
//!
//! \param psSSI specifies the SoftSSI data structure.
//!
//! This function enables operation of the SoftSSI module.  The SoftSSI module
//! must be configured before it is enabled.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSIEnable(tSoftSSI *psSSI)
{
    //
    // Enable the SoftSSI module.
    //
    psSSI->ui8Flags |= SOFTSSI_FLAG_ENABLE;
}

//*****************************************************************************
//
//! Disables the SoftSSI module.
//!
//! \param psSSI specifies the SoftSSI data structure.
//!
//! This function disables operation of the SoftSSI module.  If a data transfer
//! is in progress, it is finished before the module is fully disabled.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSIDisable(tSoftSSI *psSSI)
{
    //
    // Disable the SoftSSI module.
    //
    psSSI->ui8Flags &= ~(SOFTSSI_FLAG_ENABLE);
}

//*****************************************************************************
//
//! Enables individual SoftSSI ``interrupt'' sources.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param ui32IntFlags is a bit mask of the ``interrupt'' sources to be
//! enabled.
//!
//! Enables the indicated SoftSSI ``interrupt'' sources.  Only the sources that
//! are enabled can be reflected to the callback function; disabled sources do
//! not result in a callback.  The \e ui32IntFlags parameter can be any of the
//! \b SOFTSSI_TXEOT, \b SOFTSSI_TXFF, \b SOFTSSI_RXFF, \b SOFTSSI_RXTO, or
//! \b SOFTSSI_RXOR values.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSIIntEnable(tSoftSSI *psSSI, uint32_t ui32IntFlags)
{
    //
    // Enable the specified "interrupts".
    //
    psSSI->ui8IntMask |= ui32IntFlags;
}

//*****************************************************************************
//
//! Disables individual SoftSSI ``interrupt'' sources.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param ui32IntFlags is a bit mask of the ``interrupt'' sources to be
//! disabled.
//!
//! Disables the indicated SoftSSI ``interrupt'' sources.  The \e ui32IntFlags
//! parameter can be any of the \b SOFTSSI_TXEOT, \b SOFTSSI_TXFF,
//! \b SOFTSSI_RXFF, \b SOFTSSI_RXTO, or \b SOFTSSI_RXOR values.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSIIntDisable(tSoftSSI *psSSI, uint32_t ui32IntFlags)
{
    //
    // Disable the specified "interrupts".
    //
    psSSI->ui8IntMask &= ~(ui32IntFlags);
}

//*****************************************************************************
//
//! Gets the current ``interrupt'' status.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param bMasked is \b false if the raw ``interrupt'' status is required or
//! \b true if the masked ``interrupt'' status is required.
//!
//! This function returns the ``interrupt'' status for the SoftSSI module.
//! Either the raw ``interrupt'' status or the status of ``interrupts'' that
//! are allowed to reflect to the callback can be returned.
//!
//! \return The current ``interrupt'' status, enumerated as a bit field of
//! \b SOFTSSI_TXEOT, \b SOFTSSI_TXFF, \b SOFTSSI_RXFF, \b SOFTSSI_RXTO, and
//! \b SOFTSSI_RXOR.
//
//*****************************************************************************
uint32_t
SoftSSIIntStatus(tSoftSSI *psSSI, bool bMasked)
{
    //
    // Return either the "interrupt" status or the raw "interrupt" status as
    // requested.
    //
    if(bMasked)
    {
        return(psSSI->ui8IntStatus & psSSI->ui8IntMask);
    }
    else
    {
        return(psSSI->ui8IntStatus);
    }
}

//*****************************************************************************
//
//! Clears SoftSSI ``interrupt'' sources.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param ui32IntFlags is a bit mask of the ``interrupt'' sources to be
//! cleared.
//!
//! The specified SoftSSI ``interrupt'' sources are cleared so that they no
//! longer assert.  This function must be called in the ``interrupt'' handler
//! to keep the ``interrupt'' from being recognized again immediately upon
//! exit.  The \e ui32IntFlags parameter is the logical OR of any of the
//! \b SOFTSSI_TXEOT, \b SOFTSSI_RXTO, and \b SOFTSSI_RXOR values.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSIIntClear(tSoftSSI *psSSI, uint32_t ui32IntFlags)
{
    //
    // Clear the requested "interrupt" sources.
    //
    psSSI->ui8IntStatus &= ~(ui32IntFlags) | SOFTSSI_TXFF | SOFTSSI_RXFF;
}

//*****************************************************************************
//
//! Determines if there is any data in the receive FIFO.
//!
//! \param psSSI specifies the SoftSSI data structure.
//!
//! This function determines if there is any data available to be read from the
//! receive FIFO.
//!
//! \return Returns \b true if there is data in the receive FIFO or \b false
//! if there is no data in the receive FIFO.
//
//*****************************************************************************
bool
SoftSSIDataAvail(tSoftSSI *psSSI)
{
    //
    // Return the availability of data.
    //
    return((psSSI->ui16RxBufferRead == psSSI->ui16RxBufferWrite) ? false :
           true);
}

//*****************************************************************************
//
//! Determines if there is any space in the transmit FIFO.
//!
//! \param psSSI specifies the SoftSSI data structure.
//!
//! This function determines if there is space available in the transmit FIFO.
//!
//! \return Returns \b true if there is space available in the transmit FIFO or
//! \b false if there is no space available in the transmit FIFO.
//
//*****************************************************************************
bool
SoftSSISpaceAvail(tSoftSSI *psSSI)
{
    uint16_t ui16Temp;

    //
    // Determine the values of the write pointer once incremented.
    //
    ui16Temp = psSSI->ui16TxBufferWrite + 1;
    if(ui16Temp == psSSI->ui16TxBufferLen)
    {
        ui16Temp = 0;
    }

    //
    // Return the availability of space.
    //
    return((psSSI->ui16TxBufferRead == ui16Temp) ? false : true);
}

//*****************************************************************************
//
//! Puts a data element into the SoftSSI transmit FIFO.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param ui32Data is the data to be transmitted over the SoftSSI interface.
//!
//! This function places the supplied data into the transmit FIFO of the
//! specified SoftSSI module.
//!
//! \note The upper 32 - N bits of the \e ui32Data are discarded, where N is
//! the data width as configured by SoftSSIConfigSet().  For example, if the
//! interface is configured for 8-bit data width, the upper 24 bits of
//! \e ui32Data are discarded.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSIDataPut(tSoftSSI *psSSI, uint32_t ui32Data)
{
    uint16_t ui16Temp;

    //
    // Wait until there is space.
    //
    ui16Temp = psSSI->ui16TxBufferWrite + 1;
    if(ui16Temp == psSSI->ui16TxBufferLen)
    {
        ui16Temp = 0;
    }
    while(ui16Temp == *(volatile uint16_t *)(&(psSSI->ui16TxBufferRead)))
    {
    }

    //
    // Write the data to the SoftSSI.
    //
    psSSI->pui16TxBuffer[psSSI->ui16TxBufferWrite] = ui32Data;
    psSSI->ui16TxBufferWrite = ui16Temp;

    //
    // See if a transmit FIFO "interrupt" needs to be cleared.
    //
    SoftSSITxInt(psSSI);
}

//*****************************************************************************
//
//! Puts a data element into the SoftSSI transmit FIFO.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param ui32Data is the data to be transmitted over the SoftSSI interface.
//!
//! This function places the supplied data into the transmit FIFO of the
//! specified SoftSSI module.  If there is no space in the FIFO, then this
//! function returns a zero.
//!
//! \note The upper 32 - N bits of the \e ui32Data are discarded, where N is
//! the data width as configured by SoftSSIConfigSet().  For example, if the
//! interface is configured for 8-bit data width, the upper 24 bits of
//! \e ui32Data are discarded.
//!
//! \return Returns the number of elements written to the SSI transmit FIFO.
//
//*****************************************************************************
int32_t
SoftSSIDataPutNonBlocking(tSoftSSI *psSSI, uint32_t ui32Data)
{
    uint16_t ui16Temp;

    //
    // Determine the values of the write pointer once incremented.
    //
    ui16Temp = psSSI->ui16TxBufferWrite + 1;
    if(ui16Temp == psSSI->ui16TxBufferLen)
    {
        ui16Temp = 0;
    }

    //
    // Check for space to write.
    //
    if(ui16Temp != psSSI->ui16TxBufferRead)
    {
        psSSI->pui16TxBuffer[psSSI->ui16TxBufferWrite] = ui32Data;
        psSSI->ui16TxBufferWrite = ui16Temp;
        SoftSSITxInt(psSSI);
        return(1);
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
//! Gets a data element from the SoftSSI receive FIFO.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param pui32Data is a pointer to a storage location for data that was
//! received over the SoftSSI interface.
//!
//! This function gets received data from the receive FIFO of the specified
//! SoftSSI module and places that data into the location specified by the
//! \e pui32Data parameter.
//!
//! \note Only the lower N bits of the value written to \e pui32Data contain
//! valid data, where N is the data width as configured by SoftSSIConfigSet().
//! For example, if the interface is configured for 8-bit data width, only the
//! lower 8 bits of the value written to \e pui32Data contain valid data.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSIDataGet(tSoftSSI *psSSI, uint32_t *pui32Data)
{
    //
    // Wait until there is data to be read.
    //
    while(psSSI->ui16RxBufferRead ==
          *(volatile uint16_t *)(&(psSSI->ui16RxBufferWrite)))
    {
    }

    //
    // Read data from SoftSSI.
    //
    *pui32Data = psSSI->pui16RxBuffer[psSSI->ui16RxBufferRead];
    psSSI->ui16RxBufferRead++;
    if(psSSI->ui16RxBufferRead == psSSI->ui16RxBufferLen)
    {
        psSSI->ui16RxBufferRead = 0;
    }

    //
    // See if a receive FIFO "interrupt" needs to be cleared.
    //
    SoftSSIRxInt(psSSI);
}

//*****************************************************************************
//
//! Gets a data element from the SoftSSI receive FIFO.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param pui32Data is a pointer to a storage location for data that was
//! received over the SoftSSI interface.
//!
//! This function gets received data from the receive FIFO of the specified
//! SoftSSI module and places that data into the location specified by the
//! \e ui32Data parameter.  If there is no data in the FIFO, then this function
//! returns a zero.
//!
//! \note Only the lower N bits of the value written to \e pui32Data contain
//! valid data, where N is the data width as configured by SoftSSIConfigSet().
//! For example, if the interface is configured for 8-bit data width, only the
//! lower 8 bits of the value written to \e pui32Data contain valid data.
//!
//! \return Returns the number of elements read from the SoftSSI receive FIFO.
//
//*****************************************************************************
int32_t
SoftSSIDataGetNonBlocking(tSoftSSI *psSSI, uint32_t *pui32Data)
{
    //
    // Check for data to read.
    //
    if(psSSI->ui16RxBufferRead != psSSI->ui16RxBufferWrite)
    {
        *pui32Data = psSSI->pui16RxBuffer[psSSI->ui16RxBufferRead];
        psSSI->ui16RxBufferRead++;
        if(psSSI->ui16RxBufferRead == psSSI->ui16RxBufferLen)
        {
            psSSI->ui16RxBufferRead = 0;
        }
        SoftSSIRxInt(psSSI);
        return(1);
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
//! Determines whether the SoftSSI transmitter is busy or not.
//!
//! \param psSSI specifies the SoftSSI data structure.
//!
//! Allows the caller to determine whether all transmitted bytes have cleared
//! the transmitter.  If \b false is returned, then the transmit FIFO is empty
//! and all bits of the last transmitted word have left the shift register.
//!
//! \return Returns \b true if the SoftSSI is transmitting or \b false if all
//! transmissions are complete.
//
//*****************************************************************************
bool
SoftSSIBusy(tSoftSSI *psSSI)
{
    //
    // Determine if the SSI is busy.
    //
    return(((psSSI->ui8State == SOFTSSI_STATE_IDLE) &&
            (((psSSI->ui8Flags & SOFTSSI_FLAG_ENABLE) == 0) ||
             (psSSI->ui16TxBufferRead == psSSI->ui16TxBufferWrite))) ? false :
           true);
}

//*****************************************************************************
//
//! Sets the callback used by the SoftSSI module.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param pfnCallback is a pointer to the callback function.
//!
//! This function sets the address of the callback function that is called when
//! there is an ``interrupt'' produced by the SoftSSI module.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSICallbackSet(tSoftSSI *psSSI, void (*pfnCallback)(void))
{
    //
    // Save the callback function address.
    //
    psSSI->pfnIntCallback = pfnCallback;
}

//*****************************************************************************
//
//! Sets the GPIO pin to be used as the SoftSSI Fss signal.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param ui32Base is the base address of the GPIO module.
//! \param ui8Pin is the bit-packed representation of the pin to use.
//!
//! This function sets the GPIO pin that is used for the SoftSSI Fss signal.
//! If there is not a GPIO pin allocated for Fss, the SoftSSI module does not
//! assert/deassert the Fss signal, leaving it to the application either to do
//! manually or to not do at all if the slave device has Fss tied to ground.
//!
//! The pin is specified using a bit-packed byte, where bit 0 of the byte
//! represents GPIO port pin 0, bit 1 represents GPIO port pin 1, and so on.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSIFssGPIOSet(tSoftSSI *psSSI, uint32_t ui32Base, uint8_t ui8Pin)
{
    //
    // Save the base address and pin for the Fss signal.
    //
    if(ui32Base == 0)
    {
        psSSI->ui32FssGPIO = 0;
    }
    else
    {
        psSSI->ui32FssGPIO = ui32Base + (ui8Pin << 2);
    }
}

//*****************************************************************************
//
//! Sets the GPIO pin to be used as the SoftSSI Clk signal.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param ui32Base is the base address of the GPIO module.
//! \param ui8Pin is the bit-packed representation of the pin to use.
//!
//! This function sets the GPIO pin that is used for the SoftSSI Clk signal.
//!
//! The pin is specified using a bit-packed byte, where bit 0 of the byte
//! represents GPIO port pin 0, bit 1 represents GPIO port pin 1, and so on.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSIClkGPIOSet(tSoftSSI *psSSI, uint32_t ui32Base, uint8_t ui8Pin)
{
    //
    // Save the base address and pin for the Clk signal.
    //
    psSSI->ui32ClkGPIO = ui32Base + (ui8Pin << 2);
}

//*****************************************************************************
//
//! Sets the GPIO pin to be used as the SoftSSI Tx signal.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param ui32Base is the base address of the GPIO module.
//! \param ui8Pin is the bit-packed representation of the pin to use.
//!
//! This function sets the GPIO pin that is used for the SoftSSI Tx signal.
//!
//! The pin is specified using a bit-packed byte, where bit 0 of the byte
//! represents GPIO port pin 0, bit 1 represents GPIO port pin 1, and so on.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSITxGPIOSet(tSoftSSI *psSSI, uint32_t ui32Base, uint8_t ui8Pin)
{
    //
    // Save the base address and pin for the Tx signal.
    //
    psSSI->ui32TxGPIO = ui32Base + (ui8Pin << 2);
}

//*****************************************************************************
//
//! Sets the GPIO pin to be used as the SoftSSI Rx signal.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param ui32Base is the base address of the GPIO module.
//! \param ui8Pin is the bit-packed representation of the pin to use.
//!
//! This function sets the GPIO pin that is used for the SoftSSI Rx signal.  If
//! there is not a GPIO pin allocated for Rx, the SoftSSI module does not read
//! data from the slave device.
//!
//! The pin is specified using a bit-packed byte, where bit 0 of the byte
//! represents GPIO port pin 0, bit 1 represents GPIO port pin 1, and so on.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSIRxGPIOSet(tSoftSSI *psSSI, uint32_t ui32Base, uint8_t ui8Pin)
{
    //
    // Save the base address and pin for the Rx signal.
    //
    if(ui32Base == 0)
    {
        psSSI->ui32RxGPIO = 0;
    }
    else
    {
        psSSI->ui32RxGPIO = ui32Base + (ui8Pin << 2);
    }
}

//*****************************************************************************
//
//! Sets the transmit FIFO buffer for a SoftSSI module.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param pui16TxBuffer is the address of the transmit FIFO buffer.
//! \param ui16Len is the size, in 16-bit half-words, of the transmit FIFO
//! buffer.
//!
//! This function sets the address and size of the transmit FIFO buffer and
//! also resets the read and write pointers, marking the transmit FIFO as
//! empty.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSITxBufferSet(tSoftSSI *psSSI, uint16_t *pui16TxBuffer,
                   uint16_t ui16Len)
{
    //
    // Save the transmit FIFO buffer address and length.
    //
    psSSI->pui16TxBuffer = pui16TxBuffer;
    psSSI->ui16TxBufferLen = ui16Len;

    //
    // Reset the transmit FIFO read and write pointers.
    //
    psSSI->ui16TxBufferRead = 0;
    psSSI->ui16TxBufferWrite = 0;
}

//*****************************************************************************
//
//! Sets the receive FIFO buffer for a SoftSSI module.
//!
//! \param psSSI specifies the SoftSSI data structure.
//! \param pui16RxBuffer is the address of the receive FIFO buffer.
//! \param ui16Len is the size, in 16-bit half-words, of the receive FIFO
//! buffer.
//!
//! This function sets the address and size of the receive FIFO buffer and also
//! resets the read and write pointers, marking the receive FIFO as empty.
//! When the buffer pointer and length are configured as zero, all data
//! received from the slave device is discarded.  This capability is useful
//! when there is no GPIO pin allocated for the Rx signal.
//!
//! \return None.
//
//*****************************************************************************
void
SoftSSIRxBufferSet(tSoftSSI *psSSI, uint16_t *pui16RxBuffer,
                   uint16_t ui16Len)
{
    //
    // Save the receive FIFO buffer address and length.
    //
    psSSI->pui16RxBuffer = pui16RxBuffer;
    psSSI->ui16RxBufferLen = ui16Len;

    //
    // Reset the receive FIFO read and write pointers.
    //
    psSSI->ui16RxBufferRead = 0;
    psSSI->ui16RxBufferWrite = 0;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
