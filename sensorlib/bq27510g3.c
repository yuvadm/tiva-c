//*****************************************************************************
//
// bq27510g3.c - Driver for the TI BQ27510G3 Battery Fuel Gauge
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
#include "sensorlib/hw_bq27510g3.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/bq27510g3.h"

//*****************************************************************************
//
//! \addtogroup bq27510g3_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The states of the BQ27510G3 state machine.
//
//*****************************************************************************
#define BQ27510G3_STATE_IDLE         0
#define BQ27510G3_STATE_INIT         1
#define BQ27510G3_STATE_READ         2
#define BQ27510G3_STATE_WRITE        3
#define BQ27510G3_STATE_RMW          4
#define BQ27510G3_STATE_READ_DATA_1  5
#define BQ27510G3_STATE_READ_DATA_2  6
#define BQ27510G3_STATE_READ_DATA_3  7

//*****************************************************************************
//
// The constants used to calculate object temperature.
//
//*****************************************************************************
#define T_REF                   273

//*****************************************************************************
//
// The callback function that is called when I2C transactions to/from the
// BQ27510G3 have completed.
//
//*****************************************************************************
static void
BQ27510G3Callback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tBQ27510G3 *psInst;

    //
    // Convert the instance data into a pointer to a tBQ27510G3 structure.
    //
    psInst = pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result in a callback to propagate the
    // error).
    //
    if(ui8Status != I2CM_STATUS_SUCCESS)
    {
        psInst->ui8State = BQ27510G3_STATE_IDLE;
    }

    //
    // Determine the current state of the BQ27510G3 state machine.
    //
    switch(psInst->ui8State)
    {
        //
        // The first data read state, has finished setup and trigger data read
        // state 2.
        //
        case BQ27510G3_STATE_READ_DATA_1:
        {
            //
            // Move the state machine to the next read state.
            //
            psInst->ui8State = BQ27510G3_STATE_READ_DATA_2;

            //
            // Read the requested data from the BQ27510G3.
            //
            psInst->uCommand.pui8Buffer[0] = BQ27510G3_O_NOM_AV_CAP_LSB;
            I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                        psInst->uCommand.pui8Buffer, 1, psInst->pui8Data + 6,
                        24, BQ27510G3Callback, psInst);

            //
            // break
            //
            break;
        }

        //
        // The 2nd data read state, has finished setup and trigger data read
        // state 3.  Read state 3 is the final state and when done will return
        // to idle and trigger the application level callback.
        //
        case BQ27510G3_STATE_READ_DATA_2:
        {
            //
            // Move the state machine to the next read state.
            //
            psInst->ui8State = BQ27510G3_STATE_READ_DATA_3;

            //
            // Read the requested data from the BQ27510G3.
            //
            psInst->uCommand.pui8Buffer[0] = BQ27510G3_O_INT_TEMP_LSB;
            I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                        psInst->uCommand.pui8Buffer, 1, psInst->pui8Data + 30,
                        2, BQ27510G3Callback, psInst);

            //
            // break
            //
            break;
        }

        //
        // All states that trivially transition to IDLE, and all unknown
        // states.
        //
        case BQ27510G3_STATE_INIT:
        case BQ27510G3_STATE_READ:
        case BQ27510G3_STATE_WRITE:
        case BQ27510G3_STATE_READ_DATA_3:
        case BQ27510G3_STATE_RMW:
        default:
        {
            //
            // The state machine is now idle.
            //
            psInst->ui8State = BQ27510G3_STATE_IDLE;

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == BQ27510G3_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Initializes the BQ27510G3 driver.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param psI2CInst is a pointer to the I2C driver instance data.
//! \param ui8I2CAddr is the I2C address of the BQ27510G3 device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the BQ27510G3 driver, preparing it for operation.
//!
//! \return Returns 1 if the BQ27510G3 driver was successfully initialized and
//! 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
BQ27510G3Init(tBQ27510G3 *psInst, tI2CMInstance *psI2CInst,
              uint_fast8_t ui8I2CAddr, tSensorCallback *pfnCallback,
              void *pvCallbackData)
{
    //
    // Initialize the BQ27510G3 instance structure
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8I2CAddr;
    psInst->ui8State = BQ27510G3_STATE_IDLE;

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // The default settings are ok.  Return success and call the callback.
    //
    if(pfnCallback)
    {
        pfnCallback(pvCallbackData, 0);
    }

    //
    // Success
    //
    return(1);
}

//*****************************************************************************
//
//! Reads data from BQ27510G3 registers.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param ui8Reg is the first register to read.
//! \param pui16Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count the number of register values to read.
//! \param pfnCallback is the function to be called when data read is complete
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the BQ27510G3.
//!
//! \note The BQ27510G3 does not auto-increment the register pointer, so reads
//! of more than one value returns garbage for the subsequent values.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
BQ27510G3Read(tBQ27510G3 *psInst, uint_fast8_t ui8Reg, uint16_t *pui16Data,
           uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
           void *pvCallbackData)
{
    //
    // Return a failure if the BQ27510G3 driver is not idle (in other words,
    // there is already an outstanding request to the BQ27510G3).
    //
    if(psInst->ui8State != BQ27510G3_STATE_IDLE)
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
    psInst->ui8State = BQ27510G3_STATE_READ;

    //
    // Read the requested registers from the BQ27510G3.
    //
    if(I2CMRead16BE(&(psInst->uCommand.sReadState), psInst->psI2CInst,
                    psInst->ui8Addr, ui8Reg, pui16Data, ui16Count,
                    BQ27510G3Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = BQ27510G3_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Writes data to BQ27510G3 registers.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param ui8Reg is the first register to write.
//! \param pui16Data is a pointer to the 16-bit register data to write.
//! \param ui16Count is the number of 16-bit registers to write.
//! \param pfnCallback is the function to be called when the data has been
//! written (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the BQ27510G3.  The first value in the \e pui16Data buffer contains the
//! data to be written into the \e ui8Reg register, the second value contains
//! the data to be written into the next register, and so on.
//!
//! \note The BQ27510G3 does not auto-increment the register pointer, so writes
//! of more than one register are rejected by the BQ27510G3.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
BQ27510G3Write(tBQ27510G3 *psInst, uint_fast8_t ui8Reg, 
               const uint16_t *pui16Data, uint_fast16_t ui16Count,
               tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the BQ27510G3 driver is not idle (in other words,
    // there is already an outstanding request to the BQ27510G3).
    //
    if(psInst->ui8State != BQ27510G3_STATE_IDLE)
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
    psInst->ui8State = BQ27510G3_STATE_WRITE;

    //
    // Write the requested registers to the BQ27510G3.
    //
    if(I2CMWrite16BE(&(psInst->uCommand.sWriteState), psInst->psI2CInst,
                     psInst->ui8Addr, ui8Reg, pui16Data, ui16Count,
                     BQ27510G3Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = BQ27510G3_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs a read-modify-write of a BQ27510G3 register.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param ui8Reg is the register offset to read modify and write
//! \param ui16Mask is the bit mask that is ANDed with the current register
//! value.
//! \param ui16Value is the bit mask that is ORed with the result of the AND
//! operation.
//! \param pfnCallback is the function to be called when the data has been
//! changed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function changes the value of a register in the BQ27510G3 via a
//! read-modify-write operation, allowing one of the fields to be changed
//! without disturbing the other fields.  The \e ui8Reg register is read, ANDed
//! with \e ui16Mask, ORed with \e ui16Value, and then written back to the
//! BQ27510G3.
//!
//! \return Returns 1 if the read-modify-write was successfully started and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
BQ27510G3ReadModifyWrite(tBQ27510G3 *psInst, uint_fast8_t ui8Reg,
                      uint_fast16_t ui16Mask, uint_fast16_t ui16Value,
                      tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the BQ27510G3 driver is not idle (in other words,
    // there is already an outstanding request to the BQ27510G3).
    //
    if(psInst->ui8State != BQ27510G3_STATE_IDLE)
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
    psInst->ui8State = BQ27510G3_STATE_RMW;

    //
    // Submit the read-modify-write request to the BQ27510G3.
    //
    if(I2CMReadModifyWrite16BE(&(psInst->uCommand.sReadModifyWriteState),
                               psInst->psI2CInst, psInst->ui8Addr, ui8Reg,
                               ui16Mask, ui16Value, BQ27510G3Callback,
                               psInst) == 0)
    {
        //
        // The I2C read-modify-write failed, so move to the idle state and
        // return a failure.
        //
        psInst->ui8State = BQ27510G3_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs a read of a BQ27510G3 data register.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read of the BQ27510G3 data registers.  When the
//! read has completed (as indicated by calling the callback function), the new
//! readings can be obtained via functions like:
//!
//! - BQ27510G3DataTCurrentInstantaneousGetRaw()
//! - BQ27510G3DataTCurrentInstantaneousGetFloat()
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
BQ27510G3DataRead(tBQ27510G3 *psInst, tSensorCallback *pfnCallback,
                  void *pvCallbackData)
{
    //
    // Return a failure if the BQ27510G3 driver is not idle (in other words,
    // there is already an outstanding request to the BQ27510G3).
    //
    if(psInst->ui8State != BQ27510G3_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Move the state machine to the first read state.  Reads are done in three
    // parts based on address ranges of the information being read.
    //
    psInst->ui8State = BQ27510G3_STATE_READ_DATA_1;

    //
    // Read the requested data from the BQ27510G3.
    //
    psInst->uCommand.pui8Buffer[0] = BQ27510G3_O_AT_RATE_TTE_LSB;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, psInst->pui8Data, 6,
                BQ27510G3Callback, psInst) == 0)
    {
        //
        // The I2C read failed, so move to the idle state and return a failure.
        //
        psInst->ui8State = BQ27510G3_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Gets the raw "at rate time to empty" data.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataAtRateTimeToEmptyGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[1] << 8) | psInst->pui8Data[0];
}

//*****************************************************************************
//
//! Gets the "at rate time to empty" data as a floating point value.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the data from the most recent data read,
//! converted into float value. Units are minutes.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataAtRateTimeToEmptyGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataAtRateTimeToEmptyGetRaw(psInst, &i16Data);

    //
    // Covert to float.
    //
    *pfData = (float)(i16Data);

}

//*****************************************************************************
//
//! Gets the raw battery temperature from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataTemperatureBatteryGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[3] << 8) | psInst->pui8Data[2];
}

//*****************************************************************************
//
//! Gets the battery temperature measurement data from the most recent  data
//! read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the data from the most recent data read,
//! converted into float value. Units are degrees Celsius.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataTemperatureBatteryGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataTemperatureBatteryGetRaw(psInst, &i16Data);

    //
    // Device returns the units as 0.1 degrees K.  Convert first to whole
    // degrees then from K to C
    //
    *pfData = (float)(i16Data);
    *pfData = *pfData / 10.0f;
    *pfData -= 272.15f;


}

//*****************************************************************************
//
//! Gets the raw battery voltage measurement data from the most recent data
//! read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataVoltageBatteryGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[5] << 8) | psInst->pui8Data[4];
}

//*****************************************************************************
//
//! Gets the battery voltage measurement from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the data from the most recent data read,
//! converted into float value. Units are volts.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataVoltageBatteryGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataVoltageBatteryGetRaw(psInst, &i16Data);

    //
    // Covert to float.
    //
    *pfData = (float)(i16Data);
    *pfData = *pfData / 1000.0f;

}

//*****************************************************************************
//
//! Gets the raw nominal available capacity measurement from the most recent
//! data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCapacityNominalAvailableGetRaw(tBQ27510G3 *psInst,
                                            int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[7] << 8) | psInst->pui8Data[6];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the data from the most recent data read,
//! converted into float value. Units are amp-hours (Ah).
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCapacityNominalAvailableGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataCapacityNominalAvailableGetRaw(psInst, &i16Data);

    //
    // Covert to float.
    //
    *pfData = (float)(i16Data);
    *pfData = *pfData / 1000.0f;

}

//*****************************************************************************
//
//! Gets the raw available capacity of a new battery from the most recent data
//! read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCapacityFullAvailableGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[9] << 8) | psInst->pui8Data[8];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the data from the most recent data read,
//! converted into float value. Units are amp-hours (Ah).
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCapacityFullAvailableGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataCapacityFullAvailableGetRaw(psInst, &i16Data);

    //
    // Covert to float.
    //
    *pfData = (float)(i16Data);
    *pfData = *pfData / 1000.0f;

}

//*****************************************************************************
//
//! Gets the raw remaining capacity of measurement from the most recent data
//! read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCapacityRemainingGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[11] << 8) | psInst->pui8Data[10];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the data from the most recent data read,
//! converted into float value. Units are amp-hours (Ah).
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCapacityRemainingGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataCapacityRemainingGetRaw(psInst, &i16Data);

    //
    // Covert to float.
    //
    *pfData = (float)(i16Data);
    *pfData = *pfData / 1000.0f;

}

//*****************************************************************************
//
//! Gets the raw full charge capacity from the most recent data
//! read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCapacityFullChargeGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[13] << 8) | psInst->pui8Data[12];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the data from the most recent data read,
//! converted into float value. Units are amp-hours (Ah).
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCapacityFullChargeGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataCapacityFullChargeGetRaw(psInst, &i16Data);

    //
    // Covert to float.
    //
    *pfData = (float)(i16Data);
    *pfData = *pfData / 1000.0f;

}


//*****************************************************************************
//
//! Gets the raw average current measurement from the most recent data
//! read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCurrentAverageGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[15] << 8) | psInst->pui8Data[14];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the data from the most recent data read,
//! converted into float value. Units are amps.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCurrentAverageGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataCurrentAverageGetRaw(psInst, &i16Data);

    //
    // Covert to float.
    //
    *pfData = (float)(i16Data);
    *pfData = *pfData / 1000.0f;

}


//*****************************************************************************
//
//! Gets the raw time to empty estimate from the most recent data
//! read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataTimeToEmptyGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[17] << 8) | psInst->pui8Data[16];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the data from the most recent data read,
//! converted into float value. Units are minutes. Value of 65,535 indicates
//! battery is not being discharged.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataTimeToEmptyGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataTimeToEmptyGetRaw(psInst, &i16Data);

    //
    // Covert to float.
    //
    *pfData = (float)(i16Data);

}

//*****************************************************************************
//
//! Gets the raw standby current from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCurrentStandbyGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[19] << 8) | psInst->pui8Data[18];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the data from the most recent data read,
//! converted into float value. Units are amps.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCurrentStandbyGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataCurrentStandbyGetRaw(psInst, &i16Data);

    //
    // Covert to float.
    //
    *pfData = (float)(i16Data);
    *pfData = *pfData / 1000.0f;

}

//*****************************************************************************
//
//! Gets the raw standby time to empty data from the most recent data
//! read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataTimeToEmptyStandbyGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[21] << 8) | psInst->pui8Data[20];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the data from the most recent data read,
//! converted into float value. Units are minutes.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataTimeToEmptyStandbyGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataTimeToEmptyStandbyGetRaw(psInst, &i16Data);

    //
    // Covert to float.
    //
    *pfData = (float)(i16Data);

}

//*****************************************************************************
//
//! Gets the raw cycle count data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCycleCountGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[25] << 8) | psInst->pui8Data[24];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the data from the most recent data read, converted
//! into float value. This data does not have units.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCycleCountGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataCycleCountGetRaw(psInst, &i16Data);

    //
    // Covert to float.
    //
    *pfData = (float)(i16Data);

}

//*****************************************************************************
//
//! Gets the raw health data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataHealthGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[23] << 8) | psInst->pui8Data[22];
}

//*****************************************************************************
//
//! Gets the health data from the most recent health data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfHealth is a pointer to the value into which the battery
//! health data is stored as floating point ratio of current/design capacity.
//!
//! This function returns the health data from the most recent data read,
//! converted into percent health. The health status bits are dropped. These
//! can be obtained with BQ27510G3DataHealthGetRaw function.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataHealthGetFloat(tBQ27510G3 *psInst, float *pfHealth)
{
    int16_t i16Health;

    //
    // Get the raw readings.
    //
    BQ27510G3DataHealthGetRaw(psInst, &i16Health);

    //
    // Mask off health bit field
    //
    *pfHealth = (float)(i16Health & 0xFF);

}

//*****************************************************************************
//
//! Gets the raw charge state data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataChargeStateGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[27] << 8) | psInst->pui8Data[26];
}

//*****************************************************************************
//
//! Gets the charge state from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the charge state from the most recent data read,
//! converted into percent charged.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataChargeStateGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataChargeStateGetRaw(psInst, &i16Data);

    //
    // Convert to floating point.
    //
    *pfData = (float)(i16Data);

}

//*****************************************************************************
//
//! Gets the instantaneous current data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCurrentInstantaneousGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[29] << 8) | psInst->pui8Data[28];
}

//*****************************************************************************
//
//! Gets the instantaneous current data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the current measurement from the most recent data
//! read, converted into floating point amps.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataCurrentInstantaneousGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataCurrentInstantaneousGetRaw(psInst, &i16Data);

    //
    // Convert to floating point.
    //
    *pfData = (float)(i16Data);
    *pfData /= 1000.0f;

}

//*****************************************************************************
//
//! Gets the raw internal temparature data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pi16Data is a pointer to the value into which the raw data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataTemperatureInternalGetRaw(tBQ27510G3 *psInst, int16_t *pi16Data)
{
    //
    // Return the raw data value.
    //
    *pi16Data = ((int16_t)psInst->pui8Data[31] << 8) | psInst->pui8Data[30];
}

//*****************************************************************************
//
//! Gets the internal temperature data from the most recent data read.
//!
//! \param psInst is a pointer to the BQ27510G3 instance data.
//! \param pfData is a pointer to the value into which the data is stored as
//! floating point.
//!
//! This function returns the internal temperature from the most recent data
//! read.
//!
//! \return None.
//
//*****************************************************************************
void
BQ27510G3DataTemperatureInternalGetFloat(tBQ27510G3 *psInst, float *pfData)
{
    int16_t i16Data;

    //
    // Get the raw readings.
    //
    BQ27510G3DataTemperatureInternalGetRaw(psInst, &i16Data);

    //
    // Convert to floating point Kelvin, then Celsius.
    //
    *pfData = (float)(i16Data);
    *pfData = *pfData / 10.0f;
    *pfData -= 272.15f;



}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
