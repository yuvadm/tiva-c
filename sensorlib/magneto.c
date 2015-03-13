//*****************************************************************************
//
// magneto.c - Functions for manipulating magnetometer readings.
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
#include "sensorlib/magneto.h"

//*****************************************************************************
//
//! \addtogroup magneto_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Initializes the magnetometer hard- and soft-iron compensation state.
//!
//! \param psInst is a pointer to the magnetometer compensation state
//! structure.
//! \param fXOffset is the hard-iron compensation for the X axis.
//! \param fYOffset is the hard-iron compensation for the Y axis.
//! \param fZOffset is the hard-iron compensation for the Z axis.
//! \param fXYAngle is the amount to rotate around the Z axis prior to scaling
//! the Y axis reading, in radians.
//! \param fYRatio is the amount to scale the Y axis reading.
//! \param fXZAngle is the amount to rotate around the Y axis prior to scaling
//! the Z axis reading, in radians.
//! \param fZRatio is the amount to scale the Z axis reading.
//!
//! This function initializes the magnetometer compensation state structure
//! with the values that are used to perform hard- and soft-iron compensation
//! of magnetometer readings.
//!
//! \return None.
//
//*****************************************************************************
void
MagnetoCompensateInit(tMagnetoCompensation *psInst, float fXOffset,
                      float fYOffset, float fZOffset, float fXYAngle,
                      float fYRatio, float fXZAngle, float fZRatio)
{
    //
    // Save the hard- and soft-iron compensation values.
    //
    psInst->fXOffset = fXOffset;
    psInst->fYOffset = fYOffset;
    psInst->fZOffset = fZOffset;
    psInst->fXYAngle = fXYAngle;
    psInst->fYRatio = fYRatio;
    psInst->fXZAngle = fXZAngle;
    psInst->fZRatio = fZRatio;
}

//*****************************************************************************
//
//! Performs hard- and soft-iron compensation on magnetometer readings.
//!
//! \param psInst is a pointer to the magnetometer compensation state
//! structure.
//! \param pfMagnetoX is a pointer to the magnetometer X-axis reading.
//! \param pfMagnetoY is a pointer to the magnetometer Y-axis reading.
//! \param pfMagnetoZ is a pointer to the magnetometer Z-axis reading.
//!
//! This function performs hard- and soft-iron compensation on the given
//! magnetometer reading.  Hard-iron distortions cause a fixed offset in the
//! reading, regardless of orientation.  Hard-iron compensation is performed by
//! negating this fixed offset.
//!
//! Soft-iron distortion is more complicated, causing an offset that varies as
//! the sensor rotates, which results in the sensor returning an ellipse as it
//! rotates instead of a circle.  Performing soft-iron compensation requires
//! rotating the sensor reading such that the major axis of the ellipse is
//! aligned with one of the magnetometer axes, scaling one of the axes, then
//! rotating the scaled sensor reading back.  This operation is performed two
//! times; once to scale the Y axis to the same scale as the X axis, and once
//! again to scale the Z axis to the same scale as the X axis.
//!
//! Hard-iron compensation is performed prior to soft-iron compensation.
//!
//! \return None.
//
//*****************************************************************************
void
MagnetoCompensate(tMagnetoCompensation *psInst, float *pfMagnetoX,
                  float *pfMagnetoY, float *pfMagnetoZ)
{
    float fSin, fCos, fX, fY, fZ, fTemp;

    //
    // Get the magnetometer values.
    //
    fX = *pfMagnetoX;
    fY = *pfMagnetoY;
    fZ = *pfMagnetoZ;

    //
    // Perform hard-iron distortion compensation.
    //
    fX += psInst->fXOffset;
    fY += psInst->fYOffset;
    fZ += psInst->fZOffset;

    //
    // Perform soft-iron distortion compensation on the X-Y plane.  Start by
    // computing the sine and cosine of the rotation angle (which will be used
    // multiple times below).
    //
    fSin = sinf(psInst->fXYAngle);
    fCos = cosf(psInst->fXYAngle);

    //
    // Rotate the magnetometer reading around the Z axis.
    //
    fTemp = (fCos * fX) - (fSin * fY);
    fY = (fCos * fY) + (fSin * fX);
    fX = fTemp;

    //
    // Scale the Y-axis reading so that it has the same range as the X-axis
    // reading.
    //
    fY *= psInst->fYRatio;

    //
    // Rotate the magnetometer reading around the Z axis again, this time in
    // the opposite direction.
    //
    fTemp = (fCos * fX) + (fSin * fY);
    fY = (fCos * fY) - (fSin * fX);
    fX = fTemp;

    //
    // Perform soft-iron distortion compensation on the X-Z plane.  Start by
    // computing the sine and cosine of the rotation angle (which will be used
    // multiple times below).
    //
    fSin = sinf(psInst->fXZAngle);
    fCos = cosf(psInst->fXZAngle);

    //
    // Rotate the magnetometer reading around the Y axis.
    //
    fTemp = (fCos * fZ) - (fSin * fX);
    fX = (fCos * fX) + (fSin * fZ);
    fZ = fTemp;

    //
    // Scale the Z-axis reading so that it has the same range as the X-axis
    // reading.
    //
    fZ *= psInst->fZRatio;

    //
    // Rotate the magnetometer reading around the Y axis again, this time in
    // the opposite direction.
    //
    fTemp = (fCos * fZ) + (fSin * fX);
    fX = (fCos * fX) - (fSin * fZ);
    fZ = fTemp;

    //
    // Return the compensated magnetometer values.
    //
    *pfMagnetoX = fX;
    *pfMagnetoY = fY;
    *pfMagnetoZ = fZ;
}

//*****************************************************************************
//
//! Computes the compass heading from magnetometer data and roll/pitch.
//!
//! \param fMagnetoX is the X component of the magnetometer reading.
//! \param fMagnetoY is the Y component of the magnetometer reading.
//! \param fMagnetoZ is the Z component of the magnetometer reading.
//! \param fRoll is the roll angle, in radians.
//! \param fPitch is the pitch angle, in radians.
//!
//! This function computes the compass heading by performing tilt compensation
//! on the magnetometer reading.
//!
//! \return Returns the compass heading, in radians.
//
//*****************************************************************************
float
MagnetoHeadingCompute(float fMagnetoX, float fMagnetoY, float fMagnetoZ,
                      float fRoll, float fPitch)
{
    float fSinRoll, fCosRoll, fSinPitch, fCosPitch, fX, fY, fHeading;

    //
    // Compute the sine and cosine of the roll and pitch angles.
    //
    fSinRoll = sinf(fRoll);
    fCosRoll = cosf(fRoll);
    fSinPitch = sinf(fPitch);
    fCosPitch = cosf(fPitch);

    //
    // Rotate the magnetometer data such that it is level with the ground,
    // based on the provided roll and pitch.
    //
    fX = ((fMagnetoX * fCosPitch) + (fMagnetoY * fSinRoll * fSinPitch) +
          (fMagnetoZ * fCosRoll * fSinPitch));
    fY = (fMagnetoY * fCosRoll) - (fMagnetoZ * fSinRoll);

    //
    // Compute the compass heading and make it positive.
    //
    fHeading = atan2f(-fY, fX);
    if(fHeading < 0)
    {
        fHeading += 2 * 3.141592;
    }

    //
    // Return the computed compass heading.
    //
    return(fHeading);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
