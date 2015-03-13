//*****************************************************************************
//
// isl29023.h - Prototypes for the ISL29023 light sensor driver.
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

#ifndef __SENSORLIB_ISL29023_H__
#define __SENSORLIB_ISL29023_H__

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
// The structure that defines the internal state of the ISL29023 driver.
//
//*****************************************************************************
typedef struct
{
    //
    // The pointer to the I2C master interface instance used to communicate
    // with the ISL29023.
    //
    tI2CMInstance *psI2CInst;

    //
    // The I2C address of the ISL29023.
    //
    uint8_t ui8Addr;

    //
    // The state of the state machine used while accessing the ISL29023.
    //
    uint8_t ui8State;

    //
    // The data buffer used for sending/receiving data to/from the ISL29023.
    //
    uint8_t pui8Data[4];

    //
    // Instance copy of the range setting.  Used in GetFloat functions
    //
    uint8_t ui8Range;

    //
    // The new range, which is used when a register write succeeds.
    //
    uint8_t ui8NewRange;

    //
    // Instance copy of the resolution setting.  Used in GetFloat function.
    //
    uint8_t ui8Resolution;

    //
    // The new resolution, which is used when a register write succeeds.
    //
    uint8_t ui8NewResolution;

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
        uint8_t pui8Buffer[3];

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
tISL29023;

//*****************************************************************************
//
// Function prototypes.
//
//*****************************************************************************
extern uint_fast8_t ISL29023Init(tISL29023 *psInst, tI2CMInstance *psI2CInst,
                                 uint_fast8_t ui8I2CAddr,
                                 tSensorCallback *pfnCallback,
                                 void *pvCallbackData);
extern uint_fast8_t ISL29023Read(tISL29023 *psInst, uint_fast8_t ui8Reg,
                                 uint8_t *pui8Data, uint_fast16_t ui16Count,
                                 tSensorCallback *pfnCallback,
                                 void *pvCallbackData);
extern uint_fast8_t ISL29023Write(tISL29023 *psInst, uint_fast8_t ui8Reg,
                                 uint8_t *pui8Data, uint_fast16_t ui16Count,
                                 tSensorCallback *pfnCallback,
                                 void *pvCallbackData);
extern uint_fast8_t ISL29023ReadModifyWrite(tISL29023 *psInst,
                                            uint_fast8_t ui8Reg,
                                            uint8_t ui8Mask, uint8_t ui8Value,
                                            tSensorCallback *pfnCallback,
                                            void *pvCallbackData);
extern uint_fast8_t ISL29023DataRead(tISL29023 *psInst,
                                     tSensorCallback *pfnCallback,
                                     void *pvCallbackData);
extern void ISL29023DataLightVisibleGetRaw(tISL29023 *psInst,
                                           uint16_t *pui16Visible);
extern void ISL29023DataLightVisibleGetFloat(tISL29023 *psInst,
                                            float *pfVisible);
extern void ISL29023DataLightIRGetRaw(tISL29023 *psInst, uint16_t *pui16IR);
extern void ISL29023DataLightIRGetFloat(tISL29023 *psInst, float *pfIR);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __SENSORLIB_ISL29023_H__

