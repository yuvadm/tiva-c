//*****************************************************************************
//
// checkbox.h - Prototypes for the check box widget.
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

#ifndef __CHECKBOX_H__
#define __CHECKBOX_H__

//*****************************************************************************
//
//! \addtogroup checkbox_api
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
//! The structure that describes a check box widget.
//
//*****************************************************************************
typedef struct
{
    //
    //! The generic widget information.
    //
    tWidget sBase;

    //
    //! The style for this check box.  This is a set of flags defined by
    //! CB_STYLE_xxx.
    //
    uint16_t ui16Style;

    //
    //! The size of the check box itself, not including the text and/or image
    //! that accompanies it (in other words, the size of the actual box that is
    //! checked or unchecked).
    //
    uint16_t ui16BoxSize;

    //
    //! The 24-bit RGB color used to fill this check box, if CB_STYLE_FILL is
    //! selected, and to use as the background color if CB_STYLE_TEXT_OPAQUE is
    //! selected.
    //
    uint32_t ui32FillColor;

    //
    //! The 24-bit RGB color used to outline this check box, if
    //! CB_STYLE_OUTLINE is selected.
    //
    uint32_t ui32OutlineColor;

    //
    //! The 24-bit RGB color used to draw text on this check box, if
    //! CB_STYLE_TEXT is selected.
    //
    uint32_t ui32TextColor;

    //
    //! The font used to draw the check box text, if CB_STYLE_TEXT is selected.
    //
    const tFont *psFont;

    //
    //! A pointer to the text to draw on this check box, if CB_STYLE_TEXT is
    //! selected.
    //
    const char *pcText;

    //
    //! A pointer to the image to be drawn onto this check box, if CB_STYLE_IMG
    //! is selected.
    //
    const uint8_t *pui8Image;

    //
    //! A pointer to the function to be called when the check box is pressed.
    //! This function is called when the state of the check box is changed.
    //
    void (*pfnOnChange)(tWidget *psWidget, uint32_t bSelected);
}
tCheckBoxWidget;

//*****************************************************************************
//
//! This flag indicates that the check box should be outlined.
//
//*****************************************************************************
#define CB_STYLE_OUTLINE        0x0001

//*****************************************************************************
//
//! This flag indicates that the check box should be filled.
//
//*****************************************************************************
#define CB_STYLE_FILL           0x0002

//*****************************************************************************
//
//! This flag indicates that the check box should have text drawn on it.
//
//*****************************************************************************
#define CB_STYLE_TEXT           0x0004

//*****************************************************************************
//
//! This flag indicates that the check box should have an image drawn on it.
//
//*****************************************************************************
#define CB_STYLE_IMG            0x0008

//*****************************************************************************
//
//! This flag indicates that the check box text should be drawn opaque (in
//! other words, drawing the background pixels as well as the foreground
//! pixels).
//
//*****************************************************************************
#define CB_STYLE_TEXT_OPAQUE    0x0010

//*****************************************************************************
//
//! This flag indicates that the check box is selected.
//
//*****************************************************************************
#define CB_STYLE_SELECTED       0x0020

//*****************************************************************************
//
//! Declares an initialized check box widget data structure.
//!
//! \param psParent is a pointer to the parent widget.
//! \param psNext is a pointer to the sibling widget.
//! \param psChild is a pointer to the first child widget.
//! \param psDisplay is a pointer to the display on which to draw the check box.
//! \param i32X is the X coordinate of the upper left corner of the check box.
//! \param i32Y is the Y coordinate of the upper left corner of the check box.
//! \param i32Width is the width of the check box.
//! \param i32Height is the height of the check box.
//! \param ui16Style is the style to be applied to this check box.
//! \param ui16BoxSize is the size of the box that is checked.
//! \param ui32FillColor is the color used to fill in the check box.
//! \param ui32OutlineColor is the color used to outline the check box.
//! \param ui32TextColor is the color used to draw text on the check box.
//! \param psFont is a pointer to the font to be used to draw text on the check
//! box.
//! \param pcText is a pointer to the text to draw on this check box.
//! \param pui8Image is a pointer to the image to draw on this check box.
//! \param pfnOnChange is a pointer to the function that is called when the
//! check box is pressed.
//!
//! This macro provides an initialized check box widget data structure, which
//! can be used to construct the widget tree at compile time in global
//! variables (as opposed to run-time via function calls).  This must be
//! assigned to a variable, such as:
//!
//! \verbatim
//!     tCheckBoxWidget g_sCheckBox = CheckBoxStruct(...);
//! \endverbatim
//!
//! Or, in an array of variables:
//!
//! \verbatim
//!     tCheckBoxWidget g_psCheckBoxes[] =
//!     {
//!         CheckBoxStruct(...),
//!         CheckBoxStruct(...)
//!     };
//! \endverbatim
//!
//! \e ui16Style is the logical OR of the following:
//!
//! - \b #CB_STYLE_OUTLINE to indicate that the check box should be outlined.
//! - \b #CB_STYLE_FILL to indicate that the check box should be filled.
//! - \b #CB_STYLE_TEXT to indicate that the check box should have text drawn
//!   on it (using \e psFont and \e pcText).
//! - \b #CB_STYLE_IMG to indicate that the check box should have an image
//!   drawn on it (using \e pui8Image).
//! - \b #CB_STYLE_TEXT_OPAQUE to indicate that the check box text should be
//!   drawn opaque (in other words, drawing the background pixels).
//! - \b #CB_STYLE_SELECTED to indicate that the check box is selected.
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define CheckBoxStruct(psParent, psNext, psChild, psDisplay, i32X, i32Y,      \
                       i32Width, i32Height, ui16Style, ui16BoxSize,           \
                       ui32FillColor, ui32OutlineColor, ui32TextColor, psFont,\
                       pcText, pui8Image, pfnOnChange)                        \
        {                                                                     \
            {                                                                 \
                sizeof(tCheckBoxWidget),                                      \
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
                CheckBoxMsgProc                                               \
            },                                                                \
            ui16Style,                                                        \
            ui16BoxSize,                                                      \
            ui32FillColor,                                                    \
            ui32OutlineColor,                                                 \
            ui32TextColor,                                                    \
            psFont,                                                           \
            pcText,                                                           \
            pui8Image,                                                        \
            pfnOnChange                                                       \
        }

//*****************************************************************************
//
//! Declares an initialized variable containing a check box widget data
//! structure.
//!
//! \param sName is the name of the variable to be declared.
//! \param psParent is a pointer to the parent widget.
//! \param psNext is a pointer to the sibling widget.
//! \param psChild is a pointer to the first child widget.
//! \param psDisplay is a pointer to the display on which to draw the check box.
//! \param i32X is the X coordinate of the upper left corner of the check box.
//! \param i32Y is the Y coordinate of the upper left corner of the check box.
//! \param i32Width is the width of the check box.
//! \param i32Height is the height of the check box.
//! \param ui16Style is the style to be applied to this check box.
//! \param ui16BoxSize is the size of the box that is checked.
//! \param ui32FillColor is the color used to fill in the check box.
//! \param ui32OutlineColor is the color used to outline the check box.
//! \param ui32TextColor is the color used to draw text on the check box.
//! \param psFont is a pointer to the font to be used to draw text on the check
//! box.
//! \param pcText is a pointer to the text to draw on this check box.
//! \param pui8Image is a pointer to the image to draw on this check box.
//! \param pfnOnChange is a pointer to the function that is called when the
//! check box is pressed.
//!
//! This macro provides an initialized check box widget data structure, which
//! can be used to construct the widget tree at compile time in global
//! variables (as opposed to run-time via function calls).
//!
//! \e ui16Style is the logical OR of the following:
//!
//! - \b #CB_STYLE_OUTLINE to indicate that the check box should be outlined.
//! - \b #CB_STYLE_FILL to indicate that the check box should be filled.
//! - \b #CB_STYLE_TEXT to indicate that the check box should have text drawn
//!   on it (using \e psFont and \e pcText).
//! - \b #CB_STYLE_IMG to indicate that the check box should have an image
//!   drawn on it (using \e pui8Image).
//! - \b #CB_STYLE_TEXT_OPAQUE to indicate that the check box text should be
//!   drawn opaque (in other words, drawing the background pixels).
//! - \b #CB_STYLE_SELECTED to indicate that the check box is selected.
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define CheckBox(sName, psParent, psNext, psChild, psDisplay, i32X, i32Y,     \
                 i32Width, i32Height, ui16Style, ui16BoxSize, ui32FillColor,  \
                 ui32OutlineColor, ui32TextColor, psFont, pcText, pui8Image,  \
                 pfnOnChange)                                                 \
        tCheckBoxWidget sName =                                               \
            CheckBoxStruct(psParent, psNext, psChild, psDisplay, i32X, i32Y,  \
                           i32Width, i32Height, ui16Style, ui16BoxSize,       \
                           ui32FillColor, ui32OutlineColor, ui32TextColor,    \
                           psFont, pcText, pui8Image, pfnOnChange)

//*****************************************************************************
//
//! Sets size of the box to be checked.
//!
//! \param psWidget is a pointer to the check box widget to modify.
//! \param ui16Size is the size of the box, in pixels.
//!
//! This function sets the size of the box that is drawn as part of the check
//! box.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxBoxSizeSet(psWidget, ui16Size)                                \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui16BoxSize = ui16Size;                                      \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the function to call when this check box widget is toggled.
//!
//! \param psWidget is a pointer to the check box widget to modify.
//! \param pfnOnChg is a pointer to the function to call.
//!
//! This function sets the function to be called when this check box is
//! toggled.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxCallbackSet(psWidget, pfnOnChg)                               \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->pfnOnChange = pfnOnChg;                                      \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the fill color of a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to be modified.
//! \param ui32Color is the 24-bit RGB color to use to fill the check box.
//!
//! This function changes the color used to fill the check box on the display.
//! The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxFillColorSet(psWidget, ui32Color)                             \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui32FillColor = ui32Color;                                   \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables filling of a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to modify.
//!
//! This function disables the filling of a check box widget.  The display is
//! not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxFillOff(psWidget)                                             \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui16Style &= ~(CB_STYLE_FILL);                               \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables filling of a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to modify.
//!
//! This function enables the filling of a check box widget.  The display is
//! not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxFillOn(psWidget)                                              \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui16Style |= CB_STYLE_FILL;                                  \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the font for a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to modify.
//! \param pFnt is a pointer to the font to use to draw text on the check box.
//!
//! This function changes the font used to draw text on the check box.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxFontSet(psWidget, pFnt)                                       \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            const tFont *pF = pFnt;                                           \
            psW->psFont = pF;                                                 \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Changes the image drawn on a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to be modified.
//! \param pImg is a pointer to the image to draw onto the check box.
//!
//! This function changes the image that is drawn onto the check box.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxImageSet(psWidget, pImg)                                      \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            const uint8_t *pI = pImg;                                         \
            psW->pui8Image = pI;                                              \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables the image on a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to modify.
//!
//! This function disables the drawing of an image on a check box widget.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxImageOff(psWidget)                                            \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui16Style &= ~(CB_STYLE_IMG);                                \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables the image on a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to modify.
//!
//! This function enables the drawing of an image on a check box widget.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxImageOn(psWidget)                                             \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui16Style |= CB_STYLE_IMG;                                   \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the outline color of a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to be modified.
//! \param ui32Color is the 24-bit RGB color to use to outline the check box.
//!
//! This function changes the color used to outline the check box on the
//! display.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxOutlineColorSet(psWidget, ui32Color)                          \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui32OutlineColor = ui32Color;                                \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables outlining of a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to modify.
//!
//! This function disables the outlining of a check box widget.  The display is
//! not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxOutlineOff(psWidget)                                          \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui16Style &= ~(CB_STYLE_OUTLINE);                            \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables outlining of a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to modify.
//!
//! This function enables the outlining of a check box widget.  The display is
//! not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxOutlineOn(psWidget)                                           \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui16Style |= CB_STYLE_OUTLINE;                               \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the text color of a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to be modified.
//! \param ui32Color is the 24-bit RGB color to use to draw text on the check
//! box.
//!
//! This function changes the color used to draw text on the check box on the
//! display.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxTextColorSet(psWidget, ui32Color)                             \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui32TextColor = ui32Color;                                   \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables the text on a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to modify.
//!
//! This function disables the drawing of text on a check box widget.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxTextOff(psWidget)                                             \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui16Style &= ~(CB_STYLE_TEXT);                               \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables the text on a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to modify.
//!
//! This function enables the drawing of text on a check box widget.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxTextOn(psWidget)                                              \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui16Style |= CB_STYLE_TEXT;                                  \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables opaque text on a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to modify.
//!
//! This function disables the use of opaque text on this check box.  When not
//! using opaque text, only the foreground pixels of the text are drawn on the
//! screen, allowing the previously drawn pixels (such as the check box image)
//! to show through the text.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxTextOpaqueOff(psWidget)                                       \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui16Style &= ~(CB_STYLE_TEXT_OPAQUE);                        \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables opaque text on a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to modify.
//!
//! This function enables the use of opaque text on this check box.  When using
//! opaque text, both the foreground and background pixels of the text are
//! drawn on the screen, blocking out the previously drawn pixels.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxTextOpaqueOn(psWidget)                                        \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            psW->ui16Style |= CB_STYLE_TEXT_OPAQUE;                           \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Changes the text drawn on a check box widget.
//!
//! \param psWidget is a pointer to the check box widget to be modified.
//! \param pcTxt is a pointer to the text to draw onto the check box.
//!
//! This function changes the text that is drawn onto the check box.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define CheckBoxTextSet(psWidget, pcTxt)                                      \
        do                                                                    \
        {                                                                     \
            tCheckBoxWidget *psW = psWidget;                                  \
            const char *pcT = pcTxt;                                          \
            psW->pcText = pcT;                                                \
        }                                                                     \
        while(0)

//*****************************************************************************
//
// Prototypes for the check box widget APIs.
//
//*****************************************************************************
extern int32_t CheckBoxMsgProc(tWidget *psWidget, uint32_t ui32Msg,
                               uint32_t ui32Param1, uint32_t ui32Param2);
extern void CheckBoxInit(tCheckBoxWidget *psWidget, const tDisplay *psDisplay,
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

#endif // __CHECKBOX_H__
