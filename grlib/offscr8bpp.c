//*****************************************************************************
//
// offscr8bpp.c - 8 BPP off-screen display buffer driver.
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

//*****************************************************************************
//
//! \addtogroup primitives_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Translates a 24-bit RGB color to a display driver-specific color.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param ui32Value is the 24-bit RGB color.  The least-significant byte is
//! the blue channel, the next byte is the green channel, and the third byte is
//! the red channel.
//!
//! This function translates a 24-bit RGB color into a value that can be
//! written into the display's frame buffer in order to reproduce that color,
//! or the closest possible approximation of that color.
//!
//! \return Returns the display-driver specific color.
//
//*****************************************************************************
static uint32_t
GrOffScreen8BPPColorTranslate(void *pvDisplayData, uint32_t ui32Value)
{
    uint32_t ui32Idx, ui32Diff, ui32MatchIdx, ui32MatchDiff;
    uint32_t ui32R, ui32G, ui32B;
    uint8_t *pui8Palette;

    //
    // Check the arguments.
    //
    ASSERT(pvDisplayData);

    //
    // Get a pointer to the palette for the off-screen buffer.
    //
    pui8Palette = (uint8_t *)pvDisplayData + 6;

    //
    // Extract the red, green, and blue component from the input color.
    //
    ui32R = (ui32Value >> ClrRedShift) & 0xff;
    ui32G = (ui32Value >> ClrGreenShift) & 0xff;
    ui32B = (ui32Value >> ClrBlueShift) & 0xff;

    //
    // Set the match such that the first palette entry will be a better match.
    //
    ui32MatchIdx = 0;
    ui32MatchDiff = 0xffffffff;

    //
    // Loop through the colors in the palette.
    //
    for(ui32Idx = 0; ui32Idx < 256; ui32Idx++, pui8Palette += 3)
    {
        //
        // Compute the Cartesian distance between these two colors.
        //
        ui32Diff = (((pui8Palette[2] - ui32R) * (pui8Palette[2] - ui32R)) +
                  ((pui8Palette[1] - ui32G) * (pui8Palette[1] - ui32G)) +
                  ((pui8Palette[0] - ui32B) * (pui8Palette[0] - ui32B)));

        //
        // See if this color is a closer match than any of the previous colors.
        //
        if(ui32Diff < ui32MatchDiff)
        {
            //
            // Save this color as the new best match.
            //
            ui32MatchDiff = ui32Diff;
            ui32MatchIdx = ui32Idx;
        }

        //
        // Stop looking if an exact match was found.
        //
        if(ui32Diff == 0)
        {
            break;
        }
    }

    //
    // Return the index of the best match.
    //
    return(ui32MatchIdx);
}

//*****************************************************************************
//
//! Draws a pixel on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param i32X is the X coordinate of the pixel.
//! \param i32Y is the Y coordinate of the pixel.
//! \param ui32Value is the color of the pixel.
//!
//! This function sets the given pixel to a particular color.  The coordinates
//! of the pixel are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void
GrOffScreen8BPPPixelDraw(void *pvDisplayData, int32_t i32X, int32_t i32Y,
                         uint32_t ui32Value)
{
    uint8_t *pui8Data;

    //
    // Check the arguments.
    //
    ASSERT(pvDisplayData);

    //
    // Create a character pointer for the display-specific data (which points
    // to the image buffer).
    //
    pui8Data = (uint8_t *)pvDisplayData;

    //
    // Get the offset to the byte of the image buffer that contains the pixel
    // in question.
    //
    pui8Data += (*(uint16_t *)(pui8Data + 1) * i32Y) + i32X + 6 + (256 * 3);

    //
    // Write this pixel into the image buffer.
    //
    *pui8Data = ui32Value;
}

//*****************************************************************************
//
//! Draws a horizontal sequence of pixels on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param i32X is the X coordinate of the first pixel.
//! \param i32Y is the Y coordinate of the first pixel.
//! \param i32X0 is sub-pixel offset within the pixel data, which is valid for
//! 1 or 4 bit per pixel formats.
//! \param i32Count is the number of pixels to draw.
//! \param i32BPP is the number of bits per pixel ORed with a flag indicating
//! whether or not this run represents the start of a new image.
//! \param pui8Data is a pointer to the pixel data.  For 1 and 4 bit per pixel
//! formats, the most significant bit(s) represent the left-most pixel.
//! \param pui8Palette is a pointer to the palette used to draw the pixels.
//!
//! This function draws a horizontal sequence of pixels on the screen, using
//! the supplied palette.  For 1 bit per pixel format, the palette contains
//! pre-translated colors; for 4 and 8 bit per pixel formats, the palette
//! contains 24-bit RGB values that must be translated before being written to
//! the display.
//!
//! The \e i32BPP parameter will take the value 1, 4 or 8 and may be ORed with
//! \b GRLIB_DRIVER_FLAG_NEW_IMAGE to indicate that this run represents the
//! start of a new image.  Drivers which make use of lookup tables to convert
//! from the source to destination pixel values should rebuild their lookup
//! table when \b GRLIB_DRIVER_FLAG_NEW_IMAGE is set.
//!
//! \return None.
//
//*****************************************************************************
static void
GrOffScreen8BPPPixelDrawMultiple(void *pvDisplayData, int32_t i32X,
                                 int32_t i32Y, int32_t i32X0, int32_t i32Count,
                                 int32_t i32BPP, const uint8_t *pui8Data,
                                 const uint8_t *pui8Palette)
{
    uint8_t *pui8Ptr;
    uint32_t ui32Byte;

    //
    // Check the arguments.
    //
    ASSERT(pvDisplayData);
    ASSERT(pui8Data);
    ASSERT(pui8Palette);

    //
    // Create a character pointer for the display-specific data (which points
    // to the image buffer).
    //
    pui8Ptr = (uint8_t *)pvDisplayData;

    //
    // Get the offset to the byte of the image buffer that contains the
    // starting pixel.
    //
    pui8Ptr += (*(uint16_t *)(pui8Ptr + 1) * i32Y) + i32X + 6 + (256 * 3);

    //
    // Determine how to interpret the pixel data based on the number of bits
    // per pixel.
    //
    switch(i32BPP & 0xFF)
    {
        //
        // The pixel data is in 1 bit per pixel format.
        //
        case 1:
        {
            //
            // Loop while there are more pixels to draw.
            //
            while(i32Count)
            {
                //
                // Get the next byte of image data.
                //
                ui32Byte = *pui8Data++;

                //
                // Loop through the pixels in this byte of image data.
                //
                for(; (i32X0 < 8) && i32Count; i32X0++, i32Count--)
                {
                    //
                    // Draw this pixel in the appropriate color.
                    //
                    *pui8Ptr++ = (((uint32_t *)pui8Palette)[(ui32Byte >>
                                                                (7 - i32X0)) &
                                                               1]);
                }

                //
                // Start at the beginning of the next byte of image data.
                //
                i32X0 = 0;
            }

            //
            // The image data has been drawn.
            //
            break;
        }

        //
        // The pixel data is in 4 bit per pixel format.
        //
        case 4:
        {
            //
            // Loop while there are more pixels to draw.  "Duff's device" is
            // used to jump into the middle of the loop if the first nibble of
            // the pixel data should not be used.  Duff's device makes use of
            // the fact that a case statement is legal anywhere within a
            // sub-block of a switch statement.  See
            // http://en.wikipedia.org/wiki/Duff's_device for detailed
            // information about Duff's device.
            //
            switch(i32X0 & 1)
            {
                case 0:
                    while(i32Count)
                    {
                        //
                        // Get the upper nibble of the next byte of pixel data
                        // and extract the corresponding entry from the
                        // palette.
                        //
                        ui32Byte = (*pui8Data >> 4) * 3;
                        ui32Byte = (*(uint32_t *)(pui8Palette + ui32Byte) &
                                  0x00ffffff);

                        //
                        // Translate this palette entry and write it to the
                        // screen.
                        //
                        *pui8Ptr++ =
                            GrOffScreen8BPPColorTranslate(pvDisplayData,
                                                          ui32Byte);

                        //
                        // Decrement the count of pixels to draw.
                        //
                        i32Count--;

                        //
                        // See if there is another pixel to draw.
                        //
                        if(i32Count)
                        {
                case 1:
                            //
                            // Get the lower nibble of the next byte of pixel
                            // data and extract the corresponding entry from
                            // the palette.
                            //
                            ui32Byte = (*pui8Data++ & 15) * 3;
                            ui32Byte = (*(uint32_t *)(pui8Palette + ui32Byte) &
                                        0x00ffffff);

                            //
                            // Translate this palette entry and write it to the
                            // screen.
                            //
                            *pui8Ptr++ =
                                GrOffScreen8BPPColorTranslate(pvDisplayData,
                                                              ui32Byte);

                            //
                            // Decrement the count of pixels to draw.
                            //
                            i32Count--;
                        }
                    }
            }

            //
            // The image data has been drawn.
            //
            break;
        }

        //
        // The pixel data is in 8 bit per pixel format.
        //
        case 8:
        {
            //
            // Loop while there are more pixels to draw.
            //
            while(i32Count--)
            {
                //
                // Get the next byte of pixel data and extract the
                // corresponding entry from the palette.
                //
                ui32Byte = *pui8Data++ * 3;
                ui32Byte = *(uint32_t *)(pui8Palette + ui32Byte) & 0x00ffffff;

                //
                // Translate this palette entry and write it to the screen.
                //
                *pui8Ptr++ = GrOffScreen8BPPColorTranslate(pvDisplayData,
                                                          ui32Byte);
            }

            //
            // The image data has been drawn.
            //
            break;
        }
    }
}

//*****************************************************************************
//
//! Draws a horizontal line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param i32X1 is the X coordinate of the start of the line.
//! \param i32X2 is the X coordinate of the end of the line.
//! \param i32Y is the Y coordinate of the line.
//! \param ui32Value is the color of the line.
//!
//! This function draws a horizontal line on the display.  The coordinates of
//! the line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void
GrOffScreen8BPPLineDrawH(void *pvDisplayData, int32_t i32X1, int32_t i32X2,
                         int32_t i32Y, uint32_t ui32Value)
{
    uint8_t *pui8Data;

    //
    // Check the arguments.
    //
    ASSERT(pvDisplayData);

    //
    // Create a character pointer for the display-specific data (which points
    // to the image buffer).
    //
    pui8Data = (uint8_t *)pvDisplayData;

    //
    // Get the offset to the byte of the image buffer that contains the
    // starting pixel.
    //
    pui8Data += (*(uint16_t *)(pui8Data + 1) * i32Y) + i32X1 + 6 + (256 * 3);

    //
    // Copy the pixel value into all 4 pixels of the uint32_t.  This will
    // be used later to write multiple pixels into memory (as opposed to one at
    // a time).
    //
    ui32Value = (ui32Value << 24) | (ui32Value << 16) |
                (ui32Value << 8) | ui32Value;

    //
    // See if the buffer pointer is not half-word aligned.
    //
    if(((uint32_t)pui8Data) & 1)
    {
        //
        // Draw one pixel to half-word align the buffer pointer.
        //
        *pui8Data++ = ui32Value & 0xff;
        i32X1++;
    }

    //
    // See if the buffer pointer is not word aligned and there are at least two
    // pixels left to draw.
    //
    if(((uint32_t)pui8Data & 2) && ((i32X2 - i32X1) > 0))
    {
        //
        // Draw two pixels to word align the buffer pointer.
        //
        *(uint16_t *)pui8Data = ui32Value & 0xffff;
        pui8Data += 2;
        i32X1 += 2;
    }

    //
    // Loop while there are at least four pixels left to draw.
    //
    while((i32X1 + 3) <= i32X2)
    {
        //
        // Draw four pixels.
        //
        *(uint32_t *)pui8Data = ui32Value;
        pui8Data += 4;
        i32X1 += 4;
    }

    //
    // See if there are at least two pixels left to draw.
    //
    if((i32X1 + 1) <= i32X2)
    {
        //
        // Draw two pixels, leaving the buffer pointer half-word aligned.
        //
        *(uint16_t *)pui8Data = ui32Value & 0xffff;
        pui8Data += 2;
        i32X1 += 2;
    }

    //
    // See if there is one pixel left to draw.
    //
    if(i32X1 == i32X2)
    {
        //
        // Draw the final pixel.
        //
        *pui8Data = ui32Value & 0xff;
    }
}

//*****************************************************************************
//
//! Draws a vertical line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param i32X is the X coordinate of the line.
//! \param i32Y1 is the Y coordinate of the start of the line.
//! \param i32Y2 is the Y coordinate of the end of the line.
//! \param ui32Value is the color of the line.
//!
//! This function draws a vertical line on the display.  The coordinates of the
//! line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void
GrOffScreen8BPPLineDrawV(void *pvDisplayData, int32_t i32X, int32_t i32Y1,
                         int32_t i32Y2, uint32_t ui32Value)
{
    uint8_t *pui8Data;
    int32_t i32BytesPerRow;

    //
    // Check the arguments.
    //
    ASSERT(pvDisplayData);

    //
    // Create a character pointer for the display-specific data (which points
    // to the image buffer).
    //
    pui8Data = (uint8_t *)pvDisplayData;

    //
    // Compute the number of bytes per row in the image buffer.
    //
    i32BytesPerRow = *(uint16_t *)(pui8Data + 1);

    //
    // Get the offset to the byte of the image buffer that contains the
    // starting pixel.
    //
    pui8Data += (i32BytesPerRow * i32Y1) + i32X + 6 + (256 * 3);

    //
    // Loop over the rows of the line.
    //
    for(; i32Y1 <= i32Y2; i32Y1++)
    {
        *pui8Data = ui32Value;
        pui8Data += i32BytesPerRow;
    }
}

//*****************************************************************************
//
//! Fills a rectangle.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param pRect is a pointer to the structure describing the rectangle.
//! \param ui32Value is the color of the rectangle.
//!
//! This function fills a rectangle on the display.  The coordinates of the
//! rectangle are assumed to be within the extents of the display, and the
//! rectangle specification is fully inclusive (in other words, both i16XMin
//! and i16XMax are drawn, along with i16YMin and i16YMax).
//!
//! \return None.
//
//*****************************************************************************
static void
GrOffScreen8BPPRectFill(void *pvDisplayData, const tRectangle *pRect,
                        uint32_t ui32Value)
{
    uint8_t *pui8Data, *pui8Ptr;
    int32_t i32BytesPerRow, i32X, i32Y;

    //
    // Check the arguments.
    //
    ASSERT(pvDisplayData);
    ASSERT(pRect);

    //
    // Create a character pointer for the display-specific data (which points
    // to the image buffer).
    //
    pui8Data = (uint8_t *)pvDisplayData;

    //
    // Compute the number of bytes per row in the image buffer.
    //
    i32BytesPerRow = *(uint16_t *)(pui8Data + 1);

    //
    // Get the offset to the byte of the image buffer that contains the
    // starting pixel.
    //
    pui8Data += (i32BytesPerRow * pRect->i16YMin) + pRect->i16XMin +
                 6 + (256 * 3);

    //
    // Copy the pixel value into all 4 pixels of the uint32_t.  This will
    // be used later to write multiple pixels into memory (as opposed to one at
    // a time).
    //
    ui32Value = (ui32Value << 24) | (ui32Value << 16) | (ui32Value << 8) |
                ui32Value;

    //
    // Get the starting X coordinate.
    //
    i32X = pRect->i16XMin;

    //
    // See if the buffer pointer is not half-word aligned.
    //
    if(((uint32_t)pui8Data) & 1)
    {
        //
        // Draw one pixel column to half-word align the buffer pointer.
        //
        for(i32Y = pRect->i16YMin, pui8Ptr = pui8Data; i32Y <= pRect->i16YMax;
            i32Y++, pui8Ptr += i32BytesPerRow)
        {
            *pui8Ptr = ui32Value & 0xff;
        }
        pui8Data++;
        i32X++;
    }

    //
    // See if the buffer pointer is not word aligned and there are at least two
    // pixel columns left to draw.
    //
    if(((uint32_t)pui8Data & 2) && ((pRect->i16XMax - i32X) > 0))
    {
        //
        // Draw two pixel columns to word align the buffer pointer.
        //
        for(i32Y = pRect->i16YMin, pui8Ptr = pui8Data; i32Y <= pRect->i16YMax;
            i32Y++, pui8Ptr += i32BytesPerRow)
        {
            *(uint16_t *)pui8Ptr = ui32Value & 0xffff;
        }
        pui8Data += 2;
        i32X += 2;
    }

    //
    // Loop while there are at least four pixel columns left to draw.
    //
    while((i32X + 3) <= pRect->i16XMax)
    {
        //
        // Draw four pixel columns.
        //
        for(i32Y = pRect->i16YMin, pui8Ptr = pui8Data; i32Y <= pRect->i16YMax;
            i32Y++, pui8Ptr += i32BytesPerRow)
        {
            *(uint32_t *)pui8Ptr = ui32Value;
        }
        pui8Data += 4;
        i32X += 4;
    }

    //
    // See if ther are at least two pixel columns left to draw.
    //
    if((i32X + 1) <= pRect->i16XMax)
    {
        //
        // Draw two pixel columns, leaving the buffer pointer half-word
        // aligned.
        //
        ui32Value &= 0xffff;
        for(i32Y = pRect->i16YMin, pui8Ptr = pui8Data; i32Y <= pRect->i16YMax;
            i32Y++, pui8Ptr += i32BytesPerRow)
        {
            *(uint16_t *)pui8Ptr = ui32Value;
        }
        pui8Data += 2;
        i32X += 2;
    }

    //
    // See if there is one pixel column left to draw.
    //
    if(i32X == pRect->i16XMax)
    {
        //
        // Draw the final pixel column.
        //
        ui32Value &= 0xff;
        for(i32Y = pRect->i16YMin; i32Y <= pRect->i16YMax;
            i32Y++, pui8Data += i32BytesPerRow)
        {
            *pui8Data = ui32Value;
        }
    }
}

//*****************************************************************************
//
//! Flushes any cached drawing operations.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//!
//! This functions flushes any cached drawing operations to the display.  This
//! is useful when a local frame buffer is used for drawing operations, and the
//! flush would copy the local frame buffer to the display.  For the off-screen
//! display buffer driver, the flush is a no operation.
//!
//! \return None.
//
//*****************************************************************************
static void
GrOffScreen8BPPFlush(void *pvDisplayData)
{
    //
    // Check the arguments.
    //
    ASSERT(pvDisplayData);
}

//*****************************************************************************
//
//! Initializes an 8 BPP off-screen buffer.
//!
//! \param psDisplay is a pointer to the display structure to be configured for
//! the 4 BPP off-screen buffer.
//! \param pui8Image is a pointer to the image buffer to be used for the
//! off-screen buffer.
//! \param i32Width is the width of the image buffer in pixels.
//! \param i32Height is the height of the image buffer in pixels.
//!
//! This function initializes a display structure, preparing it to draw into
//! the supplied image buffer.  The image buffer is assumed to be large enough
//! to hold an image of the specified geometry.
//!
//! \return None.
//
//*****************************************************************************
void
GrOffScreen8BPPInit(tDisplay *psDisplay, uint8_t *pui8Image, int32_t i32Width,
                    int32_t i32Height)
{
    //
    // Check the arguments.
    //
    ASSERT(psDisplay);
    ASSERT(pui8Image);

    //
    // Initialize the display structure.
    //
    psDisplay->i32Size = sizeof(tDisplay);
    psDisplay->pvDisplayData = pui8Image;
    psDisplay->ui16Width = i32Width;
    psDisplay->ui16Height = i32Height;
    psDisplay->pfnPixelDraw = GrOffScreen8BPPPixelDraw;
    psDisplay->pfnPixelDrawMultiple = GrOffScreen8BPPPixelDrawMultiple;
    psDisplay->pfnLineDrawH = GrOffScreen8BPPLineDrawH;
    psDisplay->pfnLineDrawV = GrOffScreen8BPPLineDrawV;
    psDisplay->pfnRectFill = GrOffScreen8BPPRectFill;
    psDisplay->pfnColorTranslate = GrOffScreen8BPPColorTranslate;
    psDisplay->pfnFlush = GrOffScreen8BPPFlush;

    //
    // Initialize the image buffer.
    //
    pui8Image[0] = IMAGE_FMT_8BPP_UNCOMP;
    *(uint16_t *)(pui8Image + 1) = i32Width;
    *(uint16_t *)(pui8Image + 3) = i32Height;
    pui8Image[5] = 255;
}

//*****************************************************************************
//
//! Sets the palette of an 8 BPP off-screen buffer.
//!
//! \param psDisplay is a pointer to the display structure for the 4 BPP
//! off-screen buffer.
//! \param pui32Palette is a pointer to the array of 24-bit RGB values to be
//! placed into the palette.
//! \param ui32Offset is the starting offset into the image palette.
//! \param ui32Count is the number of palette entries to set.
//!
//! This function sets the entries of the palette used by the 8 BPP off-screen
//! buffer.  The palette is used to select colors for drawing via
//! GrOffScreen4BPPColorTranslate(), and for the final rendering of the image
//! to a real display via GrImageDraw().
//!
//! \return None.
//
//*****************************************************************************
void
GrOffScreen8BPPPaletteSet(tDisplay *psDisplay, uint32_t *pui32Palette,
                          uint32_t ui32Offset, uint32_t ui32Count)
{
    uint8_t *pui8Data;

    //
    // Check the arguments.
    //
    ASSERT(psDisplay);
    ASSERT(pui32Palette);
    ASSERT(ui32Offset < 256);
    ASSERT((ui32Offset + ui32Count) <= 256);

    //
    // Get a pointer to the start of the image buffer's palette.
    //
    pui8Data = (uint8_t *)psDisplay->pvDisplayData + 6;

    //
    // Skip to the specified offset in the palette.
    //
    pui8Data += ui32Offset * 3;

    //
    // Loop while there are more palette entries to add.
    //
    while(ui32Count--)
    {
        //
        // Copy this palette entry to the image buffer's palette.
        //
        *pui8Data++ = (*pui32Palette >> ClrBlueShift) & 0xff;
        *pui8Data++ = (*pui32Palette >> ClrGreenShift) & 0xff;
        *pui8Data++ = (*pui32Palette++ >> ClrRedShift) & 0xff;
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
