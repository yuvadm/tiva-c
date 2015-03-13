//*****************************************************************************
//
// comp_dcm.h - Prototypes for the complementary filter direction cosine matrix
//              functions.
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

#ifndef __SENSORLIB_COMP_DCM_H__
#define __SENSORLIB_COMP_DCM_H__

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
// The structure that defines the internal state of the complementary filter
// DCM algorithm.
//
//*****************************************************************************
typedef struct
{
    //
    // The state of the direction cosine matrix.
    //
    float ppfDCM[3][3];

    //
    // The time delta between updates to the DCM.
    //
    float fDeltaT;

    //
    // The scaling factor for the DCM update based on the accelerometer
    // reading.
    //
    float fScaleA;

    //
    // The scaling factor for the DCM update based on the gyroscope reading.
    //
    float fScaleG;

    //
    // The scaling factor for the DCM update based on the magnetometer reading.
    //
    float fScaleM;

    //
    // The most recent accelerometer readings.
    //
    float pfAccel[3];

    //
    // The most recent gyroscope readings.
    //
    float pfGyro[3];

    //
    // The most recent magnetometer readings.
    //
    float pfMagneto[3];
}
tCompDCM;

//*****************************************************************************
//
// Prototypes.
//
//*****************************************************************************
extern void CompDCMInit(tCompDCM *psDCM, float fDeltaT, float fScaleA,
                        float fScaleG, float fScaleM);
extern void CompDCMAccelUpdate(tCompDCM *psDCM, float fAccelX, float fAccelY,
                               float fAccelZ);
extern void CompDCMGyroUpdate(tCompDCM *psDCM, float fGyroX, float fGyroY,
                              float fGyroZ);
extern void CompDCMMagnetoUpdate(tCompDCM *psDCM, float fMagnetoX,
                                 float fMagnetoY, float fMagnetoZ);
extern void CompDCMStart(tCompDCM *psDCM);
extern void CompDCMUpdate(tCompDCM *psDCM);
extern void CompDCMMatrixGet(tCompDCM *psDCM, float ppfDCM[3][3]);
extern void CompDCMComputeEulers(tCompDCM *psDCM, float *pfRoll,
                                 float *pfPitch, float *pfYaw);
extern void CompDCMComputeQuaternion(tCompDCM *psDCM, float pfQuaternion[4]);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __SENSORLIB_COMP_DCM_H__
