//*****************************************************************************
//
// slider.h - Prototypes for the slider widget class.
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

#ifndef __SLIDER_H__
#define __SLIDER_H__

//*****************************************************************************
//
//! \addtogroup slider_api
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
//! The structure that describes a slider widget.
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
    //! SL_STYLE_xxx.
    //
    uint32_t ui32Style;

    //
    //! The 24-bit RGB color used to fill this slider, if SL_STYLE_FILL is
    //! selected, and to use as the background color if SL_STYLE_TEXT_OPAQUE is
    //! selected.
    //
    uint32_t ui32FillColor;

    //
    //! The 24-bit RGB color used to fill the background portion of the slider
    //! if SL_STYLE_FILL is selected, and to use as the background color if
    //! SL_STYLE_TEXT_OPAQUE is selected.
    //
    uint32_t ui32BackgroundFillColor;

    //
    //! The 24-bit RGB color used to outline this slider, if
    //! SL_STYLE_OUTLINE is selected.
    //
    uint32_t ui32OutlineColor;

    //
    //! The 24-bit RGB color used to draw text on the "active" portion of
    //! this slider, if SL_STYLE_TEXT is selected.
    //
    uint32_t ui32TextColor;

    //
    //! The 24-bit RGB color used to draw text on the background portion of
    //! this slider, if SL_STYLE_TEXT is selected.
    //
    uint32_t ui32BackgroundTextColor;

    //
    //! A pointer to the font used to render the slider text, if
    //! SL_STYLE_TEXT is selected.
    //
    const tFont *psFont;

    //
    //! A pointer to the text to draw on this slider, if SL_STYLE_TEXT is
    //! selected.
    //
    const char *pcText;

    //
    //! A pointer to the image to be drawn onto this slider, if
    //! SL_STYLE_IMG is selected.
    //
    const uint8_t *pui8Image;

    //
    //! A pointer to the image to be drawn onto this slider background if
    //! SL_STYLE_BACKG_IMG is selected.
    //
    const uint8_t *pui8BackgroundImage;

    //
    //! A pointer to the function to be called when the state of the slider
    //! changes.
    //
    void (*pfnOnChange)(tWidget *psWidget, int32_t i32Value);

    //
    //! The value represented by the slider at its zero position.  This
    //! value is returned if a horizontal slider is pulled to the far left or
    //! a vertical slider is pulled to the bottom of widget's bounding
    //! rectangle.
    //
    int32_t i32Min;

    //
    //! The value represented by the slider at its maximum position.  This value
    //! is returned if a horizontal slider is pulled to the far right or a
    //! vertical slider is pulled to the top of the widget's bounding
    //! rectangle.
    //
    int32_t i32Max;

    //
    //! The current slider value scaled according to the minimum and maximum
    //! values for the control.
    //
    int32_t i32Value;

    //
    //! This internal work variable stores the pixel position representing the
    //! current slider value.
    //
    int16_t i16Pos;
}
tSliderWidget;

//*****************************************************************************
//
//! This flag indicates that the slider should be outlined.
//
//*****************************************************************************
#define SL_STYLE_OUTLINE            0x00000001

//*****************************************************************************
//
//! This flag indicates that the active portion of the slider should be filled.
//
//*****************************************************************************
#define SL_STYLE_FILL               0x00000002

//*****************************************************************************
//
//! This flag indicates that the background portion of the slider should be
//! filled.
//
//*****************************************************************************
#define SL_STYLE_BACKG_FILL         0x00000004

//*****************************************************************************
//
//! This flag indicates that the slider should have text drawn on top of the
//! active portion.
//
//*****************************************************************************
#define SL_STYLE_TEXT               0x00000008

//*****************************************************************************
//
//! This flag indicates that the slider should have text drawn on top of the
//! background portion.
//
//*****************************************************************************
#define SL_STYLE_BACKG_TEXT         0x00000010

//*****************************************************************************
//
//! This flag indicates that the slider should have an image drawn on it.
//
//*****************************************************************************
#define SL_STYLE_IMG                0x00000020

//*****************************************************************************
//
//! This flag indicates that the slider should have an image drawn on its
//! background.
//
//*****************************************************************************
#define SL_STYLE_BACKG_IMG          0x00000040

//*****************************************************************************
//
//! This flag indicates that the slider text should be drawn opaque (in
//! other words, drawing the background pixels as well as the foreground
//! pixels) in the active portion of the slider.
//
//*****************************************************************************
#define SL_STYLE_TEXT_OPAQUE        0x00000080

//*****************************************************************************
//
//! This flag indicates that the slider text should be drawn opaque (in
//! other words, drawing the background pixels as well as the foreground
//! pixels) in the background portion of the slider.
//
//*****************************************************************************
#define SL_STYLE_BACKG_TEXT_OPAQUE  0x00000100

//*****************************************************************************
//
//! This flag indicates that the slider is vertical rather than horizontal.  If
//! the flag is absent, the slider is assumed to operate horizontally with the
//! reported value increasing from left to right.  If set, the reported value
//! increases from the bottom of the widget towards the top.
//
//*****************************************************************************
#define SL_STYLE_VERTICAL           0x00000200

//*****************************************************************************
//
//! This flag causes the slider to ignore pointer input and act as a passive
//! indicator.  An application may set its value and repaint it as normal but
//! its value will not be changed in response to any touchscreen activity.
//
//*****************************************************************************
#define SL_STYLE_LOCKED             0x00000400

//*****************************************************************************
//
//! Declares an initialized slider widget data structure.
//!
//! \param psParent is a pointer to the parent widget.
//! \param psNext is a pointer to the sibling widget.
//! \param psChild is a pointer to the first child widget.
//! \param psDisplay is a pointer to the display on which to draw the slider.
//! \param i32X is the X coordinate of the upper left corner of the slider.
//! \param i32Y is the Y coordinate of the upper left corner of the slider.
//! \param i32Width is the width of the slider.
//! \param i32Height is the height of the slider.
//! \param i32Min is the minimum value for the slider (corresponding to the
//!  left or bottom position).
//! \param i32Max is the maximum value for the slider (corresponding to the
//!  right or top position).
//! \param i32Value is the initial value of the slider.  This must lie in the
//!  range defined by \e i32Min and \e i32Max.
//! \param ui32Style is the style to be applied to the slider.
//! \param ui32FillColor is the color used to fill in the slider.
//! \param ui32BackgroundFillColor is the color used to fill the background
//! area of the slider.
//! \param ui32OutlineColor is the color used to outline the slider.
//! \param ui32TextColor is the color used to draw text on the slider.
//! \param ui32BackgroundTextColor is the color used to draw text on the
//! background portion of the slider.
//! \param psFont is a pointer to the font to be used to draw text on the
//! slider.
//! \param pcText is a pointer to the text to draw on this slider.
//! \param pui8Image is a pointer to the image to draw on this slider.
//! \param pui8BackgroundImage is a pointer to the image to draw on the slider
//! background.
//! \param pfnOnChange is a pointer to the function that is called to notify
//! the application of slider value changes.
//!
//! This macro provides an initialized slider widget data structure, which can
//! be used to construct the widget tree at compile time in global variables
//! (as opposed to run-time via function calls).  This must be assigned to a
//! variable, such as:
//!
//! \verbatim
//!     tSliderWidget g_sSlider = SliderStruct(...);
//! \endverbatim
//!
//! Or, in an array of variables:
//!
//! \verbatim
//!     tSliderWidget g_psSliders[] =
//!     {
//!         SliderStruct(...),
//!         SliderStruct(...)
//!     };
//! \endverbatim
//!
//! \e ui32Style is the logical OR of the following:
//!
//! - \b #SL_STYLE_OUTLINE to indicate that the slider should be outlined.
//! - \b #SL_STYLE_FILL to indicate that the slider should be filled.
//! - \b #SL_STYLE_BACKG_FILL to indicate that the background portion of the
//!   slider should be filled.
//! - \b #SL_STYLE_TEXT to indicate that the slider should have text drawn
//!   on its active portion (using \e psFont and \e pcText).
//! - \b #SL_STYLE_BACKG_TEXT to indicate that the slider should have text drawn
//!   on its background portion (using \e psFont and \e pcText).
//! - \b #SL_STYLE_IMG to indicate that the slider should have an image
//!   drawn on it (using \e pui8Image).
//! - \b #SL_STYLE_BACKG_IMG to indicate that the slider should have an image
//!   drawn on its background (using \e pui8BackgroundImage).
//! - \b #SL_STYLE_TEXT_OPAQUE to indicate that the slider text should be
//!   drawn opaque (in other words, drawing the background pixels).
//! - \b #SL_STYLE_BACKG_TEXT_OPAQUE to indicate that the slider text should be
//!   drawn opaque in the background portion of the widget. (in other words,
//!   drawing the background pixels).
//! - \b #SL_STYLE_VERTICAL to indicate that this is a vertical slider
//!   rather than a horizontal one (the default if this style flag is not set).
//! - \b #SL_STYLE_LOCKED to indicate that the slider is being used as an
//!   indicator and should ignore user input.
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define SliderStruct(psParent, psNext, psChild, psDisplay, i32X, i32Y,        \
                     i32Width, i32Height, i32Min, i32Max, i32Value,           \
                     ui32Style, ui32FillColor, ui32BackgroundFillColor,       \
                     ui32OutlineColor, ui32TextColor,                         \
                     ui32BackgroundTextColor, psFont, pcText, pui8Image,      \
                     pui8BackgroundImage, pfnOnChange)                        \
        {                                                                     \
            {                                                                 \
                sizeof(tSliderWidget),                                        \
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
                SliderMsgProc                                                 \
            },                                                                \
            ui32Style,                                                        \
            ui32FillColor,                                                    \
            ui32BackgroundFillColor,                                          \
            ui32OutlineColor,                                                 \
            ui32TextColor,                                                    \
            ui32BackgroundTextColor,                                          \
            psFont,                                                           \
            pcText,                                                           \
            pui8Image,                                                        \
            pui8BackgroundImage,                                              \
            pfnOnChange,                                                      \
            i32Min,                                                           \
            i32Max,                                                           \
            i32Value,                                                         \
            0                                                                 \
        }

//*****************************************************************************
//
//! Declares an initialized variable containing a slider widget data structure.
//!
//! \param sName is the name of the variable to be declared.
//! \param psParent is a pointer to the parent widget.
//! \param psNext is a pointer to the sibling widget.
//! \param psChild is a pointer to the first child widget.
//! \param psDisplay is a pointer to the display on which to draw the slider.
//! \param i32X is the X coordinate of the upper left corner of the slider.
//! \param i32Y is the Y coordinate of the upper left corner of the slider.
//! \param i32Width is the width of the slider.
//! \param i32Height is the height of the slider.
//! \param i32Min is the minimum value for the slider (corresponding to the
//!  left or bottom position).
//! \param i32Max is the maximum value for the slider (corresponding to the
//!  right or top position).
//! \param i32Value is the initial value of the slider.  This must lie in the
//!  range defined by \e i32Min and \e i32Max.
//! \param ui32Style is the style to be applied to the slider.
//! \param ui32FillColor is the color used to fill in the slider.
//! \param ui32BackgroundFillColor is the color used to fill in the background
//!  area of the slider.
//! \param ui32OutlineColor is the color used to outline the slider.
//! \param ui32TextColor is the color used to draw text on the slider.
//! \param ui32BackgroundTextColor is the color used to draw text on the
//! background portion of the slider.
//! \param psFont is a pointer to the font to be used to draw text on the
//! slider.
//! \param pcText is a pointer to the text to draw on this slider.
//! \param pui8Image is a pointer to the image to draw on this slider.
//! \param pui8BackgroundImage is a pointer to the image to draw on the slider
//! background.
//! \param pfnOnChange is a pointer to the function that is called to notify
//! the application of slider value changes.
//!
//! This macro provides an initialized slider widget data structure, which can
//! be used to construct the widget tree at compile time in global variables
//! (as opposed to run-time via function calls).
//!
//! \e ui32Style is the logical OR of the following:
//!
//! - \b #SL_STYLE_OUTLINE to indicate that the slider should be outlined.
//! - \b #SL_STYLE_FILL to indicate that the slider should be filled.
//! - \b #SL_STYLE_BACKG_FILL to indicate that the background portion of the
//!   slider should be filled.
//! - \b #SL_STYLE_TEXT to indicate that the slider should have text drawn
//!   on its active portion (using \e psFont and \e pcText).
//! - \b #SL_STYLE_BACKG_TEXT to indicate that the slider should have text
//!   drawn on its background portion (using \e psFont and \e pcText).
//! - \b #SL_STYLE_IMG to indicate that the slider should have an image
//!   drawn on it (using \e pui8Image).
//! - \b #SL_STYLE_BACKG_IMG to indicate that the slider should have an image
//!   drawn on its background (using \e pui8BackgroundImage).
//! - \b #SL_STYLE_TEXT_OPAQUE to indicate that the slider text should be
//!   drawn opaque (in other words, drawing the background pixels).
//! - \b #SL_STYLE_BACKG_TEXT_OPAQUE to indicate that the slider text should be
//!   drawn opaque in the background portion of the widget. (in other words,
//!   drawing the background pixels).
//! - \b #SL_STYLE_VERTICAL to indicate that this is a vertical slider
//!   rather than a horizontal one (the default if this style flag is not set).
//! - \b #SL_STYLE_LOCKED to indicate that the slider is being used as an
//!   indicator and should ignore user input.
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define Slider(sName, psParent, psNext, psChild, psDisplay, i32X, i32Y,       \
               i32Width, i32Height, i32Min, i32Max, i32Value, ui32Style,      \
               ui32FillColor, ui32BackgroundFillColor, ui32OutlineColor,      \
               ui32TextColor, ui32BackgroundTextColor, psFont, pcText,        \
               pui8Image, pui8BackgroundImage, pfnOnChange)                   \
        tSliderWidget sName =                                                 \
            SliderStruct(psParent, psNext, psChild, psDisplay, i32X, i32Y,    \
                         i32Width, i32Height, i32Min, i32Max, i32Value,       \
                         ui32Style, ui32FillColor, ui32BackgroundFillColor,   \
                         ui32OutlineColor, ui32TextColor,                     \
                         ui32BackgroundTextColor, psFont, pcText,             \
                         pui8Image, pui8BackgroundImage, pfnOnChange)

//*****************************************************************************
//
//! Sets the function to call when this slider widget's value changes.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//! \param pfnCallback is a pointer to the function to call.
//!
//! This function sets the function to be called when the value represented by
//! the slider changes.
//!
//! \return None.
//
//*****************************************************************************
#define SliderCallbackSet(psWidget, pfnCallback)                              \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->pfnOnChange = pfnCallback;                                   \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the fill color for the active area of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to be modified.
//! \param ui32Color is the 24-bit RGB color to use to fill the slider.
//!
//! This function changes the color used to fill the active are of the slider
//! on the display.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderFillColorSet(psWidget, ui32Color)                               \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32FillColor = ui32Color;                                   \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the fill color for the background area of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to be modified.
//! \param ui32Color is the 24-bit RGB color to use to fill the background area
//! of the slider.
//!
//! This function changes the color used to fill the background area of the
//! slider on the display.  The display is not updated until the next paint
//! request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderFillColorBackgroundedSet(psWidget, ui32Color)                   \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32BackgroundFillColor = ui32Color;                         \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables filling of the active area of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function disables the filling of the active area of a slider widget.
//! The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderFillOff(psWidget)                                               \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style &= ~(SL_STYLE_FILL);                               \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables filling of the active area of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function enables the filling of the active area of a slider widget.
//! The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderFillOn(psWidget)                                                \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style |= SL_STYLE_FILL;                                  \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables filling of the background area of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function disables the filling of the background area of a slider
//! widget.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderBackgroundFillOff(psWidget)                                     \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style &= ~(SL_STYLE_BACKG_FILL );                        \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables filling of the background area of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function enables the filling of the background area of a slider widget.
//! The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderBackgroundFillOn(psWidget)                                      \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style |= SL_STYLE_BACKG_FILL ;                           \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the font for a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//! \param psFnt is a pointer to the font to use to draw text on the slider.
//!
//! This function changes the font used to draw text on the slider.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderFontSet(psWidget, psFnt)                                        \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            const tFont *psF = psFnt;                                         \
            psW->psFont = psF;                                                \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Changes the image drawn on the active area of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to be modified.
//! \param pImg is a pointer to the image to draw onto the slider.
//!
//! This function changes the image that is drawn on the active area of the
//! slider.  This image will be centered within the widget rectangle and the
//! portion represented by the current slider value will be visible.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderImageSet(psWidget, pImg)                                        \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            const uint8_t *pI = pImg;                                         \
            psW->pui8Image = pI;                                              \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Changes the image drawn on the background area of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to be modified.
//! \param pImg is a pointer to the image to draw onto the background area of
//! the slider.
//!
//! This function changes the image that is drawn onto the background area of
//! the slider.  This image will be centered within the widget rectangle and
//! the portion in the area not represented by the current slider value will
//! be visible.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderBackgroundImageSet(psWidget, pImg)                              \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            const uint8_t *pI = pImg;                                         \
            psW->pui8BackgroundImage = pI;                                    \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables the image on the active area of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function disables the drawing of an image on the active area of a
//! slider widget.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderImageOff(psWidget)                                              \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style &= ~(SL_STYLE_IMG);                                \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables the image on the active area of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function enables the drawing of an image on the active area of a
//! slider widget.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderImageOn(psWidget)                                               \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style |= SL_STYLE_IMG;                                   \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables the image on the background area of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function disables the drawing of an image on the background area of a
//! slider widget.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderBackgroundImageOff(psWidget)                                    \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style &= ~(SL_STYLE_BACKG_IMG);                          \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables the image on the background area of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function enables the drawing of an image on the background area of a
//! slider widget.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderBackgroundImageOn(psWidget)                                     \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style |= SL_STYLE_BACKG_IMG;                             \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the outline color of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to be modified.
//! \param ui32Color is the 24-bit RGB color to use to outline the slider.
//!
//! This function changes the color used to outline the slider on the
//! display.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderOutlineColorSet(psWidget, ui32Color)                            \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32OutlineColor = ui32Color;                                \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables outlining of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function disables the outlining of a slider widget.  The display
//! is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderOutlineOff(psWidget)                                            \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style &= ~(SL_STYLE_OUTLINE);                            \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables outlining of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function enables the outlining of a slider widget.  The display
//! is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderOutlineOn(psWidget)                                             \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style |= SL_STYLE_OUTLINE;                               \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the text color of the active portion of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to be modified.
//! \param ui32Color is the 24-bit RGB color to use to draw text on the slider.
//!
//! This function changes the color used to draw text on the active portion of
//! the slider on the display.  The display is not updated until the next paint
//! request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderTextColorSet(psWidget, ui32Color)                               \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32TextColor = ui32Color;                                   \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the background text color of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to be modified.
//! \param ui32Color is the 24-bit RGB color to use to draw background text on
//! the slider.
//!
//! This function changes the color used to draw text on the slider's
//! background portion on the display.  The display is not updated until the
//! next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderBackgroundTextColorSet(psWidget, ui32Color)                     \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32BackgroundTextColor = ui32Color;                         \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables the text on the active portion of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function disables the drawing of text on the active portion of a
//! slider widget.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderTextOff(psWidget)                                               \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style &= ~(SL_STYLE_TEXT);                               \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables the text on the active portion of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function enables the drawing of text on the active portion of a slider
//! widget.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderTextOn(psWidget)                                                \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style |= SL_STYLE_TEXT;                                  \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables opaque text on the active portion of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function disables the use of opaque text on the active portion of this
//! slider.  When not using opaque text, only the foreground pixels of the text
//! are drawn on the screen, allowing the previously drawn pixels (such as the
//! slider image) to show through the text.  Note that SL_STYLE_TEXT must also
//! be cleared to disable text rendering on the slider active area.
//!
//! \return None.
//
//*****************************************************************************
#define SliderTextOpaqueOff(psWidget)                                         \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style &= ~(SL_STYLE_TEXT_OPAQUE);                        \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables opaque text on the active portion of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function enables the use of opaque text on  the active portion of this
//! slider.  When using opaque text, both the foreground and background pixels
//! of the text are drawn on the screen, blocking out the previously drawn
//! pixels.    Note that SL_STYLE_TEXT must also be set to enable text
//! rendering on the slider active area.
//!
//! \return None.
//
//*****************************************************************************
#define SliderTextOpaqueOn(psWidget)                                          \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style |= SL_STYLE_TEXT_OPAQUE;                           \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables the text on the background portion of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function disables the drawing of text on the background portion of
//! a slider widget.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderBackgroundTextOff(psWidget)                                     \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style &= ~(SL_STYLE_BACKG_TEXT);                         \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables the text on the background portion of a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function enables the drawing of text on the background portion of a
//! slider widget.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderBackgroundTextOn(psWidget)                                      \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style |= SL_STYLE_BACKG_TEXT;                            \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Disables opaque background text on a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function disables the use of opaque text on the background portion of
//! this slider.  When not using opaque text, only the foreground pixels of the
//! text are drawn on the screen, allowing the previously drawn pixels (such as
//! the slider image) to show through the text.  Note that SL_STYLE_BACKG_TEXT
//! must also be cleared to disable text rendering on the slider background
//! area.
//!
//! \return None.
//
//*****************************************************************************
#define SliderBackgroundTextOpaqueOff(psWidget)                               \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style &= ~(SL_STYLE_BACKG_TEXT_OPAQUE);                  \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Enables opaque background text on a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function enables the use of opaque text on the background portion of
//! this slider.  When using opaque text, both the foreground and background
//! pixels of the text are drawn on the screen, blocking out the previously
//! drawn pixels.  Note that SL_STYLE_BACKG_TEXT must also be set to enable
//! text rendering on the slider background area.
//!
//! \return None.
//
//*****************************************************************************
#define SliderBackgroundTextOpaqueOn(psWidget)                                \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style |= SL_STYLE_BACKG_TEXT_OPAQUE;                     \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Locks a slider making it ignore pointer input.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function locks a slider widget and makes it ignore all pointer input.
//! When locked, a slider acts as a passive indicator.  Its value may be
//! changed using SliderValueSet() and the value display updated using
//! WidgetPaint() but no user interaction via the pointer will change the
//! widget value.
//!
//! \return None.
//
//*****************************************************************************
#define SliderLock(psWidget)                                                  \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style |= SL_STYLE_LOCKED;                                \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Unlocks a slider making it pay attention to pointer input.
//!
//! \param psWidget is a pointer to the slider widget to modify.
//!
//! This function unlocks a slider widget.  When unlocked, a slider will
//! respond to pointer input by setting its value appropriately and informing
//! the application via callbacks.
//!
//! \return None.
//
//*****************************************************************************
#define SliderUnlock(psWidget)                                                \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->ui32Style &= ~(SL_STYLE_LOCKED);                             \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Changes the text drawn on a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to be modified.
//! \param pcTxt is a pointer to the text to draw onto the slider.
//!
//! This function changes the text that is drawn onto the slider.  The string
//! is centered across the slider and straddles the active and background
//! portions of the widget.  The display is not updated until the next paint
//! request.
//!
//! \return None.
//
//*****************************************************************************
#define SliderTextSet(psWidget, pcTxt)                                        \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            const char *pcT = pcTxt;                                          \
            psW->pcText = pcT;                                                \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Changes the value range for a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to be modified.
//! \param i32Minimum is the minimum value that the slider will report.
//! \param i32Maximum is the maximum value that the slider will report.
//!
//! This function changes the range of a slider.  Slider positions are reported
//! in terms of this range with the current position of the slider on the
//! display being scaled and translated into this range such that the minimum
//! value represents the left position of a horizontal slider or the bottom
//! position of a vertical slider and the maximum value represents the other
//! end of the slider range.  Note that this function does not cause the slider
//! to be redrawn.  The caller must call WidgetPaint() explicitly after this
//! call to ensure that the widget is redrawn.
//!
//! \return None.
//
//*****************************************************************************
#define SliderRangeSet(psWidget, i32Minimum, i32Maximum)                      \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->i32Min = (i32Minimum);                                       \
            psW->i32Max = (i32Maximum);                                       \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Changes the minimum value for a slider widget.
//!
//! \param psWidget is a pointer to the slider widget to be modified.
//! \param i32Val is the new value to set for the slider.  This is in terms of
//! the value range currently set for the slider.
//!
//! This function changes the value that the slider will display the next time
//! the widget is painted.  The caller is responsible for ensuring that the
//! value passed is within the range specified for the target widget.  The
//! caller must call WidgetPaint() explicitly after this call to ensure that
//! the widget is redrawn.
//!
//! \return None.
//
//*****************************************************************************
#define SliderValueSet(psWidget, i32Val)                                      \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            psW->i32Value = (i32Val);                                         \
        }                                                                     \
        while(0)

//*****************************************************************************
//
//! Sets the vertical or horizontal style for a slider widget
//!
//! \param psWidget is a pointer to the slider widget to be modified.
//! \param bVertical is \b true to set the vertical slider style or \b false
//! to set the horizontal slider style.
//!
//! This function allows the vertical or horizontal style to be set when
//! creating slider widgets dynamically.  The function will typically be called
//! before the slider is first attached to the active widget tree.  Since the
//! vertical or horizontal style is intimately linked with the slider size
//! and position on the display, it seldom makes sense to change this style for
//! a widget which is already on the display.
//!
//! \return None.
//
//*****************************************************************************
#define SliderVerticalSet(psWidget, bVertical)                                \
        do                                                                    \
        {                                                                     \
            tSliderWidget *psW = psWidget;                                    \
            if(bVertical)                                                     \
            {                                                                 \
                psW->ui32Style |= (SL_STYLE_VERTICAL);                        \
            }                                                                 \
            else                                                              \
            {                                                                 \
                psW->ui32Style &= ~(SL_STYLE_VERTICAL);                       \
            }                                                                 \
        }                                                                     \
        while(0)

//*****************************************************************************
//
// Prototypes for the slider widget APIs.
//
//*****************************************************************************
extern int32_t SliderMsgProc(tWidget *psWidget, uint32_t ui32Msg,
                          uint32_t ui32Param1, uint32_t ui32Param2);
extern void SliderInit(tSliderWidget *psWidget,
                       const tDisplay *psDisplay, int32_t i32X, int32_t i32Y,
                       int32_t i32Width, int32_t i32Height);

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

#endif // __SLIDER_H__
