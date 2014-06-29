//*****************************************************************************
//
// kentec320x240x16_ssd2119.c - Display driver for the Kentec K350QVG-V2-F
//                              TFT display attached to the LCD controller via
//                              an 8-bit LIDD interface.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C129X Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "driverlib/lcd.h"
#include "grlib/grlib.h"
#include "drivers/kentec320x240x16_ssd2119.h"

//*****************************************************************************
//
//! \addtogroup kentec320x240x16_ssd2119_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// This driver operates in four different screen orientations.  They are:
//
// * Portrait - The screen is taller than it is wide, and the flex connector is
//              on the left of the display.  This is selected by defining
//              PORTRAIT.
//
// * Landscape - The screen is wider than it is tall, and the flex connector is
//               on the bottom of the display.  This is selected by defining
//               LANDSCAPE.
//
// * Portrait flip - The screen is taller than it is wide, and the flex
//                   connector is on the right of the display.  This is
//                   selected by defining PORTRAIT_FLIP.
//
// * Landscape flip - The screen is wider than it is tall, and the flex
//                    connector is on the top of the display.  This is
//                    selected by defining LANDSCAPE_FLIP.
//
// These can also be imagined in terms of screen rotation; if portrait mode is
// 0 degrees of screen rotation, landscape is 90 degrees of counter-clockwise
// rotation, portrait flip is 180 degrees of rotation, and landscape flip is
// 270 degress of counter-clockwise rotation.
//
// If no screen orientation is selected, landscape mode will be used.
//
//*****************************************************************************
#if ! defined(PORTRAIT) && ! defined(PORTRAIT_FLIP) &&                        \
    ! defined(LANDSCAPE) && ! defined(LANDSCAPE_FLIP)
#define LANDSCAPE_FLIP
#endif

//*****************************************************************************
//
// Various definitions controlling coordinate space mapping and drawing
// direction in the four supported orientations.
//
//*****************************************************************************
#ifdef PORTRAIT
#define HORIZ_DIRECTION         0x28
#define VERT_DIRECTION          0x20
#define MAPPED_X(x, y)          (319 - (y))
#define MAPPED_Y(x, y)          (x)
#endif
#ifdef LANDSCAPE
#define HORIZ_DIRECTION         0x00
#define VERT_DIRECTION          0x08
#define MAPPED_X(x, y)          (319 - (x))
#define MAPPED_Y(x, y)          (239 - (y))
#endif
#ifdef PORTRAIT_FLIP
#define HORIZ_DIRECTION         0x18
#define VERT_DIRECTION          0x10
#define MAPPED_X(x, y)          (y)
#define MAPPED_Y(x, y)          (239 - (x))
#endif
#ifdef LANDSCAPE_FLIP
#define HORIZ_DIRECTION         0x30
#define VERT_DIRECTION          0x38
#define MAPPED_X(x, y)          (x)
#define MAPPED_Y(x, y)          (y)
#endif

//*****************************************************************************
//
// Various internal SD2119 registers name labels
//
//*****************************************************************************
#define SSD2119_DEVICE_CODE_READ_REG                                          \
                                0x00
#define SSD2119_OSC_START_REG   0x00
#define SSD2119_OUTPUT_CTRL_REG 0x01
#define SSD2119_LCD_DRIVE_AC_CTRL_REG                                         \
                                0x02
#define SSD2119_PWR_CTRL_1_REG  0x03
#define SSD2119_DISPLAY_CTRL_REG                                              \
                                0x07
#define SSD2119_FRAME_CYCLE_CTRL_REG                                          \
                                0x0b
#define SSD2119_PWR_CTRL_2_REG  0x0c
#define SSD2119_PWR_CTRL_3_REG  0x0d
#define SSD2119_PWR_CTRL_4_REG  0x0e
#define SSD2119_GATE_SCAN_START_REG                                           \
                                0x0f
#define SSD2119_SLEEP_MODE_1_REG                                              \
                                0x10
#define SSD2119_ENTRY_MODE_REG  0x11
#define SSD2119_SLEEP_MODE_2_REG                                              \
                                0x12
#define SSD2119_GEN_IF_CTRL_REG 0x15
#define SSD2119_PWR_CTRL_5_REG  0x1e
#define SSD2119_RAM_DATA_REG    0x22
#define SSD2119_FRAME_FREQ_REG  0x25
#define SSD2119_ANALOG_SET_REG  0x26
#define SSD2119_VCOM_OTP_1_REG  0x28
#define SSD2119_VCOM_OTP_2_REG  0x29
#define SSD2119_GAMMA_CTRL_1_REG                                              \
                                0x30
#define SSD2119_GAMMA_CTRL_2_REG                                              \
                                0x31
#define SSD2119_GAMMA_CTRL_3_REG                                              \
                                0x32
#define SSD2119_GAMMA_CTRL_4_REG                                              \
                                0x33
#define SSD2119_GAMMA_CTRL_5_REG                                              \
                                0x34
#define SSD2119_GAMMA_CTRL_6_REG                                              \
                                0x35
#define SSD2119_GAMMA_CTRL_7_REG                                              \
                                0x36
#define SSD2119_GAMMA_CTRL_8_REG                                              \
                                0x37
#define SSD2119_GAMMA_CTRL_9_REG                                              \
                                0x3a
#define SSD2119_GAMMA_CTRL_10_REG                                             \
                                0x3b
#define SSD2119_V_RAM_POS_REG   0x44
#define SSD2119_H_RAM_START_REG 0x45
#define SSD2119_H_RAM_END_REG   0x46
#define SSD2119_X_RAM_ADDR_REG  0x4e
#define SSD2119_Y_RAM_ADDR_REG  0x4f

#define ENTRY_MODE_DEFAULT      0x6830
#define MAKE_ENTRY_MODE(x)      ((ENTRY_MODE_DEFAULT & 0xff00) | (x))

//*****************************************************************************
//
// Read Access Timing
// ------------------
//
// Direction  OOOIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIOOOOOOOOOOOOO
//
// ~RD        -----                    --------------------------
//                 \                  /                          |
//                  ------------------
//                 <       Trdl       ><        Trdh             >
//                 <                  Tcycle                     >
//                 < Tacc >
//                         /------------------|
// DATA       -------------                    ------------------
//                         \------------------/
//                                     < Tdh  >
//
// Delays          <   Trad  >< Tdhd ><    Trhd   ><  Trcd      >
//
// This design keeps CS tied low so pulse width constraints relating to CS
// have been transfered to ~RD here.
//
// Tcycle Read Cycle Time  1000nS
// Tacc   Data Access Time  100nS
// Trdl   Read Data Low     500nS
// Trdh   Read Data High    500nS
// Tdh    Data Hold Time    100nS
//
// Trad (READ_DATA_ACCESS_DELAY) controls the delay between asserting ~RD and
//       reading the data from the bus.
// Tdhd (READ_DATA_HOLD_DELAY) controls the delay after reading the data and
//       before deasserting ~RD.
// Trhd (READ_HOLD_DELAY) controls the delay between deasserting ~RD and
//       switching the data bus direction back to output.
// Trcd (READ_DATA_CYCLE_DELAY) controls the delay after switching the
//       direction of the data bus.
//
//*****************************************************************************

//*****************************************************************************
//
// The delay to impose after setting the state of the read/write line and
// before reading the data bus.  This is expressed in terms of cycles of a
// tight loop whose body performs a single GPIO register access and needs to
// comply with the 500nS read cycle pulse width constraint.
//
//*****************************************************************************
#define READ_DATA_ACCESS_DELAY  5

//*****************************************************************************
//
// The delay to impose after reading the data and before resetting the state of
// the read/write line during a read operation.  This is expressed in terms of
// cycles of a tight loop whose body performs a single GPIO register access and
// needs to comply with the 500nS read cycle pulse width constraint.
//
//*****************************************************************************
#define READ_DATA_HOLD_DELAY    5

//*****************************************************************************
//
// The delay to impose after deasserting ~RD and before setting the bus back to
// an output.  This is expressed in terms of cycles of a tight loop whose body
// performs a single GPIO register access.
//
//*****************************************************************************
#define READ_HOLD_DELAY         5

//*****************************************************************************
//
// The delay to impose after completing a read cycle and before returning to
// the caller.  This is expressed in terms of cycles of a tight loop whose body
// performs a single GPIO register access and needs to comply with the 1000nS
// read cycle pulse width constraint.
//
//*****************************************************************************
#define READ_DATA_CYCLE_DELAY   5

//*****************************************************************************
//
// The dimensions of the LCD panel.
//
//*****************************************************************************
#define LCD_HORIZONTAL_MAX      320
#define LCD_VERTICAL_MAX        240

//*****************************************************************************
//
// Translates a 24-bit RGB color to a display driver-specific color.
//
// \param c is the 24-bit RGB color.  The least-significant byte is the blue
// channel, the next byte is the green channel, and the third byte is the red
// channel.
//
// This macro translates a 24-bit RGB color into a value that can be written
// into the display's frame buffer in order to reproduce that color, or the
// closest possible approximation of that color.
//
// \return Returns the display-driver specific color.
//
//*****************************************************************************
#define DPYCOLORTRANSLATE(c)    ((((c) & 0x00f80000) >> 8) |                  \
                                 (((c) & 0x0000fc00) >> 5) |                  \
                                 (((c) & 0x000000f8) >> 3))

//*****************************************************************************
//
// Writes a data word to the SSD2119.
//
//*****************************************************************************
static inline void
WriteData(uint16_t ui16Data)
{
    //
    // Split the write into two bytes and pass them to the LCD controller.
    //
    LCDIDDDataWrite(LCD0_BASE, 0, ui16Data >> 8);
    LCDIDDDataWrite(LCD0_BASE, 0, ui16Data & 0xff);
}

//*****************************************************************************
//
// Writes a command to the SSD2119.
//
//*****************************************************************************
static inline void
WriteCommand(uint8_t ui8Data)
{
    //
    // Pass the write on to the controller.
    //
    LCDIDDCommandWrite(LCD0_BASE, 0, (uint16_t)ui8Data);
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
Kentec320x240x16_SSD2119PixelDraw(void *pvDisplayData, int32_t i32X,
                                  int32_t i32Y, uint32_t ui32Value)
{
    //
    // Set the X address of the display cursor.
    //
    WriteCommand(SSD2119_X_RAM_ADDR_REG);
    WriteData(MAPPED_X(i32X, i32Y));

    //
    // Set the Y address of the display cursor.
    //
    WriteCommand(SSD2119_Y_RAM_ADDR_REG);
    WriteData(MAPPED_Y(i32X, i32Y));

    //
    // Write the pixel value.
    //
    WriteCommand(SSD2119_RAM_DATA_REG);
    WriteData(ui32Value);
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
//! \param i32BPP is the number of bits per pixel; must be 1, 4, or 8.
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
//! \return None.
//
//*****************************************************************************
static void
Kentec320x240x16_SSD2119PixelDrawMultiple(void *pvDisplayData, int32_t i32X,
                                          int32_t i32Y, int32_t i32X0,
                                          int32_t i32Count, int32_t i32BPP,
                                          const uint8_t *pui8Data,
                                          const uint8_t *pui8Palette)
{
    uint32_t ui32Byte;

    //
    // Set the cursor increment to left to right, followed by top to bottom.
    //
    WriteCommand(SSD2119_ENTRY_MODE_REG);
    WriteData(MAKE_ENTRY_MODE(HORIZ_DIRECTION));

    //
    // Set the starting X address of the display cursor.
    //
    WriteCommand(SSD2119_X_RAM_ADDR_REG);
    WriteData(MAPPED_X(i32X, i32Y));

    //
    // Set the Y address of the display cursor.
    //
    WriteCommand(SSD2119_Y_RAM_ADDR_REG);
    WriteData(MAPPED_Y(i32X, i32Y));

    //
    // Write the data RAM write command.
    //
    WriteCommand(SSD2119_RAM_DATA_REG);

    //
    // Determine how to interpret the pixel data based on the number of bits
    // per pixel.
    //
    switch(i32BPP & ~GRLIB_DRIVER_FLAG_NEW_IMAGE)
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
                    WriteData(((uint32_t *)pui8Palette)[(ui32Byte >>
                                                         (7 - i32X0)) & 1]);
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
                        WriteData(DPYCOLORTRANSLATE(ui32Byte));

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
                            WriteData(DPYCOLORTRANSLATE(ui32Byte));

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
                WriteData(DPYCOLORTRANSLATE(ui32Byte));
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
Kentec320x240x16_SSD2119LineDrawH(void *pvDisplayData, int32_t i32X1,
                                  int32_t i32X2, int32_t i32Y,
                                  uint32_t ui32Value)
{
    //
    // Set the cursor increment to left to right, followed by top to bottom.
    //
    WriteCommand(SSD2119_ENTRY_MODE_REG);
    WriteData(MAKE_ENTRY_MODE(HORIZ_DIRECTION));

    //
    // Set the starting X address of the display cursor.
    //
    WriteCommand(SSD2119_X_RAM_ADDR_REG);
    WriteData(MAPPED_X(i32X1, i32Y));

    //
    // Set the Y address of the display cursor.
    //
    WriteCommand(SSD2119_Y_RAM_ADDR_REG);
    WriteData(MAPPED_Y(i32X1, i32Y));

    //
    // Write the data RAM write command.
    //
    WriteCommand(SSD2119_RAM_DATA_REG);

    //
    // Loop through the pixels of this horizontal line.
    //
    while(i32X1++ <= i32X2)
    {
        //
        // Write the pixel value.
        //
        WriteData(ui32Value);
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
Kentec320x240x16_SSD2119LineDrawV(void *pvDisplayData, int32_t i32X,
                                  int32_t i32Y1, int32_t i32Y2,
                                  uint32_t ui32Value)
{
    //
    // Set the cursor increment to top to bottom, followed by left to right.
    //
    WriteCommand(SSD2119_ENTRY_MODE_REG);
    WriteData(MAKE_ENTRY_MODE(VERT_DIRECTION));

    //
    // Set the X address of the display cursor.
    //
    WriteCommand(SSD2119_X_RAM_ADDR_REG);
    WriteData(MAPPED_X(i32X, i32Y1));

    //
    // Set the starting Y address of the display cursor.
    //
    WriteCommand(SSD2119_Y_RAM_ADDR_REG);
    WriteData(MAPPED_Y(i32X, i32Y1));

    //
    // Write the data RAM write command.
    //
    WriteCommand(SSD2119_RAM_DATA_REG);

    //
    // Loop through the pixels of this vertical line.
    //
    while(i32Y1++ <= i32Y2)
    {
        //
        // Write the pixel value.
        //
        WriteData(ui32Value);
    }
}

//*****************************************************************************
//
//! Fills a rectangle.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param psRect is a pointer to the structure describing the rectangle.
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
Kentec320x240x16_SSD2119RectFill(void *pvDisplayData, const tRectangle *psRect,
                                 uint32_t ui32Value)
{
    int32_t i32Count;

    //
    // Write the Y extents of the rectangle.
    //
    WriteCommand(SSD2119_ENTRY_MODE_REG);
    WriteData(MAKE_ENTRY_MODE(HORIZ_DIRECTION));

    //
    // Write the X extents of the rectangle.
    //
    WriteCommand(SSD2119_H_RAM_START_REG);
#if (defined PORTRAIT) || (defined LANDSCAPE)
    WriteData(MAPPED_X(psRect->i16XMax, psRect->i16YMax));
#else
    WriteData(MAPPED_X(psRect->i16XMin, psRect->i16YMin));
#endif

    WriteCommand(SSD2119_H_RAM_END_REG);
#if (defined PORTRAIT) || (defined LANDSCAPE)
    WriteData(MAPPED_X(psRect->i16XMin, psRect->i16YMin));
#else
    WriteData(MAPPED_X(psRect->i16XMax, psRect->i16YMax));
#endif

    //
    // Write the Y extents of the rectangle
    //
    WriteCommand(SSD2119_V_RAM_POS_REG);
#if (defined LANDSCAPE_FLIP) || (defined PORTRAIT)
    WriteData(MAPPED_Y(psRect->i16XMin, psRect->i16YMin) |
             (MAPPED_Y(psRect->i16XMax, psRect->i16YMax) << 8));
#else
    WriteData(MAPPED_Y(psRect->i16XMax, psRect->i16YMax) |
             (MAPPED_Y(psRect->i16XMin, psRect->i16YMin) << 8));
#endif

    //
    // Set the display cursor to the upper left of the rectangle (in
    // application coordinate space).
    //
    WriteCommand(SSD2119_X_RAM_ADDR_REG);
    WriteData(MAPPED_X(psRect->i16XMin, psRect->i16YMin));
    WriteCommand(SSD2119_Y_RAM_ADDR_REG);
    WriteData(MAPPED_Y(psRect->i16XMin, psRect->i16YMin));

    //
    // Tell the controller to write data into its RAM.
    //
    WriteCommand(SSD2119_RAM_DATA_REG);

    //
    // Loop through the pixels of this filled rectangle.
    //
    for(i32Count = ((psRect->i16XMax - psRect->i16XMin + 1) *
                    (psRect->i16YMax - psRect->i16YMin + 1)); i32Count >= 0;
        i32Count--)
    {
        //
        // Write the pixel value.
        //
        WriteData(ui32Value);
    }

    //
    // Reset the X extents to the entire screen.
    //
    WriteCommand(SSD2119_H_RAM_START_REG);
    WriteData(0x0000);
    WriteCommand(SSD2119_H_RAM_END_REG);
    WriteData(0x013f);

    //
    // Reset the Y extent to the full screen
    //
    WriteCommand(SSD2119_V_RAM_POS_REG);
    WriteData(0xef00);
}

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
Kentec320x240x16_SSD2119ColorTranslate(void *pvDisplayData, uint32_t ui32Value)
{
    //
    // Translate from a 24-bit RGB color to a 5-6-5 RGB color.
    //
    return(DPYCOLORTRANSLATE(ui32Value));
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
//! flush would copy the local frame buffer to the display.  For the SSD2119
//! driver, the flush is a no operation.
//!
//! \return None.
//
//*****************************************************************************
static void
Kentec320x240x16_SSD2119Flush(void *pvDisplayData)
{
    //
    // There is nothing to be done.
    //
}

//*****************************************************************************
//
//! The display structure that describes the driver for the Kentec K350QVG-V2-F
//! TFT panel with an SSD2119 controller.
//
//*****************************************************************************
const tDisplay g_sKentec320x240x16_SSD2119 =
{
    sizeof(tDisplay),
    0,
#if defined(PORTRAIT) || defined(PORTRAIT_FLIP)
    240,
    320,
#else
    320,
    240,
#endif
    Kentec320x240x16_SSD2119PixelDraw,
    Kentec320x240x16_SSD2119PixelDrawMultiple,
    Kentec320x240x16_SSD2119LineDrawH,
    Kentec320x240x16_SSD2119LineDrawV,
    Kentec320x240x16_SSD2119RectFill,
    Kentec320x240x16_SSD2119ColorTranslate,
    Kentec320x240x16_SSD2119Flush
};

//*****************************************************************************
//
//! Initializes the display driver.
//!
//! \param ui32SysClock is the frequency of the system clock.
//!
//! This function initializes the LCD controller and the SSD2119 display
//! controller on the panel, preparing it to display data.
//!
//! \return None.
//
//*****************************************************************************
void
Kentec320x240x16_SSD2119Init(uint32_t ui32SysClock)
{
    uint32_t ui32ClockMS, ui32Count;
    tLCDIDDTiming sTimings;

    //
    // Determine the number of system clock cycles in 1mS
    //
    ui32ClockMS = CYCLES_FROM_TIME_US(ui32SysClock, 1000);

    //
    // Divide by 3 to get the number of SysCtlDelay loops in 1mS.
    //
    ui32ClockMS /= 3;

    //
    // Enable the LCD controller.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_LCD0);

    //
    // Assert the LCD reset signal.
    //
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_6, 0);

    //
    // Delay for 50ms.
    //
    SysCtlDelay(50 * ui32ClockMS);

    //
    // Deassert the LCD reset signal.
    //
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_6, GPIO_PIN_6);

    //
    // Delay for 50ms while the LCD comes out of reset.
    //
    SysCtlDelay(50 * ui32ClockMS);

    //
    // Configure the LCD controller for LIDD-mode operation.
    //
    LCDModeSet(LCD0_BASE, LCD_MODE_LIDD, ui32SysClock, ui32SysClock);

    //
    // Configure DMA-related parameters.
    //
    LCDDMAConfigSet(LCD0_BASE, LCD_DMA_BURST_4);

    //
    // Set control signal parameters and polarities.
    //
    LCDIDDConfigSet(LCD0_BASE, LIDD_CONFIG_ASYNC_MPU80);

    //
    // Set the LIDD interface timings for the Kentec display.  Note that the
    // inter-transaction delay is set at at 50nS to match the write case.
    // Software needs to ensure that it delays at least 450nS more between each
    // read or the read timings will be violated.
    //
    sTimings.ui8WSSetup = CYCLES_FROM_TIME_NS(ui32SysClock, 5);
    sTimings.ui8WSDuration = CYCLES_FROM_TIME_NS(ui32SysClock, 40);
    sTimings.ui8WSHold = CYCLES_FROM_TIME_NS(ui32SysClock, 5);
    sTimings.ui8RSSetup = CYCLES_FROM_TIME_NS(ui32SysClock, 0);
    sTimings.ui8RSDuration = CYCLES_FROM_TIME_NS(ui32SysClock, 500);
    sTimings.ui8RSHold = CYCLES_FROM_TIME_NS(ui32SysClock, 100);
    sTimings.ui8DelayCycles = CYCLES_FROM_TIME_NS(ui32SysClock, 50);
    LCDIDDTimingSet(LCD0_BASE, 0, &sTimings);

    //
    // Enter sleep mode (if not already there).
    //
    WriteCommand(SSD2119_SLEEP_MODE_1_REG);
    WriteData(0x0001);

    //
    // Set initial power parameters.
    //
    WriteCommand(SSD2119_PWR_CTRL_5_REG);
    WriteData(0x00b2);
    WriteCommand(SSD2119_VCOM_OTP_1_REG);
    WriteData(0x0006);

    //
    // Start the oscillator.
    //
    WriteCommand(SSD2119_OSC_START_REG);
    WriteData(0x0001);

    //
    // Set pixel format and basic display orientation (scanning direction).
    //
    WriteCommand(SSD2119_OUTPUT_CTRL_REG);
    WriteData(0x30ef);
    WriteCommand(SSD2119_LCD_DRIVE_AC_CTRL_REG);
    WriteData(0x0600);

    //
    // Exit sleep mode.
    //
    WriteCommand(SSD2119_SLEEP_MODE_1_REG);
    WriteData(0x0000);

    //
    // Delay 30mS
    //
    SysCtlDelay(30 * ui32ClockMS);

    //
    // Configure pixel color format and MCU interface parameters.
    //
    WriteCommand(SSD2119_ENTRY_MODE_REG);
    WriteData(ENTRY_MODE_DEFAULT);

    //
    // Set analog parameters.
    //
    WriteCommand(SSD2119_SLEEP_MODE_2_REG);
    WriteData(0x0999);
    WriteCommand(SSD2119_ANALOG_SET_REG);
    WriteData(0x3800);

    //
    // Enable the display.
    //
    WriteCommand(SSD2119_DISPLAY_CTRL_REG);
    WriteData(0x0033);

    //
    // Set VCIX2 voltage to 6.1V.
    //
    WriteCommand(SSD2119_PWR_CTRL_2_REG);
    WriteData(0x0005);

    //
    // Configure gamma correction.
    //
    WriteCommand(SSD2119_GAMMA_CTRL_1_REG);
    WriteData(0x0000);
    WriteCommand(SSD2119_GAMMA_CTRL_2_REG);
    WriteData(0x0303);
    WriteCommand(SSD2119_GAMMA_CTRL_3_REG);
    WriteData(0x0407);
    WriteCommand(SSD2119_GAMMA_CTRL_4_REG);
    WriteData(0x0301);
    WriteCommand(SSD2119_GAMMA_CTRL_5_REG);
    WriteData(0x0301);
    WriteCommand(SSD2119_GAMMA_CTRL_6_REG);
    WriteData(0x0403);
    WriteCommand(SSD2119_GAMMA_CTRL_7_REG);
    WriteData(0x0707);
    WriteCommand(SSD2119_GAMMA_CTRL_8_REG);
    WriteData(0x0400);
    WriteCommand(SSD2119_GAMMA_CTRL_9_REG);
    WriteData(0x0a00);
    WriteCommand(SSD2119_GAMMA_CTRL_10_REG);
    WriteData(0x1000);

    //
    // Configure Vlcd63 and VCOMl.
    //
    WriteCommand(SSD2119_PWR_CTRL_3_REG);
    WriteData(0x000a);
    WriteCommand(SSD2119_PWR_CTRL_4_REG);
    WriteData(0x2e00);

    //
    // Set the display size and ensure that the GRAM window is set to allow
    // access to the full display buffer.
    //
    WriteCommand(SSD2119_V_RAM_POS_REG);
    WriteData((LCD_VERTICAL_MAX-1) << 8);
    WriteCommand(SSD2119_H_RAM_START_REG);
    WriteData(0x0000);
    WriteCommand(SSD2119_H_RAM_END_REG);
    WriteData(LCD_HORIZONTAL_MAX-1);
    WriteCommand(SSD2119_X_RAM_ADDR_REG);
    WriteData(0x0000);
    WriteCommand(SSD2119_Y_RAM_ADDR_REG);
    WriteData(0x0000);

    //
    // Clear the contents of the display buffer.
    //
    WriteCommand(SSD2119_RAM_DATA_REG);
    for(ui32Count = 0; ui32Count < (320 * 240); ui32Count++)
    {
        WriteData(0x0000);
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
