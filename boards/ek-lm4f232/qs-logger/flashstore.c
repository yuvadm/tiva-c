//*****************************************************************************
//
// flashstore.c - Data logger module to handle storage in flash.
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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/flash.h"
#include "utils/ustdlib.h"
#include "qs-logger.h"
#include "usbstick.h"
#include "flashstore.h"

//*****************************************************************************
//
// This module manages the storage of data logger data into flash memory.
//
//*****************************************************************************

//*****************************************************************************
//
// Define the beginning and end of the flash storage area.  You must make sure
// that this area is well clear of any space occupied by the application
// binary, and that this space is not used for any other purpose.
// The start and end addresses must be 1K aligned.  The end address is
// exclusive - it is 1 value greater than the last valid location used for
// storage.
//
//*****************************************************************************
#define FLASH_STORE_START_ADDR  0x20000
#define FLASH_STORE_END_ADDR    0x40000

//*****************************************************************************
//
// The next address in flash, that will be used for storing a data record.
//
//*****************************************************************************
static uint32_t g_ui32StoreAddr;

//*****************************************************************************
//
// A buffer used to assemble a complete record of data prior to storing it
// in the flash.
//
//*****************************************************************************
static uint32_t g_pui32RecordBuf[32];

//*****************************************************************************
//
// Initializes the flash storage. This is a stub because there is nothing
// special to do.
//
//*****************************************************************************
void
FlashStoreInit(void)
{
}

//*****************************************************************************
//
// Saves data records that are stored in the flash to an externally connected
// USB memory storage device (USB stick).
// The flash memory is scanned for the presence of store data records.  When
// records are found they are written in CSV format to the USB stick.  This
// function assumes a non-corrupted storage area, and that any records, once
// found, are contiguous with all stored records.  It will find the oldest
// record and start with that when storing.
//
//*****************************************************************************
int32_t
FlashStoreSave(void)
{
    uint32_t ui32Addr, ui32OldestRecord, ui32OldestSeconds, ui32Count,
             ui32PartialCount;
    tLogRecord *psRecord;

    //
    // Initialize locals.
    //
    ui32OldestRecord = FLASH_STORE_START_ADDR;
    ui32OldestSeconds = 0xFFFFFFFF;

    //
    // Show a message to the user.
    //
    SetStatusText("SAVE", "SCANNING", "FLASH", 0);

    //
    // Start at beginning of flash storage area
    //
    ui32Addr = FLASH_STORE_START_ADDR;

    //
    // Search all of flash area checking every stored record.
    //
    while(ui32Addr < FLASH_STORE_END_ADDR)
    {
        //
        // If a record signature is found, check for oldest record, then
        // increment to the next record
        //
        if((HWREG(ui32Addr) & 0xFFFFFF00) == 0x53554100)
        {
            //
            // Get a pointer to the data record (account for flash header word)
            //
            psRecord = (tLogRecord *)(ui32Addr + 4);

            //
            // If the seconds in this record are older than any found so far
            // then save the seconds value, and the address of this record
            //
            if(psRecord->ui32Seconds < ui32OldestSeconds)
            {
                ui32OldestSeconds = psRecord->ui32Seconds;
                ui32OldestRecord = ui32Addr;
            }

            //
            // Advance the address to the next record.
            //
            ui32Addr += HWREG(ui32Addr) & 0xFF;
        }
        else
        {
            //
            // A record was not found so just advance to the next location in
            // flash
            //
            ui32Addr += 4;
        }
    }

    //
    // If no "oldest" seconds was found, then there is no valid data stored
    //
    if(ui32OldestSeconds == 0xFFFFFFFF)
    {
        SetStatusText("SAVE", "NO RECORDS", "FOUND", "PRESS <");
        return(1);
    }

    //
    // Open the output file on the USB stick.  It will return NULL if there
    // was any problem.
    //
    if(!USBStickOpenLogFile(0))
    {
        SetStatusText("SAVE", 0, "USB ERROR", "PRESS <");
        return(1);
    }

    //
    // Notify user we are saving data to USB
    //
    SetStatusText("SAVE", "SAVING", "TO USB", 0);

    //
    // Start reading records from flash, start at the address of the oldest
    // record, as found above.  We scan through records, assuming the flash
    // store is not corrupted.  Continue scanning until a blank space is
    // found which should indicate the end of recorded data, or until we
    // have read all the records.
    //
    ui32Addr = ui32OldestRecord;
    while(HWREG(ui32Addr) != 0xFFFFFFFF)
    {
        //
        // If a record signature is found (which it should be), extract the
        // record data and send it to USB stick.
        //
        if((HWREG(ui32Addr) & 0xFFFFFF00) == 0x53554100)
        {
            //
            // Get byte count for this record
            //
            ui32Count = HWREG(ui32Addr) & 0xFF;

            //
            // Adjust the count and the address to remove the flash header
            //
            ui32Count -= 4;
            ui32Addr += 4;

            //
            // Adjust for memory wrap
            //
            if(ui32Addr >= FLASH_STORE_END_ADDR)
            {
                ui32Addr = FLASH_STORE_START_ADDR;
            }

            //
            // If the contents of this record go past the end of the memory
            // storage area, then perform a partial copy first.
            //
            ui32PartialCount = 0;
            if((ui32Addr + ui32Count) >= FLASH_STORE_END_ADDR)
            {
                //
                // Find how many bytes are left on this page
                //
                ui32PartialCount = FLASH_STORE_END_ADDR - ui32Addr;

                //
                // Copy the portion until the end of memory store, adjust
                // remaining count and address
                //
                memcpy(g_pui32RecordBuf, (void *)ui32Addr, ui32PartialCount);
                ui32Count -= ui32PartialCount;
                ui32Addr = FLASH_STORE_START_ADDR;
            }

            //
            // Copy entire record (or remaining part of record if memory wrap)
            // into record buffer
            //
            memcpy(&g_pui32RecordBuf[ui32PartialCount / 4], (void *)ui32Addr,
                   ui32Count);

            //
            // Update address pointer to next record
            //
            ui32Addr += ui32Count;

            //
            // Now we have an entire data logger record copied from flash
            // storage into a local (contiguous) memory buffer.  Pass it
            // to the USB file writing function to write the record to the
            // USB stick.
            //
            USBStickWriteRecord((tLogRecord *)g_pui32RecordBuf);
        }
        else
        {
            //
            // This should not happen, but it means we ended up in a non-blank
            // location that is not the start of a record.  In this case just
            // advance through memory until either a blank location or another
            // record is found.
            //
            // Increment to next word in flash, adjust for memory wrap.
            //
            ui32Addr += 4;
            if(ui32Addr >= FLASH_STORE_END_ADDR)
            {
                ui32Addr = FLASH_STORE_START_ADDR;
            }
        }
    }

    //
    // Close the USB stick file so that any buffers will be flushed.
    //
    USBStickCloseFile();

    //
    // Inform user that save is complete.
    //
    SetStatusText("SAVE", "USB SAVE", "COMPLETE", "PRESS <");

    //
    // Return success
    //
    return(0);
}

//*****************************************************************************
//
// This is called at the start of logging to prepare space in flash for
// storage of logged data.  It searches for the first blank area in the
// flash storage to be used for storing records.
//
// If a starting address is specified then the search is skipped and it goes
// directly to the new address.  If the starting address is 0, then it performs
// the search.
//
//*****************************************************************************
int32_t
FlashStoreOpenLogFile(uint32_t ui32StartAddr)
{
    uint32_t ui32Addr;

    //
    // If a valid starting address is specified, then just use that and skip
    // the search below.
    //
    if((ui32StartAddr >= FLASH_STORE_START_ADDR) &&
       (ui32StartAddr < FLASH_STORE_END_ADDR))
    {
        g_ui32StoreAddr = ui32StartAddr;
        return(0);
    }

    //
    // Start at beginning of flash storage area
    //
    ui32Addr = FLASH_STORE_START_ADDR;

    //
    // Search until a blank is found or the end of flash storage area
    //
    while((HWREG(ui32Addr) != 0xFFFFFFFF) && (ui32Addr < FLASH_STORE_END_ADDR))
    {
        //
        // If a record signature is found, then increment to the next record
        //
        if((HWREG(ui32Addr) & 0xFFFFFF00) == 0x53554100)
        {
            ui32Addr += HWREG(ui32Addr) & 0xFF;
        }
        else
        {
            //
            // Just advance to the next location in flash
            //
            ui32Addr += 4;
        }
    }

    //
    // If we are at the end of flash that means no blank area was found.
    // So reset to the beginning and erase the first page.
    //
    if(ui32Addr >= FLASH_STORE_END_ADDR)
    {
        ui32Addr = FLASH_STORE_START_ADDR;
        FlashErase(ui32Addr);
    }

    //
    // When we reach here we either found a blank location, or made a new
    // blank location by erasing the first page.
    // To keep things simple we are making an assumption that the flash store
    // is not corrupted and that the first blank location implies the start
    // of a blank area suitable for storing data records.
    //
    g_ui32StoreAddr = ui32Addr;

    //
    // Return success indication to caller
    //
    return(0);
}

//*****************************************************************************
//
// This is called each time there is a new data record to log to the flash
// storage area.  A simple algorithm is used which rotates programming
// data log records through an area of flash.  It is assumed that the current
// page is blank.  Records are stored on the current page until a page
// boundary is crossed.  If the page boundary is crossed and the new page
// is not blank (testing only the first location), then the new page is
// erased.  Finally the entire record is programmed into flash and the
// storage pointers are updated.
//
// While storing and when crossing to a new page, if the flash page is not
// blank it is erased.  So this algorithm overwrites old data.
//
// The data is stored in flash as a record, with a flash header prepended,
// and with the record length padded to be a multiple of 4 bytes.  The flash
// header is a 3-byte magic number and one byte of record length.
//
//*****************************************************************************
int32_t
FlashStoreWriteRecord(tLogRecord *psRecord)
{
    uint32_t ui32Idx, ui32ItemCount, *pui32Record;

    //
    // Check the arguments
    //
    ASSERT(psRecord);
    if(!psRecord)
    {
        return(1);
    }

    //
    // Determine how many channels are to be logged
    //
    ui32Idx = psRecord->ui16ItemMask;
    ui32ItemCount = 0;
    while(ui32Idx)
    {
        if(ui32Idx & 1)
        {
            ui32ItemCount++;
        }
        ui32Idx >>= 1;
    }

    //
    // Add 16-bit count equivalent of record header, time stamp, and
    // selected items mask.  This is the total number of 16 bit words
    // of the record.
    //
    ui32ItemCount += 6;

    //
    // Convert the count to bytes, be sure to pad to 32-bit alignment.
    //
    ui32ItemCount = ((ui32ItemCount * 2) + 3) & ~3;

    //
    // Create the flash record header, which is a 3-byte signature and a
    // one byte count of bytes in the record.  Save it at the beginning
    // of the write buffer.
    //
    ui32Idx = 0x53554100 | (ui32ItemCount & 0xFF);
    g_pui32RecordBuf[0] = ui32Idx;

    //
    // Copy the rest of the record to the buffer, and get a pointer to
    // the buffer.
    //
    memcpy(&g_pui32RecordBuf[1], psRecord, ui32ItemCount - 4);
    pui32Record = g_pui32RecordBuf;

    //
    // Check to see if the record is going to cross a page boundary.
    //
    if(((g_ui32StoreAddr & 0x3FF) + ui32ItemCount) > 0x3FF)
    {
        //
        // Find number of bytes remaining on this page
        //
        ui32Idx = 0x400 - (g_ui32StoreAddr & 0x3FF);

        //
        // Program part of the record on the space remaining on the current
        // page
        //
        FlashProgram(pui32Record, g_ui32StoreAddr, ui32Idx);

        //
        // Increment the store address by the amount just written, which
        // should make the new store address be at the beginning of the next
        // flash page.
        //
        g_ui32StoreAddr += ui32Idx;

        //
        // Adjust the remaining bytes to program, and the pointer to the
        // remainder of the record data.
        //
        ui32ItemCount -= ui32Idx;
        pui32Record = &g_pui32RecordBuf[ui32Idx / 4];

        //
        // Check to see if the new page is past the end of store and adjust
        //
        if(g_ui32StoreAddr  >= FLASH_STORE_END_ADDR)
        {
            g_ui32StoreAddr = FLASH_STORE_START_ADDR;
        }

        //
        // If new page is not blank, then erase it
        //
        if(HWREG(g_ui32StoreAddr) != 0xFFFFFFFF)
        {
            FlashErase(g_ui32StoreAddr);
        }
    }

    //
    // Now program the remaining part of the record (if we crossed a page
    // boundary above) or the full record to the current location in flash
    //
    FlashProgram(pui32Record, g_ui32StoreAddr, ui32ItemCount);

    //
    // Increment the storage address to the next location.
    //
    g_ui32StoreAddr += ui32ItemCount;

    //
    // Return success indication to caller.
    //
    return(0);
}

//*****************************************************************************
//
// Return the current address being used for storing records.
//
//*****************************************************************************
uint32_t
FlashStoreGetAddr(void)
{
    return(g_ui32StoreAddr);
}

//*****************************************************************************
//
// Erase the data storage area of flash.
//
//*****************************************************************************
void
FlashStoreErase(void)
{
    uint32_t ui32Addr;

    //
    // Inform user we are erasing
    //
    SetStatusText("ERASE", 0, "ERASING", 0);

    //
    // Loop through entire storage area and erase each page.
    //
    for(ui32Addr = FLASH_STORE_START_ADDR; ui32Addr < FLASH_STORE_END_ADDR;
        ui32Addr += 0x400)
    {
        FlashErase(ui32Addr);
    }

    //
    // Inform user the erase is done.
    //
    SetStatusText("SAVE", "ERASE", "COMPLETE", "PRESS <");
}

//*****************************************************************************
//
// Determine if the flash block that contains the address is blank.
//
//*****************************************************************************
static int32_t
IsBlockFree(uint32_t ui32BaseAddr)
{
    uint32_t ui32Addr;

    //
    // Make sure we start at the beginning of a 1K block
    //
    ui32BaseAddr &= ~0x3FF;

    //
    // Loop through every address in this block and test if it is blank.
    //
    for(ui32Addr = 0; ui32Addr < 0x400; ui32Addr += 4)
    {
        if(HWREG(ui32BaseAddr + ui32Addr) != 0xFFFFFFFF)
        {
            //
            // Found a non-blank location, so return indication that block
            // is not free.
            //
            return(0);
        }
    }

    //
    // If we made it to here then every location in this block is erased,
    // so return indication that the block is free.
    //
    return(1);
}

//*****************************************************************************
//
// Report to the user the amount of free space and used space in the data
// storage area.
//
//*****************************************************************************
void
FlashStoreReport(void)
{
    uint32_t ui32Addr, ui32FreeBlocks, ui32UsedBlocks = 0;
    static char pcBufFree[16], pcBufUsed[16];

    //
    // Initialize locals.
    //
    ui32FreeBlocks = 0;
    ui32UsedBlocks = 0;

    //
    // Loop through each block of the storage area and count how many blocks
    // are free and non-free.
    //
    for(ui32Addr = FLASH_STORE_START_ADDR; ui32Addr < FLASH_STORE_END_ADDR;
        ui32Addr += 0x400)
    {
        if(IsBlockFree(ui32Addr))
        {
            ui32FreeBlocks++;
        }
        else
        {
            ui32UsedBlocks++;
        }
    }

    //
    // Report the result to the user via a status display screen.
    //
    usnprintf(pcBufFree, sizeof(pcBufFree), "FREE: %3uK", ui32FreeBlocks);
    usnprintf(pcBufUsed, sizeof(pcBufUsed), "USED: %3uK", ui32UsedBlocks);
    SetStatusText("FREE FLASH", pcBufFree, pcBufUsed, "PRESS <");
}
