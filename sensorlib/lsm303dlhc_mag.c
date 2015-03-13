//*****************************************************************************
//
// lsm303dlhc.c - Driver for the ST LSM303DLHC magnetometer
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
#include "sensorlib/hw_lsm303dlhc.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/lsm303dlhc_mag.h"

//*****************************************************************************
//
//! \addtogroup lsm303dlhc_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The states of the LSM303DLHC state machine.
//
//*****************************************************************************
#define LSM303DLHC_STATE_IDLE   0           // State machine is idle
#define LSM303DLHC_STATE_READ   1           // Waiting for read
#define LSM303DLHC_STATE_WRITE  2           // Waiting for write
#define LSM303DLHC_STATE_RMW    3           // Waiting for read-modify-write

//*****************************************************************************
//
// The factors used to convert the magnetometer readings from the LSM303 into
// floating point values in tesla
//
//*****************************************************************************
static const float g_pfLSM303DLHCMagnetoFactors[] =
{
    0,
    9.09E-08f,
    1.17E-07f,
    1.49E-07f,
    2.22E-07f,
    2.50E-07f,
    3.03E-07f,
    4.35E-07f,
};

//*****************************************************************************
//
// The callback function that is called when I2C transations to/from the
// LSM303DLHC have completed.
//
//*****************************************************************************
static void
LSM303DLHCCallback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tLSM303DLHCMag *psInst;

    //
    // Convert the instance data into a pointer to a tLSM303DLHC structure.
    //
    psInst = pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result in a callback to propagate the
    // error).
    //
    if(ui8Status != I2CM_STATUS_SUCCESS)
    {
        psInst->ui8State = LSM303DLHC_STATE_IDLE;
    }

    //
    // Determine the current state of the LSM303DLHC state machine.
    //
    switch(psInst->ui8State)
    {
        //
        // All states that trivially transition to IDLE, and all unknown
        // states.
        //
        case LSM303DLHC_STATE_READ:
        default:
        {
            //
            // The state machine is now idle.
            //
            psInst->ui8State = LSM303DLHC_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        //
        // A write just completed
        //
        case LSM303DLHC_STATE_WRITE:
        {
            //
            // Set the magneto ranges to the new values.  If the register was
            // not modified, the values will be the same so this has no effect.
            //
            psInst->ui8MagnetoFsSel = psInst->ui8NewMagnetoFsSel;

            //
            // The state machine is now idle.
            //
            psInst->ui8State = LSM303DLHC_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        //
        // A read-modify-write just completed
        //
        case LSM303DLHC_STATE_RMW:
        {
            //
            // See if the MAGNETO_CONFIG register was just modified.
            //
            if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[0] ==
               LSM303DLHC_O_MAG_CRB)
            {
                //
                // Extract the FS_SEL from the MAGNETO_CONFIG register value.
                //
                psInst->ui8MagnetoFsSel =
                    ((psInst->uCommand.sReadModifyWriteState.pui8Buffer[1] &
                      LSM303DLHC_MAG_CRB_GAIN_M) >> LSM303DLHC_MAG_CRB_GAIN_S);
            }

            //
            // The state machine is now idle.
            //
            psInst->ui8State = LSM303DLHC_STATE_IDLE;

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == LSM303DLHC_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Initializes the LSM303DLHC driver.
//!
//! \param psInst is a pointer to the LSM303DLHC instance data.
//! \param psI2CInst is a pointer to the I2C master driver instance data.
//! \param ui8I2CAddr is the I2C address of the LSM303DLHC device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the LSM303DLHC driver, preparing it for
//! operation.
//!
//! \return Returns 1 if the LSM303DLHC driver was successfully initialized and
//! 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
LSM303DLHCMagInit(tLSM303DLHCMag *psInst, tI2CMInstance *psI2CInst,
                  uint_fast8_t ui8I2CAddr, tSensorCallback *pfnCallback,
                  void *pvCallbackData)
{
    //
    // Initialize the LSM303DLHC instance structure.
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8I2CAddr;

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Default range setting is +/- 1.3
    // TODO: double-check default
    //
    psInst->ui8MagnetoFsSel = (LSM303DLHC_MAG_CRB_GAIN_1_3GAUSS >>
                               LSM303DLHC_MAG_CRB_GAIN_S);
    psInst->ui8NewMagnetoFsSel = (LSM303DLHC_MAG_CRB_GAIN_1_3GAUSS >>
                                  LSM303DLHC_MAG_CRB_GAIN_S);
    psInst->ui8State = LSM303DLHC_STATE_IDLE;

    if(pfnCallback)
    {
        pfnCallback(pvCallbackData, I2CM_STATUS_SUCCESS);
    }

    //
    // Success
    //
    return(1);
}

//*****************************************************************************
//
//! Reads data from LSM303DLHC registers.
//!
//! \param psInst is a pointer to the LSM303DLHC instance data.
//! \param ui8Reg is the first register to read.
//! \param pui8Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count is the number of data bytes to read.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the LSM303DLHC.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
LSM303DLHCMagRead(tLSM303DLHCMag *psInst, uint_fast8_t ui8Reg,
                  uint8_t *pui8Data, uint_fast16_t ui16Count,
                  tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the LSM303DLHC driver is not idle (in other words,
    // there is already an outstanding request to the LSM303DLHC).
    //
    if(psInst->ui8State != LSM303DLHC_STATE_IDLE)
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
    psInst->ui8State = LSM303DLHC_STATE_READ;

    //
    // Read the requested registers from the LSM303DLHC.
    //
    psInst->uCommand.pui8Buffer[0] = ui8Reg;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, pui8Data, ui16Count,
                LSM303DLHCCallback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = LSM303DLHC_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Writes data to LSM303DLHC registers.
//!
//! \param psInst is a pointer to the LSM303DLHC instance data.
//! \param ui8Reg is the first register to write.
//! \param pui8Data is a pointer to the data to write.
//! \param ui16Count is the number of data bytes to write.
//! \param pfnCallback is the function to be called when the data has been
//! written (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the LSM303DLHC.  The first byte of the \e pui8Data buffer contains the
//! value to be written into the \e ui8Reg register, the second value contains
//! the data to be written into the next register, and so on.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
LSM303DLHCMagWrite(tLSM303DLHCMag *psInst, uint_fast8_t ui8Reg,
                   const uint8_t *pui8Data, uint_fast16_t ui16Count,
                   tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the LSM303DLHC driver is not idle (in other words,
    // there is already an outstanding request to the LSM303DLHC).
    //
    if(psInst->ui8State != LSM303DLHC_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // See if the MAGNETO_CONFIG register is being written.
    //
    if((ui8Reg <= LSM303DLHC_O_MAG_CRB) &&
       ((ui8Reg + ui16Count) > LSM303DLHC_O_MAG_CRB))
    {
        //
        // Extract the FS_SEL from the MAGNETO_CONFIG register value.
        //
        psInst->ui8NewMagnetoFsSel = ((pui8Data[ui8Reg - LSM303DLHC_O_MAG_CRB] &
                                       LSM303DLHC_MAG_CRB_GAIN_M) >>
                                      LSM303DLHC_MAG_CRB_GAIN_S);
    }

    //
    // Move the state machine to the wait for write state.
    //
    psInst->ui8State = LSM303DLHC_STATE_WRITE;

    //
    // Write the requested registers to the LSM303DLHC.
    //
    if(I2CMWrite8(&(psInst->uCommand.sWriteState), psInst->psI2CInst,
                  psInst->ui8Addr, ui8Reg, pui8Data, ui16Count,
                  LSM303DLHCCallback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = LSM303DLHC_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs a read-modify-write of a LSM303DLHC register.
//!
//! \param psInst is a pointer to the LSM303DLHC instance data.
//! \param ui8Reg is the register to modify.
//! \param ui8Mask is the bit mask that is ANDed with the current register
//! value.
//! \param ui8Value is the bit mask that is ORed with the result of the AND
//! operation.
//! \param pfnCallback is the function to be called when the data has been
//! changed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function changes the value of a register in the LSM303DLHC via a
//! read-modify-write operation, allowing one of the fields to be changed
//! without disturbing the other fields.  The \e ui8Reg register is read, ANDed
//! with \e ui8Mask, ORed with \e ui8Value, and then written back to the
//! LSM303DLHC.
//!
//! \return Returns 1 if the read-modify-write was successfully started and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
LSM303DLHCMagReadModifyWrite(tLSM303DLHCMag *psInst, uint_fast8_t ui8Reg,
                             uint_fast8_t ui8Mask, uint_fast8_t ui8Value,
                             tSensorCallback *pfnCallback,
                             void *pvCallbackData)
{
    //
    // Return a failure if the LSM303DLHC driver is not idle (in other words,
    // there is already an outstanding request to the LSM303DLHC).
    //
    if(psInst->ui8State != LSM303DLHC_STATE_IDLE)
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
    psInst->ui8State = LSM303DLHC_STATE_RMW;

    //
    // Submit the read-modify-write request to the LSM303DLHC.
    //
    if(I2CMReadModifyWrite8(&(psInst->uCommand.sReadModifyWriteState),
                            psInst->psI2CInst, psInst->ui8Addr, ui8Reg,
                            ui8Mask, ui8Value, LSM303DLHCCallback,
                            psInst) == 0)
    {
        //
        // The I2C read-modify-write failed, so move to the idle state and
        // return a failure.
        //
        psInst->ui8State = LSM303DLHC_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads the magneto data from the LSM303DLHC.
//!
//! \param psInst is a pointer to the LSM303DLHC instance data.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read of the LSM303DLHC data registers.  When the
//! read has completed (as indicated by calling the callback function), the new
//! readings can be obtained via:
//!
//! - LSM303DLHCDataMagnetoGetRaw()
//! - LSM303DLHCDataMagnetoGetFloat()
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
LSM303DLHCMagDataRead(tLSM303DLHCMag *psInst, tSensorCallback *pfnCallback,
                      void *pvCallbackData)
{
    //
    // Return a failure if the LSM303DLHC driver is not idle (in other words,
    // there is already an outstanding request to the LSM303DLHC).
    //
    if(psInst->ui8State != LSM303DLHC_STATE_IDLE)
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
    psInst->ui8State = LSM303DLHC_STATE_READ;

    //
    // Read the data registers from the LSM303DLHC.
    //
    psInst->pui8Data[0] = LSM303DLHC_O_MAG_OUT_X_MSB;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr, psInst->pui8Data, 1,
                psInst->pui8Data, 7, LSM303DLHCCallback, psInst) == 0)
    {
        //
        // The I2C read failed, so move to the idle state and return a failure.
        //
        psInst->ui8State = LSM303DLHC_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Gets the raw magneto data from the most recent data read.
//!
//! \param psInst is a pointer to the LSM303DLHC instance data.
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
LSM303DLHCMagDataMagnetoGetRaw(tLSM303DLHCMag *psInst,
                               uint_fast16_t *pui16MagnetoX,
                               uint_fast16_t *pui16MagnetoY,
                               uint_fast16_t *pui16MagnetoZ)
{
    //
    // Return the raw magnetometer values.
    //
    if(pui16MagnetoX)
    {
        *pui16MagnetoX = (psInst->pui8Data[0] << 8) | psInst->pui8Data[1];
    }
    if(pui16MagnetoY)
    {
        *pui16MagnetoY = (psInst->pui8Data[2] << 8) | psInst->pui8Data[3];
    }
    if(pui16MagnetoZ)
    {
        *pui16MagnetoZ = (psInst->pui8Data[4] << 8) | psInst->pui8Data[5];
    }
}

//*****************************************************************************
//
//! Gets the magnetometer data from the most recent data read.
//!
//! \param psInst is a pointer to the LSM303DLHC instance data.
//! \param pfMagnetoX is a pointer to the value into which the X-axis
//! magnetometer data is stored.
//! \param pfMagnetoY is a pointer to the value into which the Y-axis
//! magnetometer data is stored.
//! \param pfMagnetoZ is a pointer to the value into which the Z-axis
//! magnetometer data is stored.
//!
//! This function returns the magnetometer data from the most recent data read,
//! converted into radians per second.  If any of the output data pointers are
//! \b NULL, the corresponding data is not provided.
//!
//! \return None.
//
//*****************************************************************************
void
LSM303DLHCMagDataMagnetoGetFloat(tLSM303DLHCMag *psInst, float *pfMagnetoX,
                                 float *pfMagnetoY, float *pfMagnetoZ)
{
    float fFactor;

    //
    // Get the conversion factor for the current data format.
    //
    fFactor = g_pfLSM303DLHCMagnetoFactors[psInst->ui8MagnetoFsSel];

    //
    // Convert the magnetometer values into rad/sec
    //
    if(pfMagnetoX)
    {
        *pfMagnetoX = (float)(((int16_t)((psInst->pui8Data[0] << 8) |
                                         psInst->pui8Data[1])) * fFactor);
    }
    if(pfMagnetoY)
    {
        *pfMagnetoY = (float)(((int16_t)((psInst->pui8Data[2] << 8) |
                                         psInst->pui8Data[3])) * fFactor);
    }
    if(pfMagnetoZ)
    {
        *pfMagnetoZ = (float)(((int16_t)((psInst->pui8Data[4] << 8) |
                                         psInst->pui8Data[5])) * fFactor);
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
