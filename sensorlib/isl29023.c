//*****************************************************************************
//
// isl29023.c - Driver for the ISL29023 Light Sensor
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
#include <stdbool.h>
#include "sensorlib/hw_isl29023.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/isl29023.h"

//*****************************************************************************
//
//! \addtogroup isl29023_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Range setting to floating point range value lookup table
//
//*****************************************************************************
const float g_fRangeLookup[4] =
{
    1000.0,
    4000.0,
    16000.0,
    64000.0
};

//*****************************************************************************
//
// Resolution setting to floating point resolution value lookup table
//
//*****************************************************************************
const float g_fResolutionLookup[4] =
{
    65536.0,
    4096.0,
    256.0,
    16.0
};

//*****************************************************************************
//
// Beta value lookup based on datasheet typical values for DATA_IR1, DATA_IR2,
// DATA_IR3, DATA_IR4.  These should be reasonable for 16 bit conversions.
// However, Beta changes with resolution and background IR conditions.
//
//*****************************************************************************
const float g_fBetaLookup[4] =
{
    95.238,
    23.810,
    5.952,
    1.486
};

//*****************************************************************************
//
// The states of the ISL29023 state machine.
//
//*****************************************************************************
#define ISL29023_STATE_IDLE           0
#define ISL29023_STATE_INIT           1
#define ISL29023_STATE_READ           2
#define ISL29023_STATE_WRITE          3
#define ISL29023_STATE_RMW            4
#define ISL29023_STATE_READ_DATA      5

//*****************************************************************************
//
// The callback function that is called when I2C transations to/from the
// ISL29023 have completed.
//
//*****************************************************************************
static void
ISL29023Callback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tISL29023 *psInst;
    uint8_t *pui8Data;

    //
    // Convert the instance data into a pointer to a tBMA180 structure.
    //
    psInst = (tISL29023 *)pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result in a callback to propagate the
    // error).
    //
    if(ui8Status != I2CM_STATUS_SUCCESS)
    {
        psInst->ui8State = ISL29023_STATE_IDLE;
    }

    //
    // Determine the current state of the ISL29023 state machine.
    //
    switch(psInst->ui8State)
    {
        //
        // All states that trivially transition to IDLE, and all unknown
        // states.
        //
        case ISL29023_STATE_INIT:
        case ISL29023_STATE_READ:
        case ISL29023_STATE_READ_DATA:
        default:
        {
            //
            // A register operation is complete.  Return to IDLE state
            // the value read from the register is in the pui8Data buffer and
            // can be accessed directly by the calling application after
            // receiving the callback.
            //
            psInst->ui8State = ISL29023_STATE_IDLE;
            break;
        }

        //
        // A write to the ISL29023 control and config registers is complete.
        //
        case ISL29023_STATE_WRITE:
        {
            //
            // Set the range and resolution to the new values.  If the register
            // was not modified, the values will be the same so this has no
            // effect.
            //
            psInst->ui8Range = psInst->ui8NewRange;
            psInst->ui8Resolution = psInst->ui8NewResolution;

            //
            // The state machine is now idle.
            //
            psInst->ui8State = ISL29023_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        //
        // A read modify write operation has just completed.
        //
        case ISL29023_STATE_RMW:
        {
            //
            // Check if the register that was written contained the range or
            // resolution data that we need to track.
            //
            pui8Data = psInst->uCommand.sReadModifyWriteState.pui8Buffer;
            if((pui8Data[0] == ISL29023_O_CMD_II) &&
               (ui8Status == I2CM_STATUS_SUCCESS))
            {
                //
                // Store the latest range and resolution settings
                //
                psInst->ui8Range = ((pui8Data[1] & ISL29023_CMD_II_RANGE_M) >>
                                    ISL29023_CMD_II_RANGE_S);
                psInst->ui8Resolution = ((pui8Data[1] &
                                          ISL29023_CMD_II_ADC_RES_M) >>
                                         ISL29023_CMD_II_ADC_RES_S);
            }

            //
            // The state machine is now idle.
            //
            psInst->ui8State = ISL29023_STATE_IDLE;

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == ISL29023_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Initializes the ISL29023 driver.
//!
//! \param psInst is a pointer to the ISL29023 instance data.
//! \param psI2CInst is a pointer to the I2C driver instance data.
//! \param ui8I2CAddr is the I2C address of the ISL29023 device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the ISL29023 driver, preparing it for operation.
//! This function also asserts a reset signal to the ISL29023 to clear any
//! previous configuration data.
//!
//! \return Returns 1 if the ISL29023 driver was successfully initialized and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
ISL29023Init(tISL29023 *psInst, tI2CMInstance *psI2CInst,
             uint_fast8_t ui8I2CAddr, tSensorCallback *pfnCallback,
             void *pvCallbackData)
{
    //
    // Initialize the ISL29023 instance structure
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8I2CAddr;
    psInst->ui8State = ISL29023_STATE_INIT;
    psInst->ui8Range = ISL29023_CMD_II_RANGE_1K >> ISL29023_CMD_II_RANGE_S;
    psInst->ui8NewRange = ISL29023_CMD_II_RANGE_1K >> ISL29023_CMD_II_RANGE_S;
    psInst->ui8Resolution = (ISL29023_CMD_II_ADC_RES_16 >>
                             ISL29023_CMD_II_ADC_RES_S);
    psInst->ui8NewResolution = (ISL29023_CMD_II_ADC_RES_16 >>
                                ISL29023_CMD_II_ADC_RES_S);

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Put the device into power down mode.
    //
    psInst->pui8Data[0] = ISL29023_O_CMD_I;
    psInst->pui8Data[1] = ISL29023_CMD_I_OP_MODE_POWER_DOWN;
    if(I2CMWrite(psInst->psI2CInst, psInst->ui8Addr, psInst->pui8Data, 2,
                 ISL29023Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = ISL29023_STATE_IDLE;
        return(0);
    }

    //
    // Success
    //
    return(1);
}

//*****************************************************************************
//
//! Reads data from ISL29023 registers.
//!
//! \param psInst is a pointer to the ISL29023 instance data.
//! \param ui8Reg is the first register to read.
//! \param pui8Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count is the number of data bytes to read.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the ISL29023.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
ISL29023Read(tISL29023 *psInst, uint_fast8_t ui8Reg, uint8_t *pui8Data,
             uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
             void *pvCallbackData)
{
    //
    // Return a failure if the ISL29023 driver is not idle (in other words,
    // there is already an outstanding request to the ISL29023).
    //
    if(psInst->ui8State != ISL29023_STATE_IDLE)
    {
        return(0);
    }
    //
    // Store the Callback information
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // set ISL29023 state
    //
    psInst->ui8State = ISL29023_STATE_READ;

    //
    // Load the command buffer with the appropriate register information
    //
    psInst->uCommand.pui8Buffer[0] = ui8Reg;

    //
    // Start the I2CM read and return indication.
    //
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, pui8Data, ui16Count,
                ISL29023Callback, psInst) == 0)
    {
        psInst->ui8State = ISL29023_STATE_IDLE;
        return(0);
    }

    return(1);
}

//*****************************************************************************
//
//! Write register data to the ISL29023.
//!
//! \param psInst is a pointer to the ISL29023 instance data.
//! \param ui8Reg is the first register to write.
//! \param pui8Data is a pointer to the data to write.
//! \param ui16Count is the number of data bytes to write.
//! \param pfnCallback is the function to be called when the data has been
//! written (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the ISL29023.  The first byte of the \e pui8Data buffer contains the value
//! to be written into the \e ui8Reg register, the second value contains the
//! data to be written into the next register, and so on.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
ISL29023Write(tISL29023 *psInst, uint_fast8_t ui8Reg, uint8_t *pui8Data,
              uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
              void *pvCallbackData)
{
    //
    // Return a failure if the ISL29023 driver is not idle (in other words,
    // there is already an outstanding request to the ISL29023).
    //
    if(psInst->ui8State != ISL29023_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // See if the CMD_II register is being written.
    //
    if((ui8Reg <= ISL29023_O_CMD_II) &&
       ((ui8Reg + ui16Count) > ISL29023_O_CMD_II))
    {
        //
        // Extract the range and resolution from the CMD_II register value.
        //
        psInst->ui8NewRange = ((pui8Data[ui8Reg - ISL29023_O_CMD_II] &
                                ISL29023_CMD_II_RANGE_M) >>
                               ISL29023_CMD_II_RANGE_S);
        psInst->ui8NewResolution = ((pui8Data[ui8Reg - ISL29023_O_CMD_II] &
                                     ISL29023_CMD_II_ADC_RES_M) >>
                                    ISL29023_CMD_II_ADC_RES_S);
    }

    //
    // Move state machine to the write state
    //
    psInst->ui8State = ISL29023_STATE_WRITE;

    //
    // Add the i2c address and the register offset to the data buffer.
    //
    if(I2CMWrite8(&(psInst->uCommand.sWriteState), psInst->psI2CInst,
                  psInst->ui8Addr, ui8Reg, pui8Data, ui16Count,
                  ISL29023Callback, psInst) == 0)
    {
        //
        // I2C write failed, move to idle state and return the failure.
        //
        psInst->ui8State = ISL29023_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs a read-modify-write of an ISL29023 register.
//!
//! \param psInst is a pointer to the ISL29023 instance data.
//! \param ui8Reg is the register to modify.
//! \param ui8Mask is the bit mask that is ANDed with the current register
//! value.
//! \param ui8Value is the bit mask that is ORed with the result of the AND
//! operation.
//! \param pfnCallback is the function to be called when the data has been
//! changed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function changes the value of a register in the ISL29023 via a
//! read-modify-write operation, allowing one of the fields to be changed
//! without disturbing the other fields.  The \e ui8Reg register is read, ANDed
//! with \e ui8Mask, ORed with \e ui8Value, and then written back to the
//! ISL29023.
//!
//! \return Returns 1 if the read-modify-write was successfully started and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
ISL29023ReadModifyWrite(tISL29023 *psInst, uint_fast8_t ui8Reg,
                        uint8_t ui8Mask, uint8_t ui8Value,
                        tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the ISL29023 driver is not in the idle state.
    //
    if(psInst->ui8State != ISL29023_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // set ISL29023 state
    //
    psInst->ui8State = ISL29023_STATE_RMW;

    //
    // Submit the read-modify-write request to the ISL29023.
    //
    if(I2CMReadModifyWrite8(&(psInst->uCommand.sReadModifyWriteState),
                            psInst->psI2CInst, psInst->ui8Addr, ui8Reg,
                            ui8Mask, ui8Value, ISL29023Callback, psInst) == 0)
    {
        //
        // The I2C read-modify-write failed, so move to the idle state and
        // return a failure.
        //
        psInst->ui8State = ISL29023_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads the light data from the ISL29023.
//!
//! \param psInst is a pointer to the ISL29023 instance data.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read of the ISL29023 data registers.  When the
//! read has completed (as indicated by calling the callback function), the new
//! readings can be obtained via:
//!
//! - ISL29023DataLightVisibleGetRaw()
//! - ISL29023DataLightVisibleGetFloat()
//! - ISL29023DataLightIRGetRaw()
//! - ISL29023DataLightIRGetFloat()
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
ISL29023DataRead(tISL29023 *psInst, tSensorCallback *pfnCallback,
                 void *pvCallbackData)
{
    //
    // Return a failure if the ISL29023 driver is not idle (in other words,
    // there is already an outstanding request to the ISL29023).
    //
    if(psInst->ui8State != ISL29023_STATE_IDLE)
    {
        return(0);
    }

    //
    // Store the Callback information
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // set ISL29023 state
    //
    psInst->ui8State = ISL29023_STATE_READ_DATA;

    psInst->uCommand.pui8Buffer[0] = ISL29023_O_DATA_OUT_LSB;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, psInst->pui8Data, 2,
                ISL29023Callback, psInst) == 0)
    {
        //
        // The I2C read failed, so move to the idle state and return a failure.
        //
        psInst->ui8State = ISL29023_STATE_IDLE;
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
//! \param psInst is a pointer to the ISL29023 instance data.
//! \param pui16Visible is a pointer to the value into which the raw light data
//! is stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
ISL29023DataLightVisibleGetRaw(tISL29023 *psInst, uint16_t *pui16Visible)
{
    //
    // Return the raw light value.
    //
    *pui16Visible = (psInst->pui8Data[1] << 8) | psInst->pui8Data[0];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the ISL29023 instance data.
//! \param pfVisibleLight is a pointer to the value into which the light data
//! is stored as floating point lux.
//!
//! This function returns the light data from the most recent data read,
//! converted into lux.
//!
//! \return None.
//
//*****************************************************************************
void
ISL29023DataLightVisibleGetFloat(tISL29023 *psInst, float *pfVisibleLight)
{
    uint16_t ui16Light;
    float fRange, fResolution;

    //
    // Get the raw light data from the instance structure
    //
    ISL29023DataLightVisibleGetRaw(psInst, &ui16Light);

    //
    // Get the floating point values for range and resolution from the lookup.
    //
    fRange = g_fRangeLookup[psInst->ui8Range];
    fResolution = g_fResolutionLookup[psInst->ui8Resolution];

    //
    // Calculate light reading in lux.
    //
    *pfVisibleLight = ((float)ui16Light) * (fRange / fResolution);
}

//*****************************************************************************
//
//! Gets the raw measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the ISL29023 instance data.
//! \param pui16IR is a pointer to the value into which the raw IR data is
//! stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
ISL29023DataLightIRGetRaw(tISL29023 *psInst, uint16_t *pui16IR)
{
    //
    // Return the raw light value.
    //
    *pui16IR = (psInst->pui8Data[1] << 8) | psInst->pui8Data[0];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the ISL29023 instance data.
//! \param pfIR is a pointer to the value into which the IR data is stored as
//! floating point lux.
//!
//! This function returns the IR data from the most recent data read, converted
//! into lux.
//!
//! \return None.
//
//*****************************************************************************
void
ISL29023DataLightIRGetFloat(tISL29023 *psInst, float *pfIR)
{
    uint16_t i16IR;

    ISL29023DataLightIRGetRaw(psInst, &i16IR);

    *pfIR = ((float) i16IR) / g_fBetaLookup[psInst->ui8Range];
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
