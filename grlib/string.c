//*****************************************************************************
//
// string.c - Routines for drawing text.
//
// Copyright (c) 2007-2014 Texas Instruments Incorporated.  All rights reserved.
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
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "grlib/grlib.h"

//*****************************************************************************
//
//! \addtogroup primitives_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The character printed by GrStringDraw in place of any character in the
// string which does not appear in the font.  When using a font which does not
// include this character, a space is left instead.
//
//*****************************************************************************
#define ABSENT_CHAR_REPLACEMENT '.'

//*****************************************************************************
//
//! Determines the width of a string.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param pcString is the string in question.
//! \param i32Length is the length of the string.
//!
//! This function determines the width of a string (or portion of the string)
//! when drawn with a particular font.  The \e i32Length parameter allows a
//! portion of the string to be examined without having to insert a NULL
//! character at the stopping point (would not be possible if the string was
//! located in flash); specifying a length of -1 will cause the width of the
//! entire string to be computed.
//!
//! \return Returns the width of the string in pixels.
//
//*****************************************************************************
#ifdef GRLIB_REMOVE_WIDE_FONT_SUPPORT
int32_t
GrStringWidthGet(const tContext *pContext, const char *pcString,
                 int32_t i32Length)
{
    const uint16_t *pui16Offset;
    const uint8_t *pui8Data;
    uint8_t ui8First, ui8Last, ui8Absent;
    int32_t i32Width;

    //
    // Check the arguments.
    //
    ASSERT(pContext);
    ASSERT(pcString);

    //
    // This function doesn't support wide character fonts or wrapped fonts.
    //
    ASSERT(!(pContext->psFont->ui8Format &&
            (FONT_FMT_WRAPPED | FONT_WIDE_MARKER)));

    //
    // Get some pointers to relevant information in the font to make things
    // easier, and give the compiler a hint about extraneous loads that it can
    // avoid.
    //
    if(pContext->psFont->ui8Format & FONT_EX_MARKER)
    {
        tFontEx *psFont;

        psFont = (tFontEx *)(pContext->psFont);

        pui8Data = psFont->pui8Data;
        pui16Offset = psFont->pui16Offset;
        ui8First = psFont->ui8First;
        ui8Last = psFont->ui8Last;

        //
        // Does the default absent character replacement exist in the font?
        //
        if((ABSENT_CHAR_REPLACEMENT >= ui8First) &&
           (ABSENT_CHAR_REPLACEMENT <= ui8Last))
        {
            //
            // Yes - use the standard character when an absent character is
            // found.
            //
            ui8Absent = ABSENT_CHAR_REPLACEMENT;
        }
        else
        {
            //
            // The default absent character is not present in the font so use
            // the first character (we only use its width here) instead.
            //
            ui8Absent = psFont->ui8First;
        }
    }
    else
    {
        pui8Data = pContext->psFont->pui8Data;
        pui16Offset = pContext->psFont->pui16Offset;
        ui8First = 32;
        ui8Last = 126;
        ui8Absent = ABSENT_CHAR_REPLACEMENT;
    }

    //
    // Loop through the characters in the string.
    //
    for(i32Width = 0; *pcString && i32Length; pcString++, i32Length--)
    {
        //
        // Get a pointer to the font data for the next character from the
        // string.  If there is not a glyph for the next character, replace it
        // with a ".".
        //
        if((*pcString >= ui8First) && (*pcString <= ui8Last))
        {
            //
            // Add the width of this character as drawn with the given font.
            //
            i32Width += pui8Data[pui16Offset[*pcString - ui8First] + 1];
        }
        else
        {
            //
            // This character does not exist in the font so replace it with
            // a '.' instead.  This matches the approach taken in GrStringDraw
            // and ensures that the width returned here represents the
            // rendered dimension of the string.
            //
            i32Width += pui8Data[pui16Offset[ui8Absent - ui8First] + 1];
        }
    }

    //
    // Return the width of the string.
    //
    return(i32Width);
}
#else
int32_t
GrStringWidthGet(const tContext *pContext, const char *pcString,
                 int32_t i32Length)
{
    const uint8_t *pui8Data;
    uint8_t ui8Width, ui8Height, ui8Baseline, ui8Format;
    uint32_t ui32Count, ui32Char, ui32Skip;
    int32_t i32Width;

    //
    // Check the arguments.
    //
    ASSERT(pContext);
    ASSERT(pcString);

    //
    // Initialize our string length.
    //
    i32Width = 0;

    //
    // Set the maximum number of characters we should render.  Note that the
    // value -1 is used to indicate that the function should render until it
    // hits the end of the string so casting it to a uint32_t here is
    // fine since this says keep rendering for 2^32 characters.  We are very
    // unlikely to ever be passed a string this long.
    //
    ui32Count = (uint32_t)i32Length;

    //
    // Loop through each character in the string.
    //
    while(ui32Count)
    {
        //
        // Get the next character to render.
        //
        ui32Char = GrStringNextCharGet(pContext, pcString, ui32Count,
                                       &ui32Skip);

        //
        // If we ran out of characters to render, drop out of the loop.
        //
        if(!ui32Char)
        {
            break;
        }

        //
        // Get information on this glyph.
        //
        pui8Data = GrFontGlyphDataGet(pContext->psFont, ui32Char, &ui8Width);

        //
        // Does the glyph exist?
        //
        if(!pui8Data)
        {
            //
            // No - get the absent character replacement information.
            //
            pui8Data = GrFontGlyphDataGet(pContext->psFont,
                                          ABSENT_CHAR_REPLACEMENT, &ui8Width);

            //
            // Does this character exist in the font?
            //
            if(!pui8Data)
            {
                //
                // No - look for the ASCII/Unicode space character.
                //
                pui8Data = GrFontGlyphDataGet(pContext->psFont, 0x20,
                                              &ui8Width);

                //
                // Does this exist?
                //
                if(!pui8Data)
                {
                    //
                    // No - give up and just pad with a character cell of space.
                    //
                    GrFontInfoGet(pContext->psFont, &ui8Format, &ui8Width,
                                  &ui8Height, &ui8Baseline);
                }
            }
        }

        //
        // Increment our string length.
        //
        i32Width += (int32_t)ui8Width;

        //
        // Move on to the next character.
        //
        pcString += ui32Skip;
        ui32Count -= ui32Skip;
    }

    //
    // Return the width of the string.
    //
    return(i32Width);
}
#endif

//*****************************************************************************
//
//! Draws a string.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param pcString is a pointer to the string to be drawn.
//! \param i32Length is the number of characters from the string that should be
//! drawn on the screen.
//! \param i32X is the X coordinate of the upper left corner of the string
//! position on the screen.
//! \param i32Y is the Y coordinate of the upper left corner of the string
//! position on the screen.
//! \param bOpaque is true of the background of each character should be drawn
//! and false if it should not (leaving the background as is).
//!
//! This function draws a string of text on the screen.  The \e i32Length
//! parameter allows a portion of the string to be examined without having to
//! insert a NULL character at the stopping point (which would not be possible
//! if the string was located in flash); specifying a length of -1 will cause
//! the entire string to be rendered (subject to clipping).
//!
//! \return None.
//
//*****************************************************************************
#ifdef GRLIB_REMOVE_WIDE_FONT_SUPPORT
//
// This version of GrStringDraw supports the original tFont and tFontEx ASCII
// and ISO8859 (8 bit) fonts only.
//
void
GrStringDraw(const tContext *pContext, const char *pcString, int32_t i32Length,
             int32_t i32X, int32_t i32Y, uint32_t bOpaque)
{
    int32_t i32Idx, i32X0, i32Y0, i32Count, i32Off, i32On, i32Bit;
    const uint8_t *pui8Data;
    const uint8_t *pui8Glyphs;
    const uint16_t *pui16Offset;
    uint8_t ui8First, ui8Last, ui8Absent;
    tContext i16Con;

    //
    // Check the arguments.
    //
    ASSERT(pContext);
    ASSERT(pcString);

    //
    // This function doesn't support wide character fonts or wrapped fonts.
    //
    ASSERT(!(pContext->psFont->ui8Format &&
            (FONT_FMT_WRAPPED | FONT_WIDE_MARKER)));

    //
    // Copy the drawing context into a local structure that can be modified.
    //
    i16Con = *pContext;

    //
    // Extract various parameters from the font depending upon whether it's
    // in the tFont or tFontEx format.
    //
    if(pContext->psFont->ui8Format & FONT_EX_MARKER)
    {
        tFontEx *psFont;

        psFont = (tFontEx *)(pContext->psFont);

        pui8Glyphs = psFont->pui8Data;
        pui16Offset = psFont->pui16Offset;
        ui8First = psFont->ui8First;
        ui8Last = psFont->ui8Last;

        //
        // Does the default absent character replacement exist in the font?
        //
        if((ABSENT_CHAR_REPLACEMENT >= ui8First) &&
           (ABSENT_CHAR_REPLACEMENT <= ui8Last))
        {
            //
            // Yes - use the standard character when an absent character is
            // found.
            //
            ui8Absent = ABSENT_CHAR_REPLACEMENT;
        }
        else
        {
            //
            // The default absent character is not present in the font so use
            // the first character instead.
            //
            ui8Absent = psFont->ui8First;
        }
    }
    else
    {
        pui8Glyphs = pContext->psFont->pui8Data;
        pui16Offset = pContext->psFont->pui16Offset;
        ui8First = 32;
        ui8Last = 126;
        ui8Absent = ABSENT_CHAR_REPLACEMENT;
    }

    //
    // Loop through the characters in the string.
    //
    while(*pcString && i32Length--)
    {
        //
        // Stop drawing the string if the right edge of the clipping region has
        // been exceeded.
        //
        if(i32X > i16Con.sClipRegion.i16XMax)
        {
            break;
        }

        //
        // Get a pointer to the font data for the next character from the
        // string.  If there is not a glyph for the next character, replace it
        // with the "absent" character (usually '.').
        //
        if((*pcString >= ui8First) && (*pcString <= ui8Last))
        {
            pui8Data = (pui8Glyphs + pui16Offset[*pcString - ui8First]);
        }
        else
        {
            pui8Data = (pui8Glyphs + pui16Offset[ui8Absent - ui8First]);
        }
        pcString++;

        //
        // See if the entire character is to the left of the clipping region.
        //
        if((i32X + pui8Data[1]) < i16Con.sClipRegion.i16XMin)
        {
            //
            // Increment the X coordinate by the width of the character.
            //
            i32X += pui8Data[1];

            //
            // Go to the next character in the string.
            //
            continue;
        }

        //
        // Loop through the bytes in the encoded data for this glyph.
        //
        for(i32Idx = 2, i32X0 = 0, i32Bit = 0, i32Y0 = 0;
            i32Idx < pui8Data[0]; )
        {
            //
            // See if the bottom of the clipping region has been exceeded.
            //
            if((i32Y + i32Y0) > i16Con.sClipRegion.i16YMax)
            {
                //
                // Stop drawing this character.
                //
                break;
            }

            //
            // See if the font is uncompressed.
            //
            if((i16Con.psFont->ui8Format & ~FONT_EX_MARKER) ==
                FONT_FMT_UNCOMPRESSED)
            {
                //
                // Count the number of off pixels from this position in the
                // glyph image.
                //
                for(i32Off = 0; i32Idx < pui8Data[0]; )
                {
                    //
                    // Get the number of zero pixels at this position.
                    //
                    i32Count = NumLeadingZeros(pui8Data[i32Idx] <<
                                               (24 + i32Bit));

                    //
                    // If there were more than 8, then it is a "false" result
                    // since it counted beyond the end of the current byte.
                    // Therefore, simply limit it to the number of pixels
                    // remaining in this byte.
                    //
                    if(i32Count > 8)
                    {
                        i32Count = 8 - i32Bit;
                    }

                    //
                    // Increment the number of off pixels.
                    //
                    i32Off += i32Count;

                    //
                    // Increment the bit position within the byte.
                    //
                    i32Bit += i32Count;

                    //
                    // See if the end of the byte has been reached.
                    //
                    if(i32Bit == 8)
                    {
                        //
                        // Advance to the next byte and continue counting off
                        // pixels.
                        //
                        i32Bit = 0;
                        i32Idx++;
                    }
                    else
                    {
                        //
                        // Since the end of the byte was not reached, there
                        // must be an on pixel.  Therefore, stop counting off
                        // pixels.
                        //
                        break;
                    }
                }

                //
                // Count the number of on pixels from this position in the
                // glyph image.
                //
                for(i32On = 0; i32Idx < pui8Data[0]; )
                {
                    //
                    // Get the number of one pixels at this location (by
                    // inverting the data and counting the number of zeros).
                    //
                    i32Count = NumLeadingZeros(~(pui8Data[i32Idx] <<
                                                 (24 + i32Bit)));

                    //
                    // If there were more than 8, then it is a "false" result
                    // since it counted beyond the end of the current byte.
                    // Therefore, simply limit it to the number of pixels
                    // remaining in this byte.
                    //
                    if(i32Count > 8)
                    {
                        i32Count = 8 - i32Bit;
                    }

                    //
                    // Increment the number of on pixels.
                    //
                    i32On += i32Count;

                    //
                    // Increment the bit position within the byte.
                    //
                    i32Bit += i32Count;

                    //
                    // See if the end of the byte has been reached.
                    //
                    if(i32Bit == 8)
                    {
                        //
                        // Advance to the next byte and continue counting on
                        // pixels.
                        //
                        i32Bit = 0;
                        i32Idx++;
                    }
                    else
                    {
                        //
                        // Since the end of the byte was not reached, there
                        // must be an off pixel.  Therefore, stop counting on
                        // pixels.
                        //
                        break;
                    }
                }
            }

            //
            // Otherwise, the font is compressed with a pixel RLE scheme.
            //
            else
            {
                //
                // See if this is a byte that encodes some on and off pixels.
                //
                if(pui8Data[i32Idx])
                {
                    //
                    // Extract the number of off pixels.
                    //
                    i32Off = (pui8Data[i32Idx] >> 4) & 15;

                    //
                    // Extract the number of on pixels.
                    //
                    i32On = pui8Data[i32Idx] & 15;

                    //
                    // Skip past this encoded byte.
                    //
                    i32Idx++;
                }

                //
                // Otherwise, see if this is a repeated on pixel byte.
                //
                else if(pui8Data[i32Idx + 1] & 0x80)
                {
                    //
                    // There are no off pixels in this encoding.
                    //
                    i32Off = 0;

                    //
                    // Extract the number of on pixels.
                    //
                    i32On = (pui8Data[i32Idx + 1] & 0x7f) * 8;

                    //
                    // Skip past these two encoded bytes.
                    //
                    i32Idx += 2;
                }

                //
                // Otherwise, this is a repeated off pixel byte.
                //
                else
                {
                    //
                    // Extract the number of off pixels.
                    //
                    i32Off = pui8Data[i32Idx + 1] * 8;

                    //
                    // There are no on pixels in this encoding.
                    //
                    i32On = 0;

                    //
                    // Skip past these two encoded bytes.
                    //
                    i32Idx += 2;
                }
            }

            //
            // Loop while there are any off pixels.
            //
            while(i32Off)
            {
                //
                // See if the bottom of the clipping region has been exceeded.
                //
                if((i32Y + i32Y0) > i16Con.sClipRegion.i16YMax)
                {
                    //
                    // Ignore the remainder of the on pixels.
                    //
                    break;
                }

                //
                // See if there is more than one on pixel that will fit onto
                // the current row.
                //
                if((i32Off > 1) && ((i32X0 + 1) < pui8Data[1]))
                {
                    //
                    // Determine the number of on pixels that will fit on this
                    // row.
                    //
                    i32Count = (((i32X0 + i32Off) > pui8Data[1]) ?
                                pui8Data[1] - i32X0 : i32Off);

                    //
                    // If this row is within the clipping region, draw a
                    // horizontal line that corresponds to the sequence of on
                    // pixels.
                    //
                    if(((i32Y + i32Y0) >= i16Con.sClipRegion.i16YMin) &&
                       bOpaque)
                    {
                        i16Con.ui32Foreground = pContext->ui32Background;
                        GrLineDrawH(&i16Con, i32X + i32X0, i32X + i32X0 +
                                    i32Count - 1, i32Y + i32Y0);
                    }

                    //
                    // Decrement the count of on pixels by the number on this
                    // row.
                    //
                    i32Off -= i32Count;

                    //
                    // Increment the X offset by the number of on pixels.
                    //
                    i32X0 += i32Count;
                }

                //
                // Otherwise, there is only a single on pixel that can be
                // drawn.
                //
                else
                {
                    //
                    // If this pixel is within the clipping region, then draw
                    // it.
                    //
                    if(((i32X + i32X0) >= i16Con.sClipRegion.i16XMin) &&
                       ((i32X + i32X0) <= i16Con.sClipRegion.i16XMax) &&
                       ((i32Y + i32Y0) >= i16Con.sClipRegion.i16YMin) &&
                       bOpaque)
                    {
                        DpyPixelDraw(pContext->psDisplay, i32X + i32X0,
                                     i32Y + i32Y0, pContext->ui32Background);
                    }

                    //
                    // Decrement the count of on pixels.
                    //
                    i32Off--;

                    //
                    // Increment the X offset.
                    //
                    i32X0++;
                }

                //
                // See if the X offset has reached the right side of the
                // character glyph.
                //
                if(i32X0 == pui8Data[1])
                {
                    //
                    // Increment the Y offset.
                    //
                    i32Y0++;

                    //
                    // Reset the X offset to the left side of the character
                    // glyph.
                    //
                    i32X0 = 0;
                }
            }

            //
            // Loop while there are any on pixels.
            //
            while(i32On)
            {
                //
                // See if the bottom of the clipping region has been exceeded.
                //
                if((i32Y + i32Y0) > i16Con.sClipRegion.i16YMax)
                {
                    //
                    // Ignore the remainder of the on pixels.
                    //
                    break;
                }

                //
                // See if there is more than one on pixel that will fit onto
                // the current row.
                //
                if((i32On > 1) && ((i32X0 + 1) < pui8Data[1]))
                {
                    //
                    // Determine the number of on pixels that will fit on this
                    // row.
                    //
                    i32Count = (((i32X0 + i32On) > pui8Data[1]) ?
                                pui8Data[1] - i32X0 : i32On);

                    //
                    // If this row is within the clipping region, draw a
                    // horizontal line that corresponds to the sequence of on
                    // pixels.
                    //
                    if((i32Y + i32Y0) >= i16Con.sClipRegion.i16YMin)
                    {
                        i16Con.ui32Foreground = pContext->ui32Foreground;
                        GrLineDrawH(&i16Con, i32X + i32X0, i32X + i32X0 +
                                    i32Count - 1, i32Y + i32Y0);
                    }

                    //
                    // Decrement the count of on pixels by the number on this
                    // row.
                    //
                    i32On -= i32Count;

                    //
                    // Increment the X offset by the number of on pixels.
                    //
                    i32X0 += i32Count;
                }

                //
                // Otherwise, there is only a single on pixel that can be
                // drawn.
                //
                else
                {
                    //
                    // If this pixel is within the clipping region, then draw
                    // it.
                    //
                    if(((i32X + i32X0) >= i16Con.sClipRegion.i16XMin) &&
                       ((i32X + i32X0) <= i16Con.sClipRegion.i16XMax) &&
                       ((i32Y + i32Y0) >= i16Con.sClipRegion.i16YMin))
                    {
                        DpyPixelDraw(pContext->psDisplay, i32X + i32X0,
                                     i32Y + i32Y0, pContext->ui32Foreground);
                    }

                    //
                    // Decrement the count of on pixels.
                    //
                    i32On--;

                    //
                    // Increment the X offset.
                    //
                    i32X0++;
                }

                //
                // See if the X offset has reached the right side of the
                // character glyph.
                //
                if(i32X0 == pui8Data[1])
                {
                    //
                    // Increment the Y offset.
                    //
                    i32Y0++;

                    //
                    // Reset the X offset to the left side of the character
                    // glyph.
                    //
                    i32X0 = 0;
                }
            }
        }

        //
        // Increment the X coordinate by the width of the character.
        //
        i32X += pui8Data[1];
    }
}
#else
//
// This version of GrStringDraw supports the original tFont and tFontEx ASCII
// and ISO8859 (8 bit) fonts along with new wide character set, relocatable
// fonts.  Support for source and font codepages is also included.
//
void
GrStringDraw(const tContext *pContext, const char *pcString, int32_t i32Length,
             int32_t i32X, int32_t i32Y, uint32_t bOpaque)
{
    ASSERT(pContext);
    ASSERT(pContext->pfnStringRenderer);

    //
    // Call the currently registered string rendering function.
    //
    pContext->pfnStringRenderer(pContext, pcString, i32Length, i32X, i32Y,
                                bOpaque);
}

//*****************************************************************************
//
//! The default text string rendering function.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param pcString is a pointer to the string to be drawn.
//! \param i32Length is the number of characters from the string that should be
//! drawn on the screen.
//! \param i32X is the X coordinate of the upper left corner of the string
//! position on the screen.
//! \param i32Y is the Y coordinate of the upper left corner of the string
//! position on the screen.
//! \param bOpaque is true of the background of each character should be drawn
//! and false if it should not (leaving the background as is).
//!
//! This function acts as the default string rendering function called by
//! GrStringDraw() if no language-specific renderer is registered. It draws a
//! string of text on the screen using the text orientation currently set in
//! the graphics context.  The \e i32Length parameter allows a portion of the
//! string to be examined without having to insert a NULL character at the
//! stopping point (which would not be possible if the string was located in
//! flash); specifying a length of -1 will cause the entire string to be
//! rendered (subject to clipping).
//!
//! Applications are not expected to call this function directly but should
//! call GrStringDraw() instead.  This function is provided as an aid to
//! language-specific renders which may call it to render parts of a string
//! at particular positions after dealing with any language-specific layout
//! issues such as, for example, inserting left-to-right numbers within a
//! right-to-left Arabic string.
//!
//! \return None.
//
//*****************************************************************************
void
GrDefaultStringRenderer(const tContext *pContext, const char *pcString,
                        int32_t i32Length, int32_t i32X, int32_t i32Y,
                        bool bOpaque)
{
    uint8_t ui8Format, ui8Width, ui8MaxWidth, ui8Height, ui8Baseline;
    uint32_t ui32Char, ui32Count, ui32Skip;
    const uint8_t *pui8Data;

    //
    // Check the arguments.
    //
    ASSERT(pContext);
    ASSERT(pcString);

    //
    // Get information on the font we are rendering the text in.
    //
    GrFontInfoGet(pContext->psFont, &ui8Format, &ui8MaxWidth, &ui8Height,
                  &ui8Baseline);

    //
    // If the string is completely outside the clipping region, don't even
    // start rendering it.
    //
    if((i32Y > pContext->sClipRegion.i16YMax) ||
       ((i32Y + ui8Height) < pContext->sClipRegion.i16YMin))
    {
        return;
    }

    //
    // Set the maximum number of characters we should render.  Note that the
    // value -1 is used to indicate that the function should render until it
    // hits the end of the string so casting it to an uint32_t here is
    // fine since this says keep rendering for 2^32 characters.  We are very
    // unlikely to ever be passed a string this long.
    //
    ui32Count = (uint32_t)i32Length;

    //
    // Loop through each character in the string.
    //
    while(ui32Count)
    {
        //
        // Get the next character to render.
        //
        ui32Char = GrStringNextCharGet(pContext, pcString, ui32Count,
                                       &ui32Skip);

        //
        // If we ran out of characters to render, return immediately.
        //
        if(!ui32Char)
        {
            return;
        }

        //
        // If we are already outside the clipping region, exit early.
        //
        if(i32X >= pContext->sClipRegion.i16XMax)
        {
            return;
        }

        //
        // Get the glyph data pointer for this character.
        //
        pui8Data = GrFontGlyphDataGet(pContext->psFont, ui32Char, &ui8Width);

        //
        // Does this glyph exist in the font?
        //
        if(!pui8Data)
        {
            //
            // Look for the character we are supposed to use in place of absent
            // glyphs.
            //
            pui8Data = GrFontGlyphDataGet(pContext->psFont,
                                          ABSENT_CHAR_REPLACEMENT, &ui8Width);

            //
            // Does this glyph exist in the font?
            //
            if(!pui8Data)
            {
                //
                // Last chance - look for the space character.
                //
                pui8Data = GrFontGlyphDataGet(pContext->psFont, ' ',
                                              &ui8Width);
            }
        }

        //
        // Did we find something to render?
        //
        if(pui8Data)
        {
            GrFontGlyphRender(pContext, pui8Data, i32X, i32Y,
                              (ui8Format & FONT_FMT_PIXEL_RLE) ? true : false,
                              bOpaque);
            i32X += ui8Width;
        }
        else
        {
            //
            // Leave a space in place of the undefined glyph.
            //
            i32X += ui8MaxWidth;
        }

        //
        // Move on to the next character.
        //
        pcString += ui32Skip;
        ui32Count -= ui32Skip;
    }
}

//*****************************************************************************
//
//! Returns the codepoint of the first character in a string.
//!
//! \param pContext points to the graphics context in use.
//! \param pcString points to the first byte of the string from which the
//!        next character is to be parsed.
//! \param ui32Count provides the number of bytes in the pcString buffer.
//! \param pui32Skip points to storage which will be written with the number of
//!        bytes that must be skipped in the string buffer to move past the
//!        current character.
//!
//! This function is used to walk through a string extracting one character at
//! a time.  The input string is assumed to be encoded using the currently-
//! selected string codepage (as set via a call to the GrStringCodepageSet()
//! function).  The value returned is the codepoint of the first character in
//! the string as mapped into the current font's codepage.  This may be passed
//! to the GrFontGlyphDataGet() function to retrieve the glyph data for the
//! character.
//!
//! Since variable length encoding schemes such as UTF-8 are supported, this
//! function also returns information on the size of the character that has
//! been parsed, allowing the caller to increment the string pointer by the
//! relevant amount before querying the next character in the string.
//!
//! \return Returns the font codepoint representing the first character in the
//! string or 0 if no valid character was found.
//!
//*****************************************************************************
uint32_t
GrStringNextCharGet(const tContext *pContext, const char *pcString,
                    uint32_t ui32Count, uint32_t *pui32Skip)
{
    ASSERT(pContext);
    ASSERT(pcString);
    ASSERT(pui32Skip);

    //
    // If the string is empty, return immediately.
    //
    if(!ui32Count)
    {
        return(0);
    }

    //
    // Has a codepage mapping table been registered for this context?
    //
    if(pContext->pCodePointMapTable)
    {
        //
        // Yes - use the relevant mapping function
        //
        return(pContext->pCodePointMapTable[pContext->ui8CodePointMap].
               pfnMapChar(pcString, ui32Count, pui32Skip));
    }
    else
    {
        //
        // No codepage mapping table has been registered so fall back on the
        // assumption that we are using ASCII or ISO8859-1 for both the
        // string and font codepages (i.e. the legacy case).
        //
        *pui32Skip = 1;
        return((uint32_t)*pcString);
    }
}

//*****************************************************************************
//
//! Renders a single character glyph on the display at a given position.
//!
//! \param pContext points to the graphics context in use.
//! \param pui8Data points to the first byte of data for the glyph to be
//!        rendered.
//! \param i32X is the X coordinate of the top left pixel of the glyph.
//! \param i32Y is the Y coordinate of the top left pixel of the glyph.
//! \param bCompressed is \b true if the data pointed to by \b pui8Data is in
//!        compressed format or \b false if uncompressed.
//! \param bOpaque is \b true of background pixels are to be written or \b
//!        false if only foreground pixels are drawn.
//!
//! This function is included as an aid to language-specific string rendering
//! functions.  Applications are expected to render strings and characters
//! using calls to GrStringDraw or GrStringDrawCentered and should not call
//! this function directly.
//!
//! A string rendering function may call this low level API to place a single
//! character glyph on the display at a particular position.  The rendered
//! glyph is subject to the clipping rectangle currently set in the passed
//! graphics context.  Rendering colors are also taken from the context
//! structure.  Glyph data pointed to by \b pui8Data should be retrieved using
//! a call to GrFontGlyphDataGet().
//!
//! \return None.
//
//*****************************************************************************
void
GrFontGlyphRender(const tContext *pContext, const uint8_t *pui8Data,
                  int32_t i32X, int32_t i32Y, bool bCompressed,
                  bool bOpaque)
{
    int32_t i32Idx, i32X0, i32Y0, i32Count, i32Off, i32On, i32Bit;
    int32_t i32ClipX1, i32ClipX2;

    //
    // Check the arguments.
    //
    ASSERT(pContext);
    ASSERT(pui8Data);

    //
    // Stop drawing the string if the right edge of the clipping region has
    // been exceeded.
    //
    if(i32X > pContext->sClipRegion.i16XMax)
    {
        return;
    }

    //
    // See if the entire character is to the left of the clipping region.
    //
    if((i32X + pui8Data[1]) < pContext->sClipRegion.i16XMin)
    {
        return;
    }

    //
    // Loop through the bytes in the encoded data for this glyph.
    //
    for(i32Idx = 2, i32X0 = 0, i32Bit = 0, i32Y0 = 0; i32Idx < pui8Data[0]; )
    {
        //
        // See if the bottom of the clipping region has been exceeded.
        //
        if((i32Y + i32Y0) > pContext->sClipRegion.i16YMax)
        {
            //
            // Stop drawing this character.
            //
            break;
        }

        //
        // See if the font is uncompressed.
        //
        if(!bCompressed)
        {
            //
            // Count the number of off pixels from this position in the
            // glyph image.
            //
            for(i32Off = 0; i32Idx < pui8Data[0]; )
            {
                //
                // Get the number of zero pixels at this position.
                //
                i32Count = NumLeadingZeros(pui8Data[i32Idx] << (24 + i32Bit));

                //
                // If there were more than 8, then it is a "false" result
                // since it counted beyond the end of the current byte.
                // Therefore, simply limit it to the number of pixels
                // remaining in this byte.
                //
                if(i32Count > 8)
                {
                    i32Count = 8 - i32Bit;
                }

                //
                // Increment the number of off pixels.
                //
                i32Off += i32Count;

                //
                // Increment the bit position within the byte.
                //
                i32Bit += i32Count;

                //
                // See if the end of the byte has been reached.
                //
                if(i32Bit == 8)
                {
                    //
                    // Advance to the next byte and continue counting off
                    // pixels.
                    //
                    i32Bit = 0;
                    i32Idx++;
                }
                else
                {
                    //
                    // Since the end of the byte was not reached, there
                    // must be an on pixel.  Therefore, stop counting off
                    // pixels.
                    //
                    break;
                }
            }

            //
            // Count the number of on pixels from this position in the
            // glyph image.
            //
            for(i32On = 0; i32Idx < pui8Data[0]; )
            {
                //
                // Get the number of one pixels at this location (by
                // inverting the data and counting the number of zeros).
                //
                i32Count = NumLeadingZeros(~(pui8Data[i32Idx] <<
                                             (24 + i32Bit)));

                //
                // If there were more than 8, then it is a "false" result
                // since it counted beyond the end of the current byte.
                // Therefore, simply limit it to the number of pixels
                // remaining in this byte.
                //
                if(i32Count > 8)
                {
                    i32Count = 8 - i32Bit;
                }

                //
                // Increment the number of on pixels.
                //
                i32On += i32Count;

                //
                // Increment the bit position within the byte.
                //
                i32Bit += i32Count;

                //
                // See if the end of the byte has been reached.
                //
                if(i32Bit == 8)
                {
                    //
                    // Advance to the next byte and continue counting on
                    // pixels.
                    //
                    i32Bit = 0;
                    i32Idx++;
                }
                else
                {
                    //
                    // Since the end of the byte was not reached, there
                    // must be an off pixel.  Therefore, stop counting on
                    // pixels.
                    //
                    break;
                }
            }
        }

        //
        // Otherwise, the font is compressed with a pixel RLE scheme.
        //
        else
        {
            //
            // See if this is a byte that encodes some on and off pixels.
            //
            if(pui8Data[i32Idx])
            {
                //
                // Extract the number of off pixels.
                //
                i32Off = (pui8Data[i32Idx] >> 4) & 15;

                //
                // Extract the number of on pixels.
                //
                i32On = pui8Data[i32Idx] & 15;

                //
                // Skip past this encoded byte.
                //
                i32Idx++;
            }

            //
            // Otherwise, see if this is a repeated on pixel byte.
            //
            else if(pui8Data[i32Idx + 1] & 0x80)
            {
                //
                // There are no off pixels in this encoding.
                //
                i32Off = 0;

                //
                // Extract the number of on pixels.
                //
                i32On = (pui8Data[i32Idx + 1] & 0x7f) * 8;

                //
                // Skip past these two encoded bytes.
                //
                i32Idx += 2;
            }

            //
            // Otherwise, this is a repeated off pixel byte.
            //
            else
            {
                //
                // Extract the number of off pixels.
                //
                i32Off = pui8Data[i32Idx + 1] * 8;

                //
                // There are no on pixels in this encoding.
                //
                i32On = 0;

                //
                // Skip past these two encoded bytes.
                //
                i32Idx += 2;
            }
        }

        //
        // Loop while there are any off pixels.
        //
        while(i32Off)
        {
            //
            // See if the bottom of the clipping region has been exceeded.
            //
            if((i32Y + i32Y0) > pContext->sClipRegion.i16YMax)
            {
                //
                // Ignore the remainder of the on pixels.
                //
                break;
            }

            //
            // See if there is more than one on pixel that will fit onto
            // the current row.
            //
            if((i32Off > 1) && ((i32X0 + 1) < pui8Data[1]))
            {
                //
                // Determine the number of on pixels that will fit on this
                // row.
                //
                i32Count = (((i32X0 + i32Off) > pui8Data[1]) ?
                            pui8Data[1] - i32X0 : i32Off);

                //
                // If this row is within the clipping region, draw a
                // horizontal line that corresponds to the sequence of on
                // pixels.
                //
                if(((i32Y + i32Y0) >= pContext->sClipRegion.i16YMin) &&
                   bOpaque)
                {
                    if((i32X + i32X0) < pContext->sClipRegion.i16XMin)
                    {
                        i32ClipX1 = pContext->sClipRegion.i16XMin;
                    }
                    else
                    {
                        i32ClipX1 = i32X + i32X0;
                    }

                    if((i32X + i32X0 + i32Count - 1) >
                       pContext->sClipRegion.i16XMax)
                    {
                        i32ClipX2 = pContext->sClipRegion.i16XMax;
                    }
                    else
                    {
                        i32ClipX2 = i32X + i32X0 + i32Count - 1;
                    }

                    DpyLineDrawH(pContext->psDisplay, i32ClipX1, i32ClipX2,
                                 i32Y + i32Y0, pContext->ui32Background);
                }

                //
                // Decrement the count of on pixels by the number on this
                // row.
                //
                i32Off -= i32Count;

                //
                // Increment the X offset by the number of on pixels.
                //
                i32X0 += i32Count;
            }

            //
            // Otherwise, there is only a single on pixel that can be
            // drawn.
            //
            else
            {
                //
                // If this pixel is within the clipping region, then draw
                // it.
                //
                if(((i32X + i32X0) >= pContext->sClipRegion.i16XMin) &&
                   ((i32X + i32X0) <= pContext->sClipRegion.i16XMax) &&
                   ((i32Y + i32Y0) >= pContext->sClipRegion.i16YMin) &&
                   bOpaque)
                {
                    DpyPixelDraw(pContext->psDisplay, i32X + i32X0,
                                 i32Y + i32Y0, pContext->ui32Background);
                }

                //
                // Decrement the count of on pixels.
                //
                i32Off--;

                //
                // Increment the X offset.
                //
                i32X0++;
            }

            //
            // See if the X offset has reached the right side of the
            // character glyph.
            //
            if(i32X0 == pui8Data[1])
            {
                //
                // Increment the Y offset.
                //
                i32Y0++;

                //
                // Reset the X offset to the left side of the character
                // glyph.
                //
                i32X0 = 0;
            }
        }

        //
        // Loop while there are any on pixels.
        //
        while(i32On)
        {
            //
            // See if the bottom of the clipping region has been exceeded.
            //
            if((i32Y + i32Y0) > pContext->sClipRegion.i16YMax)
            {
                //
                // Ignore the remainder of the on pixels.
                //
                break;
            }

            //
            // See if there is more than one on pixel that will fit onto
            // the current row.
            //
            if((i32On > 1) && ((i32X0 + 1) < pui8Data[1]))
            {
                //
                // Determine the number of on pixels that will fit on this
                // row.
                //
                i32Count = (((i32X0 + i32On) > pui8Data[1]) ?
                            pui8Data[1] - i32X0 : i32On);

                //
                // If this row is within the clipping region, draw a
                // horizontal line that corresponds to the sequence of on
                // pixels.
                //
                if((i32Y + i32Y0) >= pContext->sClipRegion.i16YMin)
                {
                    if((i32X + i32X0) >= pContext->sClipRegion.i16XMin)
                    {
                        i32ClipX1 = i32X + i32X0;
                    }
                    else
                    {
                        i32ClipX1 = pContext->sClipRegion.i16XMin;
                    }

                    if((i32X + i32X0 + i32Count - 1) >
                       pContext->sClipRegion.i16XMax)
                    {
                        i32ClipX2 = pContext->sClipRegion.i16XMax;
                    }
                    else
                    {
                        i32ClipX2 = i32X + i32X0 + i32Count - 1;
                    }

                    DpyLineDrawH(pContext->psDisplay, i32ClipX1, i32ClipX2,
                                 i32Y + i32Y0, pContext->ui32Foreground);
                }

                //
                // Decrement the count of on pixels by the number on this
                // row.
                //
                i32On -= i32Count;

                //
                // Increment the X offset by the number of on pixels.
                //
                i32X0 += i32Count;
            }

            //
            // Otherwise, there is only a single on pixel that can be
            // drawn.
            //
            else
            {
                //
                // If this pixel is within the clipping region, then draw
                // it.
                //
                if(((i32X + i32X0) >= pContext->sClipRegion.i16XMin) &&
                   ((i32X + i32X0) <= pContext->sClipRegion.i16XMax) &&
                   ((i32Y + i32Y0) >= pContext->sClipRegion.i16YMin))
                {
                    DpyPixelDraw(pContext->psDisplay, i32X + i32X0,
                                 i32Y + i32Y0, pContext->ui32Foreground);
                }

                //
                // Decrement the count of on pixels.
                //
                i32On--;

                //
                // Increment the X offset.
                //
                i32X0++;
            }

            //
            // See if the X offset has reached the right side of the
            // character glyph.
            //
            if(i32X0 == pui8Data[1])
            {
                //
                // Increment the Y offset.
                //
                i32Y0++;

                //
                // Reset the X offset to the left side of the character
                // glyph.
                //
                i32X0 = 0;
            }
        }
    }
}

//*****************************************************************************
//
//! Retrieves header information from a font.
//!
//! \param psFont points to the font whose information is to be queried.
//! \param pui8Format points to storage which will be written with the font
//!        format.
//! \param pui8MaxWidth points to storage which will be written with the
//!        maximum character width for the font in pixels.
//! \param pui8Height points to storage which will be written with the height
//!        of the font character cell in pixels.
//! \param pui8Baseline points to storage which will be written with the font
//!        baseline offset in pixels.
//!
//! This function may be used to retrieve information about a given font.  The
//! \e psFont parameter may point to any supported font format including wrapped
//! fonts described using a \e tFontWrapper structure (with the pointer cast
//! to a tFont pointer).
//!
//! \return None.
//
//*****************************************************************************
void
GrFontInfoGet(const tFont *psFont, uint8_t *pui8Format, uint8_t *pui8MaxWidth,
              uint8_t *pui8Height, uint8_t *pui8Baseline)
{
    //
    // Parameter sanity checks.
    //
    ASSERT(psFont);
    ASSERT(pui8Format);
    ASSERT(pui8MaxWidth);
    ASSERT(pui8Height);
    ASSERT(pui8Baseline);

    //
    // Is this a wrapped font?
    //
    if(psFont->ui8Format & FONT_FMT_WRAPPED)
    {
        tFontWrapper *psFontWrapper;

        //
        // Yes - get a pointer to the relevant header type and call the
        // access function to retrieve the font information.
        //
        psFontWrapper = (tFontWrapper *)psFont;
        psFontWrapper->pFuncs->pfnFontInfoGet(psFontWrapper->pui8FontId,
                                              pui8Format, pui8MaxWidth,
                                              pui8Height, pui8Baseline);
    }
    else
    {
        //
        // This is not a wrapped font so we can read the information directly
        // from the font structure passed.
        //
        *pui8Format = psFont->ui8Format;
        *pui8MaxWidth = psFont->ui8MaxWidth;
        *pui8Height = psFont->ui8Height;
        *pui8Baseline = psFont->ui8Baseline;
    }
}

//*****************************************************************************
//
//! Gets the baseline of a font.
//!
//! \param psFont is a pointer to the font to query.
//!
//! This function determines the baseline position of a font.  The baseline is
//! the offset between the top of the font and the bottom of the capital
//! letters.  The only font data that exists below the baseline are the
//! descenders on some lower-case letters (such as ``y'').
//!
//! \return Returns the baseline of the font, in pixels.
//
//*****************************************************************************
uint32_t
GrFontBaselineGet(const tFont *psFont)
{
    uint8_t ui8Format, ui8Width, ui8Height, ui8Baseline;

    ASSERT(psFont);

    if(psFont->ui8Format != FONT_FMT_WRAPPED)
    {
        return((uint32_t)(psFont->ui8Baseline));
    }
    else
    {
        tFontWrapper *pWrap;

        pWrap = (tFontWrapper *)psFont;
        pWrap->pFuncs->pfnFontInfoGet(pWrap->pui8FontId, &ui8Format, &ui8Width,
                                      &ui8Height, &ui8Baseline);
        return((uint32_t)ui8Baseline);
    }
}

//*****************************************************************************
//
//! Gets the height of a font.
//!
//! \param psFont is a pointer to the font to query.
//!
//! This function determines the height of a font.  The height is the offset
//! between the top of the font and the bottom of the font, including any
//! ascenders and descenders.
//!
//! \return Returns the height of the font, in pixels.
//
//*****************************************************************************
uint32_t
GrFontHeightGet(const tFont *psFont)
{
    uint8_t ui8Format, ui8Width, ui8Height, ui8Baseline;

    ASSERT(psFont);

    if(psFont->ui8Format != FONT_FMT_WRAPPED)
    {
        return(psFont->ui8Height);
    }
    else
    {
        tFontWrapper *pWrap;

        pWrap = (tFontWrapper *)psFont;
        pWrap->pFuncs->pfnFontInfoGet(pWrap->pui8FontId, &ui8Format, &ui8Width,
                                      &ui8Height, &ui8Baseline);
        return((uint32_t)ui8Height);
    }
}

//*****************************************************************************
//
//! Gets the maximum width of a font.
//!
//! \param psFont is a pointer to the font to query.
//!
//! This function determines the maximum width of a font.  The maximum width is
//! the width of the widest individual character in the font.
//!
//! \return Returns the maximum width of the font, in pixels.
//
//*****************************************************************************
uint32_t
GrFontMaxWidthGet(const tFont *psFont)
{
    uint8_t ui8Format, ui8Width, ui8Height, ui8Baseline;

    ASSERT(psFont);

    if(psFont->ui8Format != FONT_FMT_WRAPPED)
    {
        return(psFont->ui8MaxWidth);
    }
    else
    {
        tFontWrapper *pWrap;

        pWrap = (tFontWrapper *)psFont;
        pWrap->pFuncs->pfnFontInfoGet(pWrap->pui8FontId, &ui8Format, &ui8Width,
                                      &ui8Height, &ui8Baseline);
        return((uint32_t)ui8Width);
    }
}

//*****************************************************************************
//
// Retrieves a pointer to the data for a specific glyph in a tFont or tFontEx
// font.
//
// \param psFont points to the font whose glyph is to be queried.
// \param ui32CodePoint idenfities the specific glyph whose data is being
//        queried.
// \param pui8Width points to storage which will be written with the
//        width of the requested glyph in pixels.
//
// This function may be used to retrieve the pixel data for a particular glyph
// in a font described using a tFont or tFontEx type.
//
// \return Returns a pointer to the data for the requested glyph or NULL if
// the glyph does not exist in the font.
//
//*****************************************************************************
static const uint8_t *
FontGlyphDataGet(const tFont *psFont, uint32_t ui32CodePoint,
                 uint8_t *pui8Width)
{
    const uint8_t *pui8Glyphs;
    const uint16_t *pui16Offset;
    uint8_t ui8First, ui8Last;
    const tFontEx *psFontEx;
    const uint8_t *pui8Data;

    //
    // Extract various parameters from the font depending upon whether it's
    // in the tFont or tFontEx format.
    //
    if(psFont->ui8Format & FONT_EX_MARKER)
    {
        psFontEx = (const tFontEx *)psFont;

        pui8Glyphs = psFontEx->pui8Data;
        pui16Offset = psFontEx->pui16Offset;
        ui8First = psFontEx->ui8First;
        ui8Last = psFontEx->ui8Last;
    }
    else
    {
        pui8Glyphs = psFont->pui8Data;
        pui16Offset = psFont->pui16Offset;
        ui8First = 32;
        ui8Last = 126;
    }

    //
    // Does the codepoint passed exist in the font?
    //
    if((ui32CodePoint >= ui8First) && (ui32CodePoint <= ui8Last))
    {
        //
        // Yes - return a pointer to the glyph data for the character.
        //
        pui8Data = pui8Glyphs + pui16Offset[ui32CodePoint - ui8First];
        *pui8Width = pui8Data[1];
        return(pui8Data);
    }
    else
    {
        //
        // No - the glyph doesn't exist so return NULL to indicate this.
        //
        return(0);
    }
}

//*****************************************************************************
//
// Retrieves a pointer to the data for a specific glyph in a tFontWide font.
//
// \param psFont points to the font whose glyph is to be queried.
// \param ui32CodePoint idenfities the specific glyph whose data is being
//        queried.
// \param pui8Width points to storage which will be written with the
//        width of the requested glyph in pixels.
//
// This function may be used to retrieve the pixel data for a particular glyph
// in a font described using a tFontWide type.
//
// \return Returns a pointer to the data for the requested glyph or NULL if
// the glyph does not exist in the font.
//
//*****************************************************************************
static const uint8_t *
FontWideGlyphDataGet(const tFontWide *psFont, uint32_t ui32CodePoint,
                     uint8_t *pui8Width)
{
    const uint8_t *pui8Data;
    tFontBlock *pBlock;
    uint32_t *pui32OffsetTable;
    uint32_t ui32Loop;
    uint32_t ui32Offset;

    //
    // Get a pointer to the first block description in the font.
    //
    pBlock = (tFontBlock *)(psFont + 1);

    //
    // Run through the blocks of the font looking for the one that contains
    // our codepoint.
    //
    for(ui32Loop = 0; ui32Loop < psFont->ui16NumBlocks; ui32Loop++)
    {
        //
        // Does the codepoint lie within this block?
        //
        if((ui32CodePoint >= pBlock[ui32Loop].ui32StartCodepoint) &&
           (ui32CodePoint < (pBlock[ui32Loop].ui32StartCodepoint +
                             pBlock[ui32Loop].ui32NumCodepoints)))
        {
            //
            // Yes - drop out of the loop early.
            //
            break;
        }
    }

    //
    // Did we find the block?
    //
    if(ui32Loop == psFont->ui16NumBlocks)
    {
        //
        // No - return NULL to indicate that the character wasn't found.
        //
        return(0);
    }

    //
    // Get the offset to the glyph data via the block's offset table.
    //
    pui32OffsetTable = (uint32_t *)((uint8_t *)psFont +
                                    pBlock[ui32Loop].ui32GlyphTableOffset);
    ui32Offset = pui32OffsetTable[ui32CodePoint -
                                  pBlock[ui32Loop].ui32StartCodepoint];

    //
    // Is the offset non-zero? Zero offset indicates that the glyph is not
    // encoded in the font.
    //
    if(ui32Offset)
    {
        //
        // The offset is not 0 so this glyph does exist. Return a pointer to
        // its data.
        //
        pui8Data = (const uint8_t *)pui32OffsetTable + ui32Offset;
        *pui8Width = pui8Data[1];
        return(pui8Data);
    }
    else
    {
        //
        // The glyph offset was 0 so this implies that the glyph does not
        // exist.  Return NULL to indicate this.
        //
        return(0);
    }
}

//*****************************************************************************
//
//! Retrieves a pointer to the data for a specific font glyph.
//!
//! \param psFont points to the font whose glyph is to be queried.
//! \param ui32CodePoint idenfities the specific glyph whose data is being
//!        queried.
//! \param pui8Width points to storage which will be written with the
//!        width of the requested glyph in pixels.
//!
//! This function may be used to retrieve the pixel data for a particular glyph
//! in a font.  The pointer returned may be passed to GrFontGlyphRender to
//! draw the glyph on the display.  The format of the data may be determined
//! from the font format returned via a call to GrFontInfoGet().
//!
//! \return Returns a pointer to the data for the requested glyph or NULL if
//! the glyph does not exist in the font.
//
//*****************************************************************************
const uint8_t *
GrFontGlyphDataGet(const tFont *psFont, uint32_t ui32CodePoint,
                   uint8_t *pui8Width)
{
    ASSERT(psFont);
    ASSERT(pui8Width);

    //
    // What type of font are we dealing with here?
    //
    if(psFont->ui8Format == FONT_FMT_WRAPPED)
    {
        tFontWrapper *psFontWrapper;

        //
        // This is a wrapped font so call the access function required to get
        // the required information.
        //
        psFontWrapper = (tFontWrapper *)psFont;
        return(psFontWrapper->pFuncs->
               pfnFontGlyphDataGet(psFontWrapper->pui8FontId, ui32CodePoint,
                                   pui8Width));
    }
    else if (psFont->ui8Format & FONT_WIDE_MARKER)
    {
        //
        // This is a wide character set font so call the relevant function to
        // retrieve the glyph data pointer.
        //
        return(FontWideGlyphDataGet((const tFontWide *)psFont, ui32CodePoint,
                                    pui8Width));
    }
    else
    {
        //
        // This is an 8 bit font so call the relevant function to retrieve the
        // glyph data pointer.
        //
        return(FontGlyphDataGet(psFont, ui32CodePoint, pui8Width));
    }
}

//*****************************************************************************
//
//! Returns the codepage supported by the given font.
//!
//! \param psFont points to the font whose codepage is to be returned.
//!
//! This function returns the codepage supported by the font whose pointer is
//! passed.  The codepage defines the mapping between a given character code
//! and the glyph that represents it.  Standard codepages are identified by
//! labels of the form \b CODEPAGE_xxxx.  Fonts may also be encoded using
//! application specific codepages with values of 0x8000 or higher.
//!
//! \return Returns the font codepage identifier.
//!
//*****************************************************************************
uint16_t
GrFontCodepageGet(const tFont *psFont)
{
    ASSERT(psFont);

    //
    // Is this a wide character set font?
    //
    if(psFont->ui8Format & FONT_WIDE_MARKER)
    {
        //
        // Yes - read the font codepage from the header.
        //
        return(((tFontWide *)psFont)->ui16Codepage);
    }
    else if(psFont->ui8Format & FONT_FMT_WRAPPED)
    {
        //
        // This is a wrapper-based font so call the access function to get
        // its codepage.
        //
        ASSERT(((tFontWrapper *)psFont)->pFuncs->pfnFontCodepageGet);

        return(((tFontWrapper *)psFont)->pFuncs->
               pfnFontCodepageGet(((tFontWrapper *)psFont)->pui8FontId));
    }
    else
    {
        //
        // No - this is an old format font so just return ISO8859-1.  This is
        // compatible with ASCII so should be benign.
        //
        return(CODEPAGE_ISO8859_1);
    }
}

//*****************************************************************************
//
// Determines which codepoint mapping function to use based on the current
// source codepage and font selection in the context.
//
//*****************************************************************************
static int32_t
UpdateContextCharMapping(tContext *pContext)
{
    uint32_t ui32Loop;
    uint16_t ui16FontCodepage;

    //
    // Make sure we have a font selected.
    //
    if(!pContext->psFont)
    {
        //
        // No font is yet selected so we can't determine the codepage map to
        // use.
        //
        return(-1);
    }

    //
    // Get the current font's codepage.
    //
    ui16FontCodepage = GrFontCodepageGet(pContext->psFont);

    //
    // Look through the codepage mapping functions we have been given and
    // find an appropriate one.
    //
    for(ui32Loop = 0; ui32Loop < pContext->ui8NumCodePointMaps; ui32Loop++)
    {
        if((pContext->pCodePointMapTable[ui32Loop].ui16SrcCodepage ==
            pContext->ui16Codepage) &&
           (pContext->pCodePointMapTable[ui32Loop].ui16FontCodepage ==
            ui16FontCodepage))
        {
            //
            // We found a suitable mapping function so remember it.
            //
            pContext->ui8CodePointMap = (uint8_t)ui32Loop;
            return((int32_t)ui32Loop);
        }
    }

    //
    // If we get here, no suitable mapping function could be found.  Set things
    // up to use the first mapping function (even though it's not right).
    //
    pContext->ui8CodePointMap = 0;
    return(-1);
}
//*****************************************************************************
//
//! Returns the number of blocks of character encoded by a font.
//!
//! \param psFont is a pointer to the font which is to be queried.
//!
//! This function may be used to query the number of contiguous blocks of
//! codepoints (characters) encoded by a given font.  This is primarily of use
//! to applications which wish to parse fonts directly to, for example, display
//! all glyphs in the font.  It is unlikely that applications which wish to
//! display text strings would need to call this function.
//!
//! The \e psFont parameter may point to any supported font format including
//! wrapped fonts described using the \e tFontWrapper structure (assuming, of
//! course, that the structure pointer is cast to a \e tFont pointer).
//!
//! \return Returns the number of blocks of codepoints within the font.
//
//*****************************************************************************
uint16_t
GrFontNumBlocksGet(const tFont *psFont)
{
    ASSERT(psFont);

    //
    // Is this a wide character set font?
    //
    if(psFont->ui8Format & FONT_WIDE_MARKER)
    {
        //
        // Yes - read the number of blocks from the header.
        //
        return(((tFontWide *)psFont)->ui16NumBlocks);
    }
    else if(psFont->ui8Format & FONT_FMT_WRAPPED)
    {
        //
        // This is a wrapper-based font so call the access function to get
        // the information.
        //
        ASSERT(((tFontWrapper *)psFont)->pFuncs->pfnFontNumBlocksGet);

        return(((tFontWrapper *)psFont)->pFuncs->
               pfnFontNumBlocksGet(((tFontWrapper *)psFont)->pui8FontId));
    }
    else
    {
        //
        // No - this is an old format font so it only supports a single block
        // of characters.
        //
        return(1);
    }
}

//*****************************************************************************
//
//! Returns the number of blocks of character encoded by a font.
//!
//! \param psFont is a pointer to the font which is to be queried.
//! \param ui16BlockIndex is the index of the codepoint block to be queried.
//! \param pui32Start points to storage which is written with the codepoint
//! number of the first glyph in the block.
//!
//! This function may be used to query the contents of a particular block of
//! codepoints (characters) encoded by a given font.  This is primarily of use
//! to applications which wish to parse fonts directly to, for example, display
//! all glyphs in the font.  It is unlikely that applications which wish to
//! display text strings would need to call this function.
//!
//! The number of blocks in the font may be queried by calling
//! GrFontNumBlocksGet(). The \e ui16BlockIndex selects a block and valid
//! values are from, 0 to the number of blocks in the font - 1.
//!
//! The \e pui32Start pointer is written with the codepoint number of the
//! first glyph in the given block.  It is assumed that each block contains
//! a contiguous block of glyphs so the actual codepoints represented in the
//! block will be from \e *pui32Start to (\e *pui32Start + return value - 1).
//!
//! The \e psFont parameter may point to any supported font format including
//! wrapped fonts described using the \e tFontWrapper structure (assuming, of
//! course, that the structure pointer is cast to a \e tFont pointer).
//!
//! \return Returns the number of blocks of codepoints within the block.
//
//*****************************************************************************
uint32_t
GrFontBlockCodepointsGet(const tFont *psFont, uint16_t ui16BlockIndex,
                         uint32_t *pui32Start)
{
    ASSERT(psFont);
    ASSERT(pui32Start);

    //
    // Is this a wide character set font?
    //
    if(psFont->ui8Format & FONT_WIDE_MARKER)
    {
        tFontWide *psFontWide;
        tFontBlock *pBlock;

        //
        // This is a wide character set font.  Is the block index valid?
        //
        psFontWide = (tFontWide *)psFont;
        if(ui16BlockIndex >= psFontWide->ui16NumBlocks)
        {
            //
            // The block number is invalid so return 0 to indicate the error.
            //
            return(0);
        }

        //
        // Yes - find the relevant block table and extract the required
        // information from it.
        //
        pBlock = (tFontBlock *)(psFontWide + 1);
        *pui32Start = pBlock[ui16BlockIndex].ui32StartCodepoint;
        return(pBlock[ui16BlockIndex].ui32NumCodepoints);
    }
    else if(psFont->ui8Format & FONT_FMT_WRAPPED)
    {
        //
        // This is a wrapper-based font so call the access function to get
        // its codepage.
        //
        ASSERT(((tFontWrapper *)psFont)->pFuncs->pfnFontBlockCodepointsGet);

        return(((tFontWrapper *)psFont)->pFuncs->
               pfnFontBlockCodepointsGet(((tFontWrapper *)psFont)->pui8FontId,
                                         ui16BlockIndex, pui32Start));
    }
    else
    {
        //
        // No - this is an old format font so it only supports a single block
        // of
        //
        if(ui16BlockIndex != 0)
        {
            //
            // An invalid block number was passed so return 0 to indicate an
            // error.
            //
            return(0);
        }

        //
        // We were passed a valid block number (0) so return the start
        // codepoint and number of characters.  Is this an extended font or
        // the original ASCII-only flavor?
        //
         if(psFont->ui8Format & FONT_EX_MARKER)
         {
             tFontEx *psFontEx;

             //
             // It's an extended font so read the character range from the
             // header.
             //
             psFontEx = (tFontEx *)psFont;

             *pui32Start = (uint32_t)psFontEx->ui8First;
             return((uint32_t)(psFontEx->ui8Last - psFontEx->ui8First + 1));
         }
         else
         {
             //
             // This is an ASCII font so it supports a fixed set of characters.
             //
             *pui32Start = 0x20;
             return(96);
         }
    }
}

//*****************************************************************************
//
//! Provides GrLib with a table of source/font codepage mapping functions.
//!
//! \param pContext is a pointer to the context to modify.
//! \param pCodePointMapTable points to an array of structures each defining
//! the mapping from a source text codepage to a destination font codepage.
//! \param ui8NumMaps provides the number of entries in the \e
//! pCodePointMapTable array.
//!
//! This function provides GrLib with a set of functions that can be used to
//! map text encoded in a particular codepage to one other codepage.  These
//! functions are used to allow GrLib to parse text strings and display the
//! correct glyphs from the font.  The mapping function used by the library
//! will depend upon the source text codepage set using a call to
//! GrStringCodepageSet() and the context's font, set using GrContextFontSet().
//!
//! If no conversion function is available to map from the selected source
//! codepage to the font's codepage, GrLib use the first conversion function
//! provided in the codepoint map table and the displayed text will likely be
//! incorrect.
//!
//! If this call is not made, GrLib assumes ISO8859-1 encoding for both the
//! source text and font to maintain backwards compatibility for applications
//! which were developed prior to the introduction of international character
//! set support.
//!
//! \return None.
//
//*****************************************************************************
void
GrCodepageMapTableSet(tContext *pContext, tCodePointMap *pCodePointMapTable,
                      uint8_t ui8NumMaps)
{
    ASSERT(pContext);
    ASSERT(pCodePointMapTable);
    ASSERT(ui8NumMaps);

    //
    // Remember the table details.
    //
    pContext->pCodePointMapTable = pCodePointMapTable;
    pContext->ui8NumCodePointMaps = ui8NumMaps;

    //
    // Update the character mapping to ensure that we show the right glyphs.
    //
    UpdateContextCharMapping(pContext);
}

//*****************************************************************************
//
//! Sets the source text codepage to be used.
//!
//! \param pContext is a pointer to the context to modify.
//! \param ui16Codepage is the identifier of the codepage for the text that the
//! application will pass on future calls to the GrStringDraw() and
//! GrStringDrawCentered() functions.
//!
//! This function sets the codepage that will be used when rendering text
//! via future calls to GrStringDraw() or GrStringDrawCentered().  The codepage
//! defines the mapping between specific numbers used to define characters and
//! the actual character glyphs displayed.  By default, GrLib assumes text
//! passed is encoded using the ISO8859-1 which supports ASCII and western
//! European character sets.  Applications wishing to use multi-byte character
//! sets or alphabets other than those supported by ISO8859-1 should set an
//! appropriate codepage such as UTF-8.
//!
//! It is important to ensure that your application makes use of fonts which
//! support the required codepage or that you have supplied GrLib with a
//! codepage mapping function that allows translation of your chosen text
//! codepage into the codepage supported by the fonts in use.  Several
//! mapping functions for commonly-used codepages are provided and others can
//! be written easily to support different text and font codepage combinations.
//! Codepage mapping functions are provided to GrLib in a table passed as a
//! parameter to the function GrCodepageMapTableSet().
//!
//! \return None.
//
//*****************************************************************************
int32_t
GrStringCodepageSet(tContext *pContext, uint16_t ui16Codepage)
{
    ASSERT(pContext);

    //
    // Remember the codepage to be used.
    //
    pContext->ui16Codepage = ui16Codepage;

    //
    // Update the character mapping to ensure that we show the right glyphs.
    //
    return(UpdateContextCharMapping(pContext));
}

//*****************************************************************************
//
//! Sets the font to be used.
//!
//! \param pContext is a pointer to the drawing context to modify.
//! \param psFont is a pointer to the font to be used.
//!
//! This function sets the font to be used for string drawing operations in the
//! specified drawing context.
//!
//! \return None.
//
//*****************************************************************************
void
GrContextFontSet(tContext *pContext, const tFont *psFont)
{
    ASSERT(pContext);
    ASSERT(psFont);

    //
    // Remember the font to be used.
    //
    pContext->psFont = psFont;

    //
    // Update the character mapping to ensure that we show the right glyphs.
    //
    UpdateContextCharMapping(pContext);
}
#endif

//*****************************************************************************
//
// Definitions and variables used by the decompression routine for the string
// table.
//
//*****************************************************************************
#define SC_MAX_INDEX            2047
#define SC_IS_NULL              0x0000ffff
#define SC_GET_LEN(v)           ((v) >> (32 - 5))
#define SC_GET_INDEX(v)         (((v) >> 16) & SC_MAX_INDEX)
#define SC_GET_OFF(v)           ((v) & SC_IS_NULL)

#define SC_FLAG_COMPRESSED      0x00008000
#define SC_OFFSET_M             0x00007fff

//*****************************************************************************
//
// The globals that hold the shortcuts to various locations and values in the
// table.
//
//*****************************************************************************
static const uint32_t *g_pui32StringTable;
static const uint16_t *g_pui16LanguageTable;
static const uint8_t *g_pui8StringData;

static uint16_t g_ui16Language;
static uint16_t g_ui16NumLanguages;
static uint16_t g_ui16NumStrings;

//*****************************************************************************
//
//! This function sets the location of the current string table.
//!
//! \param pvTable is a pointer to a string table that was generated by the
//! string compression utility.
//!
//! This function is used to set the string table to use for strings in an
//! application.  This string table is created by the string compression
//! utility.  This function is used to swap out multiple string tables if the
//! application requires more than one table.  It does not allow using more
//! than one string table at a time.
//!
//! \return None.
//
//*****************************************************************************
void
GrStringTableSet(const void *pvTable)
{
    //
    // Save the number of languages and number of strings.
    //
    g_ui16NumStrings = ((uint16_t *)pvTable)[0];
    g_ui16NumLanguages = ((uint16_t *)pvTable)[1];

    //
    // Save a pointer to the Language Identifier table.
    //
    g_pui16LanguageTable = (uint16_t *)pvTable + 2;

    //
    // Save a pointer to the String Index table.
    //
    g_pui32StringTable = (uint32_t *)(g_pui16LanguageTable +
                                      g_ui16NumLanguages);

    //
    // Save a pointer to the String Data.
    //
    g_pui8StringData = (uint8_t *)(g_pui32StringTable +
                                   (g_ui16NumStrings * g_ui16NumLanguages));
}

//*****************************************************************************
//
//! This function sets the current language for strings returned by the
//! GrStringGet() function.
//!
//! \param ui16LangID is one of the language identifiers provided in the string
//! table.
//!
//! This function is used to set the language identifier for the strings
//! returned by the GrStringGet() function.  The \e ui16LangID parameter should
//! match one of the identifiers that was included in the string table.  These
//! are provided in a header file in the graphics library and must match the
//! values that were passed through the sting compression utility.
//!
//! \return This function returns 0 if the language was not found and a
//! non-zero value if the laguage was found.
//
//*****************************************************************************
uint32_t
GrStringLanguageSet(uint16_t ui16LangID)
{
    int32_t i32Lang;

    //
    // Search for the requested language.
    //
    for(i32Lang = 0; i32Lang < g_ui16NumLanguages; i32Lang++)
    {
        //
        // Once found, break out and save the new language.
        //
        if(g_pui16LanguageTable[i32Lang] == ui16LangID)
        {
            break;
        }
    }

    //
    // Only accept the language if it was found, otherwise continue using
    // previous language.
    //
    if(i32Lang != g_ui16NumLanguages)
    {
        g_ui16Language = i32Lang;
        return(1);
    }

    return(0);
}

//*****************************************************************************
//
//! This function returns a string from the current string table.
//!
//! \param i32Index is the index of the string to retrieve.
//! \param pcData is the pointer to the buffer to store the string into.
//! \param ui32Size is the size of the buffer provided by pcData.
//!
//! This function will return a string from the string table in the language
//! set by the GrStringLanguageSet() function.  The value passed in \e iIndex
//! parameter is the string that is being requested and will be returned in
//! the buffer provided in the \e pcData parameter.  The amount of data
//! returned will be limited by the ui32Size parameter.
//!
//! \return Returns the number of valid bytes returned in the \e pcData buffer.
//
//*****************************************************************************
uint32_t
GrStringGet(int32_t i32Index, char *pcData, uint32_t ui32Size)
{
    uint32_t ui32Len, ui32Offset, ui32SubCode[16];
    int32_t i32Pos, i32Idx, i32Bit, i32Skip, i32Buf;
    uint8_t *pui8BufferOut;
    const uint8_t *pui8String;

    ASSERT(i32Index < g_ui16NumStrings);
    ASSERT(pcData != 0);

    //
    // Initialize the output buffer state.
    //
    i32Pos = 0;
    pui8BufferOut = 0;

    //
    // if built up from another string, we need to process that
    // this could nest multiple layers, so we follow in
    //
    ui32SubCode[i32Pos] = g_pui32StringTable[(g_ui16Language *
                                              g_ui16NumStrings) + i32Index];

    if(SC_GET_LEN(ui32SubCode[i32Pos]))
    {
        //
        // recurse down
        //
        while(i32Pos < 16)
        {
            //
            // Copy over the partial (if any) from a previous string.
            //
            i32Idx = SC_GET_INDEX(ui32SubCode[i32Pos++]);

            ui32SubCode[i32Pos] = g_pui32StringTable[(g_ui16Language *
                                                      g_ui16NumStrings) +
                                                     i32Idx];

            if(!SC_GET_LEN(ui32SubCode[i32Pos]))
            {
                //
                // not linked, just string
                //
                break;
            }
        }
    }

    //
    // Now work backwards out.
    //
    i32Idx = 0;

    //
    // Build up the string in pieces.
    //
    while(i32Pos >= 0)
    {
        //
        // Get the offset in string table.
        //
        ui32Offset = SC_GET_OFF(ui32SubCode[i32Pos]);

        if(ui32Offset == SC_IS_NULL)
        {
            //
            // An empty string.
            //
            pcData[i32Idx] = 0;
        }
        else if(ui32Offset & SC_FLAG_COMPRESSED)
        {
            //
            // This is a compressed string so initialize the pointer to the
            // compressed data.
            //
            pui8String = g_pui8StringData + (ui32Offset & SC_OFFSET_M);

            //
            // Initialize the bit variables.
            //
            i32Bit = 0;
            i32Skip = 0;

            //
            // Make a pointer to the current buffer out location.
            //
            pui8BufferOut = (uint8_t *)pcData + i32Idx;

            //
            // If the out buffer is beyond the maximum size then just break
            // out and return what we have so far.
            //
            if((char *)pui8BufferOut > (pcData + ui32Size))
            {
                break;
            }

            //
            // Now build up real string by decompressing bits.
            //
            if(!SC_GET_LEN(ui32SubCode[i32Pos]) &&
               SC_GET_INDEX(ui32SubCode[i32Pos]))
            {
                i32Skip = SC_GET_INDEX(ui32SubCode[i32Pos]);

                if(i32Pos)
                {
                    ui32Len = SC_GET_LEN(ui32SubCode[i32Pos - 1]);
                }
                else
                {
                    ui32Len = (i32Skip & 0x3f);
                }

                i32Skip >>= 6;
                i32Idx += ui32Len;
                ui32Len += i32Skip;
            }
            else if(i32Pos)
            {
                //
                // Get the length of the partial string.
                //
                ui32Len = SC_GET_LEN(ui32SubCode[i32Pos - 1]) - i32Idx;
                i32Idx += ui32Len;
            }
            else if(!SC_GET_LEN(ui32SubCode[0]) &&
                    SC_GET_INDEX(ui32SubCode[0]))
            {
                ui32Len = SC_GET_INDEX(ui32SubCode[0]);
                i32Skip = ui32Len >> 6;
                ui32Len = (ui32Len & 0x3f) + i32Skip;
            }
            else
            {
                //
                // Arbitrary as null character ends the string.
                //
                ui32Len = 1024;
            }

            for(; ui32Len; ui32Len--)
            {
                //
                // Packed 6 bits for each char
                //
                *pui8BufferOut = (*pui8String >> i32Bit) & 0x3f;

                if(i32Bit >= 2)
                {
                    *pui8BufferOut |= (*++pui8String << (8 - i32Bit)) & 0x3f;
                }

                i32Bit = (i32Bit + 6) & 0x7;

                if(!*pui8BufferOut)
                {
                    //
                    // end of string
                    //
                    break;
                }

                if(i32Skip)
                {
                    i32Skip--;
                    continue;
                }

                //
                // Put back removed bit
                //
                *pui8BufferOut |= 0x40;

                //
                // Now look for a few special chars we mapped up into other
                // characters.
                //
                if(*pui8BufferOut == '`')
                {
                    *pui8BufferOut = ' ';
                }
                else if(*pui8BufferOut == '~')
                {
                    *pui8BufferOut = '-';
                }
                else if(*pui8BufferOut == 0x7f)
                {
                    *pui8BufferOut = '.';
                }
                else if(*pui8BufferOut == '\\')
                {
                    *pui8BufferOut = ':';
                }

                //
                // Increment the pointer and break out if the pointer is now
                // beyond the end of the buffer provided.
                //
                pui8BufferOut++;

                if((char *)pui8BufferOut >= (pcData + ui32Size))
                {
                    break;
                }
            }
        }
        else if(i32Pos)
        {
            //
            // Part of another string
            //
            ui32Len = SC_GET_LEN(ui32SubCode[i32Pos - 1]) - i32Idx;

            //
            // Prevent this copy from going beyond the end of the buffer
            // provided.
            //
            if((i32Idx + ui32Len) > ui32Size)
            {
                ui32Len = ui32Size - i32Idx;
            }

            //
            // Copy this portion of the string to the output buffer.
            //
            for(i32Buf = 0; i32Buf < ui32Len; i32Buf++)
            {
                pcData[i32Idx + i32Buf] = g_pui8StringData[ui32Offset +
                                                           i32Buf];
            }

            i32Idx += ui32Len;
        }
        else if(SC_GET_INDEX(ui32SubCode[0]) && !SC_GET_LEN(ui32SubCode[0]))
        {
            //
            // Copy this portion of the string to the output buffer.
            //
            for(i32Buf = 0; i32Buf < SC_GET_INDEX(ui32SubCode[0]); i32Buf++)
            {
                if((i32Idx + i32Buf) < ui32Size)
                {
                    pcData[i32Idx + i32Buf] = g_pui8StringData[ui32Offset +
                                                               i32Buf];
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            //
            // Now copy the last piece of the string.
            //
            for(i32Buf = 0; i32Buf < (ui32Size - i32Idx); i32Buf++)
            {
                //
                // Copy the string to the output buffer.
                //
                pcData[i32Idx + i32Buf] = g_pui8StringData[ui32Offset +
                                                           i32Buf];

                //
                // If a null is hit then terminate the copy.
                //
                if(pcData[i32Idx + i32Buf] == 0)
                {
                    break;
                }
            }

            //
            // If we had not copied any characters before hitting this case,
            // initialize the output pointer (this keeps the code at the end of
            // the function that returns the length happy).  This will be the
            // case if we are using an uncompressed string table.
            //
            if(!pui8BufferOut)
            {
                pui8BufferOut = (uint8_t *)pcData + (i32Idx + i32Buf);
            }
        }
        i32Pos--;
    }

    //
    // Return the number of bytes copied into the output buffer.
    //
    if(pui8BufferOut)
    {
        ui32Len = ((uint32_t)pui8BufferOut - (uint32_t)pcData);

        //
        // Null terminate the string if there is room.
        //
        if(ui32Len < ui32Size)
        {
            pcData[ui32Len] = 0;
        }
    }
    else
    {
        ui32Len = 0;
    }

    return(ui32Len);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
