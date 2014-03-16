//*****************************************************************************
//
// softuart.h - Defines and macros for the SoftUART.
//
// Copyright (c) 2010-2014 Texas Instruments Incorporated.  All rights reserved.
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

#ifndef __SOFTUART_H__
#define __SOFTUART_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
//! \addtogroup softuart_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! This structure contains the state of a single instance of a SoftUART
//! module.
//
//*****************************************************************************
typedef struct
{
    //
    //! The address of the callback function that is called to simulate the
    //! interrupts that would be produced by a hardware UART implementation.
    //! This address can be set via a direct structure access or using the
    //! SoftUARTCallbackSet function.
    //
    void (*pfnIntCallback)(void);

    //
    //! The address of the GPIO pin to be used for the Tx signal.  This member
    //! can be set via a direct structure access or using the SoftUARTTxGPIOSet
    //! function.
    //
    uint32_t ui32TxGPIO;

    //
    //! The address of the GPIO port to be used for the Rx signal.  This member
    //! can be set via a direct structure access or using the SoftUARTRxGPIOSet
    //! function.
    //
    uint32_t ui32RxGPIOPort;

    //
    //! The address of the data buffer used for the transmit buffer.  This
    //! member can be set via a direct structure access or using the
    //! SoftUARTTxBufferSet function.
    //
    uint8_t *pui8TxBuffer;

    //
    //! The address of the data buffer used for the receive buffer.  This
    //! member can be set via a direct structure access or using the
    //! SoftUARTRxBufferSet function.
    //
    uint16_t *pui16RxBuffer;

    //
    //! The length of the transmit buffer.  This member can be set via a direct
    //! structure access or using the SoftUARTTxBufferSet function.
    //
    uint16_t ui16TxBufferLen;

    //
    //! The index into the transmit buffer of the next character to be
    //! transmitted.  This member should not be accessed or modified by the
    //! application.
    //
    uint16_t ui16TxBufferRead;

    //
    //! The index into the transmit buffer of the next location to store a
    //! character into the buffer.  This member should not be accessed or
    //! modified by the application.
    //
    uint16_t ui16TxBufferWrite;

    //
    //! The transmit buffer level at which the transmit interrupt is asserted.
    //! This member should not be accessed or modified by the application.
    //
    uint16_t ui16TxBufferLevel;

    //
    //! The length of the receive buffer.  This member can be set via a direct
    //! structure access or using the SoftUARTRxBufferSet function.
    //
    uint16_t ui16RxBufferLen;

    //
    //! The index into the receive buffer of the next character to be read from
    //! the buffer.  This member should not be accessed or modified by the
    //! application.
    //
    uint16_t ui16RxBufferRead;

    //
    //! The index into the receive buffer of the lcoation to store the next
    //! character received.  This member should not be accessed or modified by
    //! the application.
    //
    uint16_t ui16RxBufferWrite;

    //
    //! The receive buffer level at which the receive interrupt is asserted.
    //! This member should not be accessed or modified by the application.
    //
    uint16_t ui16RxBufferLevel;

    //
    //! The set of virtual interrupts that are currently asserted.  This member
    //! should not be accessed or modified by the application.
    //
    uint16_t ui16IntStatus;

    //
    //! The set of virtual interrupts that should be sent to the callback
    //! function.  This member should not be accessed or modified by the
    //! application.
    //
    uint16_t ui16IntMask;

    //
    //! The configuration of the SoftUART module.  This member can be set via
    //! the SoftUARTConfigSet and SoftUARTFIFOLevelSet functions.
    //
    uint16_t ui16Config;

    //
    //! The flags that control the operation of the SoftUART module.  This
    //! member should not be be accessed or modified by the application.
    //
    uint8_t ui8Flags;

    //
    //! The current state of the SoftUART transmit state machine.  This member
    //! should not be accessed or modified by the application.
    //
    uint8_t ui8TxState;

    //
    //! The value that is written to the Tx pin at the start of the next
    //! transmit timer tick.  This member should not be accessed or modified
    //! by the application.
    //
    uint8_t ui8TxNext;

    //
    //! The character that is currently be sent via the Tx pin.  This member
    //! should not be accessed or modified by the application.
    //
    uint8_t ui8TxData;

    //
    //! The GPIO pin to be used for the Rx signal.  This member can be set via
    //! a direct structure access or using the SoftUARTRxGPIOSet function.
    //
    uint8_t ui8RxPin;

    //
    //! The current state of the SoftUART receive state machine.  This member
    //! should not be accessed or modified by the application.
    //
    uint8_t ui8RxState;

    //
    //! The character that is currently being received via the Rx pin.  This
    //! member should not be accessed or modified by the application.
    //
    uint8_t ui8RxData;

    //
    //! The flags that indicate any errors that have occurred during the
    //! reception of the current character via the Rx pin.  This member should
    //! not be accessed or modified by the application.
    //
    uint8_t ui8RxFlags;

    //
    //! The receive error status.  This member should only be accessed via the
    //! SoftUARTRxErrorGet and SoftURATRxErrorClear functions.
    //
    uint8_t ui8RxStatus;
}
tSoftUART;

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// Values that can be passed to SoftUARTIntEnable(), SoftUARTIntDisable(), and
// SoftUARTIntClear() as the ui32IntFlags parameter, and returned from
// SoftUARTIntStatus().
//
//*****************************************************************************
#define SOFTUART_INT_EOT        0x800       // End of transmission interrupt
#define SOFTUART_INT_OE         0x400       // Overrun error interrupt
#define SOFTUART_INT_BE         0x200       // Break error interrupt
#define SOFTUART_INT_PE         0x100       // Parity error interrupt
#define SOFTUART_INT_FE         0x080       // Framing error interrupt
#define SOFTUART_INT_RT         0x040       // Receive timeout interrupt
#define SOFTUART_INT_TX         0x020       // Transmit interrupt
#define SOFTUART_INT_RX         0x010       // Receive interrupt

//*****************************************************************************
//
// Values that can be passed to SoftUARTConfigSet() as the ui32Config parameter
// and returned by SoftUARTConfigGet() in the pui32Config parameter.
// Additionally, the UART_CONFIG_PAR_* subset can be passed to
// SoftUARTParityModeSet() as the ui32Parity parameter, and are returned by
// SoftUARTParityModeGet().
//
//*****************************************************************************
#define SOFTUART_CONFIG_WLEN_MASK                                             \
                                0x00000060  // Mask for extracting word length
#define SOFTUART_CONFIG_WLEN_8  0x00000060  // 8 bit data
#define SOFTUART_CONFIG_WLEN_7  0x00000040  // 7 bit data
#define SOFTUART_CONFIG_WLEN_6  0x00000020  // 6 bit data
#define SOFTUART_CONFIG_WLEN_5  0x00000000  // 5 bit data
#define SOFTUART_CONFIG_STOP_MASK                                             \
                                0x00000008  // Mask for extracting stop bits
#define SOFTUART_CONFIG_STOP_ONE                                              \
                                0x00000000  // One stop bit
#define SOFTUART_CONFIG_STOP_TWO                                              \
                                0x00000008  // Two stop bits
#define SOFTUART_CONFIG_PAR_MASK                                              \
                                0x00000086  // Mask for extracting parity
#define SOFTUART_CONFIG_PAR_NONE                                              \
                                0x00000000  // No parity
#define SOFTUART_CONFIG_PAR_EVEN                                              \
                                0x00000006  // Even parity
#define SOFTUART_CONFIG_PAR_ODD 0x00000002  // Odd parity
#define SOFTUART_CONFIG_PAR_ONE 0x00000082  // Parity bit is one
#define SOFTUART_CONFIG_PAR_ZERO                                              \
                                0x00000086  // Parity bit is zero
#define SOFTUART_CONFIG_WLEN_S  5

//*****************************************************************************
//
// Values that can be passed to SoftUARTFIFOLevelSet() as the ui32TxLevel
// parameter and returned by SoftUARTFIFOLevelGet() in the pui32TxLevel.
//
//*****************************************************************************
#define SOFTUART_FIFO_TX1_8     0x00000000  // Transmit interrupt at 1/8 Full
#define SOFTUART_FIFO_TX2_8     0x00000001  // Transmit interrupt at 1/4 Full
#define SOFTUART_FIFO_TX4_8     0x00000002  // Transmit interrupt at 1/2 Full
#define SOFTUART_FIFO_TX6_8     0x00000003  // Transmit interrupt at 3/4 Full
#define SOFTUART_FIFO_TX7_8     0x00000004  // Transmit interrupt at 7/8 Full

//*****************************************************************************
//
// Values that can be passed to SoftUARTFIFOLevelSet() as the ui32RxLevel
// parameter and returned by SoftUARTFIFOLevelGet() in the pui32RxLevel.
//
//*****************************************************************************
#define SOFTUART_FIFO_RX1_8     0x00000000  // Receive interrupt at 1/8 Full
#define SOFTUART_FIFO_RX2_8     0x00000008  // Receive interrupt at 1/4 Full
#define SOFTUART_FIFO_RX4_8     0x00000010  // Receive interrupt at 1/2 Full
#define SOFTUART_FIFO_RX6_8     0x00000018  // Receive interrupt at 3/4 Full
#define SOFTUART_FIFO_RX7_8     0x00000020  // Receive interrupt at 7/8 Full

//*****************************************************************************
//
// Values returned from SoftUARTRxErrorGet().
//
//*****************************************************************************
#define SOFTUART_RXERROR_OVERRUN                                              \
                                0x00000008  // An overrun error occurred
#define SOFTUART_RXERROR_BREAK  0x00000004  // A break was received
#define SOFTUART_RXERROR_PARITY 0x00000002  // A parity error occurred
#define SOFTUART_RXERROR_FRAMING                                              \
                                0x00000001  // A framing error occurred

//*****************************************************************************
//
// Values returned from SoftUARTRxTick().
//
//*****************************************************************************
#define SOFTUART_RXTIMER_NOP    0           // The timer should continue to run
#define SOFTUART_RXTIMER_END    1           // The timer should be stopped

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void SoftUARTInit(tSoftUART *psUART);
extern void SoftUARTParityModeSet(tSoftUART *psUART, uint32_t ui32Parity);
extern uint32_t SoftUARTParityModeGet(tSoftUART *psUART);
extern void SoftUARTFIFOLevelSet(tSoftUART *psUART, uint32_t ui32TxLevel,
                                 uint32_t ui32RxLevel);
extern void SoftUARTFIFOLevelGet(tSoftUART *psUART, uint32_t *pui32TxLevel,
                                 uint32_t *pui32RxLevel);
extern void SoftUARTConfigSet(tSoftUART *psUART, uint32_t ui32Config);
extern void SoftUARTConfigGet(tSoftUART *psUART, uint32_t *pui32Config);
extern void SoftUARTEnable(tSoftUART *psUART);
extern void SoftUARTDisable(tSoftUART *psUART);
extern void SoftUARTFIFOEnable(tSoftUART *psUART);
extern void SoftUARTFIFODisable(tSoftUART *psUART);
extern bool SoftUARTCharsAvail(tSoftUART *psUART);
extern bool SoftUARTSpaceAvail(tSoftUART *psUART);
extern int32_t SoftUARTCharGetNonBlocking(tSoftUART *psUART);
extern int32_t SoftUARTCharGet(tSoftUART *psUART);
extern bool SoftUARTCharPutNonBlocking(tSoftUART *psUART,
                                           uint8_t ui8Data);
extern void SoftUARTCharPut(tSoftUART *psUART, uint8_t ui8Data);
extern void SoftUARTBreakCtl(tSoftUART *psUART, bool bBreakState);
extern bool SoftUARTBusy(tSoftUART *psUART);
extern void SoftUARTIntEnable(tSoftUART *psUART, uint32_t ui32IntFlags);
extern void SoftUARTIntDisable(tSoftUART *psUART, uint32_t ui32IntFlags);
extern uint32_t SoftUARTIntStatus(tSoftUART *psUART, bool bMasked);
extern void SoftUARTIntClear(tSoftUART *psUART, uint32_t ui32IntFlags);
extern uint32_t SoftUARTRxErrorGet(tSoftUART *psUART);
extern void SoftUARTRxErrorClear(tSoftUART *psUART);
extern uint32_t SoftUARTRxTick(tSoftUART *psUART, bool bEdgeInt);
extern void SoftUARTTxIntModeSet(tSoftUART *psUART, uint32_t ui32Mode);
extern uint32_t SoftUARTTxIntModeGet(tSoftUART *psUART);
extern void SoftUARTTxTimerTick(tSoftUART *psUART);
extern void SoftUARTCallbackSet(tSoftUART *psUART, void (*pfnCallback)(void));
extern void SoftUARTTxGPIOSet(tSoftUART *psUART, uint32_t ui32Base,
                              uint8_t ui8Pin);
extern void SoftUARTRxGPIOSet(tSoftUART *psUART, uint32_t ui32Base,
                              uint8_t ui8Pin);
extern void SoftUARTTxBufferSet(tSoftUART *psUART, uint8_t *pui8TxBuffer,
                                uint16_t ui16Len);
extern void SoftUARTRxBufferSet(tSoftUART *psUART, uint16_t *pui16RxBuffer,
                                uint16_t ui16Len);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __SOFTUART_H__
