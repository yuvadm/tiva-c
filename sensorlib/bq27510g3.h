//*****************************************************************************
//
// bq27510g3.c - Prototypes for the TI BQ27510G3 Battery Fuel Guage 
//               driver.
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

#ifndef __SENSORLIB_BQ27510G3_H__
#define __SENSORLIB_BQ27510G3_H__

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
// The structure that defines the internal state of the BQ27510G3 driver.
//
//*****************************************************************************
typedef struct
{
    //
    // The pointer to the I2C master interface instance used to communicate
    // with the BQ27510G3.
    //
    tI2CMInstance *psI2CInst;

    //
    // The I2C address of the BQ27510G3.
    //
    uint8_t ui8Addr;

    //
    // The state of the state machine used while accessing the BQ27510G3.
    //
    uint8_t ui8State;

    //
    // The data buffer used for sending/receiving data to/from the BQ27510G3.
    //
    uint8_t pui8Data[32];

    //
    // The function that is called when the current request has completed
    // processing.
    //
    tSensorCallback *pfnCallback;

    //
    // The pointer provided to the callback function.
    //
    void *pvCallbackData;

    //
    // A union of structures that are used for read, write and
    // read-modify-write operations.  Since only one operation can be active at
    // a time, it is safe to re-use the memory in this manner.
    //
    union
    {
        //
        // A buffer used to store the write portion of a register read.
        //
        uint8_t pui8Buffer[4];

        //
        // The read state used to read register values.
        //
        tI2CMRead16BE sReadState;

        //
        // The write state used to write register values.
        //
        tI2CMWrite16BE sWriteState;

        //
        // The read-modify-write state used to modify register values.
        //
        tI2CMReadModifyWrite16 sReadModifyWriteState;
    }
    uCommand;
}
tBQ27510G3;

//*****************************************************************************
//
// Function prototypes.
//
//*****************************************************************************
extern uint_fast8_t BQ27510G3Init(tBQ27510G3 *psInst, tI2CMInstance *psI2CInst,
                                  uint_fast8_t ui8I2CAddr,
                                  tSensorCallback *pfnCallback,
                                  void *pvCallbackData);
extern uint_fast8_t BQ27510G3Read(tBQ27510G3 *psInst, uint_fast8_t ui8Reg,
                                  uint16_t *pui16Data, uint_fast16_t ui16Count,
                                  tSensorCallback *pfnCallback,
                               void *pvCallbackData);
extern uint_fast8_t BQ27510G3Write(tBQ27510G3 *psInst, uint_fast8_t ui8Reg,
                                   const uint16_t *pui16Data,
                                   uint_fast16_t ui16Count,
                                   tSensorCallback *pfnCallback,
                                   void *pvCallbackData);
extern uint_fast8_t BQ27510G3ReadModifyWrite(tBQ27510G3 *psInst,
                                             uint_fast8_t ui8Reg,
                                             uint_fast16_t ui16Mask,
                                             uint_fast16_t ui16Value,
                                             tSensorCallback *pfnCallback,
                                             void *pvCallbackData);
extern uint_fast8_t BQ27510G3DataRead(tBQ27510G3 *psInst,
                                      tSensorCallback *pfnCallback,
                                      void *pvCallbackData);

extern void BQ27510G3DataAtRateTimeToEmptyGetRaw(tBQ27510G3 *psInst,
                                                 int16_t *pui16Data);
extern void BQ27510G3DataAtRateTimeToEmptyGetFloat(tBQ27510G3 *psInst,
                                                   float *pfData);
extern void BQ27510G3DataTemperatureBatteryGetRaw(tBQ27510G3 *psInst,
                                                  int16_t *pui16Data);
extern void BQ27510G3DataTemperatureBatteryGetFloat(tBQ27510G3 *psInst,
                                                    float *pfData);
extern void BQ27510G3DataVoltageBatteryGetRaw(tBQ27510G3 *psInst,
                                              int16_t *pui16Data);
extern void BQ27510G3DataVoltageBatteryGetFloat(tBQ27510G3 *psInst,
                                                float *pfData);
extern void BQ27510G3DataCapacityNominalAvailableGetRaw(tBQ27510G3 *psInst,
                                                        int16_t *pui16Data);
extern void BQ27510G3DataCapacityNominalAvailalbeGetFloat(tBQ27510G3 *psInst,
                                                          float *pfData);
extern void BQ27510G3DataCapacityFullAvailableGetRaw(tBQ27510G3 *psInst,
                                                     int16_t *pui16Data);
extern void BQ27510G3DataCapacityFullAvailableGetFloat(tBQ27510G3 *psInst,
                                                       float *pfData);
extern void BQ27510G3DataCapacityRemainingGetRaw(tBQ27510G3 *psInst,
                                                 int16_t *pui16Data);
extern void BQ27510G3DataCapacityRemainingGetFloat(tBQ27510G3 *psInst,
                                                   float *pfData);
extern void BQ27510G3DataCapacityFullChargeGetRaw(tBQ27510G3 *psInst,
                                                  int16_t *pui16Data);
extern void BQ27510G3DataCapacityFullChargeGetFloat(tBQ27510G3 *psInst,
                                                    float *pfData);
extern void BQ27510G3DataCurrentAverageGetRaw(tBQ27510G3 *psInst,
                                              int16_t *pui16Data);
extern void BQ27510G3DataCurrentAverageGetFloat(tBQ27510G3 *psInst,
                                                float *pfData);
extern void BQ27510G3DataTimeToEmptyGetRaw(tBQ27510G3 *psInst,
                                           int16_t *pui16Data);
extern void BQ27510G3DataTimeToEmptyGetFloat(tBQ27510G3 *psInst,
                                             float *pfData);
extern void BQ27510G3DataCurrentStandbyGetRaw(tBQ27510G3 *psInst,
                                              int16_t *pui16Data);
extern void BQ27510G3DataCurrentStandbyGetFloat(tBQ27510G3 *psInst,
                                                float *pfData);
extern void BQ27510G3DataTimeToEmptyStandbyGetRaw(tBQ27510G3 *psInst,
                                                  int16_t *pui16Data);
extern void BQ27510G3DataTimeToEmptyStandbyGetFloat(tBQ27510G3 *psInst,
                                                    float *pfData);
extern void BQ27510G3DataCycleCountGetRaw(tBQ27510G3 *psInst,
                                          int16_t *pui16Data);
extern void BQ27510G3DataCycleCountGetFloat(tBQ27510G3 *psInst, float *pfData);
extern void BQ27510G3DataHealthGetRaw(tBQ27510G3 *psInst, int16_t *pui16Data);
extern void BQ27510G3DataHealthGetFloat(tBQ27510G3 *psInst, float *pfData);
extern void BQ27510G3DataChargeStateGetRaw(tBQ27510G3 *psInst,
                                           int16_t *pui16Data);
extern void BQ27510G3DataChargeStateGetFloat(tBQ27510G3 *psInst,
                                             float *pfData);
extern void BQ27510G3DataCurrentInstantaneousGetRaw(tBQ27510G3 *psInst,
                                                    int16_t *pui16Data);
extern void BQ27510G3DataCurrentInstantaneousGetFloat(tBQ27510G3 *psInst,
                                                      float *pfData);
extern void BQ27510G3DataTemperatureInternalGetRaw(tBQ27510G3 *psInst,
                                                   int16_t *pui16Data);
extern void BQ27510G3DataTemperatureInternalGetFloat(tBQ27510G3 *psInst,
                                                     float *pfData);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __SENSORLIB_BQ27510G3_H__

