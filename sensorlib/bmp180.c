//*****************************************************************************
//
// bmp180.c - Driver for the BMP180 pressure sensor.
//
// Copyright (c) 2012-2014 Texas Instruments Incorporated.  All rights reserved.
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
#include "sensorlib/hw_bmp180.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/bmp180.h"

//*****************************************************************************
//
//! \addtogroup bmp180_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The states of the BMP180 state machine.
//
//*****************************************************************************
#define BMP180_STATE_IDLE      0           // State machine is idle
#define BMP180_STATE_INIT1     1           // Waiting for initialization 1
#define BMP180_STATE_INIT2     2           // Waiting for initialization 2
#define BMP180_STATE_READ      3           // Waiting for read
#define BMP180_STATE_WRITE     4           // Waiting for write
#define BMP180_STATE_RMW       5           // Waiting for read-modify-write
#define BMP180_STATE_REQ_TEMP  6           // Requested temperature
#define BMP180_STATE_WAIT_TEMP 7           // Waiting for temperature ready
#define BMP180_STATE_READ_TEMP 8           // Reading temperature value
#define BMP180_STATE_REQ_PRES  9           // Requested pressure
#define BMP180_STATE_WAIT_PRES 10          // Waiting for pressure ready
#define BMP180_STATE_READ_PRES 11          // Reading pressure value

//*****************************************************************************
//
// The callback function that is called when I2C transations to/from the
// BMP180 have completed.
//
//*****************************************************************************
static void
BMP180Callback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tBMP180 *psInst;
    uint16_t ui16ReadVerify;

    //
    // Convert the instance data into a pointer to a tBMP180 structure.
    //
    psInst = pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result in a callback to propagate the
    // error).
    //
    if(ui8Status != I2CM_STATUS_SUCCESS)
    {
        psInst->ui8State = BMP180_STATE_IDLE;
    }

    //
    // Determine the current state of the BMP180 state machine.
    //
    switch(psInst->ui8State)
    {
        //
        // All states that trivially transition to IDLE, and all unknown
        // states.
        //
        case BMP180_STATE_READ:
        case BMP180_STATE_READ_PRES:
        default:
        {
            //
            // The state machine is now idle.
            //
            psInst->ui8State = BMP180_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        //
        // The first step of initialization has just completed.
        //
        case BMP180_STATE_INIT1:
        {
            //
            // Read the calibration data from the BMP180.
            //
            psInst->pui8Data[0] = BMP180_O_AC1_MSB;
            I2CMRead(psInst->psI2CInst, psInst->ui8Addr, psInst->pui8Data, 1,
                     psInst->uCommand.pui8Buffer, 22, BMP180Callback, psInst);

            //
            // Move to the wait for initialization step 2 state.
            //
            psInst->ui8State = BMP180_STATE_INIT2;

            //
            // Done.
            //
            break;
        }

        //
        // The second step of initialization has just completed.
        //
        case BMP180_STATE_INIT2:
        {
            //
            // Data communication is checked by verifying that the calibration
            // data is neither 0 nor 0xFFFF.  This is used to check that reset
            // is complete and the part is ready.  It also verifies that we
            // have valid calibration data before proceeding.
            //
            ui16ReadVerify = psInst->uCommand.pui8Buffer[0];
            ui16ReadVerify <<= 8;
            ui16ReadVerify |= psInst->uCommand.pui8Buffer[1];
            if((ui16ReadVerify == 0) || (ui16ReadVerify == 0xFFFF))
            {
                //
                // Reread the calibration data from the BMP180.
                //
                psInst->pui8Data[0] = BMP180_O_AC1_MSB;
                I2CMRead(psInst->psI2CInst, psInst->ui8Addr, psInst->pui8Data,
                         1, psInst->uCommand.pui8Buffer, 22, BMP180Callback,
                         psInst);
            }
            else
            {
                //
                // Extract the calibration data from the data that was read.
                //
                psInst->i16AC1 =
                    (int16_t)((psInst->uCommand.pui8Buffer[0] << 8) |
                              psInst->uCommand.pui8Buffer[1]);
                psInst->i16AC2 =
                    (int16_t)((psInst->uCommand.pui8Buffer[2] << 8) |
                              psInst->uCommand.pui8Buffer[3]);
                psInst->i16AC3 =
                    (int16_t)((psInst->uCommand.pui8Buffer[4] << 8) |
                              psInst->uCommand.pui8Buffer[5]);
                psInst->ui16AC4 =
                    (uint16_t)((psInst->uCommand.pui8Buffer[6] << 8) |
                               psInst->uCommand.pui8Buffer[7]);
                psInst->ui16AC5 =
                    (uint16_t)((psInst->uCommand.pui8Buffer[8] << 8) |
                               psInst->uCommand.pui8Buffer[9]);
                psInst->ui16AC6 =
                    (uint16_t)((psInst->uCommand.pui8Buffer[10] << 8) |
                               psInst->uCommand.pui8Buffer[11]);
                psInst->i16B1 =
                    (int16_t)((psInst->uCommand.pui8Buffer[12] << 8) |
                              psInst->uCommand.pui8Buffer[13]);
                psInst->i16B2 =
                    (int16_t)((psInst->uCommand.pui8Buffer[14] << 8) |
                              psInst->uCommand.pui8Buffer[15]);
                psInst->i16MC =
                    (int16_t)((psInst->uCommand.pui8Buffer[18] << 8) |
                              psInst->uCommand.pui8Buffer[19]);
                psInst->i16MD =
                    (int16_t)((psInst->uCommand.pui8Buffer[20] << 8) |
                              psInst->uCommand.pui8Buffer[21]);

                //
                // The state machine is now idle.
                //
                psInst->ui8State = BMP180_STATE_IDLE;
            }

            //
            // Done.
            //
            break;
        }

        //
        // A write has just completed.
        //
        case BMP180_STATE_WRITE:
        {
            //
            // Set the mode to the new mode.  If the register was not modified,
            // the values will be the same so this has no effect.
            //
            psInst->ui8Mode = psInst->ui8NewMode;

            //
            // The state machine is now idle.
            //
            psInst->ui8State = BMP180_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        //
        // A read-modify-write has just completed.
        //
        case BMP180_STATE_RMW:
        {
            //
            // See if the CTRL_MEAS register was just modified.
            //
            if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[0] ==
               BMP180_O_CTRL_MEAS)
            {
                //
                // Extract the measurement mode from the CTRL_MEAS register
                // value.
                //
                psInst->ui8Mode =
                    (psInst->uCommand.sReadModifyWriteState.pui8Buffer[1] &
                     BMP180_CTRL_MEAS_OSS_M);
            }

            //
            // The state machine is now idle.
            //
            psInst->ui8State = BMP180_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        //
        // The temperature has been requested.
        //
        case BMP180_STATE_REQ_TEMP:
        {
            //
            // Read the control register to see if the temperature reading is
            // available.
            //
            I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                     psInst->uCommand.pui8Buffer, 1,
                     psInst->uCommand.pui8Buffer + 1, 1, BMP180Callback,
                     psInst);

            //
            // Move to the wait for temperature state.
            //
            psInst->ui8State = BMP180_STATE_WAIT_TEMP;

            //
            // Done.
            //
            break;
        }

        //
        // Waiting for the temperature reading to be available.
        //
        case BMP180_STATE_WAIT_TEMP:
        {
            //
            // See if the temperature reading is available.
            //
            if(psInst->uCommand.pui8Buffer[1] & BMP180_CTRL_MEAS_SCO)
            {
                //
                // The temperature reading is not ready yet, so read the
                // control register again.
                //
                I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                         psInst->uCommand.pui8Buffer, 1,
                         psInst->uCommand.pui8Buffer + 1, 1, BMP180Callback,
                         psInst);
            }
            else
            {
                //
                // The temperature reading is ready, so read it now.
                //
                psInst->uCommand.pui8Buffer[0] = BMP180_O_OUT_MSB;
                I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                         psInst->uCommand.pui8Buffer, 1, psInst->pui8Data, 2,
                         BMP180Callback, psInst);

                //
                // Move to the temperature reading state.
                //
                psInst->ui8State = BMP180_STATE_READ_TEMP;
            }

            //
            // Done.
            //
            break;
        }

        //
        // The temperature reading has been retrieved.
        //
        case BMP180_STATE_READ_TEMP:
        {
            //
            // Request the pressure reading from the BMP180.
            //
            psInst->uCommand.pui8Buffer[0] = BMP180_O_CTRL_MEAS;
            psInst->uCommand.pui8Buffer[1] = (BMP180_CTRL_MEAS_SCO |
                                              BMP180_CTRL_MEAS_PRESSURE |
                                              psInst->ui8Mode);
            I2CMWrite(psInst->psI2CInst, psInst->ui8Addr,
                      psInst->uCommand.pui8Buffer, 2, BMP180Callback, psInst);

            //
            // Move to the pressure reading request state.
            //
            psInst->ui8State = BMP180_STATE_REQ_PRES;

            //
            // Done.
            //
            break;
        }

        //
        // The pressure has been requested.
        //
        case BMP180_STATE_REQ_PRES:
        {
            //
            // Read the control register to see if the pressure reading is
            // available.
            //
            I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                     psInst->uCommand.pui8Buffer, 1,
                     psInst->uCommand.pui8Buffer + 1, 1, BMP180Callback,
                     psInst);

            //
            // Move to the wait for pressure state.
            //
            psInst->ui8State = BMP180_STATE_WAIT_PRES;

            //
            // Done.
            //
            break;
        }

        //
        // Waiting for the pressure reading to be available.
        //
        case BMP180_STATE_WAIT_PRES:
        {
            //
            // See if the pressure reading is available.
            //
            if(psInst->uCommand.pui8Buffer[1] & BMP180_CTRL_MEAS_SCO)
            {
                //
                // The pressure reading is not ready yet, so read the control
                // register again.
                //
                I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                         psInst->uCommand.pui8Buffer, 1,
                         psInst->uCommand.pui8Buffer + 1, 1, BMP180Callback,
                         psInst);
            }
            else
            {
                //
                // The pressure reading is ready, so read it now.
                //
                psInst->uCommand.pui8Buffer[0] = BMP180_O_OUT_MSB;
                I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                         psInst->uCommand.pui8Buffer, 1, psInst->pui8Data + 2,
                         3, BMP180Callback, psInst);

                //
                // Move to the pressure reading state.
                //
                psInst->ui8State = BMP180_STATE_READ_PRES;
            }

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == BMP180_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Initializes the BMP180 driver.
//!
//! \param psInst is a pointer to the BMP180 instance data.
//! \param psI2CInst is a pointer to the I2C master driver instance data.
//! \param ui8I2CAddr is the I2C address of the BMP180 device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the BMP180 driver, preparing it for operation.
//!
//! \return Returns 1 if the BMP180 driver was successfully initialized and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
BMP180Init(tBMP180 *psInst, tI2CMInstance *psI2CInst, uint_fast8_t ui8I2CAddr,
           tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Initialize the BMP180 instance structure.
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8I2CAddr;
    psInst->ui8State = BMP180_STATE_INIT1;
    psInst->ui8Mode = 0;
    psInst->ui8NewMode = 0;

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Perform a soft reset of the BMP180.
    //
    psInst->pui8Data[0] = BMP180_O_SOFT_RESET;
    psInst->pui8Data[1] = BMP180_SOFT_RESET_VALUE;
    if(I2CMWrite(psI2CInst, ui8I2CAddr, psInst->pui8Data, 2, BMP180Callback,
                 psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = BMP180_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads data from BMP180 registers.
//!
//! \param psInst is a pointer to the BMP180 instance data.
//! \param ui8Reg is the first register to read.
//! \param pui8Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count is the number of data bytes to read.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the BMP180.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
BMP180Read(tBMP180 *psInst, uint_fast8_t ui8Reg, uint8_t *pui8Data,
           uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
           void *pvCallbackData)
{
    //
    // Return a failure if the BMP180 driver is not idle (in other words, there
    // is already an outstanding request to the BMP180).
    //
    if(psInst->ui8State != BMP180_STATE_IDLE)
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
    psInst->ui8State = BMP180_STATE_READ;

    //
    // Read the requested registers from the BMP180.
    //
    psInst->uCommand.pui8Buffer[0] = ui8Reg;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, pui8Data, ui16Count,
                BMP180Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = BMP180_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Writes data to BMP180 registers.
//!
//! \param psInst is a pointer to the BMP180 instance data.
//! \param ui8Reg is the first register to write.
//! \param pui8Data is a pointer to the data to write.
//! \param ui16Count is the number of data bytes to write.
//! \param pfnCallback is the function to be called when the data has been
//! written (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the BMP180.  The first byte of the \e pui8Data buffer contains the value to
//! be written into the \e ui8Reg register, the second value contains the data
//! to be written into the next register, and so on.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
BMP180Write(tBMP180 *psInst, uint_fast8_t ui8Reg, uint8_t *pui8Data,
            uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
            void *pvCallbackData)
{
    //
    // Return a failure if the BMP180 driver is not idle (in other words, there
    // is already an outstanding request to the BMP180).
    //
    if(psInst->ui8State != BMP180_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // See if the CTRL_MEAS register is being written.
    //
    if((ui8Reg <= BMP180_O_CTRL_MEAS) &&
       ((ui8Reg + ui16Count) > BMP180_O_CTRL_MEAS))
    {
        //
        // Extract the measurement mode from the CTRL_MEAS register value.
        //
        psInst->ui8NewMode = (pui8Data[ui8Reg - BMP180_O_CTRL_MEAS] &
                              BMP180_CTRL_MEAS_OSS_M);
    }

    //
    // Move the state machine to the wait for write state.
    //
    psInst->ui8State = BMP180_STATE_WRITE;

    //
    // Write the requested registers to the BMP180.
    //
    if(I2CMWrite8(&(psInst->uCommand.sWriteState), psInst->psI2CInst,
                  psInst->ui8Addr, ui8Reg, pui8Data, ui16Count, BMP180Callback,
                  psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = BMP180_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs a read-modify-write of a BMP180 register.
//!
//! \param psInst is a pointer to the BMP180 instance data.
//! \param ui8Reg is the register to modify.
//! \param ui8Mask is the bit mask that is ANDed with the current register
//! value.
//! \param ui8Value is the bit mask that is ORed with the result of the AND
//! operation.
//! \param pfnCallback is the function to be called when the data has been
//! changed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function changes the value of a register in the BMP180 via a
//! read-modify-write operation, allowing one of the fields to be changed
//! without disturbing the other fields.  The \e ui8Reg register is read, ANDed
//! with \e ui8Mask, ORed with \e ui8Value, and then written back to the
//! BMP180.
//!
//! \return Returns 1 if the read-modify-write was successfully started and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
BMP180ReadModifyWrite(tBMP180 *psInst, uint_fast8_t ui8Reg,
                      uint_fast8_t ui8Mask, uint_fast8_t ui8Value,
                      tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the BMP180 driver is not idle (in other words, there
    // is already an outstanding request to the BMP180).
    //
    if(psInst->ui8State != BMP180_STATE_IDLE)
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
    psInst->ui8State = BMP180_STATE_RMW;

    //
    // Submit the read-modify-write request to the BMP180.
    //
    if(I2CMReadModifyWrite8(&(psInst->uCommand.sReadModifyWriteState),
                            psInst->psI2CInst, psInst->ui8Addr, ui8Reg,
                            ui8Mask, ui8Value, BMP180Callback, psInst) == 0)
    {
        //
        // The I2C read-modify-write failed, so move to the idle state and
        // return a failure.
        //
        psInst->ui8State = BMP180_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads the pressure data from the BMP180.
//!
//! \param psInst is a pointer to the BMP180 instance data.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read of the BMP180 data registers.  When the
//! read has completed (as indicated by calling the callback function), the
//! new temperature and pressure readings can be obtained via:
//!
//! - BMP180DataPressureGetRaw()
//! - BMP180DataPressureGetFloat()
//! - BMP180DataTemperatureGetRaw()
//! - BMP180DataTemperatureGetFloat()
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
BMP180DataRead(tBMP180 *psInst, tSensorCallback *pfnCallback,
               void *pvCallbackData)
{
    //
    // Return a failure if the BMP180 driver is not idle (in other words, there
    // is already an outstanding request to the BMP180).
    //
    if(psInst->ui8State != BMP180_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Move the state machine to the temperature reading request state.
    //
    psInst->ui8State = BMP180_STATE_REQ_TEMP;

    //
    // Request the temperature reading from the BMP180.
    //
    psInst->uCommand.pui8Buffer[0] = BMP180_O_CTRL_MEAS;
    psInst->uCommand.pui8Buffer[1] = (BMP180_CTRL_MEAS_SCO |
                                      BMP180_CTRL_MEAS_TEMPERATURE);
    if(I2CMWrite(psInst->psI2CInst, psInst->ui8Addr,
                 psInst->uCommand.pui8Buffer, 2, BMP180Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = BMP180_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Gets the raw pressure data from the most recent data read.
//!
//! \param psInst is a pointer to the BMP180 instance data.
//! \param pui32Pressure is a pointer to the value into which the raw pressure
//! data is stored.
//!
//! This function returns the raw pressure data from the most recent data read.
//! The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
BMP180DataPressureGetRaw(tBMP180 *psInst, uint_fast32_t *pui32Pressure)
{
    //
    // Return the raw pressure value.
    //
    *pui32Pressure = ((psInst->pui8Data[2] << 16) |
                      (psInst->pui8Data[3] << 8) |
                      (psInst->pui8Data[4] & BMP180_OUT_XLSB_M));
}

//*****************************************************************************
//
//! Gets the pressure data from the most recent data read.
//!
//! \param psInst is a pointer to the BMP180 instance data.
//! \param pfPressure is a pointer to the value into which the pressure data is
//! stored.
//!
//! This function returns the pressure data from the most recent data read,
//! converted into pascals.
//!
//! \return None.
//
//*****************************************************************************
void
BMP180DataPressureGetFloat(tBMP180 *psInst, float *pfPressure)
{
    float fUT, fUP, fX1, fX2, fX3, fB3, fB4, fB5, fB6, fB7, fP;
    int_fast8_t i8Oss;

    //
    // Get the oversampling ratio.
    //
    i8Oss = psInst->ui8Mode >> BMP180_CTRL_MEAS_OSS_S;

    //
    // Retrieve the uncompensated temperature and pressure.
    //
    fUT = (float)(uint16_t)((psInst->pui8Data[0] << 8) |
                           psInst->pui8Data[1]);
    fUP = ((float)(int32_t)((psInst->pui8Data[2] << 16) |
                            (psInst->pui8Data[3] << 8) |
                            (psInst->pui8Data[4] & BMP180_OUT_XLSB_M)) /
           (1 << (8 - i8Oss)));

    //
    // Calculate the true temperature.
    //
    fX1 = ((fUT - (float)psInst->ui16AC6) * (float)psInst->ui16AC5) / 32768.f;
    fX2 = ((float)psInst->i16MC * 2048.f) / (fX1 + (float)psInst->i16MD);
    fB5 = fX1 + fX2;

    //
    // Calculate the true pressure.
    //
    fB6 = fB5 - 4000;
    fX1 = ((float)psInst->i16B2 * ((fB6 * fB6) / 4096)) / 2048;
    fX2 = ((float)psInst->i16AC2 * fB6) / 2048;
    fX3 = fX1 + fX2;
    fB3 = ((((float)psInst->i16AC1 * 4) + fX3) * (1 << i8Oss)) / 4;
    fX1 = ((float)psInst->i16AC3 * fB6) / 8192;
    fX2 = ((float)psInst->i16B1 * ((fB6 * fB6) / 4096)) / 65536;
    fX3 = (fX1 + fX2) / 4;
    fB4 = (float)psInst->ui16AC4 * ((fX3 / 32768) + 1);
    fB7 = (fUP - fB3) * (50000 >> i8Oss);
    fP = (fB7 * 2) / fB4;
    fX1 = (fP / 256) * (fP / 256);
    fX1 = (fX1 * 3038) / 65536;
    fX2 = (fP * -7357) / 65536;
    fP += (fX1 + fX2 + 3791) / 16;
    *pfPressure = fP;
}

//*****************************************************************************
//
//! Gets the raw temperature data from the most recent data read.
//!
//! \param psInst is a pointer to the BMP180 instance data.
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
BMP180DataTemperatureGetRaw(tBMP180 *psInst, uint_fast16_t *pui16Temperature)
{
    //
    // Return the raw temperature value.
    //
    *pui16Temperature = (psInst->pui8Data[0] << 8) | psInst->pui8Data[1];
}

//*****************************************************************************
//
//! Gets the temperature data from the most recent data read.
//!
//! \param psInst is a pointer to the BMP180 instance data.
//! \param pfTemperature is a pointer to the value into which the temperature
//! data is stored.
//!
//! This function returns the temperature data from the most recent data read,
//! converted into Celsius.
//!
//! \return None.
//
//*****************************************************************************
void
BMP180DataTemperatureGetFloat(tBMP180 *psInst, float *pfTemperature)
{
    float fUT, fX1, fX2, fB5;

    //
    // Get the uncompensated temperature.
    //
    fUT = (float)(uint16_t)((psInst->pui8Data[0] << 8) |
                            psInst->pui8Data[1]);

    //
    // Calculate the true temperature.
    //
    fX1 = ((fUT - (float)psInst->ui16AC6) * (float)psInst->ui16AC5) / 32768.f;
    fX2 = ((float)psInst->i16MC * 2048.f) / (fX1 + (float)psInst->i16MD);
    fB5 = fX1 + fX2;
    *pfTemperature = fB5 / 160.f;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
