//*****************************************************************************
//
// grlib_driver_test.c - A test tool intended to aid developers of drivers
//                       for the TivaWare Graphics Library
//
// Copyright (c) 2012-2014 Texas Instruments Incorporated.  All rights reserved.
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

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_lcd.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/lcd.h"
#include "driverlib/interrupt.h"
#include "driverlib/systick.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "grlib/grlib.h"
#include "utils/cmdline.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "images.h"
#include "driver_config.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Graphics Driver Test Tool (grlib_driver_test)</h1>
//!
//! This application provides a simple, command-line tool to aid in
//! debugging TivaWare Graphics Library display drivers.  As shipped in
//! TivaWare, it is configured to operate with a DK-TM4C129X board using its
//! QVGA display.  The code is written, however, to allow easy retargeting to
//! other boards and display drivers via modifications to the driver_config.h
//! header file.
//!
//! The tool is driven via a command line interface provided on UART0.  Use a
//! terminal emulator on your host system and set the serial port to use
//! 115200bps, 8-N-1.  To see a list of supported commands, enter ``help'' on
//! the command line and to see extended help on a specific command enter
//! ``help [command]''.
//!
//! The commands in the tool fall broadly into three categories:
//!
//! - commands allowing a given low level graphics function to be executed with
//!   parameters provided by the user,
//! - commands providing the ability to read and write arbitrary memory
//!   locations and registers, and
//! - tests displaying test patterns intended to exercise specific display
//!   driver functions.
//!
//! The first group of commands includes ``r'' to read a word from a memory
//! location or register, ``w'' to write a word to a memory location or
//! register, ``dump'' to dump a range of memory as words and ``db'' to dump
//! a range of memory as bytes.  Note that no checking is performed to ensure
//! that addresses passed to these functions are valid.  If an invalid address
//! is passed, the test tool will fault.
//!
//! The second group of commands contains ``fill'' which fills the screen with
//! a given color, ``rect'' which draws a rectangle outline at a given position
//! on the display, ''hline`` which draws a horizontal line, ``vline'' which
//! draws a vertical line, ``image'' which draws an image at provided
//! coordinates, and ``text'' which renders a given text string.  The output of
//! these commands is also modified via several other commands. ``fg'' selects
//! the foreground color to be used by the drawing commands and ``bg'' selects
//! the background color.  ``setimg'' selects from one of four different test
//! images that are drawn in response to the ``image'' command and ``clipimg''
//! allows image clipping to be adjusted to test handling of the \e i32X0
//! parameter to the driver's PixelDrawMultiple function.
//!
//! Additional graphics commands are ``pat'' which redraws the test pattern
//! displayed when the tool starts, ``colbar'' which fills the display with
//! a set of color bars and ``perf'' which draws randomly positioned and
//! colored rectangles for a given number of seconds and determines the drawing
//! speed in pixels-per-second.
//!
//! All driver function test patterns are generated using the ``test'' command
//! whose first parameter indicates the test to display.  Tests are as follow:
//!
//! - ``color'' tests the driver's color handling.  The test starts by
//!   splitting the screen into two and showing a different primary or
//!   secondary color in each half.  Verify that the correct colors are
//!   displayed.  After this, red, blue and green color gradients are
//!   displayed.  Again, verify that these are correct and that no color other
//!   than the shades of the specific primary are displayed.  If any color is
//!   incorrect, this likely indicates an error in the driver's
//!   pfnColorTranslate function or the function used to set the display
//!   palette if the driver provides this feature.
//!
//! - ``pixel'' tests basic pixel plotting.  A test pattern is drawn with a
//!   single white pixel in each corner of the display, a small cross
//!   comprising 5 white pixels in the center, and small arrows near each
//!   corner.  If any of the corner dots are missing or any of the other
//!   pattern elements are incorrect, this points to a problem in the
//!   driver's pfnPixelDraw function or, more generally, a problem with the
//!   display coordinate space handling.
//!
//! - ``hline'' tests horizontal line drawing.  White horizontal lines are
//!   drawn at the top and bottom and a right-angled triangle is constructed in
//!   the center of the display.  If any line is missing or the triangle is
//!   incorrect, this points to a problem in the driver's pfnLineDrawH
//!   function.
//!
//! - ``vline'' tests vertical line drawing.  White vertical lines are drawn
//!   at the left and right and a right-angled triangle is constructed in
//!   the center of the display.  If any line is missing or the triangle is
//!   incorrect, this points to a problem in the driver's pfnLineDrawV
//!   function.
//!
//! - ``mult'' tests the driver's pfnPixelDrawMultiple function.  This is the
//!   most complex driver function and the one most prone to errors.  The tool
//!   fills the display with each of the included test images in turn.  These
//!   cover all the pixel formats (1-, 4- and 8-bpp) that the driver is
//!   required to handle and the image clipping and x positions are set to
//!   ensure that all alignment cases are handled for each format.  In each
//!   case, the image is drawn inside a single pixel red rectangle.  If the
//!   driver is handling each case correctly, the image should look correct
//!   and no part of the red rectangle should be overwritten when the image
//!   is drawn.  In the displayed grid of images, the x alignment increases
//!   from 1 to 8 across the display and each line increases the left-side
//!   image clipping by one pixel from 0 in the top row to 7 in the bottom
//!   row.  An error in any image indicates that one of the cases handled by
//!   the driver's pfnPixelDrawMultiple function is not handled correctly.
//!
//*****************************************************************************

//*****************************************************************************
//
// The number of SysTick interrupts to generate per second.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 10

//*****************************************************************************
//
// Global system tick count.
//
//*****************************************************************************
volatile uint32_t g_ui32SysTickCount;

//*****************************************************************************
//
// System clock rate in Hertz.
//
//*****************************************************************************
volatile uint32_t g_ui32SysClk;

//*****************************************************************************
//
// Default colors.
//
//*****************************************************************************
uint32_t g_ui32Foreground = ClrWhite;
uint32_t g_ui32Background = ClrBlack;

//*****************************************************************************
//
// Definitions related to the color bar pattern.
//
//*****************************************************************************
const uint32_t g_pui32BarColors[] =
{
    ClrBlack,
    ClrWhite,
    ClrYellow,
    ClrCyan,
    ClrGreen,
    ClrMagenta,
    ClrRed,
    ClrBlue
};

#define NUM_COLOR_BARS (sizeof(g_pui32BarColors) / sizeof(uint32_t))

//*****************************************************************************
//
// Names for each of the colors in the g_pui32BarColors array.  These two
// arrays must be kept in sync!
//
//*****************************************************************************
const char *g_ppcBarColors[] =
{
    "Black", "White", "Yellow", "Cyan", "Green", "Magenta", "Red", "Blue"
};

#if (DRIVER_BPP < 16)
//*****************************************************************************
//
// Set aside space for a color palette if the driver being tested uses a
// palettized pixel format.
//
//*****************************************************************************
uint32_t g_pui32Palette[1 << DRIVER_BPP];
#endif

//*****************************************************************************
//
// The number of pixels to clip off the left edge of an image drawn using the
// "image" command.
//
//*****************************************************************************
uint32_t g_ui32Clip = 0;

//*****************************************************************************
//
// Graphics context used to show text on the QVGA display.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE    64

//*****************************************************************************
//
// Simple macros to return the maximum or minimum of two values.
//
//*****************************************************************************
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

//*****************************************************************************
//
// The buffer that holds the command line.
//
//*****************************************************************************
static char g_cCmdBuf[CMD_BUF_SIZE];

typedef struct
{
    char *pcDesc;
    const unsigned char *pucImage;
}
tTestImage;

tTestImage g_pImages[] =
{
   {"TI Logo, 4bpp", g_pucLogo},
   {"32x32 test image, 1bpp", g_pucTest32x32x1Comp},
   {"32x32 test image, 4bpp", g_pucTest32x32x4Comp},
   {"32x32 test image, 8bpp", g_pucTest32x32x8Comp}
};

#define NUM_IMAGES (sizeof(g_pImages) / sizeof (tTestImage))

//*****************************************************************************
//
// The size of the arrows drawn in the PixelDraw test.
//
//*****************************************************************************
#define ARROW_SIZE 10

//*****************************************************************************
//
// A pointer to the currently selected image
//
//*****************************************************************************
const unsigned char *g_pucCurrentImage = g_pucLogo;

//*****************************************************************************
//
// Command handler function prototypes.
//
//*****************************************************************************
int Cmd_Help(int argc, char *argv[]);
int Cmd_Foreground(int argc, char *argv[]);
int Cmd_Background(int argc, char *argv[]);
int Cmd_Fill(int argc, char *argv[]);
int Cmd_Rect(int argc, char *argv[]);
int Cmd_HLine(int argc, char *argv[]);
int Cmd_VLine(int argc, char *argv[]);
int Cmd_SetImage(int argc, char *argv[]);
int Cmd_Image(int argc, char *argv[]);
int Cmd_ClipImage(int argc, char *argv[]);
int Cmd_ColorBars(int argc, char *argv[]);
int Cmd_Text(int argc, char *argv[]);
int Cmd_Test(int argc, char *argv[]);
int Cmd_Pal(int argc, char *argv[]);
int Cmd_Pattern(int argc, char *argv[]);
int Cmd_Perf(int argc, char *argv[]);
int Cmd_Read(int argc, char *argv[]);
int Cmd_Write(int argc, char *argv[]);
int Cmd_Dump(int argc, char *argv[]);
int Cmd_DumpBytes(int argc, char *argv[]);

//*****************************************************************************
//
// This is the table that holds the command names, implementing functions,
// and brief description.
//
//*****************************************************************************
tCmdLineEntry g_psCmdTable[] =
{
    { "fg",     Cmd_Foreground,   "[RGB24] Set the foreground color. Default is white." },
    { "bg",     Cmd_Background,   "[RGB24] Set the background color. Default is black." },
    { "fill",   Cmd_Fill,         "[RGB24] Fill the screen with a color." },
    { "rect",   Cmd_Rect,         "[xTL yTL xBR yBR] Draw a rectangle" },
    { "hline",  Cmd_HLine,        "<x1> <x2> <y> Draw a horizontal line on the display" },
    { "vline",  Cmd_VLine,        "<x> <y1> <y2> Draw a vertical line on the display" },
    { "setimg", Cmd_SetImage,     "<index> Sets the index of the current image." },
    { "image",  Cmd_Image,        "[x y] Draw an image at (x,y) or tile whole screen" },
    { "clipimg",Cmd_ClipImage,    "[clip] Sets number of x image clipping pixels. Default 0." },
    { "colbar", Cmd_ColorBars,    "Draw a color bar pattern on the display" },
    { "text",   Cmd_Text,         "<text> [x y] Write text string at (x,y). Default center" },
    { "test",   Cmd_Test,         "<test name> Run tests for specific driver functions" },
    { "pal",    Cmd_Pal,          "<index> <RGB24> Sets a palette entry to a given color" },
    { "pat",    Cmd_Pattern,      "Draw the initial test pattern." },
    { "perf",   Cmd_Perf,         "<seconds> Run grlib performance test for some period" },
    { "r",      Cmd_Read,         "<addr> Read a memory location or register" },
    { "w",      Cmd_Write,        "<addr> <val> Write a memory location" },
    { "dump",   Cmd_Dump,         "<addr> <wcount> Dump words from a given address" },
    { "d",      Cmd_Dump,         " alias for dump" },
    { "db",     Cmd_DumpBytes,    "<addr> <bcount> Dump bytes from a given address" },
    { "help",   Cmd_Help,         "[command] Display help on a command or a list of commands" },
    { "h",      Cmd_Help,         " alias for help" },
    { "?",      Cmd_Help,         " alias for help" },
    { 0, 0, 0 }
};

//*****************************************************************************
//
// Prototypes for specific test functions.
//
//*****************************************************************************
int Test_ColorTranslate(int argc, char *argv[]);
int Test_PixelDraw(int argc, char *argv[]);
int Test_LineDrawH(int argc, char *argv[]);
int Test_LineDrawV(int argc, char *argv[]);
int Test_PixelDrawMultiple(int argc, char *argv[]);

//*****************************************************************************
//
// A table of test functions that are called based on the first command line
// parameter passed to the "test" command.
//
//*****************************************************************************
tCmdLineEntry g_psTestTable[] =
{
    { "color", Test_ColorTranslate, "" },
    { "pixel", Test_PixelDraw, "" },
    { "hline", Test_LineDrawH, "" },
    { "vline", Test_LineDrawV, "" },
    { "mult", Test_PixelDrawMultiple, "" },
    { 0, 0, 0 }
};

//*****************************************************************************
//
// A structure used to hold command-based help information.
//
//*****************************************************************************
typedef struct
{
        char *pcCommand;
        char *pcHelp;
}
tCommandHelp;

tCommandHelp g_psCommandHelp[] =
{
    { "fg",
      "Sets the foreground color used in future ""rect"", ""hline"", ""vlins"" \n"
      """image"" and ""text"" commands.  The color is provided as a 24-bit RGB\n"
      "value of the form 0xRRGGBB\n" },
    { "bg",
      "Sets the background color used in future ""text"" and ""image"" commands.\n"
      "The color is provided as a 24-bit RGB value of the form 0xRRGGBB\n" },
    { "fill",
      "Fill the entire display with the provided RGB color or, if no parameter is\n"
      "given, the current background color.\n" },
    { "rect",
      "Draw a rectangle in the current foreground color at the given position on\n"
      "the screen.  If no parameters are provided, the rectangle is drawn around the\n"
      "entire display area.  Note that, unlike many other graphics APIs, rectangle\n"
      "coordinates are bottom-right inclusive.\n" },
    { "hline",
      "Draw a single horizontal line on the display using the current foreground\n"
      "color.  The command accepts three parameters, the starting and ending x\n"
      "coordinates and the y coordinate for the line.\n" },
    { "vline",
      "Draw a single vertical line on the display using the current foreground\n"
      "color.  The command accepts three parameters, the starting and ending y\n"
      "coordinates and the x coordinate for the line.\n" },
    { "setimg",
      "Determine which test image will be drawn on future calls to the ""image""\n"
      "command.  The index passed must be between 0 and 3 (inclusive) and identifies\n"
      "the following images:\n"
      "    0 - TI logo, 4bpp, 80 x 75, compressed\n"
      "    1 - 4 square quadrants, 1bpp, 32 x 32, compressed\n"
      "    2 - 16 color test pattern, 4bpp, 32 x 32, compressed\n"
      "    3 - 256 color test pattern, 8bpp, 32 x 32, compressed\n" },
    { "image",
      "Draw the image selected by the previous ""setimg"" command at position (x,y)\n"
      "on the display.  If a 1bpp image is selected, the current foreground and\n"
      "background colors are used, otherwise the image's own palette determines the\n"
      "color.  If the ""clipimg"" command has previously been issued, the left edge\n"
      "of the image will be clipped by the number of pixels indicated in that command.\n"
      "When clipping is enabled, the (x, y) position is not adjusted to compensate for\n"
      "the fact that the image is being cropped.  For example, if a 32x32 image is \n"
      "selected and the clip value has been set to 4, drawing the image at (0, 0) will\n"
      "result in 28 pixels of each image line being drawn at x=4 on the display with\n"
      "the first displayed pixel coming from the 5th column of the source image.\n" },
    { "clipimg",
      "Sets the number of pixels that will be clipped or cropped off the left edge of\n"
      "future images drawn using the ""image"" command.  If no parameter is passed,\n"
      "image clipping is disabled, otherwise the parameter represents the number of\n"
      "pixels to clip.\n" },
    { "colbar",
      "Draw a series of vertical color bars on the display.  From left to right, the\n"
      "bars are black, white, yellow, cyan, green, magenta, red and blue.\n" },
    { "text",
      "Renders text at a given location on the display.  The foreground and background\n"
      "colors are as set using previous ""fg"" and ""bg"" commands.  If no parameters\n"
      "are provided, a short string is drawn in the center of the screen.  If\n"
      "parameters are provided they must be the required string (containing no spaces)\n"
      "followed by the x coordinate and the y coordinate for the top left corner of\n"
      "the string.\n" },
    { "pal",
      "Sets the given color lookup table location to a particular RGB color.  This\n"
      "command is only available if the display frame buffer uses a format containing\n"
      "less than 16 bits per pixel.  The range of valid indices depends upon the frame\n"
      "buffer format.  For a 1bpp display, indices 0 and 1 are valid.  For 4bpp, the\n"
      "index must be in the range 0 to 15, and for an 8bpp buffer, values 0 to 255\n"
      "are valid.\n" },
    { "pat",
      "Clears the display and redraws the initial test pattern that was shown when the\n"
      "tool originally started.\n" },
    { "perf",
      "Draws random filled rectangles on the display for a given number of seconds and\n"
      "then calculates the approximate throughput in megapixels per second and\n"
      "megabytes per second.\n" },
    { "r",
      "Reads a word from a given memory address.  Note that no checking is performed\n"
      "on the validity of the supplied address.  If an invalid address is supplied, the\n"
      "command will cause an exception and the test tool will hang.\n" },
    { "w",
      "Writes a word to a given memory address.  Note that no checking is performed\n"
      "on the validity of the supplied address.  If an invalid address is supplied, the\n"
      "command will cause an exception and the test tool will hang.\n" },
    { "dump",
      "Reads a block of words from a given memory address.  Note that no checking\n"
      "is performed on the validity of the supplied address.  If an invalid address\n"
      "is supplied, the command will cause an exception and the test tool will hang.\n" },
    { "db",
      "Reads a block of bytes from a given memory address and displays them in hex\n"
      "format.  Note that no checking is performed on the validity of the supplied\n"
      "address.  If an invalid address is supplied, the command will cause an\n"
      "exception and the test tool will hang.\n" },
    { "test",
      "Run one of a number of tests designed to exercise a specific display driver\n"
      "function.  Valid tests are as follow:\n"
      "  ""color""  - tests pfnColorTranslate and color handling.\n"
      "  ""pixel""  - tests pfnPixelDraw().\n"
      "  ""hline""  - tests pfnLineDrawH().\n"
      "  ""vline""  - tests pfnLineDrawV().\n"
      "  ""mult""   - tests pfnPixelDrawMultiple().\n" },
    { "help",
      "Show a list of supported commands or provide additional help on a single command.\n" },
    { 0, 0 }
};

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
void
__error__(char *pcFilename, unsigned long ulLine)
{
    //
    // A runtime error was detected so stop here to allow debug.
    //
    while(1)
    {
        //
        // Hang.
        //
    }
}

//*****************************************************************************
//
// The handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Increment the SysTick count.
    //
    g_ui32SysTickCount++;
}

//*****************************************************************************
//
// A wrapper around GrImageDraw() that allows us to supply an x pixel clipping
// value and sets the clip rectangle as required before drawing the image.
//
//*****************************************************************************
void DrawImage(tContext *psContext, const uint32_t ui32Clip,
               const uint8_t *pui8Image, int32_t i32X, int32_t i32Y)
{
    tRectangle sRect;

    sRect.i16XMax = GrContextDpyWidthGet(psContext) - 1;
    sRect.i16YMax = GrContextDpyHeightGet(psContext) - 1;
    sRect.i16YMin = 0;

    //
    // Are we performing clipping?
    //
    if(ui32Clip == 0)
    {
        //
        // No - set the clipping rectangle to the whole screen.
        //
        sRect.i16XMin = 0;
    }
    else
    {
        //
        // Yes - set the clipping rectangle to clip off only the leftmost
        // ui32Clip pixels from the image to be drawn.
        //
        sRect.i16XMin = i32X + ui32Clip;
    }

    //
    // Set the required clipping rectangle.
    //
    GrContextClipRegionSet(psContext, &sRect);

    //
    // Draw the image.
    //
    GrImageDraw(psContext, pui8Image, i32X, i32Y);

    //
    // Clear the clipping rectangle again.
    //
    sRect.i16XMin = 0;
    GrContextClipRegionSet(psContext, &sRect);
}

//*****************************************************************************
//
// Draw a test pattern onto the display.  This pattern contains a variety of
// graphics primitives and will call every display driver API (although not all
// cases that each API must support).
//
//*****************************************************************************
void
DrawTestPattern(tContext *psContext)
{
    int32_t i32Loop, i32XInc, i32YInc, i32Width, i32Height;
    tRectangle sRect;

    //
    // Fill the the display with black and draw a white box around it.
    //
    sRect.i16XMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(psContext) - 1;
    sRect.i16YMin = 0;
    sRect.i16YMax = GrContextDpyHeightGet(psContext) - 1;
    GrContextForegroundSet(psContext, ClrBlack);
    GrRectFill(psContext, &sRect);
    GrContextForegroundSet(psContext, ClrWhite);
    GrRectDraw(psContext, &sRect);

    //
    // Draw a pattern of lines within the main display area.
    //
    i32XInc = GrContextDpyWidthGet(psContext) / 20;
    i32YInc = GrContextDpyHeightGet(psContext) / 20;

    GrContextForegroundSet(psContext, ClrWhite);

    //
    // Draw a pattern of lines.
    //
    for(i32Loop = 0; i32Loop < 20; i32Loop++)
    {
        GrLineDraw(psContext, 0, i32YInc * i32Loop, i32XInc * i32Loop,
               (GrContextDpyHeightGet(psContext) - 1));
        GrLineDraw(psContext,
               (GrContextDpyWidthGet(psContext) - 1),
               (GrContextDpyHeightGet(psContext) -
                (i32YInc * i32Loop + 1)),
               (GrContextDpyWidthGet(psContext) -
                (i32XInc * i32Loop + 1)),
                0);
    }

    //
    // Fill the top 22 lines of the display with blue.
    //
    sRect.i16XMin = 1;
    sRect.i16XMax = GrContextDpyWidthGet(psContext) - 2;
    sRect.i16YMin = 1;
    sRect.i16YMax = 22;

    GrContextForegroundSet(psContext, ClrBlue);
    GrRectFill(psContext, &sRect);

    //
    // Draw a white line at the bottom of the blue box.
    //
    GrContextForegroundSet(psContext, ClrWhite);
    GrLineDraw(psContext, sRect.i16XMin, sRect.i16YMax,
               sRect.i16XMax, sRect.i16YMax);

    //
    // Write the application name inside the blue box at the top of the screen.
    //
    GrContextFontSet(psContext, g_psFontCmss18b);
    GrStringDrawCentered(psContext, "grlib-driver-test", -1, sRect.i16XMax / 2,
                         8, false);

    //
    // Draw a circle around the center of the main display area.
    //
    GrContextForegroundSet(psContext, ClrRed);
    GrCircleDraw(psContext, GrContextDpyWidthGet(psContext) / 2,
                 23 + ((GrContextDpyHeightGet(psContext) - 23) / 2), 70);

    //
    // Determine where to draw the logo so that it is centered in the
    // main section of the display.
    //
    i32Width = *(uint16_t *)(g_pImages[0].pucImage + 1);
    i32Height = *(uint16_t *)(g_pImages[0].pucImage + 3);
    i32XInc = (GrContextDpyWidthGet(psContext) - i32Width) / 2;
    i32YInc = ((GrContextDpyHeightGet(psContext) - (23 + i32Height)) / 2) + 23;

    //
    // Draw the TI logo in the center of the display
    //
    GrTransparentImageDraw(psContext, g_pImages[0].pucImage, i32XInc, i32YInc,
                           ClrBlack);

    //
    // Flush any cached drawing operations.
    //
    GrFlush(psContext);
}

//*****************************************************************************
//
// Draw test patterns that exercises the display driver's ColorTranslate
// function and, if applicable, its palette manipulation function.
//
//*****************************************************************************
int
Test_ColorTranslate(int argc, char *argv[])
{
    int32_t i32Loop, i32NumBars, i32BarWidth, i32BarLoop;
    uint32_t ui32Color;
    tRectangle sRect;

    UARTprintf("Running ColorTranslate Test.\n\n");

    UARTprintf("This test shows various color patterns to allow a driver writer "
               "to ensure that\n"
               "color mapping and palette functions are operating correctly.\n");

    UARTprintf("\nFirst, some basic primary and secondary colors...\n");

    //
    // Loop through pairs of colors that we have configured for the color bar
    // test pattern.
    //
    for(i32Loop = 0; i32Loop < NUM_COLOR_BARS; i32Loop += 2)
    {
        //
        // Does this driver require a color lookup table?
        //
#if (DRIVER_BPP < 16)
        //
        // Set the palette we want to use for this portion of the test.
        //
        g_pui32Palette[0] = g_pui32BarColors[i32Loop];
        g_pui32Palette[1] = g_pui32BarColors[i32Loop + 1];
        DRIVER_PALETTE_SET(g_pui32Palette, 0, 2);
#endif

        //
        // Fill the left half of the display with one color of the pair.
        //
        sRect.i16XMin = 0;
        sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) / 2;
        sRect.i16YMin = 0;
        sRect.i16YMax = GrContextDpyHeightGet(&g_sContext) - 1;
        GrContextForegroundSet(&g_sContext, g_pui32BarColors[i32Loop]);
        GrRectFill(&g_sContext, &sRect);

        //
        // Fill the right half of the display with the other color.
        //
        sRect.i16XMin = (GrContextDpyWidthGet(&g_sContext) / 2) + 1;
        sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
        GrContextForegroundSet(&g_sContext, g_pui32BarColors[i32Loop + 1]);
        GrRectFill(&g_sContext, &sRect);

        //
        // Tell the user what they should be seeing.
        //
        UARTprintf("Left %s, right %s. Press ""Enter"" to continue.\n", g_ppcBarColors[i32Loop],
                   g_ppcBarColors[i32Loop + 1]);

        //
        // Wait for the user to press "Enter"
        //
        UARTgets(g_cCmdBuf, sizeof(g_cCmdBuf));
    }

    //
    // Clear the screen again.
    //
    sRect.i16XMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMin = 0;
    sRect.i16YMax = GrContextDpyHeightGet(&g_sContext) - 1;
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);

    //
    // From the display width and the frame buffer color depth, determine the
    // number of bars to draw and the width of each.
    //
#if (DRIVER_BPP < 16)
    //
    // For a palettized frame buffer, the number of bars we draw is the
    // the number of simultaneous colors that the pixel format can represent.
    //
    i32NumBars = (1 << DRIVER_BPP) - 1;
#else
    //
    // For a direct color frame buffer, we show no more than 256 steps because
    // that's as many independent shades of any primary that you can represent
    // in RGB24 format.
    //
    i32NumBars = 256;
#endif

    //
    // If the number of bars we need to draw is more than the number of pixels
    // across the display, reduce the number of bars.
    //
    i32NumBars = MIN(i32NumBars, GrContextDpyWidthGet(&g_sContext));

    //
    // What's the width of each bar?  We store this in 24.8 fixed point
    // notation.
    //
    i32BarWidth = (GrContextDpyWidthGet(&g_sContext) * 256) / i32NumBars;

    //
    // Now draw a color gradient across the display in each primary color.
    //
    sRect.i16YMin = 0;
    sRect.i16YMax = GrContextDpyHeightGet(&g_sContext) - 1;

    for(i32Loop = 0; i32Loop < 3; i32Loop++)
    {
#if (DRIVER_BPP < 16)
        //
        // For palettized frame buffers, set the desired color palette.
        //
        for(i32BarLoop = 0; i32BarLoop < (1 << DRIVER_BPP); i32BarLoop++)
        {
            g_pui32Palette[i32BarLoop] = (i32BarLoop * 256) / (1 << DRIVER_BPP);
            g_pui32Palette[i32BarLoop] <<= (8 * i32Loop);
        }
        DRIVER_PALETTE_SET(g_pui32Palette, 0, (1 << DRIVER_BPP));
#endif

        //
        // Draw each of the vertical bars making up the gradient pattern.
        //
        for(i32BarLoop = 0; i32BarLoop < i32NumBars; i32BarLoop++)
        {
            //
            // Calculate the position of this bar.
            //
            sRect.i16XMin = sRect.i16XMax + 1;
            sRect.i16XMax = ((i32BarLoop * i32BarWidth) / 256) - 1;

            //
            // Set the color of this bar.
            //
            ui32Color = (i32BarLoop == (i32NumBars - 1)) ? 255 :
                         ((i32BarLoop * 256) / i32NumBars);
            ui32Color <<= (i32Loop * 8);
            GrContextForegroundSet(&g_sContext, ui32Color);

            //
            // Draw the bar.
            //
            GrRectFill(&g_sContext, &sRect);
        }

        //
        // Tell the user what they should be seeing.
        //
        UARTprintf("%s gradient with %d steps. Press ""Enter"" to continue\n",
                   (i32Loop == 0) ? "Blue" :
                    ((i32Loop == 1) ? "Green" : "Red"),
                   i32NumBars);

        //
        // Wait for the user to press "Enter"
        //
        UARTgets(g_cCmdBuf, sizeof(g_cCmdBuf));
    }

    return(0);
}

//*****************************************************************************
//
// Draw a test pattern that exercises the display driver's PixelDraw function.
//
//*****************************************************************************
int
Test_PixelDraw(int argc, char *argv[])
{
    tRectangle sRect;
    int32_t i32Loop;

    UARTprintf("Running PixelDraw Test.\n\n");

    UARTprintf("The display should show single white dots in each corner, a "
               "small cross at\n"
               "the center of the display, and small arrows pointing to each "
               "corner.\n");

    //
    // Set the drawing color to black and clear the screen.
    //
    sRect.i16XMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMin = 0;
    sRect.i16YMax = GrContextDpyHeightGet(&g_sContext) - 1;
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);

    //
    // Set the drawing color to white.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);

    //
    // Plot points at each corner.
    //
    GrPixelDraw(&g_sContext, 0, 0);
    GrPixelDraw(&g_sContext, sRect.i16XMax, 0);
    GrPixelDraw(&g_sContext, 0, sRect.i16YMax);
    GrPixelDraw(&g_sContext, sRect.i16XMax, sRect.i16YMax);

    //
    // Draw a small cross at the center of the display.
    //
    GrPixelDraw(&g_sContext, sRect.i16XMax / 2, sRect.i16YMax / 2);
    GrPixelDraw(&g_sContext, (sRect.i16XMax / 2) + 1, (sRect.i16YMax / 2));
    GrPixelDraw(&g_sContext, (sRect.i16XMax / 2) - 1, (sRect.i16YMax / 2));
    GrPixelDraw(&g_sContext, (sRect.i16XMax / 2), (sRect.i16YMax / 2) + 1);
    GrPixelDraw(&g_sContext, (sRect.i16XMax / 2), (sRect.i16YMax / 2) - 1);

    //
    // Draw the arrows one pixel at a time.
    //
    for(i32Loop = 1; i32Loop < ARROW_SIZE; i32Loop++)
    {
        //
        // Top left arrow.
        //
        GrPixelDraw(&g_sContext, ARROW_SIZE + i32Loop, ARROW_SIZE + i32Loop);
        GrPixelDraw(&g_sContext, ARROW_SIZE, ARROW_SIZE + i32Loop);
        GrPixelDraw(&g_sContext, ARROW_SIZE + i32Loop, ARROW_SIZE);

        //
        // Top right arrow.
        //
        GrPixelDraw(&g_sContext, sRect.i16XMax - (ARROW_SIZE + i32Loop),
                    ARROW_SIZE + i32Loop);
        GrPixelDraw(&g_sContext, sRect.i16XMax - ARROW_SIZE,
                    ARROW_SIZE + i32Loop);
        GrPixelDraw(&g_sContext, sRect.i16XMax - (ARROW_SIZE + i32Loop),
                    ARROW_SIZE);

        //
        // Bottom right arrow.
        //
        GrPixelDraw(&g_sContext, sRect.i16XMax - (ARROW_SIZE + i32Loop),
                        sRect.i16YMax - (ARROW_SIZE + i32Loop));
        GrPixelDraw(&g_sContext, sRect.i16XMax - ARROW_SIZE,
                        sRect.i16YMax - (ARROW_SIZE + i32Loop));
        GrPixelDraw(&g_sContext, sRect.i16XMax - (ARROW_SIZE + i32Loop),
                    sRect.i16YMax - ARROW_SIZE);

        //
        // Bottom left arrow.
        //
        GrPixelDraw(&g_sContext, ARROW_SIZE + i32Loop,
                        sRect.i16YMax - (ARROW_SIZE + i32Loop));
        GrPixelDraw(&g_sContext, ARROW_SIZE,
                        sRect.i16YMax - (ARROW_SIZE + i32Loop));
        GrPixelDraw(&g_sContext, ARROW_SIZE + i32Loop,
                    sRect.i16YMax - ARROW_SIZE);
    }

    return(0);
}

//*****************************************************************************
//
// Draw a test pattern that exercises the display driver's LineDrawH function.
//
//*****************************************************************************
int
Test_LineDrawH(int argc, char *argv[])
{
    tRectangle sRect;
    int32_t i32Loop, i32X, i32Y, i32Size;

    UARTprintf("Running LineDrawH Test.\n\n");

    UARTprintf("The display should show horizontal white lines across the "
               "complete width of\n"
               "the screen at top and bottom and a right-angled triangle with "
               "horizontal\n"
               "edge at the bottom and vertical edge on the left.\n");

    //
    // Set the drawing color to black and clear the screen.
    //
    sRect.i16XMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMin = 0;
    sRect.i16YMax = GrContextDpyHeightGet(&g_sContext) - 1;
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);

    //
    // Set the drawing color to white.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);

    //
    // Draw horizontal lines at the top and bottom of the display.
    //
    GrLineDrawH(&g_sContext, 0, sRect.i16XMax, 0);
    GrLineDrawH(&g_sContext, 0, sRect.i16XMax, sRect.i16YMax);

    //
    // Determine the size of the triangle we will draw.
    //
    i32Size = (MIN(sRect.i16YMax, sRect.i16XMax) * 3) / 4;
    i32X = (GrContextDpyWidthGet(&g_sContext) - i32Size) / 2;
    i32Y = (GrContextDpyHeightGet(&g_sContext) - i32Size) / 2;

    //
    // Draw the rectangle based on the size and position calculated.
    //
    for(i32Loop = 1; i32Loop < i32Size; i32Loop++)
    {
        GrLineDrawH(&g_sContext, i32X, i32X + i32Loop, i32Y + i32Loop);
    }

    return(0);
}

//*****************************************************************************
//
// Draw a test pattern that exercises the display driver's LineDrawV function.
//
//*****************************************************************************
int
Test_LineDrawV(int argc, char *argv[])
{
    tRectangle sRect;
    int32_t i32Loop, i32X, i32Y, i32Size;

    UARTprintf("Running LineDrawV Test.\n\n");

    UARTprintf("The display should show vertical white lines down the "
               "complete height of\n"
               "the screen at left and right and a right-angled triangle with "
               "horizontal\n"
               "edge at the top and vertical edge on the right.\n");

    //
    // Set the drawing color to black and clear the screen.
    //
    sRect.i16XMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMin = 0;
    sRect.i16YMax = GrContextDpyHeightGet(&g_sContext) - 1;
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);

    //
    // Set the drawing color to white.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);

    //
    // Draw vertical lines on the left and right sides of the display.
    //
    GrLineDrawV(&g_sContext, 0, 0, sRect.i16YMax);
    GrLineDrawV(&g_sContext, sRect.i16XMax, 0, sRect.i16YMax);

    //
    // Determine the size of the triangle we will draw.
    //
    i32Size = (MIN(sRect.i16YMax, sRect.i16XMax) * 3) / 4;
    i32X = (GrContextDpyWidthGet(&g_sContext) - i32Size) / 2;
    i32Y = (GrContextDpyHeightGet(&g_sContext) - i32Size) / 2;

    //
    // Draw the rectangle based on the size and position calculated.
    //
    for(i32Loop = 1; i32Loop < i32Size; i32Loop++)
    {
        GrLineDrawV(&g_sContext, i32X + i32Loop, i32Y,
                    i32Y + i32Loop);
    }

    return(0);
}

//*****************************************************************************
//
// Draw test patterns that exercise the display driver's PixelDrawMultiple
// function.  This function ensures that all supported image pixel formats
// are used along with all possible drawing alignment and sub-byte clipping
// settings.
//
//*****************************************************************************
int
Test_PixelDrawMultiple(int argc, char *argv[])
{
    int32_t i32X, i32Y, i32Width, i32Height;
    int32_t i32ImageLoop, i32PositionLoop, i32ClipLoop;
    tRectangle sRect;

    UARTprintf("Running PixelDrawMultiple Test.\n\n");

    //
    // Loop through each of the test images.
    //
    for(i32ImageLoop = 0; i32ImageLoop < NUM_IMAGES; i32ImageLoop++)
    {
        //
        // Tell the user what we're doing.
        //
        UARTprintf("Image %d: %s\n", i32ImageLoop,
                    g_pImages[i32ImageLoop].pcDesc);

        //
        // Set the drawing color to black and clear the screen.
        //
        sRect.i16XMin = 0;
        sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
        sRect.i16YMin = 0;
        sRect.i16YMax = GrContextDpyHeightGet(&g_sContext) - 1;
        GrContextForegroundSet(&g_sContext, ClrBlack);
        GrRectFill(&g_sContext, &sRect);

        //
        // Determine the size of the current test image.
        //
        i32Width = (int32_t)g_pImages[i32ImageLoop].pucImage[1] +
                    256 * (int32_t)g_pImages[i32ImageLoop].pucImage[2];
        i32Height = (int32_t)g_pImages[i32ImageLoop].pucImage[3] +
                    256 * (int32_t)g_pImages[i32ImageLoop].pucImage[4];
        //
        // Loop through each possible x offset from 0 to 7.
        //
        for(i32PositionLoop = 0; i32PositionLoop < 8; i32PositionLoop++)
        {
            //
            // Loop through all clipping values from 0 to 7
            //
            for(i32ClipLoop = 0; i32ClipLoop < 8; i32ClipLoop++)
            {
                //
                // Determine the (x, y) position for this image.
                //
                i32X = i32PositionLoop * (i32Width + 4) + i32PositionLoop + 1;
                i32Y = i32ClipLoop * (i32Height + 4) + 1;

                //
                // Draw the outline rectangle that the image should fit inside.
                //
                sRect.i16XMin = i32X + i32ClipLoop - 1;
                sRect.i16XMax = i32X + i32Width;
                sRect.i16YMin = i32Y - 1;
                sRect.i16YMax = i32Y + i32Height;
                GrContextForegroundSet(&g_sContext, ClrRed);
                GrRectDraw(&g_sContext, &sRect);

                //
                // Set drawing colors to blue and yellow in case we're using
                // a 1bpp source image.
                //
                GrContextForegroundSet(&g_sContext, ClrYellow);
                GrContextBackgroundSet(&g_sContext, ClrBlue);

                //
                // Draw the test image at the relevant position.
                //
                DrawImage(&g_sContext, i32ClipLoop,
                          g_pImages[i32ImageLoop].pucImage, i32X, i32Y);
            }
        }

        //
        // Tell the user what they should be seeing.
        //
        UARTprintf("Check pattern. No image should overdraw the red rectangle "
                   "edges.\nPress ""Enter"" to continue.");

        //
        // Wait for the user to press "Enter"
        //
        UARTgets(g_cCmdBuf, sizeof(g_cCmdBuf));
    }

    //
    // Reset the foreground and background colors to the user's original
    // choices.
    //
    GrContextForegroundSet(&g_sContext, g_ui32Foreground);
    GrContextBackgroundSet(&g_sContext, g_ui32Background);

    UARTprintf("PixelDrawMultiple test complete.\n");

    return(0);
}

//*****************************************************************************
//
// This function implements the "test" command.  It requires a single parameter
// which identifies the test to run.  Any additional command line parameters
// are passed on to the specific test function.
//
//*****************************************************************************
int
Cmd_Test(int argc, char *argv[])
{
    tCmdLineEntry *pTests;

    //
    // Make sure we have at least one additional parameter.
    //
    if(argc < 2)
    {
        UARTprintf("ERROR: This command requires one parameter <test name>.\n");
        return(0);
    }

    //
    // Walk through the list of configured tests and look for a match with
    // the identifier passed.
    //
    pTests = g_psTestTable;

    while(pTests->pcCmd)
    {
        //
        // Does this test match the string passed?
        //
        if(ustrcmp(pTests->pcCmd, argv[1]) == 0)
        {
            //
            // Yes - call the test function.
            //
            return((pTests->pfnCmd)(argc - 1, &argv[1]));
        }

        //
        // Move on to the next test in the list.
        //
        pTests++;
    }

    //
    // If we get here, the test name provided is not recognized.
    //
    UARTprintf("Test ""%s"" cannot be found.\n", argv[1]);
    return(0);
}

//*****************************************************************************
//
// This function implements the "help" command.  It prints a simple list
// of the available commands with a brief description.
//
//*****************************************************************************
int
Cmd_Help(int argc, char *argv[])
{
    tCmdLineEntry *psEntry;
    tCommandHelp *psHelp;
    bool bFound;

    //
    // Get pointers to the beginning of the command table and the extended
    // help table.
    //
    psEntry = g_psCmdTable;
    psHelp = g_psCommandHelp;
    bFound = false;

    //
    // Are we being asked to list all the commands?
    //
    if(argc == 1)
    {
        //
        // Print some header text.
        //
        UARTprintf("\nAvailable commands\n");
        UARTprintf("------------------\n");

        //
        // Enter a loop to read each entry from the command table.  The
        // end of the table has been reached when the command name is NULL.
        //
        while(psEntry->pcCmd)
        {
            //
            // Print the command name and the brief description.
            //
            UARTprintf("%12s : %s\n", psEntry->pcCmd, psEntry->pcHelp);

            //
            // Advance to the next entry in the table.
            //
            psEntry++;
        }
    }
    else
    {
        //
        // We are being asked for help on a specific command.
        //

        //
        // Enter a loop find the requested command in the basic command table
        // and print the summary help for that command.
        //
        while(psEntry->pcCmd)
        {
            //
            // Does the provided command match the current table entry?
            //
            if(ustrcmp(argv[1], psEntry->pcCmd) == 0)
            {
                //
                // Yes - print the command name and the brief description.
                //
                UARTprintf("\n%s : %s\n", psEntry->pcCmd, psEntry->pcHelp);

                bFound = true;

                //
                // Drop out of the loop.
                //
                break;
            }

            //
            // Advance to the next entry in the table.
            //
            psEntry++;
        }

        //
        // If we get here without finding the command, then it's not a valid
        // command.
        //
        if(!bFound)
        {
            UARTprintf("Command ""%s"" is not supported.\n", argv[1]);
            return(0);
        }

        //
        // We found the command so now go and look for extended help.
        //
        bFound = false;

        //
        // Walk through the table of detailed help entries until we see a NULL
        // for the command string pointer.
        //
        while(psHelp->pcCommand)
        {
            //
            // Does the command passed match this entry in the detailed help
            // table?
            //
            if(ustrcmp(argv[1], psHelp->pcCommand) == 0)
            {
                UARTprintf("\n%s\n", psHelp->pcHelp);
                bFound = true;
                break;
            }

            //
            // Move to the next entry in the list.
            //
            psHelp++;
        }

        if(!bFound)
        {
            UARTprintf("No extended help is available for ""%s"".\n", argv[1]);
        }
    }

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// This function implements the "r" command and reads a single word from a
// given memory location.
//
//*****************************************************************************
int
Cmd_Read(int argc, char *argv[])
{
    uint32_t ulAddr;

    //
    // Parameter check.
    //
    if(argc != 2)
    {
        UARTprintf("ERROR: This command requires one parameter <addr>.\n");
        return(0);
    }

    //
    // Get the address to read.
    //
    ulAddr = ustrtoul(argv[1], 0, 0);

    UARTprintf("0x%08x: 0x%08x\n", ulAddr, HWREG(ulAddr));

    return(0);
}

//*****************************************************************************
//
// This function implements the "w" command and writes a single word to a
// given memory location.
//
//*****************************************************************************
int
Cmd_Write(int argc, char *argv[])
{
    uint32_t ulAddr, ulVal;

    //
    // Parameter check.
    //
    if(argc != 3)
    {
        UARTprintf("ERROR: This command requires 2 parameters <addr> <val>.\n");
        return(0);
    }

    //
    // Get the address and value to write.
    //
    ulAddr = ustrtoul(argv[1], 0, 0);
    ulVal = ustrtoul(argv[2], 0, 0);

    HWREG(ulAddr) = ulVal;
    UARTprintf("0x%08x: 0x%08x\n", ulAddr, HWREG(ulAddr));

    return(0);
}

//*****************************************************************************
//
// This function implements the "d" command and dumps a number of words from a
// given memory location.
//
//*****************************************************************************
int
Cmd_Dump(int argc, char *argv[])
{
    uint32_t ulAddr, ui32Count, ulLoop;

    //
    // Parameter check.
    //
    if(argc != 3)
    {
        UARTprintf("ERROR: This command requires 2 parameters <addr> <wcount>.\n");
        return(0);
    }

    //
    // Get the address and value to write.
    //
    ulAddr  = ustrtoul(argv[1], 0, 0);
    ui32Count = ustrtoul(argv[2], 0, 0);

    //
    // Walk through the memory range making sure that we align the addresses
    // on 16 byte boundaries (to make the output look good).
    //
    for(ulLoop = ulAddr & ~0x0F; ulLoop < ulAddr + (ui32Count * 4); ulLoop += 4)
    {
        //
        // Take a new line and print the address every 16 bytes.
        //
        if(!(ulLoop % 16))
        {
            UARTprintf("\n0x%08x: ", ulLoop);
        }

        //
        // Display the value of a particular word or pad with spaces if we are
        // still below the requested address.
        //
        if(ulLoop >= ulAddr)
        {
            UARTprintf("%08x ", HWREG(ulLoop));
        }
        else
        {
            UARTprintf("         ");
        }
    }

    UARTprintf("\n");

    return(0);
}

//*****************************************************************************
//
// Dump the contents of ui32Count bytes from address ulAddr.
//
//*****************************************************************************
static void
DumpBytes(uint32_t ulAddr, uint32_t ui32Count)
{
    uint32_t ulLoop;

    //
    // Walk through the memory range making sure that we align the addresses
    // on 16 byte boundaries (to make the output look good).
    //
    for(ulLoop = ulAddr & ~0x0F; ulLoop < ulAddr + ui32Count; ulLoop++)
    {
        //
        // Take a new line and print the address every 16 bytes.
        //
        if(!(ulLoop % 16))
        {
            UARTprintf("\n0x%08x: ", ulLoop);
        }

        //
        // Display the value of a particular byte or pad with spaces if we are
        // still below the requested address.
        //
        if(ulLoop >= ulAddr)
        {
            UARTprintf("%02x ", HWREGB(ulLoop));
        }
        else
        {
            UARTprintf("   ");
        }
    }

    UARTprintf("\n");
}

//*****************************************************************************
//
// This function implements the "db" command and dumps a number of bytes from a
// given memory location.
//
//*****************************************************************************
int
Cmd_DumpBytes(int argc, char *argv[])
{
    uint32_t ulAddr, ui32Count;

    //
    // Parameter check.
    //
    if(argc != 3)
    {
        UARTprintf("ERROR: This command requires 2 parameters <addr> <bcount>.\n");
        return(0);
    }

    //
    // Get the address and value to write.
    //
    ulAddr  = ustrtoul(argv[1], 0, 0);
    ui32Count = ustrtoul(argv[2], 0, 0);

    DumpBytes(ulAddr, ui32Count);

    return(0);
}

//*****************************************************************************
//
// This function implements the "fill" command and fills the whole screen with
// a given color value.
//
//*****************************************************************************
int
Cmd_Fill(int argc, char *argv[])
{
    tRectangle sRect;
    uint32_t ui32Color;

    //
    // Get the color value to use if one was provided.  Default to
    // the current background if none was specified.
    //
    if(argc > 1)
    {
        ui32Color  = ustrtoul(argv[1], 0, 0);
    }
    else
    {
        ui32Color = g_ui32Background;
    }

    UARTprintf("Filling display with RGB 0x%06x.\n", ui32Color);

    //
    // Fill the frame buffer with the desired color.
    //
    sRect.i16XMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMin = 0;
    sRect.i16YMax = GrContextDpyHeightGet(&g_sContext) - 1;
    GrContextForegroundSet(&g_sContext, ui32Color);
    GrRectFill(&g_sContext, &sRect);

    return(0);
}

//*****************************************************************************
//
// This function implements the "rect" command and draws a rectangle in the
// current foreground color.  If 4 parameters are supplied, they represent
// the X and Y coordinates of the top left and bottom right points.  If no
// parameters are supplied, the display edge outline is drawn.
//
//*****************************************************************************
int
Cmd_Rect(int argc, char *argv[])
{
    tRectangle sRect;

    if(argc == 1)
    {
        sRect.i16XMin = 0;
        sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
        sRect.i16YMin = 0;
        sRect.i16YMax = GrContextDpyHeightGet(&g_sContext) - 1;
    }
    else
    {
        if(argc != 5)
        {
            UARTprintf("This command reguires either 0 or 4 arguments!\n");
            return(0);
        }
        else
        {
            sRect.i16XMin = ustrtoul(argv[1], 0, 0);
            sRect.i16XMax = ustrtoul(argv[3], 0, 0);
            sRect.i16YMin = ustrtoul(argv[2], 0, 0);
            sRect.i16YMax = ustrtoul(argv[4], 0, 0);;
        }
    }

    //
    // Set the foreground color.
    //
    GrContextForegroundSet(&g_sContext, g_ui32Foreground);

    //
    // Draw the required rectangle.
    //
    GrRectDraw(&g_sContext, &sRect);

    return(0);
}

//*****************************************************************************
//
// This function implements the "line" command and draws a pattern of angled
// lines on the display.  If 4 parameters are provided, this function draws
// a single line between the specified points.
//
//*****************************************************************************
int
Cmd_Line(int argc, char *argv[])
{
    int32_t i32Loop, i32XInc, i32YInc;
    tRectangle sRect;

    //
    // Set the foreground color.
    //
    GrContextForegroundSet(&g_sContext, g_ui32Foreground);

    //
    // Are we drawing the pattern or just a single line?
    //
    if(argc == 1)
    {
        i32XInc = GrContextDpyWidthGet(&g_sContext) / 20;
        i32YInc = GrContextDpyHeightGet(&g_sContext) / 20;

        //
        // Draw a pattern of lines.
        //
        for(i32Loop = 0; i32Loop < 20; i32Loop++)
        {
            GrLineDraw(&g_sContext, 0, i32YInc * i32Loop, i32XInc * i32Loop,
                   (GrContextDpyHeightGet(&g_sContext) - 1));
            GrLineDraw(&g_sContext,
                   (GrContextDpyWidthGet(&g_sContext) - 1),
                   (GrContextDpyHeightGet(&g_sContext) -
                           (i32YInc * i32Loop + 1)),
                   (GrContextDpyWidthGet(&g_sContext) -
                           (i32XInc * i32Loop + 1)),
                    0);
        }
    }
    else
    {
        //
        // We're drawing a single line.  Do we have the correct number of
        // parameters?
        //
        if(argc != 5)
        {
            UARTprintf("This command reguires either 0 or 4 arguments!\n");
            return(0);
        }

        //
        // Get the line start and end points.
        //
        sRect.i16XMin = ustrtoul(argv[1], 0, 0);
        sRect.i16XMax = ustrtoul(argv[3], 0, 0);
        sRect.i16YMin = ustrtoul(argv[2], 0, 0);
        sRect.i16YMax = ustrtoul(argv[4], 0, 0);;

        //
        // Draw the line.
        //
        GrLineDraw(&g_sContext, sRect.i16XMin, sRect.i16YMin,
                   sRect.i16XMax, sRect.i16YMax);
    }

    return(0);
}

//*****************************************************************************
//
// This function implements the "fg" command and saves the given color as the
// foreground color for future drawing operations.
//
//*****************************************************************************
int
Cmd_Foreground(int argc, char *argv[])
{
    //
    // Get the color value to use if one was provided.  Default to white if
    // no color was specified.
    //
    if(argc > 1)
    {
        g_ui32Foreground  = ustrtoul(argv[1], 0, 0);
    }
    else
    {
        g_ui32Foreground = ClrWhite;
    }

    UARTprintf("Foreground color set to 0x%06x.\n", g_ui32Foreground);

    return(0);
}


//*****************************************************************************
//
// This function implements the "bg" command and saves the given color as the
// background color for future drawing operations.
//
//*****************************************************************************
int
Cmd_Background(int argc, char *argv[])
{
    //
    // Get the color value to use if one was provided.  Default to black if
    // no color was specified.
    //
    if(argc > 1)
    {
        g_ui32Background  = ustrtoul(argv[1], 0, 0);
    }
    else
    {
        g_ui32Background = ClrBlack;
    }

    UARTprintf("Background color set to 0x%06x.\n", g_ui32Background);

    return(0);
}

//*****************************************************************************
//
// This function implements the "hlines" command and draws a single horizontal
// line on the display.
//
//*****************************************************************************
int
Cmd_HLine(int argc, char *argv[])
{
    int32_t i32X1, i32X2, i32Y;

    //
    // Ensure that we were passed the correct number of parameters.
    //
    if(argc != 4)
    {
        UARTprintf("This command requires 3 parameters, x1, x2, y.\n");
        return(0);
    }

    //
    // Extract the command line parameters.
    //
    i32X1 = ustrtoul(argv[1], 0, 0);
    i32X2 = ustrtoul(argv[2], 0, 0);
    i32Y = ustrtoul(argv[3], 0, 0);

    UARTprintf("Drawing a horizontal line on the display in color 0x%06x.\n",
                g_ui32Foreground);
    UARTprintf("Line (%d, %d) to (%d, %d).\n", i32X1, i32Y, i32X2, i32Y);

    //
    // Set the desired color.
    //
    GrContextForegroundSet(&g_sContext, g_ui32Foreground);

    //
    // Draw the line.
    //
    GrLineDrawH(&g_sContext, i32X1, i32X2, i32Y);

    return(0);
}

//*****************************************************************************
//
// This function implements the "vline" command and draws a pattern of
// horizontal lines on the display.
//
//*****************************************************************************
int
Cmd_VLine(int argc, char *argv[])
{
    int32_t i32Y1, i32Y2, i32X;

    //
    // Ensure that we were passed the correct number of parameters.
    //
    if(argc != 4)
    {
        UARTprintf("This command requires 3 parameters, x, y1, y2.\n");
        return(0);
    }

    //
    // Extract the command line parameters.
    //
    i32X = ustrtoul(argv[1], 0, 0);
    i32Y1 = ustrtoul(argv[2], 0, 0);
    i32Y2 = ustrtoul(argv[3], 0, 0);

    UARTprintf("Drawing a vertical line on the display in color 0x%06x.\n",
                g_ui32Foreground);
    UARTprintf("Line (%d, %d) to (%d, %d).\n", i32X, i32Y1, i32X, i32Y2);

    //
    // Set the desired color.
    //
    GrContextForegroundSet(&g_sContext, g_ui32Foreground);

    //
    // Draw the line.
    //
    GrLineDrawV(&g_sContext, i32X, i32Y1, i32Y2);

    return(0);
}

//*****************************************************************************
//
// This function implements the "pal" command and sets a single palette entry
// to the given color.
//
//*****************************************************************************
int
Cmd_Pal(int argc, char *argv[])
{
#if (DRIVER_BPP == 16)
    UARTprintf("This command is not supported for 16bpp frame buffers.\n");
#else
    uint32_t ui32Color, ui32Index;

    //
    // Get the palette index and color value to use.
    //
    if(argc > 2)
    {
        ui32Index  = ustrtoul(argv[1], 0, 0);
        ui32Color  = ustrtoul(argv[2], 0, 0);
    }
    else
    {
        UARTprintf("This command requires 2 parameters - index, color.\n");
        return(0);
    }

    if(ui32Index > ((DRIVER_BPP == 8) ? 255 : 15))
    {
        UARTprintf("Invalid palette index! Must be less than %d.\n",
                   ((DRIVER_BPP == 8) ? 256 : 16));
        return(0);
    }

    UARTprintf("Setting palette entry %d to color 0x%06x.\n", ui32Index,
                ui32Color);

    //
    // Set the desired color.
    //
    DRIVER_PALETTE_SET(&ui32Color, ui32Index, 1);
#endif

    return(0);
}

//*****************************************************************************
//
// This function implements the "image" command and draws an image at a given
// (x, y) position on the display.  If no parameters are provided, the image
// is tiled across the whole display.
//
//*****************************************************************************
int
Cmd_Image(int argc, char *argv[])
{
    int32_t i32X, i32Y, i32Width, i32Height;

    //
    // Set foreground and background colors to black and white.
    //
    GrContextBackgroundSet(&g_sContext, g_ui32Foreground);
    GrContextForegroundSet(&g_sContext, g_ui32Background);

    //
    // Get the palette index and color value to use.
    //
    if(argc == 1)
    {
        UARTprintf("Tiling image across the whole display.\n");

        //
        // Get the image dimensions.
        //
        i32Width = (int32_t)g_pucCurrentImage[1] +
                    256 * (int32_t)g_pucCurrentImage[2];
        i32Height = (int32_t)g_pucCurrentImage[3] +
                    256 * (int32_t)g_pucCurrentImage[4];

        //
        // Step through each row of images.
        //
        for(i32Y = 0; i32Y < GrContextDpyHeightGet(&g_sContext);
            i32Y += i32Height)
        {
            //
            // Step across each row of images.
            //
            for(i32X = 0; i32X < GrContextDpyWidthGet(&g_sContext);
                i32X += i32Width)
            {
                //
                // Draw the next tile on the display.
                //
                DrawImage(&g_sContext, g_ui32Clip, g_pucCurrentImage,
                          i32X, i32Y);
            }
        }
    }
    else
    {
        //
        // The command has at least 1 parameter.  Make sure there are 2.
        if(argc != 3)
        {
            UARTprintf("This command requires 2 parameters - x, y.\n");
            return(0);
        }

        //
        // Read the coordinates.
        //
        i32X  = (int32_t)ustrtoul(argv[1], 0, 0);
        i32Y  = (int32_t)ustrtoul(argv[2], 0, 0);

        //
        // Draw the image at this position.
        //
        DrawImage(&g_sContext, g_ui32Clip, g_pucCurrentImage, i32X, i32Y);
    }

    return(0);
}

//*****************************************************************************
//
// This function implements the "setimg" command and sets the image that
// will be used with all future calls to Cmd_Image.
//
//*****************************************************************************
int
Cmd_SetImage(int argc, char *argv[])
{
    uint32_t ui32Index;

    //
    // Get the image index to use.
    //
    if(argc != 2)
    {
        UARTprintf("This command requires 1 parameter, the image index.\n");
        return(0);
    }

    //
    // Get the index.
    //
    ui32Index = (uint32_t)ustrtoul(argv[1], 0, 0);

    //
    // Check that it is valid.
    //
    if(ui32Index >= NUM_IMAGES)
    {
        UARTprintf("Image index must be less than %d!\n", NUM_IMAGES);
    }
    else
    {
        g_pucCurrentImage = g_pImages[ui32Index].pucImage;
        UARTprintf("Current image is %d - %s.\n", ui32Index,
                    g_pImages[ui32Index].pcDesc);
    }

    return(0);
}

//*****************************************************************************
//
// This function implements the "clipimg" command and sets the number of
// pixels to clip off the left of an image when drawing as a result of all
// future calls to Cmd_Image.
//
//*****************************************************************************
int
Cmd_ClipImage(int argc, char *argv[])
{
    //
    // Were we passed any parameters?
    //
    if(argc == 1)
    {
        UARTprintf("Disabling image clipping.\n");
        g_ui32Clip = 0;
    }
    else if(argc == 2)
    {
        g_ui32Clip = (uint32_t)ustrtoul(argv[1], 0, 0);
        UARTprintf("Image clipping set to %d pixels.\n", g_ui32Clip);
    }
    else
    {
        UARTprintf("This function requires either zero or 1 parameter!\n");
    }

    return(0);
}

//*****************************************************************************
//
// This function implements the "circle" command and draws a circle in a given
// color and with a given radius on the display.  If no parameters are
// provided, filled and unfilled circles are tiled across the whole display.
//
//*****************************************************************************
int
Cmd_Circle(int argc, char *argv[])
{
    int32_t i32X, i32Y, i32R;
    bool bFill;

    //
    // Set the foreground and background colors.
    //
    GrContextForegroundSet(&g_sContext, g_ui32Foreground);
    GrContextBackgroundSet(&g_sContext, g_ui32Background);

    //
    // Have we been provided with any parameters?
    //
    if(argc == 1)
    {
        UARTprintf("Tiling circles across the whole display.\n");

        //
        // We fill the first circle.
        //
        bFill = true;

        //
        // Set the foreground color to white.
        //
        GrContextForegroundSet(&g_sContext, ClrWhite);

        //
        // Step through each row of circles.
        //
        for(i32Y = 20; i32Y <= GrContextDpyHeightGet(&g_sContext);
            i32Y += 40)
        {
            //
            // Step across each row of circles.
            //
            for(i32X = 20; i32X <= GrContextDpyWidthGet(&g_sContext);
                i32X += 40)
            {
                //
                // Draw the next circle on the display.
                //
                if(bFill)
                {
                    GrCircleFill(&g_sContext, i32X, i32Y, 20);
                }
                else
                {
                    GrCircleDraw(&g_sContext, i32X, i32Y, 20);
                }


                //
                // Flip the outline/fill marker.
                //
                bFill = !bFill;
            }
        }
    }
    else
    {
        //
        // The command has at least 1 parameter.  Make sure there are 3.
        if(argc != 4)
        {
            UARTprintf("This command requires 3 parameters - x, y, r.\n");
            return(0);
        }

        //
        // Read the coordinates.
        //
        i32X  = (int32_t)ustrtoul(argv[1], 0, 0);
        i32Y  = (int32_t)ustrtoul(argv[2], 0, 0);
        i32R = (int32_t)ustrtoul(argv[3], 0, 0);

        //
        // Fill the circle at this position.
        //
        GrCircleFill(&g_sContext, i32X, i32Y, i32R);
    }

    return(0);
}


//*****************************************************************************
//
// This function implements the "text" command and displays a string of text
// at a given screen position.
//
//*****************************************************************************
int
Cmd_Text(int argc, char *argv[])
{
    uint32_t ui32Color;
    int32_t i32X, i32Y;
    bool bCenter;
    char *pcString;

    //
    // Set the foreground and background colors.
    //
    GrContextForegroundSet(&g_sContext, g_ui32Foreground);
    GrContextBackgroundSet(&g_sContext, g_ui32Background);

    //
    // Set the font to be used.
    //
    GrContextFontSet(&g_sContext, g_psFontCmss28);

    //
    // Set default values for the position and color of the string.
    //
    i32X = GrContextDpyWidthGet(&g_sContext) / 2;
    i32Y = GrContextDpyHeightGet(&g_sContext) / 2;
    ui32Color = ClrBlack;
    bCenter = true;

    //
    // Have we been provided with any parameters?
    //
    if(argc == 1)
    {
        //
        // No parameters were provided so just show the default string in
        // the middle of the display.
        //
        pcString = "Some Arbitrary Text";
    }
    else
    {
        //
        // The command has at least 1 parameter.  This is the string so
        // remember it.
        //
        pcString = argv[1];

        //
        // Has the X parameter been provided?
        //
        if(argc > 2)
        {
            //
            // The X position has been provided.
            //
            i32X  = (int32_t)ustrtoul(argv[2], 0, 0);
            bCenter = false;
        }

        //
        // Has the Y parameter been provided?
        //
        if(argc > 3)
        {
            //
            // The Y position has been provided.
            //
            i32Y  = (int32_t)ustrtoul(argv[3], 0, 0);
            bCenter = false;
        }
    }

    UARTprintf("Displaying %sstring at (%d, %d)\n",
               bCenter ? "centered " : "", i32X, i32Y, ui32Color);

    //
    // Draw the text.Fill the circle at this position.
    //
    if(bCenter)
    {
        GrStringDrawCentered(&g_sContext, pcString, -1, i32X, i32Y, true);
    }
    else
    {
        GrStringDraw(&g_sContext, pcString, -1, i32X, i32Y, true);
    }

    return(0);
}

//*****************************************************************************
//
// This function implements the "colbar" command and fills the screen with a
// color bar pattern.
//
//*****************************************************************************
int
Cmd_ColorBars(int argc, char *argv[])
{
    uint32_t ui32X, ui32Width, ui32Height;
    tRectangle sRect;

    //
    // Determine the size of the color bar.
    //
    ui32Width =  GrContextDpyWidthGet(&g_sContext) / NUM_COLOR_BARS;
    ui32Height =  GrContextDpyHeightGet(&g_sContext);

    //
    // The Y coordinates for each color bar don't change.
    //
    sRect.i16YMin = 0;
    sRect.i16YMax = (ui32Height - 1);

    for(ui32X = 0; ui32X < NUM_COLOR_BARS; ui32X++)
    {
        sRect.i16XMin = ui32X * ui32Width;
        if(ui32X < (NUM_COLOR_BARS - 1))
        {
            //
            // For all but the rightmost bar, set the calculated width.
            //
            sRect.i16XMax = sRect.i16XMin + ui32Width - 1;
        }
        else
        {
            //
            // Ensure the bars reach the right edge. This prevents any
            // rounding error from leaving undrawn pixels on the right
            // depending upon the screen size and number of color bars
            // in use.
            //
            sRect.i16XMax = GrContextDpyWidthGet(&g_sContext);
        }

        GrContextForegroundSet(&g_sContext, g_pui32BarColors[ui32X]);
        GrRectFill(&g_sContext, &sRect);
    }

    //
    // Revert to the expected foreground color.
    //
    GrContextForegroundSet(&g_sContext, g_ui32Foreground);

    return(0);
}

//*****************************************************************************
//
// This function implements the "pat" command and draws the same test pattern
// that is displayed when the application starts.
//
//*****************************************************************************
int
Cmd_Pattern(int argc, char *argv[])
{
    UARTprintf("Drawing initial test pattern.\n");
    DrawTestPattern(&g_sContext);
    return(0);
}

//*****************************************************************************
//
// This function implements the "perf" command and draws randomly positioned,
// filled rectangles for some period of time.
//
//*****************************************************************************
int
Cmd_Perf(int argc, char *argv[])
{
    uint32_t ui32NumSeconds, ui32EndTime, ui32NumPixels;
    tRectangle sRect, sRectScreen, sRectDraw;
    uint32_t ui32Width, ui32Height, ui32Color, ui32X, ui32Y;

    //
    // We need 1 parameter. Is it there?
    //
    if(argc != 2)
    {
        UARTprintf("This command requires one parameter!\n");
        return(0);
    }

    //
    // Get the required timeout.
    //
    ui32NumSeconds = ustrtoul(argv[1], 0, 0);
    ui32NumPixels = 0;

    UARTprintf("Drawing random rectangles for %d seconds...\n", ui32NumSeconds);

    //
    // When must the test end?
    //
    ui32EndTime = g_ui32SysTickCount + (ui32NumSeconds * SYSTICKS_PER_SECOND);

    //
    // Get a rectangle representing the screen.
    //
    sRectScreen.i16XMin = 0;
    sRectScreen.i16YMin = 0;
    sRectScreen.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRectScreen.i16YMax = GrContextDpyHeightGet(&g_sContext) - 1;

    //
    // Loop for the required time.
    //
    while(g_ui32SysTickCount < ui32EndTime)
    {
        //
        // Get some randomized parameters.
        //
        ui32Color = urand() & 0xFFFFFF;
        ui32Width = (urand() & 0xFF) + 64;
        ui32Height = (urand() & 0xFF) + 32;
        ui32X = urand() & 0x1FF;
        ui32Y = urand() & 0x1FF;

        sRect.i16XMin = (short)ui32X;
        sRect.i16YMin = (short)ui32Y;
        sRect.i16XMax = sRect.i16XMin + ui32Width;
        sRect.i16YMax = sRect.i16YMin + ui32Height;

        //
        // Clip the rectangle to the screen.
        //
        if(GrRectIntersectGet(&sRectScreen, &sRect, &sRectDraw))
        {
            //
            // Set the color and fill this rectangle.
            //
            GrContextForegroundSet(&g_sContext, ui32Color);
            GrRectFill(&g_sContext, &sRectDraw);

            //
            // Update our pixel count.
            //
            ui32NumPixels += ((sRectDraw.i16XMax - sRectDraw.i16XMin) + 1) *
                             ((sRectDraw.i16YMax - sRectDraw.i16YMin) + 1);
        }
    }

    UARTprintf("Performance test completed.\n");
    if(ui32NumSeconds)
    {
        //
        // Recycle a couple of variables we're finished with to store the
        // drawing throughput values.
        //
        ui32NumPixels /= ui32NumSeconds;
        ui32Width = ui32NumPixels >> 20;
        ui32Height = ((ui32NumPixels - (ui32Width << 20)) * 10) / (1024 * 1024);

        //
        // Tell the user how fast we managed to draw pixels.
        //
        UARTprintf("Throughput %d.%01dMpps\n", ui32Width, ui32Height);

        //
        // Calculate the number of MB per second.
        //
        ui32NumPixels = (ui32NumPixels * DRIVER_BPP) / 8;
        ui32Width = ui32NumPixels >> 20;
        ui32Height = ((ui32NumPixels - (ui32Width << 20)) * 10) / (1024 * 1024);
        UARTprintf("           %d.%01dMBps\n", ui32Width, ui32Height);
    }

    return(0);
}

//*****************************************************************************
//
// A simple testcase allowing various graphics primitives to be tested.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32Val;
    int nStatus;

    //
    // Set the PLL and system clock to the frequencies needed to allow
    // generation of the required pixel clock.
    //
    g_ui32SysClk = MAP_SysCtlClockFreqSet(DRIVER_SYS_CLOCK_CONFIG,
                                          DRIVER_SYS_CLOCK_FREQUENCY);

    //
    // Enable GPIOA for the UART.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Set GPIO A0 and A1 as UART pins.
    //
    ui32Val = HWREG(GPIO_PORTA_BASE + GPIO_O_PCTL);
    ui32Val &= 0xFFFFFF00;
    ui32Val |= 0x00000011;
    HWREG(GPIO_PORTA_BASE + GPIO_O_PCTL) = ui32Val;

    //
    // Set GPIO A0 and A1 as UART.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART as a console for text I/O.
    //
    UARTStdioConfig(0, 115200, g_ui32SysClk);

    //
    // Set up the system tick to run and interrupt when it times out.
    //
    MAP_SysTickIntEnable();
    MAP_SysTickPeriodSet(g_ui32SysClk / SYSTICKS_PER_SECOND);
    MAP_SysTickEnable();

    //
    // Enable interrupts.
    //
    IntMasterEnable();

    //
    // Print hello message to user.
    //
    UARTprintf("\n\nGrLib Driver Test Tool\n\n");
    UARTprintf("Display configured for %dx%d at %dbpp.\n",
               DRIVER_NAME.ui16Width, DRIVER_NAME.ui16Height, DRIVER_BPP);
    UARTprintf("System clock is %dMHz\n", g_ui32SysClk / 1000000);
    UARTprintf("\nEnter ""help"" for a list of supported commands\n\n");

    //
    // Initialize the display.
    //
    DRIVER_INIT(g_ui32SysClk);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &DRIVER_NAME);

    //
    // We allow the testcase to be built such that it issues no drawing orders
    // other than those requested via the command line.  This can be helpful
    // when bringing up a new display driver.
    //
#ifndef NO_GRLIB_CALLS_ON_STARTUP
    //
    // Draw a test pattern on the display.
    //
    DrawTestPattern(&g_sContext);
#endif

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Print a prompt to the console.  Show the CWD.
        //
        UARTprintf("> ");

        //
        // Get a line of text from the user.
        //
        UARTgets(g_cCmdBuf, sizeof(g_cCmdBuf));

        //
        // Pass the line from the user to the command processor.
        // It will be parsed and valid commands executed.
        //
        nStatus = CmdLineProcess(g_cCmdBuf);

        //
        // Handle the case of bad command.
        //
        if(nStatus == CMDLINE_BAD_CMD)
        {
            UARTprintf("Bad command!\n");
        }

        //
        // Handle the case of too many arguments.
        //
        else if(nStatus == CMDLINE_TOO_MANY_ARGS)
        {
            UARTprintf("Too many arguments for command processor!\n");
        }
    }
}
