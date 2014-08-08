Custom Font for lang_demo
-------------------------

This directory contains files illustrating how to generate an
application-specific font containing only the minimum set of
characters required to display the contents of a given string
table in multiple languages. The fonts and string tables
built here are used by the lang_demo example found in the
TivaWare releases for several boards.

Input files to this process are:

1. language.csv - a comma-separated-value file containing UTF8
   strings in each of the 7 supported languages.  This file is
   application-specific.

2. Font files containing the characters required to render the
   strings in langauge.csv.  In this example, we require a total
   of 3 fonts, one for basic western characters, one for
   simplified Chinese ideographs and another for Korean and
   Japanese.

Run "make" to generate the following files from language.csv:

1. language.c and langremap.c - Two versions of the output
   string table, one encoded using the original UTF8 text and
   the other with the codepage remapped to minimize the size of
   the strings.
2. language.h and langremap.h - Header files containing string
   IDs for the appropriate string tables.
3. fontcustom16pt.c - A custom font encoding only the characters
   found in the language.csv string table.  This version creates
   a font indexed using the Unicode codepage.  This should be used
   with the language.c and language.h versions of the string
   table.
4. fontcustomr16pt.c - Another cusom font encoding only the
   characters found in the language.csv string table.  This font
   is indexed using an optimized custom codepage and should be
   used with the langremap.c and langremap.h versions of the
   string table output.


2. font
