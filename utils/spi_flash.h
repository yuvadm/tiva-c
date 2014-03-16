//*****************************************************************************
//
// spi_flash.h - Prototypes for the SPI flash driver.
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

#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

//*****************************************************************************
//
//! \addtogroup spi_flash_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The state structure used when performing non-blocking SPI flash operations.
//
//*****************************************************************************
typedef struct
{
    //
    //! The base address of the SSI module that is being used.
    //
    uint32_t ui32Base;

    //
    //! The command that is being send to the SPI flash.
    //
    uint16_t ui16Cmd;

    //
    //! The current state of the SPI flash state machine.
    //
    uint16_t ui16State;

    //
    //! The SPI flash address associated with the command.
    //
    uint32_t ui32Addr;

    //
    //! A pointer to the data buffer that is being read or written.
    //
    uint8_t *pui8Buffer;

    //
    //! The count of bytes left to be read.
    //
    uint32_t ui32ReadCount;

    //
    //! The count of bytes left to be written.
    //
    uint32_t ui32WriteCount;

    //
    //! A flag that is true if uDMA used be used for the transfer.
    //
    bool bUseDMA;

    //
    //! The uDMA channel to use for transmitting when using uDMA for the
    //! transfer.
    //
    uint32_t ui32TxChannel;

    //
    //! The uDMA channel to use for receiving when using uDMA for the transfer.
    //
    uint32_t ui32RxChannel;
}
tSPIFlashState;

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// The possible return values from the SPI flash interrupt handler.
//
//*****************************************************************************
#define SPI_FLASH_IDLE          0
#define SPI_FLASH_WORKING       1
#define SPI_FLASH_DONE          3

//*****************************************************************************
//
// Prototypes.
//
//*****************************************************************************
extern uint32_t SPIFlashIntHandler(tSPIFlashState *pState);
extern void SPIFlashInit(uint32_t ui32Base, uint32_t ui32Clock,
                         uint32_t ui32BitRate);
extern void SPIFlashWriteStatus(uint32_t ui32Base, uint8_t ui8Status);
extern void SPIFlashPageProgram(uint32_t ui32Base, uint32_t ui32Addr,
                                const uint8_t *pui8Data, uint32_t ui32Count);
extern void SPIFlashPageProgramNonBlocking(tSPIFlashState *pState,
                                           uint32_t ui32Base,
                                           uint32_t ui32Addr,
                                           const uint8_t *pui8Data,
                                           uint32_t ui32Count, bool bUseDMA,
                                           uint32_t ui32TxChannel);
extern void SPIFlashRead(uint32_t ui32Base, uint32_t ui32Addr,
                         uint8_t *pui8Data, uint32_t ui32Count);
extern void SPIFlashReadNonBlocking(tSPIFlashState *pState, uint32_t ui32Base,
                                    uint32_t ui32Addr, uint8_t *pui8Data,
                                    uint32_t ui32Count, bool bUseDMA,
                                    uint32_t ui32TxChannel,
                                    uint32_t ui32RxChannel);
extern void SPIFlashWriteDisable(uint32_t ui32Base);
extern uint8_t SPIFlashReadStatus(uint32_t ui32Base);
extern void SPIFlashWriteEnable(uint32_t ui32Base);
extern void SPIFlashFastRead(uint32_t ui32Base, uint32_t ui32Addr,
                             uint8_t *pui8Data, uint32_t ui32Count);
extern void SPIFlashFastReadNonBlocking(tSPIFlashState *pState,
                                        uint32_t ui32Base, uint32_t ui32Addr,
                                        uint8_t *pui8Data, uint32_t ui32Count,
                                        bool bUseDMA, uint32_t ui32TxChannel,
                                        uint32_t ui32RxChannel);
extern void SPIFlashSectorErase(uint32_t ui32Base, uint32_t ui32Addr);
extern void SPIFlashDualRead(uint32_t ui32Base, uint32_t ui32Addr,
                             uint8_t *pui8Data, uint32_t ui32Count);
extern void SPIFlashDualReadNonBlocking(tSPIFlashState *pState,
                                        uint32_t ui32Base, uint32_t ui32Addr,
                                        uint8_t *pui8Data, uint32_t ui32Count,
                                        bool bUseDMA, uint32_t ui32TxChannel,
                                        uint32_t ui32RxChannel);
extern void SPIFlashBlockErase32(uint32_t ui32Base, uint32_t ui32Addr);
extern void SPIFlashQuadRead(uint32_t ui32Base, uint32_t ui32Addr,
                             uint8_t *pui8Data, uint32_t ui32Count);
extern void SPIFlashQuadReadNonBlocking(tSPIFlashState *pState,
                                        uint32_t ui32Base, uint32_t ui32Addr,
                                        uint8_t *pui8Data, uint32_t ui32Count,
                                        bool bUseDMA, uint32_t ui32TxChannel,
                                        uint32_t ui32RxChannel);
extern void SPIFlashReadID(uint32_t ui32Base, uint8_t *pui8ManufacturerID,
                           uint16_t *pui16DeviceID);
extern void SPIFlashChipErase(uint32_t ui32Base);
extern void SPIFlashBlockErase64(uint32_t ui32Base, uint32_t ui32Addr);

#endif // __SPI_FLASH_H__
