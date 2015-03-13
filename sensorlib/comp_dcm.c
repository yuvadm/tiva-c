//*****************************************************************************
//
// comp_dcm.c - Complementary filter algorithm on a Direction Cosine Matrix for
//              fusing sensor data from an accelerometer, gyroscope, and
//              magnetometer.
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

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "driverlib/debug.h"
#include "sensorlib/comp_dcm.h"
#include "sensorlib/vector.h"

//*****************************************************************************
//
//! \addtogroup comp_dcm_api
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
//! Initializes the complementary filter DCM attitude estimation state.
//!
//! \param psDCM is a pointer to the DCM state structure.
//! \param fDeltaT is the amount of time between DCM updates, in seconds.
//! \param fScaleA is the weight of the accelerometer reading in determining
//! the updated attitude estimation.
//! \param fScaleG is the weight of the gyroscope reading in determining the
//! updated attitude estimation.
//! \param fScaleM is the weight of the magnetometer reading in determining the
//! updated attitude estimation.
//!
//! This function initializes the complementary filter DCM attitude estimation
//! state, and must be called prior to performing any attitude estimation.
//!
//! New readings must be supplied to the complementary filter DCM attitude
//! estimation algorithm at the rate specified by the \e fDeltaT parameter.
//! Failure to provide new readings at this rate results in inaccuracies in the
//! attitude estimation.
//!
//! The \e fScaleA, \e fScaleG, and \e fScaleM weights must sum to one.
//!
//! \return None.
//
//*****************************************************************************
void
CompDCMInit(tCompDCM *psDCM, float fDeltaT, float fScaleA, float fScaleG,
            float fScaleM)
{
    //
    // Initialize the DCM matrix to the identity matrix.
    //
    psDCM->ppfDCM[0][0] = 1.0;
    psDCM->ppfDCM[0][1] = 0.0;
    psDCM->ppfDCM[0][2] = 0.0;
    psDCM->ppfDCM[1][0] = 0.0;
    psDCM->ppfDCM[1][1] = 1.0;
    psDCM->ppfDCM[1][2] = 0.0;
    psDCM->ppfDCM[2][0] = 0.0;
    psDCM->ppfDCM[2][1] = 0.0;
    psDCM->ppfDCM[2][2] = 1.0;

    //
    // Save the time delta between DCM updates.
    //
    psDCM->fDeltaT = fDeltaT;

    //
    // Save the scaling factors that are applied to the accelerometer,
    // gyroscope, and magnetometer readings.
    //
    psDCM->fScaleA = fScaleA;
    psDCM->fScaleG = fScaleG;
    psDCM->fScaleM = fScaleM;
}

//*****************************************************************************
//
//! Updates the accelerometer reading used by the complementary filter DCM
//! algorithm.
//!
//! \param psDCM is a pointer to the DCM state structure.
//! \param fAccelX is the accelerometer reading in the X body axis.
//! \param fAccelY is the accelerometer reading in the Y body axis.
//! \param fAccelZ is the accelerometer reading in the Z body axis.
//!
//! This function updates the accelerometer reading used by the complementary
//! filter DCM algorithm.  The accelerometer readings provided to this function
//! are used by subsequent calls to CompDCMStart() and CompDCMUpdate() to
//! compute the attitude estimate.
//!
//! \return None.
//
//*****************************************************************************
void
CompDCMAccelUpdate(tCompDCM *psDCM, float fAccelX, float fAccelY,
                   float fAccelZ)
{
    //
    // The user should never pass in values that are not-a-number
    //
    ASSERT(!isnan(fAccelX));
    ASSERT(!isnan(fAccelY));
    ASSERT(!isnan(fAccelZ));

    //
    // Save the new accelerometer reading.
    //
    psDCM->pfAccel[0] = fAccelX;
    psDCM->pfAccel[1] = fAccelY;
    psDCM->pfAccel[2] = fAccelZ;
}

//*****************************************************************************
//
//! Updates the gyroscope reading used by the complementary filter DCM
//! algorithm.
//!
//! \param psDCM is a pointer to the DCM state structure.
//! \param fGyroX is the gyroscope reading in the X body axis.
//! \param fGyroY is the gyroscope reading in the Y body axis.
//! \param fGyroZ is the gyroscope reading in the Z body axis.
//!
//! This function updates the gyroscope reading used by the complementary
//! filter DCM algorithm.  The gyroscope readings provided to this function are
//! used by subsequent calls to CompDCMUpdate() to compute the attitude
//! estimate.
//!
//! \return None.
//
//*****************************************************************************
void
CompDCMGyroUpdate(tCompDCM *psDCM, float fGyroX, float fGyroY, float fGyroZ)
{
    //
    // The user should never pass in values that are not-a-number
    //
    ASSERT(!isnan(fGyroX));
    ASSERT(!isnan(fGyroY));
    ASSERT(!isnan(fGyroZ));

    //
    // Save the new gyroscope reading.
    //
    psDCM->pfGyro[0] = fGyroX;
    psDCM->pfGyro[1] = fGyroY;
    psDCM->pfGyro[2] = fGyroZ;
}

//*****************************************************************************
//
//! Updates the magnetometer reading used by the complementary filter DCM
//! algorithm.
//!
//! \param psDCM is a pointer to the DCM state structure.
//! \param fMagnetoX is the magnetometer reading in the X body axis.
//! \param fMagnetoY is the magnetometer reading in the Y body axis.
//! \param fMagnetoZ is the magnetometer reading in the Z body axis.
//!
//! This function updates the magnetometer reading used by the complementary
//! filter DCM algorithm.  The magnetometer readings provided to this function
//! are used by subsequent calls to CompDCMStart() and CompDCMUpdate() to
//! compute the attitude estimate.
//!
//! \return None.
//
//*****************************************************************************
void
CompDCMMagnetoUpdate(tCompDCM *psDCM, float fMagnetoX, float fMagnetoY,
                     float fMagnetoZ)
{
    //
    // The user should never pass in values that are not-a-number
    //
    ASSERT(!isnan(fMagnetoX));
    ASSERT(!isnan(fMagnetoY));
    ASSERT(!isnan(fMagnetoZ));

    //
    // Save the new magnetometer reading.
    //
    psDCM->pfMagneto[0] = fMagnetoX;
    psDCM->pfMagneto[1] = fMagnetoY;
    psDCM->pfMagneto[2] = fMagnetoZ;
}

//*****************************************************************************
//
//! Starts the complementary filter DCM attitude estimation from an initial
//! sensor reading.
//!
//! \param psDCM is a pointer to the DCM state structure.
//!
//! This function computes the initial complementary filter DCM attitude
//! estimation state based on the initial accelerometer and magnetometer
//! reading.  While not necessary for the attitude estimation to converge,
//! using an initial state based on sensor readings results in quicker
//! convergence.
//!
//! \return None.
//
//*****************************************************************************
void
CompDCMStart(tCompDCM *psDCM)
{
    float pfI[3], pfJ[3], pfK[3];

    //
    // The magnetometer reading forms the initial I vector, pointing north.
    //
    pfI[0] = psDCM->pfMagneto[0];
    pfI[1] = psDCM->pfMagneto[1];
    pfI[2] = psDCM->pfMagneto[2];

    //
    // The accelerometer reading forms the initial K vector, pointing down.
    //
    pfK[0] = psDCM->pfAccel[0];
    pfK[1] = psDCM->pfAccel[1];
    pfK[2] = psDCM->pfAccel[2];

    //
    // Compute the initial J vector, which is the cross product of the K and I
    // vectors.
    //
    VectorCrossProduct(pfJ, pfK, pfI);

    //
    // Recompute the I vector from the cross product of the J and K vectors.
    // This makes it fully orthogonal, which it wasn't before since magnetic
    // north points inside the Earth in many places.
    //
    VectorCrossProduct(pfI, pfJ, pfK);

    //
    // Normalize the I, J, and K vectors.
    //
    VectorScale(pfI, pfI, 1 / sqrtf(VectorDotProduct(pfI, pfI)));
    VectorScale(pfJ, pfJ, 1 / sqrtf(VectorDotProduct(pfJ, pfJ)));
    VectorScale(pfK, pfK, 1 / sqrtf(VectorDotProduct(pfK, pfK)));

    //
    // Initialize the DCM matrix from the I, J, and K vectors.
    //
    psDCM->ppfDCM[0][0] = pfI[0];
    psDCM->ppfDCM[0][1] = pfI[1];
    psDCM->ppfDCM[0][2] = pfI[2];
    psDCM->ppfDCM[1][0] = pfJ[0];
    psDCM->ppfDCM[1][1] = pfJ[1];
    psDCM->ppfDCM[1][2] = pfJ[2];
    psDCM->ppfDCM[2][0] = pfK[0];
    psDCM->ppfDCM[2][1] = pfK[1];
    psDCM->ppfDCM[2][2] = pfK[2];
}

//*****************************************************************************
//
//! Updates the complementary filter DCM attitude estimation based on an
//! updated set of sensor readings.
//!
//! \param psDCM is a pointer to the DCM state structure.
//!
//! This function updates the complementary filter DCM attitude estimation
//! state based on the current sensor readings.  This function must be called
//! at the rate specified to CompDCMInit(), with new readings supplied at an
//! appropriate rate (for example, magnetometers typically sample at a much
//! slower rate than accelerometers and gyroscopes).
//!
//! \return None.
//
//*****************************************************************************
void
CompDCMUpdate(tCompDCM *psDCM)
{
    float pfI[3], pfJ[3], pfK[3], pfDelta[3], pfTemp[3], fError;
    bool bNAN;

    //
    // The magnetometer reading forms the new Im vector, pointing north.
    //
    pfI[0] = psDCM->pfMagneto[0];
    pfI[1] = psDCM->pfMagneto[1];
    pfI[2] = psDCM->pfMagneto[2];

    //
    // The accelerometer reading forms the new Ka vector, pointing down.
    //
    pfK[0] = psDCM->pfAccel[0];
    pfK[1] = psDCM->pfAccel[1];
    pfK[2] = psDCM->pfAccel[2];

    //
    // Compute the new J vector, which is the cross product of the Ka and Im
    // vectors.
    //
    VectorCrossProduct(pfJ, pfK, pfI);

    //
    // Recompute the Im vector from the cross product of the J and Ka vectors.
    // This makes it fully orthogonal, which it wasn't before since magnetic
    // north points inside the Earth in many places.
    //
    VectorCrossProduct(pfI, pfJ, pfK);

    //
    // Normalize the Im and Ka vectors.
    //
    VectorScale(pfI, pfI, 1 / sqrtf(VectorDotProduct(pfI, pfI)));
    VectorScale(pfK, pfK, 1 / sqrtf(VectorDotProduct(pfK, pfK)));

    //
    // Compute and scale the rotation as inferred from the accelerometer,
    // storing it in the rotation accumulator.
    //
    VectorCrossProduct(pfTemp, psDCM->ppfDCM[2], pfK);
    VectorScale(pfDelta, pfTemp, psDCM->fScaleA);

    //
    // Compute and scale the rotation as measured by the gyroscope, adding it
    // to the rotation accumulator.
    //
    pfTemp[0] = psDCM->pfGyro[0] * psDCM->fDeltaT * psDCM->fScaleG;
    pfTemp[1] = psDCM->pfGyro[1] * psDCM->fDeltaT * psDCM->fScaleG;
    pfTemp[2] = psDCM->pfGyro[2] * psDCM->fDeltaT * psDCM->fScaleG;
    VectorAdd(pfDelta, pfDelta, pfTemp);

    //
    // Compute and scale the rotation as inferred from the magnetometer, adding
    // it to the rotation accumulator.
    //
    VectorCrossProduct(pfTemp, psDCM->ppfDCM[0], pfI);
    VectorScale(pfTemp, pfTemp, psDCM->fScaleM);
    VectorAdd(pfDelta, pfDelta, pfTemp);

    //
    // Rotate the I vector from the DCM matrix by the scaled rotation.
    //
    VectorCrossProduct(pfI, pfDelta, psDCM->ppfDCM[0]);
    VectorAdd(psDCM->ppfDCM[0], psDCM->ppfDCM[0], pfI);

    //
    // Rotate the K vector from the DCM matrix by the scaled rotation.
    //
    VectorCrossProduct(pfK, pfDelta, psDCM->ppfDCM[2]);
    VectorAdd(psDCM->ppfDCM[2], psDCM->ppfDCM[2], pfK);

    //
    // Compute the orthogonality error between the rotated I and K vectors and
    // adjust each by half the error, bringing them closer to orthogonality.
    //
    fError = VectorDotProduct(psDCM->ppfDCM[0], psDCM->ppfDCM[2]) / -2.0;
    VectorScale(pfI, psDCM->ppfDCM[0], fError);
    VectorScale(pfK, psDCM->ppfDCM[2], fError);
    VectorAdd(psDCM->ppfDCM[0], psDCM->ppfDCM[0], pfK);
    VectorAdd(psDCM->ppfDCM[2], psDCM->ppfDCM[2], pfI);

    //
    // Normalize the I and K vectors.
    //
    VectorScale(psDCM->ppfDCM[0], psDCM->ppfDCM[0],
                0.5 * (3.0 - VectorDotProduct(psDCM->ppfDCM[0],
                                              psDCM->ppfDCM[0])));
    VectorScale(psDCM->ppfDCM[2], psDCM->ppfDCM[2],
                0.5 * (3.0 - VectorDotProduct(psDCM->ppfDCM[2],
                                              psDCM->ppfDCM[2])));

    //
    // Compute the rotated J vector from the cross product of the rotated,
    // corrected K and I vectors.
    //
    VectorCrossProduct(psDCM->ppfDCM[1], psDCM->ppfDCM[2], psDCM->ppfDCM[0]);

    //
    // Determine if the newly updated DCM contains any invalid (in other words,
    // NaN) values.
    //
    bNAN = (isnan(psDCM->ppfDCM[0][0]) ||
            isnan(psDCM->ppfDCM[0][1]) ||
            isnan(psDCM->ppfDCM[0][2]) ||
            isnan(psDCM->ppfDCM[1][0]) ||
            isnan(psDCM->ppfDCM[1][1]) ||
            isnan(psDCM->ppfDCM[1][2]) ||
            isnan(psDCM->ppfDCM[2][0]) ||
            isnan(psDCM->ppfDCM[2][1]) ||
            isnan(psDCM->ppfDCM[2][2]));

    //
    // As a debug measure, we check for NaN in the DCM.  The user can trap
    // this event depending on their implementation of __error__.  Should they
    // choose to disable interrupts and loop forever then they will have
    // preserved the stack and can analyze how they arrived at NaN.
    //
    ASSERT(!bNAN);

    //
    // If any part of the matrix is not-a-number then reset the DCM back to the
    // identity matrix.
    //
    if(bNAN)
    {
        psDCM->ppfDCM[0][0] = 1.0;
        psDCM->ppfDCM[0][1] = 0.0;
        psDCM->ppfDCM[0][2] = 0.0;
        psDCM->ppfDCM[1][0] = 0.0;
        psDCM->ppfDCM[1][1] = 1.0;
        psDCM->ppfDCM[1][2] = 0.0;
        psDCM->ppfDCM[2][0] = 0.0;
        psDCM->ppfDCM[2][1] = 0.0;
        psDCM->ppfDCM[2][2] = 1.0;
    }
}

//*****************************************************************************
//
//! Returns the current DCM attitude estimation matrix.
//!
//! \param psDCM is a pointer to the DCM state structure.
//! \param ppfDCM is a pointer to the array into which to store the DCM matrix
//! values.
//!
//! This function returns the current value of the DCM matrix.
//!
//! \return None.
//
//*****************************************************************************
void
CompDCMMatrixGet(tCompDCM *psDCM, float ppfDCM[3][3])
{
    //
    // Return the current DCM matrix.
    //
    ppfDCM[0][0] = psDCM->ppfDCM[0][0];
    ppfDCM[0][1] = psDCM->ppfDCM[0][1];
    ppfDCM[0][2] = psDCM->ppfDCM[0][2];
    ppfDCM[1][0] = psDCM->ppfDCM[1][0];
    ppfDCM[1][1] = psDCM->ppfDCM[1][1];
    ppfDCM[1][2] = psDCM->ppfDCM[1][2];
    ppfDCM[2][0] = psDCM->ppfDCM[2][0];
    ppfDCM[2][1] = psDCM->ppfDCM[2][1];
    ppfDCM[2][2] = psDCM->ppfDCM[2][2];
}

//*****************************************************************************
//
//! Computes the Euler angles from the DCM attitude estimation matrix.
//!
//! \param psDCM is a pointer to the DCM state structure.
//! \param pfRoll is a pointer to the value into which the roll is stored.
//! \param pfPitch is a pointer to the value into which the pitch is stored.
//! \param pfYaw is a pointer to the value into which the yaw is stored.
//!
//! This function computes the Euler angles that are represented by the DCM
//! attitude estimation matrix.  If any of the Euler angles is not required,
//! the corresponding parameter can be \b NULL.
//!
//! \return None.
//
//*****************************************************************************
void
CompDCMComputeEulers(tCompDCM *psDCM, float *pfRoll, float *pfPitch,
                     float *pfYaw)
{
    //
    // Compute the roll, pitch, and yaw as required.
    //
    if(pfRoll)
    {
        *pfRoll = atan2f(psDCM->ppfDCM[2][1], psDCM->ppfDCM[2][2]);
    }
    if(pfPitch)
    {
        *pfPitch = -asinf(psDCM->ppfDCM[2][0]);
    }
    if(pfYaw)
    {
        *pfYaw = atan2f(psDCM->ppfDCM[1][0], psDCM->ppfDCM[0][0]);
    }
}

//*****************************************************************************
//
//! Computes the quaternion from the DCM attitude estimation matrix.
//!
//! \param psDCM is a pointer to the DCM state structure.
//! \param pfQuaternion is an array into which the quaternion is stored.
//!
//! This function computes the quaternion that is represented by the DCM
//! attitude estimation matrix.
//!
//! \return None.
//
//*****************************************************************************
void
CompDCMComputeQuaternion(tCompDCM *psDCM, float pfQuaternion[4])
{
    float fQs, fQx, fQy, fQz;

    //
    // Partially compute Qs, Qx, Qy, and Qz based on the DCM diagonals.  The
    // square root, an expensive operation, is computed for only one of these
    // as determined later.
    //
    fQs = 1 + psDCM->ppfDCM[0][0] + psDCM->ppfDCM[1][1] + psDCM->ppfDCM[2][2];
    fQx = 1 + psDCM->ppfDCM[0][0] - psDCM->ppfDCM[1][1] - psDCM->ppfDCM[2][2];
    fQy = 1 - psDCM->ppfDCM[0][0] + psDCM->ppfDCM[1][1] - psDCM->ppfDCM[2][2];
    fQz = 1 - psDCM->ppfDCM[0][0] - psDCM->ppfDCM[1][1] + psDCM->ppfDCM[2][2];

    //
    // See if Qs is the largest of the diagonal values.
    //
    if((fQs > fQx) && (fQs > fQy) && (fQs > fQz))
    {
        //
        // Finish the computation of Qs.
        //
        fQs = sqrtf(fQs) / 2;

        //
        // Compute the values of the quaternion based on Qs.
        //
        pfQuaternion[0] = fQs;
        pfQuaternion[1] = ((psDCM->ppfDCM[2][1] - psDCM->ppfDCM[1][2]) /
                           (4 * fQs));
        pfQuaternion[2] = ((psDCM->ppfDCM[0][2] - psDCM->ppfDCM[2][0]) /
                           (4 * fQs));
        pfQuaternion[3] = ((psDCM->ppfDCM[1][0] - psDCM->ppfDCM[0][1]) /
                           (4 * fQs));
    }

    //
    // Qs is not the largest, so see if Qx is the largest remaining diagonal
    // value.
    //
    else if((fQx > fQy) && (fQx > fQz))
    {
        //
        // Finish the computation of Qx.
        //
        fQx = sqrtf(fQx) / 2;

        //
        // Compute the values of the quaternion based on Qx.
        //
        pfQuaternion[0] = ((psDCM->ppfDCM[2][1] - psDCM->ppfDCM[1][2]) /
                           (4 * fQx));
        pfQuaternion[1] = fQx;
        pfQuaternion[2] = ((psDCM->ppfDCM[1][0] + psDCM->ppfDCM[0][1]) /
                           (4 * fQx));
        pfQuaternion[3] = ((psDCM->ppfDCM[0][2] + psDCM->ppfDCM[2][0]) /
                           (4 * fQx));
    }

    //
    // Qs and Qx are not the largest, so see if Qy is the largest remaining
    // diagonal value.
    //
    else if(fQy > fQz)
    {
        //
        // Finish the computation of Qy.
        //
        fQy = sqrtf(fQy) / 2;

        //
        // Compute the values of the quaternion based on Qy.
        //
        pfQuaternion[0] = ((psDCM->ppfDCM[0][2] - psDCM->ppfDCM[2][0]) /
                           (4 * fQy));
        pfQuaternion[1] = ((psDCM->ppfDCM[1][0] + psDCM->ppfDCM[0][1]) /
                           (4 * fQy));
        pfQuaternion[2] = fQy;
        pfQuaternion[3] = ((psDCM->ppfDCM[2][1] + psDCM->ppfDCM[1][2]) /
                           (4 * fQy));
    }

    //
    // Qz is the largest diagonal value.
    //
    else
    {
        //
        // Finish the computation of Qz.
        //
        fQz = sqrtf(fQz) / 2;

        //
        // Compute the values of the quaternion based on Qz.
        //
        pfQuaternion[0] = ((psDCM->ppfDCM[1][0] - psDCM->ppfDCM[0][1]) /
                           (4 * fQz));
        pfQuaternion[1] = ((psDCM->ppfDCM[0][2] + psDCM->ppfDCM[2][0]) /
                           (4 * fQz));
        pfQuaternion[2] = ((psDCM->ppfDCM[2][1] + psDCM->ppfDCM[1][2]) /
                           (4 * fQz));
        pfQuaternion[3] = fQz;
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
