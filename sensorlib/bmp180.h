//*****************************************************************************
//
// bmp180.h - Prototypes for the BMP180 pressure sensor driver.
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

#ifndef __SENSORLIB_BMP180_H__
#define __SENSORLIB_BMP180_H__

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
// The structure that defines the internal state of the BMP180 driver.
//
//*****************************************************************************
typedef struct
{
    //
    // The pointer to the I2C master interface instance used to communicate
    // with the BMP180.
    //
    tI2CMInstance *psI2CInst;

    //
    // The I2C address of the BMP180.
    //
    uint8_t ui8Addr;

    //
    // The state of the state machine used while accessing the BMP180.
    //
    uint8_t ui8State;

    //
    // The sampling mode to be used by the BMP180.
    //
    uint8_t ui8Mode;

    //
    // The new sampling mode, which is used when a register write succeeds.
    //
    uint8_t ui8NewMode;

    //
    // The AC1 calibration from the BMP180.
    //
    int16_t i16AC1;

    //
    // The AC2 calibration from the BMP180.
    //
    int16_t i16AC2;

    //
    // The AC3 calibration from the BMP180.
    //
    int16_t i16AC3;

    //
    // The AC4 calibration from the BMP180.
    //
    uint16_t ui16AC4;

    //
    // The AC5 calibration from the BMP180.
    //
    uint16_t ui16AC5;

    //
    // The AC6 calibration from the BMP180.
    //
    uint16_t ui16AC6;

    //
    // The B1 calibration from the BMP180.
    //
    int16_t i16B1;

    //
    // The B2 calibration from the BMP180.
    //
    int16_t i16B2;

    //
    // The MC calibration from the BMP180.
    //
    int16_t i16MC;

    //
    // The MD calibration from the BMP180.
    //
    int16_t i16MD;

    //
    // The data buffer used for sending/receiving data to/from the BMP180.
    //
    uint8_t pui8Data[5];

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
        // A buffer used to store the write portion of a register read.  This
        // is also used to read back the calibration data from the device.
        //
        uint8_t pui8Buffer[22];

        //
        // The write state used to write register values.
        //
        tI2CMWrite8 sWriteState;

        //
        // The read-modify-write state used to modify register values.
        //
        tI2CMReadModifyWrite8 sReadModifyWriteState;
    }
    uCommand;
}
tBMP180;

//*****************************************************************************
//
// Function prototypes.
//
//*****************************************************************************
extern uint_fast8_t BMP180Init(tBMP180 *psInst, tI2CMInstance *psI2CInst,
                               uint_fast8_t ui8I2CAddr,
                               tSensorCallback *pfnCallback,
                               void *pvCallbackData);
extern uint_fast8_t BMP180Read(tBMP180 *psInst, uint_fast8_t ui8Reg,
                               uint8_t *pui8Data, uint_fast16_t ui16Count,
                               tSensorCallback *pfnCallback,
                               void *pvCallbackData);
extern uint_fast8_t BMP180Write(tBMP180 *psInst, uint_fast8_t ui8Reg,
                                uint8_t *pui8Data, uint_fast16_t ui16Count,
                                tSensorCallback *pfnCallback,
                                void *pvCallbackData);
extern uint_fast8_t BMP180ReadModifyWrite(tBMP180 *psInst, uint_fast8_t ui8Reg,
                                          uint_fast8_t ui8Mask,
                                          uint_fast8_t ui8Value,
                                          tSensorCallback *pfnCallback,
                                          void *pvCallbackData);
extern uint_fast8_t BMP180DataRead(tBMP180 *psInst,
                                   tSensorCallback *pfnCallback,
                                   void *pvCallbackData);
extern void BMP180DataPressureGetRaw(tBMP180 *psInst,
                                     uint_fast32_t *pui32Pressure);
extern void BMP180DataPressureGetFloat(tBMP180 *psInst, float *pfPressure);
extern void BMP180DataTemperatureGetRaw(tBMP180 *psInst,
                                        uint_fast16_t *pui16Temperature);
extern void BMP180DataTemperatureGetFloat(tBMP180 *psInst,
                                          float *pfTemperature);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __SENSORLIB_BMP180_H__
