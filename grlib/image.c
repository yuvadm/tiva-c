//*****************************************************************************
//
// image.c - Routines for drawing bitmap images.
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

#include <stdbool.h>
#include <stdint.h>
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
// The buffer that holds the dictionary used by the Lempel-Ziv-Storer-Szymanski
// compression algorithm.  This is simply the last 32 bytes decoded from the
// stream, and is initially filled with zeros.
//
//*****************************************************************************
static uint8_t g_pui8Dictionary[32];

//*****************************************************************************
//
// Draws a run of pixels, dropping out any in a given transparent color.
// Returns true if any pixels were drawn or false if none were drawn.
//
//*****************************************************************************
static bool
PixelTransparentDraw(const tContext *pContext, int32_t i32X, int32_t i32Y,
                     int32_t i32X0, int32_t i32Count, int32_t i32BPP,
                     const uint8_t *pui8Data, const uint8_t *pui8Palette,
                     uint32_t ui32Transparent)
{
    int32_t i32Start, i32Len, i32Index, i32StartX0, i32On, i32Off;
    int32_t i32NumBytes, i32Bit, i32Draw;
    uint32_t ui32Mask;
    uint8_t ui8Pixel;
    bool bSkip, bRet;

    //
    // Assume we drew no pixels until we determine otherwise.
    //
    bRet = false;

    //
    // What format are we dealing with?
    //
    switch(i32BPP & 0xFF)
    {
        //
        // Two color bitmap.
        //
        case 1:
        {
            //
            // How many bytes do we need to read to cover the line of data
            // we've been passed.
            //
            i32NumBytes = (i32Count + i32X0 + 7) / 8;

            //
            // Where must we end the line of pixels?
            //
            i32Len = i32Count;

            //
            // Set our mask to allow us to make either foreground or background
            // pixels transparent.
            //
            ui32Mask = ui32Transparent ? 0xFF : 0;

            //
            // Loop through the bytes in the pixel data.
            //
            i32Bit = i32X0;
            i32X0 = 0;
            for(i32Index = 0; i32Index < i32NumBytes; )
            {
                //
                // Count the number of off pixels from this position in the
                // glyph image.
                //
                for(i32Off = 0; i32Index < i32NumBytes; )
                {
                    //
                    // Get the number of zero pixels at this position.
                    //
                    i32Count = NumLeadingZeros(((pui8Data[i32Index] ^
                                                 ui32Mask) <<
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
                        i32Index++;
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
                for(i32On = 0; i32Index < i32NumBytes; )
                {
                    //
                    // Get the number of one pixels at this location (by
                    // inverting the data and counting the number of zeros).
                    //
                    i32Count = NumLeadingZeros(~(((pui8Data[i32Index] ^
                                                   ui32Mask) <<
                                                 (24 + i32Bit))));

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
                        i32Index++;
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

                //
                // At this point, we have the next off and on run lengths
                // determined so draw the on run if it is non-zero length
                // and falls within the range we need to draw.
                //
                if(i32On && (i32Off < i32Len))
                {
                    i32Draw = ((i32Off + i32On) > i32Len) ? (i32X + i32Len) :
                              (i32X + i32Off + i32On);
                    DpyLineDrawH(pContext->psDisplay, i32X + i32Off,
                                 i32Draw - 1, i32Y,
                                 *(uint32_t *)(pui8Palette +
                                               (ui32Transparent ? 0 : 4)));

                    //
                    // Remember that we actually drew something.
                    //
                    bRet = true;
                }

                //
                // Move right past these two runs.
                //
                i32X += (i32Off + i32On);
                i32Len -= (i32Off + i32On);
            }
        }
        break;

        //
        // 4 bits per pixel (16 color) bitmap.
        //
        case 4:
        {
            //
            // Are we starting by drawing or skipping pixels?
            //
            ui8Pixel = (pui8Data[0] >> (i32X0 ? 0 : 4)) & 0x0F;
            bSkip = (ui8Pixel == (uint8_t)ui32Transparent) ? true : false;
            i32Start = 0;
            i32StartX0 = i32X0;
            i32Bit = i32X0;
            i32Len = bSkip ? 0 : 1;

            //
            // Scan all pixels in the line of data provided.
            //
            for(i32Index = 1; i32Index < i32Count; i32Index++)
            {
                //
                // Toggle the sub-byte pixel indicator;
                //
                i32X0 = 1 - i32X0;

                //
                // Read the next pixel.
                //
                ui8Pixel = (pui8Data[(i32Index + i32Bit) / 2] >>
                           (i32X0 ? 0 : 4)) & 0x0F;

                //
                // Is this pixel a transparent one?
                //
                if(ui8Pixel != (uint8_t)ui32Transparent)
                {
                    //
                    // It's not transparent.  Have we just ended a run of
                    // transparent pixels?
                    //
                    if(bSkip)
                    {
                        //
                        // We are currently skipping pixels so this starts a
                        // new run.
                        //
                        i32Start = i32Index;
                        i32StartX0 = i32X0;
                        i32Len = 1;
                        bSkip = false;
                    }
                    else
                    {
                        //
                        // We were already in the middle of a run of non-
                        // transparent pixels so increment the run length.
                        //
                        i32Len++;
                    }
                }
                else
                {
                    //
                    // Pixel is transparent.  Do we have a run to draw?
                    //
                    if(!bSkip)
                    {
                        //
                        // Yes - draw what we have.
                        //
                        DpyPixelDrawMultiple(pContext->psDisplay,
                                            i32X + i32Start, i32Y,
                                            i32StartX0, i32Len, i32BPP,
                                            &pui8Data[(i32Start + i32Bit) / 2],
                                            pui8Palette);

                        //
                        // Reset for the transparent run.
                        //
                        i32Len = 0;
                        bSkip = true;

                        //
                        // Remember that we actually drew something.
                        //
                        bRet = true;
                    }
                }
            }

            //
            // If we drop out of the pixel loop with a run not drawn, draw it
            // here.
            //
            if(!bSkip && i32Len)
            {
                DpyPixelDrawMultiple(pContext->psDisplay, i32X + i32Start,
                                     i32Y, i32StartX0, i32Len, i32BPP,
                                     &pui8Data[(i32Start + i32Bit) / 2],
                                     pui8Palette);

                //
                // Remember that we actually drew something.
                //
                bRet = true;
            }
        }
        break;

        //
        // 8 bit per pixel (256 color) bitmap.
        //
        case 8:
        {
            //
            // Are we starting by drawing or skipping pixels?
            //
            bSkip = (pui8Data[0] == (uint8_t)ui32Transparent) ? true : false;
            i32Start = 0;
            i32Len = bSkip ? 0 : 1;

            //
            // Scan all pixels in the line of data provided.
            //
            for(i32Index = 1; i32Index < i32Count; i32Index++)
            {
                //
                // Is this pixel a transparent one?
                //
                if(pui8Data[i32Index] != (uint8_t)ui32Transparent)
                {
                    //
                    // It's not transparent.  Have we just ended a run of
                    // transparent pixels?
                    //
                    if(bSkip)
                    {
                        //
                        // We are currently skipping pixels so this starts a
                        // new run.
                        //
                        i32Start = i32Index;
                        i32Len = 1;
                        bSkip = false;
                    }
                    else
                    {
                        //
                        // We were already in the middle of a run of non-
                        // transparent pixels so increment the run length.
                        //
                        i32Len++;
                    }
                }
                else
                {
                    //
                    // Pixel is transparent.  Do we have a run to draw?
                    //
                    if(!bSkip)
                    {
                        //
                        // Yes - draw what we have.
                        //
                        DpyPixelDrawMultiple(pContext->psDisplay,
                                               i32X + i32Start, i32Y, 0,
                                               i32Len, i32BPP,
                                               &pui8Data[i32Start],
                                               pui8Palette);

                        //
                        // Reset for the transparent run.
                        //
                        i32Len = 0;
                        bSkip = true;

                        //
                        // Remember that we actually drew something.
                        //
                        bRet = true;
                    }
                }
            }

            //
            // If we drop out of the pixel loop with a run not drawn, draw it
            // here.
            //
            if(!bSkip && i32Len)
            {
                DpyPixelDrawMultiple(pContext->psDisplay, i32X + i32Start,
                                     i32Y, i32X0, i32Len, i32BPP,
                                     &pui8Data[i32Start], pui8Palette);

                //
                // Remember that we actually drew something.
                //
                bRet = true;
            }
        }
        break;
    }

    //
    // Tell the caller whether or not we actually drew something.
    //
    return(bRet);
}

//*****************************************************************************
//
// Internal function implementing both normal and transparent image drawing.
//
//*****************************************************************************
static void
InternalImageDraw(const tContext *pContext, const uint8_t *pui8Image,
                  int32_t i32X, int32_t i32Y, uint32_t ui32Transparent,
                  bool bTransparent)
{
    uint32_t ui32Byte, ui32Bits, ui32Match, ui32Size, ui32Idx, ui32Count;
    uint32_t ui32Num;
    int32_t i32BPP, i32Width, i32Height, i32X0, i32X1, i32X2, i32XMask;
    const uint8_t *pui8Palette;
    uint32_t pui32BWPalette[2];
    int32_t i32Flag;

    //
    // Check the arguments.
    //
    ASSERT(pContext);
    ASSERT(pui8Image);

    //
    // Get the image format from the image data.
    //
    i32BPP = *pui8Image++;

    //
    // Get the image width from the image data.
    //
    i32Width = *(uint16_t *)pui8Image;
    pui8Image += 2;

    //
    // Get the image height from the image data.
    //
    i32Height = *(uint16_t *)pui8Image;
    pui8Image += 2;

    //
    // Return without doing anything if the entire image lies outside the
    // current clipping region.
    //
    if((i32X > pContext->sClipRegion.i16XMax) ||
       ((i32X + i32Width - 1) < pContext->sClipRegion.i16XMin) ||
       (i32Y > pContext->sClipRegion.i16YMax) ||
       ((i32Y + i32Height - 1) < pContext->sClipRegion.i16YMin))
    {
        return;
    }

    //
    // Set the flag indicating that we are drawing a new image.  This will
    // be cleared after the first pixel run is drawn.
    //
    i32Flag = GRLIB_DRIVER_FLAG_NEW_IMAGE;

    //
    // Get the starting X offset within the image based on the current clipping
    // region.
    //
    if(i32X < pContext->sClipRegion.i16XMin)
    {
        i32X0 = pContext->sClipRegion.i16XMin - i32X;
    }
    else
    {
        i32X0 = 0;
    }

    //
    // Get the ending X offset within the image based on the current clipping
    // region.
    //
    if((i32X + i32Width - 1) > pContext->sClipRegion.i16XMax)
    {
        i32X2 = pContext->sClipRegion.i16XMax - i32X;
    }
    else
    {
        i32X2 = i32Width - 1;
    }

    //
    // Reduce the height of the image, if required, based on the current
    // clipping region.
    //
    if((i32Y + i32Height - 1) > pContext->sClipRegion.i16YMax)
    {
        i32Height = pContext->sClipRegion.i16YMax - i32Y + 1;
    }

    //
    // Determine the color palette for the image based on the image format.
    //
    if((i32BPP & 0x7f) == IMAGE_FMT_1BPP_UNCOMP)
    {
        //
        // Construct a local "black & white" palette based on the foreground
        // and background colors of the drawing context.
        //
        pui32BWPalette[0] = pContext->ui32Background;
        pui32BWPalette[1] = pContext->ui32Foreground;

        //
        // Set the palette pointer to the local "black & white" palette.
        //
        pui8Palette = (uint8_t *)pui32BWPalette;
    }
    else
    {
        //
        // For 4 and 8 BPP images, the palette is contained at the start of the
        // image data.
        //
        pui8Palette = pui8Image + 1;
        pui8Image += (pui8Image[0] * 3) + 4;
    }

    //
    // See if the image is compressed.
    //
    if(!(i32BPP & 0x80))
    {
        //
        // The image is not compressed.  See if the top portion of the image
        // lies above the clipping region.
        //
        if(i32Y < pContext->sClipRegion.i16YMin)
        {
            //
            // Determine the number of rows that lie above the clipping region.
            //
            i32X1 = pContext->sClipRegion.i16YMin - i32Y;

            //
            // Skip past the data for the rows that lie above the clipping
            // region.
            //
            pui8Image += (((i32Width * i32BPP) + 7) / 8) * i32X1;

            //
            // Decrement the image height by the number of skipped rows.
            //
            i32Height -= i32X1;

            //
            // Increment the starting Y coordinate by the number of skipped
            // rows.
            //
            i32Y += i32X1;
        }

        //
        // Determine the starting offset for the first source pixel within
        // the byte.
        //
        switch(i32BPP)
        {
            case 1:
            {
                i32XMask = i32X0 & 7;
                break;
            }

            case 4:
            {
                i32XMask = i32X0 & 1;
                break;
            }

            default:
            {
                i32XMask = 0;
                break;
            }
        }

        //
        // Loop while there are more rows to draw.
        //
        while(i32Height--)
        {
            //
            // Draw this row of image pixels.
            //
            if(bTransparent)
            {
                bool bRet;

                //
                // Draw a run of pixels dropping out any which are
                // transparent.
                //
                bRet = PixelTransparentDraw(pContext, i32X + i32X0, i32Y,
                                            i32XMask, i32X2 - i32X0 + 1,
                                            i32BPP | i32Flag,
                                            pui8Image + ((i32X0 * i32BPP) / 8),
                                            pui8Palette, ui32Transparent);

                //
                // Did we actually draw anything in this run?
                //
                if(bRet)
                {
                    //
                    // Yes. Clear the flag that tells the driver that this is
                    // the first run of a new image.  If we clear this when
                    // nothing was drawn, the driver will not see the flag
                    // and may not correctly rebuild its color lookup table.
                    //
                    i32Flag = 0;
                }
            }
            else
            {
                DpyPixelDrawMultiple(pContext->psDisplay, i32X + i32X0,
                                     i32Y, i32XMask, i32X2 - i32X0 + 1,
                                     i32BPP | i32Flag, pui8Image +
                                     ((i32X0 * i32BPP) / 8), pui8Palette);

                //
                // Clear the flag since we've drawn the first line now.
                //
                i32Flag = 0;
            }

            //
            // Skip past the data for this row.
            //
            pui8Image += ((i32Width * i32BPP) + 7) / 8;

            //
            // Increment the Y coordinate.
            //
            i32Y++;
        }
    }
    else
    {
        //
        // The image is compressed.  Clear the compressed flag in the format
        // specifier so that the bits per pixel remains.
        //
        i32BPP &= 0x7f;

        //
        // Reset the dictionary used to uncompress the image.
        //
        for(ui32Bits = 0; ui32Bits < sizeof(g_pui8Dictionary); ui32Bits += 4)
        {
            *(uint32_t *)(g_pui8Dictionary + ui32Bits) = 0;
        }

        //
        // Determine the number of bytes of data to decompress.
        //
        ui32Count = (((i32Width * i32BPP) + 7) / 8) * i32Height;

        //
        // Initialize the pointer into the dictionary.
        //
        ui32Idx = 0;

        //
        // Start off with no encoding byte.
        //
        ui32Bits = 0;
        ui32Byte = 0;

        //
        // Start from the upper left corner of the image.
        //
        i32X1 = 0;

        //
        // Loop while there are more rows or more data in the image.
        //
        while(i32Height && ui32Count)
        {
            //
            // See if an encoding byte needs to be read.
            //
            if(ui32Bits == 0)
            {
                //
                // Read the encoding byte, which indicates if each of the
                // following eight bytes are encoded or literal.
                //
                ui32Byte = *pui8Image++;
                ui32Bits = 8;
            }

            //
            // See if the next byte is encoded or literal.
            //
            if(ui32Byte & (1 << (ui32Bits - 1)))
            {
                //
                // This byte is encoded, so extract the location and size of
                // the encoded data within the dictionary.
                //
                ui32Match = *pui8Image >> 3;
                ui32Size = (*pui8Image++ & 7) + 2;

                //
                // Decrement the count of bytes to decode by the number of
                // copied bytes.
                //
                ui32Count -= ui32Size;
            }
            else
            {
                //
                // This byte is a literal, so copy it into the dictionary.
                //
                g_pui8Dictionary[ui32Idx++] = *pui8Image++;

                //
                // Decrement the count of bytes to decode.
                //
                ui32Count--;

                //
                // Clear any previous encoded data information.
                //
                ui32Match = 0;
                ui32Size = 0;
            }

            //
            // Loop while there are bytes to copy for the encoded data, or
            // once for literal data.
            //
            while(ui32Size || !(ui32Byte & (1 << (ui32Bits - 1))))
            {
                //
                // Set the encoded data bit for this data so that this loop
                // will only be executed once for literal data.
                //
                ui32Byte |= 1 << (ui32Bits - 1);

                //
                // Loop while there is more encoded data to copy and there is
                // additional space in the dictionary (before the buffer
                // wraps).
                //
                while(ui32Size && (ui32Idx != sizeof(g_pui8Dictionary)))
                {
                    //
                    // Copy this byte.
                    //
                    g_pui8Dictionary[ui32Idx] =
                        g_pui8Dictionary[(ui32Idx + ui32Match) %
                                        sizeof(g_pui8Dictionary)];

                    //
                    // Increment the dictionary pointer.
                    //
                    ui32Idx++;

                    //
                    // Decrement the encoded data size.
                    //
                    ui32Size--;
                }

                //
                // See if the dictionary pointer is about to wrap, or if there
                // is no more data to decompress.
                //
                if((ui32Idx == sizeof(g_pui8Dictionary)) || !ui32Count)
                {
                    //
                    // Loop through the data in the dictionary buffer.
                    //
                    for(ui32Idx = 0;
                        (ui32Idx < sizeof(g_pui8Dictionary)) && i32Height; )
                    {
                        //
                        // Compute the number of pixels that remain in the
                        // dictionary buffer.
                        //
                        ui32Num = ((sizeof(g_pui8Dictionary) - ui32Idx) * 8) /
                                  i32BPP;

                        //
                        // See if any of the pixels in the dictionary buffer
                        // are within the clipping region.
                        //
                        if((i32Y >= pContext->sClipRegion.i16YMin) &&
                           ((i32X1 + ui32Num) >= i32X0) && (i32X1 <= i32X2))
                        {
                            //
                            // Skip some pixels at the start of the scan line
                            // if required to stay within the clipping region.
                            //
                            if(i32X1 < i32X0)
                            {
                                ui32Idx += ((i32X0 - i32X1) * i32BPP) / 8;
                                i32X1 = i32X0;
                            }

                            //
                            // Shorten the scan line if required to stay within
                            // the clipping region.
                            //
                            if(ui32Num > (i32X2 - i32X1 + 1))
                            {
                                ui32Num = i32X2 - i32X1 + 1;
                            }

                            //
                            // Determine the starting offset for the first
                            // source pixel within the byte.
                            //
                            switch(i32BPP)
                            {

                                case 1:
                                {
                                    i32XMask = i32X1 & 7;
                                    break;
                                }

                                case 4:
                                {
                                    i32XMask = i32X1 & 1;
                                    break;
                                }

                                default:
                                {
                                    i32XMask = 0;
                                    break;
                                }
                            }

                            //
                            // Draw this row of image pixels.
                            //
                            if(bTransparent)
                            {
                                bool bRet;

                                bRet = PixelTransparentDraw(pContext,
                                                            i32X + i32X1,
                                                            i32Y, i32XMask,
                                                            ui32Num,
                                                            i32BPP | i32Flag,
                                                            g_pui8Dictionary +
                                                            ui32Idx,
                                                            pui8Palette,
                                                            ui32Transparent);

                                //
                                // Clear the flag only if we actually drew
                                // something.
                                //
                                if(bRet)
                                {
                                    //
                                    // We drew something so that NEW_IMAGE
                                    // flag is no longer needed.
                                    //
                                    i32Flag = 0;
                                }
                            }
                            else
                            {
                                DpyPixelDrawMultiple(pContext->psDisplay,
                                                     i32X + i32X1, i32Y,
                                                     i32XMask, ui32Num,
                                                     i32BPP | i32Flag,
                                                     g_pui8Dictionary + ui32Idx,
                                                     pui8Palette);

                                //
                                // We've drawn the first line so clear the flag.
                                //
                                i32Flag = 0;
                            }
                        }

                        //
                        // Move the X coordinate back to the start of the first
                        // data byte in this portion of the dictionary buffer.
                        //
                        i32X1 = ((i32X1 * i32BPP) & ~7) / i32BPP;

                        //
                        // See if the remainder of this scan line resides
                        // within the dictionary buffer.
                        //
                        if(((((i32Width - i32X1) * i32BPP) + 7) / 8) >
                           (sizeof(g_pui8Dictionary) - ui32Idx))
                        {
                            //
                            // There is more to this scan line than is in the
                            // dictionary buffer at this point, so move the
                            // X coordinate by by the number of pixels in the
                            // dictionary buffer.
                            //
                            i32X1 += (((sizeof(g_pui8Dictionary) - ui32Idx) *
                                       8) / i32BPP);

                            //
                            // The entire dictionary buffer has been scanned.
                            //
                            ui32Idx = sizeof(g_pui8Dictionary);
                        }
                        else
                        {
                            //
                            // The remainder of this scan line resides in the
                            // dictionary buffer, so skip past it.
                            //
                            ui32Idx += (((i32Width - i32X1) * i32BPP) + 7) / 8;

                            //
                            // Move to the start of the next scan line.
                            //
                            i32X1 = 0;
                            i32Y++;

                            //
                            // There is one less scan line to process.
                            //
                            i32Height--;
                        }
                    }

                    //
                    // Start over from the beginning of the dictionary buffer.
                    //
                    ui32Idx = 0;
                }
            }

            //
            // Advance to the next bit in the encoding byte.
            //
            ui32Bits--;
        }
    }
}

//*****************************************************************************
//
//! Draws a bitmap image, dropping out a single transparent color.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param pui8Image is a pointer to the image to draw.
//! \param i32X is the X coordinate of the upper left corner of the image.
//! \param i32Y is the Y coordinate of the upper left corner of the image.
//! \param ui32Transparent is the image color which is to be considered
//! transparent.
//!
//! This function draws a bitmap image but, unlike GrImageDraw, will drop out
//! any pixel of a particular color allowing the previous background to ``shine
//! through''.  The image may be 1 bit per pixel (using the foreground and
//! background color from the drawing context), 4 bits per pixel (using a
//! palette supplied in the image data), or 8 bits per pixel (using a palette
//! supplied in the image data).  It can be uncompressed data, or it can be
//! compressed using the Lempel-Ziv-Storer-Szymanski algorithm (as published in
//! the Journal of the ACM, 29(4):928-951, October 1982).  For 4bpp and 8bpp
//! images, the \b ui32Transparent parameter contains the palette index of the
//! colour which is to be considered transparent.  For 1bpp images, the
//! \b ui32Transparent parameter should be set to 0 to draw only foreground
//! pixels or 1 to draw only background pixels.
//!
//! \return None.
//
//*****************************************************************************
void
GrTransparentImageDraw(const tContext *pContext, const uint8_t *pui8Image,
                       int32_t i32X, int32_t i32Y, uint32_t ui32Transparent)
{
    InternalImageDraw(pContext, pui8Image, i32X, i32Y, ui32Transparent,
                        true);
}

//*****************************************************************************
//
//! Draws a bitmap image.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param pui8Image is a pointer to the image to draw.
//! \param i32X is the X coordinate of the upper left corner of the image.
//! \param i32Y is the Y coordinate of the upper left corner of the image.
//!
//! This function draws a bitmap image.  The image may be 1 bit per pixel
//! (using the foreground and background color from the drawing context), 4
//! bits per pixel (using a palette supplied in the image data), or 8 bits per
//! pixel (using a palette supplied in the image data).  It can be uncompressed
//! data, or it can be compressed using the Lempel-Ziv-Storer-Szymanski
//! algorithm (as published in the Journal of the ACM, 29(4):928-951, October
//! 1982).
//!
//! \return None.
//
//*****************************************************************************
void
GrImageDraw(const tContext *pContext, const uint8_t *pui8Image,
            int32_t i32X, int32_t i32Y)
{
    InternalImageDraw(pContext, pui8Image, i32X, i32Y, 0, false);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
