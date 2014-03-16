//*****************************************************************************
//
// flash_pb.c - Flash parameter block functions.
//
// Copyright (c) 2008-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the Tiva Utility Library.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_flash.h"
#include "inc/hw_types.h"
#include "inc/hw_sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/flash.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "utils/flash_pb.h"

//*****************************************************************************
//
//! \addtogroup flash_pb_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The address of the beginning of the flash used for storing parameter blocks;
// this must be the start of an erase block in the flash.
//
//*****************************************************************************
static uint8_t *g_pui8FlashPBStart;

//*****************************************************************************
//
// The address of the end of the flash used for storing parameter blocks; this
// must be the start of an erase block in the flash, or the first location
// after the end of the flash array if the last erase block is used for storing
// parameters.
//
//*****************************************************************************
static uint8_t *g_pui8FlashPBEnd;

//*****************************************************************************
//
// The size of the parameter block when stored in flash; this must be a power
// of two less than or equal to the flash erase sector size such that a single
// flash sector contains an integral number of parameter blocks.
//
//*****************************************************************************
static uint32_t g_ui32FlashPBSize;

//*****************************************************************************
//
// The address of the most recent parameter block in flash.
//
//*****************************************************************************
static uint8_t *g_pui8FlashPBCurrent;

//*****************************************************************************
//
// The erase sector size of the current flash.
//
//*****************************************************************************
#define FLASH_SECTOR_SIZE       MAP_SysCtlFlashSectorSizeGet()

//*****************************************************************************
//
//! Determines if the parameter block at the given address is valid.
//!
//! \param pui8Offset is the address of the parameter block to check.
//!
//! This function will compute the checksum of a parameter block in flash to
//! determine if it is valid.
//!
//! \return Returns one if the parameter block is valid and zero if it is not.
//
//*****************************************************************************
static uint32_t
FlashPBIsValid(uint8_t *pui8Offset)
{
    uint32_t ui32Idx, ui32Sum;

    //
    // Check the arguments.
    //
    ASSERT(pui8Offset != (void *)0);

    //
    // Loop through the bytes in the block, computing the checksum.
    //
    for(ui32Idx = 0, ui32Sum = 0; ui32Idx < g_ui32FlashPBSize; ui32Idx++)
    {
        ui32Sum += pui8Offset[ui32Idx];
    }

    //
    // The checksum should be zero, so return a failure if it is not.
    //
    if((ui32Sum & 255) != 0)
    {
        return(0);
    }

    //
    // If the sum is equal to the size * 255, then the block is all ones and
    // should not be considered valid.
    //
    if((g_ui32FlashPBSize * 255) == ui32Sum)
    {
        return(0);
    }

    //
    // This is a valid parameter block.
    //
    return(1);
}

//*****************************************************************************
//
//! Gets the address of the most recent parameter block.
//!
//! This function returns the address of the most recent parameter block that
//! is stored in flash.
//!
//! \return Returns the address of the most recent parameter block, or NULL if
//! there are no valid parameter blocks in flash.
//
//*****************************************************************************
uint8_t *
FlashPBGet(void)
{
    //
    // See if there is a valid parameter block.
    //
    if(g_pui8FlashPBCurrent)
    {
        //
        // Return the address of the most recent parameter block.
        //
        return(g_pui8FlashPBCurrent);
    }

    //
    // There are no valid parameter blocks in flash, so return NULL.
    //
    return(0);
}

//*****************************************************************************
//
//! Writes a new parameter block to flash.
//!
//! \param pui8Buffer is the address of the parameter block to be written to
//! flash.
//!
//! This function will write a parameter block to flash.  Saving the new
//! parameter blocks involves three steps:
//!
//! - Setting the sequence number such that it is one greater than the sequence
//!   number of the latest parameter block in flash.
//! - Computing the checksum of the parameter block.
//! - Writing the parameter block into the storage immediately following the
//!   latest parameter block in flash; if that storage is at the start of an
//!   erase block, that block is erased first.
//!
//! By this process, there is always a valid parameter block in flash.  If
//! power is lost while writing a new parameter block, the checksum will not
//! match and the partially written parameter block will be ignored.  This is
//! what makes this fault-tolerant.
//!
//! Another benefit of this scheme is that it provides wear leveling on the
//! flash.  Since multiple parameter blocks fit into each erase block of flash,
//! and multiple erase blocks are used for parameter block storage, it takes
//! quite a few parameter block saves before flash is re-written.
//!
//! \return None.
//
//*****************************************************************************
void
FlashPBSave(uint8_t *pui8Buffer)
{
    uint8_t *pui8New;
    uint32_t ui32Idx, ui32Sum;

    //
    // Check the arguments.
    //
    ASSERT(pui8Buffer != (void *)0);

    //
    // See if there is a valid parameter block in flash.
    //
    if(g_pui8FlashPBCurrent)
    {
        //
        // Set the sequence number to one greater than the most recent
        // parameter block.
        //
        pui8Buffer[0] = g_pui8FlashPBCurrent[0] + 1;

        //
        // Try to write the new parameter block immediately after the most
        // recent parameter block.
        //
        pui8New = g_pui8FlashPBCurrent + g_ui32FlashPBSize;
        if(pui8New == g_pui8FlashPBEnd)
        {
            pui8New = g_pui8FlashPBStart;
        }
    }
    else
    {
        //
        // There is not a valid parameter block in flash, so set the sequence
        // number of this parameter block to zero.
        //
        pui8Buffer[0] = 0;

        //
        // Try to write the new parameter block at the beginning of the flash
        // space for parameter blocks.
        //
        pui8New = g_pui8FlashPBStart;
    }

    //
    // Compute the checksum of the parameter block to be written.
    //
    for(ui32Idx = 0, ui32Sum = 0; ui32Idx < g_ui32FlashPBSize; ui32Idx++)
    {
        ui32Sum -= pui8Buffer[ui32Idx];
    }

    //
    // Store the checksum into the parameter block.
    //
    pui8Buffer[1] += ui32Sum;

    //
    // Look for a location to store this parameter block.  This infinite loop
    // will be explicitly broken out of when a valid location is found.
    //
    while(1)
    {
        //
        // See if this location is at the start of an erase block.
        //
        if(((uint32_t)pui8New & (FLASH_SECTOR_SIZE - 1)) == 0)
        {
            //
            // Erase this block of the flash.  This does not assume that the
            // erase succeeded in case this block of the flash has become bad
            // through too much use.  Given the extremely low frequency that
            // the parameter blocks are written, this will likely never fail.
            // But, that assumption is not made in order to be safe.
            //
            MAP_FlashErase((uint32_t)pui8New);
        }

        //
        // Loop through this portion of flash to see if is all ones (in other
        // words, it is an erased portion of flash).
        //
        for(ui32Idx = 0; ui32Idx < g_ui32FlashPBSize; ui32Idx++)
        {
            if(pui8New[ui32Idx] != 0xff)
            {
                break;
            }
        }

        //
        // If all bytes in this portion of flash are ones, then break out of
        // the loop since this is a good location for storing the parameter
        // block.
        //
        if(ui32Idx == g_ui32FlashPBSize)
        {
            break;
        }

        //
        // Increment to the next parameter block location.
        //
        pui8New += g_ui32FlashPBSize;
        if(pui8New == g_pui8FlashPBEnd)
        {
            pui8New = g_pui8FlashPBStart;
        }

        //
        // If every possible location has been checked and none are valid, then
        // it will not be possible to write this parameter block.  Simply
        // return without writing it.
        //
        if((g_pui8FlashPBCurrent && (pui8New == g_pui8FlashPBCurrent)) ||
           (!g_pui8FlashPBCurrent && (pui8New == g_pui8FlashPBStart)))
        {
            return;
        }
    }

    //
    // Write this parameter block to flash.
    //
    MAP_FlashProgram((uint32_t *)pui8Buffer, (uint32_t)pui8New,
                     g_ui32FlashPBSize);

    //
    // Compare the parameter block data to the data that should now be in
    // flash.  Return if any of the data does not compare, leaving the previous
    // parameter block in flash as the most recent (since the current parameter
    // block failed to properly program).
    //
    for(ui32Idx = 0; ui32Idx < g_ui32FlashPBSize; ui32Idx++)
    {
        if(pui8New[ui32Idx] != pui8Buffer[ui32Idx])
        {
            return;
        }
    }

    //
    // The new parameter block becomes the most recent parameter block.
    //
    g_pui8FlashPBCurrent = pui8New;
}

//*****************************************************************************
//
//! Initializes the flash parameter block.
//!
//! \param ui32Start is the address of the flash memory to be used for storing
//! flash parameter blocks; this must be the start of an erase block in the
//! flash.
//! \param ui32End is the address of the end of flash memory to be used for
//! storing flash parameter blocks; this must be the start of an erase block in
//! the flash (the first block that is NOT part of the flash memory to be
//! used), or the address of the first word after the flash array if the last
//! block of flash is to be used.
//! \param ui32Size is the size of the parameter block when stored in flash;
//! this must be a power of two less than or equal to the flash erase block
//! size (typically 1024).
//!
//! This function initializes a fault-tolerant, persistent storage mechanism
//! for a parameter block for an application.  The last several erase blocks
//! of flash (as specified by \e ui32Start and \e ui32End are used for the
//! storage; more than one erase block is required in order to be
//! fault-tolerant.
//!
//! A parameter block is an array of bytes that contain the persistent
//! parameters for the application.  The only special requirement for the
//! parameter block is that the first byte is a sequence number (explained
//! in FlashPBSave()) and the second byte is a checksum used to validate the
//! correctness of the data (the checksum byte is the byte such that the sum of
//! all bytes in the parameter block is zero).
//!
//! The portion of flash for parameter block storage is split into N
//! equal-sized regions, where each region is the size of a parameter block
//! (\e ui32Size).  Each region is scanned to find the most recent valid
//! parameter block.  The region that has a valid checksum and has the highest
//! sequence number (with special consideration given to wrapping back to zero)
//! is considered to be the current parameter block.
//!
//! In order to make this efficient and effective, three conditions must be
//! met.  The first is \e ui32Start and \e ui32End must be specified such that
//! at least two erase blocks of flash are dedicated to parameter block
//! storage.  If not, fault tolerance can not be guaranteed since an erase of a
//! single block will leave a window where there are no valid parameter blocks
//! in flash.  The second condition is that the size (\e ui32Size) of the
//! parameter block must be an integral divisor of the size of an erase block
//! of flash.  If not, a parameter block will end up spanning between two erase
//! blocks of flash, making it more difficult to manage.  The final condition
//! is that the size of the flash dedicated to parameter blocks (\e ui32End -
//! \e ui32Start) divided by the parameter block size (\e ui32Size) must be
//! less than or equal to 128.  If not, it will not be possible in all cases to
//! determine which parameter block is the most recent (specifically when
//! dealing with the sequence number wrapping back to zero).
//!
//! When the microcontroller is initially programmed, the flash blocks used for
//! parameter block storage are left in an erased state.
//!
//! This function must be called before any other flash parameter block
//! functions are called.
//!
//! \return None.
//
//*****************************************************************************
void
FlashPBInit(uint32_t ui32Start, uint32_t ui32End, uint32_t ui32Size)
{
    uint8_t *pui8Offset, *pui8Current;
    uint8_t ui8One, ui8Two;

    //
    // Check the arguments.
    //
    ASSERT((ui32Start % FLASH_SECTOR_SIZE) == 0);
    ASSERT((ui32End % FLASH_SECTOR_SIZE) == 0);
    ASSERT((FLASH_SECTOR_SIZE % ui32Size) == 0);

    //
    // Save the characteristics of the flash memory to be used for storing
    // parameter blocks.
    //
    g_pui8FlashPBStart = (uint8_t *)ui32Start;
    g_pui8FlashPBEnd = (uint8_t *)ui32End;
    g_ui32FlashPBSize = ui32Size;

    //
    // Loop through the portion of flash memory used for storing parameter
    // blocks.
    //
    for(pui8Offset = g_pui8FlashPBStart, pui8Current = 0;
        pui8Offset < g_pui8FlashPBEnd; pui8Offset += g_ui32FlashPBSize)
    {
        //
        // See if this is a valid parameter block (in other words, the checksum
        // is correct).
        //
        if(FlashPBIsValid(pui8Offset))
        {
            //
            // See if a valid parameter block has been previously found.
            //
            if(pui8Current != 0)
            {
                //
                // Get the sequence numbers for the current and new parameter
                // blocks.
                //
                ui8One = pui8Current[0];
                ui8Two = pui8Offset[0];

                //
                // See if the sequence number for the new parameter block is
                // greater than the current block.  The comparison isn't
                // straightforward since the one byte sequence number will wrap
                // after 256 parameter blocks.
                //
                if(((ui8One > ui8Two) && ((ui8One - ui8Two) < 128)) ||
                   ((ui8Two > ui8One) && ((ui8Two - ui8One) > 128)))
                {
                    //
                    // The new parameter block is older than the current
                    // parameter block, so skip the new parameter block and
                    // keep searching.
                    //
                    continue;
                }
            }

            //
            // The new parameter block is more recent than the current one, so
            // make it the new current parameter block.
            //
            pui8Current = pui8Offset;
        }
    }

    //
    // Save the address of the most recent parameter block found.  If no valid
    // parameter blocks were found, this will be a NULL pointer.
    //
    g_pui8FlashPBCurrent = pui8Current;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
