//*****************************************************************************
//
// tmp100.c - Driver for the TI TMP100 Temperature Sensor
//
// Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the Tiva Firmware Development Package.
//
//*****************************************************************************

#include <math.h>
#include <stdint.h>
#include "sensorlib/hw_tmp100.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/tmp100.h"

//*****************************************************************************
//
//! \addtogroup tmp100_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The states of the TMP100 state machine.
//
//*****************************************************************************
#define TMP100_STATE_IDLE       0
#define TMP100_STATE_INIT       1
#define TMP100_STATE_READ       2
#define TMP100_STATE_WRITE      3
#define TMP100_STATE_RMW        4

//*****************************************************************************
//
// The callback function that is called when I2C transations to/from the TMP100
// have completed.
//
//*****************************************************************************
static void
TMP100Callback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tTMP100 *psInst;

    //
    // Convert the instance data into a pointer to a tTMP100 structure.
    //
    psInst = pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result in a callback to propagate the
    // error).
    //
    if(ui8Status != I2CM_STATUS_SUCCESS)
    {
        psInst->ui8State = TMP100_STATE_IDLE;
    }

    //
    // Determine the current state of the TMP100 state machine.
    //
    switch(psInst->ui8State)
    {
        //
        // All states that trivially transition to IDLE, and all unknown
        // states.
        //
        case TMP100_STATE_INIT:
        case TMP100_STATE_READ:
        case TMP100_STATE_WRITE:
        case TMP100_STATE_RMW:
        default:
        {
            //
            // The state machine is now idle.
            //
            psInst->ui8State = TMP100_STATE_IDLE;

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == TMP100_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Initializes the TMP100 driver.
//!
//! \param psInst is a pointer to the TMP100 instance data.
//! \param psI2CInst is a pointer to the I2C driver instance data.
//! \param ui8I2CAddr is the I2C address of the TMP100 device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the TMP100 driver, preparing it for operation,
//! and initiates a reset of the TMP100 device, clearing any previous
//! configuration data.
//!
//! \return Returns 1 if the TMP100 driver was successfully initialized and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
TMP100Init(tTMP100 *psInst, tI2CMInstance *psI2CInst, uint_fast8_t ui8I2CAddr,
           tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Initialize the TMP100 instance structure
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8I2CAddr;
    psInst->ui8State = TMP100_STATE_INIT;

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Write the configuration register to its default value.
    //
    psInst->pui8Data[0] = TMP100_O_CONFIG;
    psInst->pui8Data[1] = 0x00;

    //
    // Write the reset bit and issue a callback when finished.
    //
    if(I2CMWrite(psInst->psI2CInst, ui8I2CAddr, psInst->pui8Data, 2,
                 TMP100Callback, psInst) == 0)
    {
        //
        // I2CMWrite failed so reset TMP100 state and return zero to indicate
        // failure.
        //
        psInst->ui8State = TMP100_STATE_IDLE;
        return(0);
    }

    //
    // Success
    //
    return(1);
}

//*****************************************************************************
//
//! Reads data from TMP100 registers.
//!
//! \param psInst is a pointer to the TMP100 instance data.
//! \param ui8Reg is the first register to read.
//! \param pui16Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count the number of register values to read.
//! \param pfnCallback is the function to be called when data read is complete
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the TMP100.
//!
//! \note The TMP100 does not auto-increment the register pointer, so reads of
//! more than one value returns garbage for the subsequent values.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
TMP100Read(tTMP100 *psInst, uint_fast8_t ui8Reg, uint16_t *pui16Data,
           uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
           void *pvCallbackData)
{
    //
    // Return a failure if the TMP100 driver is not idle (in other words, there
    // is already an outstanding request to the TMP100).
    //
    if(psInst->ui8State != TMP100_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Move the state machine to the wait for read state.
    //
    psInst->ui8State = TMP100_STATE_READ;

    //
    // Read the requested registers from the TMP100.
    //
    if(ui8Reg == TMP100_O_CONFIG)
    {
        //
        // The configuration register is only one byte, so only a single byte
        // read is necessary and no endian swapping is required.
        //
        psInst->uCommand.pui8Buffer[0] = ui8Reg;
        if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                    psInst->uCommand.pui8Buffer, 1, (uint8_t *)pui16Data, 1,
                    TMP100Callback, psInst) == 0)
        {
            //
            // The I2C write failed, so move to the idle state and return a
            // failure.
            //
            psInst->ui8State = TMP100_STATE_IDLE;
            return(0);
        }
    }
    else
    {
        //
        // This is one of the temperature registers, which are 16-bit
        // big-endian registers.
        //
        if(I2CMRead16BE(&(psInst->uCommand.sReadState), psInst->psI2CInst,
                        psInst->ui8Addr, ui8Reg, pui16Data, ui16Count,
                        TMP100Callback, psInst) == 0)
        {
            //
            // The I2C write failed, so move to the idle state and return a
            // failure.
            //
            psInst->ui8State = TMP100_STATE_IDLE;
            return(0);
        }
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Writes data to TMP100 registers.
//!
//! \param psInst is a pointer to the TMP100 instance data.
//! \param ui8Reg is the first register to write.
//! \param pui16Data is a pointer to the 16-bit register data to write.
//! \param ui16Count is the number of 16-bit registers to write.
//! \param pfnCallback is the function to be called when the data has been
//! written (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the TMP100.  The first value in the \e pui16Data buffer contains the data
//! to be written into the \e ui8Reg register, the second value contains the
//! data to be written into the next register, and so on.
//!
//! \note The TMP100 does not auto-increment the register pointer, so writes of
//! more than one register are rejected by the TMP100.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
TMP100Write(tTMP100 *psInst, uint_fast8_t ui8Reg, const uint16_t *pui16Data,
            uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
            void *pvCallbackData)
{
    //
    // Return a failure if the TMP100 driver is not idle (in other words, there
    // is already an outstanding request to the TMP100).
    //
    if(psInst->ui8State != TMP100_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Move the state machine to the wait for write state.
    //
    psInst->ui8State = TMP100_STATE_WRITE;

    //
    // Write the requested registers to the TMP100.
    //
    if(ui8Reg == TMP100_O_CONFIG)
    {
        //
        // The configuration register is only one byte, so only a single byte
        // write is necessary and no endian swapping is required.
        //
        psInst->uCommand.pui8Buffer[0] = ui8Reg;
        psInst->uCommand.pui8Buffer[1] = *pui16Data & 0xff;
        if(I2CMWrite(psInst->psI2CInst, psInst->ui8Addr,
                     psInst->uCommand.pui8Buffer, 2, TMP100Callback,
                     psInst) == 0)
        {
            //
            // The I2C write failed, so move to the idle state and return a
            // failure.
            //
            psInst->ui8State = TMP100_STATE_IDLE;
            return(0);
        }
    }
    else
    {
        //
        // This is one of the temperature registers, which are 16-bit
        // big-endian registers.
        //
        if(I2CMWrite16BE(&(psInst->uCommand.sWriteState), psInst->psI2CInst,
                         psInst->ui8Addr, ui8Reg, pui16Data, ui16Count,
                         TMP100Callback, psInst) == 0)
        {
            //
            // The I2C write failed, so move to the idle state and return a
            // failure.
            //
            psInst->ui8State = TMP100_STATE_IDLE;
            return(0);
        }
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs a read-modify-write of a TMP100 register.
//!
//! \param psInst is a pointer to the TMP100 instance data.
//! \param ui8Reg is the register offset to read modify and write
//! \param ui16Mask is the bit mask that is ANDed with the current register
//! value.
//! \param ui16Value is the bit mask that is ORed with the result of the AND
//! operation.
//! \param pfnCallback is the function to be called when the data has been
//! changed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function changes the value of a register in the TMP100 via a
//! read-modify-write operation, allowing one of the fields to be changed
//! without disturbing the other fields.  The \e ui8Reg register is read, ANDed
//! with \e ui16Mask, ORed with \e ui16Value, and then written back to the
//! TMP100.
//!
//! \return Returns 1 if the read-modify-write was successfully started and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
TMP100ReadModifyWrite(tTMP100 *psInst, uint_fast8_t ui8Reg,
                      uint_fast16_t ui16Mask, uint_fast16_t ui16Value,
                      tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the TMP100 driver is not idle (in other words, there
    // is already an outstanding request to the TMP100).
    //
    if(psInst->ui8State != TMP100_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Move the state machine to the wait for read-modify-write state.
    //
    psInst->ui8State = TMP100_STATE_RMW;

    //
    // Submit the read-modify-write request to the TMP100.
    //
    if(ui8Reg == TMP100_O_CONFIG)
    {
        //
        // The configuration register is only one byte, so only a single byte
        // read-modify-write is necessary and no endian swapping is required.
        //
        if(I2CMReadModifyWrite8(&(psInst->uCommand.sReadModifyWriteState8),
                                psInst->psI2CInst, psInst->ui8Addr, ui8Reg,
                                ui16Mask & 0xff, ui16Value & 0xff,
                                TMP100Callback, psInst) == 0)
        {
            //
            // The I2C read-modify-write failed, so move to the idle state and
            // return a failure.
            //
            psInst->ui8State = TMP100_STATE_IDLE;
            return(0);
        }
    }
    else
    {
        //
        // This is one of the temperature registers, which are 16-bit
        // big-endian registers.
        //
        if(I2CMReadModifyWrite16BE(&(psInst->uCommand.sReadModifyWriteState16),
                                   psInst->psI2CInst, psInst->ui8Addr, ui8Reg,
                                   ui16Mask, ui16Value, TMP100Callback,
                                   psInst) == 0)
        {
            //
            // The I2C read-modify-write failed, so move to the idle state and
            // return a failure.
            //
            psInst->ui8State = TMP100_STATE_IDLE;
            return(0);
        }
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads the temperature data from the TMP100.
//!
//! \param psInst is a pointer to the TMP100 instance data.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read of the TMP100 data registers.  When the read
//! has completed (as indicated by calling the callback function), the new
//! readings can be obtained via:
//!
//! - TMP100DataTemperatureGetRaw()
//! - TMP100DataTemperatureGetFloat()
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
TMP100DataRead(tTMP100 *psInst, tSensorCallback *pfnCallback,
               void *pvCallbackData)
{
    //
    // Return a failure if the TMP100 driver is not idle (in other words, there
    // is already an outstanding request to the TMP100).
    //
    if(psInst->ui8State != TMP100_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Move the state machine to the wait for data read state.
    //
    psInst->ui8State = TMP100_STATE_READ;

    //
    // Read the temperature data from the TMP100.
    //
    psInst->uCommand.pui8Buffer[0] = TMP100_O_TEMP;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, psInst->pui8Data, 2,
                TMP100Callback, psInst) == 0)
    {
        //
        // The I2C read failed, so move to the idle state and return a failure.
        //
        psInst->ui8State = TMP100_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Gets the raw measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the TMP100 instance data.
//! \param pi16Temperature is a pointer to the value into which the raw
//! temperature data is stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
TMP100DataTemperatureGetRaw(tTMP100 *psInst, int16_t *pi16Temperature)
{
    //
    // Return the raw temperature value.
    //
    *pi16Temperature =
        ((int16_t)psInst->pui8Data[0] << 8) | psInst->pui8Data[1];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the TMP100 instance data.
//! \param pfTemperature is a pointer to the value into which the temperature
//! data is stored as floating point degrees Celsius.
//!
//! This function returns the temperature data from the most recent data read,
//! converted into Celsius.
//!
//! \return None.
//
//*****************************************************************************
void
TMP100DataTemperatureGetFloat(tTMP100 *psInst, float *pfTemperature)
{
    //
    // Convert the temperature reading into Celcius.
    //
    *pfTemperature = ((float)(((int16_t)psInst->pui8Data[0] << 8) |
                              psInst->pui8Data[1]) / 256.0);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
