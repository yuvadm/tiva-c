Graphics Library String Table Demonstration

This application provides a demonstration of the capabilities of the
TivaWare Graphics Library's string table functions.  Two panels show
different implementations of features of the string table functions.
For each panel, the bottom provides a forward and back button
(when appropriate).

The first panel provides a large string with introductory text and basic
instructions for operation of the application.

The second panel shows the available languages and allows them to be
switched between English, German, Spanish and Italian.

The string table and custom fonts used by this application can be found
under /third_party/fonts/lang_demo.  The original strings that the
application intends displaying are found in the language.csv file (encoded
in UTF8 format to allow accented characters and Asian language ideographs
to be included.  The mkstringtable tool is used to generate two versions
of the string table, one which remains encoded in UTF8 format and the other
which has been remapped to a custom codepage allowing the table to be
reduced in size compared to the original UTF8 text.  The tool also produces
character map files listing each character used in the string table.  These
are then provided as input to the ftrasterize tool which generates two
custom fonts for the application, one indexed using Unicode and a smaller
one indexed using the custom codepage generated for this string table.

The command line parameters required for mkstringtable and ftrasterize
can be found in the makefile in third_party/fonts/lang_demo.

By default, the application builds to use the custom codepage version of
the string table and its matching custom font.  To build using the UTF8
string table and Unicode-indexed custom font, ensure that the definition of
\b USE_REMAPPED_STRINGS at the top of the lang_demo.c source file is
commented out.


-------------------------------------------------------------------------------

Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
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
