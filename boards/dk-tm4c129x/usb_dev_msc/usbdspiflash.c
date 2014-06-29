//*****************************************************************************
//
// usbdspiflash.c - Routines supplied for use by the mass storage class device
//                  class.
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
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdmsc.h"
#include "usb_msc_structs.h"
#include "drivers/mx66l51235f.h"

#define SPIFLASH_IN_USE         0x00000001

struct
{
    uint32_t ulFlags;
}
g_sDriveInformation;

//*****************************************************************************
//
// The number of bytes that have been read from the SPI flash.
//
//*****************************************************************************
uint32_t g_ui32ReadCount;

//*****************************************************************************
//
// The number of bytes that have been written to the SPI flash.
//
//*****************************************************************************
uint32_t g_ui32WriteCount;

//*****************************************************************************
//
// This function opens the drive number and prepares it for use by the Mass
// storage class device.
//
// \param ulDrive is the driver number to open.
//
// This function is used to initialize and open the physical drive number
// associated with the parameter \e ulDrive.  The function will return zero if
// the drive has already been opened.
//
// \return Returns a pointer to data that should be passed to other APIs or it
// will return 0 if no drive was found.
//
//*****************************************************************************
void *
USBDMSCStorageOpen(uint32_t ulDrive)
{
    ASSERT(ulDrive == 0);

    //
    // Return if already in use.
    //
    if(g_sDriveInformation.ulFlags & SPIFLASH_IN_USE)
    {
        return(0);
    }

    //
    // Flash is in use.
    //
    g_sDriveInformation.ulFlags = SPIFLASH_IN_USE;

    return((void *)&g_sDriveInformation);
}

//*****************************************************************************
//
// This function close the drive number in use by the mass storage class device.
//
// \param pvDrive is the pointer that was returned from a call to
// USBDMSCStorageOpen().
//
// This function is used to close the physical drive number associated with the
// parameter \e pvDrive.  This function will return 0 if the drive was closed
// successfully and any other value will indicate a failure.
//
// \return Returns 0 if the drive was successfully closed or non-zero for a
// failure.
//
//*****************************************************************************
void
USBDMSCStorageClose(void *pvDrive)
{
    ASSERT(pvDrive != 0);

    //
    // Clear all flags.
    //
    g_sDriveInformation.ulFlags = 0;

}

//*****************************************************************************
//
// This function will read a block from a device opened by the
// USBDMSCStorageOpen() call.
//
// \param pvDrive is the pointer that was returned from a call to
// USBDMSCStorageOpen().
// \param pucData is the buffer that data will be written into.
// \param ui32NumBlocks is the number of blocks to read.
//
// This function is use to read blocks from a physical device and return them
// in the \e pucData buffer.  The data area pointed to by \e pucData should be
// at least \e ui32NumBlocks * Block Size bytes to prevent overwriting data.
//
// \return Returns the number of bytes that were read from the device.
//
//*****************************************************************************
uint32_t
USBDMSCStorageRead(void *pvDrive, uint8_t *pui8Data, uint32_t ui32Sector,
                   uint32_t ui32NumBlocks)
{
    ASSERT(pvDrive != 0);

    g_ui32ReadCount += ui32NumBlocks * MX66L51235F_BLOCK_SIZE;

    MX66L51235FRead(ui32Sector * MX66L51235F_BLOCK_SIZE, pui8Data, 
                    ui32NumBlocks * MX66L51235F_BLOCK_SIZE);

    return(ui32NumBlocks * MX66L51235F_BLOCK_SIZE);
}

//*****************************************************************************
//
// This function will write a block to a device opened by the
// USBDMSCStorageOpen() call.
//
// \param pvDrive is the pointer that was returned from a call to
// USBDMSCStorageOpen().
// \param pucData is the buffer that data will be used for writing.
// \param ui32NumBlocks is the number of blocks to write.
//
// This function is use to write blocks to a physical device from the buffer
// pointed to by the \e pucData buffer.  If the number of blocks is greater
// than one then the block address will increment and write to the next block
// until \e ui32NumBlocks * Block Size bytes have been written.
//
// \return Returns the number of bytes that were written to the device.
//
//*****************************************************************************
uint32_t
USBDMSCStorageWrite(void *pvDrive, uint8_t *pui8Data, uint32_t ui32Sector,
                    uint32_t ui32NumBlocks)
{
    uint32_t ui32Idx, ui32BlockAddr;
    uint32_t ui32PageIdx;

    ASSERT(pvDrive != 0);

    g_ui32WriteCount += ui32NumBlocks * MX66L51235F_BLOCK_SIZE;

    for(ui32Idx = 0; ui32Idx < ui32NumBlocks; ui32Idx++)
    {
        //
        // each block is 4K(0x1000) bytes
        //
        ui32BlockAddr = (ui32Sector + ui32Idx) * MX66L51235F_BLOCK_SIZE;

        //
        // erase the block
        // 
        MX66L51235FSectorErase(ui32BlockAddr);

        //
        // program the block one page(256 bytes) a time
        // 
        for(ui32PageIdx = 0; ui32PageIdx < (MX66L51235F_BLOCK_SIZE / 256);
            ui32PageIdx++)
        {
            MX66L51235FPageProgram((ui32BlockAddr + (ui32PageIdx * 256)), 
                                   (pui8Data +
                                    (ui32Idx * MX66L51235F_BLOCK_SIZE) +
                                    (ui32PageIdx * 256)), 256);
        }
    }

    return(ui32NumBlocks * MX66L51235F_BLOCK_SIZE);
}

//*****************************************************************************
//
// This function will return the number of blocks present on a device.
//
// \param pvDrive is the pointer that was returned from a call to
// USBDMSCStorageOpen().
//
// This function is used to return the total number of blocks on a physical
// device based on the \e pvDrive parameter.
//
// \return Returns the number of blocks that are present in a device.
//
//*****************************************************************************
uint32_t
USBDMSCStorageNumBlocks(void *pvDrive)
{
    //
    // return number of blocks.
    //
    return(MX66L51235F_MEMORY_SIZE / MX66L51235F_BLOCK_SIZE);
}

//*****************************************************************************
//
// This function will return the block size on a device.
//
// \param pvDrive is the pointer that was returned from a call to
// USBDMSCStorageOpen().
//
// This function is used to return the block size on a physical
// device based on the \e pvDrive parameter.
//
// \return Returns the block size for a device.
//
//*****************************************************************************
uint32_t
USBDMSCStorageBlockSize(void *pvDrive)
{
    //
    // return block size.
    //
    return(MX66L51235F_BLOCK_SIZE);
}
