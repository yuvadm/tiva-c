//*****************************************************************************
//
// scheduler.c - A simple task scheduler
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

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/debug.h"
#include "utils/scheduler.h"

//*****************************************************************************
//
//! \addtogroup scheduler_api
//! @{
//
//*****************************************************************************

static volatile uint32_t g_ui32SchedulerTickCount;

//*****************************************************************************
//
//! Handles the SysTick interrupt on behalf of the scheduler module.
//!
//! Applications using the scheduler module must ensure that this function is
//! hooked to the SysTick interrupt vector.
//!
//! \return None.
//
//*****************************************************************************
void
SchedulerSysTickIntHandler(void)
{
    g_ui32SchedulerTickCount++;
}

//*****************************************************************************
//
//! Initializes the task scheduler.
//!
//! \param ui32TicksPerSecond sets the basic frequency of the SysTick interrupt
//! used by the scheduler to determine when to run the various task functions.
//!
//! This function must be called during application startup to configure the
//! SysTick timer.  This is used by the scheduler module to determine when each
//! of the functions provided in the g_psSchedulerTable array is called.
//!
//! The caller is responsible for ensuring that SchedulerSysTickIntHandler()
//! has previously been installed in the SYSTICK vector in the vector table
//! and must also ensure that interrupts are enabled at the CPU level.
//!
//! Note that this call does not start the scheduler calling the configured
//! functions.  All function calls are made in the context of later calls to
//! SchedulerRun().  This call merely configures the SysTick interrupt that is
//! used by the scheduler to determine what the current system time is.
//!
//! \return None.
//
//*****************************************************************************
void
SchedulerInit(uint32_t ui32TicksPerSecond)
{
    ASSERT(ui32TicksPerSecond);

    //
    // Configure SysTick for a periodic interrupt.
    //
    SysTickPeriodSet(SysCtlClockGet() / ui32TicksPerSecond);
    SysTickEnable();
    SysTickIntEnable();
}

//*****************************************************************************
//
//! Instructs the scheduler to update its task table and make calls to
//! functions needing called.
//!
//! This function must be called periodically by the client to allow the
//! scheduler to make calls to any configured task functions if it is their
//! time to be called.  The call must be made at least as frequently as the
//! most frequent task configured in the g_psSchedulerTable array.
//!
//! Although the scheduler makes use of the SysTick interrupt, all calls to
//! functions configured in \e g_psSchedulerTable are made in the context of
//! SchedulerRun().
//!
//! \return None.
//
//*****************************************************************************
void
SchedulerRun(void)
{
    uint32_t ui32Loop;
    tSchedulerTask *pi16Task;

    //
    // Loop through each task in the task table.
    //
    for(ui32Loop = 0; ui32Loop < g_ui32SchedulerNumTasks; ui32Loop++)
    {
        //
        // Get a pointer to the task information.
        //
        pi16Task = &g_psSchedulerTable[ui32Loop];

        //
        // Is this task active and, if so, is it time to call it's function?
        //
        if(pi16Task->bActive &&
           (SchedulerElapsedTicksGet(pi16Task->ui32LastCall) >=
            pi16Task->ui32FrequencyTicks))
        {
            //
            // Remember the timestamp at which we make the function call.
            //
            pi16Task->ui32LastCall = g_ui32SchedulerTickCount;

            //
            // Call the task function, passing the provided parameter.
            //
            pi16Task->pfnFunction(pi16Task->pvParam);
        }
    }
}

//*****************************************************************************
//
//! Enables a task and allows the scheduler to call it periodically.
//!
//! \param ui32Index is the index of the task which is to be enabled in the
//! global \e g_psSchedulerTable array.
//! \param bRunNow is \b true if the task is to be run on the next call to
//! SchedulerRun() or \b false if one whole period is to elapse before the task
//! is run.
//!
//! This function marks one of the configured tasks as enabled and causes
//! SchedulerRun() to call that task periodically.  The caller may choose to
//! have the enabled task run for the first time on the next call to
//! SchedulerRun() or to wait one full task period before making the first
//! call.
//!
//! \return None.
//
//*****************************************************************************
void
SchedulerTaskEnable(uint32_t ui32Index, bool bRunNow)
{
    //
    // Is the task index passed valid?
    //
    if(ui32Index < g_ui32SchedulerNumTasks)
    {
        //
        // Yes - mark the task as active.
        //
        g_psSchedulerTable[ui32Index].bActive = true;

        //
        // Set the last call time to ensure that the function is called either
        // next time the scheduler is run or after the desired number of ticks
        // depending upon the value of the bRunNow parameter.
        //
        if(bRunNow)
        {
            //
            // Cause the task to run on the next call to SchedulerRun().
            //
            g_psSchedulerTable[ui32Index].ui32LastCall =
                (g_ui32SchedulerTickCount -
                 g_psSchedulerTable[ui32Index].ui32FrequencyTicks);
        }
        else
        {
            //
            // Cause the task to run after one full time period.
            //
            g_psSchedulerTable[ui32Index].ui32LastCall =
                g_ui32SchedulerTickCount;
        }
    }
}

//*****************************************************************************
//
//! Disables a task and prevents the scheduler from calling it.
//!
//! \param ui32Index is the index of the task which is to be disabled in the
//! global \e g_psSchedulerTable array.
//!
//! This function marks one of the configured tasks as inactive and prevents
//! SchedulerRun() from calling it.  The task may be reenabled by calling
//! SchedulerTaskEnable().
//!
//! \return None.
//
//*****************************************************************************
void
SchedulerTaskDisable(uint32_t ui32Index)
{
    //
    // Is the task index passed valid?
    //
    if(ui32Index < g_ui32SchedulerNumTasks)
    {
        //
        // Yes - mark the task as inactive.
        //
        g_psSchedulerTable[ui32Index].bActive = false;
    }
}

//*****************************************************************************
//
//! Returns the current system time in ticks since power on.
//!
//! This function may be called by a client to retrieve the current system
//! time.  The value returned is a count of ticks elapsed since the system
//! last booted.
//!
//! \return Tick count since last boot.
//
//*****************************************************************************
uint32_t
SchedulerTickCountGet(void)
{
    return(g_ui32SchedulerTickCount);
}

//*****************************************************************************
//
//! Returns the number of ticks elapsed since the provided tick count.
//!
//! \param ui32TickCount is the tick count from which to determine the elapsed
//! time.
//!
//! This function may be called by a client to determine how much time has
//! passed since a particular tick count provided in the \e ui32TickCount
//! parameter.  This function takes into account wrapping of the global tick
//! counter and assumes that the provided tick count always represents a time
//! in the past.  The returned value will, of course, be wrong if the tick
//! counter has wrapped more than once since the passed \e ui32TickCount.  As a
//! result, please do not use this function if you are dealing with timeouts
//! of 497 days or longer (assuming you use a 10mS tick period).
//!
//! \return The number of ticks elapsed since the provided tick count.
//
//*****************************************************************************
uint32_t
SchedulerElapsedTicksGet(uint32_t ui32TickCount)
{
    //
    // Determine the calculation based upon whether the global tick count has
    // wrapped since the passed ui32TickCount.
    //
    return(SchedulerElapsedTicksCalc(ui32TickCount, g_ui32SchedulerTickCount));
}

//*****************************************************************************
//
//! Returns the number of ticks elapsed between two times.
//!
//! \param ui32TickStart is the system tick count for the start of the period.
//! \param ui32TickEnd is the system tick count for the end of the period.
//!
//! This function may be called by a client to determine the number of ticks
//! which have elapsed between provided starting and ending tick counts.  The
//! function takes into account wrapping cases where the end tick count is
//! lower than the starting count assuming that the ending tick count always
//! represents a later time than the starting count.
//!
//! \return The number of ticks elapsed between the provided start and end
//! counts.
//
//*****************************************************************************
uint32_t
SchedulerElapsedTicksCalc(uint32_t ui32TickStart, uint32_t ui32TickEnd)
{
    return((ui32TickEnd > ui32TickStart) ? (ui32TickEnd - ui32TickStart) :
           ((0xFFFFFFFF - ui32TickStart) + ui32TickEnd + 1));
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
