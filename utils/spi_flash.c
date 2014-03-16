//*****************************************************************************
//
// spi_flash.c - Driver for a SPI flash that supports the "Intel" SPI flash
//               command set, capable of utilizing Bi-SPI and Quad-SPI.
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
// This is part of revision 2.1.0.12573 of the Tiva Utility Library.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"
#include "inc/hw_udma.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/ssi.h"
#include "driverlib/udma.h"
#include "utils/spi_flash.h"

//*****************************************************************************
//
//! \addtogroup spi_flash_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The commands that can be sent to the SPI flash.  This is the "generic"
// command set that is supported by a wide number of SPI flashes.
//
//*****************************************************************************
#define CMD_WRSR                0x01        // Write status register
#define CMD_PP                  0x02        // Page program
#define CMD_READ                0x03        // Read data
#define CMD_WRDI                0x04        // Disable writes
#define CMD_RDSR                0x05        // Read status register
#define CMD_WREN                0x06        // Enable writes
#define CMD_FREAD               0x0b        // Fast read data
#define CMD_SE                  0x20        // Sector erase (4K)
#define CMD_DREAD               0x3b        // 1 in 2 out read data
#define CMD_BE32                0x52        // Block erase (32K)
#define CMD_QREAD               0x6b        // 1 in 4 out read data
#define CMD_RDID                0x9f        // Read JEDEC ID
#define CMD_CE                  0xc7        // Chip erase
#define CMD_BE64                0xd8        // Block erase (64K)

//*****************************************************************************
//
// The states for the SPI flash interrupt handler state machine.
//
//*****************************************************************************
#define STATE_IDLE              0
#define STATE_CMD               1
#define STATE_ADDR1             2
#define STATE_ADDR2             3
#define STATE_ADDR3             4
#define STATE_READ_DUMMY        5
#define STATE_READ_DATA_SETUP   6
#define STATE_READ_DATA         7
#define STATE_READ_DATA_DMA     8
#define STATE_READ_DATA_END     9
#define STATE_WRITE_DATA_SETUP  10
#define STATE_WRITE_DATA        11
#define STATE_WRITE_DATA_DMA    12
#define STATE_WRITE_DATA_END    13

//*****************************************************************************
//
//! Handles SSI module interrupts for the SPI flash driver.
//!
//! \param pState is a pointer to the SPI flash driver instance data.
//!
//! This function handles SSI module interrupts that are generated as a result
//! of SPI flash driver operations.  This must be called by the application in
//! response to the SSI module interrupt when using the SPIFlashxxxNonBlocking
//! APIs.
//!
//! \return Returns \b SPI_FLASH_IDLE if there is no transfer in progress,
//! \b SPI_FLASH_WORKING is the requested transfer is still in progress, or
//! \b SPI_FLASH_DONE if the requested transfer has completed.
//
//*****************************************************************************
uint32_t
SPIFlashIntHandler(tSPIFlashState *pState)
{
    uint32_t ui32Data, ui32Count;

    //
    // Set the write count to four.  This is the maximum number of bytes that
    // will be written into the SSI transmit FIFO in the interrupt handler.
    // Writing more might be possible but makes the latency of handling future
    // SSI interrupt critical to preventing receive FIFO overruns.
    //
    ui32Count = 4;

    //
    // Get the set of asserted and unmasked SSI module interrupts.  Only some
    // of these are directly handled; the others are implicitly handled via the
    // operation of the state machine.
    //
    ui32Data = HWREG(pState->ui32Base + SSI_O_MIS);

    //
    // See if the uDMA transmit complete interrupt has asserted.
    //
    if(ui32Data & SSI_MIS_DMATXMIS)
    {
        //
        // Determine the size of the uDMA transfer based on the number of bytes
        // left to write.
        //
        if(pState->ui32WriteCount > 1024)
        {
            //
            // There are more than 1024 bytes left to transfer, so the uDMA
            // transfer that just completed was for a full 1024 bytes.
            //
            pState->ui32WriteCount -= 1024;

            //
            // If a page program is being performed, then the data buffer
            // pointer needs to be incremented as well.
            //
            if(pState->ui16Cmd == CMD_PP)
            {
                //
                // Increment the data buffer pointer.
                //
                pState->pui8Buffer += 1024;

                //
                // See if there is more than one byte left to transfer.
                //
                if(pState->ui32WriteCount > 1)
                {
                    //
                    // Configure the uDMA to transmit the next portion of the
                    // data buffer.
                    //
                    uDMAChannelTransferSet(pState->ui32TxChannel,
                                           UDMA_MODE_BASIC,
                                           pState->pui8Buffer,
                                           (void *)(pState->ui32Base +
                                                    SSI_O_DR),
                                           (pState->ui32WriteCount > 1024) ?
                                           1024 : pState->ui32WriteCount - 1);

                    //
                    // Enable the uDMA transmit channel.
                    //
                    uDMAChannelEnable(pState->ui32TxChannel);
                }
            }
        }
        else
        {
            //
            // There are 1024 or less bytes left to transfer, so the uDMA
            // transfer that just copmleted was for one less than the remaining
            // transfer count.  If a page program is being performed, then the
            // data buffer pointer needs to be incremented.
            //
            if(pState->ui16Cmd == CMD_PP)
            {
                pState->pui8Buffer += (pState->ui32WriteCount - 1);
            }

            //
            // Set the remaining transfer count to 1.  The final byte will be
            // transferred with PIO since the end of frame flag needs to be set
            // first.
            //
            pState->ui32WriteCount = 1;
        }

        //
        // Clear the uDMA transmit complete interrupt.
        //
        HWREG(pState->ui32Base + SSI_O_ICR) = SSI_ICR_DMATXIC;
    }

    //
    // See if the uDMA receive complete interrupt has asserted.
    //
    if(ui32Data & SSI_MIS_DMARXMIS)
    {
        //
        // Determine the size of the uDMA transfer based on the number of bytes
        // left to read.
        //
        if(pState->ui32ReadCount >= 1024)
        {
            //
            // There are 1024 or more bytes left to transfer, so the uDMA
            // transfer that just completed was for a full 1024 bytes.
            //
            pState->ui32ReadCount -= 1024;
            if(pState->ui32WriteCount != 0)
            {
                pState->ui32WriteCount -= 1024;
            }

            //
            // The data buffer pointer needs to be incremented as well.
            //
            pState->pui8Buffer += 1024;

            //
            // See if there is additional data to transfer.
            //
            if(pState->ui32ReadCount != 0)
            {
                //
                // Configure the transmit uDMA if there is more than one byte
                // left to write.
                //
                if(pState->ui32WriteCount > 1)
                {
                    //
                    // Configure the uDMA to transmit the next portion of the
                    // data buffer.
                    //
                    uDMAChannelTransferSet(pState->ui32TxChannel,
                                           UDMA_MODE_BASIC, pState->pui8Buffer,
                                           (void *)(pState->ui32Base +
                                                    SSI_O_DR),
                                           (pState->ui32WriteCount > 1024) ?
                                           1024 : pState->ui32WriteCount - 1);

                    //
                    // Enable the uDMA transmit channel.
                    //
                    uDMAChannelEnable(pState->ui32TxChannel);
                }

                //
                // Configure the uDMA to receive the next portion of the data
                // buffer.
                //
                uDMAChannelTransferSet(pState->ui32RxChannel, UDMA_MODE_BASIC,
                                       (void *)(pState->ui32Base + SSI_O_DR),
                                       pState->pui8Buffer,
                                       (pState->ui32ReadCount >= 1024) ?
                                       1024 : pState->ui32ReadCount);

                //
                // Enable the uDMA receive channel.
                //
                uDMAChannelEnable(pState->ui32RxChannel);

                //
                // If this is the final receive uDMA buffer and there is a
                // transmit uDMA buffer associated, enable the DMA transmit
                // interrupt.
                //
                if((pState->ui32ReadCount <= 1024) &&
                   (pState->ui32WriteCount > 1))
                {
                    HWREG(pState->ui32Base + SSI_O_ICR) = SSI_ICR_DMATXIC;
                    HWREG(pState->ui32Base + SSI_O_IM) = SSI_IM_DMATXIM;
                }
            }
        }
        else
        {
            //
            // There are less than 1024 bytes left to transfer, so the uDMA
            // transfer that copmleted was for the remaining transfer count.
            //
            pState->ui32ReadCount = 0;
        }

        //
        // Clear the uDMA receive complete interrupt.
        //
        HWREG(pState->ui32Base + SSI_O_ICR) = SSI_ICR_DMARXIC;
    }

    //
    // Drain the receive FIFO is not using uDMA.
    //
    if(!pState->bUseDMA)
    {
        //
        // Loop while there is more data in the receive FIFO and more data to
        // be read.
        //
        while((pState->ui32ReadCount != 0) &&
              (MAP_SSIDataGetNonBlocking(pState->ui32Base, &ui32Data) != 0))
        {
            //
            // Save this byte into the data buffer.
            //
            *(pState->pui8Buffer)++ = ui32Data & 0xff;

            //
            // Decrement the read count.
            //
            pState->ui32ReadCount--;
        }
    }

    //
    // The SPI flash state machine.  Loop forever; the state machine will
    // explicitly return to the caller when there is no further work that can
    // be done without stalling.
    //
    while(1)
    {
        //
        // Determine the current state.
        //
        switch(pState->ui16State)
        {
            //
            // The state machine is idle.
            //
            case STATE_IDLE:
            {
                //
                // Return indicating that the state machine is idle.  This
                // should never happen since no further interrupts should occur
                // once the transfer has completed and the state machine goes
                // into the idle state.
                //
                return(SPI_FLASH_IDLE);
            }

            //
            // The state machine is in the command state.
            //
            case STATE_CMD:
            {
                //
                // Set the SSI module into write-only mode.
                //
                MAP_SSIAdvModeSet(pState->ui32Base, SSI_ADV_MODE_WRITE);

                //
                // Attempt to write the command byte into the FIFO.
                //
                if(ui32Count == 0)
                {
                    return(SPI_FLASH_WORKING);
                }
                if(MAP_SSIDataPutNonBlocking(pState->ui32Base,
                                             pState->ui16Cmd) == 0)
                {
                    //
                    // The command byte could not be written, so return
                    // indicating that the transfer is still in progress.
                    //
                    return(SPI_FLASH_WORKING);
                }
                else
                {
                    //
                    // The command byte has been written, so move to the first
                    // address byte state.
                    //
                    pState->ui16State = STATE_ADDR1;

                    //
                    // Decrement the count of bytes that have been written.
                    //
                    ui32Count--;
                }

                //
                // Done with this state.
                //
                break;
            }

            //
            // The state machine is in the first address byte state.
            //
            case STATE_ADDR1:
            {
                //
                // Attempt to write the first address byte into the FIFO.
                //
                if(ui32Count == 0)
                {
                    return(SPI_FLASH_WORKING);
                }
                if(MAP_SSIDataPutNonBlocking(pState->ui32Base,
                                             (pState->ui32Addr >> 16) &
                                             0xff) == 0)
                {
                    //
                    // The first address byte could not be written, so return
                    // indicating that the transfer is still in progress.
                    //
                    return(SPI_FLASH_WORKING);
                }
                else
                {
                    //
                    // The first address byte has been written, so move to the
                    // second address byte state.
                    //
                    pState->ui16State = STATE_ADDR2;

                    //
                    // Decrement the count of bytes that have been written.
                    //
                    ui32Count--;
                }

                //
                // Done with this state.
                //
                break;
            }

            //
            // The state machine is in the second address byte state.
            //
            case STATE_ADDR2:
            {
                //
                // Attempt to write the second address byte into the FIFO.
                //
                if(ui32Count == 0)
                {
                    return(SPI_FLASH_WORKING);
                }
                if(MAP_SSIDataPutNonBlocking(pState->ui32Base,
                                             (pState->ui32Addr >> 8) & 0xff) ==
                   0)
                {
                    //
                    // The second address byte could not be written, so return
                    // indicating that the transfer is still in progress.
                    //
                    return(SPI_FLASH_WORKING);
                }
                else
                {
                    //
                    // The second address byte has been written, so move to the
                    // third address byte state.
                    //
                    pState->ui16State = STATE_ADDR3;

                    //
                    // Decrement the count of bytes that have been written.
                    //
                    ui32Count--;
                }

                //
                // Done with this state.
                //
                break;
            }

            //
            // The state machine is in the third address byte state.
            //
            case STATE_ADDR3:
            {
                //
                // Attempt to write the third address byte into the FIFO.
                //
                if(ui32Count == 0)
                {
                    return(SPI_FLASH_WORKING);
                }
                if(MAP_SSIDataPutNonBlocking(pState->ui32Base,
                                             pState->ui32Addr & 0xff) == 0)
                {
                    //
                    // The third address byte could not be written, so return
                    // indicating that the transfer is still in progress.
                    //
                    return(SPI_FLASH_WORKING);
                }
                else
                {
                    //
                    // The third address byte has been written, so determine
                    // the next state based on the command byte.
                    //
                    if(pState->ui16Cmd == CMD_PP)
                    {
                        //
                        // A page program is being performed, so move to the
                        // write data setup state.
                        //
                        pState->ui16State = STATE_WRITE_DATA_SETUP;
                    }
                    else if(pState->ui16Cmd == CMD_READ)
                    {
                        //
                        // A read is being performed, so move to the read data
                        // setup state.
                        //
                        pState->ui16State = STATE_READ_DATA_SETUP;
                    }
                    else
                    {
                        //
                        // The other forms of read (fast read, dual read, and
                        // quad read) all require a dummy byte.  Move to the
                        // dummy byte state.
                        //
                        pState->ui16State = STATE_READ_DUMMY;
                    }

                    //
                    // Decrement the count of bytes that have been written.
                    //
                    ui32Count--;
                }

                //
                // Done with this state.
                //
                break;
            }

            //
            // The state machine is in the dummy byte state.
            //
            case STATE_READ_DUMMY:
            {
                //
                // Attempt to write the dummy byte into the FIFO.
                //
                if(ui32Count == 0)
                {
                    return(SPI_FLASH_WORKING);
                }
                if(MAP_SSIDataPutNonBlocking(pState->ui32Base, 0) == 0)
                {
                    //
                    // THe dummy byte could not be written, so return
                    // indicating that the transfer is still in progress.
                    //
                    return(SPI_FLASH_WORKING);
                }
                else
                {
                    //
                    // The dummy byte has been written, so move to the read
                    // data setup state.
                    //
                    pState->ui16State = STATE_READ_DATA_SETUP;

                    //
                    // Decrement the count of bytes that have been written.
                    //
                    ui32Count--;
                }

                //
                // Done with this state.
                //
                break;
            }

            //
            // The state machine is in the read data setup state.
            //
            case STATE_READ_DATA_SETUP:
            {
                //
                // Set the SSI module into the appropriate mode based on the
                // command byte.
                //
                if(pState->ui16Cmd == CMD_DREAD)
                {
                    //
                    // Bi-SPI read mode is used for the dual read command.
                    //
                    MAP_SSIAdvModeSet(pState->ui32Base, SSI_ADV_MODE_BI_READ);
                }
                else if(pState->ui16Cmd == CMD_QREAD)
                {
                    //
                    // Quad-SPI read mode is used for the quad read command.
                    //
                    MAP_SSIAdvModeSet(pState->ui32Base,
                                      SSI_ADV_MODE_QUAD_READ);
                }
                else
                {
                    //
                    // Advanced read/write mode is used for the read and fast
                    // read commands.
                    //
                    MAP_SSIAdvModeSet(pState->ui32Base,
                                      SSI_ADV_MODE_READ_WRITE);
                }

                //
                // See if a single byte is being transferred.
                //
                if(pState->ui32ReadCount == 1)
                {
                    //
                    // Disable the use of uDMA.
                    //
                    pState->bUseDMA = false;

                    //
                    // Move to the read data end state to transfer the single
                    // byte.  This uses PIO even if uDMA has been requested.
                    //
                    pState->ui16State = STATE_READ_DATA_END;
                }

                //
                // See if uDMA has been requested for this transfer.
                //
                else if(!pState->bUseDMA || (pState->ui32ReadCount < 4))
                {
                    //
                    // Disable the use of uDMA.
                    //
                    pState->bUseDMA = false;

                    //
                    // Move to the read data state.
                    //
                    pState->ui16State = STATE_READ_DATA;
                }

                //
                // This transfer should use uDMA.
                //
                else
                {
                    //
                    // If the transfer is larger than 1024 bytes, enable the
                    // uDMA receive complete interrupt which will be used to
                    // move to the next block of the transfer.  Otherwise,
                    // enable the uDMA transmit complete interrupt which will
                    // be used to complete the transaction.
                    //
                    if(pState->ui32ReadCount > 1024)
                    {
                        HWREG(pState->ui32Base + SSI_O_IM) = SSI_IM_DMARXIM;
                    }
                    else
                    {
                        HWREG(pState->ui32Base + SSI_O_IM) = SSI_IM_DMATXIM;
                    }

                    //
                    // Disable the uDMA channels.
                    //
                    HWREG(UDMA_ENACLR) = ((1 << pState->ui32TxChannel) |
                                          (1 << pState->ui32RxChannel));

                    //
                    // Configure the attributes for the transmit uDMA channel.
                    //
                    HWREG(UDMA_USEBURSTSET) = ((1 << pState->ui32TxChannel) |
                                               (1 << pState->ui32RxChannel));
                    HWREG(UDMA_ALTCLR) = ((1 << pState->ui32TxChannel) |
                                          (1 << pState->ui32RxChannel));
                    HWREG(UDMA_PRIOCLR) = 1 << pState->ui32TxChannel;
                    HWREG(UDMA_PRIOSET) = 1 << pState->ui32RxChannel;
                    HWREG(UDMA_REQMASKCLR) = ((1 << pState->ui32TxChannel) |
                                              (1 << pState->ui32RxChannel));

                    //
                    // Configure the control parameters of the uDMA channels.
                    //
                    uDMAChannelControlSet(pState->ui32TxChannel,
                                          UDMA_SRC_INC_NONE |
                                          UDMA_DST_INC_NONE |
                                          UDMA_SIZE_8 | UDMA_ARB_2);
                    uDMAChannelControlSet(pState->ui32RxChannel,
                                          UDMA_SRC_INC_NONE |
                                          UDMA_DST_INC_8 |
                                          UDMA_SIZE_8 | UDMA_ARB_4);

                    //
                    // Configure the uDMA receive channel to transfer the first
                    // portion of the data buffer.
                    //
                    uDMAChannelTransferSet(pState->ui32RxChannel,
                                           UDMA_MODE_BASIC,
                                           (void *)(pState->ui32Base +
                                                    SSI_O_DR),
                                           pState->pui8Buffer,
                                           (pState->ui32ReadCount >= 1024) ?
                                           1024 : pState->ui32ReadCount);

                    //
                    // Enable the uDMA receive channel.
                    //
                    uDMAChannelEnable(pState->ui32RxChannel);

                    //
                    // Configure the uDMA channel to transfer the dummy bytes
                    // for the first portion of the data buffer.  The last
                    // dummy byte will not be included since it must be treated
                    // special.
                    //
                    uDMAChannelTransferSet(pState->ui32TxChannel,
                                           UDMA_MODE_BASIC,
                                           pState->pui8Buffer,
                                           (void *)(pState->ui32Base +
                                                    SSI_O_DR),
                                           (pState->ui32WriteCount > 1024) ?
                                           1024 : pState->ui32WriteCount - 1);

                    //
                    // Enable the uDMA transmit channel.
                    //
                    uDMAChannelEnable(pState->ui32TxChannel);

                    //
                    // Clear any previously pending uDMA completion interrupt.
                    //
                    HWREG(pState->ui32Base + SSI_O_ICR) = SSI_ICR_DMARXIC;

                    //
                    // Enable uDMA transmit and receive in the SSI module.
                    //
                    MAP_SSIDMAEnable(pState->ui32Base,
                                     SSI_DMA_TX | SSI_DMA_RX);

                    //
                    // Move to the uDMA data read state.
                    //
                    pState->ui16State = STATE_READ_DATA_DMA;
                }

                //
                // Done with this state.
                //
                break;
            }

            //
            // The state machine is in the read data state.
            //
            case STATE_READ_DATA:
            {
                //
                // Loop while there is more than one byte left to write.
                //
                while(pState->ui32WriteCount != 1)
                {
                    //
                    // Dummy bytes are written into the FIFO in order to
                    // trigger the read operation.  Attempt to write another
                    // dummy byte into the FIFO.
                    //
                    if(ui32Count == 0)
                    {
                        return(SPI_FLASH_WORKING);
                    }
                    if(MAP_SSIDataPutNonBlocking(pState->ui32Base, 0) == 0)
                    {
                        //
                        // The dummy byte could not be written, so return
                        // indicating that the transfer is still in progress.
                        //
                        return(SPI_FLASH_WORKING);
                    }

                    //
                    // Decrement the count of dummy bytes to write.
                    //
                    pState->ui32WriteCount--;

                    //
                    // Decrement the count of bytes that have been written.
                    //
                    ui32Count--;
                }

                //
                // Move to the read data end state.
                //
                pState->ui16State = STATE_READ_DATA_END;

                //
                // Done with this state.
                //
                break;
            }

            //
            // The state machine is in the uDMA read data state.
            //
            case STATE_READ_DATA_DMA:
            {
                //
                // See if the write count is greater than one.
                //
                if(pState->ui32WriteCount > 1)
                {
                    //
                    // Return indicating that the transfer is still in
                    // progress.
                    //
                    return(SPI_FLASH_WORKING);
                }

                //
                // Disable uDMA transmit in the SSI module.
                //
                MAP_SSIDMADisable(pState->ui32Base, SSI_DMA_TX);

                //
                // Enable the uDMA receive done and FIFO transmit interrupt.
                //
                HWREG(pState->ui32Base + SSI_O_IM) =
                    SSI_IM_DMARXIM | SSI_IM_TXIM;

                //
                // Move to the read data end state.
                //
                pState->ui16State = STATE_READ_DATA_END;

                //
                // Done with this state.
                //
                break;
            }

            //
            // The state machine is in the data read end state.
            //
            case STATE_READ_DATA_END:
            {
                //
                // See if the final dummy byte still needs to be written.
                //
                if(pState->ui32WriteCount != 0)
                {
                    //
                    // Attempt to write the final dummy byte into the FIFO and
                    // mark it as the end of the frame.
                    //
                    if(ui32Count == 0)
                    {
                        return(SPI_FLASH_WORKING);
                    }
                    if(MAP_SSIAdvDataPutFrameEndNonBlocking(pState->ui32Base,
                                                            0) == 0)
                    {
                        //
                        // The dummy byte could not be written, so return
                        // indicating that the transfer is still in progress.
                        //
                        return(SPI_FLASH_WORKING);
                    }

                    //
                    // The write portion of the transfer has completed.
                    //
                    pState->ui32WriteCount = 0;

                    //
                    // Disable the transmit interrupt now that the write
                    // write portion of the transfer has completed.
                    //
                    HWREG(pState->ui32Base + SSI_O_IM) &= ~(SSI_IM_TXIM);
                }

                //
                // Return indicating that the transfer is still in progress if
                // there are still data bytes to be read.
                //
                if(pState->ui32ReadCount != 0)
                {
                    return(SPI_FLASH_WORKING);
                }

                //
                // Disable uDMA receive in the SSI module.
                //
                MAP_SSIDMADisable(pState->ui32Base, SSI_DMA_RX);

                //
                // The transfer is complete, so disable all interrupts.
                //
                HWREG(pState->ui32Base + SSI_O_IM) = 0;

                //
                // Move to the idle state.
                //
                pState->ui16State = STATE_IDLE;

                //
                // Return indicating that the transfer has completed.
                //
                return(SPI_FLASH_DONE);
            }

            //
            // The state machine is in the write data setup state.
            //
            case STATE_WRITE_DATA_SETUP:
            {
                //
                // See if a single data byte is being transferred.
                //
                if(pState->ui32WriteCount == 1)
                {
                    //
                    // Disable the use of uDMA.
                    //
                    pState->bUseDMA = false;

                    //
                    // Move to the write data end state to transfer the single
                    // byte.  This uses PIO even if uDMA has been requested.
                    //
                    pState->ui16State = STATE_WRITE_DATA_END;
                }

                //
                // See if uDMA has been requested for this transfer.
                //
                else if(!pState->bUseDMA || (pState->ui32WriteCount < 4))
                {
                    //
                    // Disable the use of uDMA.
                    //
                    pState->bUseDMA = false;

                    //
                    // uDMA is not being used, so move to the write data state.
                    //
                    pState->ui16State = STATE_WRITE_DATA;
                }

                //
                // This transfer should use uDMA.
                //
                else
                {
                    //
                    // Enable the uDMA transmit complete interrupt.
                    //
                    HWREG(pState->ui32Base + SSI_O_IM) = SSI_IM_DMATXIM;

                    //
                    // Disable the transmit uDMA channel.
                    //
                    HWREG(UDMA_ENACLR) = 1 << pState->ui32TxChannel;

                    //
                    // Configure the attributes for the transmit uDMA channel.
                    //
                    HWREG(UDMA_USEBURSTSET) = 1 << pState->ui32TxChannel;
                    HWREG(UDMA_ALTCLR) = 1 << pState->ui32TxChannel;
                    HWREG(UDMA_PRIOCLR) = 1 << pState->ui32TxChannel;
                    HWREG(UDMA_REQMASKCLR) = 1 << pState->ui32TxChannel;

                    //
                    // Configure the control parameters of the uDMA channel.
                    //
                    uDMAChannelControlSet(pState->ui32TxChannel,
                                          UDMA_SRC_INC_8 |
                                          UDMA_DST_INC_NONE |
                                          UDMA_SIZE_8 | UDMA_ARB_4);

                    //
                    // Configure the uDMA channel to transfer the next portion
                    // of the data buffer.  The last byte in the buffer will
                    // not be included since it must be treated special.
                    //
                    uDMAChannelTransferSet(pState->ui32TxChannel,
                                           UDMA_MODE_BASIC,
                                           pState->pui8Buffer,
                                           (void *)(pState->ui32Base +
                                                    SSI_O_DR),
                                           (pState->ui32WriteCount > 1024) ?
                                           1024 : pState->ui32WriteCount - 1);

                    //
                    // Enable the uDMA transmit channel.
                    //
                    uDMAChannelEnable(pState->ui32TxChannel);
                    HWREG(pState->ui32Base + SSI_O_ICR) = SSI_ICR_DMATXIC;

                    //
                    // Enable uDMA in the SSI module.
                    //
                    MAP_SSIDMAEnable(pState->ui32Base, SSI_DMA_TX);

                    //
                    // Move to the uDMA data write state.
                    //
                    pState->ui16State = STATE_WRITE_DATA_DMA;
                }

                //
                // Done with this state.
                //
                break;
            }

            //
            // The state machine is in the write data state.
            //
            case STATE_WRITE_DATA:
            {
                //
                // Loop while there is more than one byte left to write.
                //
                while(pState->ui32WriteCount != 1)
                {
                    //
                    // Attempt to write the next data byte into the FIFO.
                    //
                    if(ui32Count == 0)
                    {
                        return(SPI_FLASH_WORKING);
                    }
                    if(MAP_SSIDataPutNonBlocking(pState->ui32Base,
                                                 *(pState->pui8Buffer)) == 0)
                    {
                        //
                        // The next data byte could not be written, so return
                        // indicating that the transfer is still in progress.
                        //
                        return(SPI_FLASH_WORKING);
                    }

                    //
                    // Increment the buffer pointer and decrement the byte
                    // count.
                    //
                    pState->pui8Buffer++;
                    pState->ui32WriteCount--;

                    //
                    // Decrement the count of bytes that have been written.
                    //
                    ui32Count--;
                }

                //
                // Move to the write data end state.
                //
                pState->ui16State = STATE_WRITE_DATA_END;

                //
                // Done with this state.
                //
                break;
            }

            //
            // The state machine is in the uDMA write data state.
            //
            case STATE_WRITE_DATA_DMA:
            {
                //
                // See if the write count is greater than one.
                //
                if(pState->ui32WriteCount > 1)
                {
                    //
                    // Return indicating that the transfer is still in
                    // progress.
                    //
                    return(SPI_FLASH_WORKING);
                }

                //
                // Disable uDMA in the SSI module.
                //
                MAP_SSIDMADisable(pState->ui32Base, SSI_DMA_TX);

                //
                // Disable the uDMA transmit complete interrupt and enable the
                // FIFO interrupt.
                //
                HWREG(pState->ui32Base + SSI_O_IM) = SSI_IM_TXIM;

                //
                // Move to the write data end state.
                //
                pState->ui16State = STATE_WRITE_DATA_END;

                //
                // Done with this state.
                //
                break;
            }

            //
            // The state machine is in the write data end state.
            //
            case STATE_WRITE_DATA_END:
            {
                //
                // Attempt to write the final data byte into the FIFO.
                //
                if(MAP_SSIAdvDataPutFrameEndNonBlocking(pState->ui32Base,
                                                      *(pState->pui8Buffer)) ==
                   0)
                {
                    //
                    // The final data byte could not be written, so return
                    // indicating that the transfer is still in progress.
                    //
                    return(SPI_FLASH_WORKING);
                }

                //
                // The transfer is complete, so disable all interrupts.
                //
                HWREG(pState->ui32Base + SSI_O_IM) = 0;

                //
                // Move to the idle state.
                //
                pState->ui16State = STATE_IDLE;

                //
                // Return indicating that the transfer has completed.
                //
                return(SPI_FLASH_DONE);
            }
        }
    }
}

//*****************************************************************************
//
//! Initializes the SPI flash driver.
//!
//! \param ui32Base is the SSI module base address.
//! \param ui32Clock is the rate of the clock supplied to the SSI module.
//! \param ui32BitRate is the SPI clock rate.
//!
//! This function configures the SSI module for use by the SPI flash driver.
//! The SSI module will be placed into the correct mode of operation to allow
//! communication with the SPI flash.  This function must be called prior to
//! calling the remaining SPI flash driver APIs.  It can be called at a later
//! point to reconfigure the SSI module, such as to increase the SPI clock rate
//! once it has been determined that it is safe to use a higher speed clock.
//!
//! It is the responsibility of the caller to enable the SSI module and
//! configure the pins that it will utilize.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashInit(uint32_t ui32Base, uint32_t ui32Clock, uint32_t ui32BitRate)
{
    //
    // Configure the SPI module.
    //
    MAP_SSIConfigSetExpClk(ui32Base, ui32Clock, SSI_FRF_MOTO_MODE_0,
                           SSI_MODE_MASTER, ui32BitRate, 8);

    //
    // Enable the advanced mode of operation, defaulting to read/write mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_READ_WRITE);

    //
    // Enable the frame hold feature.
    //
    MAP_SSIAdvFrameHoldEnable(ui32Base);

    //
    // Enable the SPI module.
    //
    MAP_SSIEnable(ui32Base);
}

//*****************************************************************************
//
//! Writes the SPI flash status register.
//!
//! \param ui32Base is the SSI module base address.
//! \param ui8Status is the value to write to the status register.
//!
//! This function writes the SPI flash status register.  This uses the 0x01 SPI
//! flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashWriteStatus(uint32_t ui32Base, uint8_t ui8Status)
{
    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the write status register command.
    //
    MAP_SSIDataPut(ui32Base, CMD_WRSR);

    //
    // Send the new status register value, marking this byte as the end of the
    // frame.
    //
    MAP_SSIAdvDataPutFrameEnd(ui32Base, ui8Status);
}

//*****************************************************************************
//
//! Programs the SPI flash.
//!
//! \param ui32Base is the SSI module base address.
//! \param ui32Addr is the SPI flash address to be programmed.
//! \param pui8Data is a pointer to the data to be programmed.
//! \param ui32Count is the number of bytes to be programmed.
//!
//! This function programs data into the SPI flash, using PIO mode.  This
//! function will not return until the entire program command has been written
//! into the SSI transmit FIFO.  This uses the 0x02 SPI flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashPageProgram(uint32_t ui32Base, uint32_t ui32Addr,
                    const uint8_t *pui8Data, uint32_t ui32Count)
{
    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the page program command.
    //
    MAP_SSIDataPut(ui32Base, CMD_PP);

    //
    // Send the address of the first byte to program.
    //
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 16) & 0xff);
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 8) & 0xff);
    MAP_SSIDataPut(ui32Base, ui32Addr & 0xff);

    //
    // Loop while there is more than one data byte left to be sent.
    //
    while(ui32Count-- != 1)
    {
        //
        // Send the next data byte.
        //
        MAP_SSIDataPut(ui32Base, *pui8Data++);
    }

    //
    // Send the last data byte, marking it as the end of the frame.
    //
    MAP_SSIAdvDataPutFrameEnd(ui32Base, *pui8Data);
}

//*****************************************************************************
//
//! Programs the SPI flash in the background.
//!
//! \param pState is a pointer to the SPI flash state structure.
//! \param ui32Base is the SSI module base address.
//! \param ui32Addr is the SPI flash address to be programmed.
//! \param pui8Data is a pointer to the data to be programmed.
//! \param ui32Count is the number of bytes to be programmed.
//! \param bUseDMA is \b true if uDMA should be used and \b false otherwise.
//! \param ui32TxChannel is the uDMA channel to be used for writing to the SSI
//! module.
//!
//! This function programs data into the SPI flash, using either interrupts or
//! uDMA to transfer the data.  This function will return immediately and send
//! the data in the background.  In order for this to complete successfully,
//! several conditions must be satisfied:
//!
//! - Prior to calling this function:
//!   - The SSI module must be enabled in SysCtl.
//!   - The SSI pins must be configured for use by the SSI module.
//!   - The SSI module interrupt must be enabled in NVIC.
//!   - The uDMA module must be enabled in SysCtl and the control table set (if
//!     using uDMA).
//!   - The uDMA channels must be assigned to the SSI module.
//!
//! - After calling this function:
//!   - The interrupt handler for the SSI module must call
//!     SPIFlashIntHandler(), passing the same pState structure pointer that
//!     was supplied to this function.
//!   - No other SPI flash operation can be called until this operation has
//!     completed.
//!
//! Completion of the programming operation is indicated when
//! SPIFlashIntHandler() returns \b SPI_FLASH_DONE.
//!
//! Like SPIFlashPageProgram(), this uses the 0x02 SPI flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashPageProgramNonBlocking(tSPIFlashState *pState, uint32_t ui32Base,
                               uint32_t ui32Addr, const uint8_t *pui8Data,
                               uint32_t ui32Count, bool bUseDMA,
                               uint32_t ui32TxChannel)
{
    //
    // Save the parameters of this program operation to the state structure.
    //
    pState->ui32Base = ui32Base;
    pState->ui16Cmd = CMD_PP;
    pState->ui16State = STATE_CMD;
    pState->ui32Addr = ui32Addr;
    pState->pui8Buffer = (uint8_t *)pui8Data;
    pState->ui32ReadCount = 0;
    pState->ui32WriteCount = ui32Count;
    pState->bUseDMA = bUseDMA;
    pState->ui32TxChannel = ui32TxChannel & 0x1f;

    //
    // Enable the SSI transmit interrupt.  This will start the transfer.  If
    // uDMA is being used, the uDMA-related interrupt will be enabled at the
    // appropriate time by the interrupt handler.
    //
    HWREG(ui32Base + SSI_O_ICR) = SSI_ICR_DMATXIC;
    HWREG(ui32Base + SSI_O_IM) = SSI_IM_TXIM;
}

//*****************************************************************************
//
//! Reads data from the SPI flash.
//!
//! \param ui32Base is the SSI module base address.
//! \param ui32Addr is the SPI flash address to read.
//! \param pui8Data is a pointer to the data buffer to into which to read the
//! data.
//! \param ui32Count is the number of bytes to read.
//!
//! This function reads data from the SPI flash, using PIO mode.  This function
//! will not return until the read has completed.  This uses the 0x03 SPI flash
//! command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashRead(uint32_t ui32Base, uint32_t ui32Addr, uint8_t *pui8Data,
             uint32_t ui32Count)
{
    uint32_t ui32Trash;

    //
    // Drain any residual data from the receive FIFO.
    //
    while(MAP_SSIDataGetNonBlocking(ui32Base, &ui32Trash) != 0)
    {
    }

    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the read command.
    //
    MAP_SSIDataPut(ui32Base, CMD_READ);

    //
    // Send the address of the first byte to read.
    //
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 16) & 0xff);
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 8) & 0xff);
    MAP_SSIDataPut(ui32Base, ui32Addr & 0xff);

    //
    // Set the SSI module into read/write mode.  In this mode, dummy writes are
    // required in order to make the transfer occur; the SPI flash will ignore
    // the data.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_READ_WRITE);

    //
    // See if there is a single byte to be read.
    //
    if(ui32Count == 1)
    {
        //
        // Perform a single dummy write, marking it as the end of the frame.
        //
        MAP_SSIAdvDataPutFrameEnd(ui32Base, 0);
    }
    else
    {
        //
        // Perform a dummy write to prime the loop.
        //
        MAP_SSIDataPut(ui32Base, 0);

        //
        // Loop while there is more than one byte left to be read.
        //
        while(--ui32Count != 1)
        {
            //
            // Perform a dummy write to keep the transmit FIFO from going
            // empty.
            //
            MAP_SSIDataPut(ui32Base, 0);

            //
            // Read the next data byte from the receive FIFO and place it into
            // the data buffer.
            //
            MAP_SSIDataGet(ui32Base, &ui32Addr);
            *pui8Data++ = ui32Addr & 0xff;
        }

        //
        // Perform the final dummy write, marking it as the end of the frame.
        //
        MAP_SSIAdvDataPutFrameEnd(ui32Base, 0);

        //
        // Read the next data byte from the receive FIFO and place it into the
        // data buffer.
        //
        MAP_SSIDataGet(ui32Base, &ui32Addr);
        *pui8Data++ = ui32Addr & 0xff;
    }

    //
    // Read the final data byte from the receive FIFO and place it into the
    // data buffer.
    //
    MAP_SSIDataGet(ui32Base, &ui32Addr);
    *pui8Data++ = ui32Addr & 0xff;
}

//*****************************************************************************
//
//! Reads data from the SPI flash in the background.
//!
//! \param pState is a pointer to the SPI flash state structure.
//! \param ui32Base is the SSI module base address.
//! \param ui32Addr is the SPI flash address to read.
//! \param pui8Data is a pointer to the data buffer to into which to read the
//! data.
//! \param ui32Count is the number of bytes to read.
//! \param bUseDMA is \b true if uDMA should be used and \b false otherwise.
//! \param ui32TxChannel is the uDMA channel to be used for writing to the SSI
//! module.
//! \param ui32RxChannel is the uDMA channel to be used for reading from the
//! SSI module.
//!
//! This function reads data from the SPI flash, using either interrupts or
//! uDMA to transfer the data.  This function will return immediately and read
//! the data in the background.  In order for this to complete successfully,
//! several conditions must be satisfied:
//!
//! - Prior to calling this function:
//!   - The SSI module must be enabled in SysCtl.
//!   - The SSI pins must be configured for use by the SSI module.
//!   - The SSI module interrupt must be enabled in NVIC.
//!   - The uDMA module must be enabled in SysCtl and the control table set (if
//!     using uDMA).
//!   - The uDMA channels must be assigned to the SSI module.
//!
//! - After calling this function:
//!   - The interrupt handler for the SSI module must call
//!     SPIFlashIntHandler(), passing the same pState structure pointer that
//!     was supplied to this function.
//!   - No other SPI flash operation can be called until this operation has
//!     completed.
//!
//! Completion of the read operation is indicated when SPIFlashIntHandler()
//! returns \b SPI_FLASH_DONE.
//!
//! Like SPIFlashRead(), this uses the 0x03 SPI flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashReadNonBlocking(tSPIFlashState *pState, uint32_t ui32Base,
                        uint32_t ui32Addr, uint8_t *pui8Data,
                        uint32_t ui32Count, bool bUseDMA,
                        uint32_t ui32TxChannel, uint32_t ui32RxChannel)
{
    uint32_t ui32Trash;

    //
    // Drain any residual data from the receive FIFO.
    //
    while(MAP_SSIDataGetNonBlocking(ui32Base, &ui32Trash) != 0)
    {
    }

    //
    // Save the parameters of this read operation to the state structure.
    //
    pState->ui32Base = ui32Base;
    pState->ui16Cmd = CMD_READ;
    pState->ui16State = STATE_CMD;
    pState->ui32Addr = ui32Addr;
    pState->pui8Buffer = pui8Data;
    pState->ui32ReadCount = ui32Count;
    pState->ui32WriteCount = ui32Count;
    pState->bUseDMA = bUseDMA;
    pState->ui32TxChannel = ui32TxChannel & 0x1f;
    pState->ui32RxChannel = ui32RxChannel & 0x1f;

    //
    // Enable the SSI transmit and receive interrupts.  This will start the
    // transfer.  If uDMA is being used, the uDMA-related interrupts will be
    // enabled at the appropriate time by the interrupt handler.
    //
    HWREG(ui32Base + SSI_O_ICR) = SSI_ICR_DMATXIC | SSI_ICR_DMARXIC;
    HWREG(ui32Base + SSI_O_IM) = SSI_IM_TXIM | SSI_IM_RXIM | SSI_IM_RTIM;
}

//*****************************************************************************
//
//! Disables SPI flash write operations.
//!
//! \param ui32Base is the SSI module base address.
//!
//! This function sets the SPI flash to disallow program and erase operations.
//! This uses the 0x04 SPI flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashWriteDisable(uint32_t ui32Base)
{
    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the write disable command, marking this byte as the end of the
    // frame.
    //
    MAP_SSIAdvDataPutFrameEnd(ui32Base, CMD_WRDI);
}

//*****************************************************************************
//
//! Reads the SPI flash status register.
//!
//! \param ui32Base is the SSI module base address.
//!
//! This function reads the SPI flash status register.  This uses the 0x05 SPI
//! flash command.
//!
//! \return Returns the value of the SPI flash status register.
//
//*****************************************************************************
uint8_t
SPIFlashReadStatus(uint32_t ui32Base)
{
    uint32_t ui32Data;

    //
    // Drain any residual data from the receive FIFO.
    //
    while(MAP_SSIDataGetNonBlocking(ui32Base, &ui32Data) != 0)
    {
    }

    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the write status register command.
    //
    MAP_SSIDataPut(ui32Base, CMD_RDSR);

    //
    // Set the SSI module into read/write mode.  In this mode, dummy writes are
    // required in order to make the transfer occur; the SPI flash will ignore
    // the data.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_READ_WRITE);

    //
    // Perform a single dummy write, marking it as the end of the frame.
    //
    MAP_SSIAdvDataPutFrameEnd(ui32Base, 0);

    //
    // Read the value of the status register.
    //
    MAP_SSIDataGet(ui32Base, &ui32Data);

    //
    // Return the status register value.
    //
    return(ui32Data & 0xff);
}

//*****************************************************************************
//
//! Enables SPI flash write operations.
//!
//! \param ui32Base is the SSI module base address.
//!
//! This function sets the SPI flash to allow program and erase operations.
//! This must be done prior to each SPI flash program or erase operation; the
//! SPI flash will automatically disable program and erase operations once a
//! program or erase operation has completed.  This uses the 0x06 SPI flash
//! command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashWriteEnable(uint32_t ui32Base)
{
    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the write enable command, marking this byte as the end of the
    // frame.
    //
    MAP_SSIAdvDataPutFrameEnd(ui32Base, CMD_WREN);
}

//*****************************************************************************
//
//! Reads data from the SPI flash using the fast read command.
//!
//! \param ui32Base is the SSI module base address.
//! \param ui32Addr is the SPI flash address to read.
//! \param pui8Data is a pointer to the data buffer to into which to read the
//! data.
//! \param ui32Count is the number of bytes to read.
//!
//! This function reads data from the SPI flash with the fast read command,
//! using PIO mode.  The fast read command allows the SPI flash to be read at
//! a higher SPI clock rate because of the addition of a dummy cycle during the
//! command setup.  This function will not return until the read has completed.
//! This uses the 0x0b SPI flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashFastRead(uint32_t ui32Base, uint32_t ui32Addr, uint8_t *pui8Data,
                 uint32_t ui32Count)
{
    uint32_t ui32Trash;

    //
    // Drain any residual data from the receive FIFO.
    //
    while(MAP_SSIDataGetNonBlocking(ui32Base, &ui32Trash) != 0)
    {
    }

    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the fast read command.
    //
    MAP_SSIDataPut(ui32Base, CMD_FREAD);

    //
    // Send the address of the first byte to read.
    //
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 16) & 0xff);
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 8) & 0xff);
    MAP_SSIDataPut(ui32Base, ui32Addr & 0xff);

    //
    // Send a dummy byte.
    //
    MAP_SSIDataPut(ui32Base, 0);

    //
    // Set the SSI module into read/write mode.  In this mode, dummy writes are
    // required in order to make the transfer occur; the SPI flash will ignore
    // the data.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_READ_WRITE);

    //
    // See if there is a single byte to be read.
    //
    if(ui32Count == 1)
    {
        //
        // Perform a single dummy write, marking it as the end of the frame.
        //
        MAP_SSIAdvDataPutFrameEnd(ui32Base, 0);
    }
    else
    {
        //
        // Perform a dummy write to prime the loop.
        //
        MAP_SSIDataPut(ui32Base, 0);

        //
        // Loop while there is more than one byte left to be read.
        //
        while(--ui32Count != 1)
        {
            //
            // Perform a dummy write to keep the transmit FIFO from going
            // empty.
            //
            MAP_SSIDataPut(ui32Base, 0);

            //
            // Read the next data byte from the receive FIFO and place it into
            // the data buffer.
            //
            MAP_SSIDataGet(ui32Base, &ui32Addr);
            *pui8Data++ = ui32Addr & 0xff;
        }

        //
        // Perform the final dummy write, marking it as the end of the frame.
        //
        MAP_SSIAdvDataPutFrameEnd(ui32Base, 0);

        //
        // Read the next data byte from the receive FIFO and place it into the
        // data buffer.
        //
        MAP_SSIDataGet(ui32Base, &ui32Addr);
        *pui8Data++ = ui32Addr & 0xff;
    }

    //
    // Read the final data byte from the receive FIFO and place it into the
    // data buffer.
    //
    MAP_SSIDataGet(ui32Base, &ui32Addr);
    *pui8Data++ = ui32Addr & 0xff;
}

//*****************************************************************************
//
//! Reads data from the SPI flash using the fast read command in the
//! background.
//!
//! \param pState is a pointer to the SPI flash state structure.
//! \param ui32Base is the SSI module base address.
//! \param ui32Addr is the SPI flash address to read.
//! \param pui8Data is a pointer to the data buffer to into which to read the
//! data.
//! \param ui32Count is the number of bytes to read.
//! \param bUseDMA is \b true if uDMA should be used and \b false otherwise.
//! \param ui32TxChannel is the uDMA channel to be used for writing to the SSI
//! module.
//! \param ui32RxChannel is the uDMA channel to be used for reading from the
//! SSI module.
//!
//! This function reads data from the SPI flash with the fast read command,
//! using either interrupts or uDMA to transfer the data.  The fast read
//! command allows the SPI flash to be read at a higher SPI clock rate because
//! of the addition of a dummy cycle during the command setup.  This function
//! will return immediately and read the data in the background.  In order for
//! this to complete successfully, several conditions must be satisfied:
//!
//! - Prior to calling this function:
//!   - The SSI module must be enabled in SysCtl.
//!   - The SSI pins must be configured for use by the SSI module.
//!   - The SSI module interrupt must be enabled in NVIC.
//!   - The uDMA module must be enabled in SysCtl and the control table set (if
//!     using uDMA).
//!   - The uDMA channels must be assigned to the SSI module.
//!
//! - After calling this function:
//!   - The interrupt handler for the SSI module must call
//!     SPIFlashIntHandler(), passing the same pState structure pointer that
//!     was supplied to this function.
//!   - No other SPI flash operation can be called until this operation has
//!     completed.
//!
//! Completion of the read operation is indicated when SPIFlashIntHandler()
//! returns \b SPI_FLASH_DONE.
//!
//! Like SPIFlashFastRead(), this uses the 0x0b SPI flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashFastReadNonBlocking(tSPIFlashState *pState, uint32_t ui32Base,
                            uint32_t ui32Addr, uint8_t *pui8Data,
                            uint32_t ui32Count, bool bUseDMA,
                            uint32_t ui32TxChannel, uint32_t ui32RxChannel)
{
    uint32_t ui32Trash;

    //
    // Drain any residual data from the receive FIFO.
    //
    while(MAP_SSIDataGetNonBlocking(ui32Base, &ui32Trash) != 0)
    {
    }

    //
    // Save the parameters of this read operation to the state structure.
    //
    pState->ui32Base = ui32Base;
    pState->ui16Cmd = CMD_FREAD;
    pState->ui16State = STATE_CMD;
    pState->ui32Addr = ui32Addr;
    pState->pui8Buffer = pui8Data;
    pState->ui32ReadCount = ui32Count;
    pState->ui32WriteCount = ui32Count;
    pState->bUseDMA = bUseDMA;
    pState->ui32TxChannel = ui32TxChannel & 0x1f;
    pState->ui32RxChannel = ui32RxChannel & 0x1f;

    //
    // Enable the SSI transmit and receive interrupts.  This will start the
    // transfer.  If uDMA is being used, the uDMA-related interrupts will be
    // enabled at the appropriate time by the interrupt handler.
    //
    HWREG(ui32Base + SSI_O_ICR) = SSI_ICR_DMATXIC | SSI_ICR_DMARXIC;
    HWREG(ui32Base + SSI_O_IM) = SSI_IM_TXIM | SSI_IM_RXIM | SSI_IM_RTIM;
}

//*****************************************************************************
//
//! Erases a 4 KB sector of the SPI flash.
//!
//! \param ui32Base is the SSI module base address.
//! \param ui32Addr is the SPI flash address to erase.
//!
//! This function erases a sector of the SPI flash.  Each sector is 4 KB with a
//! 4 KB alignment; the SPI flash will ignore the lower ten bits of the address
//! provided.  The sector erase command is issued by this function;
//! SPIFlashReadStatus() must be used to query the SPI flash to determine when
//! the sector erase operation has completed.  This uses the 0x20 SPI flash
//! command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashSectorErase(uint32_t ui32Base, uint32_t ui32Addr)
{
    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the sector erase command.
    //
    MAP_SSIDataPut(ui32Base, CMD_SE);

    //
    // Send the address of the sector to be erased, marking the last byte of
    // the address as the end of the frame.
    //
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 16) & 0xff);
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 8) & 0xff);
    MAP_SSIAdvDataPutFrameEnd(ui32Base, ui32Addr & 0xff);
}

//*****************************************************************************
//
//! Reads data from the SPI flash using Bi-SPI.
//!
//! \param ui32Base is the SSI module base address.
//! \param ui32Addr is the SPI flash address to read.
//! \param pui8Data is a pointer to the data buffer to into which to read the
//! data.
//! \param ui32Count is the number of bytes to read.
//!
//! This function reads data from the SPI flash with Bi-SPI, using PIO mode.
//! This function will not return until the read has completed.  This uses the
//! 0x3b SPI flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashDualRead(uint32_t ui32Base, uint32_t ui32Addr, uint8_t *pui8Data,
                 uint32_t ui32Count)
{
    uint32_t ui32Trash;

    //
    // Drain any residual data from the receive FIFO.
    //
    while(MAP_SSIDataGetNonBlocking(ui32Base, &ui32Trash) != 0)
    {
    }

    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the dual read command.
    //
    MAP_SSIDataPut(ui32Base, CMD_DREAD);

    //
    // Send the address of the first byte to read.
    //
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 16) & 0xff);
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 8) & 0xff);
    MAP_SSIDataPut(ui32Base, ui32Addr & 0xff);

    //
    // Send a dummy byte.
    //
    MAP_SSIDataPut(ui32Base, 0);

    //
    // Set the SSI module into Bi-SPI read mode.  In this mode, dummy writes
    // are required in order to make the transfer occur; the SSI module will
    // ignore the data (the SPI flash will never see the dummy data since
    // Bi-SPI read mode is a uni-directional input mode).
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_BI_READ);

    //
    // See if there is a single byte to be read.
    //
    if(ui32Count == 1)
    {
        //
        // Perform a single dummy write, marking it as the end of the frame.
        //
        MAP_SSIAdvDataPutFrameEnd(ui32Base, 0);
    }
    else
    {
        //
        // Perform a dummy write to prime the loop.
        //
        MAP_SSIDataPut(ui32Base, 0);

        //
        // Loop while there is more than one byte left to be read.
        //
        while(--ui32Count != 1)
        {
            //
            // Perform a dummy write to keep the transmit FIFO from going
            // empty.
            //
            MAP_SSIDataPut(ui32Base, 0);

            //
            // Read the next data byte from the receive FIFO and place it into
            // the data buffer.
            //
            MAP_SSIDataGet(ui32Base, &ui32Addr);
            *pui8Data++ = ui32Addr & 0xff;
        }

        //
        // Perform the final dummy write, marking it as the end of the frame.
        //
        MAP_SSIAdvDataPutFrameEnd(ui32Base, 0);

        //
        // Read the next data byte from the receive FIFO and place it into the
        // data buffer.
        //
        MAP_SSIDataGet(ui32Base, &ui32Addr);
        *pui8Data++ = ui32Addr & 0xff;
    }

    //
    // Read the final data byte from the receive FIFO and place it into the
    // data buffer.
    //
    MAP_SSIDataGet(ui32Base, &ui32Addr);
    *pui8Data++ = ui32Addr & 0xff;
}

//*****************************************************************************
//
//! Reads data from the SPI flash using Bi-SPI in the background.
//!
//! \param pState is a pointer to the SPI flash state structure.
//! \param ui32Base is the SSI module base address.
//! \param ui32Addr is the SPI flash address to read.
//! \param pui8Data is a pointer to the data buffer to into which to read the
//! data.
//! \param ui32Count is the number of bytes to read.
//! \param bUseDMA is \b true if uDMA should be used and \b false otherwise.
//! \param ui32TxChannel is the uDMA channel to be used for writing to the SSI
//! module.
//! \param ui32RxChannel is the uDMA channel to be used for reading from the
//! SSI module.
//!
//! This function reads data from the SPI flash with Bi-SPI, using either
//! interrupts or uDMA to transfer the data.  This function will return
//! immediately and read the data in the background.  In order for this to
//! complete successfully, several conditions must be satisfied:
//!
//! - Prior to calling this function:
//!   - The SSI module must be enabled in SysCtl.
//!   - The SSI pins must be configured for use by the SSI module.
//!   - The SSI module interrupt must be enabled in NVIC.
//!   - The uDMA module must be enabled in SysCtl and the control table set (if
//!     using uDMA).
//!   - The uDMA channels must be assigned to the SSI module.
//!
//! - After calling this function:
//!   - The interrupt handler for the SSI module must call
//!     SPIFlashIntHandler(), passing the same pState structure pointer that
//!     was supplied to this function.
//!   - No other SPI flash operation can be called until this operation has
//!     completed.
//!
//! Completion of the read operation is indicated when SPIFlashIntHandler()
//! returns \b SPI_FLASH_DONE.
//!
//! Like SPIFLashDualRead(), this uses the 0x3b SPI flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashDualReadNonBlocking(tSPIFlashState *pState, uint32_t ui32Base,
                            uint32_t ui32Addr, uint8_t *pui8Data,
                            uint32_t ui32Count, bool bUseDMA,
                            uint32_t ui32TxChannel, uint32_t ui32RxChannel)
{
    uint32_t ui32Trash;

    //
    // Drain any residual data from the receive FIFO.
    //
    while(MAP_SSIDataGetNonBlocking(ui32Base, &ui32Trash) != 0)
    {
    }

    //
    // Save the parameters of this read operation to the state structure.
    //
    pState->ui32Base = ui32Base;
    pState->ui16Cmd = CMD_DREAD;
    pState->ui16State = STATE_CMD;
    pState->ui32Addr = ui32Addr;
    pState->pui8Buffer = pui8Data;
    pState->ui32ReadCount = ui32Count;
    pState->ui32WriteCount = ui32Count;
    pState->bUseDMA = bUseDMA;
    pState->ui32TxChannel = ui32TxChannel & 0x1f;
    pState->ui32RxChannel = ui32RxChannel & 0x1f;

    //
    // Enable the SSI transmit and receive interrupts.  This will start the
    // transfer.  If uDMA is being used, the uDMA-related interrupts will be
    // enabled at the appropriate time by the interrupt handler.
    //
    HWREG(ui32Base + SSI_O_ICR) = SSI_ICR_DMATXIC | SSI_ICR_DMARXIC;
    HWREG(ui32Base + SSI_O_IM) = SSI_IM_TXIM | SSI_IM_RXIM | SSI_IM_RTIM;
}

//*****************************************************************************
//
//! Erases a 32 KB block of the SPI flash.
//!
//! \param ui32Base is the SSI module base address.
//! \param ui32Addr is the SPI flash address to erase.
//!
//! This function erases a 32 KB block of the SPI flash.  Each 32 KB block has
//! a 32 KB alignment; the SPI flash will ignore the lower 15 bits of the
//! address provided.  The 32 KB block erase command is issued by this
//! function; SPIFlashReadStatus() must be used to query the SPI flash to
//! determine when the 32 KB block erase operation has completed.  This uses
//! the 0x52 SPI flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashBlockErase32(uint32_t ui32Base, uint32_t ui32Addr)
{
    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the 32 KB block erase command command.
    //
    MAP_SSIDataPut(ui32Base, CMD_BE32);

    //
    // Send the address of the 32 KB block to be erased, marking the last byte
    // of the address as the end of the frame.
    //
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 16) & 0xff);
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 8) & 0xff);
    MAP_SSIAdvDataPutFrameEnd(ui32Base, ui32Addr & 0xff);
}

//*****************************************************************************
//
//! Reads data from the SPI flash using Quad-SPI.
//!
//! \param ui32Base is the SSI module base address.
//! \param ui32Addr is the SPI flash address to read.
//! \param pui8Data is a pointer to the data buffer to into which to read the
//! data.
//! \param ui32Count is the number of bytes to read.
//!
//! This function reads data from the SPI flash with Quad-SPI, using PIO mode.
//! This function will not return until the read has completed.  This uses the
//! 0x6b SPI flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashQuadRead(uint32_t ui32Base, uint32_t ui32Addr, uint8_t *pui8Data,
                 uint32_t ui32Count)
{
    uint32_t ui32Trash;

    //
    // Drain any residual data from the receive FIFO.
    //
    while(MAP_SSIDataGetNonBlocking(ui32Base, &ui32Trash) != 0)
    {
    }

    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the quad read command.
    //
    MAP_SSIDataPut(ui32Base, CMD_QREAD);

    //
    // Send the address of the first byte to read.
    //
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 16) & 0xff);
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 8) & 0xff);
    MAP_SSIDataPut(ui32Base, ui32Addr & 0xff);

    //
    // Send a dummy byte.
    //
    MAP_SSIDataPut(ui32Base, 0);

    //
    // Set the SSI module into Quad-SPI read mode.  In this mode, dummy writes
    // are required in order to make the transfer occur; the SSI module will
    // ignore the data (the SPI flash will never see the dummy data since
    // Quad-SPI read mode is a uni-directional input mode).
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_QUAD_READ);

    //
    // See if there is a single byte to be read.
    //
    if(ui32Count == 1)
    {
        //
        // Perform a single dummy write, marking it as the end of the frame.
        //
        MAP_SSIAdvDataPutFrameEnd(ui32Base, 0);
    }
    else
    {
        //
        // Perform a dummy write to prime the loop.
        //
        MAP_SSIDataPut(ui32Base, 0);

        //
        // Loop while there is more than one byte left to be read.
        //
        while(--ui32Count != 1)
        {
            //
            // Perform a dummy write to keep the transmit FIFO from going
            // empty.
            //
            MAP_SSIDataPut(ui32Base, 0);

            //
            // Read the next data byte from the receive FIFO and place it into
            // the data buffer.
            //
            MAP_SSIDataGet(ui32Base, &ui32Addr);
            *pui8Data++ = ui32Addr & 0xff;
        }

        //
        // Perform the final dummy write, marking it as the end of the frame.
        //
        MAP_SSIAdvDataPutFrameEnd(ui32Base, 0);

        //
        // Read the next data byte from the receive FIFO and place it into the
        // data buffer.
        //
        MAP_SSIDataGet(ui32Base, &ui32Addr);
        *pui8Data++ = ui32Addr & 0xff;
    }

    //
    // Read the final data byte from the receive FIFO and place it into the
    // data buffer.
    //
    MAP_SSIDataGet(ui32Base, &ui32Addr);
    *pui8Data++ = ui32Addr & 0xff;
}

//*****************************************************************************
//
//! Reads data from the SPI flash using Quad-SPI in the background.
//!
//! \param pState is a pointer to the SPI flash state structure.
//! \param ui32Base is the SSI module base address.
//! \param ui32Addr is the SPI flash address to read.
//! \param pui8Data is a pointer to the data buffer to into which to read the
//! data.
//! \param ui32Count is the number of bytes to read.
//! \param bUseDMA is \b true if uDMA should be used and \b false otherwise.
//! \param ui32TxChannel is the uDMA channel to be used for writing to the SSI
//! module.
//! \param ui32RxChannel is the uDMA channel to be used for reading from the
//! SSI module.
//!
//! This function reads data from the SPI flash with Quad-SPI, using either
//! interrupts or uDMA to transfer the data.  This function will return
//! immediately and read the data in the background.  In order for this to
//! complete successfully, several conditions must be satisfied:
//!
//! - Prior to calling this function:
//!   - The SSI module must be enabled in SysCtl.
//!   - The SSI pins must be configured for use by the SSI module.
//!   - The SSI module interrupt must be enabled in NVIC.
//!   - The uDMA module must be enabled in SysCtl and the control table set (if
//!     using uDMA).
//!   - The uDMA channels must be assigned to the SSI module.
//!
//! - After calling this function:
//!   - The interrupt handler for the SSI module must call
//!     SPIFlashIntHandler(), passing the same pState structure pointer that
//!     was supplied to this function.
//!   - No other SPI flash operation can be called until this operation has
//!     completed.
//!
//! Completion of the read operation is indicated when SPIFlashIntHandler()
//! returns \b SPI_FLASH_DONE.
//!
//! Like SPIFlashQuadRead(), this uses the 0x6b SPI flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashQuadReadNonBlocking(tSPIFlashState *pState, uint32_t ui32Base,
                            uint32_t ui32Addr, uint8_t *pui8Data,
                            uint32_t ui32Count, bool bUseDMA,
                            uint32_t ui32TxChannel, uint32_t ui32RxChannel)
{
    uint32_t ui32Trash;

    //
    // Drain any residual data from the receive FIFO.
    //
    while(MAP_SSIDataGetNonBlocking(ui32Base, &ui32Trash) != 0)
    {
    }

    //
    // Save the parameters of this read operation to the state structure.
    //
    pState->ui32Base = ui32Base;
    pState->ui16Cmd = CMD_QREAD;
    pState->ui16State = STATE_CMD;
    pState->ui32Addr = ui32Addr;
    pState->pui8Buffer = pui8Data;
    pState->ui32ReadCount = ui32Count;
    pState->ui32WriteCount = ui32Count;
    pState->bUseDMA = bUseDMA;
    pState->ui32TxChannel = ui32TxChannel & 0x1f;
    pState->ui32RxChannel = ui32RxChannel & 0x1f;

    //
    // Enable the SSI transmit and receive interrupts.  This will start the
    // transfer.  If uDMA is being used, the uDMA-related interrupts will be
    // enabled at the appropriate time by the interrupt handler.
    //
    HWREG(ui32Base + SSI_O_ICR) = SSI_ICR_DMATXIC | SSI_ICR_DMARXIC;
    HWREG(ui32Base + SSI_O_IM) = SSI_IM_TXIM | SSI_IM_RXIM | SSI_IM_RTIM;
}

//*****************************************************************************
//
//! Reads the manufacturer and device IDs from the SPI flash.
//!
//! \param ui32Base is the SSI module base address.
//! \param pui8ManufacturerID is a pointer to the location into which to store
//! the manufacturer ID.
//! \param pui16DeviceID is a pointer to the location into which to store the
//! device ID.
//!
//! This function reads the manufacturer and device IDs from the SPI flash.
//! These values can be used to identify the SPI flash that is attached, as
//! well as determining if a SPI flash is attached (if the \b SSIRx pin is
//! pulled up or down, either using the pad's weak pull up/down or using an
//! external resistor, which will cause the returned IDs to be either all zeros
//! or all ones if the SPI flash is not attached).  This uses the 0x9f SPI
//! flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashReadID(uint32_t ui32Base, uint8_t *pui8ManufacturerID,
               uint16_t *pui16DeviceID)
{
    uint32_t ui32Data1, ui32Data2;

    //
    // Drain any residual data from the receive FIFO.
    //
    while(MAP_SSIDataGetNonBlocking(ui32Base, &ui32Data1) != 0)
    {
    }

    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the read ID command.
    //
    MAP_SSIDataPut(ui32Base, CMD_RDID);

    //
    // Set the SSI module into read/write mode.  In this mode, dummy writes are
    // required in order to make the transfer occur; the SPI flash will ignore
    // the data.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_READ_WRITE);

    //
    // Send three dummy bytes, marking the last as the end of the frame.
    //
    MAP_SSIDataPut(ui32Base, 0);
    MAP_SSIDataPut(ui32Base, 0);
    MAP_SSIAdvDataPutFrameEnd(ui32Base, 0);

    //
    // Read the first returned data byte, which contains the manufacturer ID.
    //
    MAP_SSIDataGet(ui32Base, &ui32Data1);
    *pui8ManufacturerID = ui32Data1 & 0xff;

    //
    // Read the remaining two data bytes, which contain the device ID.
    //
    MAP_SSIDataGet(ui32Base, &ui32Data1);
    MAP_SSIDataGet(ui32Base, &ui32Data2);
    *pui16DeviceID = ((ui32Data1 & 0xff) << 8) | (ui32Data2 & 0xff);
}

//*****************************************************************************
//
//! Erases the entire SPI flash.
//!
//! \param ui32Base is the SSI module base address.
//!
//! This command erase the entire SPI flash.  The chip erase command is issued
//! by this function; SPIFlashReadStatus() must be used to query the SPI flash
//! to determine when the chip erase operation has completed.  This uses the
//! 0xc7 SPI flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashChipErase(uint32_t ui32Base)
{
    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the chip erase command, marking this byte as the end of the frame.
    //
    MAP_SSIAdvDataPutFrameEnd(ui32Base, CMD_CE);
}

//*****************************************************************************
//
//! Erases a 64 KB block of the SPI flash.
//!
//! \param ui32Base is the SSI module base address.
//! \param ui32Addr is the SPI flash address to erase.
//!
//! This function erases a 64 KB block of the SPI flash.  Each 64 KB block has
//! a 64 KB alignment; the SPI flash will ignore the lower 16 bits of the
//! address provided.  The 64 KB block erase command is issued by this
//! function; SPIFlashReadStatus() must be used to query the SPI flash to
//! determine when the 64 KB block erase operation has completed.  This uses
//! the 0xd8 SPI flash command.
//!
//! \return None.
//
//*****************************************************************************
void
SPIFlashBlockErase64(uint32_t ui32Base, uint32_t ui32Addr)
{
    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(ui32Base, SSI_ADV_MODE_WRITE);

    //
    // Send the 64 KB block erase command command.
    //
    MAP_SSIDataPut(ui32Base, CMD_BE64);

    //
    // Send the address of the 64 KB block to be erased, marking the last byte
    // of the address as the end of the frame.
    //
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 16) & 0xff);
    MAP_SSIDataPut(ui32Base, (ui32Addr >> 8) & 0xff);
    MAP_SSIAdvDataPutFrameEnd(ui32Base, ui32Addr & 0xff);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
