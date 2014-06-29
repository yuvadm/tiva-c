Graphics Driver Test Tool

This application provides a simple, command-line tool to aid in
debugging TivaWare Graphics Library display drivers.  As shipped in
TivaWare, it is configured to operate with a DK-TM4C129X board using its
QVGA display.  The code is written, however, to allow easy retargeting to
other boards and display drivers via modifications to the driver_config.h
header file.

The tool is driven via a command line interface provided on UART0.  Use a
terminal emulator on your host system and set the serial port to use
115200bps, 8-N-1.  To see a list of supported commands, enter ``help'' on
the command line and to see extended help on a specific command enter
``help [command]''.

The commands in the tool fall broadly into three categories:

- commands allowing a given low level graphics function to be executed with
  parameters provided by the user,
- commands providing the ability to read and write arbitrary memory
  locations and registers, and
- tests displaying test patterns intended to exercise specific display
  driver functions.

The first group of commands includes ``r'' to read a word from a memory
location or register, ``w'' to write a word to a memory location or
register, ``dump'' to dump a range of memory as words and ``db'' to dump
a range of memory as bytes.  Note that no checking is performed to ensure
that addresses passed to these functions are valid.  If an invalid address
is passed, the test tool will fault.

The second group of commands contains ``fill'' which fills the screen with
a given color, ``rect'' which draws a rectangle outline at a given position
on the display, ''hline`` which draws a horizontal line, ``vline'' which
draws a vertical line, ``image'' which draws an image at provided
coordinates, and ``text'' which renders a given text string.  The output of
these commands is also modified via several other commands. ``fg'' selects
the foreground color to be used by the drawing commands and ``bg'' selects
the background color.  ``setimg'' selects from one of four different test
images that are drawn in response to the ``image'' command and ``clipimg''
allows image clipping to be adjusted to test handling of the \e i32X0
parameter to the driver's PixelDrawMultiple function.

Additional graphics commands are ``pat'' which redraws the test pattern
displayed when the tool starts, ``colbar'' which fills the display with
a set of color bars and ``perf'' which draws randomly positioned and
colored rectangles for a given number of seconds and determines the drawing
speed in pixels-per-second.

All driver function test patterns are generated using the ``test'' command
whose first parameter indicates the test to display.  Tests are as follow:

- ``color'' tests the driver's color handling.  The test starts by
  splitting the screen into two and showing a different primary or
  secondary color in each half.  Verify that the correct colors are
  displayed.  After this, red, blue and green color gradients are
  displayed.  Again, verify that these are correct and that no color other
  than the shades of the specific primary are displayed.  If any color is
  incorrect, this likely indicates an error in the driver's
  pfnColorTranslate function or the function used to set the display
  palette if the driver provides this feature.

- ``pixel'' tests basic pixel plotting.  A test pattern is drawn with a
  single white pixel in each corner of the display, a small cross
  comprising 5 white pixels in the center, and small arrows near each
  corner.  If any of the corner dots are missing or any of the other
  pattern elements are incorrect, this points to a problem in the
  driver's pfnPixelDraw function or, more generally, a problem with the
  display coordinate space handling.

- ``hline'' tests horizontal line drawing.  White horizontal lines are
  drawn at the top and bottom and a right-angled triangle is constructed in
  the center of the display.  If any line is missing or the triangle is
  incorrect, this points to a problem in the driver's pfnLineDrawH
  function.

- ``vline'' tests vertical line drawing.  White vertical lines are drawn
  at the left and right and a right-angled triangle is constructed in
  the center of the display.  If any line is missing or the triangle is
  incorrect, this points to a problem in the driver's pfnLineDrawV
  function.

- ``mult'' tests the driver's pfnPixelDrawMultiple function.  This is the
  most complex driver function and the one most prone to errors.  The tool
  fills the display with each of the included test images in turn.  These
  cover all the pixel formats (1-, 4- and 8-bpp) that the driver is
  required to handle and the image clipping and x positions are set to
  ensure that all alignment cases are handled for each format.  In each
  case, the image is drawn inside a single pixel red rectangle.  If the
  driver is handling each case correctly, the image should look correct
  and no part of the red rectangle should be overwritten when the image
  is drawn.  In the displayed grid of images, the x alignment increases
  from 1 to 8 across the display and each line increases the left-side
  image clipping by one pixel from 0 in the top row to 7 in the bottom
  row.  An error in any image indicates that one of the cases handled by
  the driver's pfnPixelDrawMultiple function is not handled correctly.


-------------------------------------------------------------------------------

Copyright (c) 2012-2014 Texas Instruments Incorporated.  All rights reserved.
Software License Agreement

Texas Instruments (TI) is supplying this software for use solely and
exclusively on TI's microcontroller products. The software is owned by
TI and/or its suppliers, and is protected under applicable copyright
laws. You may not combine this software with "viral" open-source
software in order to form a larger program.

THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
DAMAGES, FOR ANY REASON WHATSOEVER.

This is part of revision 2.1.0.12573 of the DK-TM4C129X Firmware Package.
