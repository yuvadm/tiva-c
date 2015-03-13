//*****************************************************************************
//
// l3gd20h.c - Driver for the ST L3GD20H gyroscope.
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
#include "sensorlib/hw_l3gd20h.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/l3gd20h.h"

//*****************************************************************************
//
//! \addtogroup l3gd20h_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The states of the L3GD20H state machine.
//
//*****************************************************************************
#define L3GD20H_STATE_IDLE      0           // State machine is idle
#define L3GD20H_STATE_INIT_RES  1           // Waiting for initialization
#define L3GD20H_STATE_INIT_WAIT 2           // Waiting for reset to complete
#define L3GD20H_STATE_READ      3           // Waiting for read
#define L3GD20H_STATE_WRITE     4           // Waiting for write
#define L3GD20H_STATE_RMW       5           // Waiting for read-modify-write

//*****************************************************************************
//
// The factors used to convert the gyroscope readings from the L3GD20H into
// floating point values in radians per second.
//
// Per the data-sheet, the sensitivity is 8.75, 17.50, and 70.00 mdps/digit for
// for 245, 500, and 2000 DPS scales respectively.
//
//       mdeg     1   deg     PI  rad
// 8.75  ----  * ----       * ---       = 1.5271630955e-4f rad/sec per digit
//       sec     1000 mdeg    180 deg
//
// Values are obtained by taking the degree per second conversion factors
// from the data sheet and then converting to radians per sec (1 degree =
// 0.0174532925 radians).
//
//*****************************************************************************
static const float g_pfL3GD20HGyroFactors[] =
{
    1.5271631e-5f,                          // Range = +/- 245 dps
    3.0543262e-4f,                          // Range = +/- 500 dps
    1.2217305e-3f,                          // Range = +/- 2000 dps
    1.2217305e-3f                           // Range = +/- 2000 dps
};

//*****************************************************************************
//
// The callback function that is called when I2C transations to/from the
// L3GD20H have completed.
//
//*****************************************************************************
static void
L3GD20HCallback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tL3GD20H *psInst;

    //
    // Convert the instance data into a pointer to a tL3GD20H structure.
    //
    psInst = pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result in a callback to propagate the
    // error).
    //
    if(ui8Status != I2CM_STATUS_SUCCESS)
    {
        psInst->ui8State = L3GD20H_STATE_IDLE;
    }

    //
    // Determine the current state of the L3GD20H state machine.
    //
    switch(psInst->ui8State)
    {
        //
        // All states that trivially transition to IDLE, and all unknown
        // states.
        //
        case L3GD20H_STATE_READ:
        default:
        {
            //
            // The state machine is now idle.
            //
            psInst->ui8State = L3GD20H_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        //
        // L3GD20H Device reset was issued
        //
        case L3GD20H_STATE_INIT_RES:
        {
            //
            // Issue a read of the status register to confirm reset is done.
            //
            psInst->uCommand.pui8Buffer[0] = L3GD20H_O_LOW_ODR;
            I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                     psInst->uCommand.pui8Buffer, 1, psInst->pui8Data, 1,
                     L3GD20HCallback, psInst);

            psInst->ui8State = L3GD20H_STATE_INIT_WAIT;

            //
            // Done.
            //
            break;
        }

        //
        // Status register was read, check if reset is done before proceeding.
        //
        case L3GD20H_STATE_INIT_WAIT:
        {
            //
            // Check the value read back from status to determine if device
            // is still in reset or if it is ready.
            //
            if(psInst->pui8Data[0] & L3GD20H_LOW_ODR_SWRESET_M)
            {
                //
                // Device still in reset so begin polling this register.
                //
                psInst->uCommand.pui8Buffer[0] = L3GD20H_O_LOW_ODR;
                I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                         psInst->uCommand.pui8Buffer, 1, psInst->pui8Data, 1,
                         L3GD20HCallback, psInst);
            }
            else
            {
                //
                // Device is out of reset, move to the idle state.
                //
                psInst->ui8State = L3GD20H_STATE_IDLE;
            }

            //
            // Done.
            //
            break;
        }

        //
        // A write just completed
        //
        case L3GD20H_STATE_WRITE:
        {
            //
            // Set the gyroscope ranges to the new values.  If the register was
            // not modified, the values will be the same so this has no effect.
            //
            psInst->ui8GyroFsSel = psInst->ui8NewGyroFsSel;

            //
            // The state machine is now idle.
            //
            psInst->ui8State = L3GD20H_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        //
        // A read-modify-write just completed
        //
        case L3GD20H_STATE_RMW:
        {
            //
            // See if the PWR_MGMT_1 register was just modified.
            //
            if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[0] ==
               L3GD20H_O_LOW_ODR)
            {
                //
                // See if a soft reset has been issued.
                //
                if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[1] &
                   L3GD20H_LOW_ODR_SWRESET_M)
                {
                    //
                    // Default range setting is +/- 245 degrees/s
                    //
                    psInst->ui8GyroFsSel = 0;
                    psInst->ui8NewGyroFsSel = 0;
                }
            }

            //
            // See if the GYRO_CONFIG register was just modified.
            //
            if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[0] ==
               L3GD20H_O_CTRL4)
            {
                //
                // Extract the FS_SEL from the GYRO_CONFIG register value.
                //
                psInst->ui8GyroFsSel =
                    ((psInst->uCommand.sReadModifyWriteState.pui8Buffer[1] &
                      L3GD20H_CTRL4_FS_M) >>
                     L3GD20H_CTRL4_FS_S);
            }

            //
            // The state machine is now idle.
            //
            psInst->ui8State = L3GD20H_STATE_IDLE;

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == L3GD20H_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Initializes the L3GD20H driver.
//!
//! \param psInst is a pointer to the L3GD20H instance data.
//! \param psI2CInst is a pointer to the I2C master driver instance data.
//! \param ui8I2CAddr is the I2C address of the L3GD20H device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the L3GD20H driver, preparing it for operation.
//!
//! \return Returns 1 if the L3GD20H driver was successfully initialized and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
L3GD20HInit(tL3GD20H *psInst, tI2CMInstance *psI2CInst,
            uint_fast8_t ui8I2CAddr, tSensorCallback *pfnCallback,
            void *pvCallbackData)
{
    //
    // Initialize the L3GD20H instance structure.
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8I2CAddr;

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Default range setting is +/- 245 degrees/s
    //
    psInst->ui8GyroFsSel =
        (L3GD20H_CTRL4_FS_245DPS & L3GD20H_CTRL4_FS_M) >> L3GD20H_CTRL4_FS_S;
    psInst->ui8NewGyroFsSel =
        (L3GD20H_CTRL4_FS_245DPS & L3GD20H_CTRL4_FS_M) >> L3GD20H_CTRL4_FS_S;

    //
    // Set the state to show we are initiating a reset.
    //
    psInst->ui8State = L3GD20H_STATE_INIT_RES;

    //
    // Load the buffer with command to perform device reset
    //
    psInst->pui8Data[0] = L3GD20H_O_LOW_ODR;
    psInst->pui8Data[1] = L3GD20H_LOW_ODR_SWRESET_RESET;
    if(I2CMWrite(psInst->psI2CInst, psInst->ui8Addr,
                 psInst->uCommand.pui8Buffer, 2, L3GD20HCallback, psInst) == 0)
    {
        psInst->ui8State = L3GD20H_STATE_IDLE;
        return(0);
    }

    //
    // Success
    //
    return(1);
}

//*****************************************************************************
//
//! Reads data from L3GD20H registers.
//!
//! \param psInst is a pointer to the L3GD20H instance data.
//! \param ui8Reg is the first register to read.
//! \param pui8Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count is the number of data bytes to read.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the L3GD20H.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
L3GD20HRead(tL3GD20H *psInst, uint_fast8_t ui8Reg, uint8_t *pui8Data,
            uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
            void *pvCallbackData)
{
    //
    // Return a failure if the L3GD20H driver is not idle (in other words,
    // there is already an outstanding request to the L3GD20H).
    //
    if(psInst->ui8State != L3GD20H_STATE_IDLE)
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
    psInst->ui8State = L3GD20H_STATE_READ;

    //
    // Read the requested registers from the L3GD20H.
    //
    psInst->uCommand.pui8Buffer[0] = ui8Reg;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, pui8Data, ui16Count,
                L3GD20HCallback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = L3GD20H_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Writes data to L3GD20H registers.
//!
//! \param psInst is a pointer to the L3GD20H instance data.
//! \param ui8Reg is the first register to write.
//! \param pui8Data is a pointer to the data to write.
//! \param ui16Count is the number of data bytes to write.
//! \param pfnCallback is the function to be called when the data has been
//! written (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the L3GD20H.  The first byte of the \e pui8Data buffer contains the value
//! to be written into the \e ui8Reg register, the second value contains the
//! data to be written into the next register, and so on.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
L3GD20HWrite(tL3GD20H *psInst, uint_fast8_t ui8Reg, const uint8_t *pui8Data,
             uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
             void *pvCallbackData)
{
    //
    // Return a failure if the L3GD20H driver is not idle (in other words,
    // there is already an outstanding request to the L3GD20H).
    //
    if(psInst->ui8State != L3GD20H_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // See if the PWR_MGMT_1 register is being written.
    //
    if((ui8Reg <= L3GD20H_O_LOW_ODR) &&
       ((ui8Reg + ui16Count) > L3GD20H_O_LOW_ODR))
    {
        //
        // See if a soft reset is being requested.
        //
        if(pui8Data[ui8Reg - L3GD20H_O_LOW_ODR] & L3GD20H_LOW_ODR_SWRESET_M)
        {
            //
            // Default range setting is +/- 245 degrees/s.
            //
            psInst->ui8NewGyroFsSel = 0;
        }
    }

    //
    // See if the GYRO_CONFIG register is being written.
    //
    if((ui8Reg <= L3GD20H_O_CTRL4) &&
       ((ui8Reg + ui16Count) > L3GD20H_O_CTRL4))
    {
        //
        // Extract the FS_SEL from the GYRO_CONFIG register value.
        //
        psInst->ui8NewGyroFsSel = ((pui8Data[ui8Reg - L3GD20H_O_CTRL4] &
                                    L3GD20H_CTRL4_FS_M) >>
                                   L3GD20H_CTRL4_FS_S);
    }

    //
    // Move the state machine to the wait for write state.
    //
    psInst->ui8State = L3GD20H_STATE_WRITE;

    //
    // Write the requested registers to the L3GD20H.
    //
    if(I2CMWrite8(&(psInst->uCommand.sWriteState), psInst->psI2CInst,
                  psInst->ui8Addr, ui8Reg, pui8Data, ui16Count,
                  L3GD20HCallback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = L3GD20H_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs a read-modify-write of a L3GD20H register.
//!
//! \param psInst is a pointer to the L3GD20H instance data.
//! \param ui8Reg is the register to modify.
//! \param ui8Mask is the bit mask that is ANDed with the current register
//! value.
//! \param ui8Value is the bit mask that is ORed with the result of the AND
//! operation.
//! \param pfnCallback is the function to be called when the data has been
//! changed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function changes the value of a register in the L3GD20H via a
//! read-modify-write operation, allowing one of the fields to be changed
//! without disturbing the other fields.  The \e ui8Reg register is read, ANDed
//! with \e ui8Mask, ORed with \e ui8Value, and then written back to the
//! L3GD20H.
//!
//! \return Returns 1 if the read-modify-write was successfully started and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
L3GD20HReadModifyWrite(tL3GD20H *psInst, uint_fast8_t ui8Reg,
                       uint_fast8_t ui8Mask, uint_fast8_t ui8Value,
                       tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the L3GD20H driver is not idle (in other words,
    // there is already an outstanding request to the L3GD20H).
    //
    if(psInst->ui8State != L3GD20H_STATE_IDLE)
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
    psInst->ui8State = L3GD20H_STATE_RMW;

    //
    // Submit the read-modify-write request to the L3GD20H.
    //
    if(I2CMReadModifyWrite8(&(psInst->uCommand.sReadModifyWriteState),
                            psInst->psI2CInst, psInst->ui8Addr, ui8Reg,
                            ui8Mask, ui8Value, L3GD20HCallback, psInst) == 0)
    {
        //
        // The I2C read-modify-write failed, so move to the idle state and
        // return a failure.
        //
        psInst->ui8State = L3GD20H_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads the gyroscope data from the L3GD20H.
//!
//! \param psInst is a pointer to the L3GD20H instance data.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read of the L3GD20H data registers.  When the
//! read has completed (as indicated by calling the callback function), the new
//! readings can be obtained via:
//!
//! - L3GD20HDataGyroGetRaw()
//! - L3GD20HDataGyroGetFloat()
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
L3GD20HDataRead(tL3GD20H *psInst, tSensorCallback *pfnCallback,
                void *pvCallbackData)
{
    //
    // Return a failure if the L3GD20H driver is not idle (in other words,
    // there is already an outstanding request to the L3GD20H).
    //
    if(psInst->ui8State != L3GD20H_STATE_IDLE)
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
    psInst->ui8State = L3GD20H_STATE_READ;

    //
    // Read the data registers from the L3GD20H.
    //
    psInst->pui8Data[0] = L3GD20H_O_STATUS | 0x80;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr, psInst->pui8Data, 1,
                psInst->pui8Data, 7, L3GD20HCallback, psInst) == 0)
    {
        //
        // The I2C read failed, so move to the idle state and return a failure.
        //
        psInst->ui8State = L3GD20H_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Gets the raw gyroscope data from the most recent data read.
//!
//! \param psInst is a pointer to the L3GD20H instance data.
//! \param pui16GyroX is a pointer to the value into which the raw X-axis
//! gyroscope data is stored.
//! \param pui16GyroY is a pointer to the value into which the raw Y-axis
//! gyroscope data is stored.
//! \param pui16GyroZ is a pointer to the value into which the raw Z-axis
//! gyroscope data is stored.
//!
//! This function returns the raw gyroscope data from the most recent data
//! read.  The data is not manipulated in any way by the driver.  If any of the
//! output data pointers are \b NULL, the corresponding data is not provided.
//!
//! \return None.
//
//*****************************************************************************
void
L3GD20HDataGyroGetRaw(tL3GD20H *psInst, uint_fast16_t *pui16GyroX,
                      uint_fast16_t *pui16GyroY, uint_fast16_t *pui16GyroZ)
{
    //
    // Return the raw gyroscope values.
    //
    if(pui16GyroX)
    {
        *pui16GyroX = (psInst->pui8Data[2] << 8) | psInst->pui8Data[1];
    }
    if(pui16GyroY)
    {
        *pui16GyroY = (psInst->pui8Data[4] << 8) | psInst->pui8Data[3];
    }
    if(pui16GyroZ)
    {
        *pui16GyroZ = (psInst->pui8Data[6] << 8) | psInst->pui8Data[5];
    }
}

//*****************************************************************************
//
//! Gets the gyroscope data from the most recent data read.
//!
//! \param psInst is a pointer to the L3GD20H instance data.
//! \param pfGyroX is a pointer to the value into which the X-axis gyroscope
//! data is stored.
//! \param pfGyroY is a pointer to the value into which the Y-axis gyroscope
//! data is stored.
//! \param pfGyroZ is a pointer to the value into which the Z-axis gyroscope
//! data is stored.
//!
//! This function returns the gyroscope data from the most recent data read,
//! converted into radians per second.  If any of the output data pointers are
//! \b NULL, the corresponding data is not provided.
//!
//! \return None.
//
//*****************************************************************************
void
L3GD20HDataGyroGetFloat(tL3GD20H *psInst, float *pfGyroX, float *pfGyroY,
                        float *pfGyroZ)
{
    float fFactor;

    //
    // Get the conversion factor for the current data format.
    //
    fFactor = g_pfL3GD20HGyroFactors[psInst->ui8GyroFsSel];

    //
    // Convert the gyroscope values into rad/sec.
    //
    if(pfGyroX)
    {
        *pfGyroX = ((float)(int16_t)((psInst->pui8Data[2] << 8) |
                                     psInst->pui8Data[1]) * fFactor);
    }
    if(pfGyroY)
    {
        *pfGyroY = ((float)(int16_t)((psInst->pui8Data[4] << 8) |
                                     psInst->pui8Data[3]) * fFactor);
    }
    if(pfGyroZ)
    {
        *pfGyroZ = ((float)(int16_t)((psInst->pui8Data[6] << 8) |
                                     psInst->pui8Data[5]) * fFactor);
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
