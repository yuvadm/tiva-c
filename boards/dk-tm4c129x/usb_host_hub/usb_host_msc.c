//*****************************************************************************
//
// usb_host_msc.c - The USB Mass storage handling routines.
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
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "utils/ustdlib.h"
#include "usblib/usblib.h"
#include "usblib/usbmsc.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhmsc.h"
#include "third_party/fatfs/src/ff.h"
#include "third_party/fatfs/src/diskio.h"
#include "usb_host_hub.h"

//*****************************************************************************
//
// A structure that holds a mapping between an FRESULT numerical code,
// and a string representation.  FRESULT codes are returned from the FatFs
// FAT file system driver.
//
//*****************************************************************************
typedef struct
{
    FRESULT iResult;
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
tFresultString g_sFresultStrings[] =
{
    FRESULT_ENTRY(FR_OK),
    FRESULT_ENTRY(FR_NOT_READY),
    FRESULT_ENTRY(FR_NO_FILE),
    FRESULT_ENTRY(FR_NO_PATH),
    FRESULT_ENTRY(FR_INVALID_NAME),
    FRESULT_ENTRY(FR_INVALID_DRIVE),
    FRESULT_ENTRY(FR_DENIED),
    FRESULT_ENTRY(FR_EXIST),
    FRESULT_ENTRY(FR_INVALID_OBJECT),
    FRESULT_ENTRY(FR_WRITE_PROTECTED),
    FRESULT_ENTRY(FR_NOT_ENABLED),
    FRESULT_ENTRY(FR_NO_FILESYSTEM),
    FRESULT_ENTRY(FR_INVALID_OBJECT),
    FRESULT_ENTRY(FR_MKFS_ABORTED)
};

//*****************************************************************************
//
// A macro that holds the number of result codes.
//
//*****************************************************************************
#define NUM_FRESULT_CODES (sizeof(g_sFresultStrings) / sizeof(tFresultString))

//*****************************************************************************
//
// Defines the size of the buffers that hold the path, or temporary
// data from the USB disk.  There are two buffers allocated of this size.
// The buffer size must be large enough to hold the longest expected
// full path name, including the file name, and a trailing null character.
//
//*****************************************************************************
#define PATH_BUF_SIZE   80

//*****************************************************************************
//
// This buffer holds the full path to the current working directory.
// Initially it is root ("/").
//
//*****************************************************************************
static char g_pcCwdBuf[PATH_BUF_SIZE] = "/";

//*****************************************************************************
//
// A temporary data buffer used when manipulating file paths, or reading data
// from the SD card.
//
//*****************************************************************************
static char g_pcTmpBuf[PATH_BUF_SIZE];

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;
static FIL g_sFileObject;
static uint32_t g_ui32Clock;

//*****************************************************************************
//
// Error reasons returned by ChangeDirectory().
//
//*****************************************************************************
#define NAME_TOO_LONG_ERROR     1
#define OPENDIR_ERROR           2

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
    eStateNoDevice,

    //
    // Mass storage device is being enumerated.
    //
    eStateDeviceEnum,

    //
    // Mass storage device is ready.
    //
    eStateDeviceReady,

    //
    // A mass storage device was connected but failed to ever report ready.
    //
    eStateDeviceTimeout,
}
g_iState;

//*****************************************************************************
//
// The instance data for the MSC driver.
//
//*****************************************************************************
tUSBHMSCInstance *g_psMSCInstance;

//*****************************************************************************
//
// The instance data for the MSC driver.
//
//*****************************************************************************
uint32_t g_ui32DriveTimeout;

//*****************************************************************************
//
// This function initializes the third party FAT implementation.
//
// Returns true on success or false on failure.
//
//*****************************************************************************
bool
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
// This function returns a string representation of an error code
// that was returned from a function call to FatFs.  It can be used
// for printing human readable error messages.
//
//*****************************************************************************
const char *
StringFromFresult(FRESULT iResult)
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
        if(g_sFresultStrings[ui32Idx].iResult == iResult)
        {
            return(g_sFresultStrings[ui32Idx].pcResultStr);
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
// This function implements the "ls" command.  It opens the current
// directory and enumerates through the contents, and prints a line for
// each item it finds.  It shows details such as file attributes, time and
// date, and the file size, along with the name.  It shows a summary of
// file sizes at the end along with free space.
//
//*****************************************************************************
int
Cmd_ls(int argc, char *argv[])
{
    uint32_t ui32TotalSize, ui32ItemCount, ui32FileCount, ui32DirCount;
    FRESULT iResult;
    FATFS *psFatFs;

    //
    // Open the current directory for access.
    //
    iResult = f_opendir(&g_sDirObject, g_pcCwdBuf);

    //
    // Check for error and return if there is a problem.
    //
    if(iResult != FR_OK)
    {
        //
        // Ensure that the error is reported.
        //
        WriteString("Error from file system:");
        WriteString(StringFromFresult(iResult));
        WriteString("\n");
        return(iResult);
    }

    ui32TotalSize = 0;
    ui32FileCount = 0;
    ui32DirCount = 0;
    ui32ItemCount = 0;

    //
    // Enter loop to enumerate through all directory entries.
    //
    for(;;)
    {
        //
        // Read an entry from the directory.
        //
        iResult = f_readdir(&g_sDirObject, &g_sFileInfo);

        //
        // Check for error and return if there is a problem.
        //
        if(iResult != FR_OK)
        {
            return(iResult);
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
        // Print the entry information on a single line with formatting
        // to show the attributes, date, time, size, and name.
        //
        usprintf(g_pcTmpBuf, "%c%c%c%c%c %u/%02u/%02u %02u:%02u %9u  %s\n",
                 (g_sFileInfo.fattrib & AM_DIR) ? 'D' : '-',
                 (g_sFileInfo.fattrib & AM_RDO) ? 'R' : '-',
                 (g_sFileInfo.fattrib & AM_HID) ? 'H' : '-',
                 (g_sFileInfo.fattrib & AM_SYS) ? 'S' : '-',
                 (g_sFileInfo.fattrib & AM_ARC) ? 'A' : '-',
                 (g_sFileInfo.fdate >> 9) + 1980,
                 (g_sFileInfo.fdate >> 5) & 15,
                 g_sFileInfo.fdate & 31,
                 (g_sFileInfo.ftime >> 11),
                 (g_sFileInfo.ftime >> 5) & 63,
                 g_sFileInfo.fsize,
                 g_sFileInfo.fname);

        WriteString(g_pcTmpBuf);

        //
        // If the attribute is directory, then increment the directory count.
        //
        if(g_sFileInfo.fattrib & AM_DIR)
        {
            ui32DirCount++;
        }

        //
        // Otherwise, it is a file.  Increment the file count, and
        // add in the file size to the total.
        //
        else
        {
            ui32FileCount++;
            ui32TotalSize += g_sFileInfo.fsize;
        }

        //
        // Move to the next entry in the item array we use to populate the
        // list box.
        //
        ui32ItemCount++;
    }

    //
    // Print summary lines showing the file, dir, and size totals.
    //
    usprintf(g_pcTmpBuf, "\n%4u File(s),%10u bytes total\n", ui32FileCount,
             ui32TotalSize);

    WriteString(g_pcTmpBuf);

    //
    // Get the free space.
    //
    iResult = f_getfree("/", (DWORD *)&ui32TotalSize, &psFatFs);

    //
    // Check for error and return if there is a problem.
    //
    if(iResult != FR_OK)
    {
        return(iResult);
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
// It does not understand relative paths, so do not try something like this:
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
ChangeToDirectory(char *pcDirectory, uint32_t *pui32Reason)
{
    uint32_t ui32Idx;
    FRESULT iResult;

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
            *pui32Reason = NAME_TOO_LONG_ERROR;
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
        if(strlen(g_pcTmpBuf) + strlen(pcDirectory) + 1 + 1 > sizeof(g_pcCwdBuf))
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
    // At this point, a candidate new directory path is in chTmpBuf.
    // Try to open it to make sure it is valid.
    //
    iResult = f_opendir(&g_sDirObject, g_pcTmpBuf);

    //
    // If it cannot be opened, then it is a bad path.  Inform
    // user and return.
    //
    if(iResult != FR_OK)
    {
        *pui32Reason = OPENDIR_ERROR;
        return(iResult);
    }

    //
    // Otherwise, it is a valid new path, so copy it into the CWD and update
    // the screen.
    //
    else
    {
        strncpy(g_pcCwdBuf, g_pcTmpBuf, sizeof(g_pcCwdBuf));
    }

    //
    // Return success.
    //
    return(FR_OK);
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
// It does not understand relative paths, so don't try something like this:
// ("../my/new/path")
//
// Once the new directory is specified, it attempts to open the directory
// to make sure it exists.  If the new path is opened successfully, then
// the current working directory (cwd) is changed to the new path.
//
//*****************************************************************************
int
Cmd_cd(int argc, char *argv[])
{
    uint32_t ui32Reason;
    FRESULT iResult;

    //
    // Try to change to the directory provided on the command line.
    //
    iResult = ChangeToDirectory(argv[1], &ui32Reason);

    //
    // If an error was reported, try to offer some helpful information.
    //
    if(iResult != FR_OK)
    {
        switch(ui32Reason)
        {
            case OPENDIR_ERROR:
                WriteString("Error opening new directory.\n");
                break;

            case NAME_TOO_LONG_ERROR:
                WriteString("Resulting path name is too long.\n");
                break;

            default:
                WriteString("An unrecognized error was reported.\n");
                break;
        }
    }
    else
    {
        //
        // Tell the user what happened.
        //
        WriteString("Changed to ");
        WriteString(g_pcCwdBuf);
        WriteString("\n");
    }

    //
    // Return the appropriate error code.
    //
    return(iResult);
}

//*****************************************************************************
//
// This function implements the "pwd" command.  It simply prints the
// current working directory.
//
//*****************************************************************************
int
Cmd_pwd(int argc, char *argv[])
{
    //
    // Print the CWD to the console.
    //
    WriteString(g_pcCwdBuf);
    WriteString("\n");

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// This function implements the "cat" command.  It reads the contents of
// a file and prints it to the console.  This should only be used on
// text files.  If it is used on a binary file, then a bunch of garbage
// is likely to printed on the console.
//
//*****************************************************************************
int
Cmd_cat(int argc, char *argv[])
{
    FRESULT iResult;
    uint32_t ui32BytesRead;
    int iIdx;
    char *pcCurrent;

    //
    // First, check to make sure that the current path (CWD), plus
    // the file name, plus a separator and trailing null, will all
    // fit in the temporary buffer that will be used to hold the
    // file name.  The file name must be fully specified, with path,
    // to FatFs.
    //
    if(strlen(g_pcCwdBuf) + strlen(argv[1]) + 1 + 1 > sizeof(g_pcTmpBuf))
    {
        WriteString("Resulting path name is too long\n");
        return(0);
    }

    //
    // Copy the current path to the temporary buffer so it can be manipulated.
    //
    strcpy(g_pcTmpBuf, g_pcCwdBuf);

    //
    // If not already at the root level, then append a separator.
    //
    if(strcmp("/", g_pcCwdBuf))
    {
        strcat(g_pcTmpBuf, "/");
    }

    //
    // Now finally, append the file name to result in a fully specified file.
    //
    strcat(g_pcTmpBuf, argv[1]);

    //
    // Open the file for reading.
    //
    iResult = f_open(&g_sFileObject, g_pcTmpBuf, FA_READ);

    //
    // If there was some problem opening the file, then return
    // an error.
    //
    if(iResult != FR_OK)
    {
        return(iResult);
    }

    //
    // Enter a loop to repeatedly read data from the file and display it,
    // until the end of the file is reached.
    //
    do
    {
        //
        // Read a block of data from the file.  Read as much as can fit
        // in the temporary buffer, including a space for the trailing null.
        //
        iResult = f_read(&g_sFileObject, g_pcTmpBuf, sizeof(g_pcTmpBuf) - 1,
                         (UINT *)&ui32BytesRead);

        //
        // If there was an error reading, then print a newline and
        // return the error to the user.
        //
        if(iResult != FR_OK)
        {
            WriteString("\n");
            return(iResult);
        }

        //
        // Null terminate the last block that was read to make it a
        // null terminated string that can be used with printing.
        //
        g_pcTmpBuf[ui32BytesRead] = 0;

        pcCurrent = g_pcTmpBuf;

        for(iIdx = 0; iIdx < ui32BytesRead; iIdx++)
        {
            if(g_pcTmpBuf[iIdx] == '\r')
            {
                //
                // Ignore carriage return.
                //
                g_pcTmpBuf[iIdx] = 0;
            }
            else if(g_pcTmpBuf[iIdx] == '\n')
            {
                g_pcTmpBuf[iIdx] = 0;

                //
                // Print the current line in the file.
                //
                WriteString(pcCurrent);
                WriteString("\n");

                //
                // Move the pointer up to the next line.
                //
                pcCurrent = g_pcTmpBuf + iIdx + 1;
            }
            else if(g_pcTmpBuf[iIdx] == 0)
            {
                //
                // Print the current string and move past the null.
                //
                WriteString(pcCurrent);
                pcCurrent = g_pcTmpBuf + iIdx + 1;
            }
        }

        if(pcCurrent < g_pcTmpBuf + ui32BytesRead)
        {
            //
            // Null terminate the line.
            //
            g_pcTmpBuf[ui32BytesRead] = 0;

            //
            // Print any remaining characters before reading a new line.
            //
            WriteString(pcCurrent);
        }
    }
    while(ui32BytesRead == sizeof(g_pcTmpBuf) - 1);

    WriteString("\n");

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// This is the callback from the MSC driver.
//
// ulInstance is the driver instance which is needed when communicating with
// the driver.
// ulEvent is one of the events defined by the driver.
// pvData is a pointer to data passed into the initial call to register
// the callback.
//
// This function handles callback events from the MSC driver.  The only events
// currently handled are the MSC_EVENT_OPEN and MSC_EVENT_CLOSE.  This allows
// the main routine to know when an MSC device has been detected and
// enumerated and when an MSC device has been removed from the system.
//
// This function returns no values.
//
//*****************************************************************************
void
MSCCallback(tUSBHMSCInstance *psMSCInstance, uint32_t ui32Event, void *pvData)
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
            g_iState = eStateDeviceEnum;

            break;
        }

        //
        // Called when the device driver has been unloaded due to error or
        // the device is no longer present.
        //
        case MSC_EVENT_CLOSE:
        {
            //
            // Go back to the "no device" state and wait for a new connection.
            //
            g_iState = eStateNoDevice;

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
// Prepares an instance of the USB MSC class to handle a USB flash drive.
//
//*****************************************************************************
void
MSCOpen(uint32_t ui32Clock)
{
    //
    // Save the processor clock.
    //
    g_ui32Clock = ui32Clock;

    //
    // Open an instance of the mass storage class driver.
    //
    g_psMSCInstance = USBHMSCDriveOpen(0, MSCCallback);
}

//*****************************************************************************
//
// The main routine for handling the USB mass storage device.
//
//*****************************************************************************
void
MSCMain(void)
{
    FRESULT iResult;

    switch(g_iState)
    {
        case eStateDeviceEnum:
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
                SysCtlDelay(g_ui32Clock/(3*2));

                //
                // Decrement the retry count.
                //
                g_ui32DriveTimeout--;

                //
                // If the timeout is hit then go to the
                // eStateDeviceTimeout state.
                //
                if(g_ui32DriveTimeout == 0)
                {
                    g_iState = eStateDeviceTimeout;
                }

                break;
            }

            //
            // Reset the root directory.
            //
            g_pcCwdBuf[0] = '/';
            g_pcCwdBuf[1] = 0;

            //
            // Open the current directory for access.
            //
            iResult = f_opendir(&g_sDirObject, g_pcCwdBuf);

            //
            // Check for error and return if there is a problem.
            //
            if(iResult != FR_OK)
            {
                //
                // Ensure that the error is reported.
                //
                WriteString("Error from USB disk:");
                WriteString((char *)StringFromFresult(iResult));
                WriteString("\n");
                return;
            }

            g_iState = eStateDeviceReady;

            break;
        }

        //
        // The connected mass storage device is not reporting ready.
        //
        case eStateDeviceTimeout:
        {
            WriteString("\n");
            WriteString("Device Timeout.\n");
            break;
        }
        case eStateNoDevice:
        case eStateDeviceReady:
        default:
        {
            break;
        }
    }
}
