//*****************************************************************************
//
// ak8975.c - Driver for the AK8975 magnetometer.
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

#include <stdint.h>
#include "sensorlib/hw_ak8975.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/ak8975.h"

//*****************************************************************************
//
//! \addtogroup ak8975_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The states of the AK8975 state machine.
//
//*****************************************************************************
#define AK8975_STATE_IDLE       0           // State machine is idle
#define AK8975_STATE_READ       1           // Waiting for read
#define AK8975_STATE_WRITE      2           // Waiting for write
#define AK8975_STATE_RMW        3           // Waiting for read-modify-write

//*****************************************************************************
//
// Converting sensor data to tesla (0.3 uT per LSB)
//
//*****************************************************************************
#define CONVERT_TO_TESLA        0.0000003

//*****************************************************************************
//
// The callback function that is called when I2C transations to/from the
// AK8975 have completed.
//
//*****************************************************************************
static void
AK8975Callback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tAK8975 *psInst;

    //
    // Convert the instance data into a pointer to a tAK8975 structure.
    //
    psInst = pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result in a callback to propagate the
    // error).
    //
    if(ui8Status != I2CM_STATUS_SUCCESS)
    {
        psInst->ui8State = AK8975_STATE_IDLE;
    }

    //
    // Determine the current state of the AK8975 state machine.
    //
    switch(psInst->ui8State)
    {
        //
        // All states that trivially transition to IDLE, and all unknown
        // states.
        //
        case AK8975_STATE_READ:
        case AK8975_STATE_WRITE:
        case AK8975_STATE_RMW:
        default:
        {
            //
            // The state machine is now idle.
            //
            psInst->ui8State = AK8975_STATE_IDLE;

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == AK8975_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Initializes the AK8975 driver.
//!
//! \param psInst is a pointer to the AK8975 instance data.
//! \param psI2CInst is a pointer to the I2C master driver instance data.
//! \param ui8I2CAddr is the I2C address of the AK8975 device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the AK8975 driver, preparing it for operation.
//!
//! \return Returns 1 if the AK8975 driver was successfully initialized and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
AK8975Init(tAK8975 *psInst, tI2CMInstance *psI2CInst, uint_fast8_t ui8I2CAddr,
           tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Initialize the AK8975 instance structure.
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8I2CAddr;
    psInst->ui8State = AK8975_STATE_IDLE;

    //
    // The default settings are ok.  Return success and call the callback.
    //
    if(pfnCallback)
    {
        pfnCallback(pvCallbackData, 0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads data from AK8975 registers.
//!
//! \param psInst is a pointer to the AK8975 instance data.
//! \param ui8Reg is the first register to read.
//! \param pui8Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count is the number of data bytes to read.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the AK8975.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
AK8975Read(tAK8975 *psInst, uint_fast8_t ui8Reg, uint8_t *pui8Data,
           uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
           void *pvCallbackData)
{
    //
    // Return a failure if the AK8975 driver is not idle (in other words, there
    // is already an outstanding request to the AK8975).
    //
    if(psInst->ui8State != AK8975_STATE_IDLE)
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
    psInst->ui8State = AK8975_STATE_READ;

    //
    // Read the requested registers from the AK8975.
    //
    psInst->uCommand.pui8Buffer[0] = ui8Reg;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, pui8Data, ui16Count,
                AK8975Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = AK8975_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Writes data to AK8975 registers.
//!
//! \param psInst is a pointer to the AK8975 instance data.
//! \param ui8Reg is the first register to write.
//! \param pui8Data is a pointer to the data to write.
//! \param ui16Count is the number of data bytes to write.
//! \param pfnCallback is the function to be called when the data has been
//! written (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the AK8975.  The first byte of the \e pui8Data buffer contains the value to
//! be written into the \e ui8Reg register, the second value contains the data
//! to be written into the next register, and so on.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
AK8975Write(tAK8975 *psInst, uint_fast8_t ui8Reg, uint8_t *pui8Data,
            uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
            void *pvCallbackData)
{
    //
    // Return a failure if the AK8975 driver is not idle (in other words, there
    // is already an outstanding request to the AK8975).
    //
    if(psInst->ui8State != AK8975_STATE_IDLE)
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
    psInst->ui8State = AK8975_STATE_WRITE;

    //
    // Write the requested registers to the AK8975.
    //
    if(I2CMWrite8(&(psInst->uCommand.sWriteState), psInst->psI2CInst,
                  psInst->ui8Addr, ui8Reg, pui8Data, ui16Count, AK8975Callback,
                  psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = AK8975_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs a read-modify-write of an AK8975 register.
//!
//! \param psInst is a pointer to the AK8975 instance data.
//! \param ui8Reg is the register to modify.
//! \param ui8Mask is the bit mask that is ANDed with the current register
//! value.
//! \param ui8Value is the bit mask that is ORed with the result of the AND
//! operation.
//! \param pfnCallback is the function to be called when the data has been
//! changed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function changes the value of a register in the AK8975 via a
//! read-modify-write operation, allowing one of the fields to be changed
//! without disturbing the other fields.  The \e ui8Reg register is read, ANDed
//! with \e ui8Mask, ORed with \e ui8Value, and then written back to the
//! AK8975.
//!
//! \return Returns 1 if the read-modify-write was successfully started and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
AK8975ReadModifyWrite(tAK8975 *psInst, uint_fast8_t ui8Reg,
                      uint_fast8_t ui8Mask, uint_fast8_t ui8Value,
                      tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the AK8975 driver is not idle (in other words, there
    // is already an outstanding request to the AK8975).
    //
    if(psInst->ui8State != AK8975_STATE_IDLE)
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
    psInst->ui8State = AK8975_STATE_RMW;

    //
    // Submit the read-modify-write request to the AK8975.
    //
    if(I2CMReadModifyWrite8(&(psInst->uCommand.sReadModifyWriteState),
                            psInst->psI2CInst, psInst->ui8Addr, ui8Reg,
                            ui8Mask, ui8Value, AK8975Callback, psInst) == 0)
    {
        //
        // The I2C read-modify-write failed, so move to the idle state and
        // return a failure.
        //
        psInst->ui8State = AK8975_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads the magnetometer data from the AK8975.
//!
//! \param psInst is a pointer to the AK8975 instance data.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read of the AK8975 data registers.  When the
//! read has completed (as indicated by calling the callback function), the new
//! readings can be obtained via:
//!
//! - AK8975DataMagnetoGetRaw()
//! - AK8975DataMagnetoGetFloat()
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
AK8975DataRead(tAK8975 *psInst, tSensorCallback *pfnCallback,
               void *pvCallbackData)
{
    //
    // Return a failure if the AK8975 driver is not idle (in other words, there
    // is already an outstanding request to the AK8975).
    //
    if(psInst->ui8State != AK8975_STATE_IDLE)
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
    psInst->ui8State = AK8975_STATE_READ;

    //
    // Read the data registers from the AK8975.
    //
    // ST1 + (HXL + HXH) + (HYL + HYH) + (HZL + HZH) + ST2 = 8 bytes
    //
    psInst->pui8Data[0] = AK8975_O_ST1;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr, psInst->pui8Data, 1,
                psInst->pui8Data, 8, AK8975Callback, psInst) == 0)
    {
        //
        // The I2C read failed, so move to the idle state and return a failure.
        //
        psInst->ui8State = AK8975_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Gets the raw magnetometer data from the most recent data read.
//!
//! \param psInst is a pointer to the AK8975 instance data.
//! \param pui16MagnetoX is a pointer to the value into which the raw X-axis
//! magnetometer data is stored.
//! \param pui16MagnetoY is a pointer to the value into which the raw Y-axis
//! magnetometer data is stored.
//! \param pui16MagnetoZ is a pointer to the value into which the raw Z-axis
//! magnetometer data is stored.
//!
//! This function returns the raw magnetometer data from the most recent data
//! read.  The data is not manipulated in any way by the driver.  If any of the
//! output data pointers are \b NULL, the corresponding data is not provided.
//!
//! \return None.
//
//*****************************************************************************
void
AK8975DataMagnetoGetRaw(tAK8975 *psInst, uint_fast16_t *pui16MagnetoX,
                        uint_fast16_t *pui16MagnetoY,
                        uint_fast16_t *pui16MagnetoZ)
{
    //
    // Return the raw magnetometer values.
    //
    if(pui16MagnetoX)
    {
        *pui16MagnetoX = (psInst->pui8Data[2] << 8) | psInst->pui8Data[1];
    }
    if(pui16MagnetoY)
    {
        *pui16MagnetoY = (psInst->pui8Data[4] << 8) | psInst->pui8Data[3];
    }
    if(pui16MagnetoZ)
    {
        *pui16MagnetoZ = (psInst->pui8Data[6] << 8) | psInst->pui8Data[5];
    }
}

//*****************************************************************************
//
//! Gets the magnetometer data from the most recent data read.
//!
//! \param psInst is a pointer to the AK8975 instance data.
//! \param pfMagnetoX is a pointer to the value into which the X-axis
//! magnetometer data is stored.
//! \param pfMagnetoY is a pointer to the value into which the Y-axis
//! magnetometer data is stored.
//! \param pfMagnetoZ is a pointer to the value into which the Z-axis
//! magnetometer data is stored.
//!
//! This function returns the magnetometer data from the most recent data read,
//! converted into tesla.  If any of the output data pointers are
//! \b NULL, the corresponding data is not provided.
//!
//! \return None.
//
//*****************************************************************************
void
AK8975DataMagnetoGetFloat(tAK8975 *psInst, float *pfMagnetoX,
                          float *pfMagnetoY, float *pfMagnetoZ)
{
    //
    // Convert the magnetometer values into floating-point tesla values.
    //
    if(pfMagnetoX)
    {
        *pfMagnetoX = ((float)(int16_t)((psInst->pui8Data[2] << 8) |
                                        psInst->pui8Data[1]) *
                       CONVERT_TO_TESLA);
    }
    if(pfMagnetoY)
    {
        *pfMagnetoY = ((float)(int16_t)((psInst->pui8Data[4] << 8) |
                                        psInst->pui8Data[3]) *
                       CONVERT_TO_TESLA);
    }
    if(pfMagnetoZ)
    {
        *pfMagnetoZ = ((float)(int16_t)((psInst->pui8Data[6] << 8) |
                                        psInst->pui8Data[5]) *
                       CONVERT_TO_TESLA);
    }
}

//*****************************************************************************
//
//! Gets the status registers from the most recent data read.
//!
//! \param psInst is a pointer to the AK8975 instance data.
//! \param pui8Status1 is a pointer to the value into which the ST1 data is
//! stored.
//! \param pui8Status2 is a pointer to the value into which the ST2 data is
//! stored.
//!
//! This function returns the magnetometer status registers from the most
//! recent data read.  If any of the output data pointers are \b NULL, the
//! corresponding data is not provided.
//!
//! Note that the AKM comp routines require ST1 and ST2, so we read
//! them for that reason.
//!
//! \return None.
//
//*****************************************************************************
void
AK8975DataGetStatus(tAK8975 *psInst, uint_fast8_t *pui8Status1,
                    uint_fast8_t *pui8Status2)
{
    //
    // Return the status registers
    //
    if(pui8Status1)
    {
        *pui8Status1 = psInst->pui8Data[0];
    }
    if(pui8Status2)
    {
        *pui8Status2 = psInst->pui8Data[7];
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
