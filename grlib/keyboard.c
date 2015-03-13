//*****************************************************************************
//
// keyboard.c -
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

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/debug.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/keyboard.h"

//*****************************************************************************
//
//! \addtogroup keyboard_api
//! @{
//
//*****************************************************************************
#define MAX_KEYS_US_EN_LOWER    34
const tKeyText g_psUSEnglishLower[MAX_KEYS_US_EN_LOWER] =
{
    //
    // Row 1
    //
    {
        'q', 1000, 2500, 0, 0
    },
    {
        'w', 1000, 2500, 1000, 0
    },
    {
        'e', 1000, 2500, 2000, 0
    },
    {
        'r', 1000, 2500, 3000, 0
    },
    {
        't', 1000, 2500, 4000, 0
    },
    {
        'y', 1000, 2500, 5000, 0
    },
    {
        'u', 1000, 2500, 6000, 0
    },
    {
        'i', 1000, 2500, 7000, 0
    },
    {
        'o', 1000, 2500, 8000, 0
    },
    {
        'p', 1000, 2500, 9000, 0
    },
    //
    // Row 2
    //
    {
        'a', 1000, 2500, 500, 2500
    },
    {
        's', 1000, 2500, 1500, 2500
    },
    {
        'd', 1000, 2500, 2500, 2500
    },
    {
        'f', 1000, 2500, 3500, 2500
    },
    {
        'g', 1000, 2500, 4500, 2500
    },
    {
        'h', 1000, 2500, 5500, 2500
    },
    {
        'j', 1000, 2500, 6500, 2500
    },
    {
        'k', 1000, 2500, 7500, 2500
    },
    {
        'l', 1000, 2500, 8500, 2500
    },
    //
    // Row 3
    //
    {
        UNICODE_CUSTOM_SHIFT, 1500, 2500, 0, 5000
    },
    {
        'z', 1000, 2500, 1500, 5000
    },
    {
        'x', 1000, 2500, 2500, 5000
    },
    {
        'c', 1000, 2500, 3500, 5000
    },
    {
        'v', 1000, 2500, 4500, 5000
    },
    {
        'b', 1000, 2500, 5500, 5000
    },
    {
        'n', 1000, 2500, 6500, 5000
    },
    {
        'm', 1000, 2500, 7500, 5000
    },
    {
        UNICODE_BACKSPACE, 1500, 2500, 8500, 5000
    },
    //
    // Row 4
    //
    {
        UNICODE_CUSTOM_MODE_TOG, 1500, 2500, 0, 7500
    },
    {
        ',', 1000, 2500, 1500, 7500
    },
    {
        '/', 1000, 2500, 2500, 7500
    },
    {
        ' ', 4000, 2500, 3500, 7500
    },
    {
        '.', 1000, 2500, 7500, 7500
    },
    {
        UNICODE_RETURN, 1500, 2500, 8500, 7500
    }
};

#define MAX_KEYS_US_EN_UPPER    34
const tKeyText g_psUSEnglishUpper[MAX_KEYS_US_EN_UPPER] =
{
    //
    // Row 1
    //
    {
        'Q', 1000, 2500, 0, 0
    },
    {
        'W', 1000, 2500, 1000, 0
    },
    {
        'E', 1000, 2500, 2000, 0
    },
    {
        'R', 1000, 2500, 3000, 0
    },
    {
        'T', 1000, 2500, 4000, 0
    },
    {
        'Y', 1000, 2500, 5000, 0
    },
    {
        'U', 1000, 2500, 6000, 0
    },
    {
        'I', 1000, 2500, 7000, 0
    },
    {
        'O', 1000, 2500, 8000, 0
    },
    {
        'P', 1000, 2500, 9000, 0
    },
    //
    // Row 2
    //
    {
        'A', 1000, 2500, 500, 2500
    },
    {
        'S', 1000, 2500, 1500, 2500
    },
    {
        'D', 1000, 2500, 2500, 2500
    },
    {
        'F', 1000, 2500, 3500, 2500
    },
    {
        'G', 1000, 2500, 4500, 2500
    },
    {
        'H', 1000, 2500, 5500, 2500
    },
    {
        'J', 1000, 2500, 6500, 2500
    },
    {
        'K', 1000, 2500, 7500, 2500
    },
    {
        'L', 1000, 2500, 8500, 2500
    },
    //
    // Row 3
    //
    {
        UNICODE_CUSTOM_SHIFT, 1500, 2500, 00, 5000
    },
    {
        'Z', 1000, 2500, 1500, 5000
    },
    {
        'X', 1000, 2500, 2500, 5000
    },
    {
        'C', 1000, 2500, 3500, 5000
    },
    {
        'V', 1000, 2500, 4500, 5000
    },
    {
        'B', 1000, 2500, 5500, 5000
    },
    {
        'N', 1000, 2500, 6500, 5000
    },
    {
        'M', 1000, 2500, 7500, 5000
    },
    {
        UNICODE_BACKSPACE, 1500, 2500, 8500, 5000
    },
    //
    // Row 4
    //
    {
        UNICODE_CUSTOM_MODE_TOG, 1500, 2500, 00, 7500
    },
    {
        ',', 1000, 2500, 1500, 7500
    },
    {
        '.', 1000, 2500, 2500, 7500
    },
    {
        ' ', 4000, 2500, 3500, 7500
    },
    {
        '/', 1000, 2500, 7500, 7500
    },
    {
        UNICODE_RETURN, 1500, 2500, 8500, 7500
    }
};

#define MAX_KEYS_US_EN_NUMERIC  38
const tKeyText g_psUSEnglishNumeric[MAX_KEYS_US_EN_NUMERIC] =
{
    //
    // Row 1
    //
    {
        '1', 1000, 2500, 0, 0
    },
    {
        '2', 1000, 2500, 1000, 0
    },
    {
        '3', 1000, 2500, 2000, 0
    },
    {
        '4', 1000, 2500, 3000, 0
    },
    {
        '5', 1000, 2500, 4000, 0
    },
    {
        '6', 1000, 2500, 5000, 0
    },
    {
        '7', 1000, 2500, 6000, 0
    },
    {
        '8', 1000, 2500, 7000, 0
    },
    {
        '9', 1000, 2500, 8000, 0
    },
    {
        '0', 1000, 2500, 9000, 0
    },
    //
    // Row 2
    //
    {
        '!', 1000, 2500, 0, 2500
    },
    {
        '@', 1000, 2500, 1000, 2500
    },
    {
        '#', 1000, 2500, 2000, 2500
    },
    {
        '$', 1000, 2500, 3000, 2500
    },
    {
        '%', 1000, 2500, 4000, 2500
    },
    {
        '^', 1000, 2500, 5000, 2500
    },
    {
        '&', 1000, 2500, 6000, 2500
    },
    {
        '*', 1000, 2500, 7000, 2500
    },
    {
        '(', 1000, 2500, 8000, 2500
    },
    {
        ')', 1000, 2500, 9000, 2500
    },
    //
    // Row 3
    //
    {
        '?', 1000, 2500, 0000, 5000
    },
    {
        '-', 1000, 2500, 1000, 5000
    },
    {
        '=', 1000, 2500, 2000, 5000
    },
    {
        '\'', 1000, 2500, 3000, 5000
    },
    {
        '+', 1000, 2500, 4000, 5000
    },
    {
        '[', 1000, 2500, 5000, 5000
    },
    {
        ']', 1000, 2500, 6000, 5000
    },
    {
        '\"', 1000, 2500, 7000, 5000
    },
    {
        UNICODE_BACKSPACE, 2000, 2500, 8000, 5000
    },
    //
    // Row 4
    //
    {
        UNICODE_CUSTOM_MODE_TOG, 1500, 2500, 0000, 7500
    },
    {
        ';', 1000, 2500, 1500, 7500
    },
    {
        ':', 1000, 2500, 2500, 7500
    },
    {
        '\\', 1000, 2500, 3500, 7500
    },
    {
        '|', 1000, 2500, 4500, 7500
    },
    {
        '_', 1000, 2500, 5500, 7500
    },
    {
        '/', 1000, 2500, 6500, 7500
    },
    {
        '~', 1000, 2500, 7500, 7500
    },
    {
        UNICODE_RETURN, 1500, 2500, 8500, 7500
    }
};

const tKeyboard g_psKeyboardUSEnglish[NUM_KEYBOARD_US_ENGLISH] =
{
    {
        UNICODE_CUSTOM_LOWCASE,
        MAX_KEYS_US_EN_LOWER,
        0,
        {
            //
            // Text based keys.
            //
            .psKeysText = g_psUSEnglishLower
        }
    },
    {
        UNICODE_CUSTOM_UPCASE,
        MAX_KEYS_US_EN_UPPER,
        0,
        {
            //
            // Text based keys.
            //
            .psKeysText = g_psUSEnglishUpper
        }
    },
    {
        UNICODE_CUSTOM_NUMERIC,
        MAX_KEYS_US_EN_NUMERIC,
        0,
        {
            //
            // Text based keys.
            //
            .psKeysText = g_psUSEnglishNumeric
        }
    }
};

//*****************************************************************************
//
// Local defines for the flags in the tKeyboardWidget.ui32Flags.
//
//*****************************************************************************
#define FLAG_KEY_PRESSED        0x00000001
#define FLAG_KEY_CAPSLOCK       0x00000002

//*****************************************************************************
//
//! Draws a key on the keyboard.
//!
//! \param psWidget is a pointer to the keyboard widget to be drawn.
//! \param psKey is a pointer to the key to draw.
//!
//! This function draws a single key on the display.  This is called whenever
//! a key on the keyboard needs to be updated.
//!
//! \return None.
//
//*****************************************************************************
static void
ButtonPaintText(tWidget *psWidget, const tKeyText *psKey)
{
    tKeyboardWidget *psKeyboard;
    tContext sCtx;
    int32_t i32X, i32Y;
    uint32_t ui32Range, ui32Size;
    tRectangle sRect;
    char pcKeyCap[4];

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a keyboard widget pointer.
    //
    psKeyboard = (tKeyboardWidget *)psWidget;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sCtx, psWidget->psDisplay);

    //
    // Initialize the clipping region based on the extents of this keyboard.
    //
    GrContextClipRegionSet(&sCtx, &(psWidget->sPosition));

    //
    // Calculate a keys bounding box.
    //
    ui32Range = psWidget->sPosition.i16XMax - psWidget->sPosition.i16XMin + 1;
    sRect.i16XMin = psWidget->sPosition.i16XMin + 1;
    sRect.i16XMin += (ui32Range * (uint32_t)psKey->ui16XPos) / 10000;

    sRect.i16XMax = sRect.i16XMin - 3 +
                    ((ui32Range * (uint32_t)psKey->ui16Width) / 10000);

    ui32Range = psWidget->sPosition.i16YMax - psWidget->sPosition.i16YMin + 1;
    sRect.i16YMin = psWidget->sPosition.i16YMin + 1;
    sRect.i16YMin += (ui32Range * (uint32_t)psKey->ui16YPos) / 10000;

    sRect.i16YMax = sRect.i16YMin - 3 +
                    ((ui32Range * (uint32_t)psKey->ui16Height) / 10000);

    //
    // See if the keyboard fill style is selected.
    //
    if(psKeyboard->ui32Style & KEYBOARD_STYLE_FILL)
    {
        //
        // Fill the key with the fill color.
        //
        GrContextForegroundSet(&sCtx,
                               ((psKeyboard->ui32Flags & FLAG_KEY_PRESSED) ?
                                psKeyboard->ui32PressFillColor :
                                psKeyboard->ui32FillColor));
        GrRectFill(&sCtx, &sRect);
    }

    //
    // See if the keyboard outline style is selected.
    //
    if(psKeyboard->ui32Style & KEYBOARD_STYLE_OUTLINE)
    {
        //
        // Outline the key with the outline color.
        //
        GrContextForegroundSet(&sCtx, psKeyboard->ui32OutlineColor);
        GrRectDraw(&sCtx, &sRect);
    }

    //
    // Compute the center of the key.
    //
    i32X = (sRect.i16XMin + ((sRect.i16XMax - sRect.i16XMin + 1) / 2));
    i32Y = (sRect.i16YMin + ((sRect.i16YMax - sRect.i16YMin + 1) / 2));

    //
    // If the keyboard outline style is selected then shrink the
    // clipping region by one pixel on each side so that the outline is not
    // overwritten by the text or image.
    //
    if(psKeyboard->ui32Style & KEYBOARD_STYLE_OUTLINE)
    {
        sCtx.sClipRegion.i16XMin++;
        sCtx.sClipRegion.i16YMin++;
        sCtx.sClipRegion.i16XMax--;
        sCtx.sClipRegion.i16YMax--;
    }

    //
    // Draw the text centered in the middle of the key.
    //
    GrContextFontSet(&sCtx, psKeyboard->psFont);
    GrContextForegroundSet(&sCtx, psKeyboard->ui32TextColor);
    GrContextBackgroundSet(&sCtx,
                           ((psKeyboard->ui32Flags & FLAG_KEY_PRESSED) ?
                            psKeyboard->ui32PressFillColor :
                            psKeyboard->ui32FillColor));

    ui32Size = 0;

    if(psKey->ui32Code == UNICODE_BACKSPACE)
    {
        pcKeyCap[0] = 'B';
        pcKeyCap[1] = 'S';
        ui32Size = 2;
    }
    else if(psKey->ui32Code == UNICODE_RETURN)
    {
        pcKeyCap[0] = 'E';
        pcKeyCap[1] = 'n';
        pcKeyCap[2] = 't';
        ui32Size = 3;
    }
    else if(psKey->ui32Code == UNICODE_CUSTOM_SHIFT)
    {
        pcKeyCap[0] = 'S';
        pcKeyCap[1] = 'h';
        ui32Size = 2;
    }
    else if(psKey->ui32Code == UNICODE_CUSTOM_MODE_TOG)
    {
        pcKeyCap[0] = '1';
        pcKeyCap[1] = '2';
        pcKeyCap[2] = '3';
        ui32Size = 3;
    }
    else
    {
        ui32Size = 1;
        pcKeyCap[0] = (char)psKey->ui32Code;
    }
    GrStringDrawCentered(&sCtx, (const char *)pcKeyCap, ui32Size, i32X,
                       i32Y,
                       psKeyboard->ui32Style & KEYBOARD_STYLE_TEXT_OPAQUE);
}

//*****************************************************************************
//
//! Draws a the full keyboard.
//!
//! \param psWidget is a pointer to the keyboard widget to be drawn.
//!
//! This function draws a the full keyboard.  This is called whenever
//! the full keyboard needs to be updated.
//!
//! \return None.
//
//*****************************************************************************
static void
KeyboardPaint(tWidget *psWidget)
{
    int32_t i32Key;
    tKeyboardWidget *psKeyboardWidget;
    const tKeyboard *psKeyboard;
    tContext sCtx;

    //
    // Convert the generic widget pointer into a keyboard widget pointer.
    //
    psKeyboardWidget = (tKeyboardWidget *)psWidget;

    psKeyboard = &psKeyboardWidget->psKeyboards[psKeyboardWidget->ui32Active];

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sCtx, psWidget->psDisplay);

    //
    // Initialize the clipping region based on the extents of this keyboard.
    //
    GrContextClipRegionSet(&sCtx, &(psWidget->sPosition));

    //
    // Fill the keyboard with the fill color.
    //
    if(psKeyboardWidget->ui32Style & KEYBOARD_STYLE_BG)
    {
        GrContextForegroundSet(&sCtx, psKeyboardWidget->ui32BackgroundColor);
        GrRectFill(&sCtx, &psWidget->sPosition);
    }

    for(i32Key = 0; i32Key < psKeyboard->ui16NumKeys; i32Key++)
    {
        ButtonPaintText(psWidget, &psKeyboard->uKeys.psKeysText[i32Key]);
    }
}

//*****************************************************************************
//
// Finds if a given X/Y position is within any keys on the keyboard.
//
// \param psKeyWidget is a pointer to the keyboard widget.
// \param psKeyboard is a pointer to the active keyboard.
// \param i32X is the X position for the query.
// \param i32Y is the Y position for the query.
//
// This function is used to determine if a given X/Y position is within the
// bounds of any key on the keyboard.  If the key is found the index for that
// key is returned if the key is not found the value is maximum number of keys
// for the given keyboard indicated by psKeyboard->ui16NumKeys.
//
// \return The key found or psKeyboard->ui16NumKeys if no key was found.
//
//*****************************************************************************
static uint32_t
FindKey(tKeyboardWidget *psKeyWidget, const tKeyboard *psKeyboard,
        int32_t i32X, int32_t i32Y)
{
    uint32_t ui32Key;
    uint32_t ui32Range;
    int32_t i32XMin, i32XMax, i32YMin, i32YMax;
    const tKeyText *psKey;

    //
    // Pre-scale the positions to multiples of 10000.
    //
    i32X *= 10000;
    i32Y *= 10000;

    for(ui32Key = 0; ui32Key < psKeyboard->ui16NumKeys; ui32Key++)
    {
        psKey = &psKeyboard->uKeys.psKeysText[ui32Key];

        //
        // Find the X bounds of the key.
        //
        ui32Range = psKeyWidget->sBase.sPosition.i16XMax -
                    psKeyWidget->sBase.sPosition.i16XMin + 1;
        i32XMin = psKeyWidget->sBase.sPosition.i16XMin * 10000;
        i32XMin += (ui32Range * (uint32_t)psKey->ui16XPos);

        i32XMax = i32XMin + (ui32Range * (uint32_t)psKey->ui16Width);

        //
        // Find the Y bounds of the key.
        //
        ui32Range = psKeyWidget->sBase.sPosition.i16YMax -
                    psKeyWidget->sBase.sPosition.i16YMin + 1;
        i32YMin = psKeyWidget->sBase.sPosition.i16YMin * 10000;
        i32YMin += (ui32Range * (uint32_t)psKey->ui16YPos);

        i32YMax = i32YMin + (ui32Range * (uint32_t)psKey->ui16Height);

        if((i32X >= i32XMin) && (i32X <= i32XMax) &&
           (i32Y >= i32YMin) && (i32Y <= i32YMax))
        {
            break;
        }
    }

    return(ui32Key);
}

//*****************************************************************************
//
//! Handles click events for the keyboard.
//!
//! \param psWidget is a pointer to the keyboard widget.
//! \param ui32Msg is the pointer event message.
//! \param i32X is the X coordinate of the pointer event.
//! \param i32Y is the Y coordinate of the pointer event.
//!
//! This function processes pointer event messages for a keyboard.  This is
//! called in response to a \b WIDGET_MSG_PTR_DOWN, \b WIDGET_MSG_PTR_MOVE, and
//! \b WIDGET_MSG_PTR_UP messages.
//!
//! If the \b WIDGET_MSG_PTR_UP message is received with a position within the
//! extents of the keyboard, the pfnOnEvent callback function is called when
//! the click is inside one of the keys on the keyboard.
//!
//! \return Returns 1 if the coordinates are within the extents of a key on the
//! the keyboard and 0 otherwise.
//
//*****************************************************************************
static int32_t
TextButtonEvent(tWidget *psWidget, uint32_t ui32Msg, int32_t i32X,
                int32_t i32Y)
{
    tKeyboardWidget *psKeyWidget;
    const tKeyboard *psKeyboard;
    uint32_t ui32Key;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Convert the generic widget pointer into a keyboard widget pointer.
    //
    psKeyWidget = (tKeyboardWidget *)psWidget;
    psKeyboard = &psKeyWidget->psKeyboards[psKeyWidget->ui32Active];

    //
    // Find which key was pressed.
    //
    ui32Key = FindKey(psKeyWidget, psKeyboard, i32X, i32Y);

    //
    // See if this is a pointer up message.
    //
    if(ui32Msg == WIDGET_MSG_PTR_UP)
    {
        //
        // Indicate that this key is no longer pressed.
        //
        psKeyWidget->ui32Flags &= ~FLAG_KEY_PRESSED;

        //
        // If filling is enabled for the keyboard button, then redraw the
        // keyboard to show it in its non-pressed state.
        //
        if((psKeyWidget->ui32Style & KEYBOARD_STYLE_FILL) ||
           ((psKeyWidget->ui32Style & KEYBOARD_STYLE_IMG)))
        {
            //
            // Make sure the key is valid.
            //
            if(psKeyWidget->ui32KeyPressed < psKeyboard->ui16NumKeys)
            {
                //
                // Always clear the key that was last marked pressed.
                //
                ButtonPaintText(psWidget,
                   &psKeyboard->uKeys.psKeysText[psKeyWidget->ui32KeyPressed]);
            }
        }

        //
        // If the pointer is still within the button bounds, and it is a
        // release notify button, call the notification function here.
        //
        if((ui32Key < psKeyboard->ui16NumKeys) &&
           (psKeyWidget->ui32Style & KEYBOARD_STYLE_RELEASE_NOTIFY) &&
           psKeyWidget->pfnOnEvent)
        {
            psKeyWidget->pfnOnEvent(psWidget,
                            psKeyboard->uKeys.psKeysText[ui32Key].ui32Code,
                            KEYBOARD_EVENT_RELEASE);
        }
    }

    //
    // See if the given coordinates are within the extents of the key.
    //
    if(ui32Key < psKeyboard->ui16NumKeys)
    {
        //
        // See if this is a pointer down message.
        //
        if(ui32Msg == WIDGET_MSG_PTR_DOWN)
        {
            //
            // Handle a shift to update the keyboard.
            //
            if(psKeyboard->uKeys.psKeysText[ui32Key].ui32Code ==
               UNICODE_CUSTOM_SHIFT)
            {
                if(psKeyWidget->ui32Active == 0)
                {
                    psKeyWidget->ui32Active = 1;
                }
                else if(psKeyWidget->ui32Active == 1)
                {
                    if(psKeyWidget->ui32Flags & FLAG_KEY_CAPSLOCK)
                    {
                        psKeyWidget->ui32Flags &= ~FLAG_KEY_CAPSLOCK;
                        psKeyWidget->ui32Active = 0;
                    }
                    else
                    {
                        psKeyWidget->ui32Flags |= FLAG_KEY_CAPSLOCK;
                    }
                }
                else
                {
                    psKeyWidget->ui32Active = 0;
                }

                //
                // Handle the widget paint request.
                //
                KeyboardPaint(psWidget);
                return(1);
            }
            if(psKeyboard->uKeys.psKeysText[ui32Key].ui32Code ==
               UNICODE_CUSTOM_MODE_TOG)
            {
                if(psKeyWidget->ui32Active == 2)
                {
                    psKeyWidget->ui32Active = 0;
                }
                else
                {
                    psKeyWidget->ui32Active = 2;
                }

                //
                // Handle the widget paint request.
                //
                KeyboardPaint(psWidget);

                return(1);
            }
            else if((psKeyWidget->ui32Active == 1) &&
                    ((psKeyWidget->ui32Flags & FLAG_KEY_CAPSLOCK) == 0))
            {
                psKeyWidget->ui32Flags &= ~FLAG_KEY_CAPSLOCK;
                psKeyWidget->ui32Active = 0;

                //
                // Handle the widget paint request.
                //
                KeyboardPaint(psWidget);
            }

            //
            // Indicate that a key is pressed.
            //
            psKeyWidget->ui32Flags |= FLAG_KEY_PRESSED;

            //
            // If filling is enabled for this keyboard, or if an image is
            // being used and a pressed button image is provided, then redraw
            // the keyboard to show it in its pressed state.
            //
            if((psKeyWidget->ui32Style & KEYBOARD_STYLE_FILL) ||
               ((psKeyWidget->ui32Style & KEYBOARD_STYLE_IMG)))
            {
                //
                // Save the key that was pressed.
                //
                psKeyWidget->ui32KeyPressed = ui32Key;

                ButtonPaintText(psWidget,
                                &psKeyboard->uKeys.psKeysText[ui32Key]);
            }
        }

        //
        // See if there is an OnEvent callback for this widget.
        //
        if(psKeyWidget->pfnOnEvent)
        {
            //
            // If the pointer was just pressed then call the callback.
            //
            if((ui32Msg == WIDGET_MSG_PTR_DOWN) &&
               (psKeyWidget->ui32Style & KEYBOARD_STYLE_PRESS_NOTIFY))
            {
                psKeyWidget->pfnOnEvent(psWidget,
                            psKeyboard->uKeys.psKeysText[ui32Key].ui32Code,
                            KEYBOARD_EVENT_PRESS);
            }

            //
            // See if auto-repeat is enabled for this widget.
            //
            if(psKeyWidget->ui32Style & KEYBOARD_STYLE_AUTO_REPEAT)
            {
                //
                // If the pointer was just pressed, reset the auto-repeat
                // count.
                //
                if(ui32Msg == WIDGET_MSG_PTR_DOWN)
                {
                    psKeyWidget->ui32AutoRepeatCount = 0;
                }

                //
                // See if the pointer was moved.
                //
                else if((ui32Msg == WIDGET_MSG_PTR_MOVE) &&
                        (psKeyWidget->pfnOnEvent) &&
                        (psKeyWidget->ui32Style & KEYBOARD_STYLE_PRESS_NOTIFY))
                {
                    //
                    // Increment the auto-repeat count.
                    //
                    psKeyWidget->ui32AutoRepeatCount++;

                    //
                    // If the auto-repeat count exceeds the auto-repeat delay,
                    // and it is a multiple of the auto-repeat rate, then
                    // call the callback.
                    //
                    if((psKeyWidget->ui32AutoRepeatCount >=
                        psKeyWidget->ui16AutoRepeatDelay) &&
                       (((psKeyWidget->ui32AutoRepeatCount -
                          psKeyWidget->ui16AutoRepeatDelay) %
                         psKeyWidget->ui16AutoRepeatRate) == 0))
                    {
                        psKeyWidget->pfnOnEvent(psWidget,
                              psKeyboard->uKeys.psKeysText[ui32Key].ui32Code,
                              KEYBOARD_EVENT_PRESS);
                    }
                }
            }
        }

        //
        // These coordinates are within the extents of the keyboard widget.
        //
        return(1);
    }

    //
    // These coordinates are not within the extents of the keyboard widget.
    //
    return(0);
}

//*****************************************************************************
//
//! Handles messages for a rectangular keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget.
//! \param ui32Msg is the message.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function receives messages intended for this keyboard widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
int32_t
KeyboardMsgProc(tWidget *psWidget, uint32_t ui32Msg, uint32_t ui32Param1,
                uint32_t ui32Param2)
{
    tKeyboardWidget *psKeyWidget;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    psKeyWidget = (tKeyboardWidget *)psWidget;

    //
    // Determine which message is being sent.
    //
    switch(ui32Msg)
    {
        //
        // The widget paint request has been sent.
        //
        case WIDGET_MSG_PAINT:
        {
            //
            // Only redraw if no buttons are pressed.
            //
            if((psKeyWidget->ui32Flags & FLAG_KEY_PRESSED) == 0)
            {
                //
                // Handle the widget paint request.
                //
                KeyboardPaint(psWidget);
            }

            //
            // Return one to indicate that the message was successfully
            // processed.
            //
            return(1);
        }

        //
        // One of the pointer requests has been sent.
        //
        case WIDGET_MSG_PTR_DOWN:
        case WIDGET_MSG_PTR_MOVE:
        case WIDGET_MSG_PTR_UP:
        {
            //
            // Handle the pointer request, returning the appropriate value.
            //
            return(TextButtonEvent(psWidget, ui32Msg, ui32Param1, ui32Param2));
        }

        //
        // An unknown request has been sent.
        //
        default:
        {
            //
            // Let the default message handler process this message.
            //
            return(WidgetDefaultMsgProc(psWidget, ui32Msg, ui32Param1,
                                        ui32Param2));
        }
    }
}

//*****************************************************************************
//
//! Initializes a keyboard widget.
//!
//! \param psWidget is a pointer to the keyboard widget to initialize.
//! \param psDisplay is a pointer to the display on which to draw the on-screen
//! keyboard.
//! \param i32X is the X coordinate of the upper left corner of the on-screen
//! keyboard.
//! \param i32Y is the Y coordinate of the upper left corner of the on-screen
//! keyboard.
//! \param i32Width is the width of the on-screen keyboard.
//! \param i32Height is the height of the on-screen keyboard.
//!
//! This function initializes the provided keyboard widget so that it is ready
//! to be drawn when requested.
//!
//! \return None.
//
//*****************************************************************************
void
KeyboardInit(tKeyboardWidget *psWidget, const tDisplay *psDisplay,
             int32_t i32X, int32_t i32Y, int32_t i32Width, int32_t i32Height)
{
    uint32_t ui32Idx;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);
    ASSERT(psDisplay);

    //
    // Clear out the widget structure.
    //
    for(ui32Idx = 0; ui32Idx < sizeof(tKeyboardWidget); ui32Idx += 4)
    {
        ((uint32_t *)psWidget)[ui32Idx / 4] = 0;
    }

    //
    // Set the size of the keyboard widget structure.
    //
    psWidget->sBase.i32Size = sizeof(tKeyboardWidget);

    //
    // Mark this widget as fully disconnected.
    //
    psWidget->sBase.psParent = 0;
    psWidget->sBase.psNext = 0;
    psWidget->sBase.psChild = 0;

    //
    // Save the display pointer.
    //
    psWidget->sBase.psDisplay = psDisplay;

    //
    // Set the extents of this keyboard.
    //
    psWidget->sBase.sPosition.i16XMin = i32X;
    psWidget->sBase.sPosition.i16YMin = i32Y;
    psWidget->sBase.sPosition.i16XMax = i32X + i32Width - 1;
    psWidget->sBase.sPosition.i16YMax = i32Y + i32Height - 1;

    //
    // Use the keyboard message handler to process messages.
    //
    psWidget->sBase.pfnMsgProc = KeyboardMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
