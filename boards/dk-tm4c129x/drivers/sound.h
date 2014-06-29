//*****************************************************************************
//
// sound.h - Prototypes for the sound driver.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C129X Firmware Package.
//
//*****************************************************************************

#ifndef __DRIVERS_SOUND_H__
#define __DRIVERS_SOUND_H__

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
// Prototypes for the APIs.
//
//*****************************************************************************
extern void SoundIntHandler(void);
extern void SoundInit(uint32_t ui32SysClock);
extern bool SoundStart(int16_t *pi16Buffer, uint32_t ui32Length,
                       uint32_t ui32Rate,
                       void (*pfnCallback)(uint32_t ui32Half));
extern void SoundPeriodAdjust(int32_t i32RateAdjust);
extern void SoundStop(void);
extern bool SoundBusy(void);
extern void SoundVolumeSet(int32_t i32Volume);
extern void SoundVolumeUp(int32_t i32Volume);
extern void SoundVolumeDown(int32_t i32Volume);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __DRIVERS_SOUND_H__
