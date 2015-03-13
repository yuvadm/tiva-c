//*****************************************************************************
//
// magneto.h - Prototypes for the functions that manipulate magnetometer
//             readings.
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

#ifndef __SENSORLIB_MAGNETO_H__
#define __SENSORLIB_MAGNETO_H__

//*****************************************************************************
//
// The structure that defines the internal state of the magnetometer hard and
// soft iron compensation.
//
//*****************************************************************************
typedef struct
{
    //
    // The hard iron induced offset in the X axis of the magnetometer.
    //
    float fXOffset;

    //
    // The hard iron induced offset in the Y axis of the magnetometer.
    //
    float fYOffset;

    //
    // The hard iron induced offset in the Z axis of the magnetometer.
    //
    float fZOffset;

    //
    // The Z axis rotation required to align the major/minor axes of the
    // ellipse in the X-Y plane with the X-Y axes, specified in radians.
    //
    float fXYAngle;

    //
    // The amount to scale the Y axis in order to turn the X-Y ellipse into a
    // circle.
    //
    float fYRatio;

    //
    // The Y axis rotation required to align the major/minor axes of the
    // ellipse in the X-Z plane with X-Z axes, specified in radians.
    //
    float fXZAngle;

    //
    // The amount to scale the Z axis in order to turn the X-Z ellipse into a
    // circle.
    //
    float fZRatio;
}
tMagnetoCompensation;

//*****************************************************************************
//
// Prototypes.
//
//*****************************************************************************
extern void MagnetoCompensateInit(tMagnetoCompensation *psInst, float fXOffset,
                                  float fYOffset, float fZOffset,
                                  float fXYAngle, float fYRatio,
                                  float fXZAngle, float fZRatio);
extern void MagnetoCompensate(tMagnetoCompensation *psInst, float *pfMagnetoX,
                              float *pfMagnetoY, float *pfMagnetoZ);
extern float MagnetoHeadingCompute(float fMagnetoX, float fMagnetoY,
                                   float fMagnetoZ, float fRoll, float fPitch);

#endif // __SENSORLIB_MAGNETO_H__
