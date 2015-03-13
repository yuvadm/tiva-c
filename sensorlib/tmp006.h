//*****************************************************************************
//
// tmp006.h - Prototypes for the Texas Instruments TMP006 temperature sensor
//            driver.
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

#ifndef __SENSORLIB_TMP006_H__
#define __SENSORLIB_TMP006_H__

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
// The structure that defines the internal state of the TMP006 driver.
//
//*****************************************************************************
typedef struct
{
    //
    // The pointer to the I2C master interface instance used to communicate
    // with the TMP006.
    //
    tI2CMInstance *psI2CInst;

    //
    // The I2C address of the TMP006.
    //
    uint8_t ui8Addr;

    //
    // The state of the state machine used while accessing the TMP006.
    //
    uint8_t ui8State;

    //
    // The data buffer used for sending/receiving data to/from the TMP006.
    //
    uint8_t pui8Data[4];

    //
    // Calibration factor.  Left to application to implement calibration
    // See term S0 in http://www.ti.com/lit/ug/sbou107/sbou107.pdf
    //
    float fCalibrationFactor;

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
        uint8_t pui8Buffer[2];

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
tTMP006;

//*****************************************************************************
//
// Function prototypes.
//
//*****************************************************************************
extern uint_fast8_t TMP006Init(tTMP006 *psInst, tI2CMInstance *psI2CInst,
                               uint_fast8_t ui8I2CAddr,
                               tSensorCallback *pfnCallback,
                               void *pvCallbackData);
extern uint_fast8_t TMP006Read(tTMP006 *psInst, uint_fast8_t ui8Reg,
                               uint16_t *pui16Data, uint_fast16_t ui16Count,
                               tSensorCallback *pfnCallback,
                               void *pvCallbackData);
extern uint_fast8_t TMP006Write(tTMP006 *psInst, uint_fast8_t ui8Reg,
                                const uint16_t *pui16Data,
                                uint_fast16_t ui16Count,
                                tSensorCallback *pfnCallback,
                                void *pvCallbackData);
extern uint_fast8_t TMP006ReadModifyWrite(tTMP006 *psInst, uint_fast8_t ui8Reg,
                                          uint_fast16_t ui16Mask,
                                          uint_fast16_t ui16Value,
                                          tSensorCallback *pfnCallback,
                                          void *pvCallbackData);
extern uint_fast8_t TMP006DataRead(tTMP006 *psInst,
                                   tSensorCallback *pfnCallback,
                                   void *pvCallbackData);
extern void TMP006DataTemperatureGetRaw(tTMP006 *psInst, int16_t *pui16Ambient,
                                        int16_t *pui16Object);
extern void TMP006DataTemperatureGetFloat(tTMP006 *psInst, float *pfAmbient,
                                          float *pfObject);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __SENSORLIB_TMP006_H__

