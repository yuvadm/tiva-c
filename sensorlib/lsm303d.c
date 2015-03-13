//*****************************************************************************
//
// lsm303d.c - Driver for the ST LSM303D accelerometer/magnetometer.
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
#include "sensorlib/hw_lsm303d.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/lsm303d.h"

//*****************************************************************************
//
//! \addtogroup lsm303dlhc_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The states of the LSM303D state machine.
//
//*****************************************************************************
#define LSM303D_STATE_IDLE   0           // State machine is idle
#define LSM303D_STATE_INIT   1           // Waiting for init
#define LSM303D_STATE_READ_MAG  \
                             2           // Waiting for mag read
#define LSM303D_STATE_READ_ACCEL \
                             3           // Waiting for accel read
#define LSM303D_STATE_WRITE  4           // Waiting for write
#define LSM303D_STATE_RMW    5           // Waiting for read-modify-write

//*****************************************************************************
//
// The factors used to convert the acceleration readings from the LSM303D
// into floating point values in meters per second squared.
//
// Values are obtained by taking the g conversion factors from the data sheet
// and multiplying by 9.81 (1 g = 9.81 m/s^2).
//
//*****************************************************************************
static const float g_pfLSM303DAccelFactors[] =
{
    0.00059875,                             // Range = +/- 2 g (16384 lsb/g)
    0.00119751,                             // Range = +/- 4 g (8192 lsb/g)
    0.00239502,                             // Range = +/- 8 g (4096 lsb/g)
    0.00479004                              // Range = +/- 16 g (2048 lsb/g)
};
static const float g_pfLSM303DMagFactors[] =
{
    8.0e-6f,                                // Range = +/- 2 (0.080 mgauss/lsb)
    1.6e-5f,                                // Range = +/- 4 (0.160 mgauss/lsb)
    3.2e-5f,                                // Range = +/- 8 (0.320 mgauss/lsb)
    4.79e-5f                                // Range = +/- 12 (0.479 mgauss/lsb)
};
//
// Uninitialized values will default to zero which is what we want.  0x80 is
// ORed into the register address so the writes auto-increment
//
static const uint8_t g_pui8ZeroInit[] =
{
    0x80 | LSM303D_O_MAG_INT_CTRL,
    0xE8,                       // MAG_INT_CTRL
    0x0,                        // int_src (RO)
    0x0,                        // THS_LSB
    0x0,                        // THS_MSB
    0x0,                        // OFFSET_X_LSB
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,                        // REF_X
    0x0,
    0x0,
    0x0,                        // CTRL0
    0x7,
    0x0,
    0x0,
    0x0,
    0x18,                       // CTRL5
    0x20,
    0x1,
    0x0,                        // status (RO)
    0x0,                        // out_x_lsb (RO)
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,                        // FIFO_CTRL
    0x0,                        // fifo_src (RO)
    0x0,                        // IG_CFG1
    0x0,                        // ig_src1 (RO)
    0x0,
    0x0,
    0x0,
    0x0,                        // ig_src2 (RO)
    0x0,
    0x0,
    0x0,
    0x0,                        // clk_src (RO)
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0
};

//*****************************************************************************
//
// The callback function that is called when I2C transations to/from the
// LSM303D have completed.
//
//*****************************************************************************
static void
LSM303DCallback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tLSM303D *psInst;

    //
    // Convert the instance data into a pointer to a tLSM303D structure.
    //
    psInst = pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result in a callback to propagate the
    // error).
    //
    if(ui8Status != I2CM_STATUS_SUCCESS)
    {
        psInst->ui8State = LSM303D_STATE_IDLE;
    }

    //
    // Determine the current state of the LSM303D state machine.
    //
    switch(psInst->ui8State)
    {
        //
        // All states that trivially transition to IDLE, and all unknown
        // states.
        //
        default:
        {
            //
            // The state machine is now idle.
            //
            psInst->ui8State = LSM303D_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        case LSM303D_STATE_READ_MAG:
        {
            //
            // Move the state machine to the wait for accel data read state.
            //
            psInst->ui8State = LSM303D_STATE_READ_ACCEL;

            //
            // Read the accel data registers from the LSM303D.
            //
            psInst->pui8DataAccel[0] = LSM303D_O_STATUS | 0x80;
            I2CMRead(psInst->psI2CInst, psInst->ui8Addr, psInst->pui8DataAccel,
                     1, psInst->pui8DataAccel, 7, LSM303DCallback, psInst);

            //
            // Done.
            //
            break;
        }
        case LSM303D_STATE_INIT:
        {
            psInst->ui8State = LSM303D_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        //
        // A write just completed
        //
        case LSM303D_STATE_WRITE:
        {
            //
            // Set the accelerometer ranges to the new values.  If the register
            // was not modified, the values will be the same so this has no
            // effect.
            //
            psInst->ui8AccelFSSel = psInst->ui8NewAccelFSSel;
            psInst->ui8MagFSSel = psInst->ui8NewMagFSSel;

            //
            // The state machine is now idle.
            //
            psInst->ui8State = LSM303D_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        //
        // A read-modify-write just completed
        //
        case LSM303D_STATE_RMW:
        {
            //
            // See if the accel scale register was just modified.
            //
            if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[0] ==
                LSM303D_O_CTRL2)
            {
                //
                // Extract the FS_SEL from the ACCEL_CONFIG register value.
                //
                psInst->ui8AccelFSSel =
                    ((psInst->uCommand.sReadModifyWriteState.pui8Buffer[1] &
                        LSM303D_CTRL2_AFS_M) >> LSM303D_CTRL2_AFS_S);
            }

            //
            // See if the mag scale register was just modified.
            //
            if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[0] ==
                LSM303D_O_CTRL6)
            {
                //
                // Extract the FS_SEL from the mag scale register value.
                //
                psInst->ui8MagFSSel =
                    ((psInst->uCommand.sReadModifyWriteState.pui8Buffer[1] &
                        LSM303D_CTRL6_MFS_M) >> LSM303D_CTRL6_MFS_S);

            }

            //
            // The state machine is now idle.
            //
            psInst->ui8State = LSM303D_STATE_IDLE;

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == LSM303D_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Initializes the LSM303D driver.
//!
//! \param psInst is a pointer to the LSM303D instance data.
//! \param psI2CInst is a pointer to the I2C master driver instance data.
//! \param ui8I2CAddr is the I2C address of the LSM303D device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the LSM303D driver, preparing it for
//! operation.
//!
//! \return Returns 1 if the LSM303D driver was successfully initialized and
//! 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
LSM303DInit(tLSM303D *psInst, tI2CMInstance *psI2CInst,
                    uint_fast8_t ui8I2CAddr, tSensorCallback *pfnCallback,
                    void *pvCallbackData)
{
    //
    // Initialize the LSM303D instance structure.
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
    psInst->ui8AccelFSSel = (LSM303D_CTRL2_AFS_2G >> LSM303D_CTRL2_AFS_S);
    psInst->ui8NewAccelFSSel = (LSM303D_CTRL2_AFS_2G >> LSM303D_CTRL2_AFS_S);
    psInst->ui8MagFSSel = (LSM303D_CTRL6_MFS_2G >> LSM303D_CTRL6_MFS_S);
    psInst->ui8NewMagFSSel = (LSM303D_CTRL6_MFS_2G >> LSM303D_CTRL6_MFS_S);

    //
    // There is no soft reset on the LSM303.  Force registers back to their
    // spec'ed POR defaults.
    //
    psInst->ui8State = LSM303D_STATE_INIT;
    if(I2CMWrite(psInst->psI2CInst, psInst->ui8Addr, g_pui8ZeroInit,
                 sizeof(g_pui8ZeroInit), LSM303DCallback, (void *)psInst) == 0)
    {
        psInst->ui8State = LSM303D_STATE_IDLE;
        return(0);
    }

    //
    // Success
    //
    return(1);
}

//*****************************************************************************
//
//! Reads data from LSM303D registers.
//!
//! \param psInst is a pointer to the LSM303D instance data.
//! \param ui8Reg is the first register to read.
//! \param pui8Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count is the number of data bytes to read.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the LSM303D.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
LSM303DRead(tLSM303D *psInst, uint_fast8_t ui8Reg,
                    uint8_t *pui8Data, uint_fast16_t ui16Count,
                    tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the LSM303D driver is not idle (in other words,
    // there is already an outstanding request to the LSM303D).
    //
    if(psInst->ui8State != LSM303D_STATE_IDLE)
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
    psInst->ui8State = LSM303D_STATE_READ_MAG;

    //
    // Read the requested registers from the LSM303D.
    //
    psInst->uCommand.pui8Buffer[0] = ui8Reg;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, pui8Data, ui16Count,
                LSM303DCallback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = LSM303D_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Writes data to LSM303D registers.
//!
//! \param psInst is a pointer to the LSM303D instance data.
//! \param ui8Reg is the first register to write.
//! \param pui8Data is a pointer to the data to write.
//! \param ui16Count is the number of data bytes to write.
//! \param pfnCallback is the function to be called when the data has been
//! written (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the LSM303D.  The first byte of the \e pui8Data buffer contains the
//! value to be written into the \e ui8Reg register, the second value contains
//! the data to be written into the next register, and so on.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
LSM303DWrite(tLSM303D *psInst, uint_fast8_t ui8Reg,
                     const uint8_t *pui8Data, uint_fast16_t ui16Count,
                     tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the LSM303D driver is not idle (in other words,
    // there is already an outstanding request to the LSM303D).
    //
    if(psInst->ui8State != LSM303D_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // See if the accel full scale select register is being written.
    //
    if((ui8Reg <= LSM303D_O_CTRL2) &&
       ((ui8Reg + ui16Count) > LSM303D_O_CTRL2))
    {
        //
        // Extract the AFS_SEL from the ACCEL_CONFIG register value.
        //
        psInst->ui8NewAccelFSSel =
            ((pui8Data[ui8Reg - LSM303D_O_CTRL2] &
              LSM303D_CTRL2_AFS_M) >> LSM303D_CTRL2_AFS_S);
    }

    //
    // See if the mag full scale select register is being written.
    //
    if((ui8Reg <= LSM303D_O_CTRL6) &&
       ((ui8Reg + ui16Count) > LSM303D_O_CTRL6))
    {
        //
        // Extract the AFS_SEL from the ACCEL_CONFIG register value.
        //
        psInst->ui8NewMagFSSel =
            ((pui8Data[ui8Reg - LSM303D_O_CTRL6] &
              LSM303D_CTRL6_MFS_M) >> LSM303D_CTRL6_MFS_S);
    }

    //
    // Move the state machine to the wait for write state.
    //
    psInst->ui8State = LSM303D_STATE_WRITE;

    //
    // Write the requested registers to the LSM303D.
    //
    if(I2CMWrite8(&(psInst->uCommand.sWriteState), psInst->psI2CInst,
                  psInst->ui8Addr, ui8Reg, pui8Data, ui16Count,
                  LSM303DCallback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = LSM303D_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs a read-modify-write of a LSM303D register.
//!
//! \param psInst is a pointer to the LSM303D instance data.
//! \param ui8Reg is the register to modify.
//! \param ui8Mask is the bit mask that is ANDed with the current register
//! value.
//! \param ui8Value is the bit mask that is ORed with the result of the AND
//! operation.
//! \param pfnCallback is the function to be called when the data has been
//! changed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function changes the value of a register in the LSM303D via a
//! read-modify-write operation, allowing one of the fields to be changed
//! without disturbing the other fields.  The \e ui8Reg register is read, ANDed
//! with \e ui8Mask, ORed with \e ui8Value, and then written back to the
//! LSM303D.
//!
//! \return Returns 1 if the read-modify-write was successfully started and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
LSM303DReadModifyWrite(tLSM303D *psInst, uint_fast8_t ui8Reg,
                               uint_fast8_t ui8Mask, uint_fast8_t ui8Value,
                               tSensorCallback *pfnCallback,
                               void *pvCallbackData)
{
    //
    // Return a failure if the LSM303D driver is not idle (in other words,
    // there is already an outstanding request to the LSM303D).
    //
    if(psInst->ui8State != LSM303D_STATE_IDLE)
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
    psInst->ui8State = LSM303D_STATE_RMW;

    //
    // Submit the read-modify-write request to the LSM303D.
    //
    if(I2CMReadModifyWrite8(&(psInst->uCommand.sReadModifyWriteState),
                            psInst->psI2CInst, psInst->ui8Addr, ui8Reg,
                            ui8Mask, ui8Value, LSM303DCallback,
                            psInst) == 0)
    {
        //
        // The I2C read-modify-write failed, so move to the idle state and
        // return a failure.
        //
        psInst->ui8State = LSM303D_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads the accelerometer data from the LSM303D.
//!
//! \param psInst is a pointer to the LSM303D instance data.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read of the LSM303D data registers.  When the
//! read has completed (as indicated by calling the callback function), the new
//! readings can be obtained via:
//!
//! - LSM303DDataAccelGetRaw()
//! - LSM303DDataAccelGetFloat()
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
LSM303DDataRead(tLSM303D *psInst, tSensorCallback *pfnCallback,
                        void *pvCallbackData)
{
    //
    // Return a failure if the LSM303D driver is not idle (in other words,
    // there is already an outstanding request to the LSM303D).
    //
    if(psInst->ui8State != LSM303D_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Move the state machine to the wait for mag data read state.
    //
    psInst->ui8State = LSM303D_STATE_READ_MAG;

    //
    // Read the data registers from the LSM303D.
    //
    psInst->pui8DataMag[0] = LSM303D_O_MAG_STATUS | 0x80;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr, psInst->pui8DataMag, 1,
                psInst->pui8DataMag, 7, LSM303DCallback, psInst) == 0)
    {
        //
        // The I2C read failed, so move to the idle state and return a failure.
        //
        psInst->ui8State = LSM303D_STATE_IDLE;
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
//! \param psInst is a pointer to the LSM303D instance data.
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
LSM303DDataAccelGetRaw(tLSM303D *psInst,
                               uint_fast16_t *pui16AccelX,
                               uint_fast16_t *pui16AccelY,
                               uint_fast16_t *pui16AccelZ)
{
    //
    // Return the raw accelerometer values.
    //
    if(pui16AccelX)
    {
        *pui16AccelX = (psInst->pui8DataAccel[2] << 8) | psInst->pui8DataAccel[1];
    }
    if(pui16AccelY)
    {
        *pui16AccelY = (psInst->pui8DataAccel[4] << 8) | psInst->pui8DataAccel[3];
    }
    if(pui16AccelZ)
    {
        *pui16AccelZ = (psInst->pui8DataAccel[6] << 8) | psInst->pui8DataAccel[5];
    }
}

//*****************************************************************************
//
//! Gets the raw accelerometer data from the most recent data read.
//!
//! \param psInst is a pointer to the LSM303D instance data.
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
LSM303DDataMagnetoGetRaw(tLSM303D *psInst,
                               uint_fast16_t *pui16AccelX,
                               uint_fast16_t *pui16AccelY,
                               uint_fast16_t *pui16AccelZ)
{
    //
    // Return the raw accelerometer values.
    //
    if(pui16AccelX)
    {
        *pui16AccelX = (psInst->pui8DataMag[2] << 8) | psInst->pui8DataMag[1];
    }
    if(pui16AccelY)
    {
        *pui16AccelY = (psInst->pui8DataMag[4] << 8) | psInst->pui8DataMag[3];
    }
    if(pui16AccelZ)
    {
        *pui16AccelZ = (psInst->pui8DataMag[6] << 8) | psInst->pui8DataMag[5];
    }
}

//*****************************************************************************
//
//! Gets the accelerometer data from the most recent data read.
//!
//! \param psInst is a pointer to the LSM303D instance data.
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
LSM303DDataAccelGetFloat(tLSM303D *psInst, float *pfAccelX,
                                 float *pfAccelY, float *pfAccelZ)
{
    float fFactor;

    //
    // Get the acceleration conversion factor for the current data format.
    //
    fFactor = g_pfLSM303DAccelFactors[psInst->ui8AccelFSSel];

    //
    // Convert the Accelerometer values into floating-point gravity values.
    //
    if(pfAccelX)
    {
        *pfAccelX = (float)(((int16_t)((psInst->pui8DataAccel[2] << 8) |
                                       psInst->pui8DataAccel[1])) * fFactor);
    }
    if(pfAccelY)
    {
        *pfAccelY = (float)(((int16_t)((psInst->pui8DataAccel[4] << 8) |
                                       psInst->pui8DataAccel[3])) * fFactor);
    }
    if(pfAccelZ)
    {
        *pfAccelZ = (float)(((int16_t)((psInst->pui8DataAccel[6] << 8) |
                                       psInst->pui8DataAccel[5])) * fFactor);
    }
}

//*****************************************************************************
//
//! Gets the magnetometer data from the most recent data read.
//!
//! \param psInst is a pointer to the LSM303D instance data.
//! \param pfMagX is a pointer to the value into which the X-axis
//! accelerometer data is stored.
//! \param pfMagY is a pointer to the value into which the Y-axis
//! accelerometer data is stored.
//! \param pfMagZ is a pointer to the value into which the Z-axis
//! accelerometer data is stored.
//!
//! This function returns the magnetometer data from the most recent data
//! read, converted into tesla.  If any of the output data pointers are
//! \b NULL, the corresponding data is not provided.
//!
//! \return None.
//
//*****************************************************************************
void
LSM303DDataMagnetoGetFloat(tLSM303D *psInst, float *pfMagX,
                                 float *pfMagY, float *pfMagZ)
{
    float fFactor;

    //
    // Get the magnetometer conversion factor for the current data format.
    //
    fFactor = g_pfLSM303DMagFactors[psInst->ui8MagFSSel];

    //
    // Convert the Accelerometer values into floating-point gravity values.
    //
    if(pfMagX)
    {
        *pfMagX = (float)(((int16_t)((psInst->pui8DataMag[2] << 8) |
                                       psInst->pui8DataMag[1])) * fFactor);
    }
    if(pfMagY)
    {
        *pfMagY = (float)(((int16_t)((psInst->pui8DataMag[4] << 8) |
                                       psInst->pui8DataMag[3])) * fFactor);
    }
    if(pfMagZ)
    {
        *pfMagZ = (float)(((int16_t)((psInst->pui8DataMag[6] << 8) |
                                       psInst->pui8DataMag[5])) * fFactor);
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
