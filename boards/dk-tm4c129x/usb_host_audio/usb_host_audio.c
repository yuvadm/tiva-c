//*****************************************************************************
//
// usb_host_audio.c - Main routine for the USB host audio example.
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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "driverlib/systick.h"
#include "driverlib/pin_map.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/listbox.h"
#include "grlib/pushbutton.h"
#include "utils/ustdlib.h"
#include "third_party/fatfs/src/ff.h"
#include "third_party/fatfs/src/diskio.h"
#include "drivers/usb_sound.h"
#include "utils/wavfile.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/frame.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"
#include "usblib/usblib.h"
#include "usblib/host/usbhost.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB host audio example application using SD Card FAT file system
//! (usb_host_audio)</h1>
//!
//! This example application demonstrates playing .wav files from an SD card
//! that is formatted with a FAT file system using USB host audio class.  The
//! application can browse the file system on the SD card and displays
//! all files that are found.  Files can be selected to show their format and
//! then played if the application determines that they are a valid .wav file.
//! Only PCM format (uncompressed) files may be played.
//!
//! For additional details about FatFs, see the following site:
//! http://elm-chan.org/fsw/ff/00index_e.html
//!
//! The application can be recompiled to run using and external USB phy to
//! implement a high speed host using an external USB phy.  To use the external
//! phy the application must be built with \b USE_ULPI defined.  This disables
//! the internal phy and the connector on the DK-TM4C129X board and enables the
//! connections to the external ULPI phy pins on the DK-TM4C129X board.
//
//*****************************************************************************

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
// Graphics context used to show text on the display.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// Global needed by the FAT driver to know the processor speed of the system.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

//*****************************************************************************
//
// Defines for the basic screen area used by the application.
//
//*****************************************************************************
#define STATUS_HEIGHT           40
#define BG_MIN_X                7
#define BG_MAX_X                (320 - 8)
#define BG_MIN_Y                24
#define BG_MAX_Y                (240 - 8)
#define BUTTON_HEIGHT           (STATUS_HEIGHT - 8)

//*****************************************************************************
//
// Forward declarations for functions called by the widgets used in the user
// interface.
//
//*****************************************************************************
void OnListBoxChange(tWidget *psWidget, int16_t i16Selected);
static void PlayPause(tWidget *psWidget);
static void Stop(tWidget *psWidget);

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;

//*****************************************************************************
//
// Define a pair of buffers that are used for holding path information.
// The buffer size must be large enough to hold the longest expected
// full path name, including the file name, and a trailing null character.
// The initial path is set to root "/".
//
//*****************************************************************************
#define PATH_BUF_SIZE   80
static char g_pcCwdBuf[PATH_BUF_SIZE] = "/";
static char g_pcTmpBuf[PATH_BUF_SIZE];

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100
#define MS_PER_SYSTICK (1000 / TICKS_PER_SECOND)

//*****************************************************************************
//
// Audio buffering definitions, these are optimized to deal with USB audio.
// AUDIO_TRANSFER_SIZE define one frame of audio at 48000 Stereo 16 bit
// and AUDIO_BUFFERS declares 16 frames(16ms) of audio buffering.
//
//*****************************************************************************
#define AUDIO_TRANSFER_SIZE     (192)
#define AUDIO_BUFFERS           (64)
#define AUDIO_BUFFER_SIZE       (AUDIO_TRANSFER_SIZE * AUDIO_BUFFERS)
uint32_t g_ui32TransferSize;
uint32_t g_ui32BufferSize;

//*****************************************************************************
//
// The main audio buffer and it's pointers.
//
//*****************************************************************************
uint8_t g_pui8AudioBuffer[AUDIO_BUFFER_SIZE];
volatile uint8_t *g_pui8Read;
volatile uint8_t *g_pui8Write;
volatile uint32_t g_ui32ValidBytes;

//*****************************************************************************
//
// Holds global flags for the system.
//
//*****************************************************************************
uint32_t g_ui32Flags;

//
// The last transfer has competed so a new one can be started.
//
#define FLAGS_TX_COMPLETE       1

//
// New audio device present.
//
#define FLAGS_DEVICE_CONNECT    2

//*****************************************************************************
//
// The global playback state for the application.
//
//*****************************************************************************
volatile enum
{
    AUDIO_PLAYING,
    AUDIO_PAUSED,
    AUDIO_STOPPED,
    AUDIO_NONE
}
g_ePlayState;

//*****************************************************************************
//
// These are the global .wav file states used by the application.
//
//*****************************************************************************
tWavFile g_sWavFile;
tWavHeader g_sWavHeader;

//*****************************************************************************
//
// Widget definitions
//
//*****************************************************************************

//*****************************************************************************
//
// Storage for the filename listbox widget string table.
//
//*****************************************************************************
#define NUM_LIST_STRINGS 48
const char *g_ppcDirListStrings[NUM_LIST_STRINGS];

//*****************************************************************************
//
// Storage for the names of the files in the current directory.  Filenames
// are stored in format "(D) filename.ext" for directories or "(F) filename.ext"
// for files.
//
//*****************************************************************************
#define MAX_FILENAME_STRING_LEN (4 + 8 + 1 + 3 + 1)
char g_pcFilenames[NUM_LIST_STRINGS][MAX_FILENAME_STRING_LEN];

//*****************************************************************************
//
// The canvas widgets for the wav file information.
//
//*****************************************************************************
extern tCanvasWidget g_sWaveInfoBackground;

char g_pcTime[16] = "";
Canvas(g_sWaveInfoTime, &g_sWaveInfoBackground, 0, 0,
       &g_sKentec320x240x16_SSD2119, BG_MAX_X - 166, BG_MIN_Y + 28, 158, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, ClrWhite, ClrWhite,
       g_psFontFixed6x8, g_pcTime, 0, 0);

char g_pcFormat[24] = "";
Canvas(g_sWaveInfoSample, &g_sWaveInfoBackground, &g_sWaveInfoTime, 0,
       &g_sKentec320x240x16_SSD2119, BG_MAX_X - 166, BG_MIN_Y + 18, 158, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, ClrWhite, ClrWhite,
       g_psFontFixed6x8, g_pcFormat, 0, 0);

Canvas(g_sWaveInfoFileName, &g_sWaveInfoBackground, &g_sWaveInfoSample, 0,
       &g_sKentec320x240x16_SSD2119, BG_MAX_X - 166, BG_MIN_Y + 8, 158, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, ClrWhite, ClrWhite,
       g_psFontFixed6x8, "", 0, 0);

//*****************************************************************************
//
// The canvas widget acting as the background for the wav file information.
//
//*****************************************************************************
Canvas(g_sWaveInfoBackground, WIDGET_ROOT, 0,
       &g_sWaveInfoFileName, &g_sKentec320x240x16_SSD2119,
       BG_MAX_X - 170, BG_MIN_Y + 4, 166, 80,
       CANVAS_STYLE_FILL, ClrBlack, ClrWhite, ClrWhite,
       g_psFontFixed6x8, 0, 0, 0);

extern tCanvasWidget g_sStatusPanel;

//
// Status text area.
//
Canvas(g_sStatusText, &g_sStatusPanel, 0, 0, &g_sKentec320x240x16_SSD2119,
       BG_MIN_X + 112, BG_MAX_Y - STATUS_HEIGHT + 4, 189, BUTTON_HEIGHT,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrLightGrey, ClrDarkGray, ClrBlack,
       g_psFontCmss16, "", 0, 0);

//
// Stop button.
//
RectangularButton(g_sStop, &g_sStatusPanel, &g_sStatusText, 0,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X + 58,
       BG_MAX_Y - STATUS_HEIGHT + 4, 50, BUTTON_HEIGHT,
       PB_STYLE_FILL | PB_STYLE_TEXT |
       PB_STYLE_RELEASE_NOTIFY, ClrLightGrey, ClrDarkGray, 0,
       ClrBlack, g_psFontCmss16, "Stop", 0, 0, 0 ,0 , Stop);

//
// Play/Pause button.
//
RectangularButton(g_sPlayPause, &g_sStatusPanel, &g_sStop, 0,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X + 4,
       BG_MAX_Y - STATUS_HEIGHT + 4, 50, BUTTON_HEIGHT,
       PB_STYLE_FILL | PB_STYLE_TEXT |
       PB_STYLE_RELEASE_NOTIFY, ClrLightGrey, ClrDarkGray, 0,
       ClrBlack, g_psFontCmss16, "Play", 0, 0, 0 ,0 , PlayPause);

//
// Background of the status area behind the buttons.
//
Canvas(g_sStatusPanel, WIDGET_ROOT, 0, &g_sPlayPause,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X, BG_MAX_Y - STATUS_HEIGHT,
       BG_MAX_X - BG_MIN_X, STATUS_HEIGHT,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_TOP, ClrGray, ClrWhite, ClrBlack, 0,
       0, 0, 0);

//
// The file list box.
//
ListBox(g_sDirList, WIDGET_ROOT, 0, 0,
        &g_sKentec320x240x16_SSD2119,
        BG_MIN_X + 4, BG_MIN_Y + 4, 120,
        BG_MAX_Y - BG_MIN_Y - STATUS_HEIGHT - 8,
        LISTBOX_STYLE_OUTLINE, ClrBlack, ClrDarkBlue, ClrSilver, ClrWhite,
        ClrWhite, g_psFontCmss12, g_ppcDirListStrings,
        NUM_LIST_STRINGS, 0, OnListBoxChange);

//*****************************************************************************
//
// State information for keep track of time.
//
//*****************************************************************************
static uint32_t g_ui32BytesPlayed;
static uint32_t g_ui32NextUpdate;

//*****************************************************************************
//
// Globals used to track play back position.
//
//*****************************************************************************
static uint16_t g_ui16Minutes;
static uint16_t g_ui16Seconds;

//*****************************************************************************
//
// Static constant strings used by the application.
//
//*****************************************************************************
static const char g_pcPlay[] = "Play";
static const char g_pcPause[] = "Pause";
static const char g_pcNoDevice[] ="No USB Device ";
static const char g_pcUnknownDevice[] ="Unknown Device ";
static const char g_pcDirError[] ="Directory Read Error ";

//*****************************************************************************
//
// This function is used to tell when to update the play back times for a file.
// It will only update the screen at 1 second intervals but can be called more
// often with no result.
//
//*****************************************************************************
static void
DisplayTime(uint32_t ui32ForceUpdate)
{
    uint32_t ui32Seconds;
    uint32_t ui32Minutes;

    //
    // Only display on the screen once per second.
    //
    if((g_ui32BytesPlayed >= g_ui32NextUpdate) || (ui32ForceUpdate != 0))
    {
        //
        // Set the next update time to one second later.
        //
        g_ui32NextUpdate = g_ui32BytesPlayed + g_sWavHeader.ui32AvgByteRate;

        //
        // Calculate the integer number of minutes and seconds.
        //
        ui32Seconds = g_ui32BytesPlayed / g_sWavHeader.ui32AvgByteRate;
        ui32Minutes = ui32Seconds / 60;
        ui32Seconds -= ui32Minutes * 60;

        //
        // Print the time string in the format mm.ss/mm.ss
        //
        usprintf(g_pcTime, "%2d:%02d/%d:%02d\0", ui32Minutes,ui32Seconds,
                 g_ui16Minutes, g_ui16Seconds);

        //
        // Display the updated time on the screen.
        //
        WidgetPaint((tWidget *)&g_sWaveInfoTime);
    }
}

//*****************************************************************************
//
// This function will handle stopping the play back of audio.  It will not do
// this immediately but will defer stopping audio at a later time.  This allows
// this function to be called from an interrupt handler.
//
//*****************************************************************************
static void
WaveStop(void)
{
    uint32_t ui32Idx;
    uint32_t *pui32Buffer;

    //
    // Stop playing audio.
    //
    g_ePlayState = AUDIO_STOPPED;

    //
    // Do unsigned long accesses to clear out the buffer.
    //
    pui32Buffer = (uint32_t *)g_pui8AudioBuffer;

    //
    // Zero out the buffer.
    //
    for(ui32Idx = 0; ui32Idx < (AUDIO_BUFFER_SIZE >> 2); ui32Idx++)
    {
        pui32Buffer[ui32Idx] = 0;
    }

    //
    // Reset the number of bytes played and for a time update on the screen.
    //
    g_ui32BytesPlayed = 0;
    g_ui32NextUpdate = 0;
    DisplayTime(1);

    //
    // Change the play/pause button to say play.
    //
    PushButtonTextSet(&g_sPlayPause, g_pcPlay);
    WidgetPaint((tWidget *)&g_sPlayPause);
}

//*****************************************************************************
//
// This function is used to change to a new directory in the file system.
// It takes a parameter that specifies the directory to make the current
// working directory.
// Path separators must use a forward slash "/".  The directory parameter
// can be one of the following:
// * root ("/")
// * a fully specified path ("/my/path/to/mydir")
// * a single directory name that is in the current directory ("mydir")
// * parent directory ("..")
//
// It does not understand relative paths, so dont try something like this:
// ("../my/new/path")
//
// Once the new directory is specified, it attempts to open the directory
// to make sure it exists.  If the new path is opened successfully, then
// the current working directory (cwd) is changed to the new path.
//
// In cases of error, the pui32Reason parameter will be written with one of
// the following values:
//
//  NAME_TOO_LONG_ERROR - combination of paths are too long for the buffer
//  OPENDIR_ERROR - there is some problem opening the new directory
//
//*****************************************************************************
static FRESULT
ChangeToDirectory(char *pcDirectory)
{
    uint32_t ui32Idx;
    FRESULT fresult;

    //
    // Copy the current working path into a temporary buffer so
    // it can be manipulated.
    //
    strcpy(g_pcTmpBuf, g_pcCwdBuf);

    //
    // If the first character is /, then this is a fully specified
    // path, and it should just be used as-is.
    //
    if(pcDirectory[0] == '/')
    {
        //
        // Make sure the new path is not bigger than the cwd buffer.
        //
        if(strlen(pcDirectory) + 1 > sizeof(g_pcCwdBuf))
        {
            return(FR_OK);
        }

        //
        // If the new path name (in argv[1])  is not too long, then
        // copy it into the temporary buffer so it can be checked.
        //
        else
        {
            strncpy(g_pcTmpBuf, pcDirectory, sizeof(g_pcTmpBuf));
        }
    }

    //
    // If the argument is .. then attempt to remove the lowest level
    // on the CWD.
    //
    else if(!strcmp(pcDirectory, ".."))
    {
        //
        // Get the index to the last character in the current path.
        //
        ui32Idx = strlen(g_pcTmpBuf) - 1;

        //
        // Back up from the end of the path name until a separator (/)
        // is found, or until we bump up to the start of the path.
        //
        while((g_pcTmpBuf[ui32Idx] != '/') && (ui32Idx > 1))
        {
            //
            // Back up one character.
            //
            ui32Idx--;
        }

        //
        // Now we are either at the lowest level separator in the
        // current path, or at the beginning of the string (root).
        // So set the new end of string here, effectively removing
        // that last part of the path.
        //
        g_pcTmpBuf[ui32Idx] = 0;
    }

    //
    // Otherwise this is just a normal path name from the current
    // directory, and it needs to be appended to the current path.
    //
    else
    {
        //
        // Test to make sure that when the new additional path is
        // added on to the current path, there is room in the buffer
        // for the full new path.  It needs to include a new separator,
        // and a trailing null character.
        //
        if(strlen(g_pcTmpBuf) + strlen(pcDirectory) + 2 > sizeof(g_pcCwdBuf))
        {
            return(FR_INVALID_OBJECT);
        }

        //
        // The new path is okay, so add the separator and then append
        // the new directory to the path.
        //
        else
        {
            //
            // If not already at the root level, then append a /
            //
            if(strcmp(g_pcTmpBuf, "/"))
            {
                strcat(g_pcTmpBuf, "/");
            }

            //
            // Append the new directory to the path.
            //
            strcat(g_pcTmpBuf, pcDirectory);
        }
    }

    //
    // At this point, a candidate new directory path is in g_pcTmpBuf.
    // Try to open it to make sure it is valid.
    //
    fresult = f_opendir(&g_sDirObject, g_pcTmpBuf);

    //
    // This was a valid path, so copy it into the CWD.
    //
    if(fresult == FR_OK)
    {
        strncpy(g_pcCwdBuf, g_pcTmpBuf, sizeof(g_pcCwdBuf));
    }

    //
    // Return success.
    //
    return(fresult);
}

//*****************************************************************************
//
// Fill the audio buffer with data from the open file.
//
//*****************************************************************************
static void
FillAudioBuffer(void)
{
    uint32_t ui32Count, ui32Size;
    uint8_t *pui8Read;

    //
    // If already full return.
    //
    if(g_ui32ValidBytes == g_ui32BufferSize)
    {
        return;
    }

    //
    // Snap shot the read pointer since it might change while we are filling
    // the buffer.  If the read pointer moved due to an interrupt we will
    // get the update on the next call to this function.
    //
    pui8Read = (uint8_t *)g_pui8Read;

    //
    // If write is ahead of read then fill to the end of the buffer if
    // possible.
    //
    if(pui8Read <= g_pui8Write)
    {
        //
        // Calculate the amount of space we have.
        //
        ui32Size = g_ui32BufferSize -
                   (uint32_t)(g_pui8Write - g_pui8AudioBuffer);
        ui32Count = WavRead(&g_sWavFile, (uint8_t *)g_pui8Write, ui32Size);

        g_pui8Write += ui32Count;
        IntMasterDisable();
        g_ui32ValidBytes += ui32Count;
        IntMasterEnable();

        if(g_pui8Write == (g_pui8AudioBuffer + g_ui32BufferSize))
        {
            g_pui8Write = g_pui8AudioBuffer;
        }

        ui32Size = pui8Read - g_pui8AudioBuffer;

        if(ui32Size)
        {
            ui32Count = WavRead(&g_sWavFile, (uint8_t *)g_pui8Write, ui32Size);
            g_pui8Write += ui32Count;
            IntMasterDisable();
            g_ui32ValidBytes += ui32Count;
            IntMasterEnable();
        }
    }

    //
    // If read is now ahead of write then fill to the read pointer.
    //
    if(pui8Read > g_pui8Write)
    {
        //
        // Calculate the amount of space we have.
        //
        ui32Size = (uint32_t)(pui8Read - g_pui8Write);
        ui32Count = WavRead(&g_sWavFile, (uint8_t *)g_pui8Write, ui32Size);

        g_pui8Write += ui32Count;

        IntMasterDisable();
        g_ui32ValidBytes += ui32Count;
        IntMasterEnable();
    }
}

//*****************************************************************************
//
// This function handles the callback from the USB audio device when a buffer
// has been played or a new buffer has been received.
//
//*****************************************************************************
static void
USBAudioOutCallback(void *pvBuffer, uint32_t ui32Event, uint32_t ui32Value)
{
    //
    // If a buffer has been played then schedule a new one to play.
    //
    if((ui32Event == USB_EVENT_TX_COMPLETE) && (g_ePlayState == AUDIO_PLAYING))
    {
        //
        // Indicate that a transfer was complete so that the non-interrupt
        // code can read in more data from the file.
        //
        HWREGBITW(&g_ui32Flags, FLAGS_TX_COMPLETE) = 1;

        //
        // Increment the read pointer.
        //
        g_pui8Read += g_ui32TransferSize;

        //
        // Remove the specified number of bytes from the buffer.
        //
        if(g_ui32ValidBytes > g_ui32TransferSize)
        {
            g_ui32ValidBytes -= g_ui32TransferSize;
        }
        else
        {
            g_ui32ValidBytes = 0;
        }

        //
        // Wrap the read pointer if necessary.
        //
        if(g_pui8Read >= (g_pui8AudioBuffer + g_ui32BufferSize))
        {
            g_pui8Read = g_pui8AudioBuffer;
        }

        //
        // Increment the number of bytes that have been played.
        //
        g_ui32BytesPlayed += g_ui32TransferSize;

        //
        // Schedule a new USB audio buffer to be transmitted to the USB
        // audio device.
        //
        USBSoundBufferOut((uint8_t *)g_pui8Read, g_ui32TransferSize,
                          USBAudioOutCallback);
    }
}

//*****************************************************************************
//
// This is the callback for the play/pause button.
//
//*****************************************************************************
static void
PlayPause(tWidget *psWidget)
{
    int16_t i16Select;

    //
    // If we are stopped, then start playing.
    //
    if(g_ePlayState == AUDIO_STOPPED)
    {
        //
        // Get the current selection from the list box.
        //
        i16Select = ListBoxSelectionGet(&g_sDirList);

        //
        // See if this is a valid .wav file that can be opened.
        //
        if(WavOpen(g_pcFilenames[i16Select], &g_sWavFile) == 0)
        {
            //
            // Initialize the read and write pointers.
            //
            g_pui8Read = g_pui8AudioBuffer;
            g_pui8Write = g_pui8AudioBuffer;
            g_ui32ValidBytes = 0;

            //
            // Fill the audio buffer from the file.
            //
            FillAudioBuffer();

            //
            // Start the audio playback.
            //
            USBSoundBufferOut((uint8_t *)g_pui8Read, g_ui32TransferSize,
                              USBAudioOutCallback);
            //
            // Indicate that wave play back should start.
            //
            g_ePlayState = AUDIO_PLAYING;

            //
            // Change the button to indicate pause.
            //
            PushButtonTextSet(&g_sPlayPause, g_pcPause);
            WidgetPaint((tWidget *)&g_sPlayPause);
        }
        else
        {
            //
            // Play was pressed on an invalid file.
            //
            CanvasTextSet(&g_sStatusText, "Invalid wav format ");
            WidgetPaint((tWidget *)&g_sPlayPause);

            return;
        }

        //
        // Initialize the read and write pointers.
        //
        g_pui8Read = g_pui8AudioBuffer;
        g_pui8Write = g_pui8AudioBuffer;
        g_ui32ValidBytes = 0;

        //
        // Fill the audio buffer from the file.
        //
        FillAudioBuffer();

        //
        // Start the audio playback.
        //
        USBSoundBufferOut((uint8_t *)g_pui8Read, g_ui32TransferSize,
                          USBAudioOutCallback);
    }
    else if(g_ePlayState == AUDIO_PLAYING)
    {
        //
        // Now switching to a paused state, so change the button to say play.
        //
        PushButtonTextSet(&g_sPlayPause, g_pcPlay);
        WidgetPaint((tWidget *)&g_sPlayPause);

        g_ePlayState = AUDIO_PAUSED;
    }
    else if(g_ePlayState == AUDIO_PAUSED)
    {
        //
        // Fill the audio buffer from the file.
        //
        FillAudioBuffer();

        //
        // Start the audio playback.
        //
        USBSoundBufferOut((uint8_t *)g_pui8Read, g_ui32TransferSize,
                          USBAudioOutCallback);

        //
        // Now switching to a play state, so change the button to say paused.
        //
        PushButtonTextSet(&g_sPlayPause, g_pcPause);
        WidgetPaint((tWidget *)&g_sPlayPause);

        g_ePlayState = AUDIO_PLAYING;
    }
}

//*****************************************************************************
//
// Stopped pressed so stop the wav playback.
//
//*****************************************************************************
static void
Stop(tWidget *psWidget)
{
    //
    // Stop play back if we are playing.
    //
    if((g_ePlayState == AUDIO_PLAYING) || (g_ePlayState == AUDIO_PAUSED))
    {
        WaveStop();
    }
}

//*****************************************************************************
//
// This function is called to read the contents of the current directory on
// the SD card and fill the listbox containing the names of all files and
// directories.
//
//*****************************************************************************
static int
PopulateFileListBox(bool bRepaint)
{
    uint32_t ui32ItemCount;
    FRESULT iFResult;

    //
    // Empty the list box on the display.
    //
    ListBoxClear(&g_sDirList);

    //
    // Make sure the list box will be redrawn next time the message queue
    // is processed.
    //
    if(bRepaint)
    {
        WidgetPaint((tWidget *)&g_sDirList);
    }

    //
    // Open the current directory for access.
    //
    iFResult = f_opendir(&g_sDirObject, g_pcCwdBuf);

    //
    // Check for error and return if there is a problem.
    //
    if(iFResult != FR_OK)
    {
        //
        // Change the text to reflect the change.
        //
        CanvasTextSet(&g_sStatusText, g_pcDirError);
        WidgetPaint((tWidget *)&g_sStatusText);
        return(iFResult);
    }

    //
    // If not at the root then add the ".." entry.
    //
    if(g_pcCwdBuf[1] != 0)
    {
        g_pcFilenames[0][0] = '.';
        g_pcFilenames[0][1] = '.';
        g_pcFilenames[0][2] = 0;
        ListBoxTextAdd(&g_sDirList, g_pcFilenames[0]);
        ui32ItemCount = 1;
    }
    else
    {
        ui32ItemCount = 0;
    }

    //
    // Enter loop to enumerate through all directory entries.
    //
    for(;;)
    {
        //
        // Read an entry from the directory.
        //
        iFResult = f_readdir(&g_sDirObject, &g_sFileInfo);

        //
        // Check for error and return if there is a problem.
        //
        if(iFResult != FR_OK)
        {
            //
            // Change the text to reflect the change.
            //
            CanvasTextSet(&g_sStatusText, g_pcDirError);
            WidgetPaint((tWidget *)&g_sStatusText);
            return(iFResult);
        }

        //
        // If the file name is blank, then this is the end of the
        // listing.
        //
        if(!g_sFileInfo.fname[0])
        {
            break;
        }

        //
        // Add the information as a line in the listbox widget.
        //
        if(ui32ItemCount < NUM_LIST_STRINGS)
        {
            if(g_sFileInfo.fattrib & AM_DIR)
            {
                usnprintf(g_pcFilenames[ui32ItemCount],
                          MAX_FILENAME_STRING_LEN, "+ %s", g_sFileInfo.fname);
            }
            else
            {
                ustrncpy(g_pcFilenames[ui32ItemCount], g_sFileInfo.fname,
                         MAX_FILENAME_STRING_LEN);
            }
            ListBoxTextAdd(&g_sDirList, g_pcFilenames[ui32ItemCount]);
        }

        //
        // Move to the next entry in the item array we use to populate the
        // list box.
        //
        ui32ItemCount++;
    }

    //
    // Made it to here, return with no errors.
    //
    return(0);
}

//*****************************************************************************
//
// The listbox widget callback function.
//
// This function is called whenever someone changes the selected entry in the
// listbox containing the files and directories found in the current directory.
//
//*****************************************************************************
void OnListBoxChange(tWidget *psWidget, int16_t i16Selected)
{
    int16_t i16Sel;

    //
    // Get the current selection from the list box.
    //
    i16Sel = ListBoxSelectionGet(&g_sDirList);

    //
    // Is there any selection?
    //
    if(i16Sel == -1)
    {
        return;
    }
    else
    {
        //
        // Is the selection a directory name.
        //
        if(g_pcFilenames[i16Sel][0] == '+')
        {
            if((g_ePlayState != AUDIO_PLAYING) &&
               (g_ePlayState != AUDIO_PAUSED))
            {
                CanvasTextSet(&g_sWaveInfoFileName, "");
                CanvasTextSet(&g_sWaveInfoSample, "");
                g_pcTime[0] = 0;
            }

            if(ChangeToDirectory(&g_pcFilenames[i16Sel][2]) == FR_OK)
            {
                PopulateFileListBox(true);
            }
        }
        else if((g_pcFilenames[i16Sel][0] == '.') &&
                (g_pcFilenames[i16Sel][1] == '.'))
        {
            //
            // If this was the .. selection then move up a directory.
            //
            if(ChangeToDirectory("..") == FR_OK)
            {
                PopulateFileListBox(true);
            }

            if((g_ePlayState != AUDIO_PLAYING) &&
               (g_ePlayState != AUDIO_PAUSED))
            {
                CanvasTextSet(&g_sWaveInfoFileName, "");
                CanvasTextSet(&g_sWaveInfoSample, "");
                g_pcTime[0] = 0;
            }
        }
        else
        {
            //
            // This was a normal file selections so see if it is a wav file.
            //
            CanvasTextSet(&g_sWaveInfoFileName, g_pcFilenames[i16Sel]);

            if((g_ePlayState == AUDIO_PLAYING) ||
               (g_ePlayState == AUDIO_PAUSED))
            {
                WaveStop();
            }

            if(WavOpen(g_pcFilenames[i16Sel], &g_sWavFile) == 0)
            {
                //
                // Read the .wav file format.
                //
                WavGetFormat(&g_sWavFile, &g_sWavHeader);

                //
                // Print the formatted string so that it can be
                // displayed.
                //
                usprintf(g_pcFormat, "%d Hz %d bit ",
                         g_sWavHeader.ui32SampleRate,
                         g_sWavHeader.ui16BitsPerSample);

                //
                // Concatenate the number of channels.
                //
                if(g_sWavHeader.ui16NumChannels == 1)
                {
                    strcat(g_pcFormat, "Mono");
                }
                else
                {
                    strcat(g_pcFormat, "Stereo");
                }

                CanvasTextSet(&g_sWaveInfoSample, g_pcFormat);

                //
                // Calculate the minutes and seconds in the file.
                //
                g_ui16Seconds = g_sWavHeader.ui32DataSize /
                              g_sWavHeader.ui32AvgByteRate;
                g_ui16Minutes = g_ui16Seconds / 60;
                g_ui16Seconds -= g_ui16Minutes * 60;

                //
                // Close the file, it will be re-opened on play.
                //
                WavClose(&g_sWavFile);

                //
                // Update the file time information.
                //
                DisplayTime(1);

                //
                // Close the file, it will be re-opened on play.
                //
                WavClose(&g_sWavFile);
            }
            else
            {
                CanvasTextSet(&g_sWaveInfoSample, "");
                g_pcTime[0] = 0;
                WidgetPaint((tWidget *)&g_sWaveInfoTime);
            }
        }

        //
        // Update the file name and wav file information
        //
        WidgetPaint((tWidget *)&g_sWaveInfoFileName);
        WidgetPaint((tWidget *)&g_sWaveInfoSample);
        WidgetPaint((tWidget *)&g_sWaveInfoTime);
    }
}

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.  FatFs requires a
// timer tick every 10ms for internal timing purposes.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Call the FatFs tick timer.
    //
    disk_timerproc();
}

//*****************************************************************************
//
// Initializes the file system module.
//
// \param None.
//
// This function initializes the third party FAT implementation.
//
// \return Returns \e true on success or \e false on failure.
//
//*****************************************************************************
static bool
FileInit(void)
{
    //
    // Mount the file system, using logical disk 0.
    //
    if(f_mount(0, &g_sFatFs) != FR_OK)
    {
        return(false);
    }
    return(true);
}

//*****************************************************************************
//
// This function handled global level events for the USB host audio.  This
// function was passed into the USBSoundInit() function.
//
//*****************************************************************************
static void
AudioEvent(uint32_t ui32Event, uint32_t ui32Param)
{
    switch(ui32Event)
    {
        case SOUND_EVENT_READY:
        {
            //
            // Flag that a new audio device is present.
            //
            HWREGBITW(&g_ui32Flags, FLAGS_DEVICE_CONNECT) = 1;

            //
            // Change the text to reflect the change.
            //
            CanvasTextSet(&g_sStatusText, "Ready ");
            WidgetPaint((tWidget *)&g_sStatusText);

            break;
        }
        case SOUND_EVENT_DISCONNECT:
        {
            //
            // If playing return the button text to play.
            //
            if(g_ePlayState == AUDIO_PLAYING)
            {
                PushButtonTextSet(&g_sPlayPause, g_pcPlay);
                WidgetPaint((tWidget *)&g_sPlayPause);

                WaveStop();
            }

            //
            // Device is no longer present.
            //
            HWREGBITW(&g_ui32Flags, FLAGS_DEVICE_CONNECT) = 0;
            g_ePlayState = AUDIO_NONE;

            //
            // Change the text to reflect the change.
            //
            CanvasTextSet(&g_sStatusText, g_pcNoDevice);
            WidgetPaint((tWidget *)&g_sStatusText);

            break;
        }
        case SOUND_EVENT_UNKNOWN_DEV:
        {
            if(ui32Param == 1)
            {
                //
                // .
                //
                CanvasTextSet(&g_sStatusText, g_pcUnknownDevice);
            }
            else
            {
                //
                // Unknown device disconnected.
                //
                CanvasTextSet(&g_sStatusText, g_pcNoDevice);
            }
            WidgetPaint((tWidget *)&g_sStatusText);

            break;
        }
        default:
        {
            break;
        }
    }
}

//*****************************************************************************
//
// The program main function.  It performs initialization, then handles wav
// file playback.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32Temp;
    uint32_t ui32Retcode, ui32PLLRate;
#ifdef USE_ULPI
    uint32_t ui32Setting;
#endif

    //
    // Set the system clock to run at 120MHz from the PLL.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                            SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                            SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet();

#ifdef USE_ULPI
    //
    // Switch the USB ULPI Pins over.
    //
    USBULPIPinoutSet();

    //
    // Enable USB ULPI with high speed support.
    //
    ui32Setting = USBLIB_FEATURE_ULPI_HS;
    USBOTGFeatureSet(0, USBLIB_FEATURE_USBULPI, &ui32Setting);

    //
    // Setting the PLL frequency to zero tells the USB library to use the
    // external USB clock.
    //
    ui32PLLRate = 0;
#else
    //
    // Save the PLL rate used by this application.
    //
    ui32PLLRate = 480000000;
#endif

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    ROM_SysTickPeriodSet(g_ui32SysClock / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(g_ui32SysClock);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&g_sContext, "usb-host-audio");

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(g_ui32SysClock);

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sDirList);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sStatusPanel);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sWaveInfoBackground);

    //
    // Issue the initial paint request to the widgets then immediately call
    // the widget manager to process the paint message.  This ensures that the
    // display is drawn as quickly as possible and saves the delay we would
    // otherwise experience if we processed the paint message after mounting
    // and reading the SD card.
    //
    WidgetPaint(WIDGET_ROOT);
    WidgetMessageQueueProcess();

    //
    // Determine whether or not an SD Card is installed.  If not, print a
    // warning and have the user install one and restart.
    //
    ui32Retcode = disk_initialize(0);

    if(ui32Retcode != RES_OK)
    {
        CanvasTextSet(&g_sStatusText,  "File system error! ");
        WidgetPaint((tWidget *)&g_sStatusText);
        while(1);
    }
    else
    {
        //
        // Mount the file system, using logical disk 0.
        //
        f_mount(0, &g_sFatFs);

        if(!FileInit())
        {
            CanvasTextSet(&g_sStatusText, "File system error! ");
            WidgetPaint((tWidget *)&g_sStatusText);
            while(1);
        }
    }

    //
    // Not playing anything right now.
    //
    g_ui32Flags = 0;
    g_ePlayState = AUDIO_NONE;

    PopulateFileListBox(true);

    //
    // Tell the USB library the CPU clock and the PLL frequency.  This is a
    // new requirement for TM4C129 devices.
    //
    USBHCDFeatureSet(0, USBLIB_FEATURE_CPUCLK, &g_ui32SysClock);
    USBHCDFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

    //
    // Configure the USB host output.
    //
    USBSoundInit(0, AudioEvent);

    //
    // Enter an (almost) infinite loop for reading and processing commands from
    // the user.
    //
    while(1)
    {
        //
        // On connect change the device state to ready.
        //
        if(HWREGBITW(&g_ui32Flags, FLAGS_DEVICE_CONNECT))
        {
            HWREGBITW(&g_ui32Flags, FLAGS_DEVICE_CONNECT) = 0;

            //
            // Getting here means the device is ready.
            // Reset the CWD to the root directory.
            //
            g_pcCwdBuf[0] = '/';
            g_pcCwdBuf[1] = 0;

            //
            // Initiate a directory change to the root.  This will
            // populate a menu structure representing the root directory.
            //
            if(ChangeToDirectory("/") == FR_OK)
            {
                //
                // Request a repaint so the file menu will be shown
                //
                WidgetPaint(WIDGET_ROOT);
            }
            else
            {
                CanvasTextSet(&g_sStatusText,
                              "Error accessing root directory ");
                WidgetPaint((tWidget *)&g_sStatusText);
                while(1);
            }

            //
            // Attempt to set the audio format to 44100 16 bit stereo by
            // default otherwise try 48000 16 bit stereo.
            //
            if(USBSoundOutputFormatSet(44100, 16, 2) == 0)
            {
                ui32Temp = 44100;
            }
            else if(USBSoundOutputFormatSet(48000, 16, 2) == 0)
            {
                ui32Temp = 48000;
            }
            else
            {
                ui32Temp = 0;
            }

            //
            // If the audio device was support put the sample rate in the
            // status line.
            //
            if(ui32Temp != 0)
            {
                //
                // Calculate the number of bytes per USB frame.
                //
                g_ui32TransferSize = (ui32Temp * 4) / 1000;

                //
                // Calculate the size of the audio buffer.
                //
                g_ui32BufferSize = AUDIO_BUFFERS * g_ui32TransferSize;

                //
                // Print the time string in the format mm.ss/mm.ss
                //
                if(ui32Temp == 44100)
                {
                    CanvasTextSet(&g_sStatusText, "44.1 kHz Ready ");
                }
                else if(ui32Temp == 48000)
                {
                    CanvasTextSet(&g_sStatusText, "48 kHz Ready ");
                }

                g_ePlayState = AUDIO_STOPPED;
            }
            else
            {
                CanvasTextSet(&g_sStatusText, "Unsupported Audio Device ");
                g_ePlayState = AUDIO_NONE;
            }
            WidgetPaint((tWidget *)&g_sStatusText);
        }

        //
        // Handle the case when the wave file is playing.
        //
        if(g_ePlayState == AUDIO_PLAYING)
        {
            //
            // Handle the transmit complete event.
            //
            if(HWREGBITW(&g_ui32Flags, FLAGS_TX_COMPLETE))
            {
                //
                // Clear the transmit complete flag.
                //
                HWREGBITW(&g_ui32Flags, FLAGS_TX_COMPLETE) = 0;

                //
                // If there is space available then fill the buffer.
                //
                FillAudioBuffer();

                //
                // If we run out of valid bytes then stop the playback.
                //
                if(g_ui32ValidBytes == 0)
                {
                    //
                    // No more data or error so stop playing.
                    //
                    WaveStop();
                }

                //
                // Update the real display time.
                //
                DisplayTime(0);
            }
        }

        //
        // Need to periodically call the USBSoundMain() routine so that
        // non-interrupt gets a chance to run.
        //
        USBSoundMain();

        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();
    }
}

