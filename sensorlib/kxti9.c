//*****************************************************************************
//
// kxti9.c - Driver for the KXTI9 accelerometer.
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

#include <stdint.h>
#include "sensorlib/hw_kxti9.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/kxti9.h"

//*****************************************************************************
//
//! \addtogroup kxti9_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The states of the KXTI9 state machine.
//
//*****************************************************************************
#define KXTI9_STATE_IDLE        0           // State machine is idle
#define KXTI9_STATE_INIT_RES    1           // Waiting for intialization
#define KXTI9_STATE_INIT_WAIT   2           // Waiting for reset to complete
#define KXTI9_STATE_LAST        3           // Last state of init
#define KXTI9_STATE_READ        4           // Waiting for read
#define KXTI9_STATE_WRITE       5           // Waiting for write
#define KXTI9_STATE_RMW         6           // Waiting for read-modify-write

//*****************************************************************************
//
// The factors used to convert the 8-bit acceleration readings from the KXTI9
// into floating point values in m/s^2.
//
//*****************************************************************************
static const float g_fAccelFactors8[] =
{
    (2.0 * 9.81) / 128.0,
    (4.0 * 9.81) / 128.0,
    (8.0 * 9.81) / 128.0
};

//*****************************************************************************
//
// The factors used to convert the 12-bit acceleration readings from the KXTI9
// into floating point values in m/s^2.
//
//*****************************************************************************
static const float g_fAccelFactors12[] =
{
    (2.0 * 9.81) / 2048.0,
    (4.0 * 9.81) / 2048.0,
    (8.0 * 9.81) / 2048.0
};

//*****************************************************************************
//
// The callback function that is called when I2C transations to/from the KXTI9
// have completed.
//
//*****************************************************************************
static void
KXTI9Callback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tKXTI9 *psInst;

    //
    // Convert the instance data into a pointer to a tKXTI9 structure.
    //
    psInst = pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result in a callback to propagate the
    // error).
    //
    if((ui8Status != I2CM_STATUS_SUCCESS) &&
       (psInst->ui8State != KXTI9_STATE_INIT_WAIT))
    {
        psInst->ui8State = KXTI9_STATE_IDLE;
    }

    //
    // Determine the current state of the KXTI9 state machine.
    //
    switch(psInst->ui8State)
    {
        //
        // All states that trivially transition to IDLE, and all unknown
        // states.
        //
        case KXTI9_STATE_LAST:
        case KXTI9_STATE_READ:
        default:
        {
            //
            // The state machine is now idle.
            //
            psInst->ui8State = KXTI9_STATE_IDLE;

            //
            // Done.
            //
            break;
        }

        case KXTI9_STATE_INIT_RES:
        {
            //
            // Try to read back to determine if reset is done.  We expect to see
            // a NAK.
            //
            psInst->uCommand.pui8Buffer[0] = KXTI9_O_CTRL3;
            I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                     psInst->uCommand.pui8Buffer, 1, psInst->pui8Data, 1,
                     KXTI9Callback, psInst);

            psInst->ui8State = KXTI9_STATE_INIT_WAIT;
            break;
        }

        case KXTI9_STATE_INIT_WAIT:
        {
            //
            // Check to see if there was finally an ACK.
            //
            if(ui8Status != I2CM_STATUS_SUCCESS)
            {
                //
                // Read again.
                //
                psInst->uCommand.pui8Buffer[0] = KXTI9_O_CTRL3;
                I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                         psInst->uCommand.pui8Buffer, 1, psInst->pui8Data, 1,
                         KXTI9Callback, psInst);
            }
            else
            {
                //
                // Check the read data to make sure it jibes.
                //
                if(psInst->pui8Data[0] == 0x4d)
                {
                    //
                    // Device is out of reset, enable the device.
                    //
                    psInst->uCommand.pui8Buffer[0] = KXTI9_O_CTRL1;
                    psInst->uCommand.pui8Buffer[1] = KXTI9_CTRL1_PC1;
                    I2CMWrite(psInst->psI2CInst, psInst->ui8Addr,
                              psInst->uCommand.pui8Buffer, 2, KXTI9Callback,
                              psInst);
                }
                else
                {
                    ui8Status = I2CM_STATUS_ERROR;
                }

                //
                // This is the last init write.
                //
                psInst->ui8State = KXTI9_STATE_LAST;
            }
            break;
        }

        case KXTI9_STATE_WRITE:
        {
            //
            // Set the accelerometer range and resolution to the new value.
            // If the register was not modified, the values will be the same so
            // this has no effect.
            //
            psInst->ui8Resolution = psInst->ui8NewResolution;
            psInst->ui8Range = psInst->ui8NewRange;

            //
            // The state machine is now idle.
            //
            psInst->ui8State = KXTI9_STATE_IDLE;
            break;
        }

        case KXTI9_STATE_RMW:
        {
            //
            // See if the CTRL3 register was just modified.
            //
            if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[0] ==
                    KXTI9_O_CTRL3)
            {
                //
                // See if a soft reset has been issued.
                //
                if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[1] &
                        KXTI9_CTRL3_SRST)
                {
                    //
                    // Default range setting is +/- 2 g
                    //
                    psInst->ui8Range = 0;
                    psInst->ui8NewRange = 0;

                    //
                    // Default resolution is 8-bit.
                    //
                    psInst->ui8Resolution = 0;
                    psInst->ui8NewResolution = 0;
                }
            }

            //
            // See if the CTRL1 register was just modified.
            //
            if(psInst->uCommand.sReadModifyWriteState.pui8Buffer[0] ==
                    KXTI9_O_CTRL1)
            {
                //
                // Extract the range and resolution from the register value.
                //
                psInst->ui8Range =
                    ((psInst->uCommand.sReadModifyWriteState.pui8Buffer[1] &
                            KXTI9_CTRL1_GSEL_M) >> KXTI9_CTRL1_GSEL_S);
                psInst->ui8Resolution =
                    ((psInst->uCommand.sReadModifyWriteState.pui8Buffer[1] &
                            KXTI9_CTRL1_RES) >> 6);
            }

            //
            // The state machine is now idle.
            //
            psInst->ui8State = KXTI9_STATE_IDLE;
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == KXTI9_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //s
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Initializes the KXTI9 driver.
//!
//! \param psInst is a pointer to the KXTI9 instance data.
//! \param psI2CInst is a pointer to the I2C master driver instance data.
//! \param ui8I2CAddr is the I2C address of the KXTI9 device.
//! \param pfnCallback is the function to be called when the initialization has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initializes the KXTI9 driver, preparing it for operation.
//!
//! \return Returns 1 if the KXTI9 driver was successfully initialized and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
KXTI9Init(tKXTI9 *psInst, tI2CMInstance *psI2CInst, uint_fast8_t ui8I2CAddr,
           tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Initialize the KXTI9 instance structure.
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8I2CAddr;
    psInst->ui8State = KXTI9_STATE_INIT_RES;
    psInst->ui8Resolution = 0;
    psInst->ui8NewResolution = 0;
    psInst->ui8Range = KXTI9_CTRL1_GSEL_2G >> KXTI9_CTRL1_GSEL_S;
    psInst->ui8NewRange = KXTI9_CTRL1_GSEL_2G >> KXTI9_CTRL1_GSEL_S;

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Write the EE_W bit of CTRL_REG0 (allowing the configuration registers to
    // be modified).
    //
    psInst->pui8Data[0] = KXTI9_O_CTRL3;
    psInst->pui8Data[1] = KXTI9_CTRL3_SRST;
    if(I2CMWrite(psInst->psI2CInst, ui8I2CAddr, psInst->pui8Data, 2,
                 KXTI9Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = KXTI9_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads data from KXTI9 registers.
//!
//! \param psInst is a pointer to the KXTI9 instance data.
//! \param ui8Reg is the first register to read.
//! \param pui8Data is a pointer to the location to store the data that is
//! read.
//! \param ui16Count is the number of data bytes to read.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function reads a sequence of data values from consecutive registers in
//! the KXTI9.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
KXTI9Read(tKXTI9 *psInst, uint_fast8_t ui8Reg, uint8_t *pui8Data,
           uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
           void *pvCallbackData)
{
    //
    // Return a failure if the KXTI9 driver is not idle (in other words, there
    // is already an outstanding request to the KXTI9).
    //
    if(psInst->ui8State != KXTI9_STATE_IDLE)
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
    psInst->ui8State = KXTI9_STATE_READ;

    //
    // Read the requested registers from the KXTI9.
    //
    psInst->uCommand.pui8Buffer[0] = ui8Reg;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr,
                psInst->uCommand.pui8Buffer, 1, pui8Data, ui16Count,
                KXTI9Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = KXTI9_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Writes data to KXTI9 registers.
//!
//! \param psInst is a pointer to the KXTI9 instance data.
//! \param ui8Reg is the first register to write.
//! \param pui8Data is a pointer to the data to write.
//! \param ui16Count is the number of data bytes to write.
//! \param pfnCallback is the function to be called when the data has been
//! written (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function writes a sequence of data values to consecutive registers in
//! the KXTI9.  The first byte of the \e pui8Data buffer contains the value to
//! be written into the \e ui8Reg register, the second value contains the data
//! to be written into the next register, and so on.
//!
//! \return Returns 1 if the write was successfully started and 0 if it was
//! not.
//
//*****************************************************************************
uint_fast8_t
KXTI9Write(tKXTI9 *psInst, uint_fast8_t ui8Reg, uint8_t *pui8Data,
            uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
            void *pvCallbackData)
{
    //
    // Return a failure if the KXTI9 driver is not idle (in other words, there
    // is already an outstanding request to the KXTI9).
    //
    if(psInst->ui8State != KXTI9_STATE_IDLE)
    {
        return(0);
    }

    //
    // Save the callback information.
    //
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    psInst->ui8NewRange = psInst->ui8Range;
    psInst->ui8NewResolution = psInst->ui8Resolution;

    //
    // See if the CTRL3 register is being written.
    //
    if((ui8Reg <= KXTI9_O_CTRL3) &&
       ((ui8Reg + ui16Count) > KXTI9_O_CTRL3))
    {
        //
        // See if a soft reset is being requested.
        //
        if(pui8Data[ui8Reg - KXTI9_O_CTRL3] & KXTI9_CTRL3_SRST)
        {
            //
            // Default range setting is +/- 2 g.
            //
            psInst->ui8NewRange = 0;

            //
            // Default resolution is 8-bit.
            //
            psInst->ui8NewResolution = 0;
        }
    }

    //
    // See if the CTRL1 register is being written.
    //
    if((ui8Reg <= KXTI9_O_CTRL1) &&
       ((ui8Reg + ui16Count) > KXTI9_O_CTRL1))
    {
        //
        // Extract the range and resolution the register value.
        //
        psInst->ui8NewRange =
            ((pui8Data[ui8Reg - KXTI9_O_CTRL1] & KXTI9_CTRL1_GSEL_M)
                    >> KXTI9_CTRL1_GSEL_S);
        psInst->ui8NewResolution =
                ((pui8Data[ui8Reg - KXTI9_O_CTRL1] & KXTI9_CTRL1_RES) >> 6);
    }

    //
    // Save the details of this write.
    //
    psInst->uCommand.sWriteState.pui8Data = pui8Data;
    psInst->uCommand.sWriteState.ui16Count = ui16Count;

    //
    // Move the state machine to the wait for write state.
    //
    psInst->ui8State = KXTI9_STATE_WRITE;

    //
    // Write the requested registers to the KXTI9.
    //
    pui8Data[0] = ui8Reg;
    if(I2CMWrite(psInst->psI2CInst, psInst->ui8Addr, pui8Data, ui16Count + 1,
                 KXTI9Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so move to the idle state and return a
        // failure.
        //
        psInst->ui8State = KXTI9_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs a read-modify-write of a KXTI9 register.
//!
//! \param psInst is a pointer to the KXTI9 instance data.
//! \param ui8Reg is the register to modify.
//! \param ui8Mask is the bit mask that is ANDed with the current register
//! value.
//! \param ui8Value is the bit mask that is ORed with the result of the AND
//! operation.
//! \param pfnCallback is the function to be called when the data has been
//! changed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function changes the value of a register in the KXTI9 via a
//! read-modify-write operation, allowing one of the fields to be changed
//! without disturbing the other fields.  The \e ui8Reg register is read, ANDed
//! with \e ui8Mask, ORed with \e ui8Value, and then written back to the
//! KXTI9.
//!
//! \return Returns 1 if the read-modify-write was successfully started and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
KXTI9ReadModifyWrite(tKXTI9 *psInst, uint_fast8_t ui8Reg,
                      uint_fast8_t ui8Mask, uint_fast8_t ui8Value,
                      tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Return a failure if the KXTI9 driver is not idle (in other words, there
    // is already an outstanding request to the KXTI9).
    //
    if(psInst->ui8State != KXTI9_STATE_IDLE)
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
    psInst->ui8State = KXTI9_STATE_RMW;

    //
    // Submit the read-modify-write request to the KXTI9.
    //
    if(I2CMReadModifyWrite8(&(psInst->uCommand.sReadModifyWriteState),
                            psInst->psI2CInst, psInst->ui8Addr, ui8Reg,
                            ui8Mask, ui8Value, KXTI9Callback, psInst) == 0)
    {
        //
        // The I2C read-modify-write failed, so move to the idle state and
        // return a failure.
        //
        psInst->ui8State = KXTI9_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Reads the acceleration and temperature data from the KXTI9.
//!
//! \param psInst is a pointer to the KXTI9 instance data.
//! \param pfnCallback is the function to be called when the data has been read
//! (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read of the KXTI9 data registers.  When the read
//! has completed (as indicated by calling the callback function), the new
//! readings can be obtained via:
//!
//! - KXTI9DataAccelGetRaw()
//! - KXTI9DataAccelGetFloat()
//! - KXTI9DataTemperatureGetRaw()
//! - KXTI9DataTemperatureGetFloat()
//!
//! \return Returns 1 if the read was successfully started and 0 if it was not.
//
//*****************************************************************************
uint_fast8_t
KXTI9DataRead(tKXTI9 *psInst, tSensorCallback *pfnCallback,
               void *pvCallbackData)
{
    //
    // Return a failure if the KXTI9 driver is not idle (in other words, there
    // is already an outstanding request to the KXTI9).
    //
    if(psInst->ui8State != KXTI9_STATE_IDLE)
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
    psInst->ui8State = KXTI9_STATE_READ;

    //
    // Read the data registers from the KXTI9.
    //
    psInst->pui8Data[0] = KXTI9_O_XOUT_L;
    if(I2CMRead(psInst->psI2CInst, psInst->ui8Addr, psInst->pui8Data, 1,
                psInst->pui8Data, 6, KXTI9Callback, psInst) == 0)
    {
        //
        // The I2C read failed, so move to the idle state and return a failure.
        //
        psInst->ui8State = KXTI9_STATE_IDLE;
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Gets the raw acceleration data from the most recent data read.
//!
//! \param psInst is a pointer to the KXTI9 instance data.
//! \param pui16AccelX is a pointer to the value into which the raw X-axis
//! acceleration data is stored.
//! \param pui16AccelY is a pointer to the value into which the raw Y-axis
//! acceleration data is stored.
//! \param pui16AccelZ is a pointer to the value into which the raw Z-axis
//! acceleration data is stored.
//!
//! This function returns the raw acceleration data from the most recent data
//! read.  The data is not manipulated in any way by the driver.  If any of the
//! output data pointers are \b NULL, the corresponding data is not provided.
//!
//! \return None.
//
//*****************************************************************************
void
KXTI9DataAccelGetRaw(tKXTI9 *psInst, uint_fast16_t *pui16AccelX,
                      uint_fast16_t *pui16AccelY, uint_fast16_t *pui16AccelZ)
{
    //
    // Return the raw acceleration values.
    //
    if(pui16AccelX)
    {
        *pui16AccelX = ((psInst->pui8Data[1] << 4) |
                        (psInst->pui8Data[0] >> 4));
    }
    if(pui16AccelY)
    {
        *pui16AccelY = ((psInst->pui8Data[3] << 4) |
                        (psInst->pui8Data[2] >> 4));
    }
    if(pui16AccelZ)
    {
        *pui16AccelZ = ((psInst->pui8Data[5] << 4) |
                        (psInst->pui8Data[4] >> 4));
    }
}

//*****************************************************************************
//
//! Gets the acceleration data from the most recent data read.
//!
//! \param psInst is a pointer to the KXTI9 instance data.
//! \param pfAccelX is a pointer to the value into which the X-axis
//! acceleration data is stored.
//! \param pfAccelY is a pointer to the value into which the Y-axis
//! acceleration data is stored.
//! \param pfAccelZ is a pointer to the value into which the Z-axis
//! acceleration data is stored.
//!
//! This function returns the acceleration data from the most recent data read,
//! converted into g.  If any of the output data pointers are \b NULL, the
//! corresponding data is not provided.
//!
//! \return None.
//
//*****************************************************************************
void
KXTI9DataAccelGetFloat(tKXTI9 *psInst, float *pfAccelX, float *pfAccelY,
                        float *pfAccelZ)
{
    float fFactor;
    int16_t iX, iY, iZ;

    //
    // Get the acceleration conversion factor for the current range.
    //
    fFactor = (psInst->ui8Resolution == 0) ? g_fAccelFactors8[psInst->ui8Range] :
                                             g_fAccelFactors12[psInst->ui8Range];

    if(psInst->ui8Resolution)
    {
        //
        // Get conversion data and store in temporary variables.
        //
        iX = (int16_t)((psInst->pui8Data[1] << 4) | (psInst->pui8Data[0] >> 4));
        iY = (int16_t)((psInst->pui8Data[3] << 4) | (psInst->pui8Data[2] >> 4));
        iZ = (int16_t)((psInst->pui8Data[5] << 4) | (psInst->pui8Data[4] >> 4));

        //
        // Sign extend 12-bit data.
        //
        iX |= (iX & 0x800) ? 0xf000 : 0;
        iY |= (iY & 0x800) ? 0xf000 : 0;
        iZ |= (iZ & 0x800) ? 0xf000 : 0;
    }
    else
    {
        //
        // Chop off the lower 4 bits of the data.  8-bit mode only returns 8
        // valid bits, but can have garbage in the lower 4.
        //
        iX = (int16_t)(psInst->pui8Data[1]);
        iY = (int16_t)(psInst->pui8Data[3]);
        iZ = (int16_t)(psInst->pui8Data[5]);

        //
        // Sign extend 8-bit data.
        //
        iX |= (iX & 0x80) ? 0xff00 : 0;
        iY |= (iY & 0x80) ? 0xff00 : 0;
        iZ |= (iZ & 0x80) ? 0xff00 : 0;
    }

    //
    // Convert the acceleration values into floating-point g values.
    //
    if(pfAccelX)
    {
        *pfAccelX = (float)(iX) * fFactor;
    }
    if(pfAccelY)
    {
        *pfAccelY = (float)(iY) * fFactor;
    }
    if(pfAccelZ)
    {
        *pfAccelZ = (float)(iZ) * fFactor;
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
