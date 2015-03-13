//*****************************************************************************
//
// i2cm_drv.h - Prototypes for the interrupt-driven I2C master driver.
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

#ifndef __SENSORLIB_I2CM_DRV_H__
#define __SENSORLIB_I2CM_DRV_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// A prototype for the callback function used by the I2C master driver.
//
//*****************************************************************************
typedef void (tSensorCallback)(void *pvData, uint_fast8_t ui8Status);

//*****************************************************************************
//
// The possible status values that can be returned by the I2C command callback.
//
//*****************************************************************************
#define I2CM_STATUS_SUCCESS     0
#define I2CM_STATUS_ADDR_NACK   1
#define I2CM_STATUS_DATA_NACK   2
#define I2CM_STATUS_ARB_LOST    3
#define I2CM_STATUS_ERROR       4
#define I2CM_STATUS_BATCH_DONE  5
#define I2CM_STATUS_BATCH_READY 6

//*****************************************************************************
//
// The maximum number of outstanding commands for each I2C master instance.
//
//*****************************************************************************
#define NUM_I2CM_COMMANDS       10

//*****************************************************************************
//
// The structure that defines an I2C master command.
//
//*****************************************************************************
typedef struct
{
    //
    // The I2C address of the device being accessed.
    //
    uint8_t ui8Addr;

    //
    // The data buffer containing the data to be written.
    //
    const uint8_t *pui8WriteData;

    //
    // The total number of bytes to be written by the command.
    //
    uint16_t ui16WriteCount;

    //
    // The number of bytes to be written in each batch.
    //
    uint16_t ui16WriteBatchSize;

    //
    // The data buffer to store data that has been read.
    //
    uint8_t *pui8ReadData;

    //
    // The total number of bytes to be read by the command.
    //
    uint16_t ui16ReadCount;

    //
    // The number of bytes to be read in each chuck.
    //
    uint16_t ui16ReadBatchSize;

    //
    // The function that is called when this command has been transferred.
    //
    tSensorCallback *pfnCallback;

    //
    // The pointer provided to the callback function.
    //
    void *pvCallbackData;
}
tI2CMCommand;

//*****************************************************************************
//
// The structure that contains the state of an I2C master instance.
//
//*****************************************************************************
typedef struct
{
    //
    // The base address of the I2C module.
    //
    uint32_t ui32Base;

    //
    // The interrupt number associated with the I2C module.
    //
    uint8_t ui8Int;

    //
    // The uDMA channel used to write data to the I2C module.
    //
    uint8_t ui8TxDMA;

    //
    // The uDMA channel used to read data from the I2C module.
    //
    uint8_t ui8RxDMA;

    //
    // The current state of the I2C master driver.
    //
    uint8_t ui8State;

    //
    // The offset of the next command to be read.  The buffer is empty when
    // this value is equal to the write pointer.
    //
    uint8_t ui8ReadPtr;

    //
    // The offset of the next space in the buffer to write a command.  The
    // buffer is full if this value is one less than the read pointer.
    //
    uint8_t ui8WritePtr;

    //
    // The index into the data buffer of the next byte to be transferred.
    //
    uint16_t ui16Index;

    //
    // An array of commands queued up to be sent via the I2C module.
    //
    tI2CMCommand pCommands[NUM_I2CM_COMMANDS];
}
tI2CMInstance;

//*****************************************************************************
//
// The structure that contains the state of an I2C read-modify-write request of
// 8 bits of data.
//
//*****************************************************************************
typedef struct
{
    //
    // A pointer to the I2C master interface instance used for the
    // read-modify-write request.
    //
    tI2CMInstance *psI2CInst;

    //
    // The buffer used for the I2C transfers.
    //
    uint8_t pui8Buffer[4];

    //
    // The current state of the I2C read-modify-write state machine.
    //
    uint8_t ui8State;

    //
    // The I2C address of the device being accessed.
    //
    uint8_t ui8Addr;

    //
    // The value to AND with the I2C register data.
    //
    uint8_t ui8Mask;

    //
    // The value to OR with the I2C register data.
    //
    uint8_t ui8Value;

    //
    // The function that is called when the read-modify-write has been
    // completed.
    //
    tSensorCallback *pfnCallback;

    //
    // The pointer provided to the callback function.
    //
    void *pvCallbackData;
}
tI2CMReadModifyWrite8;

//*****************************************************************************
//
// The structure that contains the state of an I2C read-modify-write request of
// 16 bits of data.
//
//*****************************************************************************
typedef struct
{
    //
    // A pointer to the I2C master interface instance used for the
    // read-modify-write request.
    //
    tI2CMInstance *psI2CInst;

    //
    // The buffer used for the I2C transfers.
    //
    uint8_t pui8Buffer[4];

    //
    // The current state of the I2C read-modify-write state machine.
    //
    uint8_t ui8State;

    //
    // The I2C address of the device being accessed.
    //
    uint8_t ui8Addr;

    //
    // The value to AND with the I2C register data.
    //
    uint16_t ui16Mask;

    //
    // The value to OR with the I2C register data.
    //
    uint16_t ui16Value;

    //
    // The function that is called when the read-modify-write has been
    // completed.
    //
    tSensorCallback *pfnCallback;

    //
    // The pointer provided to the callback function.
    //
    void *pvCallbackData;
}
tI2CMReadModifyWrite16;

//*****************************************************************************
//
// The structure that contains the state of an I2C write request of 8-bit data.
//
//*****************************************************************************
typedef struct
{
    //
    // A pointer to the I2C master interface instance used for the write
    // request.
    //
    tI2CMInstance *psI2CInst;

    //
    // The buffer used for the I2C transfers.
    //
    uint8_t pui8Buffer[2];

    //
    // The number of values to write to the I2C device.
    //
    uint16_t ui16Count;

    //
    // A pointer to the buffer containing the data to write to the I2C device.
    //
    const uint8_t *pui8Data;

    //
    // The function that is called when the write has been completed.
    //
    tSensorCallback *pfnCallback;

    //
    // The pointer provided to the callback function.
    //
    void *pvCallbackData;
}
tI2CMWrite8;

//*****************************************************************************
//
// The structure that contains the state of an I2C read request of 16-bit data
// from a big-endian device.
//
//*****************************************************************************
typedef struct
{
    //
    // A pointer to the I2C master interface instance used for the read
    // request.
    //
    tI2CMInstance *psI2CInst;

    //
    // A pointer to the buffer containing the data read from the I2C device.
    //
    uint8_t *pui8Data;

    //
    // The number of 16-bit values to read from the I2C device.
    //
    uint16_t ui16Count;

    //
    // The function that is called when the read has been completed.
    //
    tSensorCallback *pfnCallback;

    //
    // The pointer provided to the callback function.
    //
    void *pvCallbackData;
}
tI2CMRead16BE;

//*****************************************************************************
//
// The structure that contains the state of an I2C write request of 16-bit data
// to a big-endian device.
//
//*****************************************************************************
typedef struct
{
    //
    // A pointer to the I2C master interface instance used for the write
    // request.
    //
    tI2CMInstance *psI2CInst;

    //
    // The buffer used for the I2C transfers.
    //
    uint8_t pui8Buffer[2];

    //
    // The number of 16-bit values to write to the I2C device.
    //
    uint16_t ui16Count;

    //
    // A pointer to the buffer containing the data to write to the I2C device.
    //
    const uint8_t *pui8Data;

    //
    // The function that is called when the write has been completed.
    //
    tSensorCallback *pfnCallback;

    //
    // The pointer provided to the callback function.
    //
    void *pvCallbackData;
}
tI2CMWrite16BE;

//*****************************************************************************
//
// Prototypes.
//
//*****************************************************************************
extern void I2CMIntHandler(tI2CMInstance *psInst);
extern void I2CMInit(tI2CMInstance *psInst, uint32_t ui32Base,
                     uint_fast8_t ui8Int, uint_fast8_t ui8TxDMA,
                     uint_fast8_t ui8RxDMA, uint32_t ui32Clock);
extern uint_fast8_t I2CMCommand(tI2CMInstance *psInst, uint_fast8_t ui8Addr,
                                const uint8_t *pui8WriteData,
                                uint_fast16_t ui16WriteCount,
                                uint_fast16_t ui16WriteBatchSize,
                                uint8_t *pui8ReadData,
                                uint_fast16_t ui16ReadCount,
                                uint_fast16_t ui16ReadBatchSize,
                                tSensorCallback *pfnCallback,
                                void *pvCallbackData);
extern uint_fast8_t I2CMTransferResume(tI2CMInstance *psInst,
                                       uint8_t *pui8Data);
extern uint_fast8_t I2CMReadModifyWrite8(tI2CMReadModifyWrite8 *psInst,
                                         tI2CMInstance *psI2CInst,
                                         uint_fast8_t ui8Addr,
                                         uint_fast8_t ui8Reg,
                                         uint_fast8_t ui8Mask,
                                         uint_fast8_t ui8Value,
                                         tSensorCallback *pfnCallback,
                                         void *pvCallbackData);
extern uint_fast8_t I2CMReadModifyWrite16LE(tI2CMReadModifyWrite16 *psInst,
                                            tI2CMInstance *psI2CInst,
                                            uint_fast8_t ui8Addr,
                                            uint_fast8_t ui8Reg,
                                            uint_fast16_t ui16Mask,
                                            uint_fast16_t ui16Value,
                                            tSensorCallback *pfnCallback,
                                            void *pvCallbackData);
extern uint_fast8_t I2CMWrite8(tI2CMWrite8 *psInst, tI2CMInstance *psI2CInst,
                               uint_fast8_t ui8Addr, uint_fast8_t ui8Reg,
                               const uint8_t *pui8Data,
                               uint_fast16_t ui16Count,
                               tSensorCallback *pfnCallback,
                               void *pvCallbackData);
extern uint_fast8_t I2CMRead16BE(tI2CMRead16BE *psInst,
                                 tI2CMInstance *psI2CInst,
                                 uint_fast8_t ui8Addr, uint_fast8_t ui8Reg,
                                 uint16_t *pui16Data, uint_fast16_t ui16Count,
                                 tSensorCallback *pfnCallback,
                                 void *pvCallbackData);
extern uint_fast8_t I2CMWrite16BE(tI2CMWrite16BE *psInst,
                                  tI2CMInstance *psI2CInst,
                                  uint_fast8_t ui8Addr, uint_fast8_t ui8Reg,
                                  const uint16_t *pui16Data,
                                  uint_fast16_t ui16Count,
                                  tSensorCallback *pfnCallback,
                                  void *pvCallbackData);

//*****************************************************************************
//
// A convenience wrapper around I2CMCommand to perform a write.
//
//*****************************************************************************
inline uint_fast8_t
I2CMWrite(tI2CMInstance *psInst, uint_fast8_t ui8Addr, const uint8_t *pui8Data,
          uint_fast16_t ui16Count, tSensorCallback *pfnCallback,
          void *pvCallbackData)
{
    return(I2CMCommand(psInst, ui8Addr, pui8Data, ui16Count, ui16Count, 0, 0,
                       0, pfnCallback, pvCallbackData));
}

//*****************************************************************************
//
// A convenience wrapper around I2CMCommand to perform a read.
//
//*****************************************************************************
inline uint_fast8_t
I2CMRead(tI2CMInstance *psInst, uint_fast8_t ui8Addr,
         const uint8_t *pui8WriteData, uint_fast16_t ui16WriteCount,
         uint8_t *pui8ReadData, uint_fast16_t ui16ReadCount,
         tSensorCallback *pfnCallback, void *pvCallbackData)
{
    return(I2CMCommand(psInst, ui8Addr, pui8WriteData, ui16WriteCount,
                       ui16WriteCount, pui8ReadData, ui16ReadCount,
                       ui16ReadCount, pfnCallback, pvCallbackData));
}

//*****************************************************************************
//
// A convenience wrapper around I2CMCommand to perform a batched write.
//
//*****************************************************************************
inline uint_fast8_t
I2CMWriteBatched(tI2CMInstance *psInst, uint_fast8_t ui8Addr,
                 const uint8_t *pui8Data, uint_fast16_t ui16Count,
                 uint_fast16_t ui16BatchSize, tSensorCallback *pfnCallback,
                 void *pvCallbackData)
{
    return(I2CMCommand(psInst, ui8Addr, pui8Data, ui16Count, ui16BatchSize, 0,
                       0, 0, pfnCallback, pvCallbackData));
}

//*****************************************************************************
//
// A convenience wrapper around I2CMCommand to perform a batched read.
//
//*****************************************************************************
inline uint_fast8_t
I2CMReadBatched(tI2CMInstance *psInst, uint_fast8_t ui8Addr,
                const uint8_t *pui8WriteData, uint_fast16_t ui16WriteCount,
                uint_fast16_t ui16WriteBatchSize, uint8_t *pui8ReadData,
                uint_fast16_t ui16ReadCount, uint_fast16_t ui16ReadBatchSize,
                tSensorCallback *pfnCallback, void *pvCallbackData)
{
    return(I2CMCommand(psInst, ui8Addr, pui8WriteData, ui16WriteCount,
                       ui16WriteBatchSize, pui8ReadData, ui16ReadCount,
                       ui16ReadBatchSize, pfnCallback, pvCallbackData));
}

//*****************************************************************************
//
// A 16-bit big-endian read-modify-write in terms of the little-endian version.
//
//*****************************************************************************
inline uint_fast8_t
I2CMReadModifyWrite16BE(tI2CMReadModifyWrite16 *psInst,
                        tI2CMInstance *psI2CInst, uint_fast8_t ui8Addr,
                        uint_fast8_t ui8Reg, uint_fast16_t ui16Mask,
                        uint_fast16_t ui16Value, tSensorCallback *pfnCallback,
                        void *pvCallbackData)
{
    return(I2CMReadModifyWrite16LE(psInst, psI2CInst, ui8Addr, ui8Reg,
                                   (((ui16Mask & 0xff00) >> 8) |
                                    ((ui16Mask & 0x00ff) << 8)),
                                   (((ui16Value & 0xff00) >> 8) |
                                    ((ui16Value & 0x00ff) << 8)),
                                   pfnCallback, pvCallbackData));
}

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __SENSORLIB_I2CM_DRV_H__
