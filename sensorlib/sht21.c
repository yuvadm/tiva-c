//*****************************************************************************
//
// sht21.c - Driver for the SHT21 accelerometer.
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
#include "sensorlib/hw_sht21.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/sht21.h"

//*****************************************************************************
//
//! \addtogroup sht21_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The states of the SHT21 state machine.
//
//*****************************************************************************
#define SHT21_STATE_IDLE        0           // State machine is idle
#define SHT21_STATE_INIT        1           // Waiting for initialization
#define SHT21_STATE_READ        2           // Waiting for register read
#define SHT21_STATE_WRITE       3           // Waiting for register write
#define SHT21_STATE_RMW         4
#define SHT21_STATE_READ_DATA   5           // Waiting for temperature or
                                            // humidity data

//*****************************************************************************
//
// The callback function that is called when I2C transactions to/from the
// SHT21 have completed.
//
//*****************************************************************************
static void
SHT21Callback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tSHT21 *psInst;

    //
    // Convert the instance data into a pointer to a tSHT221 structure.
    //
    psInst = (tSHT21 *)pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result in a callback to propagate the
    // error).
    //
    if(ui8Status != I2CM_STATUS_SUCCESS)
    {
        psInst->ui8State = SHT21_STATE_IDLE;
    }

    //
    // Determine the current state of the SHT21 state machine.
    //
    switch(psInst->ui8State)
    {
        //
        // All states that trivially transition to IDLE, and all unknown
        // states.
        //
        case SHT21_STATE_INIT:
        case SHT21_STATE_READ:
        case SHT21_STATE_WRITE:
        case SHT21_STATE_READ_DATA:
        case SHT21_STATE_RMW:
        default:
        {
            //
            // The state machine is now idle.
            //
            psInst->ui8State = SHT21_STATE_IDLE;

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == SHT21_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Initializes the SHT21 driver.
//!
//! \param psInst is a pointer to the SHT21 instance data.
//! \param psI2CInst is a pointer to the I2C driver instance data.
//! \param ui8I2CAddr is the I2C address of the SHT21 device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the SHT21 driver, preparing it for operation, and
//! initiates a reset of the SHT21 device, clearing any previous configuration
//! data.
//!
//! \return Returns 1 if the SHT21 driver was successfully initialized and 0 if
//! it was not.
//
//*****************************************************************************
uint_fast8_t
SHT21Init(tSHT21 *psInst, tI2CMInstance *psI2CInst, uint_fast8_t ui8I2CAddr,
          tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Initialize the SHT21 instance structure.
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8I2CAddr;
    psInst->ui8State = SHT21_STATE_INIT;

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Perform a soft reset of the SHT21.
    //
    psInst->pui8Data[0] = SHT21_CMD_SOFT_RESET;
    if(I2CMWrite(psInst->psI2CInst, ui8I2CAddr, psInst->pui8Data, 1,
                 SHT21Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = SHT21_STATE_IDLE;
        return(0);
    }

    //
    // Success
    //
    return(1);
}

//*****************************************************************************
//
//! Reads data from SHT21 registers.
//!
//! \param psInst is a pointer to the SHT21 instance data.
//! \param ui8Reg is the first register to read.
//! \param pui8Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count the number of data bytes to read.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData pointer that is passed to the callback function.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the SHT21.
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
SHT21Read(tSHT21 *psInst, uint_fast8_t ui8Reg, uint8_t *pui8Data,
          uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
          void *pvCallbackData)
{
    //
    // Return a failure if the SHT21 driver is not idle (in other words, there
    // is already an outstanding request to the SHT21).
    //
    if(psInst->ui8State != SHT21_STATE_IDLE)
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
    psInst->ui8State = SHT21_STATE_READ;

    //
    // Read the requested registers from the SHT21.
    //
    psInst->uCommand.pui8Buffer[0] = ui8Reg;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, pui8Data, ui16Count,
                SHT21Callback, (void *)psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = SHT21_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Writes data to SHT21 registers.
//!
//! \param psInst is a pointer to the SHT21 instance data.
//! \param ui8Reg is the register offset to be written.
//! \param pui8Data is the data buffer bytes to write.
//! \param ui16Count is the number of bytes to write.
//! \param pfnCallback is the function to be called when the data has been
//! written (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the SHT21.  The first byte of the \e pui8Data buffer contains the value to
//! be written into the \e ui8Reg register, the second value contains the data
//! to be written into the next register, and so on.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
SHT21Write(tSHT21 *psInst, uint_fast8_t ui8Reg, const uint8_t *pui8Data,
           uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
           void *pvCallbackData)
{
    //
    // Return a failure if the SHT21 driver is not idle (in other words, there
    // is already an outstanding request to the SHT21).
    //
    if(psInst->ui8State != SHT21_STATE_IDLE)
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
    psInst->ui8State = SHT21_STATE_WRITE;

    //
    // Write the requested registers to the SHT21.
    //
    if(I2CMWrite8(&(psInst->uCommand.sWriteState), psInst->psI2CInst,
                  psInst->ui8Addr, ui8Reg, pui8Data, ui16Count, SHT21Callback,
                  psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = SHT21_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs a read-modify-write of a SHT21 register.
//!
//! \param psInst is a pointer to the SHT21 instance data.
//! \param ui8Reg is the register to modify.
//! \param ui8Mask is the bit mask that is ANDed with the current register
//! value.
//! \param ui8Value is the bit mask that is ORed with the result of the AND
//! operation.
//! \param pfnCallback is the function to be called when the data has been
//! changed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function changes the value of a register in the SHT21 via a
//! read-modify-write operation, allowing one of the fields to be changed
//! without disturbing the other fields.  The \e ui8Reg register is read, ANDed
//! with \e ui8Mask, ORed with \e ui8Value, and then written back to the SHT21.
//!
//! \return Returns 1 if the read-modify-write was successfully started and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
SHT21ReadModifyWrite(tSHT21 *psInst, uint_fast8_t ui8Reg, uint_fast8_t ui8Mask,
                     uint_fast8_t ui8Value, tSensorCallback *pfnCallback,
                     void *pvCallbackData)
{
    //
    // Return a failure if the SHT21 driver is not idle (in other words, there
    // is already an outstanding request to the SHT21).
    //
    if(psInst->ui8State != SHT21_STATE_IDLE)
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
    psInst->ui8State = SHT21_STATE_RMW;

    //
    // Submit the read-modify-write request to the TMP006.
    //
    if(I2CMReadModifyWrite8(&(psInst->uCommand.sReadModifyWriteState),
                            psInst->psI2CInst, psInst->ui8Addr, ui8Reg,
                            ui8Mask, ui8Value, SHT21Callback, psInst) == 0)
    {
        //
        // The I2C read-modify-write failed, so move to the idle state and
        // return a failure.
        //
        psInst->ui8State = SHT21_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads the temperature and humidity data from the SHT21.
//!
//! \param psInst is a pointer to the SHT21 instance data
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read of the SHT21 data registers.  The user must
//! first initiate a measurement by using the SHT21Write() function configured
//! to write the command for a humidity or temperature measurement.  In the
//! case of a measurement with I2C bus hold, this function is not needed.  When
//! the read has completed (as indicated by callback function), the new
//! readings can be obtained via:
//!
//! - SHT21DataTemperatureGetRaw()
//! - SHT21DataTemperatureGetFloat()
//! - SHT21DataHumidityGetRaw()
//! - SHT21DataHumidityGetFloat()
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
SHT21DataRead(tSHT21 *psInst, tSensorCallback *pfnCallback,
              void *pvCallbackData)
{
    //
    // Return a failure if the SHT21 driver is not idle (in other words, there
    // is already an outstanding request to the SHT21).
    //
    if(psInst->ui8State != SHT21_STATE_IDLE)
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
    psInst->ui8State = SHT21_STATE_READ_DATA;

    //
    // Read the data registers from the SHT21.
    //
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr, 0, 0, psInst->pui8Data, 2,
                SHT21Callback, psInst) == 0)
    {
        psInst->ui8State = SHT21_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Returns the raw temperature measurement as received from the SHT21.
//!
//! \param psInst is a pointer to the SHT21 instance data.
//! \param pui16Temperature is a pointer to the value into which the raw
//! temperature data is stored.
//!
//! This function returns the raw temperature data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
SHT21DataTemperatureGetRaw(tSHT21 *psInst, uint16_t *pui16Temperature)
{
    //
    // Return the raw temperature value.
    //
    *pui16Temperature = ((((uint16_t)psInst->pui8Data[0]) << 8) |
                         (uint16_t)psInst->pui8Data[1]);
}

//*****************************************************************************
//
//! Returns the most recent temperature measurement in floating point degrees
//! Celsius.
//!
//! \param psInst is a pointer to the SHT21 instance data.
//! \param pfTemperature is a pointer to the value into which the temperature
//! data is stored.
//!
//! This function converts the raw temperature measurement data into floating
//! point degrees Celsius and returns the result.  See the SHT21 datasheet
//! section 6.2 for more information about the conversion formula used.
//!
//! \return None.
//
//*****************************************************************************
void
SHT21DataTemperatureGetFloat(tSHT21 *psInst, float *pfTemperature)
{
    uint16_t ui16TemperatureRaw;

    //
    // Get the raw temperature into a floating point variable
    //
    SHT21DataTemperatureGetRaw(psInst, &ui16TemperatureRaw);
    *pfTemperature = (float)(ui16TemperatureRaw & 0xFFFC);

    //
    // Equation from SHT21 datasheet for raw to Celsius conversion.
    //
    *pfTemperature = -46.85 + 175.72 * (*pfTemperature / 65536.0);
}

//*****************************************************************************
//
//! Returns the raw humidity measurement from the SHT21.
//!
//! \param psInst is a pointer to the SHT21 instance data.
//! \param pui16Humidity is a pointer to the value into which the raw humidity
//! data is stored.
//!
//! This function returns the raw humidity data from the most recent data read.
//! The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
SHT21DataHumidityGetRaw(tSHT21 *psInst, uint16_t *pui16Humidity)
{
    //
    // Return the raw humidity value.
    //
    *pui16Humidity = ((((uint16_t)psInst->pui8Data[0]) << 8) |
                      (uint16_t)psInst->pui8Data[1]);
}

//*****************************************************************************
//
//! Returns the relative humidity measurement as a floating point percentage.
//!
//! \param psInst pointer to the SHT21 instance data.
//! \param pfHumidity is a pointer to the value into which the humidity data
//! is stored.
//!
//! This function converts the raw humidity measurement to
//! floating-point-percentage relative humidity over water.  For more
//! information on the conversion algorithm see the SHT21 datasheet section
//! 6.1.
//!
//! \return None.
//
//*****************************************************************************
void
SHT21DataHumidityGetFloat(tSHT21 *psInst, float *pfHumidity)
{
    uint16_t ui16HumidityRaw;

    //
    // Convert the raw measure to float for later math.
    //
    SHT21DataHumidityGetRaw(psInst, &ui16HumidityRaw);
    *pfHumidity = (float)(ui16HumidityRaw & 0xFFFC);

    //
    // Convert from raw measurement to percent relative humidity over water
    // per the datasheet formula.
    //
    *pfHumidity = -6.0 + 125.0 * (*pfHumidity / 65536.0);

    //
    // Convert to a number from 0 to 1.0 instead of 0 to 100%.
    //
    *pfHumidity /= 100.0;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
