//*****************************************************************************
//
// cm3218.c - Driver for the CM3218 light sensor
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
#include "sensorlib/hw_cm3218.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/cm3218.h"

//*****************************************************************************
//
//! \addtogroup cm3218_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The states of the CM3218 state machine.
//
//*****************************************************************************
#define CM3218_STATE_IDLE       0
#define CM3218_STATE_INIT       1
#define CM3218_STATE_READ       2
#define CM3218_STATE_WRITE      3

//*****************************************************************************
//
// Sensitivity setting to floating point range value lookup table.
//
//*****************************************************************************
const float g_pfSensitivityLookup[4] =
{
    0.02857,
    0.01328,
    0.00714,
    0.003571
};

//*****************************************************************************
//
// The callback function that is called when I2C transations to/from the CM3218
// have completed.
//
//*****************************************************************************
static void
CM3218Callback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tCM3218 *psInst;

    //
    // Convert the instance data into a pointer to a tCM3218 structure.
    //
    psInst = pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result in a callback to propagate the
    // error).
    //
    if(ui8Status != I2CM_STATUS_SUCCESS)
    {
        psInst->ui8State = CM3218_STATE_IDLE;
    }

    //
    // Determine the current state of the CM3218 state machine.
    //
    switch(psInst->ui8State)
    {
        //
        // All states that trivially transition to IDLE, and all unknown
        // states.
        //
        case CM3218_STATE_INIT:
        case CM3218_STATE_READ:
        default:
        {
            //
            // The state machine is now idle.
            //
            psInst->ui8State = CM3218_STATE_IDLE;
            break;
        }

        //
        // A write has just completed.
        //
        case CM3218_STATE_WRITE:
        {
            //
            // Set the integration time to the new integration time.  If the
            // register was not modified, the values will be the same so this
            // has no effect.
            //
            psInst->ui8IntTime = psInst->ui8NewIntTime;

            //
            // The state machine is now idle.
            //
            psInst->ui8State = CM3218_STATE_IDLE;

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == CM3218_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Initializes the CM3218 driver.
//!
//! \param psInst is a pointer to the CM3218 instance data.
//! \param psI2CInst is a pointer to the I2C driver instance data.
//! \param ui8I2CAddr is the I2C address of the CM3218 device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the CM3218 driver, preparing it for operation.
//!
//! \return Returns 1 if the CM3218 driver was successfully initialized and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
CM3218Init(tCM3218 *psInst, tI2CMInstance *psI2CInst, uint_fast8_t ui8I2CAddr,
           tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Initialize the CM3218 instance structure
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8I2CAddr;
    psInst->ui8State = CM3218_STATE_IDLE;
    psInst->ui8IntTime = CM3218_CMD_CONFIG_IT_10 >> CM3218_CMD_CONFIG_IT_S;
    psInst->ui8NewIntTime = CM3218_CMD_CONFIG_IT_10 >> CM3218_CMD_CONFIG_IT_S;

    //
    // The default settings are ok.  Call the callback function if provided.
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
//! Reads data from CM3218 registers.
//!
//! \param psInst is a pointer to the CM3218 instance data.
//! \param ui8Reg is the first register to read.
//! \param pui16Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count the number of register values bytes to read.
//! \param pfnCallback is the function to be called when data read is complete
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the CM3218.
//!
//! \note The CM3218 does not auto-increment the register pointer, so reads of
//! more than one value returns the same data.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
CM3218Read(tCM3218 *psInst, uint_fast8_t ui8Reg, uint16_t *pui16Data,
           uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
           void *pvCallbackData)
{
    //
    // Return a failure if the CM3218 driver is not idle (in other words, there
    // is already an outstanding request to the CM3218).
    //
    if(psInst->ui8State != CM3218_STATE_IDLE)
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
    psInst->ui8State = CM3218_STATE_READ;

    //
    // Read the requested registers from the CM3218.
    //
    if(I2CMRead16BE(&(psInst->uCommand.sReadState), psInst->psI2CInst,
                    psInst->ui8Addr, ui8Reg, pui16Data, ui16Count,
                    CM3218Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = CM3218_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Writes data to CM3218 registers.
//!
//! \param psInst is a pointer to the CM3218 instance data.
//! \param ui8Reg is the first register to write.
//! \param pui16Data is a pointer to the 16-bit register data to write.
//! \param ui16Count is the number of data bytes to write.
//! \param pfnCallback is the function to be called when the data has been
//! written (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the CM3218.  The first value in the \e pui16Data buffer contains the data
//! to be written into the \e ui8Reg register, the second value contains the
//! data to be written into the next register, and so on.
//!
//! \note The CM3218 does not auto-increment the register pointer, so writes of
//! more than one register are rejected by the CM3218.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
CM3218Write(tCM3218 *psInst, uint_fast8_t ui8Reg, const uint16_t *pui16Data,
            uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
            void *pvCallbackData)
{
    //
    // Return a failure if the CM3218 driver is not idle (in other words, there
    // is already an outstanding request to the CM3218).
    //
    if(psInst->ui8State != CM3218_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // See if the CMD_CONFIG register is being written.
    //
    if((ui8Reg <= CM3218_CMD_CONFIG) &&
       ((ui8Reg + ui16Count) > CM3218_CMD_CONFIG))
    {
        //
        // Extract the integration time from the CMD_CONFIG register value.
        //
        psInst->ui8NewIntTime = ((pui16Data[ui8Reg - CM3218_CMD_CONFIG] &
                                  CM3218_CMD_CONFIG_IT_M) >>
                                 CM3218_CMD_CONFIG_IT_S);
    }

    //
    // Move the state machine to the wait for write state.
    //
    psInst->ui8State = CM3218_STATE_WRITE;

    //
    // Write the requested registers to the CM3218.
    //
    if(I2CMWrite16BE(&(psInst->uCommand.sWriteState), psInst->psI2CInst,
                     psInst->ui8Addr, ui8Reg, pui16Data, ui16Count,
                     CM3218Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = CM3218_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads the light data from the CM3218.
//!
//! \param psInst is a pointer to the CM3218 instance data.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read of the CM3218 data registers.  When the read
//! has completed (as indicated by calling the callback function), the new
//! readings can be obtained via:
//!
//! - CM3218DataLightVisibleGetRaw()
//! - CM3218DataLightVisibleGetFloat()
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
CM3218DataRead(tCM3218 *psInst, tSensorCallback *pfnCallback,
               void *pvCallbackData)
{
    //
    // Return a failure if the CM3218 driver is not idle (in other words, there
    // is already an outstanding request to the CM3218).
    //
    if(psInst->ui8State != CM3218_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Move the state machine to the wait for data read
    //
    psInst->ui8State = CM3218_STATE_READ;

    //
    // Read the ambient light data from the CM3218.
    //
    psInst->uCommand.pui8Buffer[0] = CM3218_CMD_ALS_DATA;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, psInst->pui8Data, 2,
                CM3218Callback, psInst) == 0)
    {
        //
        // The I2C read failed, so move to the idle state and return a failure.
        //
        psInst->ui8State = CM3218_STATE_IDLE;
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
//! \param psInst is a pointer to the CM3218 instance data.
//! \param pui16Visible is a pointer to the value into which the raw visible
//! light data is stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
void
CM3218DataLightVisibleGetRaw(tCM3218 *psInst, uint16_t *pui16Visible)
{
    //
    // Return the raw Light value.
    //
    *pui16Visible = (psInst->pui8Data[1] << 8) | psInst->pui8Data[0];
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the CM3218 instance data.
//! \param pfVisibleLight is a pointer to the value into which the light
//! data is stored as floating point lux.
//!
//! This function returns the light data from the most recent data read,
//! converted into lux.
//!
//! \return None.
//
//*****************************************************************************
void
CM3218DataLightVisibleGetFloat(tCM3218 *psInst, float *pfVisibleLight)
{
    uint16_t ui16Light;
    float fSensitivity;

    //
    // Get the raw light data from the instance structure
    //
    CM3218DataLightVisibleGetRaw(psInst, &ui16Light);

    //
    // Get the floating point values for sensitivity
    //
    fSensitivity = g_pfSensitivityLookup[psInst->ui8IntTime];

    //
    // Calculate light reading in lux.
    //
    *pfVisibleLight = ((float)ui16Light) * fSensitivity;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
