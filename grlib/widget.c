//*****************************************************************************
//
// widget.c - Generic widget tree handling code.
//
// Copyright (c) 2008-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the Tiva Graphics Library.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/debug.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"

//*****************************************************************************
//
//! \addtogroup widget_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Flags that indicate how messages from the message queue are processed.  They
// can be sent via either a pre-order or post-order search, and can optionally
// be sent to no other widgets once one accepts the message.
//
//*****************************************************************************
#define MQ_FLAG_POST_ORDER      1
#define MQ_FLAG_STOP_ON_SUCCESS 2

//*****************************************************************************
//
// The size of the message queue.  In order to make the queue pointer
// arithmetic more efficient, this should be a power of two.
//
//*****************************************************************************
#define QUEUE_SIZE              16

#ifdef DEBUG_MSGQ
//*****************************************************************************
//
// In debug builds, keep track of the number of cases where a message was
// lost due to the queue being full.  We count the following occurrences:
//
// 1. All messages discarded due to queue overflow (g_ui32MQOverflow)
// 2. Messages other than WIDGET_MSG_PTR_MOVE discarded due to queue
//    overflow (g_ui32MQNonMouseOverflow).  In this case, we also remember the
//    last message that was discarded (g_ui32MQLastLostMsg).
// 3. The number of calls to WidgetMessageQueueAdd that fail due to the queue
//    mutex already being held.
// 4. The number of cases where WidgetMessageQueueAdd reused an unread
//    WIDGET_MSG_PTR_MOVE message when a second one arrived before the previous
//    one had been processed.
//
//*****************************************************************************
uint32_t g_ui32MQOverflow = 0;
uint32_t g_ui32MQNonMouseOverflow = 0;
uint32_t g_ui32MQLastLostMsg = 0;
uint32_t g_ui32MQMutexClash = 0;
uint32_t g_ui32MQMoveOverwrite = 0;
#endif

//*****************************************************************************
//
// This structure describes the message queue used to hold widget messages.
//
//*****************************************************************************
typedef struct
{
    //
    // The flags that describe how this message should be processed; this is
    // defined by the MQ_FLAG_xxx flags.
    //
    uint32_t ui32Flags;

    //
    // The widget (or widget tree) to which the message should be sent.
    //
    tWidget *psWidget;

    //
    // The message to be sent.
    //
    uint32_t ui32Message;

    //
    // The first parameter to the message.
    //
    uint32_t ui32Param1;

    //
    // The second parameter to the message.
    //
    uint32_t ui32Param2;
}
tWidgetMessageQueue;

//*****************************************************************************
//
// The root of the widget tree.  This is the widget used when no parent is
// specified when adding a widget, or when no widget is specified when sending
// a message.  The parent and sibling of this widget are always zero.  This
// should not be directly referenced by applications; WIDGET_ROOT should be
// used instead.
//
//*****************************************************************************
tWidget g_sRoot =
{
    sizeof(tWidget),
    0,
    0,
    0,
    0,
    {
        0,
        0,
        0,
        0,
    },
    WidgetDefaultMsgProc
};

//*****************************************************************************
//
// The widget that has captured pointer messages.  When a pointer down message
// is accepted by a widget, that widget is saved in this variable and all
// subsequent pointer move and pointer up messages are sent directly to this
// widget.
//
//*****************************************************************************
static tWidget *g_psPointerWidget = 0;

//*****************************************************************************
//
// The message queue that holds messages that are waiting to be processed.
//
//*****************************************************************************
static volatile tWidgetMessageQueue g_psMQ[QUEUE_SIZE];

//*****************************************************************************
//
// The offset to the next message to be read from the message queue.  The
// message queue is empty when this has the same value as g_ui32MQWrite.
//
//*****************************************************************************
static uint32_t g_ui32MQRead = 0;

//*****************************************************************************
//
// The offset to the next message to be written to the message queue.  The
// message queue is full when this value is one less than g_ui32MQRead (modulo
// the queue size).
//
//*****************************************************************************
static volatile uint32_t g_ui32MQWrite = 0;

//*****************************************************************************
//
// The mutex used to protect access to the message queue.
//
//*****************************************************************************
static uint8_t g_ui8MQMutex = 0;

//*****************************************************************************
//
//! Initializes a mutex to the unowned state.
//!
//! \param pi8Mutex is a pointer to mutex that is to be initialized.
//!
//! This function initializes a mutual exclusion semaphore (mutex) to its
//! unowned state in preparation for use with WidgetMutexGet() and
//! WidgetMutexPut().  A mutex is a two state object typically used to
//! serialize access to a shared resource.  An application will call
//! WidgetMutexGet() to request ownership of the mutex.  If ownership is
//! granted, the caller may safely access the resource then release the mutex
//! using WidgetMutexPut() once it is finished.  If ownership is not granted,
//! the caller knows that some other context is currently modifying the shared
//! resource and it must not access the resource at that time.
//!
//! Note that this function must not be called if the mutex passed in
//! \e pi8Mutex is already in use since this will have the effect of releasing
//! the lock even if some caller currently owns it.
//!
//! \return None.
//
//*****************************************************************************
void
WidgetMutexInit(uint8_t *pi8Mutex)
{
    //
    // Catch NULL pointers in a debug build.
    //
    ASSERT(pi8Mutex);

    //
    // Clear the mutex location to set it to the unowned state.
    //
    *pi8Mutex = 0;
}

//*****************************************************************************
//
//! Attempts to acquire a mutex.
//!
//! \param pi8Mutex is a pointer to mutex that is to be acquired.
//!
//! This function attempts to acquire a mutual exclusion semaphore (mutex) on
//! behalf of the caller.  If the mutex is not already held, 0 is returned to
//! indicate that the caller may safely access whichever resource the mutex is
//! protecting.  If the mutex is already held, 1 is returned and the caller
//! must not access the shared resource.
//!
//! When access to the shared resource is complete, the mutex owner should call
//! WidgetMutexPut() to release the mutex and relinquish ownership of the
//! shared resource.
//!
//! \return Returns 0 if the mutex is acquired successfully or 1 if it is
//! already held by another caller.
//
//*****************************************************************************
#if defined(ewarm) || defined(DOXYGEN)
uint32_t
WidgetMutexGet(uint8_t *pi8Mutex)
{
    //
    // Acquire the mutex if possible.
    //
    __asm("    mov     r1, #1\n"
          "    ldrexb  r2, [r0]\n"
          "    cmp     r2, #0\n"
          "    it      eq\n"
          "    strexb  r2, r1, [r0]\n"
          "    mov     r0, r2\n");

    //
    // "Warning[Pe940]: missing return statement at end of non-void function"
    // is suppressed here to avoid putting a "bx lr" in the inline assembly
    // above and a superfluous return statement here.
    //
#pragma diag_suppress=Pe940
}
#pragma diag_default=Pe940
#endif
#if defined(codered) || defined(gcc) || defined(sourcerygxx)
uint32_t __attribute__((naked))
WidgetMutexGet(uint8_t *pi8Mutex)
{
    uint32_t ui32Ret;

    //
    // Acquire the mutex if possible.
    //
    __asm("    mov      r1, #1\n"
          "    ldrexb   r2, [r0]\n"
          "    cmp      r2, #0\n"
          "    it       eq\n"
          "    strexbeq r2, r1, [r0]\n"
          "    mov      r0, r2\n"
          "    bx       lr\n"
          : "=r" (ui32Ret));

    //
    // The return is handled in the inline assembly, but the compiler will
    // still complain if there is not an explicit return here (despite the fact
    // that this does not result in any code being produced because of the
    // naked attribute).
    //
    return(ui32Ret);
}
#endif
#if defined(rvmdk) || defined(__ARMCC_VERSION)
__asm uint32_t
WidgetMutexGet(uint8_t *pi8Mutex)
{
    mov         r1, #1
    ldrexb      r2, [r0]
    cmp         r2, #0
    it          eq
    strexbeq    r2, r1, [r0]
    mov         r0, r2
    bx          lr
}
#endif
//
// For CCS implement this function in pure assembly.  This prevents the TI
// compiler from doing funny things with the optimizer.
//
#if defined(ccs)
    __asm("    .sect \".text:WidgetMutexGet\"\n"
          "    .clink\n"
          "    .thumbfunc WidgetMutexGet\n"
          "    .thumb\n"
          "    .global WidgetMutexGet\n"
          "WidgetMutexGet:\n"
          "    mov         r1, #1\n"
          "    ldrexb      r2, [r0]\n"
          "    cmp         r2, #0\n"
          "    it          EQ\n" // TI assembler requires upper case cond
          "    strexbeq    r2, r1, [r0]\n"
          "    mov         r0, r2\n"
          "    bx          lr\n");
#endif


//*****************************************************************************
//
//! Release a mutex.
//!
//! \param pi8Mutex is a pointer to mutex that is to be released.
//!
//! This function releases a mutual exclusion semaphore (mutex), leaving it in
//! the unowned state.
//!
//! \return None.
//
//*****************************************************************************
void
WidgetMutexPut(uint8_t *pi8Mutex)
{
    //
    // Release the mutex.
    //
    *pi8Mutex = 0;
}

//*****************************************************************************
//
// Determines if a widget exists in the tree below a given point.
//
// \param psWidget is a pointer to the widget tree.
// \param psFind is a pointer to the widget that is being searched for.
//
// This function searches the widget tree below psWidget to determine whether
// or not the widget pointed to by \e psFind exists in the subtree.
//
// \return Returns \b true if \e psFind exists in the subtree or \b false if it
// does not.
//
//*****************************************************************************
static bool
WidgetIsInTree(tWidget *psWidget, tWidget *psFind)
{
    tWidget *psTemp;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);
    ASSERT(psFind);

    //
    // Loop through the tree under the widget until every widget is searched.
    //
    for(psTemp = psWidget; psTemp != psWidget->psParent; )
    {
        //
        // See if this widget has a child.
        //
        if(psTemp->psChild)
        {
            //
            // Go to this widget's child first.
            //
            psTemp = psTemp->psChild;
        }

        //
        // This widget does not have a child, so either a sibling or a parent
        // must be checked.  When moving back to the parent, another move must
        // be performed as well to avoid getting stuck in a loop (since the
        // parent's children have already been searched.
        //
        else
        {
            //
            // Loop until returning to the parent of the starting widget.  This
            // loop will be explicitly broken out of if an intervening widget
            // is encountered that has not been searched.
            //
            while(psTemp != psWidget->psParent)
            {
                if(psTemp == psFind)
                {
                    return(true);
                }

                //
                // See if this widget has a sibling.
                //
                if(psTemp->psNext)
                {
                    //
                    // Visit the sibling of this widget.
                    //
                    psTemp = psTemp->psNext;

                    //
                    // Since this widget has not been searched yet, break out
                    // of the controlling loop.
                    //
                    break;
                }
                else
                {
                    //
                    // This widget has no siblings, so go to its parent.  Since
                    // the parent has already been searched, the same sibling
                    // vs. parent decision must be made on this widget as well.
                    //
                    psTemp = psTemp->psParent;
                }
            }
        }
    }

    //
    // The widget could not be found.
    //
    return(false);
}

//*****************************************************************************
//
//! Handles widget messages.
//!
//! \param psWidget is a pointer to the widget.
//! \param ui32Message is the message to be processed.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function is a default handler for widget messages; it simply ignores
//! all messages sent to it.  This is used as the message handler for the root
//! widget, and should be called by the message handler for other widgets when
//! they do not explicitly handle the provided message (in case new messages
//! are added that require some default but override-able processing).
//!
//! \return Always returns 0.
//
//*****************************************************************************
int32_t
WidgetDefaultMsgProc(tWidget *psWidget, uint32_t ui32Message,
                     uint32_t ui32Param1, uint32_t ui32Param2)
{
    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Return zero for all messages.
    //
    return(0);
}

//*****************************************************************************
//
//! Adds a widget to the widget tree.
//!
//! \param psParent is the parent for the widget. To add to the root of the tree
//! set this parameter to \b WIDGET_ROOT.
//! \param psWidget is the widget to add.
//!
//! This function adds a widget to the widget tree at the given position within
//! the tree.  The widget will become the last child of its parent, and will
//! therefore be searched after the existing children.
//!
//! The added widget can be a full widget tree, allowing addition of an entire
//! heirarchy all at once (for example, adding an entire screen to the widget
//! tree all at once).  In this case, it is the responsibility of the caller to
//! ensure that the psParent field of each widget in the added tree is correctly
//! set (in other words, only the widget pointed to by \e psWidget is updated to
//! properly reside in the tree).
//!
//! It is the responsibility of the caller to initialize the psNext and psChild
//! field of the added widget; either of these fields being non-zero results in
//! a pre-defined tree of widgets being added instead of a single one.
//!
//! \return None.
//
//*****************************************************************************
void
WidgetAdd(tWidget *psParent, tWidget *psWidget)
{
    //
    // Check the arguments.
    //
    ASSERT(psParent);
    ASSERT(psWidget);

    //
    // Make this widget be a child of its parent.
    //
    psWidget->psParent = psParent;

    //
    // See if this parent already has children.
    //
    if(psParent->psChild)
    {
        //
        // Find the last child of this parent and also check that widget is not
        // already present at this level of the tree.
        //
        for(psParent = psParent->psChild; psParent->psNext;
            psParent = psParent->psNext)
        {
            //
            // If we find this widget here already, just return.  If we don't
            // do this, we allow errant programs to add the same child twice
            // resulting in looping on message processing.
            //
            if(psParent == psWidget)
            {
                return;
            }
        }

        //
        // We perform one final check to see if we are about to add the widget
        // twice.  We need this to catch the case of a single child which
        // causes the previous loop to exit before performing the widget check.
        //
        if(psParent == psWidget)
        {
            return;
        }

        //
        // Add this widget to the end of the list of children of this parent.
        //
        psParent->psNext = psWidget;
    }
    else
    {
        //
        // Make this widget be the first (and only) child of this parent.
        //
        psParent->psChild = psWidget;
    }
}

//*****************************************************************************
//
//! Removes a widget from the widget tree.
//!
//! \param psWidget is the widget to be removed.
//!
//! This function removes a widget from the widget tree.  The removed widget
//! can be a full widget tree, allowing removal of an entire heirarchy all at
//! once (for example, removing an entire screen from the widget tree).
//!
//! \return None.
//
//*****************************************************************************
void
WidgetRemove(tWidget *psWidget)
{
    tWidget *psTemp;

    //
    // Check the argument.
    //
    ASSERT(psWidget);

    //
    // Make sure that the supplied widget is actually in the tree section
    // owned by its parent and, hence, removeable.
    //
    if(!psWidget->psParent || !WidgetIsInTree(psWidget->psParent, psWidget))
    {
        return;
    }

    //
    // See if this widget is the first child of its parent.
    //
    if(psWidget->psParent->psChild == psWidget)
    {
        //
        // Make the first child of this widgets parent be this widget's
        // sibling.
        //
        psWidget->psParent->psChild = psWidget->psNext;
    }
    else
    {
        //
        // Find the sibling directly before this widget.
        //
        for(psTemp = psWidget->psParent->psChild; psTemp->psNext != psWidget;
            psTemp = psTemp->psNext)
        {
        }

        //
        // Make the previous sibling point to the next sibling, removing this
        // widget from the sibling chain.
        //
        psTemp->psNext = psWidget->psNext;
    }

    //
    // Check to see if the widget which currently owns the pointer has just
    // been removed and, if so, clear the pointer focus.
    //
    if(g_psPointerWidget && !WidgetIsInTree(&g_sRoot, g_psPointerWidget))
    {
        g_psPointerWidget = 0;
    }

    //
    // Clear the next pointer of the widget.
    //
    psWidget->psNext = 0;
}

//*****************************************************************************
//
//! Sends a message to a widget tree via a pre-order, depth-first search.
//!
//! \param psWidget is a pointer to the widget tree.
//! \param ui32Message is the message to send.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//! \param bStopOnSuccess is \b true if the search should be stopped when the
//! first widget is found that returns success in response to the message.
//!
//! This function performs a pre-order, depth-first search of the widget tree,
//! sending a message to each widget encountered.  In a depth-first search, the
//! children of a widget are searched before its siblings (preferring to go
//! deeper into the tree, hence the name depth-first).  A pre-order search
//! means that the message is sent to a widget before any of its children are
//! searched.
//!
//! An example use of the pre-order search is for paint messages; the larger
//! enclosing widgets should be drawn on the screen before the smaller widgets
//! that reside within the parent widget (otherwise, the children would be
//! overwritten by the parent).
//!
//! \return Returns 0 if \e bStopOnSuccess is false or no widget returned
//! success in response to the message, or the value returned by the first
//! widget to successfully process the message.
//
//*****************************************************************************
uint32_t
WidgetMessageSendPreOrder(tWidget *psWidget, uint32_t ui32Message,
                          uint32_t ui32Param1, uint32_t ui32Param2,
                          bool bStopOnSuccess)
{
    uint32_t ui32Ret;
    tWidget *psTemp;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Send the message to the initial widget and return if it succeeded and
    // the search should stop on success.
    //
    ui32Ret = psWidget->pfnMsgProc(psWidget, ui32Message, ui32Param1,
                                   ui32Param2);
    if((ui32Ret != 0) && bStopOnSuccess)
    {
        return(ui32Ret);
    }

    //
    // Return if the widget does not have any children.
    //
    if(!psWidget->psChild)
    {
        return(0);
    }

    //
    // Loop through the tree under the widget until every widget is searched.
    //
    for(psTemp = psWidget->psChild; psTemp != psWidget; )
    {
        //
        // Send the message to this widget and return if it succeeded and the
        // search should stop on success.
        //
        ui32Ret = psTemp->pfnMsgProc(psTemp, ui32Message, ui32Param1,
                                    ui32Param2);
        if((ui32Ret != 0) && bStopOnSuccess)
        {
            return(ui32Ret);
        }

        //
        // Find the next widget to examine.  If this widget has a child, then
        // that is the next widget to examine.
        //
        if(psTemp->psChild)
        {
            psTemp = psTemp->psChild;
        }

        //
        // This widget does not have a child, so either a sibling or a parent
        // must be checked.  When moving back to the parent, another move must
        // be performed as well to avoid getting stuck in a loop (since the
        // parent's children have already been searched).
        //
        else
        {
            //
            // Loop until returning to the starting widget.  This loop will be
            // explicitly broken out of if an intervening widget is encountered
            // that has not be searched.
            //
            while(psTemp != psWidget)
            {
                //
                // See if this widget has a sibling.
                //
                if(psTemp->psNext)
                {
                    //
                    // Visit the sibling of this widget.
                    //
                    psTemp = psTemp->psNext;

                    //
                    // Since this widget has not been searched yet, break out
                    // of the controlling loop.
                    //
                    break;
                }
                else
                {
                    //
                    // This widget has no siblings, so go to its parent.  Since
                    // the parent has already been searched, the same sibling
                    // vs. parent decision must be made on this widget as well.
                    //
                    psTemp = psTemp->psParent;
                }
            }
        }
    }

    //
    // No widget returned success for the message, or bStopOnSuccess was zero,
    // so return zero.
    //
    return(0);
}

//*****************************************************************************
//
//! Sends a message to a widget tree via a post-order, depth-first search.
//!
//! \param psWidget is a pointer to the widget tree; if this is zero then the
//! root of the widget tree willb e used.
//! \param ui32Message is the message to send.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//! \param bStopOnSuccess is \b true if the search should be stopped when the
//! first widget is found that returns success in response to the message.
//!
//! This function performs a post-order, depth-first search of the widget tree,
//! sending a message to each widget encountered.  In a depth-first search, the
//! children of a widget are searched before its sibling (preferring to go
//! deeper into the tree, hence the name depth-first).  A post-order search
//! means that the message is sent to a widget after all of its children are
//! searched.
//!
//! An example use of the post-order search is for pointer-related messages;
//! those messages should be delivered to the lowest widget in the tree before
//! its parents (in other words, the widget deepest in the tree that has a hit
//! should get the message, not the higher up widgets that also include the hit
//! location).
//!
//! Special handling is performed for pointer-related messages.  The widget
//! that accepts \b #WIDGET_MSG_PTR_DOWN is remembered and subsequent
//! \b #WIDGET_MSG_PTR_MOVE and \b #WIDGET_MSG_PTR_UP messages are sent
//! directly to that widget.
//!
//! \return Returns 0 if \e bStopOnSuccess is \b false or no widget returned
//! success in response to the message, or the value returned by the first
//! widget to successfully process the message.
//
//*****************************************************************************
uint32_t
WidgetMessageSendPostOrder(tWidget *psWidget, uint32_t ui32Message,
                           uint32_t ui32Param1, uint32_t ui32Param2,
                           bool bStopOnSuccess)
{
    uint32_t ui32Ret;
    tWidget *psTemp;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // See if this is a pointer move or up message.
    //
    if((ui32Message == WIDGET_MSG_PTR_MOVE) ||
       (ui32Message == WIDGET_MSG_PTR_UP))
    {
        //
        // If there is not a widget that has captured pointer messages, then
        // simply drop this message.
        //
        if(!g_psPointerWidget)
        {
            return(0);
        }

        //
        // Send the message directly to the widget that has captured pointer
        // messages.
        //
        ui32Ret = g_psPointerWidget->pfnMsgProc(g_psPointerWidget, ui32Message,
                                                ui32Param1, ui32Param2);

        //
        // See if this is a pointer up message.
        //
        if(ui32Message == WIDGET_MSG_PTR_UP)
        {
            //
            // Since this was a pointer up, the widget no longer has pointer
            // messages captured.
            //
            g_psPointerWidget = 0;
        }

        //
        // Return the value returned by the pointer capture widget.
        //
        return(ui32Ret);
    }

    //
    // Loop through the tree under the widget until every widget is searched.
    //
    for(psTemp = psWidget; psTemp != psWidget->psParent; )
    {
        //
        // See if this widget has a child.
        //
        if(psTemp->psChild)
        {
            //
            // Go to this widget's child first.
            //
            psTemp = psTemp->psChild;
        }

        //
        // This widget does not have a child, so either a sibling or a parent
        // must be checked.  When moving back to the parent, another move must
        // be performed as well to avoid getting stuck in a loop (since the
        // parent's children have already been searched.
        //
        else
        {
            //
            // Loop until returning to the parent of the starting widget.  This
            // loop will be explicitly broken out of if an intervening widget
            // is encountered that has not been searched.
            //
            while(psTemp != psWidget->psParent)
            {
                //
                // Send the message to this widget.
                //
                ui32Ret = psTemp->pfnMsgProc(psTemp, ui32Message, ui32Param1,
                                             ui32Param2);

                //
                // If this is a pointer down message, the widget accepted the
                // message and the handler didn't modify the tree such that
                // this widget is no longer present, then save a pointer to the
                // widget for subsequent pointer move or pointer up messages.
                //
                if((ui32Message == WIDGET_MSG_PTR_DOWN) && (ui32Ret != 0))
                {
                    //
                    // Is the current widget still in the tree?
                    //
                    if(WidgetIsInTree(&g_sRoot, psTemp))
                    {
                        //
                        // The widget is still in the tree so save it for later
                        // use.
                        //
                        g_psPointerWidget = psTemp;
                    }
                    else
                    {
                        //
                        // Although this widget handled the PTR_DOWN message,
                        // it's message handler rearranged the widget tree and
                        // removed itself so we don't want to send any more
                        // messages directly to it after all.
                        //
                        g_psPointerWidget = 0;
                    }
                }

                //
                // If the widget returned success and the search should stop on
                // success then return immediately.
                //
                if((ui32Ret != 0) && bStopOnSuccess)
                {
                    return(ui32Ret);
                }

                //
                // See if this widget has a sibling.
                //
                if(psTemp->psNext)
                {
                    //
                    // Visit the sibling of this widget.
                    //
                    psTemp = psTemp->psNext;

                    //
                    // Since this widget has not been searched yet, break out
                    // of the controlling loop.
                    //
                    break;
                }
                else
                {
                    //
                    // This widget has no siblings, so go to its parent.  Since
                    // the parent has already been searched, the same sibling
                    // vs. parent decision must be made on this widget as well.
                    //
                    psTemp = psTemp->psParent;
                }
            }
        }
    }

    //
    // No widget returned success for the message, or bStopOnSuccess was zero,
    // so return zero.
    //
    return(0);
}

//*****************************************************************************
//
//! Adds a message to the widget message queue.
//!
//! \param psWidget is the widget to which the message should be sent.
//! \param ui32Message is the message to be sent.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//! \param bPostOrder is \b true if the message should be sent via a post-order
//! search, and \b false if it should be sent via a pre-order search.
//! \param bStopOnSuccess is \b true if the message should be sent to widgets
//! until one returns success, and \b false if it should be sent to all
//! widgets.
//!
//! This function places a widget message into the message queue for later
//! processing.  The messages are removed from the queue by
//! WidgetMessageQueueProcess() and sent to the appropriate place.
//!
//! It is safe for code which interrupts WidgetMessageQueueProcess() (or called
//! by it) to call this function to send a message.  It is not safe for code
//! which interrupts this function to call this function as well; it is up to
//! the caller to guarantee that the later sequence never occurs.
//!
//! \return Returns 1 if the message was added to the queue, and 0 if it could
//! not be added since either the queue is full or another context is currently
//! adding a message to the queue.
//
//*****************************************************************************
int32_t
WidgetMessageQueueAdd(tWidget *psWidget, uint32_t ui32Message,
                      uint32_t ui32Param1, uint32_t ui32Param2,
                      bool bPostOrder, bool bStopOnSuccess)
{
    uint32_t ui32Next;
    uint32_t ui32Owned;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Get the mutex we use to protect access to the message queue.
    //
    ui32Owned = WidgetMutexGet(&g_ui8MQMutex);
    if(ui32Owned)
    {
        //
        // The mutex is already being held by some other caller so return a
        // failure.
        //
#ifdef DEBUG_MSGQ
        g_ui32MQMutexClash++;
#endif
        return(0);
    }

    //
    // Compute the next value for the write pointer.
    //
    ui32Next = (g_ui32MQWrite + 1) % QUEUE_SIZE;

    //
    // If the queue is not empty, and this is a pointer move message, see if
    // the previous message was also a move and, if so, replace the
    // coordinates.  Without this, the message queue can very quickly overflow
    // if the application is busy doing something while the user keeps pressing
    // the display.
    //
    if(ui32Message == WIDGET_MSG_PTR_MOVE)
    {
        //
        // Is the message queue empty?
        //
        if(g_ui32MQRead != g_ui32MQWrite)
        {
            //
            // No - what is the index of the previous message?
            //
            ui32Owned = (g_ui32MQWrite == 0) ? (QUEUE_SIZE - 1) :
                                               (g_ui32MQWrite - 1);

            //
            // Was this a pointer move message?
            //
            if(g_psMQ[g_ui32MQWrite].ui32Message == WIDGET_MSG_PTR_MOVE)
            {
                //
                // Yes - overwrite this message with the new
                // coordinate information.
                //
                g_psMQ[ui32Owned].ui32Param1 = ui32Param1;
                g_psMQ[ui32Owned].ui32Param2 = ui32Param2;
#ifdef DEBUG_MSGQ
                g_ui32MQMoveOverwrite++;
#endif

                //
                // Release the message queue mutex.
                //
                WidgetMutexPut(&g_ui8MQMutex);

                //
                // Success.
                //
                return(1);
            }
        }
    }

    //
    // Return a failure if the message queue is full.
    //
    if(ui32Next == g_ui32MQRead)
    {
#ifdef DEBUG_MSGQ
        g_ui32MQOverflow++;
        if(ui32Message != WIDGET_MSG_PTR_MOVE)
        {
            g_ui32MQNonMouseOverflow++;
            g_ui32MQLastLostMsg = ui32Message;
        }
#endif
        //
        // Release the message queue mutex.
        //
        WidgetMutexPut(&g_ui8MQMutex);

        return(0);
    }

    //
    // Write this message into the next location in the message queue.
    //
    g_psMQ[g_ui32MQWrite].ui32Flags = ((bPostOrder ? MQ_FLAG_POST_ORDER : 0) |
                                  (bStopOnSuccess ? MQ_FLAG_STOP_ON_SUCCESS :
                                   0));
    g_psMQ[g_ui32MQWrite].psWidget = psWidget;
    g_psMQ[g_ui32MQWrite].ui32Message = ui32Message;
    g_psMQ[g_ui32MQWrite].ui32Param1 = ui32Param1;
    g_psMQ[g_ui32MQWrite].ui32Param2 = ui32Param2;

    //
    // Update the message queue write pointer.
    //
    g_ui32MQWrite = ui32Next;

    //
    // Release the message queue mutex.
    //
    WidgetMutexPut(&g_ui8MQMutex);

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
//! Processes the messages in the widget message queue.
//!
//! This function extracts messages from the widget message queue one at a time
//! and processes them.  If the processing of a widget message requires that a
//! new message be sent, it is acceptable to call WidgetMessageQueueAdd().  It
//! is also acceptable for code which interrupts this function to call
//! WidgetMessageQueueAdd() to send more messages.  In both cases, the newly
//! added message will also be processed before this function returns.
//!
//! \return None.
//
//*****************************************************************************
void
WidgetMessageQueueProcess(void)
{
    tWidget *psWidget;
    uint32_t ui32Flags, ui32Message, ui32Param1, ui32Param2;

    //
    // Loop while there are more messages in the message queue.
    //
    while(g_ui32MQRead != g_ui32MQWrite)
    {
        //
        // Copy the contents of this message into local variables.
        //
        psWidget = g_psMQ[g_ui32MQRead].psWidget;
        ui32Flags = g_psMQ[g_ui32MQRead].ui32Flags;
        ui32Message = g_psMQ[g_ui32MQRead].ui32Message;
        ui32Param1 = g_psMQ[g_ui32MQRead].ui32Param1;
        ui32Param2 = g_psMQ[g_ui32MQRead].ui32Param2;

        //
        // Remove this message from the queue.
        //
        g_ui32MQRead = (g_ui32MQRead + 1) % QUEUE_SIZE;

        //
        // See if this message should be sent via a post-order or pre-order
        // search.
        //
        if(ui32Flags & MQ_FLAG_POST_ORDER)
        {
            //
            // Send this message with a post-order search of the widget tree.
            //
            WidgetMessageSendPostOrder(psWidget, ui32Message, ui32Param1,
                                       ui32Param2,
                                       ((ui32Flags & MQ_FLAG_STOP_ON_SUCCESS) ?
                                        true : false));
        }
        else
        {
            //
            // Send this message with a pre-order search of the widget tree.
            //
            WidgetMessageSendPreOrder(psWidget, ui32Message, ui32Param1,
                                      ui32Param2,
                                      ((ui32Flags & MQ_FLAG_STOP_ON_SUCCESS) ?
                                       true : false));
        }
    }
}

//*****************************************************************************
//
//! Sends a pointer message.
//!
//! \param ui32Message is the pointer message to be sent.
//! \param i32X is the X coordinate associated with the message.
//! \param i32Y is the Y coordinate associated with the message.
//!
//! This function sends a pointer message to the root widget.  A pointer driver
//! (such as a touch screen driver) can use this function to deliver pointer
//! activity to the widget tree without having to have direct knowledge of the
//! structure of the widget framework.
//!
//! \return Returns 1 if the message was added to the queue, and 0 if it could
//! not be added since the queue is full.
//
//*****************************************************************************
int32_t
WidgetPointerMessage(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    //
    // Add the message to the widget message queue.
    //
    return(WidgetMessageQueueAdd(WIDGET_ROOT, ui32Message, i32X, i32Y, true,
                                 true));
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
