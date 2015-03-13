//*****************************************************************************
//
// widget.h - Prototypes for the widget base "class".
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

#ifndef __WIDGET_H__
#define __WIDGET_H__

//*****************************************************************************
//
//! \addtogroup widget_api
//! @{
//
//*****************************************************************************

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
//! The structure that describes a generic widget.  This structure is the base
//! ``class'' for all other widgets.
//
//*****************************************************************************
typedef struct __Widget
{
    //
    //! The size of this structure.  This will be the size of the full
    //! structure, not just the generic widget subset.
    //
    int32_t i32Size;

    //
    //! A pointer to this widget's parent widget.
    //
    struct __Widget *psParent;

    //
    //! A pointer to this widget's first sibling widget.
    //
    struct __Widget *psNext;

    //
    //! A pointer to this widget's first child widget.
    //
    struct __Widget *psChild;

    //
    //! A pointer to the display on which this widget resides.
    //
    const tDisplay *psDisplay;

    //
    //! The rectangle that encloses this widget.
    //
    tRectangle sPosition;

    //
    //! The procedure that handles messages sent to this widget.
    //
    int32_t (*pfnMsgProc)(struct __Widget *psWidget, uint32_t ui32Message,
                          uint32_t ui32Param1, uint32_t ui32Param2);
}
tWidget;

//*****************************************************************************
//
//! The widget at the root of the widget tree.  This can be used when
//! constructing a widget tree at compile time (used as the psParent argument to
//! a widget declaration) or as the psWidget argument to an API (such as
//! WidgetPaint() to paint the entire widget tree).
//
//*****************************************************************************
#define WIDGET_ROOT             &g_sRoot

//*****************************************************************************
//
//! This message is sent to indicate that the widget should draw itself on the
//! display.  Neither \e ui32Param1 nor \e ui32Param2 are used by this message.
//! This message is delivered in top-down order.
//
//*****************************************************************************
#define WIDGET_MSG_PAINT        0x00000001

//*****************************************************************************
//
//! This message is sent to indicate that the pointer is now down.
//! \e ui32Param1 is the X coordinate of the location where the pointer down
//! event occurred, and \e ui32Param2 is the Y coordinate.  This message is
//! delivered in bottom-up order.
//
//*****************************************************************************
#define WIDGET_MSG_PTR_DOWN     0x00000002

//*****************************************************************************
//
//! This message is sent to indicate that the pointer has moved while being
//! down.  \e ui32Param1 is the X coordinate of the new pointer location, and
//! \e ui32Param2 is the Y coordinate.  This message is delivered in bottom-up
//! order.
//
//*****************************************************************************
#define WIDGET_MSG_PTR_MOVE     0x00000003

//*****************************************************************************
//
//! This message is sent to indicate that the pointer is now up.  \e ui32Param1
//! is the X coordinate of the location where the pointer up event occurred,
//! and \e ui32Param2 is the Y coordinate.  This message is delivered in
//! bottom-up order.
//
//*****************************************************************************
#define WIDGET_MSG_PTR_UP       0x00000004

//*****************************************************************************
//
//! This message is sent by the application to indicate that there has been a
//! key press or button press meaning "up".  \e ui32Param1 by convention is a
//! pointer to the widget that is the intended recipient of the key press.
//! This is controlled by the application.
//
//*****************************************************************************
#define WIDGET_MSG_KEY_UP       0x00000005

//*****************************************************************************
//
//! This message is sent by the application to indicate that there has been a
//! key press or button press meaning "down".  \e ui32Param1 by convention is a
//! pointer to the widget that is the intended recipient of the key press.
//! This is controlled by the application.
//
//*****************************************************************************
#define WIDGET_MSG_KEY_DOWN     0x00000006

//*****************************************************************************
//
//! This message is sent by the application to indicate that there has been a
//! key press or button press meaning "left".  \e ui32Param1 by convention is a
//! pointer to the widget that is the intended recipient of the key press.
//! This is controlled by the application.
//
//*****************************************************************************
#define WIDGET_MSG_KEY_LEFT     0x00000007

//*****************************************************************************
//
//! This message is sent by the application to indicate that there has been a
//! key press or button press meaning "right".  \e ui32Param1 by convention is a
//! pointer to the widget that is the intended recipient of the key press.
//! This is controlled by the application.
//
//*****************************************************************************
#define WIDGET_MSG_KEY_RIGHT    0x00000008

//*****************************************************************************
//
//! This message is sent by the application to indicate that there has been a
//! key press or button press meaning "select".  \e ui32Param1 by convention is
//! a pointer to the widget that is the intended recipient of the key press.
//! This is controlled by the application.
//
//*****************************************************************************
#define WIDGET_MSG_KEY_SELECT   0x00000009

//*****************************************************************************
//
//! Requests a redraw of the widget tree.
//!
//! \param psWidget is a pointer to the widget tree to paint.
//!
//! This function sends a \b #WIDGET_MSG_PAINT message to the given widgets,
//! and all of the widget beneath it, so that they will draw or redraw
//! themselves on the display.  The actual drawing will occur when this message
//! is retrieved from the message queue and processed.
//!
//! \return Returns 1 if the message was added to the message queue and 0 if it
//! cound not be added (due to the queue being full).
//
//*****************************************************************************
#define WidgetPaint(psWidget)                                                 \
        WidgetMessageQueueAdd(psWidget, WIDGET_MSG_PAINT, 0, 0, false, false)

//*****************************************************************************
//
// Prototypes for the generic widget handling functions.
//
//*****************************************************************************
extern tWidget g_sRoot;
extern int32_t WidgetDefaultMsgProc(tWidget *psWidget, uint32_t ui32Message,
                                    uint32_t ui32Param1, uint32_t ui32Param2);
extern void WidgetAdd(tWidget *psParent, tWidget *psWidget);
extern void WidgetRemove(tWidget *psWidget);
extern uint32_t WidgetMessageSendPreOrder(tWidget *psWidget,
                                          uint32_t ui32Message,
                                          uint32_t ui32Param1,
                                          uint32_t ui32Param2,
                                          bool bStopOnSuccess);
extern uint32_t WidgetMessageSendPostOrder(tWidget *psWidget,
                                           uint32_t ui32Message,
                                           uint32_t ui32Param1,
                                           uint32_t ui32Param2,
                                           bool bStopOnSuccess);
extern int32_t WidgetMessageQueueAdd(tWidget *psWidget, uint32_t ui32Message,
                                     uint32_t ui32Param1,
                                     uint32_t ui32Param2,
                                     bool bPostOrder,
                                     bool bStopOnSuccess);
extern void WidgetMessageQueueProcess(void);
extern int32_t WidgetPointerMessage(uint32_t ui32Message, int32_t i32X,
                                    int32_t i32Y);
extern void WidgetMutexInit(uint8_t *pi8Mutex);
extern uint32_t WidgetMutexGet(uint8_t *pi8Mutex);
extern void WidgetMutexPut(uint8_t *pi8Mutex);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif // __WIDGET_H__
