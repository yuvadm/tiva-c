//*****************************************************************************
//
// fatwrapper.c - A simple wrapper allowing access to binary fonts stored in
//                the FAT file system.
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
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "third_party/fatfs/src/ff.h"
#include "third_party/fatfs/src/diskio.h"

//*****************************************************************************
//
// The number of font block headers that we cache when a font is opened.
//
//*****************************************************************************
#define MAX_FONT_BLOCKS 16

//*****************************************************************************
//
// The amount of memory set aside to hold compressed data for a single glyph.
// Fonts for use with the graphics library limit compressed glyphs to 256 bytes.
// If you are sure your fonts contain small glyphs, you could reduce this to
// save space.
//
//*****************************************************************************
#define MAX_GLYPH_SIZE 256

//*****************************************************************************
//
// Instance data for a single loaded font.
//
//*****************************************************************************
typedef struct
{
    //
    // The FATfs file object associated with the font.
    //
    FIL sFile;

    //
    // The font header as read from the file.
    //
    tFontWide sFontHeader;

    //
    // Storage for the font block table.
    //
    tFontBlock psBlocks[MAX_FONT_BLOCKS];

    //
    // A marker indicating whether or not the structure is in use.
    //
    bool bInUse;

    //
    // The codepoint of the character whose glyph data is currently stored in
    // pui8GlyphStore.
    //
    uint32_t ui32CurrentGlyph;

    //
    // Storage for the compressed data of the latest glyph.  In a more complex
    // implementation, you would likely want to cache this data to reduce
    // slow disk interaction.
    //
    uint8_t pui8GlyphStore[MAX_GLYPH_SIZE];
}
tFontFile;

//*****************************************************************************
//
// Workspace for FATfs.
//
//*****************************************************************************
FATFS g_sFatFs;

//*****************************************************************************
//
// Instance data for a single loaded font.  This implementation only supports
// a single font open at any one time.  If you wanted to implement something
// more general, a memory manager could be used to allocate these structures
// dynamically in FATWrapperFontLoad().
//
//*****************************************************************************
tFontFile g_sFontFile;

//*****************************************************************************
//
// A structure that holds a mapping between an FRESULT numerical code,
// and a string represenation.  FRESULT codes are returned from the FatFs
// FAT file system driver.
//
//*****************************************************************************
typedef struct
{
    FRESULT iFResult;
    char *pcResultStr;
}
tFresultString;

//*****************************************************************************
//
// A macro to make it easy to add result codes to the table.
//
//*****************************************************************************
#define FRESULT_ENTRY(f)        { (f), (#f) }

//*****************************************************************************
//
// A table that holds a mapping between the numerical FRESULT code and
// it's name as a string.  This is used for looking up error codes for
// printing to the console.
//
//*****************************************************************************
tFresultString g_psFresultStrings[] =
{
    FRESULT_ENTRY(FR_OK),
    FRESULT_ENTRY(FR_DISK_ERR),
    FRESULT_ENTRY(FR_INT_ERR),
    FRESULT_ENTRY(FR_NOT_READY),
    FRESULT_ENTRY(FR_NO_FILE),
    FRESULT_ENTRY(FR_NO_PATH),
    FRESULT_ENTRY(FR_INVALID_NAME),
    FRESULT_ENTRY(FR_DENIED),
    FRESULT_ENTRY(FR_EXIST),
    FRESULT_ENTRY(FR_INVALID_OBJECT),
    FRESULT_ENTRY(FR_WRITE_PROTECTED),
    FRESULT_ENTRY(FR_INVALID_DRIVE),
    FRESULT_ENTRY(FR_NOT_ENABLED),
    FRESULT_ENTRY(FR_NO_FILESYSTEM),
    FRESULT_ENTRY(FR_MKFS_ABORTED),
    FRESULT_ENTRY(FR_TIMEOUT),
    FRESULT_ENTRY(FR_LOCKED),
    FRESULT_ENTRY(FR_NOT_ENOUGH_CORE),
    FRESULT_ENTRY(FR_TOO_MANY_OPEN_FILES),
    FRESULT_ENTRY(FR_INVALID_PARAMETER),
};

//*****************************************************************************
//
// Error reasons returned by ChangeDirectory().
//
//*****************************************************************************
#define NAME_TOO_LONG_ERROR 1
#define OPENDIR_ERROR       2

//*****************************************************************************
//
// A macro that holds the number of result codes.
//
//*****************************************************************************
#define NUM_FRESULT_CODES (sizeof(g_psFresultStrings) / sizeof(tFresultString))

//*****************************************************************************
//
// Internal function prototypes.
//
//*****************************************************************************
static const char *StringFromFresult(FRESULT iFResult);
static void FATWrapperFontInfoGet(uint8_t *pui8FontID,
                                  uint8_t *pui8Format,
                                  uint8_t *pui8Width,
                                  uint8_t *pui8Height,
                                  uint8_t *pui8Baseline);
static const uint8_t *FATWrapperFontGlyphDataGet(
                                  uint8_t *pui8FontID,
                                  uint32_t ui32CodePoint,
                                  uint8_t *pui8Width);
static uint16_t FATWrapperFontCodepageGet(uint8_t *pui8FontID);
static uint16_t FATWrapperFontNumBlocksGet(uint8_t *pui8FontID);
static uint32_t FATWrapperFontBlockCodepointsGet(
                                  uint8_t *pui8FontID,
                                  uint16_t ui16BlockIndex,
                                  uint32_t *pui32Start);

//*****************************************************************************
//
// Access function pointers required to complete the tFontWrapper structure
// for this font.
//
//*****************************************************************************
const tFontAccessFuncs g_sFATFontAccessFuncs =
{
    FATWrapperFontInfoGet,
    FATWrapperFontGlyphDataGet,
    FATWrapperFontCodepageGet,
    FATWrapperFontNumBlocksGet,
    FATWrapperFontBlockCodepointsGet
};

//*****************************************************************************
//
// This function returns a string representation of an error code
// that was returned from a function call to FatFs.  It can be used
// for printing human readable error messages.
//
//*****************************************************************************
static const char *
StringFromFresult(FRESULT iFResult)
{
    uint32_t ui32Idx;

    //
    // Enter a loop to search the error code table for a matching
    // error code.
    //
    for(ui32Idx = 0; ui32Idx < NUM_FRESULT_CODES; ui32Idx++)
    {
        //
        // If a match is found, then return the string name of the
        // error code.
        //
        if(g_psFresultStrings[ui32Idx].iFResult == iFResult)
        {
            return(g_psFresultStrings[ui32Idx].pcResultStr);
        }
    }

    //
    // At this point no matching code was found, so return a
    // string indicating unknown error.
    //
    return("UNKNOWN ERROR CODE");
}

//*****************************************************************************
//
// Returns information about a font previously loaded using FATFontWrapperLoad.
//
//*****************************************************************************
static void
FATWrapperFontInfoGet(uint8_t *pui8FontID, uint8_t *pui8Format,
                      uint8_t *pui8Width, uint8_t *pui8Height,
                      uint8_t *pui8Baseline)
{
    tFontFile *psFont;

    //
    // Parameter sanity check.
    //
    ASSERT(pui8FontID);
    ASSERT(pui8Format);
    ASSERT(pui8Width);
    ASSERT(pui8Height);
    ASSERT(pui8Baseline);

    //
    // Get a pointer to our instance data.
    //
    psFont = (tFontFile *)pui8FontID;

    ASSERT(psFont->bInUse);

    //
    // Return the requested information.
    //
    *pui8Format = psFont->sFontHeader.ui8Format;
    *pui8Width =  psFont->sFontHeader.ui8MaxWidth;
    *pui8Height =  psFont->sFontHeader.ui8Height;
    *pui8Baseline =  psFont->sFontHeader.ui8Baseline;
}

//*****************************************************************************
//
// Returns the codepage used by the font whose handle is passed.
//
//*****************************************************************************
static uint16_t
FATWrapperFontCodepageGet(uint8_t *pui8FontID)
{
    tFontFile *psFont;

    //
    // Parameter sanity check.
    //
    ASSERT(pui8FontID);

    //
    // Get a pointer to our instance data.
    //
    psFont = (tFontFile *)pui8FontID;

    ASSERT(psFont->bInUse);

    //
    // Return the codepage identifier from the font.
    //
    return(psFont->sFontHeader.ui16Codepage);
}

//*****************************************************************************
//
// Returns the number of glyph blocks supported by a particular font.
//
//*****************************************************************************
static uint16_t
FATWrapperFontNumBlocksGet(uint8_t *pui8FontID)
{
    tFontFile *psFont;

    //
    // Parameter sanity check.
    //
    ASSERT(pui8FontID);

    //
    // Get a pointer to our instance data.
    //
    psFont = (tFontFile *)pui8FontID;

    ASSERT(psFont->bInUse);

    //
    // Return the number of glyph blocks contained in the font.
    //
    return(psFont->sFontHeader.ui16NumBlocks);
}

//*****************************************************************************
//
// Read a given font block header from the provided file.
//
//*****************************************************************************
static bool
FATWrapperFontBlockHeaderGet(FIL *psFile, tFontBlock *psBlock,
                             uint32_t ui32Index)
{
    FRESULT iFResult;
    UINT uiRead;

    //
    // Set the file pointer to the position of the block header we want.
    //
    iFResult = f_lseek(psFile, sizeof(tFontWide) +
                      (sizeof(tFontBlock) * ui32Index));
    if(iFResult == FR_OK)
    {
        //
        // Now read the block header.
        //
        iFResult = f_read(psFile, psBlock, sizeof(tFontBlock), &uiRead);
        if((iFResult == FR_OK) && (uiRead == sizeof(tFontBlock)))
        {
            return(true);
        }
    }

    //
    // If we get here, we experienced an error so return a bad return code.
    //
    return(false);
}

//*****************************************************************************
//
// Returns information on the glyphs contained within a given font block.
//
//*****************************************************************************
static uint32_t
FATWrapperFontBlockCodepointsGet(uint8_t *pui8FontID,
                                 uint16_t ui16BlockIndex,
                                 uint32_t *pui32Start)
{
    tFontBlock sBlock;
    tFontFile *psFont;
    bool bRetcode;

    //
    // Parameter sanity check.
    //
    ASSERT(pui8FontID);

    //
    // Get a pointer to our instance data.
    //
    psFont = (tFontFile *)pui8FontID;

    ASSERT(psFont->bInUse);

    //
    // Have we been passed a valid block index?
    //
    if(ui16BlockIndex >= psFont->sFontHeader.ui16NumBlocks)
    {
        //
        // No - return an error.
        //
        return(0);
    }

    //
    // Is this block header cached?
    //
    if(ui16BlockIndex < MAX_FONT_BLOCKS)
    {
        //
        // Yes - return the information from our cached copy.
        //
        *pui32Start = psFont->psBlocks[ui16BlockIndex].ui32StartCodepoint;
        return(psFont->psBlocks[ui16BlockIndex].ui32NumCodepoints);
    }
    else
    {
        //
        // We don't have the block header cached so read it from the
        // SDCard.  First move the file pointer to the expected position.
        //
        bRetcode = FATWrapperFontBlockHeaderGet(&psFont->sFile, &sBlock,
                                                ui16BlockIndex);
        if(bRetcode)
        {
            *pui32Start = sBlock.ui32StartCodepoint;
            return(sBlock.ui32NumCodepoints);
        }
        else
        {
            UARTprintf("Error reading block header!\n");
        }
    }

    //
    // If we get here, something horrible happened so return a failure.
    //
    *pui32Start = 0;
    return(0);
}

//*****************************************************************************
//
// Retrieves the data for a particular font glyph.  This function returns
// a pointer to the glyph data in linear, random access memory if the glyph
// exists or NULL if not.
//
//*****************************************************************************
static const uint8_t *
FATWrapperFontGlyphDataGet(uint8_t *pui8FontID,
                           uint32_t ui32Codepoint,
                           uint8_t *pui8Width)
{
    tFontFile *psFont;
    uint32_t ui32Loop, ui32GlyphOffset, ui32TableOffset;
    UINT uiRead;
    tFontBlock sBlock;
    tFontBlock *psBlock;
    bool bRetcode;
    FRESULT iFResult;

    //
    // Parameter sanity check.
    //
    ASSERT(pui8FontID);
    ASSERT(pui8Width);

    //
    // If passed a NULL codepoint, return immediately.
    //
    if(!ui32Codepoint)
    {
        return(0);
    }

    //
    // Get a pointer to our instance data.
    //
    psFont = (tFontFile *)pui8FontID;

    ASSERT(psFont->bInUse);

    //
    // Look for the trivial case - do we have this glyph in our glyph store
    // already?
    //
    if(psFont->ui32CurrentGlyph == ui32Codepoint)
    {
        //
        // We struck gold - we already have this glyph in our buffer.  Return
        // the width (from the second byte of the data) and a pointer to the
        // glyph data.
        //
        *pui8Width = psFont->pui8GlyphStore[1];
        return(psFont->pui8GlyphStore);
    }

    //
    // First find the block that contains the glyph we've been asked for.
    //
    for(ui32Loop = 0; ui32Loop < psFont->sFontHeader.ui16NumBlocks; ui32Loop++)
    {
        if(ui32Loop < MAX_FONT_BLOCKS)
        {
            psBlock = &psFont->psBlocks[ui32Loop];
        }
        else
        {
            bRetcode = FATWrapperFontBlockHeaderGet(&psFont->sFile, &sBlock,
                                                    ui32Loop);
            psBlock = &sBlock;
            if(!bRetcode)
            {
                //
                // We failed to read the block header so return an error.
                //
                return(0);
            }
        }

        //
        // Does the requested character exist in this block?
        //
        if((ui32Codepoint >= (psBlock->ui32StartCodepoint)) &&
          ((ui32Codepoint < (psBlock->ui32StartCodepoint +
            psBlock->ui32NumCodepoints))))
        {
            //
            // The glyph is in this block. Calculate the offset of it's
            // glyph table entry in the file.
            //
            ui32TableOffset = psBlock->ui32GlyphTableOffset +
                            ((ui32Codepoint - psBlock->ui32StartCodepoint) *
                            sizeof(uint32_t));

            //
            // Move the file pointer to the offset position.
            //
            iFResult = f_lseek(&psFont->sFile, ui32TableOffset);

            if(iFResult != FR_OK)
            {
                return(0);
            }

            //
            // Read the glyph data offset.
            //
            iFResult = f_read(&psFont->sFile, &ui32GlyphOffset,
                             sizeof(uint32_t), &uiRead);

            //
            // Return if there was an error or if the offset is 0 (which
            // indicates that the character is not included in the font.
            //
            if((iFResult != FR_OK) || (uiRead != sizeof(uint32_t)) ||
                !ui32GlyphOffset)
            {
                return(0);
            }

            //
            // Move the file pointer to the start of the glyph data remembering
            // that the glyph table offset is relative to the start of the
            // block not the start of the file (so we add the table offset
            // here).
            //
            iFResult = f_lseek(&psFont->sFile, (psBlock->ui32GlyphTableOffset +
                              ui32GlyphOffset));

            if(iFResult != FR_OK)
            {
                return(0);
            }

            //
            // Read the first byte of the glyph data to find out how long it
            // is.
            //
            iFResult = f_read(&psFont->sFile, psFont->pui8GlyphStore, 1,
                              &uiRead);

            if((iFResult != FR_OK) || !uiRead)
            {
                return(0);
            }

            //
            // Now read the glyph data.
            //
            iFResult = f_read(&psFont->sFile, psFont->pui8GlyphStore + 1,
                              psFont->pui8GlyphStore[0] - 1, &uiRead);

            if((iFResult != FR_OK) ||
               (uiRead != (psFont->pui8GlyphStore[0] - 1)))
            {
                return(0);
            }

            //
            // If we get here, things are good. Return a pointer to the glyph
            // store data.
            //
            psFont->ui32CurrentGlyph = ui32Codepoint;
            *pui8Width = psFont->pui8GlyphStore[1];
            return(psFont->pui8GlyphStore);
        }
    }

    //
    // If we get here, the codepoint doesn't exist in the font so return an
    // error.
    //
    return(0);
}

//*****************************************************************************
//
// Prepares the FAT file system font wrapper for use.
//
// This function must be called before any attempt to use a font stored on the
// FAT file system.  It initializes FATfs for use.
//
// \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
bool
FATFontWrapperInit(void)
{
    FRESULT iFResult;

    //
    // Mount the file system, using logical disk 0.
    //
    iFResult = f_mount(0, &g_sFatFs);
    if(iFResult != FR_OK)
    {
        //
        // We failed to mount the volume.  Tell the user and return an error.
        //
        UARTprintf("f_mount error: %s (%d)\n", StringFromFresult(iFResult),
                iFResult);
        return(false);
    }

    //
    // All is well so tell the caller.
    //
    return(true);
}

//*****************************************************************************
//
// Provides the FATfs timer tick.
//
// This function must be called every 10mS or so by the application.  It
// provides the time reference required by the FAT file system.
//
// \return None.
//
//*****************************************************************************
void
FATWrapperSysTickHandler(void)
{
    //
    // Call the FatFs tick timer.
    //
    disk_timerproc();
}

//*****************************************************************************
//
// Prepares a font in the FATfs file system for use by the graphics library.
//
// This function must be called to prepare a font for use by the graphics
// library.  It opens the font file whose name is passed and reads the
// header information.  The value returned should be written into the
// pui8FontID field of the tFontWrapper structure that will be passed to
// graphics library.
//
// This is a very simple (and slow) implementation.  More complex wrappers
// may also initialize a glyph cache here.
//
// \return On success, returns a non-zero pointer identifying the font.  On
// error, zero is returned.
//
//*****************************************************************************
uint8_t *
FATFontWrapperLoad(char *pcFilename)
{
    FRESULT iFResult;
    UINT uiRead, uiToRead;

    UARTprintf("Attempting to load font %s from FAT file system.\n",
               pcFilename);

    //
    // Make sure a font is not already open.
    //
    if(g_sFontFile.bInUse)
    {
        //
        // Oops - someone tried to load a new font without unloading the
        // previous one.
        //
        UARTprintf("Another font is already loaded!\n");
        return(0);
    }

    //
    // Try to open the file whose name we've been given.
    //
    iFResult = f_open(&g_sFontFile.sFile, pcFilename, FA_READ);
    if(iFResult != FR_OK)
    {
        //
        // We can't open the file.  Either the file doesn't exist or there is
        // no SDCard installed.  Regardless, return an error.
        //
        UARTprintf("Error %s (%d) from f_open.\n", StringFromFresult(iFResult),
                   iFResult);
        return(0);
    }

    //
    // We opened the file successfully.  Does it seem to contain a valid
    // font?  Read the header and see.
    //
    iFResult = f_read(&g_sFontFile.sFile, &g_sFontFile.sFontHeader,
                      sizeof(tFontWide), &uiRead);
    if((iFResult == FR_OK) && (uiRead == sizeof(tFontWide)))
    {
        //
        // We read the font header.  Is the format correct? We only support
        // wide fonts via wrappers.
        //
        if((g_sFontFile.sFontHeader.ui8Format != FONT_FMT_WIDE_UNCOMPRESSED) &&
           (g_sFontFile.sFontHeader.ui8Format != FONT_FMT_WIDE_PIXEL_RLE))
        {
            //
            // This is not a supported font format.
            //
            UARTprintf("Unrecognized font format. Failing "
                       "FATFontWrapperLoad.\n");
            f_close(&g_sFontFile.sFile);
            return(0);
        }

        //
        // The format seems to be correct so read as many block headers as we
        // have storage for.
        //
        uiToRead = ((g_sFontFile.sFontHeader.ui16NumBlocks > MAX_FONT_BLOCKS) ?
                    MAX_FONT_BLOCKS * sizeof(tFontBlock) :
                    (g_sFontFile.sFontHeader.ui16NumBlocks *
                     sizeof(tFontBlock)));

        iFResult = f_read(&g_sFontFile.sFile, &g_sFontFile.psBlocks, uiToRead,
                          &uiRead);
        if((iFResult == FR_OK) && (uiRead == uiToRead))
        {
            //
            // All is well.  Tell the caller the font was opened successfully.
            //
            UARTprintf("Font %s opened successfully.\n", pcFilename);
            g_sFontFile.bInUse = true;
            return((uint8_t *)&g_sFontFile);
        }
        else
        {
            UARTprintf("Error %s (%d) reading block headers. Read %d, exp %d "
                       "bytes.\n", StringFromFresult(iFResult), iFResult,
                       uiRead, uiToRead);
            f_close(&g_sFontFile.sFile);
            return(0);
        }
    }
    else
    {
        //
        // We received an error while reading the file header so fail the call.
        //
        UARTprintf("Error %s (%d) reading font header.\n",
                   StringFromFresult(iFResult), iFResult);
        f_close(&g_sFontFile.sFile);
        return(0);
    }
}

//*****************************************************************************
//
// Frees a font and cleans up once an application has finished using it.
//
// This function releases all resources allocated during a previous call to
// FATFontWrapperLoad().  The caller must not make any further use of the
// font following this call unless another call to FATFontWrapperLoad() is
// made.
//
// \return None.
//
//*****************************************************************************
void FATFontWrapperUnload(uint8_t *pui8FontID)
{
    tFontFile *psFont;
    FRESULT iFResult;

    //
    // Parameter sanity check.
    //
    ASSERT(pui8FontID);

    //
    // Get a pointer to our instance data.
    //
    psFont = (tFontFile *)pui8FontID;

    //
    // Make sure a font is already open.  If not, just return.
    //
    if(!psFont->bInUse)
    {
        return;
    }

    //
    // Close the font file.
    //
    UARTprintf("Unloading font... \n");
    iFResult = f_close(&psFont->sFile);
    if(iFResult != FR_OK)
    {
        UARTprintf("Error %s (%d) from f_close.\n", StringFromFresult(iFResult),
                   iFResult);
    }

    //
    // Clean up our instance data.
    //
    psFont->bInUse = false;
    psFont->ui32CurrentGlyph = 0;
}
