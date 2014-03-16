//*****************************************************************************
//
// speexlib.h - interface to the speex coder/encoder library.
//
// Copyright (c) 2009-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the Tiva Utility Library.
//
//*****************************************************************************

#ifndef __SPEEXLIB_H__
#define __SPEEXLIB_H__

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
// Prototypes.
//
//*****************************************************************************
extern int32_t SpeexEncodeInit(int iSampleRate, int iComplexity, int iQuality);
extern int32_t SpeexEncode(int16_t *pui16InBuffer, uint32_t ui32InSize,
                           uint8_t *pui8OutBuffer, uint32_t ui32OutSize);
extern int32_t SpeexEncodeQualitySet(int iQuality);
extern int32_t SpeexEncodeFrameSizeGet(void);
extern int32_t SpeexDecodeFrameSizeGet(void);
extern int32_t SpeexDecodeInit(void);
extern int32_t SpeexDecode(uint8_t *pui8InBuffer, uint32_t ui32InSize,
                           uint8_t *pui8OutBuffer, uint32_t ui32OutSize);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __SPEEXLIB_H__
