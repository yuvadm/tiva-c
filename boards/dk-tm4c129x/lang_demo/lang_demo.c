//*****************************************************************************
//
// lang_demo.c - Demonstration of the TivaWare Graphics Library's string
//               table support.
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

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/container.h"
#include "grlib/pushbutton.h"
#include "grlib/radiobutton.h"
#include "utils/ustdlib.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/touch.h"
#include "drivers/pinout.h"
#include "images.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Graphics Library String Table Demonstration (lang_demo)</h1>
//!
//! This application provides a demonstration of the capabilities of the
//! TivaWare Graphics Library's string table functions.  Two panels show
//! different implementations of features of the string table functions.
//! For each panel, the bottom provides a forward and back button
//! (when appropriate).
//!
//! The first panel provides a large string with introductory text and basic
//! instructions for operation of the application.
//!
//! The second panel shows the available languages and allows them to be
//! switched between English, German, Spanish and Italian.
//!
//! The string table and custom fonts used by this application can be found
//! under /third_party/fonts/lang_demo.  The original strings that the
//! application intends displaying are found in the language.csv file (encoded
//! in UTF8 format to allow accented characters and Asian language ideographs
//! to be included.  The mkstringtable tool is used to generate two versions
//! of the string table, one which remains encoded in UTF8 format and the other
//! which has been remapped to a custom codepage allowing the table to be
//! reduced in size compared to the original UTF8 text.  The tool also produces
//! character map files listing each character used in the string table.  These
//! are then provided as input to the ftrasterize tool which generates two
//! custom fonts for the application, one indexed using Unicode and a smaller
//! one indexed using the custom codepage generated for this string table.
//!
//! The command line parameters required for mkstringtable and ftrasterize
//! can be found in the makefile in third_party/fonts/lang_demo.
//!
//! By default, the application builds to use the custom codepage version of
//! the string table and its matching custom font.  To build using the UTF8
//! string table and Unicode-indexed custom font, ensure that the definition of
//! \b USE_REMAPPED_STRINGS at the top of the lang_demo.c source file is
//! commented out.
//!
//*****************************************************************************

//*****************************************************************************
//
// Comment the following line to use a version of the string table and custom
// font that does not use codepage remapping.  In this version, the font is
// somewhat larger and character lookup will be slower but it has the advantage
// that the strings you retrieve from the string table are encoded exactly
// as they were in the original CSV file and are generally readable in your
// debugger (since they use a standard codepage like ISO8859-1 or UTF8).
//
//*****************************************************************************
#define USE_REMAPPED_STRINGS

#ifdef USE_REMAPPED_STRINGS
#include "langremap.h"
extern const unsigned char g_pui8Customr14pt[];
extern const unsigned char g_pui8Customr20pt[];

#define SPACE_CHAR MAP8000_CHAR_000020
#define FONT_20PT (const tFont *)g_pui8Customr20pt
#define FONT_14PT (const tFont *)g_pui8Customr14pt
#define STRING_TABLE g_pui8Tablelangremap
#define GRLIB_INIT_STRUCT g_sGrLibDefaultlangremap

#else

#include "language.h"

extern const unsigned char g_pui8Custom14pt[];
extern const unsigned char g_pui8Custom20pt[];

#define SPACE_CHAR 0x20
#define FONT_20PT (const tFont *)g_pui8Custom20pt
#define FONT_14PT (const tFont *)g_pui8Custom14pt

#define STRING_TABLE g_pui8Tablelanguage
#define GRLIB_INIT_STRUCT g_sGrLibDefaultlanguage

#endif

//*****************************************************************************
//
// The names for each of the panels, which is displayed at the bottom of the
// screen.
//
//*****************************************************************************
static const uint32_t g_ui32PanelNames[3] =
{
    STR_INTRO,
    STR_CONFIG,
    STR_UPDATE
};

//*****************************************************************************
//
// This string holds the title of the group of languages.  The size is fixed
// by LANGUAGE_MAX_SIZE since the names of the languages in this application
// are not larger than LANGUAGE_MAX_SIZE bytes.
//
//*****************************************************************************
#define LANGUAGE_MAX_SIZE       16
char g_pcLanguage[LANGUAGE_MAX_SIZE];

//*****************************************************************************
//
// This is a generic buffer that is used to retrieve strings from the string
// table.  This forces its size to be at least as big as the largest string
// in the string table which is defined by the SCOMP_MAX_STRLEN value.
//
//*****************************************************************************
char g_pcBuffer[SCOMP_MAX_STRLEN];

//*****************************************************************************
//
// This string holds the title of each "panel" in the application.  The size is
// fixed by TITLE_MAX_SIZE since the names of the panels in this application
// are not larger than TITLE_MAX_SIZE bytes.
//
//*****************************************************************************
#define TITLE_MAX_SIZE          20
char g_pcTitle[TITLE_MAX_SIZE];

//*****************************************************************************
//
// This table holds the array of languages supported.
//
//*****************************************************************************
typedef struct
{
    uint16_t ui16Language;
    bool bBreakOnSpace;
}
tLanguageParams;

const tLanguageParams g_psLanguageTable[] =
{
    { GrLangEnUS, true },
    { GrLangDe, true },
    { GrLangEsSP, true },
    { GrLangIt, true },
    { GrLangZhPRC, false },
    { GrLangKo, true },
    { GrLangJp, false }
};

#define NUM_LANGUAGES (sizeof(g_LanguageTable) / sizeof(tLanguageParams))

//*****************************************************************************
//
// The index of the current language in the g_psLanguageTable array.
//
//*****************************************************************************
uint32_t g_ui32LangIdx;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Forward declarations for the globals required to define the widgets at
// compile-time.
//
//*****************************************************************************
void OnPrevious(tWidget *psWidget);
void OnNext(tWidget *psWidget);
void OnIntroPaint(tWidget *psWidget, tContext *psContext);
void OnRadioChange(tWidget *psWidget, uint32_t bSelected);
extern tCanvasWidget g_psPanels[];

//*****************************************************************************
//
// The first panel, which contains introductory text explaining the
// application.
//
//*****************************************************************************
Canvas(g_sIntroduction, g_psPanels, 0, 0, &g_sKentec320x240x16_SSD2119, 8, 26,
       300, 154, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnIntroPaint);

//*****************************************************************************
//
// Storage for language name strings.  Note that we could hardcode these into
// the relevant widget initialization macros but since we may be using a
// custom font and remapped codepage, keeping the strings in the string table
// and loading them when the app starts is likely to create less confusion and
// prevents the risk of seeing garbled output if you accidentally use ASCII or
// ISO8859-1 text strings with the custom font.
//
//*****************************************************************************
#define MAX_LANGUAGE_NAME_LEN 10
char g_pcEnglish[MAX_LANGUAGE_NAME_LEN];
char g_pcDeutsch[MAX_LANGUAGE_NAME_LEN];
char g_pcEspanol[MAX_LANGUAGE_NAME_LEN];
char g_pcItaliano[MAX_LANGUAGE_NAME_LEN];
char g_pcChinese[MAX_LANGUAGE_NAME_LEN];
char g_pcKorean[MAX_LANGUAGE_NAME_LEN];
char g_pcJapanese[MAX_LANGUAGE_NAME_LEN];

//*****************************************************************************
//
// The language selection panel, which contains a selection of radio buttons
// for each language.
//
//*****************************************************************************
tContainerWidget g_psRadioContainers[];

tRadioButtonWidget g_psRadioButtons1[] =
{
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 1, 0,
                      &g_sKentec320x240x16_SSD2119, 18, 54, 120, 25,
                      RB_STYLE_TEXT | RB_STYLE_SELECTED, 16, 0, ClrSilver,
                      ClrSilver, FONT_20PT, g_pcEnglish, 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 2, 0,
                      &g_sKentec320x240x16_SSD2119, 18, 82, 120, 25,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, FONT_20PT,
                      g_pcDeutsch, 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 3, 0,
                      &g_sKentec320x240x16_SSD2119, 180, 54, 120, 25,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, FONT_20PT,
                      g_pcEspanol, 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 4, 0,
                      &g_sKentec320x240x16_SSD2119, 180, 82, 120, 25,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, FONT_20PT,
                      g_pcItaliano, 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 5, 0,
                      &g_sKentec320x240x16_SSD2119, 18, 110, 120, 25,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, FONT_20PT,
                      g_pcChinese, 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 6, 0,
                      &g_sKentec320x240x16_SSD2119, 180, 110, 120, 25,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, FONT_20PT,
                      g_pcKorean, 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, 0, 0,
                      &g_sKentec320x240x16_SSD2119, 18, 138, 120, 25,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, FONT_20PT,
                      g_pcJapanese, 0, OnRadioChange),
};

#define NUM_RADIO1_BUTTONS      (sizeof(g_psRadioButtons1) /   \
                                 sizeof(g_psRadioButtons1[0]))

tContainerWidget g_psRadioContainers[] =
{
    ContainerStruct(g_psPanels + 1, 0, g_psRadioButtons1,
                    &g_sKentec320x240x16_SSD2119, 8, 30, 300, 140,
                    CTR_STYLE_OUTLINE | CTR_STYLE_TEXT, 0, ClrGray, ClrSilver,
                    FONT_20PT, g_pcLanguage),
};

//*****************************************************************************
//
// An array of canvas widgets, one per panel.  Each canvas is filled with
// black, overwriting the contents of the previous panel.
//
//*****************************************************************************
tCanvasWidget g_psPanels[] =
{
    CanvasStruct(0, 0, &g_sIntroduction, &g_sKentec320x240x16_SSD2119, 8, 22,
                 300, 158, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, g_psRadioContainers, &g_sKentec320x240x16_SSD2119, 8,
                 22, 300, 158, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
};

//*****************************************************************************
//
// The number of panels.
//
//*****************************************************************************
#define NUM_PANELS              (sizeof(g_psPanels) / sizeof(g_psPanels[0]))

//*****************************************************************************
//
// The buttons and text across the bottom of the screen.
//
//*****************************************************************************
char g_pcPlus[2];
char g_pcMinus[2];

RectangularButton(g_sPrevious, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 8, 180,
                  50, 50, PB_STYLE_FILL, ClrBlack, ClrBlack, 0, ClrSilver,
                  FONT_20PT, g_pcMinus, g_pui8Blue50x50, g_pui8Blue50x50Press,
                  0, 0, OnPrevious);
Canvas(g_sTitle, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 58, 180, 200, 50,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE | CANVAS_STYLE_FILL, 0, 0,
       ClrSilver, FONT_20PT, 0, 0, 0);
RectangularButton(g_sNext, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 260, 180,
                  50, 50, PB_STYLE_IMG | PB_STYLE_TEXT, ClrBlack, ClrBlack, 0,
                  ClrSilver, FONT_20PT, g_pcPlus, g_pui8Blue50x50,
                  g_pui8Blue50x50Press, 0, 0, OnNext);

//*****************************************************************************
//
// The panel that is currently being displayed.
//
//*****************************************************************************
uint32_t g_ui32Panel;

//*****************************************************************************
//
// Handles presses of the previous panel button.
//
//*****************************************************************************
void
OnPrevious(tWidget *psWidget)
{
    //
    // There is nothing to be done if the first panel is already being
    // displayed.
    //
    if(g_ui32Panel == 0)
    {
        return;
    }

    //
    // Remove the current panel.
    //
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Decrement the panel index.
    //
    g_ui32Panel--;

    //
    // Add and draw the new panel.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Set the title of this panel.
    //
    GrStringGet(g_ui32PanelNames[g_ui32Panel], g_pcTitle, TITLE_MAX_SIZE);
    WidgetPaint((tWidget *)&g_sTitle);

    //
    // See if this is the first panel.
    //
    if(g_ui32Panel == 0)
    {
        //
        // Clear the previous button from the display since the first panel is
        // being displayed.
        //
        PushButtonImageOff(&g_sPrevious);
        PushButtonTextOff(&g_sPrevious);
        PushButtonFillOn(&g_sPrevious);
        WidgetPaint((tWidget *)&g_sPrevious);
    }

    //
    // See if the previous panel was the last panel.
    //
    if(g_ui32Panel == (NUM_PANELS - 2))
    {
        //
        // Display the next button.
        //
        PushButtonImageOn(&g_sNext);
        PushButtonTextOn(&g_sNext);
        PushButtonFillOff(&g_sNext);
        WidgetPaint((tWidget *)&g_sNext);
    }
}

//*****************************************************************************
//
// Handles presses of the next panel button.
//
//*****************************************************************************
void
OnNext(tWidget *psWidget)
{
    //
    // There is nothing to be done if the last panel is already being
    // displayed.
    //
    if(g_ui32Panel == (NUM_PANELS - 1))
    {
        return;
    }

    //
    // Remove the current panel.
    //
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Increment the panel index.
    //
    g_ui32Panel++;

    //
    // Add and draw the new panel.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Set the title of this panel.
    //
    GrStringGet(g_ui32PanelNames[g_ui32Panel], g_pcTitle, TITLE_MAX_SIZE);
    WidgetPaint((tWidget *)&g_sTitle);

    //
    // See if the previous panel was the first panel.
    //
    if(g_ui32Panel == 1)
    {
        //
        // Display the previous button.
        //
        PushButtonImageOn(&g_sPrevious);
        PushButtonTextOn(&g_sPrevious);
        PushButtonFillOff(&g_sPrevious);
        WidgetPaint((tWidget *)&g_sPrevious);
    }

    //
    // See if this is the last panel.
    //
    if(g_ui32Panel == (NUM_PANELS - 1))
    {
        //
        // Clear the next button from the display since the last panel is being
        // displayed.
        //
        PushButtonImageOff(&g_sNext);
        PushButtonTextOff(&g_sNext);
        PushButtonFillOn(&g_sNext);
        WidgetPaint((tWidget *)&g_sNext);
    }
}

//*****************************************************************************
//
// Switch out all of the dynamic strings.
//
//*****************************************************************************
void
ChangeLanguage(uint16_t ui16Language)
{
    //
    // Change the language.
    //
    GrStringLanguageSet(ui16Language);

    //
    // Update the Language string.
    //
    GrStringGet(STR_LANGUAGE, g_pcLanguage, LANGUAGE_MAX_SIZE);

    //
    // Update the title string.
    //
    GrStringGet(g_ui32PanelNames[g_ui32Panel], g_pcTitle, TITLE_MAX_SIZE);
}

//*****************************************************************************
//
// Handles wrapping a string within an area.
//
// \param psContext is the context of the area to update.
// \param pcString is the string to print out.
// \param i32LineHeight is the height of a character in the currrent font.
// \param i32X is the x position to start printing this string.
// \param i32Y is the y position to start printing this string.
// \param bSplitOnSpace is true if strings in the current language must be
// split only on space characters or false if they may be split between any
// two characters.
//
// \return Returns the number of lines that were printed due to this string.
//
//*****************************************************************************
uint32_t
DrawStringWrapped(tContext *psContext, char *pcString,
                  int32_t i32LineHeight, int32_t i32X, int32_t i32Y,
                  bool bSplitOnSpace)
{
    uint32_t ui32Width, ui32CharWidth, ui32StrWidth, ui32Char;
    uint32_t ui32Lines, ui32Skip;
    char *pcStart, *pcEnd;
    char *pcLastSpace;

    ui32Lines = 0;

    //
    // Get the number of pixels we have to fit the string into across the
    // screen.
    //
    ui32Width = GrContextDpyWidthGet(psContext) - 16 - i32X;

    //
    // Get a pointer to the terminating zero.
    //
    pcEnd = pcString;
    while(*pcEnd)
    {
        pcEnd++;
    }

    //
    // The first substring we draw will start at the beginning of the string.
    //
    pcStart = pcString;
    pcLastSpace = pcString;
    ui32StrWidth = 0;

    //
    // Keep processing until we have no more characters to display.
    //
    do
    {
        //
        // Get the next character in the string.
        //
        ui32Char = GrStringNextCharGet(psContext, pcString,
                                       (pcEnd - pcString), &ui32Skip);

        //
        // Did we reach the end of the string?
        //
        if(ui32Char)
        {
            //
            // No - how wide is this character?
            //
            ui32CharWidth = GrStringWidthGet(psContext, pcString, ui32Skip);

            //
            // Have we run off the edge of the display?
            //
            if((ui32StrWidth + ui32CharWidth) > ui32Width)
            {
                //
                // If we are splitting on spaces, rewind the string pointer to
                // the byte after the last space.
                //
                if(bSplitOnSpace)
                {
                    pcString = pcLastSpace;
                }

                //
                // Yes - draw the substring.
                //
                GrStringDraw(psContext, pcStart, (pcString - pcStart),
                             i32X, i32Y, 0);

                //
                // Increment the line count and move the y position down by the
                // current font's line height.
                //
                ui32Lines++;
                i32Y += i32LineHeight;
                ui32StrWidth = 0;

                //
                // The next string we draw will start at the current position.
                //
                pcStart = pcString;
            }
            else
            {
                //
                // No - update the width and move on to the next character.
                //
                ui32StrWidth += ui32CharWidth;
                pcString += ui32Skip;

                //
                // If this is a space, remember where we are.
                //
                if(ui32Char == SPACE_CHAR)
                {
                    pcLastSpace = pcString;
                }
            }
        }
        else
        {
            //
            // Do we have any remaining chunk of string to draw?
            //
            if(pcStart != pcString)
            {
                //
                // Yes - draw the last section of string.
                //
                GrStringDraw(psContext, pcStart, -1, i32X, i32Y, 0);
                ui32Lines++;
            }
        }
    } while(ui32Char);

    return(ui32Lines);
}

//*****************************************************************************
//
// Handles paint requests for the introduction canvas widget.
//
//*****************************************************************************
void
OnIntroPaint(tWidget *psWidget, tContext *psContext)
{
    int32_t i32LineHeight, i32Offset;
    uint32_t ui32Lines;

    i32LineHeight = GrFontHeightGet(FONT_14PT);
    i32Offset = 28;

    //
    // Display the introduction text in the canvas.
    //
    GrContextFontSet(psContext, FONT_14PT);
    GrContextForegroundSet(psContext, ClrSilver);

    //
    // Write the first paragraph of the introduction page.
    //
    GrStringGet(STR_INTRO_1, g_pcBuffer, SCOMP_MAX_STRLEN);
    ui32Lines = DrawStringWrapped(psContext, g_pcBuffer, i32LineHeight, 8,
                              i32Offset,
                              g_psLanguageTable[g_ui32LangIdx].bBreakOnSpace);
    //
    // Move down by 1/4 of a line between paragraphs.
    //
    i32Offset += i32LineHeight/4;

    //
    // Write the second paragraph of the introduction page.
    //
    GrStringGet(STR_INTRO_2, g_pcBuffer, SCOMP_MAX_STRLEN);
    ui32Lines += DrawStringWrapped(psContext, g_pcBuffer, i32LineHeight, 8,
                              i32Offset + (ui32Lines * i32LineHeight),
                              g_psLanguageTable[g_ui32LangIdx].bBreakOnSpace);
    //
    // Move down by 1/4 of a line between paragraphs.
    //
    i32Offset += i32LineHeight/4;

    //
    // Write the third paragraph of the introduction page.
    //
    GrStringGet(STR_INTRO_3, g_pcBuffer, SCOMP_MAX_STRLEN);
    DrawStringWrapped(psContext, g_pcBuffer, i32LineHeight, 8, i32Offset +
        (ui32Lines * i32LineHeight),
        g_psLanguageTable[g_ui32LangIdx].bBreakOnSpace );
}

//*****************************************************************************
//
// Handles change notifications for the radio button widgets.
//
//*****************************************************************************
void
OnRadioChange(tWidget *psWidget, uint32_t bSelected)
{
    //
    // Find the index of this radio button in the first group.
    //
    for(g_ui32LangIdx = 0; g_ui32LangIdx < NUM_RADIO1_BUTTONS; g_ui32LangIdx++)
    {
        if(psWidget == (tWidget *)(g_psRadioButtons1 + g_ui32LangIdx))
        {
            break;
        }
    }

    //
    // Change any dynamic language strings.
    //
    ChangeLanguage(g_psLanguageTable[g_ui32LangIdx].ui16Language);

    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);
}

//*****************************************************************************
//
// A simple demonstration of the features of the TivaWare Graphics Library.
//
//*****************************************************************************
int
main(void)
{
    tContext sContext;
    uint32_t ui32SysClock;

    //
    // Run from the PLL at 120 MHz.
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(ui32SysClock);

    //
    // Set graphics library text rendering defaults.
    //
    GrLibInit(&GRLIB_INIT_STRUCT);

    //
    // Set the string table and the default language.
    //
    GrStringTableSet(STRING_TABLE);

    //
    // Set the default language.
    //
    ChangeLanguage(GrLangEnUS);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&sContext, "lang-demo");

    //
    // Load the static strings from the string table.  These strings are
    // independent of the language in use but we store them in the string
    // table nonetheless since (a) we may be using codepage remapping in
    // which case it would be difficult to hardcode them into the app source
    // anyway (ASCII or ISO8859-1 text would not render properly with the
    // remapped custom font) and (b) even if we're not using codepage remapping,
    // we may have generated a custom font from the string table output and
    // we want to make sure that all glyphs required by the application are
    // present in that font.  If we hardcode some text in the application
    // source and don't put it in the string table, we run the risk of having
    // characters missing in the font.
    //
    GrStringGet(STR_ENGLISH, g_pcEnglish, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_DEUTSCH, g_pcDeutsch, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_ESPANOL, g_pcEspanol, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_ITALIANO, g_pcItaliano, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_CHINESE, g_pcChinese, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_KOREAN, g_pcKorean, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_JAPANESE, g_pcJapanese, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_PLUS, g_pcPlus, 2);
    GrStringGet(STR_MINUS, g_pcMinus, 2);

    //
    // Initialize the touch screen driver and have it route its messages to the
    // widget tree.
    //
    TouchScreenInit(ui32SysClock);
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the title block and the previous and next buttons to the widget
    // tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sPrevious);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sTitle);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sNext);

    //
    // Add the first panel to the widget tree.
    //
    g_ui32Panel = 0;
    WidgetAdd(WIDGET_ROOT, (tWidget *)g_psPanels);

    //
    // Set the string for the title.
    //
    CanvasTextSet(&g_sTitle, g_pcTitle);

    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Loop forever, processing widget messages.
    //
    while(1)
    {
        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();
    }

}
