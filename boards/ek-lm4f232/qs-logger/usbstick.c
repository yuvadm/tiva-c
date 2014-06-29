//*****************************************************************************
//
// usbstick.c - Data logger module to handle USB mass storage.
//
// Copyright (c) 2011-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "driverlib/debug.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "usblib/usblib.h"
#include "usblib/usbmsc.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhmsc.h"
#include "utils/ustdlib.h"
#include "third_party/fatfs/src/ff.h"
#include "qs-logger.h"
#include "usbstick.h"

//*****************************************************************************
//
// This module manages the USB host mass storage function. It is used when
// there is a USB memory stick attached to the evaluation board.  It manages
// the USB connection and stores data log records to the attached mass
// storage device.
//
//*****************************************************************************

//*****************************************************************************
//
// A line of text that is written to the start of a CSV file, to provide
// column headings.
//
//*****************************************************************************
static const char g_pcCSVHeaderLine[] =
    "Time(s),Frac. seconds,"                                                  \
    "CH0(mV),CH1(mV),CH2(mV),CH3(mV),"                                        \
    "AccelX(.01g),AccelY(.01g),AccelZ(.01g),"                                 \
    "Ext. Temp(.1C),Int. Temp(.1C),Current(100uA)\r\n";

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static FIL g_sFileObject;

//*****************************************************************************
//
// Holds global flags for the system.
//
//*****************************************************************************
static volatile uint32_t g_ui32Flags = 0;

//*****************************************************************************
//
// Flag indicating that some USB device is connected.
//
//*****************************************************************************
#define FLAGS_DEVICE_PRESENT    0x00000001
#define FLAGS_FILE_OPENED       0x00000002

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
    eSTATE_NO_DEVICE,

    //
    // Mass storage device is being enumerated.
    //
    eSTATE_DEVICE_ENUM,

    //
    // Mass storage device is ready.
    //
    eSTATE_DEVICE_READY,

    //
    // An unsupported device has been attached.
    //
    eSTATE_UNKNOWN_DEVICE,

    //
    // A power fault has occurred.
    //
    eSTATE_POWER_FAULT
}
g_iState;

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
tDMAControlTable g_psDMAControlTable[6];
#elif defined(ccs)
#pragma DATA_ALIGN(g_psDMAControlTable, 1024)
tDMAControlTable g_psDMAControlTable[6];
#else
tDMAControlTable g_psDMAControlTable[6] __attribute__ ((aligned(1024)));
#endif

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
// \param ui32Instance is the driver instance which is needed when
// communicating with the driver.
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
            g_iState = eSTATE_DEVICE_ENUM;

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
            g_iState = eSTATE_NO_DEVICE;

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
// pvData is actually a pointer to a tEventInfo structure.
//
// This function will be called to inform the application when a USB event has
// occurred that is outside those related to the mass storage device.  At this
// point this is used to detect unsupported devices being inserted and removed.
// It is also used to inform the application when a power fault has occurred.
// This function is required when the g_USBGenericEventDriver is included in
// the host controller driver array that is passed in to the
// USBHCDRegisterDrivers() function.
//
//
//*****************************************************************************
void
USBHCDEvents(void *pvData)
{
    tEventInfo *psEventInfo;

    //
    // Cast this pointer to its actual type.
    //
    psEventInfo = (tEventInfo *)pvData;

    //
    // Process each kind of event
    //
    switch(psEventInfo->ui32Event)
    {
        //
        // An unknown device has been connected.
        //
        case USB_EVENT_UNKNOWN_CONNECTED:
        {
            //
            // An unknown device was detected.
            //
            g_iState = eSTATE_UNKNOWN_DEVICE;
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
            g_iState = eSTATE_NO_DEVICE;
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
            g_iState = eSTATE_POWER_FAULT;
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
// Initializes the USB stack for mass storage.
//
//*****************************************************************************
void
USBStickInit(void)
{
    //
    // Enable the uDMA controller and set up the control table base.
    // The uDMA controller is used by the USB library.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    MAP_uDMAEnable();
    MAP_uDMAControlBaseSet(g_psDMAControlTable);

    //
    // Initially wait for device connection.
    //
    g_iState = eSTATE_NO_DEVICE;

    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ui32NumHostClassDrivers);

    //
    // Open an instance of the mass storage class driver.
    //
    g_psMSCInstance = USBHMSCDriveOpen(0, MSCCallback);

    //
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);

    //
    // Run initial pass through USB host stack.
    //
    USBHCDMain();

    //
    // Initialize the file system.
    //
    FileInit();
}

//*****************************************************************************
//
// This is called by the application main loop to perform regular processing.
// It keeps the USB host stack running and tracks the state of the attached
// device.
//
//*****************************************************************************
void
USBStickRun(void)
{
    //
    // Call the USB stack to keep it running.
    //
    USBHCDMain();

    //
    // Take action based on the application state.
    //
    switch(g_iState)
    {
        //
        // A device has enumerated.
        //
        case eSTATE_DEVICE_ENUM:
        {
            //
            // Check to see if the device is ready.  If not then stay
            // in this state and we will check it again on the next pass.
            //
            if(USBHMSCDriveReady(g_psMSCInstance) != 0)
            {
                //
                // Wait about 500ms before attempting to check if the
                // device is ready again.
                //
                MAP_SysCtlDelay(MAP_SysCtlClockGet()/3);
                break;
            }

            //
            // If there were no errors reported, we are ready for
            // MSC operation.
            //
            g_iState = eSTATE_DEVICE_READY;

            //
            // Set the Device Present flag.
            //
            g_ui32Flags = FLAGS_DEVICE_PRESENT;

            break;
        }

        //
        // If there is no device then just wait for one.
        //
        case eSTATE_NO_DEVICE:
        {
            if(g_ui32Flags == FLAGS_DEVICE_PRESENT)
            {
                //
                // Clear the Device Present flag.
                //
                g_ui32Flags &= ~(FLAGS_DEVICE_PRESENT | FLAGS_FILE_OPENED);
            }
            break;
        }

        //
        // An unknown device was connected.
        //
        case eSTATE_UNKNOWN_DEVICE:
        {
            //
            // If this is a new device then change the status.
            //
            if((g_ui32Flags & FLAGS_DEVICE_PRESENT) == 0)
            {
                //
                // Unknown device is present
                //
            }

            //
            // Set the Device Present flag even though the unknown device
            // is not useful to us.
            //
            g_ui32Flags = FLAGS_DEVICE_PRESENT;
            break;
        }

        //
        // The device is ready and in use.
        //
        case eSTATE_DEVICE_READY:
        {
            break;
        }

        //
        // Something has caused a power fault.
        //
        case eSTATE_POWER_FAULT:
        {
            break;
        }

        //
        // Unexpected USB state.  Set back to default.
        //
        default:
        {
            g_iState = eSTATE_NO_DEVICE;
            g_ui32Flags &= ~(FLAGS_DEVICE_PRESENT | FLAGS_FILE_OPENED);
            break;
        }
    }
}

//*****************************************************************************
//
// This is called when the data logging is stopped.  It performs a sync
// to the file system which should flush any buffered data to the storage
// device.
//
//*****************************************************************************
void
USBStickCloseFile(void)
{
    f_close(&g_sFileObject);
    g_ui32Flags &= ~FLAGS_FILE_OPENED;
}

//*****************************************************************************
//
// Create a file name for the file to be saved on the memory stick.
// This function uses an incrementing numerical search scheme to determine
// an available file name.  It tries opening file names in succession until
// it finds a file that does not yet exist.
// The file name will be of the form LOGnnnn.CSV.
// The caller supplies storage for the file name through the pcFilename
// parameter.
// The function will return 0 if successful and non-zero if a file name could
// not be found.
//
//*****************************************************************************
static int32_t
CreateFileName(char *pcFilename, uint32_t ui32Len)
{
    FRESULT iFResult;
    uint32_t ui32FileNum = 0;

    //
    // Enter loop to search for available file name
    //
    do
    {
        //
        // Prepare a numerical based file name and attempt to open it
        //
        usnprintf(pcFilename, ui32Len, "LOG%04d.CSV", ui32FileNum);
        iFResult = f_open(&g_sFileObject, pcFilename, FA_OPEN_EXISTING);

        //
        // If file does not exist, then we have found a useable file name
        //
        if(iFResult == FR_NO_FILE)
        {
            //
            // Return to caller, indicating that a file name has been found.
            //
            return(0);
        }

        //
        // Otherwise, advance to the next number in the file name sequence.
        //
        ui32FileNum++;

    } while(ui32FileNum < 1000);

    //
    // If we reach this point, it means that no useable file name was found
    // after attempting 10000 file names.
    //
    return(1);
}

//*****************************************************************************
//
// This is called at the start of logging to open a file on the storage
// device in preparation for saving data.  If no file name is specified, then
// a new file will be created.
//
// If a file name is specified, then that will be used instead of searching
// for an available file.  The file name that is passed in must be a maximum
// of 8 characters (9 including trailing 0) and represents the first part of
// the file name not including the extension.
//
// The function returns a pointer to the first part of the file name
// (without file extension).  It can be up to 8 characters (9 including the
// trailing 0).  If there is any error then a NULL pointer is returned.
//
//*****************************************************************************
char *
USBStickOpenLogFile(char *pcFilename8)
{
    FRESULT iFResult;
    uint32_t ui32BytesWritten;
    static char pcFilename[16];
    uint32_t ui32Len;

    //
    // Check state for ready device
    //
    g_ui32Flags &= ~FLAGS_FILE_OPENED;
    if(g_iState == eSTATE_DEVICE_READY)
    {
        //
        // If a file name is specified then open that file
        //
        if(pcFilename8 && pcFilename8[0])
        {
            //
            // Copy the filename into local storage and cap at 8 characters
            // length.
            //
            memcpy(pcFilename, pcFilename8, 8);
            pcFilename[8] = 0;

            //
            // Find the actual length of the string (8 chars or less) so we
            // know where to add the extension.
            //
            ui32Len = strlen(pcFilename);

            //
            // Add the extension to the file name.
            //
            usnprintf(&pcFilename[ui32Len], 5, ".CSV");
        }

        //
        // Otherwise no file name was specified so create a new one.
        //
        else
        {
            if(CreateFileName(pcFilename, sizeof(pcFilename)))
            {
                //
                // There was a problem creating a file name so return an error
                //
                return(0);
            }
        }

        //
        // Open the file by name that was determined above.  If the file exists
        // it will be opened, and if not it will be created.
        //
        iFResult = f_open(&g_sFileObject, pcFilename, (FA_OPEN_ALWAYS |
                                                      FA_WRITE));
        if(iFResult != FR_OK)
        {
            return(0);
        }

        //
        // Since it is possible that the file already existed when opened,
        // seek to the end of the file so new data will be appended.  If this
        // is a new file then this will just be the beginning of the file.
        //
        iFResult = f_lseek(&g_sFileObject, g_sFileObject.fsize);
        if(iFResult != FR_OK)
        {
            return(0);
        }

        //
        // Set flag to indicate file is now opened.
        //
        g_ui32Flags |= FLAGS_FILE_OPENED;

        //
        // If no file name was specified, then this is a new file so write a
        // header line with column titles to the CSV file.
        //
        if(!pcFilename8 || !pcFilename8[0])
        {
            //
            // Write a header line to the CSV file
            //
            iFResult = f_write(&g_sFileObject, g_pcCSVHeaderLine,
                              sizeof(g_pcCSVHeaderLine), &ui32BytesWritten);
            if(iFResult != FR_OK)
            {
                g_ui32Flags &= ~FLAGS_FILE_OPENED;
                return(0);
            }

            //
            // Since no file name was specified that means a file name was
            // created.  Terminate the new file name at the '.' separator
            // and return it to the caller.  We know that created file names
            // are always 7 characters.  Return the newly created file name
            // (the part before the '.')
            //
            pcFilename[7] = 0;
            return(pcFilename);
        }

        //
        // Otherwise, a file name was specified, so no need to write a
        // header row.  The caller's file name is unchanged so return the
        // same value back.
        //
        else
        {
            return(pcFilename8);
        }
    }

    else
    {
        //
        // Device not ready so return NULL.
        //
        return(0);
    }
}

//*****************************************************************************
//
// This is called each time there is a new data record to log to the storage
// device.  A line of text in CSV format will be written to the file.
//
//*****************************************************************************
int32_t
USBStickWriteRecord(tLogRecord *psRecord)
{
    static char pcBuf[256];
    uint32_t ui32Idx, ui32BufIdx = 0, ui32RecordIdx, ui32Selected;
    FRESULT iFResult;
    uint32_t ui32BytesWritten;

    //
    // Check the arguments
    //
    ASSERT(psRecord);
    if(!psRecord)
    {
        return(1);
    }

    //
    // Check state for ready device and opened file
    //
    if((g_iState != eSTATE_DEVICE_READY) || !(g_ui32Flags & FLAGS_FILE_OPENED))
    {
        return(1);
    }

    //
    // Print time stamp columns
    //
    ui32BufIdx += usnprintf(&pcBuf[ui32BufIdx], sizeof(pcBuf) - ui32BufIdx,
                            "%u,%u", psRecord->ui32Seconds,
                            psRecord->ui16Subseconds);

    //
    // Iterate through selected data items and print to CSV buffer
    //
    ui32RecordIdx = 0;
    ui32Selected = psRecord->ui16ItemMask;
    for(ui32Idx = 0; ui32Idx < NUM_LOG_ITEMS; ui32Idx++)
    {
        //
        // If this data item is selected, then print a value to the CSV buffer
        //
        if(ui32Selected & 1)
        {
            ui32BufIdx += usnprintf(&pcBuf[ui32BufIdx],
                                    (sizeof(pcBuf) - ui32BufIdx), ",%d",
                                    psRecord->pi16Items[ui32RecordIdx]);
            ui32RecordIdx++;
        }
        else
        {
            //
            // Otherwise, this column of data is not selected so emit just a
            // comma
            //
            ui32BufIdx += usnprintf(&pcBuf[ui32BufIdx],
                                    (sizeof(pcBuf) - ui32BufIdx), ",");
        }

        //
        // Next selected item ...
        //
        ui32Selected >>= 1;
    }

    //
    // Append a CRLF to the end
    //
    ui32BufIdx += usnprintf(&pcBuf[ui32BufIdx], (sizeof(pcBuf) - ui32BufIdx),
                            "\r\n");

    //
    // Now write the entire buffer to the USB stick file
    //
    iFResult = f_write(&g_sFileObject, pcBuf, ui32BufIdx, &ui32BytesWritten);

    //
    // Check for errors
    //
    if((iFResult != FR_OK) || (ui32BytesWritten != ui32BufIdx))
    {
        //
        // Some error occurred
        //
        g_ui32Flags &= ~FLAGS_FILE_OPENED;
        return(1);
    }
    else
    {
        //
        // No errors occurred, return success
        //
        return(0);
    }
}
