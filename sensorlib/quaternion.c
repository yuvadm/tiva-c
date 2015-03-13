//*****************************************************************************
//
// quaternion.c - Functions for performing quaternion operations.
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

#include <math.h>
#include "sensorlib/quaternion.h"

//*****************************************************************************
//
//! \addtogroup quaternion_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// If M_PI has not been defined by the system headers, define it here.
//
//*****************************************************************************
#ifndef M_PI
#define M_PI                    3.14159265358979323846
#endif

//*****************************************************************************
//
//! Computes a quaternion from a set of eueler angles specified in degrees
//!
//! \param pfQOut is the inverted quaternion in W,X,Y,Z form
//! \param fRollDeg is roll in degrees
//! \param fPitchDeg is pitch in degrees
//! \param fYawDeg is yaw in degrees
//!
//! This function computes a quaternion from a set of euler angles specified
//! in degrees
//!
//! \return Returns a quaternion representing the provided eulers
//
//*****************************************************************************
void
QuaternionFromEuler(float pfQOut[4], float fRollDeg, float fPitchDeg,
                    float fYawDeg)
{
    float fRoll, fPitch, fYaw;
    float fCOSY, fCOSP, fCOSR;
    float fSINY, fSINP, fSINR;

    //
    // Convert roll, pitch, and yaw from degrees into radians
    //
    fRoll = fRollDeg * M_PI / 180.0;
    fPitch = fPitchDeg *  M_PI / 180.0;
    fYaw = fYawDeg * M_PI / 180.0;

    //
    // Pre-calculate the cosine of (yaw, pitch, roll divided by 2)
    //
    fCOSY = cosf(fYaw / 2.0);
    fCOSP = cosf(fPitch / 2.0);
    fCOSR = cosf(fRoll / 2.0);

    //
    // Pre-calculate the sine of (yaw, pitch, roll divided by 2)
    //
    fSINY = sinf(fYaw / 2.0);
    fSINP = sinf(fPitch / 2.0);
    fSINR = sinf(fRoll / 2.0);

    //
    // The W component
    //
    pfQOut[Q_W] = fCOSY * fCOSP * fCOSR - fSINY * fSINP * fSINR;

    //
    // The X component
    //
    pfQOut[Q_X] = fSINY * fSINP * fCOSR + fCOSY * fCOSP * fSINR;

    //
    // The Y component
    //
    pfQOut[Q_Y] = fCOSY * fSINP * fCOSR - fSINY * fCOSP * fSINR;

    //
    // The Z component
    //
    pfQOut[Q_Z] = fSINY * fCOSP * fCOSR + fCOSY * fSINP * fSINR;
}

//*****************************************************************************
//
//! Computes the magnitude of a quaternion.
//!
//! \param pfQIn is the source quaternion in W,X,Y,Z form
//!
//! This function computes the magnitude of a quaternion by summing the square
//! of each of the quatnerion components.
//!
//! \return Returns the scalar magnitude of the quaternion
//
//*****************************************************************************
float
QuaternionMagnitude(float pfQIn[4])
{
    float fSumSq;

    //
    // Calculate the magnitude of the quaternion by finding the sum of the
    // squares of each component.
    //
    fSumSq = ((pfQIn[Q_W] * pfQIn[Q_W]) + (pfQIn[Q_X] * pfQIn[Q_X]) +
              (pfQIn[Q_Y] * pfQIn[Q_Y]) + (pfQIn[Q_Z] * pfQIn[Q_Z]));

    return(fSumSq);
}

//*****************************************************************************
//
//! Computes the inverse of a quaternion.
//!
//! \param pfQOut is the inverted quaternion in W,X,Y,Z form
//! \param pfQIn is the source quaternion in W,X,Y,Z form
//!
//! This function computes the inverse of a quaternion.  The inverse of a
//! quaternion produces a rotation opposite to the source quaternion.  This
//! can be achieved by simply changing the signs of the imaginary components
//! of a quaternion when the quatnerion is a unit quaternion.
//!
//! \return Returns the inverse of a quaternion.
//
//*****************************************************************************
void
QuaternionInverse(float pfQOut[4], float pfQIn[4])
{
    float fMag;

    //
    // Find magnitude of the quaternion.  This will be used to normalize the
    // source quaternion if it's not already.  If it is a unit quaternion then
    // the magnitude should be nearly equal to 1.0 and dividing by the
    // magnitude has no mathemtical effect.
    //
    fMag = QuaternionMagnitude(pfQIn);

    //
    // Normalize the W component
    //
    pfQOut[Q_W] = pfQIn[Q_W] / fMag;

    //
    // Invert and normalize the X component
    //
    pfQOut[Q_X] = -pfQIn[Q_X] / fMag;

    //
    // Invert and normalize the Y component
    //
    pfQOut[Q_Y] = -pfQIn[Q_Y] / fMag;

    //
    // Invert and normalize the Z component
    //
    pfQOut[Q_Z] = -pfQIn[Q_Z] / fMag;
}

//*****************************************************************************
//
//! Computes the product of two quaternions.
//!
//! \param pfQOut is the product of In1 X In2
//! \param pfQIn1 is the source quaternion in W,X,Y,Z form
//! \param pfQIn2 is the source quaternion in W,X,Y,Z form
//!
//! This function computes the cross product of two quaternions.
//!
//! \return Returns the cross product of the two quaternions.
//
//*****************************************************************************
void
QuaternionMult(float pfQOut[4], float pfQIn1[4], float pfQIn2[4])
{
    //
    // Let Q1 and Q2 be two quaternions with components w,x,y,z
    // Let Qp be the cross product Q1 x Q2.  The components of Qp can be
    // calculated as follows:
    //
    // Qp.w = (Q1w Q2w) - (Q1x Q2x) - (Q1y Q2y) - (Q1z Q2z)
    // Qp.x = (Q1w Q2x) + (Q1x Q2w) - (Q1z Q2y) + (Q1y Q2z)
    // Qp.y = (Q1y Q2w) + (Q1z Q2x) + (Q1w Q2y) - (Q1x Q2z)
    // Qp.z = (Q1z Q2w) - (Q1y Q2x) + (Q1x Q2y) + (Q1w Q2z)
    //

    //
    // Calculate the W term
    //
    pfQOut[Q_W] = ((pfQIn2[Q_W] * pfQIn1[Q_W]) - (pfQIn2[Q_X] * pfQIn1[Q_X]) -
                   (pfQIn2[Q_Y] * pfQIn1[Q_Y]) - (pfQIn2[Q_Z] * pfQIn1[Q_Z]));

    //
    // Calculate the X term
    //
    pfQOut[Q_X]= ((pfQIn2[Q_X] * pfQIn1[Q_W]) + (pfQIn2[Q_W] * pfQIn1[Q_X]) -
                  (pfQIn2[Q_Y] * pfQIn1[Q_Z]) + (pfQIn2[Q_Z] * pfQIn1[Q_Y]));

    //
    // Calculate the Y term
    //
    pfQOut[Q_Y]= ((pfQIn2[Q_W] * pfQIn1[Q_Y]) + (pfQIn2[Q_X] * pfQIn1[Q_Z]) -
                  (pfQIn2[Q_Y] * pfQIn1[Q_W]) - (pfQIn2[Q_Z] * pfQIn1[Q_X]));

    //
    // Calculate the Z term
    //
    pfQOut[Q_Z] = ((pfQIn2[Q_W] * pfQIn1[Q_Z]) - (pfQIn2[Q_X] * pfQIn1[Q_Y]) -
                   (pfQIn2[Q_Y] * pfQIn1[Q_X]) + (pfQIn2[Q_Z] * pfQIn1[Q_W]));
}

//*****************************************************************************
//
//! Computes the angle between two quaternions
//!
//! \param pfQIn1 is a source quaternion in W,X,Y,Z form
//! \param pfQIn2 is a source quaternion in W,X,Y,Z form
//!
//! This function computes the angle between two quaternions.
//!
//! \return Returns the angle, in radians, between the two quaternions.
//
//*****************************************************************************
float
QuaternionAngle(float pfQIn1[4], float pfQIn2[4])
{
    float pfQInv[4];
    float pfQProd[4];

    //
    // Let Q1 and Q2 be two quaternions having components w,x,y,z.  The angle
    // between the orientations represented by Q1 and Q2 can be calculated
    // with:
    //
    // angle = arccos( (Q2 * Q1').w ) * 2.0;
    //
    // where Q1' is the inverse of Q1
    //

    //
    // Calculate the inverse of Q1
    //
    QuaternionInverse(pfQInv, pfQIn1);

    //
    // Find the product of Q2 x Q1`
    //
    QuaternionMult(pfQProd, pfQIn2, pfQInv);

    //
    // calculate the arccos of the w component of the previous product.
    //
    return(acosf(pfQProd[Q_W]) * 2.0);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
