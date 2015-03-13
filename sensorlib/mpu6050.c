//*****************************************************************************
//
// mpu6050.c - Driver for the MPU6050 accelerometer and gyroscope.
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
#include "sensorlib/hw_mpu6050.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/mpu6050.h"

//*****************************************************************************
//
//! \addtogroup mpu6050_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The states of the MPU6050 state machine.
//
//*****************************************************************************
#define MPU6050_STATE_IDLE      0           // State machine is idle
#define MPU6050_STATE_INIT_RES  1           // Waiting for initialization
#define MPU6050_STATE_INIT_WAIT 2           // Waiting for reset to complete
#define MPU6050_STATE_READ      3           // Waiting for read
#define MPU6050_STATE_WRITE     4           // Waiting for write
#define MPU6050_STATE_RMW       5           // Waiting for read-modify-write

//*****************************************************************************
//
// The factors used to convert the acceleration readings from the MPU6050 into
// floating point values in meters per second squared.
//
// Values are obtained by taking the g conversion factors from the data sheet
// and multiplying by 9.81 (1 g = 9.81 m/s^2).
//
//*****************************************************************************
static const float g_fMPU6050AccelFactors[] =
{
    0.00059875,                              // Range = +/- 2 g (16384 lsb/g)
    0.00119751,                              // Range = +/- 4 g (8192 lsb/g)
    0.00239502,                              // Range = +/- 8 g (4096 lsb/g)
    0.00479004                               // Range = +/- 16 g (2048 lsb/g)
};

//*****************************************************************************
//
// The factors used to convert the acceleration readings from the MPU6050 into
// floating point values in radians per second.
//
// Values are obtained by taking the degree per second conversion factors
// from the data sheet and then converting to radians per sec (1 degree =
// 0.0174532925 radians).
//
//*****************************************************************************
static const float g_fMPU6050GyroFactors[] =
{
    1.3323124e-4f,   // Range = +/- 250 dps  (131.0 LSBs/DPS)
    2.6646248e-4f,   // Range = +/- 500 dps  (65.5 LSBs/DPS)
    5.3211258e-4f,   // Range = +/- 1000 dps (32.8 LSBs/DPS)
    0.0010642252f    // Range = +/- 2000 dps (16.4 LSBs/DPS)
};
//*****************************************************************************
//
// The callback function that is called when I2C transations to/from the
// MPU6050 have completed.
//
//*****************************************************************************
static void
MPU6050Callback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tMPU6050 *psInst;

    //
    // Convert the instance data into a pointer to a tMPU6050 structure.
    //
    psInst = pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result in a callback to propagate the
    // error).  Except in the case that we are in the reset wait state and the
    // error is an address NACK.  This error is handled by the reset wait
    // state.
    //
    if((ui8Status != I2CM_STATUS_SUCCESS) &&
        !((ui8Status == I2CM_STATUS_ADDR_NACK) &&
            (psInst->ui8State == MPU6050_STATE_INIT_WAIT)))
    {
        psInst->ui8State = MPU6050_STATE_IDLE;
    }

    //
    // Determine the current state of the MPU6050 state machine.
    //
    switch(psInst->ui8State)
    {
        //
        // All states that trivially transition to IDLE, and all unknown
        // states.
        //
        case MPU6050_STATE_READ:
        default:
        {
            //
            // The state machine is now idle.
            //
            psInst->ui8State = MPU6050_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        //
        // MPU6050 Device reset was issued
        //
        case MPU6050_STATE_INIT_RES:
        {
            //
            // Issue a read of the status register to confirm reset is done.
            //
            psInst->uCommand.pui8Buffer[0] = MPU6050_O_PWR_MGMT_1;
            I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                     psInst->uCommand.pui8Buffer, 1, psInst->pui8Data, 1,
                     MPU6050Callback, psInst);

            psInst->ui8State = MPU6050_STATE_INIT_WAIT;
            break;
        }

        //
        // Status register was read, check if reset is done before proceeding.
        //
        case MPU6050_STATE_INIT_WAIT:
        {
            //
            // Check the value read back from status to determine if device
            // is still in reset or if it is ready.  Reset state for this
            // register is 0x40, which has sleep bit set.  Device may also
            // respond with an address NACK during very early stages of the its
            // internal reset. Keep polling until we verify device is ready.
            //
            //
            if((psInst->pui8Data[0] != MPU6050_PWR_MGMT_1_SLEEP) ||
                (ui8Status == I2CM_STATUS_ADDR_NACK))
            {
                //
                // Device still in reset so begin polling this register.
                //
                psInst->uCommand.pui8Buffer[0] = MPU6050_O_PWR_MGMT_1;
                I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                         psInst->uCommand.pui8Buffer, 1, psInst->pui8Data, 1,
                         MPU6050Callback, psInst);

                //
                // Intentionally stay in this state to create polling effect.
                //
            }
            else
            {
                //
                // Device is out of reset, move to the idle state.
                //
                psInst->ui8State = MPU6050_STATE_IDLE;
            }
            break;
        }

        //
        // A write just completed
        //
        case MPU6050_STATE_WRITE:
        {
            //
            // Set the accelerometer and gyroscope ranges to the new values.
            // If the register was not modified, the values will be the same so
            // this has no effect.
            //
            psInst->ui8AccelAfsSel = psInst->ui8NewAccelAfsSel;
            psInst->ui8GyroFsSel = psInst->ui8NewGyroFsSel;

            //
            // The state machine is now idle.
            //
            psInst->ui8State = MPU6050_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        //
        // A read-modify-write just completed
        //
        case MPU6050_STATE_RMW:
        {
            //
            // See if the PWR_MGMT_1 register was just modified.
            //
            if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[0] ==
               MPU6050_O_PWR_MGMT_1)
            {
                //
                // See if a soft reset has been issued.
                //
                if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[1] &
                   MPU6050_PWR_MGMT_1_DEVICE_RESET)
                {
                    //
                    // Default range setting is +/- 2 g
                    //
                    psInst->ui8AccelAfsSel = 0;
                    psInst->ui8NewAccelAfsSel = 0;

                    //
                    // Default range setting is +/- 250 degrees/s
                    //
                    psInst->ui8GyroFsSel = 0;
                    psInst->ui8NewGyroFsSel = 0;
                }
            }

            //
            // See if the GYRO_CONFIG register was just modified.
            //
            if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[0] ==
               MPU6050_O_GYRO_CONFIG)
            {
                //
                // Extract the FS_SEL from the GYRO_CONFIG register value.
                //
                psInst->ui8GyroFsSel =
                    ((psInst->uCommand.sReadModifyWriteState.pui8Buffer[1] &
                      MPU6050_GYRO_CONFIG_FS_SEL_M) >>
                     MPU6050_GYRO_CONFIG_FS_SEL_S);
            }

            //
            // See if the ACCEL_CONFIG register was just modified.
            //
            if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[0] ==
               MPU6050_O_ACCEL_CONFIG)
            {
                //
                // Extract the FS_SEL from the ACCEL_CONFIG register value.
                //
                psInst->ui8AccelAfsSel =
                    ((psInst->uCommand.sReadModifyWriteState.pui8Buffer[1] &
                      MPU6050_ACCEL_CONFIG_AFS_SEL_M) >>
                     MPU6050_ACCEL_CONFIG_AFS_SEL_S);
            }

            //
            // The state machine is now idle.
            //
            psInst->ui8State = MPU6050_STATE_IDLE;

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == MPU6050_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Initializes the MPU6050 driver.
//!
//! \param psInst is a pointer to the MPU6050 instance data.
//! \param psI2CInst is a pointer to the I2C master driver instance data.
//! \param ui8I2CAddr is the I2C address of the MPU6050 device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the MPU6050 driver, preparing it for operation.
//!
//! \return Returns 1 if the MPU6050 driver was successfully initialized and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
MPU6050Init(tMPU6050 *psInst, tI2CMInstance *psI2CInst,
            uint_fast8_t ui8I2CAddr, tSensorCallback *pfnCallback,
            void *pvCallbackData)
{
    //
    // Initialize the MPU6050 instance structure.
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8I2CAddr;

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Default range setting is +/- 2 g
    //
    psInst->ui8AccelAfsSel = (MPU6050_ACCEL_CONFIG_AFS_SEL_2G >>
                              MPU6050_ACCEL_CONFIG_AFS_SEL_S);
    psInst->ui8NewAccelAfsSel = (MPU6050_ACCEL_CONFIG_AFS_SEL_2G >>
                                 MPU6050_ACCEL_CONFIG_AFS_SEL_S);

    //
    // Default range setting is +/- 250 degrees/s
    //
    psInst->ui8GyroFsSel = (MPU6050_GYRO_CONFIG_FS_SEL_250 >>
                            MPU6050_GYRO_CONFIG_FS_SEL_S);
    psInst->ui8NewGyroFsSel = (MPU6050_GYRO_CONFIG_FS_SEL_250 >>
                               MPU6050_GYRO_CONFIG_FS_SEL_S);

    //
    // Set the state to show we are initiating a reset.
    //
    psInst->ui8State = MPU6050_STATE_INIT_RES;

    //
    // Load the buffer with command to perform device reset
    //
    psInst->uCommand.pui8Buffer[0] = MPU6050_O_PWR_MGMT_1;
    psInst->uCommand.pui8Buffer[1] = MPU6050_PWR_MGMT_1_DEVICE_RESET;
    if(I2CMWrite(psInst->psI2CInst, psInst->ui8Addr,
                 psInst->uCommand.pui8Buffer, 2, MPU6050Callback, psInst) == 0)
    {
        psInst->ui8State = MPU6050_STATE_IDLE;
        return(0);
    }

    //
    // Success
    //
    return(1);
}

//*****************************************************************************
//
//! Reads data from MPU6050 registers.
//!
//! \param psInst is a pointer to the MPU6050 instance data.
//! \param ui8Reg is the first register to read.
//! \param pui8Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count is the number of data bytes to read.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the MPU6050.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
MPU6050Read(tMPU6050 *psInst, uint_fast8_t ui8Reg, uint8_t *pui8Data,
            uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
            void *pvCallbackData)
{
    //
    // Return a failure if the MPU6050 driver is not idle (in other words,
    // there is already an outstanding request to the MPU6050).
    //
    if(psInst->ui8State != MPU6050_STATE_IDLE)
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
    psInst->ui8State = MPU6050_STATE_READ;

    //
    // Read the requested registers from the MPU6050.
    //
    psInst->uCommand.pui8Buffer[0] = ui8Reg;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, pui8Data, ui16Count,
                MPU6050Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = MPU6050_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Writes data to MPU6050 registers.
//!
//! \param psInst is a pointer to the MPU6050 instance data.
//! \param ui8Reg is the first register to write.
//! \param pui8Data is a pointer to the data to write.
//! \param ui16Count is the number of data bytes to write.
//! \param pfnCallback is the function to be called when the data has been
//! written (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the MPU6050.  The first byte of the \e pui8Data buffer contains the value
//! to be written into the \e ui8Reg register, the second value contains the
//! data to be written into the next register, and so on.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
MPU6050Write(tMPU6050 *psInst, uint_fast8_t ui8Reg, const uint8_t *pui8Data,
             uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
             void *pvCallbackData)
{
    //
    // Return a failure if the MPU6050 driver is not idle (in other words,
    // there is already an outstanding request to the MPU6050).
    //
    if(psInst->ui8State != MPU6050_STATE_IDLE)
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
    if((ui8Reg <= MPU6050_O_PWR_MGMT_1) &&
       ((ui8Reg + ui16Count) > MPU6050_O_PWR_MGMT_1))
    {
        //
        // See if a soft reset is being requested.
        //
        if(pui8Data[ui8Reg - MPU6050_O_PWR_MGMT_1] &
           MPU6050_PWR_MGMT_1_DEVICE_RESET)
        {
            //
            // Default range setting is +/- 2 g.
            //
            psInst->ui8NewAccelAfsSel = 0;

            //
            // Default range setting is +/- 250 degrees/s.
            //
            psInst->ui8NewGyroFsSel = 0;
        }
    }

    //
    // See if the GYRO_CONFIG register is being written.
    //
    if((ui8Reg <= MPU6050_O_GYRO_CONFIG) &&
       ((ui8Reg + ui16Count) > MPU6050_O_GYRO_CONFIG))
    {
        //
        // Extract the FS_SEL from the GYRO_CONFIG register value.
        //
        psInst->ui8NewGyroFsSel = ((pui8Data[ui8Reg - MPU6050_O_GYRO_CONFIG] &
                                    MPU6050_GYRO_CONFIG_FS_SEL_M) >>
                                   MPU6050_GYRO_CONFIG_FS_SEL_S);
    }

    //
    // See if the ACCEL_CONFIG register is being written.
    //
    if((ui8Reg <= MPU6050_O_ACCEL_CONFIG) &&
       ((ui8Reg + ui16Count) > MPU6050_O_ACCEL_CONFIG))
    {
        //
        // Extract the AFS_SEL from the ACCEL_CONFIG register value.
        //
        psInst->ui8NewAccelAfsSel =
            ((pui8Data[ui8Reg - MPU6050_O_ACCEL_CONFIG] &
              MPU6050_ACCEL_CONFIG_AFS_SEL_M) >>
             MPU6050_ACCEL_CONFIG_AFS_SEL_S);
    }

    //
    // Move the state machine to the wait for write state.
    //
    psInst->ui8State = MPU6050_STATE_WRITE;

    //
    // Write the requested registers to the MPU6050.
    //
    if(I2CMWrite8(&(psInst->uCommand.sWriteState), psInst->psI2CInst,
                  psInst->ui8Addr, ui8Reg, pui8Data, ui16Count,
                  MPU6050Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = MPU6050_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs a read-modify-write of a MPU6050 register.
//!
//! \param psInst is a pointer to the MPU6050 instance data.
//! \param ui8Reg is the register to modify.
//! \param ui8Mask is the bit mask that is ANDed with the current register
//! value.
//! \param ui8Value is the bit mask that is ORed with the result of the AND
//! operation.
//! \param pfnCallback is the function to be called when the data has been
//! changed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function changes the value of a register in the MPU6050 via a
//! read-modify-write operation, allowing one of the fields to be changed
//! without disturbing the other fields.  The \e ui8Reg register is read, ANDed
//! with \e ui8Mask, ORed with \e ui8Value, and then written back to the
//! MPU6050.
//!
//! \return Returns 1 if the read-modify-write was successfully started and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
MPU6050ReadModifyWrite(tMPU6050 *psInst, uint_fast8_t ui8Reg,
                       uint_fast8_t ui8Mask, uint_fast8_t ui8Value,
                       tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the MPU6050 driver is not idle (in other words,
    // there is already an outstanding request to the MPU6050).
    //
    if(psInst->ui8State != MPU6050_STATE_IDLE)
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
    psInst->ui8State = MPU6050_STATE_RMW;

    //
    // Submit the read-modify-write request to the MPU6050.
    //
    if(I2CMReadModifyWrite8(&(psInst->uCommand.sReadModifyWriteState),
                            psInst->psI2CInst, psInst->ui8Addr, ui8Reg,
                            ui8Mask, ui8Value, MPU6050Callback, psInst) == 0)
    {
        //
        // The I2C read-modify-write failed, so move to the idle state and
        // return a failure.
        //
        psInst->ui8State = MPU6050_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads the accelerometer and gyroscope data from the MPU6050.
//!
//! \param psInst is a pointer to the MPU6050 instance data.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read of the MPU6050 data registers.  When the
//! read has completed (as indicated by calling the callback function), the new
//! readings can be obtained via:
//!
//! - MPU6050DataAccelGetRaw()
//! - MPU6050DataAccelGetFloat()
//! - MPU6050DataGyroGetRaw()
//! - MPU6050DataGyroGetFloat()
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
MPU6050DataRead(tMPU6050 *psInst, tSensorCallback *pfnCallback,
                void *pvCallbackData)
{
    //
    // Return a failure if the MPU6050 driver is not idle (in other words,
    // there is already an outstanding request to the MPU6050).
    //
    if(psInst->ui8State != MPU6050_STATE_IDLE)
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
    psInst->ui8State = MPU6050_STATE_READ;

    //
    // Read the data registers from the MPU6050.
    //
    psInst->pui8Data[0] = MPU6050_O_ACCEL_XOUT_H;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr, psInst->pui8Data, 1,
                psInst->pui8Data, 14, MPU6050Callback, psInst) == 0)
    {
        //
        // The I2C read failed, so move to the idle state and return a failure.
        //
        psInst->ui8State = MPU6050_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Gets the raw accelerometer data from the most recent data read.
//!
//! \param psInst is a pointer to the MPU6050 instance data.
//! \param pui16AccelX is a pointer to the value into which the raw X-axis
//! accelerometer data is stored.
//! \param pui16AccelY is a pointer to the value into which the raw Y-axis
//! accelerometer data is stored.
//! \param pui16AccelZ is a pointer to the value into which the raw Z-axis
//! accelerometer data is stored.
//!
//! This function returns the raw accelerometer data from the most recent data
//! read.  The data is not manipulated in any way by the driver.  If any of the
//! output data pointers are \b NULL, the corresponding data is not provided.
//!
//! \return None.
//
//*****************************************************************************
void
MPU6050DataAccelGetRaw(tMPU6050 *psInst, uint_fast16_t *pui16AccelX,
                       uint_fast16_t *pui16AccelY, uint_fast16_t *pui16AccelZ)
{
    //
    // Return the raw accelerometer values.
    //
    if(pui16AccelX)
    {
        *pui16AccelX = (psInst->pui8Data[0] << 8) | psInst->pui8Data[1];
    }
    if(pui16AccelY)
    {
        *pui16AccelY = (psInst->pui8Data[2] << 8) | psInst->pui8Data[3];
    }
    if(pui16AccelZ)
    {
        *pui16AccelZ = (psInst->pui8Data[4] << 8) | psInst->pui8Data[5];
    }
}

//*****************************************************************************
//
//! Gets the accelerometer data from the most recent data read.
//!
//! \param psInst is a pointer to the MPU6050 instance data.
//! \param pfAccelX is a pointer to the value into which the X-axis
//! accelerometer data is stored.
//! \param pfAccelY is a pointer to the value into which the Y-axis
//! accelerometer data is stored.
//! \param pfAccelZ is a pointer to the value into which the Z-axis
//! accelerometer data is stored.
//!
//! This function returns the accelerometer data from the most recent data
//! read, converted into meters per second squared (m/s^2).  If any of the
//! output data pointers are \b NULL, the corresponding data is not provided.
//!
//! \return None.
//
//*****************************************************************************
void
MPU6050DataAccelGetFloat(tMPU6050 *psInst, float *pfAccelX, float *pfAccelY,
                         float *pfAccelZ)
{
    float fFactor;

    //
    // Get the acceleration conversion factor for the current data format.
    //
    fFactor = g_fMPU6050AccelFactors[psInst->ui8AccelAfsSel];

    //
    // Convert the Accelerometer values into floating-point gravity values.
    //
    if(pfAccelX)
    {
        *pfAccelX = (float)((int16_t)((psInst->pui8Data[0] << 8) |
                                      psInst->pui8Data[1]) * fFactor);
    }
    if(pfAccelY)
    {
        *pfAccelY = (float)((int16_t)((psInst->pui8Data[2] << 8) |
                                      psInst->pui8Data[3]) * fFactor);
    }
    if(pfAccelZ)
    {
        *pfAccelZ = (float)((int16_t)((psInst->pui8Data[4] << 8) |
                                      psInst->pui8Data[5]) * fFactor);
    }
}
//*****************************************************************************
//
//! Gets the raw gyroscope data from the most recent data read.
//!
//! \param psInst is a pointer to the MPU6050 instance data.
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
MPU6050DataGyroGetRaw(tMPU6050 *psInst, uint_fast16_t *pui16GyroX,
                      uint_fast16_t *pui16GyroY, uint_fast16_t *pui16GyroZ)
{
    //
    // Return the raw gyroscope values.
    //
    if(pui16GyroX)
    {
        *pui16GyroX = (psInst->pui8Data[8] << 8) | psInst->pui8Data[9];
    }
    if(pui16GyroY)
    {
        *pui16GyroY = (psInst->pui8Data[10] << 8) | psInst->pui8Data[11];
    }
    if(pui16GyroZ)
    {
        *pui16GyroZ = (psInst->pui8Data[12] << 8) | psInst->pui8Data[13];
    }
}

//*****************************************************************************
//
//! Gets the gyroscope data from the most recent data read.
//!
//! \param psInst is a pointer to the MPU6050 instance data.
//! \param pfGyroX is a pointer to the value into which the X-axis
//! gyroscope data is stored.
//! \param pfGyroY is a pointer to the value into which the Y-axis
//! gyroscope data is stored.
//! \param pfGyroZ is a pointer to the value into which the Z-axis
//! gyroscope data is stored.
//!
//! This function returns the gyroscope data from the most recent data read,
//! converted into radians per second.  If any of the output data pointers are
//! \b NULL, the corresponding data is not provided.
//!
//! \return None.
//
//*****************************************************************************
void
MPU6050DataGyroGetFloat(tMPU6050 *psInst, float *pfGyroX, float *pfGyroY,
                        float *pfGyroZ)
{
    float fFactor;

    //
    // Get the conversion factor for the current data format.
    //
    fFactor = g_fMPU6050GyroFactors[psInst->ui8GyroFsSel];

    //
    // Convert the gyroscope values into rad/sec
    //
    if(pfGyroX)
    {
        *pfGyroX = ((float)(int16_t)((psInst->pui8Data[8] << 8) |
                                     psInst->pui8Data[9]) * fFactor);
    }
    if(pfGyroY)
    {
        *pfGyroY = ((float)(int16_t)((psInst->pui8Data[10] << 8) |
                                     psInst->pui8Data[11]) * fFactor);
    }
    if(pfGyroZ)
    {
        *pfGyroZ = ((float)(int16_t)((psInst->pui8Data[12] << 8) |
                                     psInst->pui8Data[13]) * fFactor);
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
