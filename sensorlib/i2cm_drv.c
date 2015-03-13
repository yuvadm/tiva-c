//*****************************************************************************
//
// i2cm_drv.c - Interrupt-driven I2C master driver.
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

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/i2c.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "sensorlib/i2cm_drv.h"

//*****************************************************************************
//
//! \addtogroup i2cm_drv_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The states in the interrupt handler state machine.
//
//*****************************************************************************
#define STATE_IDLE              0
#define STATE_WRITE_NEXT        1
#define STATE_WRITE_FINAL       2
#define STATE_WRITE_PAUSE       3
#define STATE_READ_ONE          4
#define STATE_READ_FIRST        5
#define STATE_READ_NEXT         6
#define STATE_READ_FINAL        7
#define STATE_READ_PAUSE        8
#define STATE_READ_WAIT         9
#define STATE_CALLBACK          10

//*****************************************************************************
//
// The states in the I2C read-modify-write state machine.
//
//*****************************************************************************
#define I2CM_RMW_STATE_IDLE     0
#define I2CM_RMW_STATE_READ     1
#define I2CM_RMW_STATE_WRITE    2

//*****************************************************************************
//
//! Writes data to an I2C device.
//!
//! \param psInst is a pointer to the I2C master instance data.
//! \param ui8Addr is the address of the I2C device to access.
//! \param pui8Data is a pointer to the data buffer to be written.
//! \param ui16Count is the number of bytes to be written.
//! \param pfnCallback is the function to be called when the write has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function adds an I2C write to the queue of commands to be sent.  If
//! successful, the I2C write is then performed in the background using the
//! interrupt handler.  When the write is complete, the callback function, if
//! provided, is called in the context of the I2C master interrupt handler.
//!
//! The first byte of the data buffer contains the I2C address of the device to
//! access, and the remaining \e ui16Count bytes contain the data to be written
//! to the device.  The \e ui16Count parameter can be zero if there are no
//! bytes to be written.
//!
//! \return Returns 1 if the command was successfully added to the queue and 0
//! if it was not.
//
// The extern here provides a non-inline definition for this function to handle
// the case where the compiler chooses not to inline the function (which is a
// valid choice for the compiler to make).
//
//*****************************************************************************
extern uint_fast8_t I2CMWrite(tI2CMInstance *psInst, uint_fast8_t ui8Addr,
                              const uint8_t *pui8Data, uint_fast16_t ui16Count,
                              tSensorCallback pfnCallback,
                              void *pvCallbackData);

//*****************************************************************************
//
//! Reads data from an I2C device.
//!
//! \param psInst is a pointer to the I2C master instance data.
//! \param ui8Addr is the address of the I2C device to access.
//! \param pui8WriteData is a pointer to the data buffer to be written.
//! \param ui16WriteCount is the number of bytes to be written.
//! \param pui8ReadData is a pointer to the buffer to be filled with the read
//! data.
//! \param ui16ReadCount is the number of bytes to be read.
//! \param pfnCallback is the function to be called when the transfer has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function adds an I2C read to the queue of commands to be sent.  If
//! successful, the I2C read is then performed in the background using the
//! interrupt handler.  When the read is complete, the callback function, if
//! provided, is called in the context of the I2C master interrupt handler.
//!
//! The first byte of \e pui8WriteData contains the I2C address of the device
//! to access, the next \e ui16WriteCount bytes contains the data to be written
//! to the device.  The data read from the device is written into the first
//! \e ui16ReadCount bytes of \e pui8ReadData.  The \e ui16WriteCount or
//! \e ui16ReadCount parameters can be zero if there are no bytes to be read or
//! written.  The write bytes are sent to the device first, and then the read
//! bytes are read from the device afterward.
//!
//! \return Returns 1 if the command was successfully added to the queue and 0
//! if it was not.
//
// The extern here provides a non-inline definition for this function to handle
// the case where the compiler chooses not to inline the function (which is a
// valid choice for the compiler to make).
//
//*****************************************************************************
extern uint_fast8_t I2CMRead(tI2CMInstance *psInst, uint_fast8_t ui8Addr,
                             const uint8_t *pui8WriteData,
                             uint_fast16_t ui16WriteCount,
                             uint8_t *pui8ReadData,
                             uint_fast16_t ui16ReadCount,
                             tSensorCallback pfnCallback,
                             void *pvCallbackData);

//*****************************************************************************
//
//! Writes data in batches to an I2C device.
//!
//! \param psInst is a pointer to the I2C master instance data.
//! \param ui8Addr is the address of the I2C device to access.
//! \param pui8Data is a pointer to the data buffer to be written.
//! \param ui16Count is the number of bytes to be written.
//! \param ui16BatchSize is the number of bytes in each write batch.
//! \param pfnCallback is the function to be called when the transfer has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function adds an I2C write to the queue of commands to be sent.  If
//! successful, the I2C write is then performed in the background using the
//! interrupt handler.  When the write is complete, the callback function, if
//! provided, is called in the context of the I2C master interrupt handler.
//!
//! The first byte of the data buffer contains the I2C address of the device to
//! access, and the remaining \e ui16Count bytes contain the data to be written
//! to the device.  The \e ui16Count parameter can be zero if there are no
//! bytes to be written.
//!
//! The data is written in batches of \e ui16WriteBatchSize.  The callback
//! function is called after each batch is written, and I2CMTransferResume()
//! must be called when the next batch should be written.
//!
//! \return Returns 1 if the command was successfully added to the queue and 0
//! if it was not.
//
// The extern here provides a non-inline definition for this function to handle
// the case where the compiler chooses not to inline the function (which is a
// valid choice for the compiler to make).
//
//*****************************************************************************
extern uint_fast8_t I2CMWriteBatched(tI2CMInstance *psInst,
                                     uint_fast8_t ui8Addr,
                                     const uint8_t *pui8Data,
                                     uint_fast16_t ui16Count,
                                     uint_fast16_t ui16BatchSize,
                                     tSensorCallback pfnCallback,
                                     void *pvCallbackData);

//*****************************************************************************
//
//! Reads data in batches from an I2C device.
//!
//! \param psInst is a pointer to the I2C master instance data.
//! \param ui8Addr is the address of the I2C device to access.
//! \param pui8WriteData is a pointer to the data buffer to be written.
//! \param ui16WriteCount is the number of bytes to be written.
//! \param ui16WriteBatchSize is the number of bytes in each write batch.
//! \param pui8ReadData is a pointer to the buffer to be filled with the read
//! data.
//! \param ui16ReadCount is the number of bytes to be read.
//! \param ui16ReadBatchSize is the number of bytes in each read batch.
//! \param pfnCallback is the function to be called when the transfer has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function adds an I2C read to the queue of commands to be sent.  If
//! successful, the I2C read is then performed in the background using the
//! interrupt handler.  When the read is complete, the callback function, if
//! provided, is called in the context of the I2C master interrupt handler.
//!
//! The first byte of \e pui8WriteData contains the I2C address of the device
//! to access, the next \e ui16WriteCount bytes contains the data to be written
//! to the device.  The data read from the device is written into the first
//! \e ui16ReadCount bytes of \e pui8ReadData.  The \e ui16WriteCount or
//! \e ui16ReadCount parameters can be zero if there are no bytes to be read or
//! written.  The write bytes are sent to the device first, and then the read
//! bytes are read from the device afterward.
//!
//! The data is written in batches of \e ui16WriteBatchSize.  The callback
//! function is called after each batch is written, and I2CMTransferResume()
//! must be called when the next batch should be written.
//!
//! The data is read in batches of \e ui16ReadBatchSize.  The callback function
//! is called after each batch is read, and I2CMTransferResume() must be called
//! when the next batch should be read.
//!
//! \return Returns 1 if the command was successfully added to the queue and 0
//! if it was not.
//
// The extern here provides a non-inline definition for this function to handle
// the case where the compiler chooses not to inline the function (which is a
// valid choice for the compiler to make).
//
//*****************************************************************************
extern uint_fast8_t I2CMReadBatched(tI2CMInstance *psInst,
                                    uint_fast8_t ui8Addr,
                                    const uint8_t *pui8WriteData,
                                    uint_fast16_t ui16WriteCount,
                                    uint_fast16_t ui16WriteBatchSize,
                                    uint8_t *pui8ReadData,
                                    uint_fast16_t ui16ReadCount,
                                    uint_fast16_t ui16ReadBatchSize,
                                    tSensorCallback pfnCallback,
                                    void *pvCallbackData);

//*****************************************************************************
//
//! Performs a read-modify-write of 16 bits of big-endian data in an I2C
//! device.
//!
//! \param psInst is a pointer to the read-modify-write instance data.
//! \param psI2CInst is a pointer to the I2C master instance data.
//! \param ui8Addr is the address of the I2C device to access.
//! \param ui8Reg is the register in the I2C device to access.
//! \param ui16Mask is the mask indicating the register bits that should be
//! maintained.
//! \param ui16Value is the value indicating the new value for the register
//! bits that are not maintained.
//! \param pfnCallback is the function to be called when the write has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read-modify-write transaction of 16 bits of
//! big-endian data in an I2C device.  The modify portion of the operation is
//! performed by AND-ing the register value with \e ui16Mask and then OR-ing
//! the result with \e ui16Value.  When the read-modify-write is complete, the
//! callback function, if provided, is called in the context of the I2C master
//! interrupt handler.
//!
//! If the mask (in \e ui16Mask) is zero, then none of the bits in the current
//! register value are maintained.  In this case, the read portion of the
//! read-modify-write is bypassed, and the new register value (in \e ui16Value)
//! is directly written to the I2C device.
//!
//! \return Returns 1 if the command was successfully added to the queue and 0
//! if it was not.
//
// The extern here provides a non-inline definition for this function to handle
// the case where the compiler chooses not to inline the function (which is a
// valid choice for the compiler to make).
//
//*****************************************************************************
extern uint_fast8_t I2CMReadModifyWrite16BE(tI2CMReadModifyWrite16 *psInst,
                                            tI2CMInstance *psI2CInst,
                                            uint_fast8_t ui8Addr,
                                            uint_fast8_t ui8Reg,
                                            uint_fast16_t ui16Mask,
                                            uint_fast16_t ui16Value,
                                            tSensorCallback *pfnCallback,
                                            void *pvCallbackData);

//*****************************************************************************
//
// This function handles the idle state of the I2C master state machine.
//
//*****************************************************************************
static void
I2CMStateIdle(tI2CMInstance *psInst, tI2CMCommand *pCommand)
{
    //
    // Do nothing if there is not another transfer in the queue.
    //
    if(psInst->ui8ReadPtr == psInst->ui8WritePtr)
    {
        return;
    }

    //
    // See if there is any data to be written.
    //
    if(pCommand->ui16WriteCount != 0)
    {
        //
        // Set the slave address and indicate a write.
        //
        MAP_I2CMasterSlaveAddrSet(psInst->ui32Base, pCommand->ui8Addr, false);

        //
        // Place the first data byte to be written in the data register.
        //
        MAP_I2CMasterDataPut(psInst->ui32Base, pCommand->pui8WriteData[0]);

        //
        // See if there is just a single byte to be written and no bytes to be
        // read.
        //
        if((pCommand->ui16WriteCount == 1) && (pCommand->ui16ReadCount == 0))
        {
            //
            // Perform a single byte send.
            //
            MAP_I2CMasterControl(psInst->ui32Base, I2C_MASTER_CMD_SINGLE_SEND);

            //
            // The next state is the callback state.
            //
            psInst->ui8State = STATE_CALLBACK;
        }

        //
        // Otherwise, see if there is just a single byte to be written and at
        // least one byte to be read.
        //
        else if(pCommand->ui16WriteCount == 1)
        {
            //
            // Perform a single send, writing the first byte as the only byte.
            //
            MAP_I2CMasterControl(psInst->ui32Base,
                                 I2C_MASTER_CMD_BURST_SEND_START);

            //
            // Set the next state of the interrupt state machine based on the
            // number of bytes to read.
            //
            psInst->ui8State = ((pCommand->ui16ReadCount == 1) ?
                                STATE_READ_ONE : STATE_READ_FIRST);
        }

        //
        // Otherwise, there is more than one byte to be written.
        //
        else
        {
            //
            // Start the burst cycle, writing the first byte.
            //
            MAP_I2CMasterControl(psInst->ui32Base,
                                 I2C_MASTER_CMD_BURST_SEND_START);

            //
            // Set the index to indicate that the first byte has been
            // transmitted.
            //
            psInst->ui16Index = 1;

            //
            // Set the next state of the interrupt state machine based on the
            // number of bytes to write.
            //
            psInst->ui8State = ((pCommand->ui16WriteCount != 2) ?
                                STATE_WRITE_NEXT : STATE_WRITE_FINAL);
        }
    }
    else
    {
        //
        // Set the slave address and indicate a read.
        //
        MAP_I2CMasterSlaveAddrSet(psInst->ui32Base, pCommand->ui8Addr, true);

        //
        // Set the index to indicate that the first byte is being read.
        //
        psInst->ui16Index = 0;

        //
        // See if there is just a single byte to be read.
        //
        if(pCommand->ui16ReadCount == 1)
        {
            //
            // Perform a single byte read.
            //
            MAP_I2CMasterControl(psInst->ui32Base,
                                 I2C_MASTER_CMD_SINGLE_RECEIVE);

            //
            // The next state is the wait for final read state.
            //
            psInst->ui8State = STATE_READ_WAIT;
        }
        else
        {
            //
            // Start the burst receive.
            //
            MAP_I2CMasterControl(psInst->ui32Base,
                                 I2C_MASTER_CMD_BURST_RECEIVE_START);

            //
            // Set the next state appropriately.  If the read count is two, the
            // next state must finish the transaction.  If it is greater than
            // two, the burst read must be continued.
            //
            psInst->ui8State = ((pCommand->ui16ReadCount == 2) ?
                                STATE_READ_FINAL : STATE_READ_NEXT);
        }
    }
}

//*****************************************************************************
//
// This function handles the write next state of the I2C master state machine.
//
//*****************************************************************************
static void
I2CMStateWriteNext(tI2CMInstance *psInst, tI2CMCommand *pCommand)
{
    //
    // See if the write batch has been sent.
    //
    if(psInst->ui16Index == pCommand->ui16WriteBatchSize)
    {
        //
        // Move to the write pause state.
        //
        psInst->ui8State = STATE_WRITE_PAUSE;

        //
        // Call the callback function.
        //
        if(pCommand->pfnCallback)
        {
            pCommand->pfnCallback(pCommand->pvCallbackData,
                                  I2CM_STATUS_BATCH_DONE);
        }
    }
    else
    {
        //
        // Write the next byte to the data register.
        //
        MAP_I2CMasterDataPut(psInst->ui32Base,
                             pCommand->pui8WriteData[psInst->ui16Index]);
        psInst->ui16Index++;

        //
        // Continue the burst write.
        //
        MAP_I2CMasterControl(psInst->ui32Base, I2C_MASTER_CMD_BURST_SEND_CONT);

        //
        // If there is one byte left, set the next state to the final write
        // state.
        //
        if((pCommand->ui16WriteCount - psInst->ui16Index) == 1)
        {
            psInst->ui8State = STATE_WRITE_FINAL;
        }
    }
}

//*****************************************************************************
//
// This function handles the write final state of the I2C master state machine.
//
//*****************************************************************************
static void
I2CMStateWriteFinal(tI2CMInstance *psInst, tI2CMCommand *pCommand)
{
    //
    // See if the write batch has been sent.
    //
    if(psInst->ui16Index == pCommand->ui16WriteBatchSize)
    {
        //
        // Move to the write pause state.
        //
        psInst->ui8State = STATE_WRITE_PAUSE;

        //
        // Call the callback function.
        //
        if(pCommand->pfnCallback)
        {
            pCommand->pfnCallback(pCommand->pvCallbackData,
                                  I2CM_STATUS_BATCH_DONE);
        }
    }
    else
    {
        //
        // Write the final byte to the data register.
        //
        MAP_I2CMasterDataPut(psInst->ui32Base,
                             pCommand->pui8WriteData[psInst->ui16Index]);

        //
        // See if there is data to be read after this byte is written.
        //
        if(pCommand->ui16ReadCount == 0)
        {
            //
            // Finish the burst write.
            //
            MAP_I2CMasterControl(psInst->ui32Base,
                                 I2C_MASTER_CMD_BURST_SEND_FINISH);

            //
            // The next state is the callback state.
            //
            psInst->ui8State = STATE_CALLBACK;
        }
        else
        {
            //
            // Finish the burst write.
            //
            MAP_I2CMasterControl(psInst->ui32Base,
                                 I2C_MASTER_CMD_BURST_SEND_CONT);

            //
            // Set the next state of the interrupt state machine based on the
            // number of bytes to read.
            //
            psInst->ui8State = ((pCommand->ui16ReadCount == 1) ?
                                STATE_READ_ONE : STATE_READ_FIRST);
        }
    }
}

//*****************************************************************************
//
// This function handles the write pause state of the I2C master state machine.
//
//*****************************************************************************
static void
I2CMStateWritePause(tI2CMInstance *psInst, tI2CMCommand *pCommand)
{
    //
    // Decrement the write count by the batch size.
    //
    pCommand->ui16WriteCount -= pCommand->ui16WriteBatchSize;

    //
    // Write the next byte to the data register.
    //
    MAP_I2CMasterDataPut(psInst->ui32Base, pCommand->pui8WriteData[0]);

    //
    // Set the index to indicate that the first byte has been transmitted.
    //
    psInst->ui16Index = 1;

    //
    // See if there is more than one byte left to be written.
    //
    if((pCommand->ui16WriteCount - psInst->ui16Index) == 0)
    {
        //
        // See if there is data to be read after this byte is written.
        //
        if(pCommand->ui16ReadCount == 0)
        {
            //
            // Finish the burst write.
            //
            MAP_I2CMasterControl(psInst->ui32Base,
                                 I2C_MASTER_CMD_BURST_SEND_FINISH);

            //
            // The next state is the callback state.
            //
            psInst->ui8State = STATE_CALLBACK;
        }
        else
        {
            //
            // Finish the burst write.
            //
            MAP_I2CMasterControl(psInst->ui32Base,
                                 I2C_MASTER_CMD_BURST_SEND_CONT);

            //
            // Set the next state of the interrupt state machine based on the
            // number of bytes to read.
            //
            psInst->ui8State = ((pCommand->ui16ReadCount == 1) ?
                                STATE_READ_ONE : STATE_READ_FIRST);
        }
    }
    else
    {
        //
        // Continue the burst write.
        //
        MAP_I2CMasterControl(psInst->ui32Base, I2C_MASTER_CMD_BURST_SEND_CONT);

        //
        // The next state is the write next state.
        //
        if((pCommand->ui16WriteCount - psInst->ui16Index) == 1)
        {
            psInst->ui8State = STATE_WRITE_FINAL;
        }
        else
        {
            psInst->ui8State = STATE_WRITE_NEXT;
        }
    }
}

//*****************************************************************************
//
// This function handles the read one state of the I2C master state machine.
//
//*****************************************************************************
static void
I2CMStateReadOne(tI2CMInstance *psInst, tI2CMCommand *pCommand)
{
    //
    // Put the I2C master into receive mode.
    //
    MAP_I2CMasterSlaveAddrSet(psInst->ui32Base, pCommand->ui8Addr, true);

    //
    // Perform a single byte read.
    //
    MAP_I2CMasterControl(psInst->ui32Base, I2C_MASTER_CMD_SINGLE_RECEIVE);

    //
    // Set the index to indicate that the first byte is being read.
    //
    psInst->ui16Index = 0;

    //
    // The next state is the wait for final read state.
    //
    psInst->ui8State = STATE_READ_WAIT;
}

//*****************************************************************************
//
// This function handles the read first state of the I2C master state machine.
//
//*****************************************************************************
static void
I2CMStateReadFirst(tI2CMInstance *psInst, tI2CMCommand *pCommand)
{
    //
    // Put the I2C master into receive mode.
    //
    MAP_I2CMasterSlaveAddrSet(psInst->ui32Base, pCommand->ui8Addr, true);

    //
    // Start the burst receive.
    //
    MAP_I2CMasterControl(psInst->ui32Base, I2C_MASTER_CMD_BURST_RECEIVE_START);

    //
    // Set the index to indicate that the first byte is being read.
    //
    psInst->ui16Index = 0;

    //
    // Set the next state appropriately.  If the count is greater than two it
    // is the middle of the burst read.  If exactly two, the next state must
    // finish the transaction.
    //
    psInst->ui8State = ((pCommand->ui16ReadCount == 2) ?
                        STATE_READ_FINAL : STATE_READ_NEXT);
}

//*****************************************************************************
//
// This function handles the read next state of the I2C master state machine.
//
//*****************************************************************************
static void
I2CMStateReadNext(tI2CMInstance *psInst, tI2CMCommand *pCommand)
{
    //
    // Read the received character.
    //
    pCommand->pui8ReadData[psInst->ui16Index] =
        MAP_I2CMasterDataGet(psInst->ui32Base);
    psInst->ui16Index++;

    //
    // See if the read batch has been filled.
    //
    if(psInst->ui16Index == pCommand->ui16ReadBatchSize)
    {
        //
        // Move to the read pause state.
        //
        psInst->ui8State = STATE_READ_PAUSE;

        //
        // Call the callback function.
        //
        if(pCommand->pfnCallback)
        {
            pCommand->pfnCallback(pCommand->pvCallbackData,
                                  I2CM_STATUS_BATCH_READY);
        }
    }
    else
    {
        //
        // Continue the burst read.
        //
        MAP_I2CMasterControl(psInst->ui32Base,
                             I2C_MASTER_CMD_BURST_RECEIVE_CONT);

        //
        // If there are two characters left to be read, make the next state be
        // the end of burst read state.
        //
        if((pCommand->ui16ReadCount - psInst->ui16Index) == 2)
        {
            psInst->ui8State = STATE_READ_FINAL;
        }
    }
}

//*****************************************************************************
//
// This function handles the read final state of the I2C master state machine.
//
//*****************************************************************************
static void
I2CMStateReadFinal(tI2CMInstance *psInst, tI2CMCommand *pCommand)
{
    //
    // Read the received character.
    //
    pCommand->pui8ReadData[psInst->ui16Index] =
        MAP_I2CMasterDataGet(psInst->ui32Base);
    psInst->ui16Index++;

    //
    // See if the read batch has been filled.
    //
    if(psInst->ui16Index == pCommand->ui16ReadBatchSize)
    {
        //
        // Move to the read pause state.
        //
        psInst->ui8State = STATE_READ_PAUSE;

        //
        // Call the callback function.
        //
        if(pCommand->pfnCallback)
        {
            pCommand->pfnCallback(pCommand->pvCallbackData,
                                  I2CM_STATUS_BATCH_READY);
        }
    }
    else
    {
        //
        // Finish the burst read.
        //
        MAP_I2CMasterControl(psInst->ui32Base,
                             I2C_MASTER_CMD_BURST_RECEIVE_FINISH);

        //
        // The next state is the wait for final read state.
        //
        psInst->ui8State = STATE_READ_WAIT;
    }
}

//*****************************************************************************
//
// This function handles the read pause state of the I2C master state machine.
//
//*****************************************************************************
static void
I2CMStateReadPause(tI2CMInstance *psInst, tI2CMCommand *pCommand)
{
    //
    // Decrement the read count by the batch size.
    //
    pCommand->ui16ReadCount -= pCommand->ui16ReadBatchSize;

    //
    // Reset the read index.
    //
    psInst->ui16Index = 0;

    //
    // See if there is more than one byte left to be read.
    //
    if((pCommand->ui16ReadCount - psInst->ui16Index) == 1)
    {
        //
        // Finish the burst read.
        //
        MAP_I2CMasterControl(psInst->ui32Base,
                             I2C_MASTER_CMD_BURST_RECEIVE_FINISH);

        //
        // The next state is the wait for final read state.
        //
        psInst->ui8State = STATE_READ_WAIT;
    }
    else
    {
        //
        // Continue the burst read.
        //
        MAP_I2CMasterControl(psInst->ui32Base,
                             I2C_MASTER_CMD_BURST_RECEIVE_CONT);

        //
        // Determine the next state based on the number of bytes left to read.
        //
        if((pCommand->ui16ReadCount - psInst->ui16Index) == 2)
        {
            psInst->ui8State = STATE_READ_FINAL;
        }
        else
        {
            psInst->ui8State = STATE_READ_NEXT;
        }
    }
}

//*****************************************************************************
//
// This function handles the read wait state of the I2C master state machine.
//
//*****************************************************************************
static void
I2CMStateReadWait(tI2CMInstance *psInst, tI2CMCommand *pCommand)
{
    //
    // Read the received character.
    //
    pCommand->pui8ReadData[psInst->ui16Index] =
        MAP_I2CMasterDataGet(psInst->ui32Base);

    //
    // The state machine is now in the callback state.
    //
    psInst->ui8State = STATE_CALLBACK;
}

//*****************************************************************************
//
// This function handles the callback state of the I2C master state machine.
//
//*****************************************************************************
static void
I2CMStateCallback(tI2CMInstance *psInst, tI2CMCommand *pCommand,
                  uint32_t ui32Status)
{
    tSensorCallback *pfnCallback;
    void *pvCallbackData;

    //
    // Save the callback information.
    //
    pfnCallback = pCommand->pfnCallback;
    pvCallbackData = pCommand->pvCallbackData;

    //
    // This command has been completed, so increment the read pointer.
    //
    psInst->ui8ReadPtr++;
    if(psInst->ui8ReadPtr == NUM_I2CM_COMMANDS)
    {
        psInst->ui8ReadPtr = 0;
    }

    //
    // If there is a callback function then call it now.
    //
    if(pfnCallback)
    {
        //
        // Convert the status from the I2C driver into the I2C master
        // driver status.
        //
        if((ui32Status & (I2C_MCS_ARBLST | I2C_MCS_ERROR)) == 0)
        {
            ui32Status = I2CM_STATUS_SUCCESS;
        }
        else if(ui32Status & I2C_MCS_ARBLST)
        {
            ui32Status = I2CM_STATUS_ARB_LOST;
        }
        else if(ui32Status & I2C_MCS_ADRACK)
        {
            ui32Status = I2CM_STATUS_ADDR_NACK;
        }
        else if(ui32Status & I2C_MCS_DATACK)
        {
            ui32Status = I2CM_STATUS_DATA_NACK;
        }
        else
        {
            ui32Status = I2CM_STATUS_ERROR;
        }

        //
        // Call the callback function.
        //
        pfnCallback(pvCallbackData, ui32Status);
    }

    //
    // The state machine is now idle.
    //
    psInst->ui8State = STATE_IDLE;
}

//*****************************************************************************
//
//! Handles I2C master interrupts.
//!
//! \param psInst is a pointer to the I2C master instance data.
//!
//! This function performs the processing required in response to an I2C
//! interrupt.  The application-supplied interrupt handler should call this
//! function with the correct instance data in response to the I2C interrupt.
//!
//! \return None.
//
//*****************************************************************************
void
I2CMIntHandler(tI2CMInstance *psInst)
{
    tI2CMCommand *pCommand;
    uint32_t ui32Status;

    //
    // Clear the I2C interrupt.
    //
    MAP_I2CMasterIntClear(psInst->ui32Base);
    ui32Status = HWREG(psInst->ui32Base + I2C_O_MCS);

    //
    // Get a pointer to the current command.
    //
    pCommand = &(psInst->pCommands[psInst->ui8ReadPtr]);

    //
    // See if an error occurred during the last transaction.
    //
    if((ui32Status & (I2C_MCS_ERROR | I2C_MCS_ARBLST)) &&
       (psInst->ui8State != STATE_IDLE))
    {
        //
        // An error occurred, so halt the I2C transaction.  The error stop
        // command for send and receive is identical, so it does not matter
        // which one is used here.  Only issue the stop if the bus is busy.
        //
        if(ui32Status & I2C_MCS_BUSBSY)
        {
            MAP_I2CMasterControl(psInst->ui32Base,
                                 I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);
        }

        //
        // Move to the callback state.
        //
        psInst->ui8State = STATE_CALLBACK;
    }

    //
    // Loop forever.  Most states will return when they have completed their
    // action.  However, a few states require multi-state processing, so those
    // states will break and this loop repeated.
    //
    while(1)
    {
        //
        // Determine what to do based on the current state.
        //
        switch(psInst->ui8State)
        {
            //
            // The idle state.
            //
            case STATE_IDLE:
            {
                //
                // Handle the idle state.
                //
                I2CMStateIdle(psInst, pCommand);

                //
                // This state is done and the next state should be handled at
                // the next interrupt.
                //
                return;
            }

            //
            // The state for the middle of a burst write.
            //
            case STATE_WRITE_NEXT:
            {
                //
                // Handle the write next state.
                //
                I2CMStateWriteNext(psInst, pCommand);

                //
                // This state is done and the next state should be handled at
                // the next interrupt.
                //
                return;
            }

            //
            // The state for the final write of a burst sequence.
            //
            case STATE_WRITE_FINAL:
            {
                //
                // Handle the write final state.
                //
                I2CMStateWriteFinal(psInst, pCommand);

                //
                // This state is done and the next state should be handled at
                // the next interrupt.
                //
                return;
            }

            //
            // The state for a paused write.
            //
            case STATE_WRITE_PAUSE:
            {
                //
                // Handle the write pause state.
                //
                I2CMStateWritePause(psInst, pCommand);

                //
                // This state is done and the next state should be handled at
                // the next interrupt.
                //
                return;
            }

            //
            // The state for a single byte read.
            //
            case STATE_READ_ONE:
            {
                //
                // Handle the read one state.
                //
                I2CMStateReadOne(psInst, pCommand);

                //
                // This state is done and the next state should be handled at
                // the next interrupt.
                //
                return;
            }

            //
            // The state for the start of a burst read.
            //
            case STATE_READ_FIRST:
            {
                //
                // Handle the read first state.
                //
                I2CMStateReadFirst(psInst, pCommand);

                //
                // This state is done and the next state should be handled at
                // the next interrupt.
                //
                return;
            }

            //
            // The state for the middle of a burst read.
            //
            case STATE_READ_NEXT:
            {
                //
                // Handle the read next state.
                //
                I2CMStateReadNext(psInst, pCommand);

                //
                // This state is done and the next state should be handled at
                // the next interrupt.
                //
                return;
            }

            //
            // The state for the end of a burst read.
            //
            case STATE_READ_FINAL:
            {
                //
                // Handle the read final state.
                //
                I2CMStateReadFinal(psInst, pCommand);

                //
                // This state is done and the next state should be handled at
                // the next interrupt.
                //
                return;
            }

            //
            // The state for a paused read.
            //
            case STATE_READ_PAUSE:
            {
                //
                // Handle the read pause state.
                //
                I2CMStateReadPause(psInst, pCommand);

                //
                // This state is done and the next state should be handled at
                // the next interrupt.
                //
                return;
            }

            //
            // This state is for the final read of a single or burst read.
            //
            case STATE_READ_WAIT:
            {
                //
                // Handle the read wait state.
                //
                I2CMStateReadWait(psInst, pCommand);

                //
                // This state is done and the next state needs to be handled
                // immediately.
                //
                break;
            }

            //
            // This state is for providing the transaction complete callback.
            //
            case STATE_CALLBACK:
            {
                //
                // Handle the callback state.
                //
                I2CMStateCallback(psInst, pCommand, ui32Status);

                //
                // If an error occurred, this state is done.  The completion of
                // the error handling stop condition above, if issued, will
                // cause the next state to be processed.
                //
                if((ui32Status & (I2C_MCS_ERROR | I2C_MCS_ARBLST)) &&
                   (ui32Status & I2C_MCS_BUSBSY))
                {
                    return;
                }

                //
                // Update the pointer to the current command.
                //
                pCommand = &(psInst->pCommands[psInst->ui8ReadPtr]);

                //
                // This state is done and the next state needs to be handled
                // immediately.
                //
                break;
            }
        }
    }
}

//*****************************************************************************
//
//! Initializes the I2C master driver.
//!
//! \param psInst is a pointer to the I2C master instance data.
//! \param ui32Base is the base address of the I2C module.
//! \param ui8Int is the interrupt number for the I2C module.
//! \param ui8TxDMA is the uDMA channel number used for transmitting data to
//! the I2C module.
//! \param ui8RxDMA is the uDMA channel number used for receiving data from
//! the I2C module.
//! \param ui32Clock is the clock frequency of the input clock to the I2C
//! module.
//!
//! This function prepares both the I2C master module and driver for operation,
//! and must be the first I2C master driver function called for each I2C master
//! instance.  It is assumed that the application has enabled the I2C module,
//! configured the I2C pins, and provided an I2C interrupt handler that calls
//! I2CMIntHandler().
//!
//! The uDMA module cannot be used at present to transmit/receive data, so the
//! \e ui8TxDMA and \e ui8RxDMA parameters are unused.  They are reserved for
//! future use and should be set to 0xff in order to ensure future
//! compatibility.
//!
//! \return None.
//
//*****************************************************************************
void
I2CMInit(tI2CMInstance *psInst, uint32_t ui32Base, uint_fast8_t ui8Int,
         uint_fast8_t ui8TxDMA, uint_fast8_t ui8RxDMA, uint32_t ui32Clock)
{
    //
    // Check the arguments.
    //
    ASSERT(psInst);
    ASSERT((ui32Base == I2C0_BASE) || (ui32Base == I2C1_BASE) ||
           (ui32Base == I2C2_BASE) || (ui32Base == I2C3_BASE) ||
           (ui32Base == I2C4_BASE) || (ui32Base == I2C5_BASE) ||
           (ui32Base == I2C6_BASE) || (ui32Base == I2C7_BASE) ||
           (ui32Base == I2C8_BASE) || (ui32Base == I2C9_BASE));
    ASSERT(ui8Int);
    ASSERT(ui32Clock);

    //
    // Initialize the state structure.
    //
    psInst->ui32Base = ui32Base;
    psInst->ui8Int = ui8Int;
    psInst->ui8TxDMA = ui8TxDMA;
    psInst->ui8RxDMA = ui8RxDMA;
    psInst->ui8State = STATE_IDLE;
    psInst->ui8ReadPtr = 0;
    psInst->ui8WritePtr = 0;

    //
    // Initialize the I2C master module.
    //
    MAP_I2CMasterInitExpClk(ui32Base, ui32Clock, true);

    //
    // Enable the I2C interrupt.
    //
    MAP_IntEnable(ui8Int);
    MAP_I2CMasterIntEnableEx(ui32Base, I2C_MASTER_INT_DATA);
}

//*****************************************************************************
//
//! Sends a command to an I2C device.
//!
//! \param psInst is a pointer to the I2C master instance data.
//! \param ui8Addr is the address of the I2C device to access.
//! \param pui8WriteData is a pointer to the data buffer to be written.
//! \param ui16WriteCount is the number of bytes to be written.
//! \param ui16WriteBatchSize is the number of bytes in each write batch.
//! \param pui8ReadData is a pointer to the buffer to be filled with the read
//! data.
//! \param ui16ReadCount is the number of bytes to be read.
//! \param ui16ReadBatchSize is the number of bytes to be read in each batch.
//! \param pfnCallback is the function to be called when the transfer has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function adds an I2C command to the queue of commands to be sent.  If
//! successful, the I2C command is then transferred in the background using the
//! interrupt handler.  When the transfer is complete, the callback function,
//! if provided, is called in the context of the I2C master interrupt handler.
//!
//! The first byte of \e pui8WriteData contains the I2C address of the device
//! to access, the next \e ui16WriteCount bytes contains the data to be written
//! to the device.  The data read from the device is written into the first
//! \e ui16ReadCount bytes of \e pui8ReadData.  The \e ui16WriteCount or
//! \e ui16ReadCount parameters can be zero if there are no bytes to be read or
//! written.  The write bytes are sent to the device first, and then the read
//! bytes are read from the device afterward.
//!
//! If \e ui16WriteBatchSize is less than \e ui16WriteCount, the write portion
//! of the transfer is broken up into as many \e ui16WriteBatchSize batches as
//! required to write \e ui16WriteCount bytes.  After each batch, the callback
//! function is called with an \b I2CM_STATUS_BATCH_DONE status, and the
//! transfer is paused (with the I2C bus held).  The transfer is resumed when
//! I2CMTransferResume() is called.  This procedure can be used to perform very
//! large writes without requiring all the data be available at once, at the
//! expense of tying up the I2C bus for the extended duration of the transfer.
//!
//! If \e ui16ReadBatchSize is less than \e ui16ReadCount, the read portion of
//! the transfer is broken up into as many \e ui16ReadBatchSize batches as
//! required to read \e ui16ReadCount bytes.  After each batch, the callback
//! function is called with an \b I2CM_STATUS_BATCH_READY status, and the
//! transfer is paused (with the I2C bus held).  The transfer is resumed when
//! I2CMTransferResume() is called.  This procedure can be used to perform very
//! large reads without requiring a large SRAM buffer, at the expense of tying
//! up the I2C bus for the extended duration of the transfer.
//!
//! \return Returns 1 if the command was successfully added to the queue and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
I2CMCommand(tI2CMInstance *psInst, uint_fast8_t ui8Addr,
            const uint8_t *pui8WriteData, uint_fast16_t ui16WriteCount,
            uint_fast16_t ui16WriteBatchSize, uint8_t *pui8ReadData,
            uint_fast16_t ui16ReadCount, uint_fast16_t ui16ReadBatchSize,
            tSensorCallback *pfnCallback, void *pvCallbackData)
{
    uint_fast8_t ui8Next, ui8Enabled;
    tI2CMCommand *pCommand;

    //
    // Check the arguments.
    //
    ASSERT(psInst);
    ASSERT(pui8WriteData || !ui16WriteCount);
    ASSERT(!ui16WriteCount || (ui16WriteBatchSize > 0));
    ASSERT(pui8ReadData || !ui16ReadCount);
    ASSERT(!ui16ReadCount || (ui16ReadBatchSize > 0));

    //
    // Disable the I2C interrupt.
    //
    if(MAP_IntIsEnabled(psInst->ui8Int))
    {
        ui8Enabled = 1;
        MAP_IntDisable(psInst->ui8Int);
    }
    else
    {
        ui8Enabled = 0;
    }

    //
    // Compute the new value of the write pointer (after this command is added
    // to the queue).
    //
    ui8Next = psInst->ui8WritePtr + 1;
    if(ui8Next == NUM_I2CM_COMMANDS)
    {
        ui8Next = 0;
    }

    //
    // Return a failure if the command queue is full.
    //
    if(psInst->ui8ReadPtr == ui8Next)
    {
        if(ui8Enabled)
        {
            MAP_IntEnable(psInst->ui8Int);
        }
        return(0);
    }

    //
    // Get a pointer to the command structure.
    //
    pCommand = &(psInst->pCommands[psInst->ui8WritePtr]);

    //
    // Fill in the command structure with the details of this command.
    //
    pCommand->ui8Addr = ui8Addr;
    pCommand->pui8WriteData = pui8WriteData;
    pCommand->ui16WriteCount = ui16WriteCount;
    pCommand->ui16WriteBatchSize = ui16WriteBatchSize;
    pCommand->pui8ReadData = pui8ReadData;
    pCommand->ui16ReadCount = ui16ReadCount;
    pCommand->ui16ReadBatchSize = ui16ReadBatchSize;
    pCommand->pfnCallback = pfnCallback;
    pCommand->pvCallbackData = pvCallbackData;

    //
    // Update the write pointer.
    //
    psInst->ui8WritePtr = ui8Next;

    //
    // See if the state machine is idle.
    //
    if(psInst->ui8State == STATE_IDLE)
    {
        //
        // Generate a fake I2C interrupt, which will commence the I2C transfer.
        //
        IntTrigger(psInst->ui8Int);
    }

    //
    // Re-enable the I2C master interrupt.
    //
    if(ui8Enabled)
    {
        MAP_IntEnable(psInst->ui8Int);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Resumes an I2C transfer.
//!
//! \param psInst is a pointer to the I2C master instance data.
//! \param pui8Data is a pointer to the buffer to be used for the next batch of
//! data.
//!
//! This function resumes an I2C transfer that has been paused via the use of
//! the write or read batch size capability.
//!
//! \return Returns 1 if the transfer was resumed and 0 if there was not a
//! paused transfer to resume.
//
//*****************************************************************************
uint_fast8_t
I2CMTransferResume(tI2CMInstance *psInst, uint8_t *pui8Data)
{
    //
    // Check the arguments.
    //
    ASSERT(psInst);
    ASSERT(pui8Data);

    //
    // Return an error if there is not a paused transfer.
    //
    if((psInst->ui8State != STATE_WRITE_PAUSE) &&
       (psInst->ui8State != STATE_READ_PAUSE))
    {
        return(0);
    }

    //
    // Save the pointer for the next buffer.
    //
    if(psInst->ui8State == STATE_WRITE_PAUSE)
    {
        psInst->pCommands[psInst->ui8ReadPtr].pui8WriteData = pui8Data;
    }
    else
    {
        psInst->pCommands[psInst->ui8ReadPtr].pui8ReadData = pui8Data;
    }

    //
    // Trigger the I2C interrupt, resuming the transfer.
    //
    IntTrigger(psInst->ui8Int);

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
// The callback function that is called when I2C transactions as part of of a
// read-modify-write operation of 8 bits of data have completed.
//
//*****************************************************************************
static void
I2CMReadModifyWrite8Callback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tI2CMReadModifyWrite8 *psInst;

    //
    // Convert the instance data into a pointer to a tI2CMReadModifyWrite8
    // structure.
    //
    psInst = pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result ina  callback to propagate the
    // error).
    //
    if(ui8Status != I2CM_STATUS_SUCCESS)
    {
        psInst->ui8State = I2CM_RMW_STATE_IDLE;
    }

    //
    // Determine the current state of the I2C master read-modify-write state
    // machine.
    //
    switch(psInst->ui8State)
    {
        //
        // The read portion of the read-modify-write has completed.
        //
        case I2CM_RMW_STATE_READ:
        {
            //
            // Modify the register data that was just read.
            //
            psInst->pui8Buffer[1] = ((psInst->pui8Buffer[1] &
                                      psInst->ui8Mask) | psInst->ui8Value);

            //
            // Write the data back to the device.
            //
            I2CMWrite(psInst->psI2CInst, psInst->ui8Addr, psInst->pui8Buffer,
                      2, I2CMReadModifyWrite8Callback, psInst);

            //
            // Move to the wait for write state.
            //
            psInst->ui8State = I2CM_RMW_STATE_WRITE;

            //
            // Done.
            //
            break;
        }

        //
        // The write portion of the read-modify-write has completed.
        //
        case I2CM_RMW_STATE_WRITE:
        {
            //
            // Move to the idle state.
            //
            psInst->ui8State = I2CM_RMW_STATE_IDLE;

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == I2CM_RMW_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Performs a read-modify-write of 8 bits of data in an I2C device.
//!
//! \param psInst is a pointer to the read-modify-write instance data.
//! \param psI2CInst is a pointer to the I2C master instance data.
//! \param ui8Addr is the address of the I2C device to access.
//! \param ui8Reg is the register in the I2C device to access.
//! \param ui8Mask is the mask indicating the register bits that should be
//! maintained.
//! \param ui8Value is the value indicating the new value for the register bits
//! that are not maintained.
//! \param pfnCallback is the function to be called when the write has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read-modify-write transaction of 8 bits of data
//! in an I2C device.  The modify portion of the operation is performed by
//! AND-ing the register value with \e ui8Mask and then OR-ing the result with
//! \e ui8Value.  When the read-modify-write is complete, the callback
//! function, if provided, is called in the context of the I2C master interrupt
//! handler.
//!
//! If the mask (in \e ui8Mask) is zero, then none of the bits in the current
//! register value are maintained.  In this case, the read portion of the
//! read-modify-write is bypassed, and the new register value (in \e ui8Value)
//! is directly written to the I2C device.
//!
//! \return Returns 1 if the command was successfully added to the queue and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
I2CMReadModifyWrite8(tI2CMReadModifyWrite8 *psInst, tI2CMInstance *psI2CInst,
                     uint_fast8_t ui8Addr, uint_fast8_t ui8Reg,
                     uint_fast8_t ui8Mask, uint_fast8_t ui8Value,
                     tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Check the arguments.
    //
    ASSERT(psInst);
    ASSERT(psI2CInst);

    //
    // Fill in the read-modify-write structure with the details of this
    // request.
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8Addr;
    psInst->ui8Mask = ui8Mask;
    psInst->ui8Value = ui8Value;
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Construct the I2C command to access the requested register.
    //
    psInst->pui8Buffer[0] = ui8Reg;

    //
    // See if this is a write or a read-modify-write.
    //
    if(ui8Mask == 0)
    {
        //
        // Set the state to waiting for the write portion of the
        // read-modify-write.
        //
        psInst->ui8State = I2CM_RMW_STATE_WRITE;

        //
        // Set the new register value in the command buffer.
        //
        psInst->pui8Buffer[1] = ui8Value;

        //
        // Add the write command to the I2C master queue.
        //
        if(I2CMWrite(psI2CInst, ui8Addr, psInst->pui8Buffer, 2,
                     I2CMReadModifyWrite8Callback, psInst) == 0)
        {
            return(0);
        }
    }
    else
    {
        //
        // Set the state to waiting for the read portion of the
        // read-modify-write.
        //
        psInst->ui8State = I2CM_RMW_STATE_READ;

        //
        // Add the read command to the I2C master queue.
        //
        if(I2CMRead(psI2CInst, ui8Addr, psInst->pui8Buffer, 1,
                    psInst->pui8Buffer + 1, 1, I2CMReadModifyWrite8Callback,
                    psInst) == 0)
        {
            return(0);
        }
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
// The callback function that is called when I2C transactions as part of a
// read-modify-write operation of 16 bits of little-endian data have completed.
//
//*****************************************************************************
static void
I2CMReadModifyWrite16LECallback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tI2CMReadModifyWrite16 *psInst;
    uint16_t ui16Value;

    //
    // Convert the instance data into a pointer to a tI2CMReadModifyWrite16
    // structure.
    //
    psInst = pvCallbackData;

    //
    // If the I2C master driver encountered a failure, force the state machine
    // to the idle state (which will also result ina  callback to propagate the
    // error).
    //
    if(ui8Status != I2CM_STATUS_SUCCESS)
    {
        psInst->ui8State = I2CM_RMW_STATE_IDLE;
    }

    //
    // Determine the current state of the I2C master read-modify-write state
    // machine.
    //
    switch(psInst->ui8State)
    {
        //
        // The read portion of the read-modify-write has completed.
        //
        case I2CM_RMW_STATE_READ:
        {
            //
            // Modify the register data that was just read.
            //
            ui16Value = (psInst->pui8Buffer[2] << 8) | psInst->pui8Buffer[1];
            ui16Value = (ui16Value & psInst->ui16Mask) | psInst->ui16Value;
            psInst->pui8Buffer[1] = ui16Value & 0xff;
            psInst->pui8Buffer[2] = (ui16Value >> 8) & 0xff;

            //
            // Write the data back to the device.
            //
            I2CMWrite(psInst->psI2CInst, psInst->ui8Addr, psInst->pui8Buffer,
                      3, I2CMReadModifyWrite16LECallback, psInst);

            //
            // Move to the wait for write state.
            //
            psInst->ui8State = I2CM_RMW_STATE_WRITE;

            //
            // Done.
            //
            break;
        }

        //
        // The write portion of the read-modify-write has completed.
        //
        case I2CM_RMW_STATE_WRITE:
        {
            //
            // Move to the idle state.
            //
            psInst->ui8State = I2CM_RMW_STATE_IDLE;

            //
            // Done.
            //
            break;
        }
    }

    //
    // See if the state machine is now idle and there is a callback function.
    //
    if((psInst->ui8State == I2CM_RMW_STATE_IDLE) && psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Performs a read-modify-write of 16 bits of little-endian data in an I2C
//! device.
//!
//! \param psInst is a pointer to the read-modify-write instance data.
//! \param psI2CInst is a pointer to the I2C master instance data.
//! \param ui8Addr is the address of the I2C device to access.
//! \param ui8Reg is the register in the I2C device to access.
//! \param ui16Mask is the mask indicating the register bits that should be
//! maintained.
//! \param ui16Value is the value indicating the new value for the register
//! bits that are not maintained.
//! \param pfnCallback is the function to be called when the write has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read-modify-write transaction of 16 bits of
//! little-endian data in an I2C device.  The modify portion of the operation
//! is performed by AND-ing the register value with \e ui16Mask and then OR-ing
//! the result with \e ui16Value.  When the read-modify-write is complete, the
//! callback function, if provided, is called in the context of the I2C master
//! interrupt handler.
//!
//! If the mask (in \e ui16Mask) is zero, then none of the bits in the current
//! register value are maintained.  In this case, the read portion of the
//! read-modify-write is bypassed, and the new register value (in \e ui16Value)
//! is directly written to the I2C device.
//!
//! \return Returns 1 if the command was successfully added to the queue and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
I2CMReadModifyWrite16LE(tI2CMReadModifyWrite16 *psInst,
                        tI2CMInstance *psI2CInst, uint_fast8_t ui8Addr,
                        uint_fast8_t ui8Reg, uint_fast16_t ui16Mask,
                        uint_fast16_t ui16Value, tSensorCallback *pfnCallback,
                        void *pvCallbackData)
{
    //
    // Check the arguments.
    //
    ASSERT(psInst);
    ASSERT(psI2CInst);

    //
    // Fill in the read-modify-write structure with the details of this
    // request.
    //
    psInst->psI2CInst = psI2CInst;
    psInst->ui8Addr = ui8Addr;
    psInst->ui16Mask = ui16Mask;
    psInst->ui16Value = ui16Value;
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Construct the I2C command to access the requested register.
    //
    psInst->pui8Buffer[0] = ui8Reg;

    //
    // See if this is a write or a read-modify-write.
    //
    if(ui16Mask == 0)
    {
        //
        // Set the state to waiting for the write portion of the
        // read-modify-write.
        //
        psInst->ui8State = I2CM_RMW_STATE_WRITE;

        //
        // Set the new register value in the command buffer.
        //
        psInst->pui8Buffer[1] = ui16Value & 0xff;
        psInst->pui8Buffer[2] = (ui16Value >> 8) & 0xff;

        //
        // Add the write command to the I2C master queue.
        //
        if(I2CMWrite(psI2CInst, ui8Addr, psInst->pui8Buffer, 3,
                     I2CMReadModifyWrite16LECallback, psInst) == 0)
        {
            return(0);
        }
    }
    else
    {
        //
        // Set the state to waiting for the read portion of the
        // read-modify-write.
        //
        psInst->ui8State = I2CM_RMW_STATE_READ;

        //
        // Add the read command to the I2C master queue.
        //
        if(I2CMRead(psI2CInst, ui8Addr, psInst->pui8Buffer, 1,
                    psInst->pui8Buffer + 1, 2, I2CMReadModifyWrite16LECallback,
                    psInst) == 0)
        {
            return(0);
        }
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
// The callback function that is called when I2C transactions as part of a
// write operation of 8-bit data have completed.
//
//*****************************************************************************
static void
I2CMWrite8Callback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tI2CMWrite8 *psInst;

    //
    // Convert the instance data into a pointer to a tI2CMWrite8 structure.
    //
    psInst = pvCallbackData;

    //
    // See if the current batch is done and more data is needed.
    //
    if(ui8Status == I2CM_STATUS_BATCH_DONE)
    {
        //
        // Place the next two bytes into the write buffer.
        //
        psInst->pui8Buffer[0] = psInst->pui8Data[0];
        if(psInst->ui16Count > 1)
        {
            psInst->pui8Buffer[1] = psInst->pui8Data[1];
        }

        //
        // Advance past the next two bytes of the input buffer.
        //
        psInst->pui8Data += 2;
        psInst->ui16Count -= 2;

        //
        // Resume the batched write.
        //
        I2CMTransferResume(psInst->psI2CInst, psInst->pui8Buffer);
    }

    //
    // The transfer has completed, or an error has occurred.  In both cases,
    // see if there is a callback function.
    //
    else if(psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Performs a write of 8-bit data to an I2C device.
//!
//! \param psInst is a pointer to the 8-bit write instance data.
//! \param psI2CInst is a pointer to the I2C master instance data.
//! \param ui8Addr is the address of the I2C device to access.
//! \param ui8Reg is the register in the I2C device to access.
//! \param pui8Data is a pointer to the register data to be written.
//! \param ui16Count is the number of register values to be written.
//! \param pfnCallback is the function to be called when the write has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a write transaction of 8-bit data to an I2C device.
//!
//! \return Returns 1 if the command was successfully added to the queue and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
I2CMWrite8(tI2CMWrite8 *psInst, tI2CMInstance *psI2CInst, uint_fast8_t ui8Addr,
           uint_fast8_t ui8Reg, const uint8_t *pui8Data,
           uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
           void *pvCallbackData)
{
    //
    // Check the arguments.
    //
    ASSERT(psInst);
    ASSERT(psI2CInst);

    //
    // Fill in the write structure with the details of this request.
    //
    psInst->psI2CInst = psI2CInst;
    psInst->pui8Data = pui8Data + 1;
    psInst->ui16Count = ui16Count - 1;
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Initiate the I2C write to this device.
    //
    psInst->pui8Buffer[0] = ui8Reg;
    psInst->pui8Buffer[1] = pui8Data[0];
    if(I2CMWriteBatched(psI2CInst, ui8Addr, psInst->pui8Buffer, ui16Count + 1,
                        2, I2CMWrite8Callback, psInst) == 0)
    {
        //
        // The I2C write failed, so return a failure.
        //
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
// The callback function that is called when I2C transactions as part of a read
// operation of 16-bit big-endian data have completed.
//
//*****************************************************************************
static void
I2CMRead16BECallback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tI2CMRead16BE *psInst;
    uint8_t ui8Temp;

    //
    // Convert the instance data into a pointer to a tI2CMRead16BE structure.
    //
    psInst = pvCallbackData;

    //
    // See if the transaction completed successfully.
    //
    if(ui8Status == I2CM_STATUS_SUCCESS)
    {
        //
        // Loop through the 16-bit values read from the I2C device.
        //
        while(psInst->ui16Count--)
        {
            //
            // Byte swap this value.
            //
            ui8Temp = psInst->pui8Data[0];
            psInst->pui8Data[0] = psInst->pui8Data[1];
            psInst->pui8Data[1] = ui8Temp;

            //
            // Skip to the next value.
            //
            psInst->pui8Data += 2;
        }
    }

    //
    // See if there is a callback function.
    //
    if(psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Performs a read of 16-bit big-endian data from an I2C device.
//!
//! \param psInst is a pointer to the 16-bit big-endian read instance data.
//! \param psI2CInst is a pointer to the I2C master instance data.
//! \param ui8Addr is the address of the I2C device to access.
//! \param ui8Reg is the register in the I2C device to access.
//! \param pui16Data is a pointer to the buffer to be filled with the register
//! data.
//! \param ui16Count is the number of 16-bit register values to be read.
//! \param pfnCallback is the function to be called when the read has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a read transaction of 16-bit big-endian data from
//! an I2C device.  The data is provided by the device in big-endian format and
//! is byte-swapped as it is read from the I2C device, returning the data in
//! little-endian format.
//!
//! \return Returns 1 if the command was successfully added to the queue and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
I2CMRead16BE(tI2CMRead16BE *psInst, tI2CMInstance *psI2CInst,
             uint_fast8_t ui8Addr, uint_fast8_t ui8Reg,
             uint16_t *pui16Data, uint_fast16_t ui16Count,
             tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Check the arguments.
    //
    ASSERT(psInst);
    ASSERT(psI2CInst);

    //
    // Fill in the read structure with the details of this request.
    //
    psInst->psI2CInst = psI2CInst;
    psInst->pui8Data = (uint8_t *)pui16Data;
    psInst->ui16Count = ui16Count;
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Initiate the I2C write to this device.
    //
    psInst->pui8Data[0] = ui8Reg;
    if(I2CMRead(psI2CInst, ui8Addr, psInst->pui8Data, 1, psInst->pui8Data,
                ui16Count * 2, I2CMRead16BECallback, psInst) == 0)
    {
        //
        // The I2C write failed, so return a failure.
        //
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
// The callback function that is called when I2C transactions as part of a
// write operation of 16-bit big-endian data have completed.
//
//*****************************************************************************
static void
I2CMWrite16BECallback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    tI2CMWrite16BE *psInst;

    //
    // Convert the instance data into a pointer to a tI2CMWrite16BE structure.
    //
    psInst = pvCallbackData;

    //
    // See if the current batch is done and more data is needed.
    //
    if(ui8Status == I2CM_STATUS_BATCH_DONE)
    {
        //
        // Place the next two bytes into the write buffer.
        //
        psInst->pui8Buffer[0] = psInst->pui8Data[0];
        if(psInst->ui16Count > 1)
        {
            psInst->pui8Buffer[1] = psInst->pui8Data[3];
        }

        //
        // Advance past the next two bytes of the input buffer.
        //
        psInst->pui8Data += 2;
        psInst->ui16Count--;

        //
        // Resume the batched write.
        //
        I2CMTransferResume(psInst->psI2CInst, psInst->pui8Buffer);
    }

    //
    // The transfer has completed, or an error has occurred.  In both cases,
    // see if there is a callback function.
    //
    else if(psInst->pfnCallback)
    {
        //
        // Call the application-supplied callback function.
        //
        psInst->pfnCallback(psInst->pvCallbackData, ui8Status);
    }
}

//*****************************************************************************
//
//! Performs a write of 16-bit big-endian data to an I2C device.
//!
//! \param psInst is a pointer to the 16-bit big-endian write instance data.
//! \param psI2CInst is a pointer to the I2C master instance data.
//! \param ui8Addr is the address of the I2C device to access.
//! \param ui8Reg is the register in the I2C device to access.
//! \param pui16Data is a pointer to the register data to be written.
//! \param ui16Count is the number of 16-bit register values to be written.
//! \param pfnCallback is the function to be called when the write has
//! completed (can be \b NULL if a callback is not required).
//! \param pvCallbackData is a pointer that is passed to the callback function.
//!
//! This function initiates a write transaction of 16-bit big-endian data to an
//! I2C device.  The data in the buffer is provided in little-endian format and
//! is byte-swapped as it is being written to the I2C device.
//!
//! \return Returns 1 if the command was successfully added to the queue and 0
//! if it was not.
//
//*****************************************************************************
uint_fast8_t
I2CMWrite16BE(tI2CMWrite16BE *psInst, tI2CMInstance *psI2CInst,
              uint_fast8_t ui8Addr, uint_fast8_t ui8Reg,
              const uint16_t *pui16Data, uint_fast16_t ui16Count,
              tSensorCallback *pfnCallback, void *pvCallbackData)
{
    //
    // Check the arguments.
    //
    ASSERT(psInst);
    ASSERT(psI2CInst);

    //
    // Fill in the write structure with the details of this request.
    //
    psInst->psI2CInst = psI2CInst;
    psInst->pui8Data = (const uint8_t *)pui16Data;
    psInst->ui16Count = ui16Count;
    psInst->pfnCallback = pfnCallback;
    psInst->pvCallbackData = pvCallbackData;

    //
    // Initiate the I2C write to this device.
    //
    psInst->pui8Buffer[0] = ui8Reg;
    psInst->pui8Buffer[1] = psInst->pui8Data[1];
    if(I2CMWriteBatched(psI2CInst, ui8Addr, psInst->pui8Buffer,
                        (ui16Count * 2) + 1, 2, I2CMWrite16BECallback,
                        psInst) == 0)
    {
        //
        // The I2C write failed, so return a failure.
        //
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
