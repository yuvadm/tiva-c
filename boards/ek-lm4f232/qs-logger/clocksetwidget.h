//*****************************************************************************
//
// clocksetwidget.h - Prototypes for a widget to set a clock date/time.
//
// Copyright (c) 2011-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#ifndef __CLOCKSETWIDGET_H__
#define __CLOCKSETWIDGET_H__

//*****************************************************************************
//
//! \addtogroup clocksetwidget_api
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
//! A structure that represents a clock setting widget.
//
//*****************************************************************************
typedef struct _ClockSetWidget
{
    //
    //! The generic widget information.
    //
    tWidget sBase;

    //
    //! The font to use for drawing text on the widget.
    //
    const tFont *psFont;

    //
    //! The foreground color of the widget.  This is the color that will be
    //! used for drawing text and lines, and will also be used as the highlight
    //! color for a selected field on the widget.
    //
    uint32_t ui32ForegroundColor;

    //
    //! The background color of the widget.
    //
    uint32_t ui32BackgroundColor;

    //
    //! An index for the date/time field that is highlighted
    //
    uint32_t ui32Highlight;

    //
    //! A pointer to a time structure that is used for showing and editing
    //! the date and time.  The application should supply the storage for this
    //! structure, and this widget will modify it as the user changes the
    //! date/time.
    //
    struct tm *psTime;

    //
    //! A pointer to the function to be called when the OK or cancel button is
    //! selected.  The OK button is used to indicate the user is done setting
    //! the time.  The CANCEL button is used to indicate that the user does
    //! not want to update the time.  The flag bOk is true if the OK button
    //! was selected, false otherwise.  The callback function can be used by
    //! the application to detect when the clock setting widget can be removed
    //! from the screen and whether or not to update the time.
    //
    void (*pfnOnOkClick)(tWidget *psWidget, bool bOk);
} tClockSetWidget;

//*****************************************************************************
//
//! Declares an initialized strip chart widget data structure.
//!
//! \param psParent is a pointer to the parent widget.
//! \param psNext is a pointer to the sibling widget.
//! \param psChild is a pointer to the first child widget.
//! \param psDisplay is a pointer to the off-screen display on which to draw.
//! \param i32X is the X coordinate of the upper left corner of the canvas.
//! \param i32Y is the Y coordinate of the upper left corner of the canvas.
//! \param i32Width is the width of the canvas.
//! \param i32Height is the height of the canvas.
//! \param psFont is the font used for rendering text on the widget.
//! \param ui32ForegroundColor is the foreground color for the widget.
//! \param ui32BackgroundColor is the background color for the chart.
//! \param psTime is a pointer to storage for the time structure that the
//! widget will modify.
//! \param pfnOnOkClick is a callback function that is called when the user
//! selects the OK or CANCEL button on the widget.
//!
//! This macro provides an initialized clock setting widget data structure,
//! which can be used to construct the widget tree at compile time in global
//! variables (as opposed to run-time via function calls).  This must be
//! assigned to a variable, such as:
//!
//! \verbatim
//!     tClockSetWidget g_sClockSetter = ClockSetStruct(...);
//! \endverbatim
//!
//! Or, in an array of variables:
//!
//! \verbatim
//!     tClockSetWidget g_psClockSetters[] =
//!     {
//!         ClockSetStruct(...),
//!         ClockSetStruct(...)
//!     };
//! \endverbatim
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define ClockSetStruct(psParent, psNext, psChild, psDisplay, i32X, i32Y,      \
                       i32Width, i32Height, psFont, ui32ForegroundColor,      \
                       ui32BackgroundColor, psTime, pfnOnOkClick)             \
        {                                                                     \
            {                                                                 \
                sizeof(tClockSetWidget),                                      \
                (tWidget *)(psParent),                                        \
                (tWidget *)(psNext),                                          \
                (tWidget *)(psChild),                                         \
                psDisplay,                                                    \
                {                                                             \
                    i32X,                                                     \
                    i32Y,                                                     \
                    (i32X) + (i32Width) - 1,                                  \
                    (i32Y) + (i32Height) - 1                                  \
                },                                                            \
                ClockSetMsgProc                                               \
            },                                                                \
            psFont, ui32ForegroundColor, ui32BackgroundColor, 6, psTime,      \
            pfnOnOkClick                                                      \
        }

//*****************************************************************************
//
//! Declares an initialized variable containing a clock setting widget data
//! structure.
//!
//! \param sName is the name of the variable to be declared.
//! \param psParent is a pointer to the parent widget.
//! \param psNext is a pointer to the sibling widget.
//! \param psChild is a pointer to the first child widget.
//! \param psDisplay is a pointer to the off-screen display on which to draw.
//! \param i32X is the X coordinate of the upper left corner of the canvas.
//! \param i32Y is the Y coordinate of the upper left corner of the canvas.
//! \param i32Width is the width of the canvas.
//! \param i32Height is the height of the canvas.
//! \param psFont is the font used for rendering text on the widget.
//! \param ui32ForegroundColor is the foreground color for the widget.
//! \param ui32BackgroundColor is the background color for the chart.
//! \param psTime is a pointer to storage for the time structure that the
//! widget will modify.
//! \param pfnOnOkClick is a callback function that is called when the user
//! selects the OK or CANCEL button on the widget.
//!
//! This macro declares a variable containing an initialized clock setting
//! widget data structure, which can be used to construct the widget tree at
//! compile time in global variables (as opposed to run-time via function
//! calls).
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define ClockSet(sName, psParent, psNext, psChild, psDisplay, i32X, i32Y,     \
                 i32Width, i32Height, psFont, ui32ForegroundColor,            \
                 ui32BackgroundColor, psTime, pfnOnOkClick)                   \
        tClockSetWidget sName = ClockSetStruct(psParent, psNext, psChild,     \
                                               psDisplay, i32X, i32Y,         \
                                               i32Width, i32Height, psFont,   \
                                               ui32ForegroundColor,           \
                                               ui32BackgroundColor, psTime,   \
                                               pfnOnOkClick)

//*****************************************************************************
//
//! Sets the pointer to the time structure for the clock set widget.
//!
//! \param psClockSetWidget is a pointer to the clock set widget to modify.
//! \param psTime is a pointer to the a time structure to use for clock
//! setting.
//!
//! This function sets the time structure used by the widget.
//!
//! \return None.
//
//*****************************************************************************
#define ClockSetTimePtrSet(psClockSetWidget, psTime)                          \
        do                                                                    \
        {                                                                     \
            (pClockSetWidget)->psTime = (psTime);                             \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the callback function to be used when OK or CANCEL is selected.
//!
//! \param pClockSetWidget is a pointer to the clock set widget to modify.
//! \param pfnCB is a pointer to function to call when OK is selected by the
//! user.
//!
//! This function sets the OK click callback function used by the widget.
//!
//! \return None.
//
//*****************************************************************************
#define ClockSetCallbackSet(pClockSetWidget, pfnCB)                           \
    do                                                                        \
    {                                                                         \
        (pClockSetWidget)->pfnOnOkClick = (pfnCB);                            \
    } while(0)

//*****************************************************************************
//
// Prototypes for the clock set widget APIs.
//
//*****************************************************************************
extern void ClockSetInit(tClockSetWidget *psWidget, const tDisplay *psDisplay,
                         int32_t i32X, int32_t i32Y, int32_t i32Width,
                         int32_t i32Height, tFont *psFont,
                         uint32_t ui32ForegroundColor,
                         uint32_t ui32BackgroundColor, struct tm *psTime,
                         void (*pfnOnOkClick)(tWidget *psWidget, bool bOk));
extern int32_t ClockSetMsgProc(tWidget *psWidget, uint32_t ui32Msg,
                               uint32_t ui32Param1, uint32_t ui32Param2);

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

#endif // __CLOCKSETWIDGET_H__
