//*****************************************************************************
//
// usb_host_msc.c - Example program for reading files from a USB flash drive.
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
#include <string.h>
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/udma.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/listbox.h"
#include "grlib/pushbutton.h"
#include "usblib/usblib.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhmsc.h"
#include "utils/ustdlib.h"
#include "third_party/fatfs/src/ff.h"
#include "third_party/fatfs/src/diskio.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Mass Storage Class Host Example (usb_host_msc)</h1>
//!
//! This example application demonstrates reading a file system from
//! a USB flash disk.  It makes use of FatFs, a FAT file system driver.  It
//! provides a simple widget-based interface on the display for viewing and
//! navigating the file system on the flash disk.
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
// Defines the size of the buffers that hold the path, or temporary
// data from the USB disk.  There are two buffers allocated of this size.
// The buffer size must be large enough to hold the int32_test expected
// full path name, including the file name, and a trailing null character.
//
//*****************************************************************************
#define PATH_BUF_SIZE   80

//*****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE    64

//*****************************************************************************
//
// Defines the number of times to call to check if the attached device is
// ready.
//
//*****************************************************************************
#define USBMSC_DRIVE_RETRY      4

//*****************************************************************************
//
// This buffer holds the full path to the current working directory.
// Initially it is root ("/").
//
//*****************************************************************************
static char g_cCwdBuf[PATH_BUF_SIZE] = "/";

//*****************************************************************************
//
// A temporary data buffer used when manipulating file paths, or reading data
// from the SD card.
//
//*****************************************************************************
static char g_cTmpBuf[PATH_BUF_SIZE];

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
// A structure that holds a mapping between an FRESULT numerical code,
// and a string representation.  FRESULT codes are returned from the FatFs
// FAT file system driver.
//
//*****************************************************************************
typedef struct
{
    FRESULT fresult;
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
// printing to the screen.
//
//*****************************************************************************
tFresultString g_cFresultStrings[] =
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
#define NUM_FRESULT_CODES (sizeof(g_cFresultStrings) / sizeof(tFresultString))

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100
#define MS_PER_SYSTICK (1000 / TICKS_PER_SECOND)

//*****************************************************************************
//
// Our running system tick counter and a global used to determine the time
// elapsed since last call to GetTickms().
//
//*****************************************************************************
uint32_t g_ui32SysTickCount;
uint32_t g_ui32LastTick;

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
// Storage for the strings which appear in the status box at the bottom of the
// display.
//
//****************************************************************************
#define NUM_STATUS_STRINGS 6
#define MAX_STATUS_STRING_LEN (36 + 1)
char g_pcStatus[NUM_STATUS_STRINGS][MAX_STATUS_STRING_LEN];

//*****************************************************************************
//
// Holds global flags for the system.
//
//*****************************************************************************
uint32_t g_ui32Flags = 0;

//*****************************************************************************
//
// Flag indicating that some USB device is connected.
//
//*****************************************************************************
#define FLAGS_DEVICE_PRESENT    0x00000001

//*****************************************************************************
//
// Hold the current state for the application.
//
//*****************************************************************************
volatile enum
{
    //
    // No device is present.
    //
    STATE_NO_DEVICE,

    //
    // Mass storage device is being enumerated.
    //
    STATE_DEVICE_ENUM,

    //
    // Mass storage device is ready.
    //
    STATE_DEVICE_READY,

    //
    // An unsupported device has been attached.
    //
    STATE_UNKNOWN_DEVICE,

    //
    // A mass storage device was connected but failed to ever report ready.
    //
    STATE_TIMEOUT_DEVICE,

    //
    // A power fault has occurred.
    //
    STATE_POWER_FAULT
}
g_eState;

//*****************************************************************************
//
// The current USB operating mode - Host, Device or unknown.
//
//*****************************************************************************
tUSBMode g_eCurrentUSBMode;

//*****************************************************************************
//
// The size of the host controller's memory pool in bytes.
//
//*****************************************************************************
#define HCD_MEMORY_SIZE         128

//*****************************************************************************
//
// The memory pool to provide to the Host controller driver.
//
//*****************************************************************************
uint8_t g_pHCDPool[HCD_MEMORY_SIZE];

//*****************************************************************************
//
// The instance data for the MSC driver.
//
//*****************************************************************************
tUSBHMSCInstance *g_psMSCInstance = 0;

//*****************************************************************************
//
// Declare the USB Events driver interface.
//
//*****************************************************************************
DECLARE_EVENT_DRIVER(g_sUSBEventDriver, 0, 0, USBHCDEvents);

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the MSC class is loaded.
//
//*****************************************************************************
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
    &g_sUSBHostMSCClassDriver,
    &g_sUSBEventDriver
};

//*****************************************************************************
//
// This global holds the number of class drivers in the g_ppHostClassDrivers
// list.
//
//*****************************************************************************
static const uint32_t g_ui32NumHostClassDrivers =
    sizeof(g_ppHostClassDrivers) / sizeof(tUSBHostClassDriver *);

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.  In this application uDMA is only used for USB,
// so only the first 6 channels are needed.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
tDMAControlTable g_sDMAControlTable[6];
#elif defined(ccs)
#pragma DATA_ALIGN(g_sDMAControlTable, 1024)
tDMAControlTable g_sDMAControlTable[6];
#else
tDMAControlTable g_sDMAControlTable[6] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// Storage for the status listbox widget string table.
//
//*****************************************************************************
const char *g_ppcStatusStrings[NUM_STATUS_STRINGS] =
{
    g_pcStatus[0],
    g_pcStatus[1],
    g_pcStatus[2],
    g_pcStatus[3],
    g_pcStatus[4],
    g_pcStatus[5]
};
uint32_t g_ui32StatusStringIndex = 0;

//*****************************************************************************
//
// Forward declarations for functions called by the widgets used in the user
// interface.
//
//*****************************************************************************
void OnListBoxChange(tWidget *pWidget, int16_t ui16Selected);
void OnBtnCD(tWidget *pWidget);
void OnBtnUp(tWidget *pWidget);
static int PopulateFileListBox(bool bRedraw);
static FRESULT ChangeToDirectory(char *pcDirectory, uint32_t *pui32Reason);
static int PrintfStatus(char *pcFormat, ...);
static const char *StringFromFresult(FRESULT fresult);

//*****************************************************************************
//
// Widget definitions
//
//*****************************************************************************

//*****************************************************************************
//
// The listbox used to display directory contents.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;
extern tCanvasWidget g_sUpBackground;
extern tCanvasWidget g_sCDBackground;

ListBox(g_sStatusList, &g_sBackground, 0, 0, &g_sKentec320x240x16_SSD2119,
        40, 170, 220, 52, (LISTBOX_STYLE_OUTLINE | LISTBOX_STYLE_LOCKED |
        LISTBOX_STYLE_WRAP), ClrBlack, ClrBlack, ClrSilver, ClrSilver, ClrWhite,
        &g_sFontFixed6x8, g_ppcStatusStrings,  NUM_STATUS_STRINGS,
        NUM_STATUS_STRINGS, 0);

ListBox(g_sDirList, &g_sBackground, &g_sStatusList, 0,
        &g_sKentec320x240x16_SSD2119,
        40, 60, 120, 100, LISTBOX_STYLE_OUTLINE, ClrBlack, ClrDarkBlue,
        ClrSilver, ClrWhite, ClrWhite, &g_sFontCmss12, g_ppcDirListStrings,
        NUM_LIST_STRINGS, 0, OnListBoxChange);

//*****************************************************************************
//
// The canvas widget used to show the current directory.
//
//*****************************************************************************
Canvas(g_sPWDTitle, &g_sBackground, &g_sDirList, 0,
       &g_sKentec320x240x16_SSD2119, 10, 35, 40, 20, CANVAS_STYLE_TEXT,
       ClrBlack, 0, ClrWhite, &g_sFontCmss12, "PWD:", 0, 0);

//*****************************************************************************
//
// The canvas widget used to show the current directory.
//
//*****************************************************************************
Canvas(g_sPWD, &g_sBackground, &g_sPWDTitle, 0, &g_sKentec320x240x16_SSD2119,
       50, 35, 260, 20, (CANVAS_STYLE_TEXT | CANVAS_STYLE_FILL |
       CANVAS_STYLE_TEXT_LEFT), ClrBlack, 0, ClrWhite, &g_sFontCmss12,
       g_cCwdBuf, 0, 0);

//*****************************************************************************
//
// The button used to change to the selected directory directory.
//
//*****************************************************************************
RectangularButton(g_sCDBtn, &g_sCDBackground, 0, 0,
                  &g_sKentec320x240x16_SSD2119, 170, 75, 90, 30,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrBlack, ClrBlue, ClrWhite, ClrWhite,
                   &g_sFontCm20, "CD", 0, 0, 0, 0, OnBtnCD);

//*****************************************************************************
//
// The canvas widget acting as the background to the CD button.
//
//*****************************************************************************
Canvas(g_sCDBackground, WIDGET_ROOT, 0, 0,
       &g_sKentec320x240x16_SSD2119, 170, 75, 90, 30,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The button used to change to the next higher directory.
//
//*****************************************************************************
RectangularButton(g_sUpBtn, &g_sUpBackground, 0, 0,
                  &g_sKentec320x240x16_SSD2119, 170, 115, 90, 30,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrBlack, ClrBlue, ClrWhite, ClrWhite,
                   &g_sFontCm20, "Up", 0, 0, 0, 0, OnBtnUp);

//*****************************************************************************
//
// The canvas widget acting as the background to the UP button.
//
//*****************************************************************************
Canvas(g_sUpBackground, WIDGET_ROOT, &g_sCDBackground, 0,
       &g_sKentec320x240x16_SSD2119, 170, 115, 90, 30,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The canvas widget acting as the background to the left portion of the
// display.
//
//*****************************************************************************
Canvas(g_sBackground, WIDGET_ROOT, &g_sUpBackground, &g_sPWD,
       &g_sKentec320x240x16_SSD2119, 10, 60, 120, 170,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(int8_t *pi8Filename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// The listbox widget callback function.
//
// This function is called whenever someone changes the selected entry in the
// listbox containing the files and directories found in the current directory.
//
//*****************************************************************************
void
OnListBoxChange(tWidget *pWidget, int16_t ui16Selected)
{
    int16_t i16Selected;

    //
    // If no USB drive is present, just ignore this.
    //
    if(g_eState != STATE_DEVICE_READY)
    {
        return;
    }

    //
    // Get the current selection from the list box.
    //
    i16Selected = ListBoxSelectionGet(&g_sDirList);

    //
    // Is there any selection?
    //
    if(i16Selected == -1)
    {
        return;
    }
    else
    {
        //
        // Is the selection a directory name?
        //
        if(g_pcFilenames[i16Selected][1] == 'D')
        {
            //
            // Enable the "CD" button.
            //
            WidgetAdd((tWidget *)&g_sCDBackground, (tWidget *)&g_sCDBtn);
        }
        else
        {
            //
            // Hide the "CD" button.
            //
            WidgetRemove((tWidget *)&g_sCDBtn);
        }

        //
        // Make sure the CD button (or its background) is drawn correctly).
        //
        WidgetPaint((tWidget *)&g_sCDBackground);
    }

    //
    // Update the status display to say what we've done.
    //
    PrintfStatus("Selected %s %s\n",
                 (g_pcFilenames[i16Selected][1] == 'D') ? "dir" : "file",
                 &g_pcFilenames[i16Selected][4]);
}

//*****************************************************************************
//
// The "CD" button widget callback function.
//
// This function is called whenever someone presses the "CD" button.
//
//*****************************************************************************
void
OnBtnCD(tWidget *pWidget)
{
    int16_t i16Selected;
    uint32_t ui32Reason;
    FRESULT fresult;
    //
    // Get the current selection from the list box.
    //
    i16Selected = ListBoxSelectionGet(&g_sDirList);

    //
    // Is there any selection?
    //
    if(i16Selected == -1)
    {
        return;
    }
    else
    {
        //
        // Is the selection a directory name?
        //
        if(g_pcFilenames[i16Selected][1] == 'D')
        {
            //
            // Yes - change to the new directory.
            //
            fresult = ChangeToDirectory(&g_pcFilenames[i16Selected][4],
                                        &ui32Reason);

            if(fresult != FR_OK)
            {
                //
                // Update the status display to show the error.
                //
                PrintfStatus("Error changing directory.");
                PrintfStatus((char *)StringFromFresult(fresult));
            }
            else
            {
                //
                // Tell the user what happened.
                //
                PrintfStatus("Changed to %s", g_cCwdBuf);

                //
                // Update the directory name and the list box contents.
                //
                PopulateFileListBox(true);
                WidgetPaint((tWidget *)&g_sPWD);

                //
                // Enable the "Up" button and disable the "CD" button.
                //
                WidgetAdd((tWidget *)&g_sUpBackground, (tWidget *)&g_sUpBtn);
                WidgetRemove((tWidget *)&g_sCDBtn);

                //
                // Make sure the buttons are repainted correctly.
                //
                WidgetPaint((tWidget *)&g_sUpBtn);
                WidgetPaint((tWidget *)&g_sCDBackground);
            }
        }
    }
}

//*****************************************************************************
//
// The "Up" button widget callback function.
//
// This function is called whenever someone presses the "Up" button.
//
//*****************************************************************************
void
OnBtnUp(tWidget *pWidget)
{
    uint32_t ui32Reason;
    FRESULT fresult;

    //
    // Change up one directory.
    //
    fresult = ChangeToDirectory("..", &ui32Reason);

    if(fresult != FR_OK)
    {
        //
        // Update the status display to show the error.
        //
        PrintfStatus("Error changing directory.");
        PrintfStatus((char *)StringFromFresult(fresult));
    }
    else
    {
        //
        // Update the directory name and the list box contents.
        //
        WidgetPaint((tWidget *)&g_sPWD);
        PopulateFileListBox(true);

        //
        // If we are now in the root directory, hide the "Up" button.
        //
        if((strlen(g_cCwdBuf) == 1) && (g_cCwdBuf[0] == '/'))
        {
            WidgetRemove((tWidget *)&g_sUpBtn);
        }
        else
        {
            WidgetAdd((tWidget *)&g_sUpBackground, (tWidget *)&g_sUpBtn);
        }

        //
        // Disable the CD button since re-populating the list removes the
        // selection.
        //
        WidgetRemove((tWidget *)&g_sCDBtn);

        //
        // Tell the user what happened.
        //
        PrintfStatus("Changed to %s", g_cCwdBuf);

        //
        // Repaint the buttons.
        //
        WidgetPaint((tWidget *)&g_sUpBackground);
        WidgetPaint((tWidget *)&g_sCDBackground);
    }
}

//*****************************************************************************
//
// This function is used to add a new string to the status list box at the
// bottom of the display.
//
//*****************************************************************************
static int
PrintfStatus(char *pi8Format, ...)
{
    int iRet;
    va_list vaArgP;

    //
    // Start the varargs processing.
    //
    va_start(vaArgP, pi8Format);

    //
    // Call vsnprintf to perform the conversion.
    //
    iRet = uvsnprintf(g_pcStatus[g_ui32StatusStringIndex],
                      MAX_STATUS_STRING_LEN, pi8Format, vaArgP);

    //
    // End the varargs processing.
    //
    va_end(vaArgP);

    //
    // Add the new string to the status listbox.
    //
    ListBoxTextAdd(&g_sStatusList, g_pcStatus[g_ui32StatusStringIndex]);

    //
    // Update our string index.
    //
    g_ui32StatusStringIndex++;
    if(g_ui32StatusStringIndex == NUM_STATUS_STRINGS)
    {
        g_ui32StatusStringIndex = 0;
    }

    //
    // Repaint the status listbox.
    //
    WidgetPaint((tWidget *)&g_sStatusList);

    //
    // Return the conversion count.
    //
    return(iRet);

}

//*****************************************************************************
//
// This function returns a string representation of an error code
// that was returned from a function call to FatFs.  It can be used
// for printing human readable error messages.
//
//*****************************************************************************
static const char *
StringFromFresult(FRESULT fresult)
{
    unsigned int uIdx;

    //
    // Enter a loop to search the error code table for a matching
    // error code.
    //
    for(uIdx = 0; uIdx < NUM_FRESULT_CODES; uIdx++)
    {
        //
        // If a match is found, then return the string name of the
        // error code.
        //
        if(g_cFresultStrings[uIdx].fresult == fresult)
        {
            return(g_cFresultStrings[uIdx].pcResultStr);
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
// This is the handler for this SysTick interrupt.  FatFs requires a
// timer tick every 10 ms for internal timing purposes.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Update our tick counter.
    //
    g_ui32SysTickCount++;
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
// This is the callback from the MSC driver.
//
// \param ps32Instance is the driver instance which is needed when communicating
// with the driver.
// \param ui32Event is one of the events defined by the driver.
// \param pvData is a pointer to data passed into the initial call to register
// the callback.
//
// This function handles callback events from the MSC driver.  The only events
// currently handled are the MSC_EVENT_OPEN and MSC_EVENT_CLOSE.  This allows
// the main routine to know when an MSC device has been detected and
// enumerated and when an MSC device has been removed from the system.
//
// \return None
//
//*****************************************************************************
static void
MSCCallback(tUSBHMSCInstance *ps32Instance, uint32_t ui32Event, void *pvData)
{
    //
    // Determine the event.
    //
    switch(ui32Event)
    {
        //
        // Called when the device driver has successfully enumerated an MSC
        // device.
        //
        case MSC_EVENT_OPEN:
        {
            //
            // Proceed to the enumeration state.
            //
            g_eState = STATE_DEVICE_ENUM;

            break;
        }

        //
        // Called when the device driver has been unloaded due to error or
        // the device is no int32_ter present.
        //
        case MSC_EVENT_CLOSE:
        {
            //
            // Go back to the "no device" state and wait for a new connection.
            //
            g_eState = STATE_NO_DEVICE;

            //
            // Re-initialize the file system.
            //
            FileInit();

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
// This is the generic callback from host stack.
//
// \param pvData is actually a pointer to a tEventInfo structure.
//
// This function will be called to inform the application when a USB event has
// occurred that is outside those related to the mass storage device.  At this
// point this is used to detect unsupported devices being inserted and removed.
// It is also used to inform the application when a power fault has occurred.
// This function is required when the g_USBGenerii8EventDriver is included in
// the host controller driver array that is passed in to the
// USBHCDRegisterDrivers() function.
//
// \return None.
//
//*****************************************************************************
void
USBHCDEvents(void *pvData)
{
    tEventInfo *pEventInfo;

    //
    // Cast this pointer to its actual type.
    //
    pEventInfo = (tEventInfo *)pvData;

    switch(pEventInfo->ui32Event)
    {
        //
        // An unknown device has been connected.
        //
        case USB_EVENT_UNKNOWN_CONNECTED:
        {
            //
            // An unknown device was detected.
            //
            g_eState = STATE_UNKNOWN_DEVICE;

            break;
        }

        //
        // The unknown device has been been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // Unknown device has been removed.
            //
            g_eState = STATE_NO_DEVICE;

            break;
        }

        //
        // A bus power fault was detected.
        //
        case USB_EVENT_POWER_FAULT:
        {
            //
            // No power means no device is present.
            //
            g_eState = STATE_POWER_FAULT;

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
// This function is called to read the contents of the current directory on
// the SD card and fill the listbox containing the names of all files and
// directories.
//
//*****************************************************************************
static int
PopulateFileListBox(bool bRepaint)
{
    uint32_t ui32ItemCount;
    FRESULT fresult;

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
    fresult = f_opendir(&g_sDirObject, g_cCwdBuf);

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
        //
        // Ensure that the error is reported.
        //
        PrintfStatus("Error from USB disk:");
        PrintfStatus((char *)StringFromFresult(fresult));
        return(fresult);
    }

    ui32ItemCount = 0;

    //
    // Enter loop to enumerate through all directory entries.
    //
    for(;;)
    {
        //
        // Read an entry from the directory.
        //
        fresult = f_readdir(&g_sDirObject, &g_sFileInfo);

        //
        // Check for error and return if there is a problem.
        //
        if(fresult != FR_OK)
        {
            PrintfStatus("Error from USB disk:");
            PrintfStatus((char *)StringFromFresult(fresult));
            return(fresult);
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
            usnprintf(g_pcFilenames[ui32ItemCount], MAX_FILENAME_STRING_LEN,
                      "(%c) %s", (g_sFileInfo.fattrib & AM_DIR) ? 'D' : 'F',
                      g_sFileInfo.fname);
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
// This function implements the "cd" command.  It takes an argument
// that specifies the directory to make the current working directory.
// Path separators must use a forward slash "/".  The argument to cd
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
//*****************************************************************************
static FRESULT
ChangeToDirectory(char *pi8Directory, uint32_t *pui32Reason)
{
    unsigned int uIdx;
    FRESULT fresult;

    //
    // Copy the current working path into a temporary buffer so
    // it can be manipulated.
    //
    strcpy(g_cTmpBuf, g_cCwdBuf);

    //
    // If the first character is /, then this is a fully specified
    // path, and it should just be used as-is.
    //
    if(pi8Directory[0] == '/')
    {
        //
        // Make sure the new path is not bigger than the cwd buffer.
        //
        if(strlen(pi8Directory) + 1 > sizeof(g_cCwdBuf))
        {
            *pui32Reason = NAME_TOO_LONG_ERROR;
            return(FR_OK);
        }

        //
        // If the new path name (in argv[1])  is not too int32_t, then
        // copy it into the temporary buffer so it can be checked.
        //
        else
        {
            strncpy(g_cTmpBuf, pi8Directory, sizeof(g_cTmpBuf));
        }
    }

    //
    // If the argument is .. then attempt to remove the lowest level
    // on the CWD.
    //
    else if(!strcmp(pi8Directory, ".."))
    {
        //
        // Get the index to the last character in the current path.
        //
        uIdx = strlen(g_cTmpBuf) - 1;

        //
        // Back up from the end of the path name until a separator (/)
        // is found, or until we bump up to the start of the path.
        //
        while((g_cTmpBuf[uIdx] != '/') && (uIdx > 1))
        {
            //
            // Back up one character.
            //
            uIdx--;
        }

        //
        // Now we are either at the lowest level separator in the
        // current path, or at the beginning of the string (root).
        // So set the new end of string here, effectively removing
        // that last part of the path.
        //
        g_cTmpBuf[uIdx] = 0;
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
        if(strlen(g_cTmpBuf) + strlen(pi8Directory) + 1 + 1 > sizeof(g_cCwdBuf))
        {
            *pui32Reason = NAME_TOO_LONG_ERROR;
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
            if(strcmp(g_cTmpBuf, "/"))
            {
                strcat(g_cTmpBuf, "/");
            }

            //
            // Append the new directory to the path.
            //
            strcat(g_cTmpBuf, pi8Directory);
        }
    }

    //
    // At this point, a candidate new directory path is in chTmpBuf.
    // Try to open it to make sure it is valid.
    //
    fresult = f_opendir(&g_sDirObject, g_cTmpBuf);

    //
    // If it cant be opened, then it is a bad path.  Inform
    // user and return.
    //
    if(fresult != FR_OK)
    {
        *pui32Reason = OPENDIR_ERROR;
        return(fresult);
    }

    //
    // Otherwise, it is a valid new path, so copy it into the CWD and update
    // the screen.
    //
    else
    {
        strncpy(g_cCwdBuf, g_cTmpBuf, sizeof(g_cCwdBuf));
    }

    //
    // Return success.
    //
    return(FR_OK);
}

//*****************************************************************************
//
// This function returns the number of ticks since the last time this function
// was called.
//
//*****************************************************************************
uint32_t
GetTickms(void)
{
    uint32_t ui32RetVal;
    uint32_t ui32Saved;

    ui32RetVal = g_ui32SysTickCount;
    ui32Saved = ui32RetVal;

    if(ui32Saved > g_ui32LastTick)
    {
        ui32RetVal = ui32Saved - g_ui32LastTick;
    }
    else
    {
        ui32RetVal = g_ui32LastTick - ui32Saved;
    }

    //
    // This could miss a few milliseconds but the timings here are on a
    // much larger scale.
    //
    g_ui32LastTick = ui32Saved;

    //
    // Return the number of milliseconds since the last time this was called.
    //
    return(ui32RetVal * MS_PER_SYSTICK);
}

//*****************************************************************************
//
// The program main function.  It performs initialization, then handles the
// user interaction via the touch screen graphical interface.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32DriveTimeout, ui32SysClock, ui32PLLRate;
#ifdef USE_ULPI
    uint32_t ui32Setting;
#endif
    tContext sContext;

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
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(ui32SysClock);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&sContext, "usb-host-msc");

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    ROM_SysTickPeriodSet(ui32SysClock / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable the uDMA controller and set up the control table base.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    ROM_uDMAEnable();
    ROM_uDMAControlBaseSet(g_sDMAControlTable);

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(ui32SysClock);

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sBackground);

    //
    // Set some initial strings.
    //
    ListBoxTextAdd(&g_sDirList, "Waiting for device...");

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
    // Initially wait for device connection.
    //
    g_eState = STATE_NO_DEVICE;

    //
    // Initialize the USB stack for host mode.
    //
    USBStackModeSet(0, eUSBModeHost, 0);

    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ui32NumHostClassDrivers);

    //
    // Open an instance of the mass storage class driver.
    //
    g_psMSCInstance = USBHMSCDriveOpen(0, MSCCallback);

    //
    // Initialize the drive timeout.
    //
    ui32DriveTimeout = USBMSC_DRIVE_RETRY;

    //
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);

    //
    // Tell the USB library the CPU clock and the PLL frequency.  This is a
    // new requirement for TM4C129 devices.
    //
    USBHCDFeatureSet(0, USBLIB_FEATURE_CPUCLK, &ui32SysClock);
    USBHCDFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

    //
    // Initialize the USB controller for host operation.
    //
    USBHCDInit(0, g_pHCDPool, HCD_MEMORY_SIZE);

    //
    // Initialize the file system.
    //
    FileInit();

    //
    // Enter an (almost) infinite loop for reading and processing commands from
    // the user.
    //
    while(1)
    {
        //
        // Call the USB stack to keep it running.
        //
        USBHCDMain();

        //
        // Process any messages in the widget message queue.  This keeps the
        // display UI running.
        //
        WidgetMessageQueueProcess();

        switch(g_eState)
        {
            case STATE_DEVICE_ENUM:
            {
                //
                // Take it easy on the Mass storage device if it is slow to
                // start up after connecting.
                //
                if(USBHMSCDriveReady(g_psMSCInstance) != 0)
                {
                    //
                    // Wait about 500ms before attempting to check if the
                    // device is ready again.
                    //
                    ROM_SysCtlDelay(ui32SysClock / (3 * 2));

                    //
                    // Decrement the retry count.
                    //
                    ui32DriveTimeout--;

                    //
                    // If the timeout is hit then go to the
                    // STATE_TIMEOUT_DEVICE state.
                    //
                    if(ui32DriveTimeout == 0)
                    {
                        g_eState = STATE_TIMEOUT_DEVICE;
                    }
                    break;
                }

                //
                // Getting here means the device is ready.
                // Reset the CWD to the root directory.
                //
                g_cCwdBuf[0] = '/';
                g_cCwdBuf[1] = 0;

                //
                // Fill the list box with the files and directories found.
                //
                if(!PopulateFileListBox(true))
                {
                    //
                    // If there were no errors reported, we are ready for
                    // MSC operation.
                    //
                    g_eState = STATE_DEVICE_READY;
                }

                //
                // Set the Device Present flag.
                //
                g_ui32Flags = FLAGS_DEVICE_PRESENT;
                break;
            }

            //
            // If there is no device then just wait for one.
            //
            case STATE_NO_DEVICE:
            {
                if(g_ui32Flags == FLAGS_DEVICE_PRESENT)
                {
                    //
                    // Empty the list box on the display.
                    //
                    ListBoxClear(&g_sDirList);
                    ListBoxTextAdd(&g_sDirList, "Waiting for device...");
                    WidgetPaint((tWidget *)&g_sDirList);

                    //
                    // Clear the Device Present flag.
                    //
                    g_ui32Flags &= ~FLAGS_DEVICE_PRESENT;
                }
                break;
            }

            //
            // An unknown device was connected.
            //
            case STATE_UNKNOWN_DEVICE:
            {
                //
                // If this is a new device then change the status.
                //
                if((g_ui32Flags & FLAGS_DEVICE_PRESENT) == 0)
                {
                    //
                    // Clear the screen and indicate that an unknown device
                    // is present.
                    //
                    ListBoxClear(&g_sDirList);
                    ListBoxTextAdd(&g_sDirList, "Unknown device.");
                    WidgetPaint((tWidget *)&g_sDirList);
                }
                //
                // Set the Device Present flag.
                //
                g_ui32Flags = FLAGS_DEVICE_PRESENT;
                break;
            }

            //
            // The connected mass storage device is not reporting ready.
            //
            case STATE_TIMEOUT_DEVICE:
            {
                //
                // If this is the first time in this state then print a
                // message.
                //
                if((g_ui32Flags & FLAGS_DEVICE_PRESENT) == 0)
                {
                    //
                    // Clear the screen and indicate that an unknown device
                    // is present.
                    //
                    ListBoxClear(&g_sDirList);
                    ListBoxTextAdd(&g_sDirList, "Device Timeout.");
                    WidgetPaint((tWidget *)&g_sDirList);
                }

                //
                // Set the Device Present flag.
                //
                g_ui32Flags = FLAGS_DEVICE_PRESENT;
                break;
            }

            //
            // Something has caused a power fault.
            //
            case STATE_POWER_FAULT:
            {
                break;
            }
            default:
            {
                break;
            }
        }
    }
}
