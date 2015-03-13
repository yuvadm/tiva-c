//*****************************************************************************
//
// vector.c - Functions for performing vector operations.
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

#include "sensorlib/vector.h"

//*****************************************************************************
//
//! \addtogroup vector_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Computes the dot product of two vectors.
//!
//! \param pfVectorIn1 is the first vector.
//! \param pfVectorIn2 is the second vector.
//!
//! This function computes the dot product of two 3-dimensional vector.
//!
//! \return Returns the dot product of the two vectors.
//
//*****************************************************************************
float
VectorDotProduct(float pfVectorIn1[3], float pfVectorIn2[3])
{
    //
    // Compute and return the vector dot product.
    //
    return((pfVectorIn1[0] * pfVectorIn2[0]) +
           (pfVectorIn1[1] * pfVectorIn2[1]) +
           (pfVectorIn1[2] * pfVectorIn2[2]));
}

//*****************************************************************************
//
//! Computes the cross product of two vectors.
//!
//! \param pfVectorOut is the output vector.
//! \param pfVectorIn1 is the first vector.
//! \param pfVectorIn2 is the second vector.
//!
//! This function computes the cross product of two 3-dimensional vectors.
//!
//! \return None.
//
//*****************************************************************************
void
VectorCrossProduct(float pfVectorOut[3], float pfVectorIn1[3],
                   float pfVectorIn2[3])
{
    //
    // Compute the cross product of the input vectors.
    //
    pfVectorOut[0] = ((pfVectorIn1[1] * pfVectorIn2[2]) -
                      (pfVectorIn1[2] * pfVectorIn2[1]));
    pfVectorOut[1] = ((pfVectorIn1[2] * pfVectorIn2[0]) -
                      (pfVectorIn1[0] * pfVectorIn2[2]));
    pfVectorOut[2] = ((pfVectorIn1[0] * pfVectorIn2[1]) -
                      (pfVectorIn1[1] * pfVectorIn2[0]));
}

//*****************************************************************************
//
//! Scales a vector.
//!
//! \param pfVectorOut is the output vector.
//! \param pfVectorIn is the input vector.
//! \param fScale is the scale factor.
//!
//! This function scales a 3-dimensional vector by multiplying each of its
//! components by the scale factor.
//!
//! \return None.
//
//*****************************************************************************
void
VectorScale(float pfVectorOut[3], float pfVectorIn[3], float fScale)
{
    //
    // Scale each component of the vector by the scale factor.
    //
    pfVectorOut[0] = pfVectorIn[0] * fScale;
    pfVectorOut[1] = pfVectorIn[1] * fScale;
    pfVectorOut[2] = pfVectorIn[2] * fScale;
}

//*****************************************************************************
//
//! Adds two vectors.
//!
//! \param pfVectorOut is the output vector.
//! \param pfVectorIn1 is the first vector.
//! \param pfVectorIn2 is the second vector.
//!
//! This function adds two 3-dimensional vectors.
//!
//! \return None.
//
//*****************************************************************************
void
VectorAdd(float pfVectorOut[3], float pfVectorIn1[3], float pfVectorIn2[3])
{
    //
    // Add the components of the two vectors.
    //
    pfVectorOut[0] = pfVectorIn1[0] + pfVectorIn2[0];
    pfVectorOut[1] = pfVectorIn1[1] + pfVectorIn2[1];
    pfVectorOut[2] = pfVectorIn1[2] + pfVectorIn2[2];
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
