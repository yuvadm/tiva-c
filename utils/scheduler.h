//*****************************************************************************
//
// scheduler.h - Public header for the simple timed function scheduler module.
//
// Copyright (c) 2010-2014 Texas Instruments Incorporated.  All rights reserved.
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
#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

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
//! \addtogroup scheduler_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Prototype of a function that the scheduler can call periodically.
//
//*****************************************************************************
typedef void (*tSchedulerFunction)(void *pvParam);

//*****************************************************************************
//
//! The structure defining a function which the scheduler will call
//! periodically.
//
//*****************************************************************************
typedef struct
{
    //
    //! A pointer to the function which is to be called periodically by the
    //! scheduler.
    //
    void (*pfnFunction)(void *);

    //
    //! The parameter which is to be passed to this function when it is called.
    //
    void *pvParam;

    //
    //! The frequency the function is to be called expressed in terms of system
    //! ticks.  If this value is 0, the function will be called on every call
    //! to SchedulerRun.
    //
    uint32_t ui32FrequencyTicks;

    //
    //! Tick count when this function was last called.  This field is updated
    //! by the scheduler.
    //
    uint32_t ui32LastCall;

    //
    //! A flag indicating whether or not this task is active.  If true, the
    //! function will be called periodically.  If false, the function is
    //! disabled and will not be called.
    //
    bool bActive;
}
tSchedulerTask;

//*****************************************************************************
//
//! This global table must be populated by the client and contains information
//! on each function that the scheduler is to call.
//
//*****************************************************************************
extern tSchedulerTask g_psSchedulerTable[];

//*****************************************************************************
//
//! This global variable must be exported by the client.  It must contain the
//! number of entries in the g_psSchedulerTable array.
//
//*****************************************************************************
extern uint32_t g_ui32SchedulerNumTasks;

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// Public function prototypes
//
//*****************************************************************************
extern void SchedulerSysTickIntHandler(void);
extern void SchedulerInit(uint32_t ui32TicksPerSecond);
extern void SchedulerRun(void);
extern void SchedulerTaskEnable(uint32_t ui32Index, bool bRunNow);
extern void SchedulerTaskDisable(uint32_t ui32Index);
extern uint32_t SchedulerTickCountGet(void);
extern uint32_t SchedulerElapsedTicksGet(uint32_t ui32TickCount);
extern uint32_t SchedulerElapsedTicksCalc(uint32_t ui32TickStart,
                                               uint32_t ui32TickEnd);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __ SCHEDULER_H_
