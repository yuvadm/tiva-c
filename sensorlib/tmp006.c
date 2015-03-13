//*****************************************************************************
//
// tmp006.c - Driver for the TI TMP006 Temperature Sensor
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
#include "sensorlib/hw_tmp006.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/tmp006.h"

//*****************************************************************************
//
//! \addtogroup tmp006_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The states of the TMP006 state machine.
//
//*****************************************************************************
#define TMP006_STATE_IDLE       0
#define TMP006_STATE_INIT       1
#define TMP006_STATE_READ       2
#define TMP006_STATE_WRITE      3
#define TMP006_STATE_RMW        4
#define TMP006_STATE_READ_AMB   5
#define TMP006_STATE_READ_OBJ   6

//*****************************************************************************
//
// The constants used to calculate object temperature.
//
//*****************************************************************************
#define T_REF                   298.15
#define A1                      1.75e-03
#define A2                      -1.678e-05
#define B0                      -2.94e-05
#define B1                      -5.70e-07
#define B2                      4.63e-09
#define C2                      13.4

//*****************************************************************************
//
// The callback function that is called when I2C transations to/from the TMP006
// have completed.
//
//*****************************************************************************
static void
TMP006Callback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tTMP006 *psInst;

    //
    // Convert the instance data into a pointer to a tTMP006 structure.
    //
    psInst = pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result in a callback to propagate the
    // error).
    //
    if(ui8Status != I2CM_STATUS_SUCCESS)
    {
        psInst->ui8State = TMP006_STATE_IDLE;
    }

    //
    // Determine the current state of the TMP006 state machine.
    //
    switch(psInst->ui8State)
    {
        //
        // All states that trivially transition to IDLE, and all unknown
        // states.
        //
        case TMP006_STATE_INIT:
        case TMP006_STATE_READ:
        case TMP006_STATE_WRITE:
        case TMP006_STATE_RMW:
        case TMP006_STATE_READ_OBJ:
        default:
        {
            //
            // The state machine is now idle.
            //
            psInst->ui8State = TMP006_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        //
        // The ambient temperature was just read.
        //
        case TMP006_STATE_READ_AMB:
        {
            //
            // Move to the read object temperature state.
            //
            psInst->ui8State = TMP006_STATE_READ_OBJ;

            //
            // Start a read of the object temperature now that this read is
            // complete.
            //
            psInst->uCommand.pui8Buffer[0] = TMP006_O_VOBJECT;
            I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                     psInst->uCommand.pui8Buffer, 1, psInst->pui8Data + 2, 2,
                     TMP006Callback, psInst);

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == TMP006_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Initializes the TMP006 driver.
//!
//! \param psInst is a pointer to the TMP006 instance data.
//! \param psI2CInst is a pointer to the I2C driver instance data.
//! \param ui8I2CAddr is the I2C address of the TMP006 device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the TMP006 driver, preparing it for operation,
//! and initiates a reset of the TMP006 device, clearing any previous
//! configuration data.
//!
//! \return Returns 1 if the TMP006 driver was successfully initialized and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
TMP006Init(tTMP006 *psInst, tI2CMInstance *psI2CInst, uint_fast8_t ui8I2CAddr,
           tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Initialize the TMP006 instance structure
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8I2CAddr;
    psInst->ui8State = TMP006_STATE_INIT;

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Set the calibration factor to a reasonable estimate, applications
    // should perform a calibration in their environment and directly overwrite
    // this value after calling TMP006Init with the system specific value.
    //
    psInst->fCalibrationFactor = 6.40e-14;

    //
    // Load the data buffer to write the reset sequence
    //
    psInst->pui8Data[0] = TMP006_O_CONFIG;
    psInst->pui8Data[1] = (uint8_t)(TMP006_CONFIG_RESET_ASSERT >> 8);
    psInst->pui8Data[2] = (uint8_t)(TMP006_CONFIG_RESET_ASSERT & 0x00FF);

    //
    // Write the reset bit and issue a callback when finished.
    //
    if(I2CMWrite(psInst->psI2CInst, ui8I2CAddr, psInst->pui8Data, 3,
                 TMP006Callback, psInst) == 0)
    {
        //
        // I2CMWrite failed so reset TMP006 state and return zero to indicate
        // failure.
        //
        psInst->ui8State = TMP006_STATE_IDLE;
        return(0);
    }

    //
    // Success
    //
    return(1);
}

//*****************************************************************************
//
//! Reads data from TMP006 registers.
//!
//! \param psInst is a pointer to the TMP006 instance data.
//! \param ui8Reg is the first register to read.
//! \param pui16Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count the number of register values to read.
//! \param pfnCallback is the function to be called when data read is complete
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the TMP006.
//!
//! \note The TMP006 does not auto-increment the register pointer, so reads of
//! more than one value returns garbage for the subsequent values.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
TMP006Read(tTMP006 *psInst, uint_fast8_t ui8Reg, uint16_t *pui16Data,
           uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
           void *pvCallbackData)
{
    //
    // Return a failure if the TMP006 driver is not idle (in other words, there
    // is already an outstanding request to the TMP006).
    //
    if(psInst->ui8State != TMP006_STATE_IDLE)
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
    psInst->ui8State = TMP006_STATE_READ;

    //
    // Read the requested registers from the TMP006.
    //
    if(I2CMRead16BE(&(psInst->uCommand.sReadState), psInst->psI2CInst,
                    psInst->ui8Addr, ui8Reg, pui16Data, ui16Count,
                    TMP006Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = TMP006_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Writes data to TMP006 registers.
//!
//! \param psInst is a pointer to the TMP006 instance data.
//! \param ui8Reg is the first register to write.
//! \param pui16Data is a pointer to the 16-bit register data to write.
//! \param ui16Count is the number of 16-bit registers to write.
//! \param pfnCallback is the function to be called when the data has been
//! written (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the TMP006.  The first value in the \e pui16Data buffer contains the data
//! to be written into the \e ui8Reg register, the second value contains the
//! data to be written into the next register, and so on.
//!
//! \note The TMP006 does not auto-increment the register pointer, so writes of
//! more than one register are rejected by the TMP006.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
TMP006Write(tTMP006 *psInst, uint_fast8_t ui8Reg, const uint16_t *pui16Data,
            uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
            void *pvCallbackData)
{
    //
    // Return a failure if the TMP006 driver is not idle (in other words, there
    // is already an outstanding request to the TMP006).
    //
    if(psInst->ui8State != TMP006_STATE_IDLE)
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
    psInst->ui8State = TMP006_STATE_WRITE;

    //
    // Write the requested registers to the TMP006.
    //
    if(I2CMWrite16BE(&(psInst->uCommand.sWriteState), psInst->psI2CInst,
                     psInst->ui8Addr, ui8Reg, pui16Data, ui16Count,
                     TMP006Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = TMP006_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs a read-modify-write of a TMP006 register.
//!
//! \param psInst is a pointer to the TMP006 instance data.
//! \param ui8Reg is the register offset to read modify and write
//! \param ui16Mask is the bit mask that is ANDed with the current register
//! value.
//! \param ui16Value is the bit mask that is ORed with the result of the AND
//! operation.
//! \param pfnCallback is the function to be called when the data has been
//! changed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function changes the value of a register in the TMP006 via a
//! read-modify-write operation, allowing one of the fields to be changed
//! without disturbing the other fields.  The \e ui8Reg register is read, ANDed
//! with \e ui16Mask, ORed with \e ui16Value, and then written back to the
//! TMP006.
//!
//! \return Returns 1 if the read-modify-write was successfully started and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
TMP006ReadModifyWrite(tTMP006 *psInst, uint_fast8_t ui8Reg,
                      uint_fast16_t ui16Mask, uint_fast16_t ui16Value,
                      tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the TMP006 driver is not idle (in other words, there
    // is already an outstanding request to the TMP006).
    //
    if(psInst->ui8State != TMP006_STATE_IDLE)
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
    psInst->ui8State = TMP006_STATE_RMW;

    //
    // Submit the read-modify-write request to the TMP006.
    //
    if(I2CMReadModifyWrite16BE(&(psInst->uCommand.sReadModifyWriteState),
                               psInst->psI2CInst, psInst->ui8Addr, ui8Reg,
                               ui16Mask, ui16Value, TMP006Callback,
                               psInst) == 0)
    {
        //
        // The I2C read-modify-write failed, so move to the idle state and
        // return a failure.
        //
        psInst->ui8State = TMP006_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads the temperature data from the TMP006.
//!
//! \param psInst is a pointer to the TMP006 instance data.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read of the TMP006 data registers.  When the read
//! has completed (as indicated by calling the callback function), the new
//! readings can be obtained via:
//!
//! - TMP006DataTemperatureGetRaw()
//! - TMP006DataTemperatureGetFloat()
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
TMP006DataRead(tTMP006 *psInst, tSensorCallback *pfnCallback,
               void *pvCallbackData)
{
    //
    // Return a failure if the TMP006 driver is not idle (in other words, there
    // is already an outstanding request to the TMP006).
    //
    if(psInst->ui8State != TMP006_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Move the state machine to the wait for ambient data read state.
    //
    psInst->ui8State = TMP006_STATE_READ_AMB;

    //
    // Read the ambient temperature data from the TMP006.
    //
    psInst->uCommand.pui8Buffer[0] = TMP006_O_TAMBIENT;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, psInst->pui8Data, 2,
                TMP006Callback, psInst) == 0)
    {
        //
        // The I2C read failed, so move to the idle state and return a failure.
        //
        psInst->ui8State = TMP006_STATE_IDLE;
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
//! \param psInst is a pointer to the TMP006 instance data.
//! \param pi16Ambient is a pointer to the value into which the raw ambient
//! temperature data is stored.
//! \param pi16Object is a pointer to the value into which the raw object
//! temperature data is stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
TMP006DataTemperatureGetRaw(tTMP006 *psInst, int16_t *pi16Ambient,
                            int16_t *pi16Object)
{
    //
    // Return the raw temperature value.
    //
    *pi16Ambient = ((int16_t)psInst->pui8Data[0] << 8) | psInst->pui8Data[1];
    *pi16Object = ((int16_t)psInst->pui8Data[2] << 8) | psInst->pui8Data[3];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the TMP006 instance data.
//! \param pfAmbient is a pointer to the value into which the ambient
//! temperature data is stored as floating point degrees Celsius.
//! \param pfObject is a pointer to the value into which the object temperature
//! data is stored as floating point degrees Celsius.
//!
//! This function returns the temperature data from the most recent data read,
//! converted into Celsius.
//!
//! \return None.
//
//*****************************************************************************
void
TMP006DataTemperatureGetFloat(tTMP006 *psInst, float *pfAmbient,
                              float *pfObject)
{
    float fTdie2, fS, fVo, fVx, fObj;
    int16_t i16Ambient;
    int16_t i16Object;

    //
    // Get the raw readings.
    //
    TMP006DataTemperatureGetRaw(psInst, &i16Ambient, &i16Object);

    //
    // The bottom two bits are not temperature data, so discard them but keep
    // the sign information.
    //
    *pfAmbient = (float)(i16Ambient / 4);

    //
    // Divide by 32 to get unit scaling correct.
    //
    *pfAmbient = *pfAmbient / 32.0;

    //
    // fTdie2 is measured ambient temperature in degrees Kelvin.
    //
    fTdie2 = *pfAmbient + T_REF;

    //
    // S is the sensitivity.
    //
    fS = psInst->fCalibrationFactor * (1.0f + (A1 * (*pfAmbient)) +
                                       (A2 * ((*pfAmbient) * (*pfAmbient))));

    //
    // Vos is the offset voltage.
    //
    fVo = B0 + (B1 * (*pfAmbient)) + (B2 * ((*pfAmbient) * (*pfAmbient)));

    //
    // Vx is the difference between raw object voltage and Vos
    // 156.25e-9 is nanovolts per least significant bit from the voltage
    // register.
    //
    fVx = (((float) i16Object) * 156.25e-9) - fVo;

    //
    // fObj is the feedback coefficient.
    //
    fObj = fVx + C2 * (fVx * fVx);

    //
    // Finally calculate the object temperature.
    //
    *pfObject = (sqrtf(sqrtf((fTdie2 * fTdie2 * fTdie2 * fTdie2) +
                             (fObj / fS))) - T_REF);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
