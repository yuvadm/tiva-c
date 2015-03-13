//*****************************************************************************
//
// keyboard.h -
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
// This is part of revision 2.1.0.12573 of the Tiva Graphics Library.
//
//*****************************************************************************

#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

//*****************************************************************************
//
//! \addtogroup keyboard_api
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
//! The structure to describe a image based key on the keyboard.
//
//*****************************************************************************
typedef struct
{
    //
    // ! The Unicode value for this key.
    //
    uint32_t ui32Code;

    //
    // ! The width as a percentage in units of 1000.
    //
    uint16_t ui16Width;

    //
    // ! The height as a percentage in units of 1000.
    //
    uint16_t ui16Height;

    //
    // ! The X position as a percentage in units of 1000.
    //
    uint16_t ui16XPos;

    //
    // ! The Y position as a percentage in units of 1000.
    //
    uint16_t ui16YPos;

    //
    //! A pointer to the image to be drawn onto this key, if
    //! KEYBOARD_STYLE_IMG is selected.
    //
    const uint8_t *pui8Image;

    //
    //! A pointer to the image to be drawn onto this key when it is pressed, if
    //! KEYBOARD_STYLE_IMG is selected.
    //
    const uint8_t *pui8PressImage;
}
tKeyImage;

//*****************************************************************************
//
//! The structure to describe a text based key on the keyboard.
//
//*****************************************************************************
typedef struct
{
    //
    // ! The Unicode value for this key.
    //
    uint32_t ui32Code;

    //
    // ! The height as a percentage in units of 1000.
    //
    uint16_t ui16Width;

    //
    // ! The width as a percentage in units of 1000.
    //
    uint16_t ui16Height;

    //
    // ! The X position as a percentage in units of 1000.
    //
    uint16_t ui16XPos;

    //
    // ! The Y position as a percentage in units of 1000.
    //
    uint16_t ui16YPos;
}
tKeyText;

//*****************************************************************************
//
// Special Unicode values used by the keyboard
//
//*****************************************************************************

//*****************************************************************************
//
//! This code is used to map a backspace key onto a keyboard.  This is used
//! in the tKeyText.ui32Code or tKeyImage.ui32Code values.
//
//*****************************************************************************
#define UNICODE_BACKSPACE       0x00000008

//*****************************************************************************
//
//! This code is used to map a return/enter key onto a keyboard.  This is used
//! in the tKeyText.ui32Code or tKeyImage.ui32Code values.
//
//*****************************************************************************
#define UNICODE_RETURN          0x0000000D

//*****************************************************************************
//
//! This code is used to map a shift/caps-lock key onto a keyboard.  This value
//! causes the keyboard to toggle between lower-case, upper-case and caps lock
//! modes.  This value is used in the tKeyText.ui32Code or tKeyImage.ui32Code
//! values.
//
//*****************************************************************************
#define UNICODE_CUSTOM_SHIFT    0x000f0000

//*****************************************************************************
//
//! This code is used to map a mode toggle key onto a keyboard.  This value
//! causes the keyboard to toggle between the custom entries in a keyboard.
//! This value is used in the tKeyText.ui32Code or tKeyImage.ui32Code
//! values.
//
//*****************************************************************************
#define UNICODE_CUSTOM_MODE_TOG 0x000f0001

//*****************************************************************************
//
//! This code is used to identify a keyboard as the upper-case keyboard.
//
//*****************************************************************************
#define UNICODE_CUSTOM_UPCASE   0x000f0002

//*****************************************************************************
//
//! This code is used to identify a keyboard as the lower-case keyboard.
//
//*****************************************************************************
#define UNICODE_CUSTOM_LOWCASE  0x000f0003

//*****************************************************************************
//
//! This code is used to identify a keyboard as the lower-case keyboard.
//
//*****************************************************************************
#define UNICODE_CUSTOM_NUMERIC  0x000f0004

//*****************************************************************************
//
//! This code is used to identify the first custom keyboard entry.
//
//*****************************************************************************
#define UNICODE_CUSTOM_KBD      0x000f0005

//*****************************************************************************
//
// Keyboard events that are passed to the pfnOnEvent function.
//
//*****************************************************************************
#define KEYBOARD_EVENT_PRESS    0x00000001
#define KEYBOARD_EVENT_RELEASE  0x00000002

//*****************************************************************************
//
//! This structure holds a single keyboard entry.  Keyboards are typically
//! made up of an array of these structures.
//
//*****************************************************************************
typedef struct
{
    //
    //! This value holds the identifier for this keyboard.
    //
    uint32_t ui32Code;

    //
    //! This value holds the total number of keys for this keyboard entry.
    //
    uint16_t ui16NumKeys;

    //
    //! This value holds the static flag entries for this keyboard entry.
    //
    uint16_t ui16Flags;

    //
    //! This union holds either the text based keys or image based keys for
    //! this keyboard.
    //
    union
    {
        const tKeyImage *psKeysImage;
        const tKeyText *psKeysText;
    }
    uKeys;
}
tKeyboard;

//*****************************************************************************
//
//! The structure that describes a keyboard widget.
//
//*****************************************************************************
typedef struct
{
    //
    //! The generic widget information.
    //
    tWidget sBase;

    //
    //! The style for this widget.  This is a set of flags defined by
    //! KEYBOARD_STYLE_xxx.
    //
    uint32_t ui32Style;

    //
    //! The 24-bit RGB color used to fill background of the on-screen
    //! keyboard if KEYBOARD_STYLE_BG is selected.
    //
    uint32_t ui32BackgroundColor;

    //
    //! The 24-bit RGB color used to fill keys of the on-screen
    //! keyboard if KEYBOARD_STYLE_FILL is selected, and to use as the
    //! background color if KEYBOARD_STYLE_TEXT_OPAQUE is selected.
    //
    uint32_t ui32FillColor;

    //
    //! The 24-bit RGB color used to fill keys when pressed, if
    //! KEYBOARD_STYLE_FILL is selected, and to use as the background color
    //! if KEYBOARD_STYLE_TEXT_OPAQUE is selected.
    //
    uint32_t ui32PressFillColor;

    //
    //! The 24-bit RGB color used to outline the keys, if
    //! KEYBOARD_STYLE_OUTLINE is selected.
    //
    uint32_t ui32OutlineColor;

    //
    //! The 24-bit RGB color used to draw text on the keys.
    //
    uint32_t ui32TextColor;

    //
    //! A pointer to the font used to render the text on the keys.
    //
    const tFont *psFont;

    //
    //! The number of pointer events to delay before starting to auto-repeat,
    //! if KEYBOARD_STYLE_AUTO_REPEAT is selected.  The amount of time to which
    //! this corresponds is dependent upon the rate at which pointer events are
    //! generated by the pointer driver.
    //
    uint16_t ui16AutoRepeatDelay;

    //
    //! The number of pointer events between key presses generated by the
    //! auto-repeat function, if KEYBOARD_STYLE_AUTO_REPEAT is selected.  The
    //! amount of time to which this corresponds is dependent up on the rate at
    //! which pointer events are generated by the pointer driver.
    //
    uint16_t ui16AutoRepeatRate;

    //
    //! The number of pointer events that have occurred.  This is used when
    //! KEYBOARD_STYLE_AUTO_REPEAT is selected to generate the auto-repeat
    //! events.
    //
    uint32_t ui32AutoRepeatCount;

    //
    //! The active keyboard index, which should be initialized to 0.
    //
    uint32_t ui32Active;

    //
    //! The total number of active keyboards in the psKeyboards structure
    //! member.
    //
    uint32_t ui32NumKeyboards;

    //
    //! The array of keyboards used by the application.
    //
    const tKeyboard *psKeyboards;

    //
    //! A pointer to the function to be called when a key is pressed.
    //! This is repeatedly called when KEYBOARD_STYLE_AUTO_REPEAT is selected.
    //
    void (*pfnOnEvent)(tWidget *psWidget, uint32_t ui32Key,
                       uint32_t ui32Event);

    //
    //! The active key being pressed.
    //
    uint32_t ui32KeyPressed;

    //
    //! Internal state flags for the keyboard.
    //
    uint32_t ui32Flags;
}
tKeyboardWidget;

//*****************************************************************************
//
//! This flag indicates that the keys should be outlined.
//
//*****************************************************************************
#define KEYBOARD_STYLE_OUTLINE  0x00000001

//*****************************************************************************
//
//! This flag indicates that the keys should be filled.
//
//*****************************************************************************
#define KEYBOARD_STYLE_FILL     0x00000002

//*****************************************************************************
//
//! This flag indicates that the keys should have text drawn on them.
//
//*****************************************************************************
#define KEYBOARD_STYLE_TEXT     0x00000004

//*****************************************************************************
//
//! This flag indicates that the keys should have an image drawn on them.
//
//*****************************************************************************
#define KEYBOARD_STYLE_IMG      0x00000008

//*****************************************************************************
//
//! This flag indicates that the text on the keys should be drawn opaque (in
//! other words, drawing the background pixels as well as the foreground
//! pixels).
//
//*****************************************************************************
#define KEYBOARD_STYLE_TEXT_OPAQUE                                            \
                                0x00000010

//*****************************************************************************
//
//! This flag indicates that the keys should auto-repeat, generating
//! repeated click events while it is pressed.
//
//*****************************************************************************
#define KEYBOARD_STYLE_AUTO_REPEAT                                            \
                                0x00000020

//*****************************************************************************
//
//! This flag indicates that a key is pressed.
//
//*****************************************************************************
#define KEYBOARD_STYLE_PRESS_NOTIFY                                           \
                                0x00000040

//*****************************************************************************
//
//! This flag indicates that the key press callback should be made when
//! the key is released rather than when it is pressed.  This does not
//! affect the operation of auto repeat keys.
//
//*****************************************************************************
#define KEYBOARD_STYLE_RELEASE_NOTIFY                                         \
                                0x00000080

//*****************************************************************************
//
//! This flag indicates that the keys should be filled.
//
//*****************************************************************************
#define KEYBOARD_STYLE_BG       0x00000100


//*****************************************************************************
//
//! Declares an initialized keyboard widget data structure.
//!
//! \param psParent is a pointer to the parent widget.
//! \param psNext is a pointer to the sibling widget.
//! \param psChild is a pointer to the first child widget.
//! \param psDisplay is a pointer to the display on which to draw the keyboard.
//! \param i32X is the X coordinate of the upper left corner of the keyboard.
//! \param i32Y is the Y coordinate of the upper left corner of the keyboard.
//! \param i32Width is the width of the keyboard.
//! \param i32Height is the height of the keyboard.
//! \param ui32Style is the style to be applied to the keyboard.
//! \param ui32BackgroundColor is the background color for the keyboard.
//! \param ui32FillColor is the color used to fill in the keyboard.
//! \param ui32PressFillColor is the color used to fill in the keyboard when
//! a key is pressed.
//! \param ui32OutlineColor is the color used to outline the keys.
//! \param ui32TextColor is the color used to draw text on the keys.
//! \param psFont is a pointer to the font used to draw text on the keys.
//! \param ui16AutoRepeatDelay is the delay before starting auto-repeat.
//! \param ui16AutoRepeatRate is the rate at which auto-repeat events are
//! generated.
//! \param ui32NumKeyboards is the number of keyboards in the psKeyboard
//! parameter.
//! \param psKeyboards is a pointer to the keyboard array to use for this
//! keyboard.
//! \param pfnOnEvent is a pointer to the function that is called when a key
//! is pressed.
//!
//! This macro provides an initialized keyboard widget data structure, which
//! can be used to construct the widget tree at compile time in global
//! variables (as opposed to run-time via function calls).  This must be
//! assigned to a variable, such as:
//!
//! \verbatim
//!     tKeyboardWidget g_sKeyboard = KeyboardStruct(...);
//! \endverbatim
//!
//! Or, in an array of variables:
//!
//! \verbatim
//!     tKeyboardWidget g_psKeyboards[] =
//!     {
//!         KeyboardStruct(...),
//!         KeyboardStruct(...)
//!     };
//! \endverbatim
//!
//! \e ui32Style is the logical OR of the following:
//!
//! - \b PB_STYLE_OUTLINE to indicate that the keys should be outlined.
//! - \b PB_STYLE_FILL to indicate that the keys should be filled.
//! - \b PB_STYLE_IMG to indicate that the keys should have an image
//!   drawn on them (using \e pui8Image).
//! - \b PB_STYLE_TEXT_OPAQUE to indicate that the key text should be
//!   drawn opaque (in other words, drawing the background pixels).
//! - \b PB_STYLE_AUTO_REPEAT to indicate that auto-repeat should be used.
//! - \b PB_STYLE_RELEASE_NOTIFY to indicate that the callback should be made
//!   when a key is released.  If absent, the callback is called when the
//!   key is initially pressed.
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define KeyboardStruct(psParent, psNext, psChild, psDisplay, i32X, i32Y,      \
                       i32Width, i32Height, ui32Style, ui32BackgroundColor,   \
                       ui32FillColor, ui32PressFillColor, ui32OutlineColor,   \
                       ui32TextColor, psFont, ui16AutoRepeatDelay,            \
                       ui16AutoRepeatRate, ui32NumKeyboards, psKeyboards,     \
                       pfnOnEvent)                                            \
        {                                                                     \
            {                                                                 \
                sizeof(tKeyboardWidget),                                      \
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
                KeyboardMsgProc                                               \
            },                                                                \
            ui32Style,                                                        \
            ui32BackgroundColor,                                              \
            ui32FillColor,                                                    \
            ui32PressFillColor,                                               \
            ui32OutlineColor,                                                 \
            ui32TextColor,                                                    \
            psFont,                                                           \
            ui16AutoRepeatDelay,                                              \
            ui16AutoRepeatRate,                                               \
            0,                                                                \
            0,                                                                \
            ui32NumKeyboards,                                                 \
            psKeyboards,                                                      \
            pfnOnEvent,                                                       \
            0,                                                                \
            0                                                                 \
        }

//*****************************************************************************
//
//! Declares an initialized variable containing a keyboard widget data
//! structure.
//!
//! \param sName is the name of the variable to be declared.
//! \param psParent is a pointer to the parent widget.
//! \param psNext is a pointer to the sibling widget.
//! \param psChild is a pointer to the first child widget.
//! \param psDisplay is a pointer to the display on which to draw the keyboard.
//! \param i32X is the X coordinate of the upper left corner of the keyboard.
//! \param i32Y is the Y coordinate of the upper left corner of the keyboard.
//! \param i32Width is the width of the keyboard.
//! \param i32Height is the height of the keyboard.
//! \param ui32Style is the style to be applied to the keyboard.
//! \param ui32BackgroundColor is the background color for the keyboard.
//! \param ui32FillColor is the color used to fill in the keys.
//! \param ui32PressFillColor is the color used to fill in the keys when
//! pressed.
//! \param ui32OutlineColor is the color used to outline the keys.
//! \param ui32TextColor is the color used to draw text on the keys.
//! \param psFont is a pointer to the font to be used to draw text on the keys.
//! \param ui16AutoRepeatDelay is the delay before starting auto-repeat.
//! \param ui16AutoRepeatRate is the rate at which auto-repeat events are
//! generated.
//! \param ui32NumKeyboards is the number of keyboards in the \e psKeyboards
//! array.
//! \param psKeyboards is an array of keyboards that are displayed as a part
//! of this keyboard.
//! \param pfnOnEvent is a pointer to the function that is called when a key is
//! pressed.
//!
//! This macro provides an initialized keyboard widget data structure, which
//! can be used to construct the widget tree at compile time in global
//! variables (as opposed to run-time via function calls).
//!
//! \e ui32Style is the logical OR of the following:
//!
//! - \b PB_STYLE_OUTLINE to indicate that the keys should be outlined.
//! - \b PB_STYLE_FILL to indicate that the keys should be filled.
//! - \b PB_STYLE_IMG to indicate that the keys should have an image
//!   drawn on them (using \e pui8Image).
//! - \b PB_STYLE_TEXT_OPAQUE to indicate that the key text should be
//!   drawn opaque (in other words, drawing the background pixels).
//! - \b PB_STYLE_AUTO_REPEAT to indicate that auto-repeat should be used.
//! - \b PB_STYLE_RELEASE_NOTIFY to indicate that the callback should be made
//!   when a key is released.  If absent, the callback is called when a key
//!   is initially pressed.
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define Keyboard(sName, psParent, psNext, psChild, psDisplay, i32X, i32Y,     \
                 i32Width, i32Height, ui32Style, ui32BackgroundColor,         \
                 ui32FillColor, ui32PressFillColor, ui32OutlineColor,         \
                 ui32TextColor, psFont, ui16AutoRepeatDelay,                  \
                 ui16AutoRepeatRate, ui32NumKeyboards, psKeyboards,           \
                 pfnOnEvent)                                                  \
        tKeyboardWidget sName =                                               \
            KeyboardStruct(psParent, psNext, psChild, psDisplay, i32X, i32Y,  \
                           i32Width, i32Height, ui32Style,                    \
                           ui32BackgroundColor, ui32FillColor,                \
                           ui32PressFillColor, ui32OutlineColor,              \
                           ui32TextColor, psFont, ui16AutoRepeatDelay,        \
                           ui16AutoRepeatRate, ui32NumKeyboards, psKeyboards, \
                           pfnOnEvent)

//*****************************************************************************
//
//! Sets the auto-repeat delay for a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to modify.
//! \param ui16Delay is the number of pointer events before auto-repeat starts.
//!
//! This function sets the delay before auto-repeat begins.  Unpredictable
//! behavior will occur if this is called while a key is pressed.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardAutoRepeatDelaySet(psWidget, ui16Delay)                       \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui16AutoRepeatDelay = ui16Delay;                             \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables auto-repeat for a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to modify.
//!
//! This function disables the auto-repeat behavior of a keyboard.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardAutoRepeatOff(psWidget)                                       \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui32Style &= ~(KEYBOARD_STYLE_AUTO_REPEAT);                  \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables auto-repeat for a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to modify.
//!
//! This function enables the auto-repeat behavior of a keyboard.
//! Unpredictable behavior will occur if this is called while a key is pressed.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardAutoRepeatOn(psWidget)                                        \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui32Style |= KEYBOARD_STYLE_AUTO_REPEAT;                     \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the auto-repeat rate for a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to modify.
//! \param ui16Rate is the number of pointer events between auto-repeat events.
//!
//! This function sets the rate at which auto-repeat events occur.
//! Unpredictable behavior will occur if this is called while a key is pressed.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardAutoRepeatRateSet(psWidget, ui16Rate)                         \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui16AutoRepeatRate = ui16Rate;                               \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the function to call when this keyboard widget is pressed.
//!
//! \param psWidget is a pointer to the keyboard widget to modify.
//! \param pfnOnEventFn is a pointer to the function to call.
//!
//! This function sets the function to be called when a key is pressed.  The
//! supplied function is called when a key is first pressed, and then repeated
//! while the key is pressed if auto-repeat is enabled.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardCallbackSet(psWidget, pfnOnEventFn)                           \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->pfnOnEvent = pfnOnEventFn;                                   \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the fill color of a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to be modified.
//! \param ui32Color is the 24-bit RGB color to use to fill the keys.
//!
//! This function changes the color used to fill the keys on the display.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardFillColorSet(psWidget, ui32Color)                             \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui32FillColor = ui32Color;                                   \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the fill color of a keyboard when it is pressed.
//!
//! \param psWidget is a pointer to the keyboard widget to be modified.
//! \param ui32Color is the 24-bit RGB color to use to fill the keys when they
//! are pressed.
//!
//! This function changes the color used to fill the keys on the display when
//! a key is pressed.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardFillColorPressedSet(psWidget, ui32Color)                      \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui32PressFillColor = ui32Color;                              \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables filling of keys in a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to modify.
//!
//! This function disables the filling of keys in a keyboard widget.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardFillOff(psWidget)                                             \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui32Style &= ~(KEYBOARD_STYLE_FILL);                         \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables filling of a keys in a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to modify.
//!
//! This function enables the filling of a push key in a keyboard widget.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardFillOn(psWidget)                                              \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui32Style |= KEYBOARD_STYLE_FILL;                            \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the font for a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to modify.
//! \param pFnt is a pointer to the font to use to draw text on the keyboard.
//!
//! This function changes the font used to draw text on keys in a keyboard.
//! The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardFontSet(psWidget, pFnt)                                       \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            const tFont *pF = pFnt;                                           \
            psW->psFont = pF;                                                 \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the outline color for keys in a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to be modified.
//! \param ui32Color is the 24-bit RGB color to use to outline the keys.
//!
//! This function changes the color used to outline the keys in a keyboard on
//! the display.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardOutlineColorSet(psWidget, ui32Color)                          \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui32OutlineColor = ui32Color;                                \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables outlining of keys in a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to modify.
//!
//! This function disables the outlining of a keys for a keyboard widget.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardOutlineOff(psWidget)                                          \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui32Style &= ~(KEYBOARD_STYLE_OUTLINE);                      \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables outlining of keys in a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to modify.
//!
//! This function enables the outlining of keys for a keyboard widget.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardOutlineOn(psWidget)                                           \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui32Style |= KEYBOARD_STYLE_OUTLINE;                         \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the text color of keys in a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to be modified.
//! \param ui32Color is the 24-bit RGB color to use to draw text on the keys.
//!
//! This function changes the color used to draw text on the keys on the
//! display.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardTextColorSet(psWidget, ui32Color)                             \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui32TextColor = ui32Color;                                   \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables opaque text on keys in a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to modify.
//!
//! This function disables the use of opaque text on a keyboard.  When
//! not using opaque text, only the foreground pixels of the text are drawn on
//! the screen, allowing the previously drawn pixels (such as the key image)
//! to show through the text.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardTextOpaqueOff(psWidget)                                       \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui32Style &= ~(KEYBOARD_STYLE_TEXT_OPAQUE);                  \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables opaque text on a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to modify.
//!
//! This function enables the use of opaque text on a keyboard.  When
//! using opaque text, both the foreground and background pixels of the text
//! are drawn on the screen, blocking out the previously drawn pixels.
//!
//! \return None.
//
//*****************************************************************************
#define KeyboardTextOpaqueOn(psWidget)                                        \
        do                                                                    \
        {                                                                     \
            tKeyboardWidget *psW = psWidget;                                  \
            psW->ui32Style |= KEYBOARD_STYLE_TEXT_OPAQUE;                     \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! The total number of keyboards in the g_psKeyboardUSEnglish array.
//
//*****************************************************************************
#define NUM_KEYBOARD_US_ENGLISH 3

extern const tKeyboard g_psKeyboardUSEnglish[NUM_KEYBOARD_US_ENGLISH];

//*****************************************************************************
//
// Prototypes for the keyboard widget APIs.
//
//*****************************************************************************
extern int32_t KeyboardMsgProc(tWidget *psWidget, uint32_t ui32Msg,
                               uint32_t ui32Param1, uint32_t ui32Param2);
extern void KeyboardInit(tKeyboardWidget *psWidget, const tDisplay *psDisplay,
                         int32_t i32X, int32_t i32Y, int32_t i32Width,
                         int32_t i32Height);

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

#endif //__KEYBOARD_H__
