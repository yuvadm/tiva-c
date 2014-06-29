//*****************************************************************************
//
// mx66l51235f.c - Driver for the on-board SPI flash.
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
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/ssi.h"
#include "utils/spi_flash.h"

//*****************************************************************************
//
//! \addtogroup mx66l51235f_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The current value of the extended address register.  This is tracked so that
// it is only updated via SPI when it needs to be changed.
//
//*****************************************************************************
static uint32_t g_ui32MX66L51235FAddr;

//*****************************************************************************
//
//! Initializes the MX66L51235F driver.
//!
//! \param ui32SysClock is the frequency of the system clock.
//!
//! This function initializes the MX66L51235F driver and SSI interface,
//! preparing for accesses to the SPI flash device.  Since the SSI interface
//! on the DK-TM4C129X board is shared with the SD card, this must be called
//! prior to any SPI flash access the immediately follows an SD card access.
//!
//! \return None.
//
//*****************************************************************************
void
MX66L51235FInit(uint32_t ui32SysClock)
{
    uint32_t ui32SPIClock;

    //
    // Set the SPI clock to the minimum of one quarter of system clock and
    // 12.5 MHz.  The SPI module can run at up to one quarter of system clock
    // and the SD card, if inserted, might cause interference on the SPI bus
    // (even though it is not selected) if the SPI clock exceeds 12.5 MHz.
    //
    ui32SPIClock = ui32SysClock / 4;
    if(ui32SPIClock > 12500000)
    {
        ui32SPIClock = 12500000;
    }

    //
    // Configure the SPI flash driver on SSI3.
    //
    ROM_SPIFlashInit(SSI3_BASE, ui32SysClock, ui32SPIClock);

    //
    // Set the current value of the extended address register to -1, causing it
    // to be written on the first access.
    //
    g_ui32MX66L51235FAddr = 0xffffffff;
}

//*****************************************************************************
//
// Enable program/erase of the MX66L51235F.
//
//*****************************************************************************
static void
MX66L51235FWriteEnable(void)
{
    //
    // Assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, 0);

    //
    // Enable programming/erasing of the MX66L51235F.
    //
    ROM_SPIFlashWriteEnable(SSI3_BASE);

    //
    // Wait until the command has been completely transmitted.
    //
    while(ROM_SSIBusy(SSI3_BASE))
    {
    }

    //
    // De-assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, GPIO_PIN_1);
}

//*****************************************************************************
//
// Waits until a program/erase operation has completed.
//
//*****************************************************************************
static void
MX66L51235FWait(void)
{
    uint8_t ui8Status;

    //
    // Loop until the requested operation has completed.
    //
    do
    {
        //
        // Assert the chip select to the MX66L51235F.
        //
        ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, 0);

        //
        // Read the status register.
        //
        ui8Status = ROM_SPIFlashReadStatus(SSI3_BASE);

        //
        // De-assert the chip select to the MX66L51235F.
        //
        ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, GPIO_PIN_1);
    }
    while(ui8Status & 1);
}

//*****************************************************************************
//
// Writes the extended address register, allowing the full contents of the
// MX66L51235F to be accessed.
//
//*****************************************************************************
static void
MX66L51235FWriteEAR(uint32_t ui32Addr)
{
    //
    // See if the extended address register needs to be written.
    //
    if((ui32Addr & 0xff000000) == (g_ui32MX66L51235FAddr & 0xff000000))
    {
        //
        // The extended address register does not need to be changed, so return
        // without doing anything.
        //
        return;
    }

    //
    // Save the new value of the extended address register.
    //
    g_ui32MX66L51235FAddr = ui32Addr;

    //
    // Enable program/erase of the SPI flash.
    //
    MX66L51235FWriteEnable();

    //
    // Assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, 0);

    //
    // Set the SSI module into write-only mode.
    //
    ROM_SSIAdvModeSet(SSI3_BASE, SSI_ADV_MODE_WRITE);

    //
    // Send the sector erase command.
    //
    ROM_SSIDataPut(SSI3_BASE, 0xc5);

    //
    // Send the address of the sector to be erased, marking the last byte of
    // the address as the end of the frame.
    //
    ROM_SSIAdvDataPutFrameEnd(SSI3_BASE, (ui32Addr >> 24) & 0xff);

    //
    // Wait until the command has been completely transmitted.
    //
    while(ROM_SSIBusy(SSI3_BASE))
    {
    }

    //
    // De-assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, GPIO_PIN_1);
}

//*****************************************************************************
//
//! Erases a 4 KB sector of the MX66L51235F.
//!
//! \param ui32Addr is the address of the sector to erase.
//!
//! This function erases a sector of the MX66L51235F.  Each sector is 4 KB with
//! a 4 KB alignment; the MX66L51235F will ignore the lower ten bits of the
//! address provided.  This function will not return until the data has be
//! erased.
//!
//! \return None.
//
//*****************************************************************************
void
MX66L51235FSectorErase(uint32_t ui32Addr)
{
    //
    // Write the extended address register.
    //
    MX66L51235FWriteEAR(ui32Addr);

    //
    // Enable program/erase of the SPI flash.
    //
    MX66L51235FWriteEnable();

    //
    // Assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, 0);

    //
    // Erase the requested sector.
    //
    ROM_SPIFlashSectorErase(SSI3_BASE, ui32Addr);

    //
    // Wait until the command has been completely transmitted.
    //
    while(ROM_SSIBusy(SSI3_BASE))
    {
    }

    //
    // De-assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, GPIO_PIN_1);

    //
    // Wait for the erase operation to complete.
    //
    MX66L51235FWait();
}

//*****************************************************************************
//
//! Erases a 32 KB block of the MX66L51235F.
//!
//! \param ui32Addr is the address of the block to erase.
//!
//! This function erases a 32 KB block of the MX66L51235F.  Each 32 KB block
//! has a 32 KB alignment; the MX66L51235F will ignore the lower 15 bits of the
//! address provided.  This function will not return until the data has be
//! erased.
//!
//! \return None.
//
//*****************************************************************************
void
MX66L51235FBlockErase32(uint32_t ui32Addr)
{
    //
    // Write the extended address register.
    //
    MX66L51235FWriteEAR(ui32Addr);

    //
    // Enable program/erase of the SPI flash.
    //
    MX66L51235FWriteEnable();

    //
    // Assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, 0);

    //
    // Erase the requested block.
    //
    ROM_SPIFlashBlockErase32(SSI3_BASE, ui32Addr);

    //
    // Wait until the command has been completely transmitted.
    //
    while(ROM_SSIBusy(SSI3_BASE))
    {
    }

    //
    // De-assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, GPIO_PIN_1);

    //
    // Wait for the erase operation to complete.
    //
    MX66L51235FWait();
}

//*****************************************************************************
//
//! Erases a 64 KB block of the MX66L51235F.
//!
//! \param ui32Addr is the address of the block to erase.
//!
//! This function erases a 64 KB block of the MX66L51235F.  Each 64 KB block
//! has a 64 KB alignment; the MX66L51235F will ignore the lower 16 bits of the
//! address provided.  This function will not return until the data has be
//! erased.
//!
//! \return None.
//
//*****************************************************************************
void
MX66L51235FBlockErase64(uint32_t ui32Addr)
{
    //
    // Write the extended address register.
    //
    MX66L51235FWriteEAR(ui32Addr);

    //
    // Enable program/erase of the SPI flash.
    //
    MX66L51235FWriteEnable();

    //
    // Assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, 0);

    //
    // Erase the requested block.
    //
    ROM_SPIFlashBlockErase64(SSI3_BASE, ui32Addr);

    //
    // Wait until the command has been completely transmitted.
    //
    while(ROM_SSIBusy(SSI3_BASE))
    {
    }

    //
    // De-assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, GPIO_PIN_1);

    //
    // Wait for the erase operation to complete.
    //
    MX66L51235FWait();
}

//*****************************************************************************
//
//! Erases the entire MX66L51235F.
//!
//! This command erase the entire contents of the MX66L51235F.  This takes two
//! minutes, nominally, to complete.  This function will not return until the
//! data has be erased.
//!
//! \return None.
//
//*****************************************************************************
void
MX66L51235FChipErase(void)
{
    //
    // Enable program/erase of the SPI flash.
    //
    MX66L51235FWriteEnable();

    //
    // Assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, 0);

    //
    // Erase the entire device.
    //
    ROM_SPIFlashChipErase(SSI3_BASE);

    //
    // Wait until the command has been completely transmitted.
    //
    while(ROM_SSIBusy(SSI3_BASE))
    {
    }

    //
    // De-assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, GPIO_PIN_1);

    //
    // Wait for the erase operation to complete.
    //
    MX66L51235FWait();
}

//*****************************************************************************
//
//! Programs the MX66L51235F.
//!
//! \param ui32Addr is the address to be programmed.
//! \param pui8Data is a pointer to the data to be programmed.
//! \param ui32Count is the number of bytes to be programmed.
//!
//! This function programs data into the MX66L51235F.  This function will not
//! return until the data has be programmed.  The addresses to be programmed
//! must not span a 256-byte boundary (in other words, ``\e ui32Addr & ~255''
//! must be the same as ``(\e ui32Addr + \e ui32Count) & ~255'').
//!
//! \return None.
//
//*****************************************************************************
void
MX66L51235FPageProgram(uint32_t ui32Addr, const uint8_t *pui8Data,
                       uint32_t ui32Count)
{
    //
    // Write the extended address register.
    //
    MX66L51235FWriteEAR(ui32Addr);

    //
    // Enable program/erase of the SPI flash.
    //
    MX66L51235FWriteEnable();

    //
    // Assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, 0);

    //
    // Program the requested data.
    //
    ROM_SPIFlashPageProgram(SSI3_BASE, ui32Addr, pui8Data, ui32Count);

    //
    // Wait until the command has been completely transmitted.
    //
    while(ROM_SSIBusy(SSI3_BASE))
    {
    }

    //
    // De-assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, GPIO_PIN_1);

    //
    // Wait for the page program operation to complete.
    //
    MX66L51235FWait();
}

//*****************************************************************************
//
//! Reads data from the MX66L51235F.
//!
//! \param ui32Addr is the address to read.
//! \param pui8Data is a pointer to the data buffer to into which to read the
//! data.
//! \param ui32Count is the number of bytes to read.
//!
//! This function reads data from the MX66L51235F.
//!
//! \return None.
//
//*****************************************************************************
void
MX66L51235FRead(uint32_t ui32Addr, uint8_t *pui8Data, uint32_t ui32Count)
{
    //
    // Write the extended address register.
    //
    MX66L51235FWriteEAR(ui32Addr);

    //
    // Assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, 0);

    //
    // Read the requested data.
    //
    ROM_SPIFlashRead(SSI3_BASE, ui32Addr, pui8Data, ui32Count);

    //
    // De-assert the chip select to the MX66L51235F.
    //
    ROM_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, GPIO_PIN_1);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
