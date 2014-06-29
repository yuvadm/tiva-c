//*****************************************************************************
//
// udma_demo.c - uDMA example.
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
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "driverlib/udma.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "grlib/grlib.h"
#include "utils/cpu_usage.h"
#include "utils/ustdlib.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/frame.h"
#include "drivers/pinout.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>uDMA (udma_demo)</h1>
//!
//! This example application demonstrates the use of the uDMA controller to
//! transfer data between memory buffers, and to transfer data to and from a
//! UART.  The test runs for 10 seconds before exiting.
//
//*****************************************************************************


//*****************************************************************************
//
// The number of SysTick ticks per second used for the SysTick interrupt.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND     100

//*****************************************************************************
//
// The size of the memory transfer source and destination buffers (in words).
//
//*****************************************************************************
#define MEM_BUFFER_SIZE         1024

//*****************************************************************************
//
// The size of the UART transmit and receive buffers.  They do not need to be
// the same size.
//
//*****************************************************************************
#define UART_TXBUF_SIZE         256
#define UART_RXBUF_SIZE         256

//*****************************************************************************
//
// The system clock frequency in Hz.
//
//*****************************************************************************
volatile uint32_t g_ui32SysClock;

//*****************************************************************************
//
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
static tContext g_sContext;

//*****************************************************************************
//
// The source and destination buffers used for memory transfers.
//
//*****************************************************************************
static uint32_t g_ui32SrcBuf[MEM_BUFFER_SIZE];
static uint32_t g_ui32DstBuf[MEM_BUFFER_SIZE];

//*****************************************************************************
//
// The transmit and receive buffers used for the UART transfers.  There is one
// transmit buffer and a pair of recieve ping-pong buffers.
//
//*****************************************************************************
static uint8_t g_ui8TxBuf[UART_TXBUF_SIZE];
static uint8_t g_ui8RxBufA[UART_RXBUF_SIZE];
static uint8_t g_ui8RxBufB[UART_RXBUF_SIZE];

//*****************************************************************************
//
// The count of uDMA errors.  This value is incremented by the uDMA error
// handler.
//
//*****************************************************************************
static volatile uint32_t g_ui32uDMAErrCount = 0;

//*****************************************************************************
//
// The count of times the uDMA interrupt occurred but the uDMA transfer was not
// complete.  This should remain 0.
//
//*****************************************************************************
static volatile uint32_t g_ui32BadISR = 0;

//*****************************************************************************
//
// The count of UART buffers filled, one for each ping-pong buffer.
//
//*****************************************************************************
static volatile uint32_t g_ui32RxBufACount = 0;
static volatile uint32_t g_ui32RxBufBCount = 0;

//*****************************************************************************
//
// The count of memory uDMA transfer blocks.  This value is incremented by the
// uDMA interrupt handler whenever a memory block transfer is completed.
//
//*****************************************************************************
static volatile uint32_t g_ui32MemXferCount = 0;

//*****************************************************************************
//
// The CPU usage in percent, in 16.16 fixed point format.
//
//*****************************************************************************
static volatile uint32_t g_ui32CPUUsage;

//*****************************************************************************
//
// The number of seconds elapsed since the start of the program.  This value is
// maintained by the SysTick interrupt handler.
//
//*****************************************************************************
static volatile uint32_t g_ui32Seconds = 0;

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
uint8_t pui8ControlTable[1024];
#elif defined(ccs)
#pragma DATA_ALIGN(pui8ControlTable, 1024)
uint8_t pui8ControlTable[1024];
#else
uint8_t pui8ControlTable[1024] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// The interrupt handler for the SysTick timer.  This handler will increment a
// seconds counter whenever the appropriate number of ticks has occurred.  It
// will also call the CPU usage tick function to find the CPU usage percent.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    static uint32_t ui32TickCount = 0;

    //
    // Increment the tick counter.
    //
    ui32TickCount++;

    //
    // If the number of ticks per second has occurred, then increment the
    // seconds counter.
    //
    if(!(ui32TickCount % SYSTICKS_PER_SECOND))
    {
        g_ui32Seconds++;
    }

    //
    // Call the CPU usage tick function.  This function will compute the amount
    // of cycles used by the CPU since the last call and return the result in
    // percent in fixed point 16.16 format.
    //
    g_ui32CPUUsage = CPUUsageTick();
}

//*****************************************************************************
//
// The interrupt handler for uDMA errors.  This interrupt will occur if the
// uDMA encounters a bus error while trying to perform a transfer.  This
// handler just increments a counter if an error occurs.
//
//*****************************************************************************
void
uDMAErrorHandler(void)
{
    uint32_t ui32Status;

    //
    // Check for uDMA error bit
    //
    ui32Status = ROM_uDMAErrorStatusGet();

    //
    // If there is a uDMA error, then clear the error and increment
    // the error counter.
    //
    if(ui32Status)
    {
        ROM_uDMAErrorStatusClear();
        g_ui32uDMAErrCount++;
    }
}

//*****************************************************************************
//
// The interrupt handler for uDMA interrupts from the memory channel.  This
// interrupt will increment a counter, and then restart another memory
// transfer.
//
//*****************************************************************************
void
uDMAIntHandler(void)
{
    uint32_t ui32Mode;

    //
    // Check for the primary control structure to indicate complete.
    //
    ui32Mode = ROM_uDMAChannelModeGet(UDMA_CHANNEL_SW);
    if(ui32Mode == UDMA_MODE_STOP)
    {
        //
        // Increment the count of completed transfers.
        //
        g_ui32MemXferCount++;

        //
        // Configure it for another transfer.
        //
        ROM_uDMAChannelTransferSet(UDMA_CHANNEL_SW, UDMA_MODE_AUTO,
                                   g_ui32SrcBuf, g_ui32DstBuf,
                                   MEM_BUFFER_SIZE);

        //
        // Initiate another transfer.
        //
        ROM_uDMAChannelEnable(UDMA_CHANNEL_SW);
        ROM_uDMAChannelRequest(UDMA_CHANNEL_SW);
    }

    //
    // If the channel is not stopped, then something is wrong.
    //
    else
    {
        g_ui32BadISR++;
    }
}

//*****************************************************************************
//
// The interrupt handler for UART0.  This interrupt will occur when a DMA
// transfer is complete using the UART0 uDMA channel.  It will also be
// triggered if the peripheral signals an error.  This interrupt handler will
// switch between receive ping-pong buffers A and B.  It will also restart a TX
// uDMA transfer if the prior transfer is complete.  This will keep the UART
// running continuously (looping TX data back to RX).
//
//*****************************************************************************
void
UART0IntHandler(void)
{
    uint32_t ui32Status;
    uint32_t ui32Mode;

    //
    // Read the interrupt status of the UART.
    //
    ui32Status = ROM_UARTIntStatus(UART0_BASE, 1);

    //
    // Clear any pending status, even though there should be none since no UART
    // interrupts were enabled.  If UART error interrupts were enabled, then
    // those interrupts could occur here and should be handled.  Since uDMA is
    // used for both the RX and TX, then neither of those interrupts should be
    // enabled.
    //
    ROM_UARTIntClear(UART0_BASE, ui32Status);

    //
    // Check the DMA control table to see if the ping-pong "A" transfer is
    // complete.  The "A" transfer uses receive buffer "A", and the primary
    // control structure.
    //
    ui32Mode = ROM_uDMAChannelModeGet(UDMA_CHANNEL_UART0RX | UDMA_PRI_SELECT);

    //
    // If the primary control structure indicates stop, that means the "A"
    // receive buffer is done.  The uDMA controller should still be receiving
    // data into the "B" buffer.
    //
    if(ui32Mode == UDMA_MODE_STOP)
    {
        //
        // Increment a counter to indicate data was received into buffer A.  In
        // a real application this would be used to signal the main thread that
        // data was received so the main thread can process the data.
        //
        g_ui32RxBufACount++;

        //
        // Set up the next transfer for the "A" buffer, using the primary
        // control structure.  When the ongoing receive into the "B" buffer is
        // done, the uDMA controller will switch back to this one.  This
        // example re-uses buffer A, but a more sophisticated application could
        // use a rotating set of buffers to increase the amount of time that
        // the main thread has to process the data in the buffer before it is
        // reused.
        //
        ROM_uDMAChannelTransferSet(UDMA_CHANNEL_UART0RX | UDMA_PRI_SELECT,
                                   UDMA_MODE_PINGPONG,
                                   (void *)(UART0_BASE + UART_O_DR),
                                   g_ui8RxBufA, sizeof(g_ui8RxBufA));
    }

    //
    // Check the DMA control table to see if the ping-pong "B" transfer is
    // complete.  The "B" transfer uses receive buffer "B", and the alternate
    // control structure.
    //
    ui32Mode = ROM_uDMAChannelModeGet(UDMA_CHANNEL_UART0RX | UDMA_ALT_SELECT);

    //
    // If the alternate control structure indicates stop, that means the "B"
    // receive buffer is done.  The uDMA controller should still be receiving
    // data into the "A" buffer.
    //
    if(ui32Mode == UDMA_MODE_STOP)
    {
        //
        // Increment a counter to indicate data was received into buffer A.  In
        // a real application this would be used to signal the main thread that
        // data was received so the main thread can process the data.
        //
        g_ui32RxBufBCount++;

        //
        // Set up the next transfer for the "B" buffer, using the alternate
        // control structure.  When the ongoing receive into the "A" buffer is
        // done, the uDMA controller will switch back to this one.  This
        // example re-uses buffer B, but a more sophisticated application could
        // use a rotating set of buffers to increase the amount of time that
        // the main thread has to process the data in the buffer before it is
        // reused.
        //
        ROM_uDMAChannelTransferSet(UDMA_CHANNEL_UART0RX | UDMA_ALT_SELECT,
                                   UDMA_MODE_PINGPONG,
                                   (void *)(UART0_BASE + UART_O_DR),
                                   g_ui8RxBufB, sizeof(g_ui8RxBufB));
    }

    //
    // If the UART0 DMA TX channel is disabled, that means the TX DMA transfer
    // is done.
    //
    if(!ROM_uDMAChannelIsEnabled(UDMA_CHANNEL_UART0TX))
    {
        //
        // Start another DMA transfer to UART0 TX.
        //
        ROM_uDMAChannelTransferSet(UDMA_CHANNEL_UART0TX | UDMA_PRI_SELECT,
                                   UDMA_MODE_BASIC, g_ui8TxBuf,
                                   (void *)(UART0_BASE + UART_O_DR),
                                   sizeof(g_ui8TxBuf));

        //
        // The uDMA TX channel must be re-enabled.
        //
        ROM_uDMAChannelEnable(UDMA_CHANNEL_UART0TX);
    }
}

//*****************************************************************************
//
// Initializes the UART0 peripheral and sets up the TX and RX uDMA channels.
// The UART is configured for loopback mode so that any data sent on TX will be
// received on RX.  The uDMA channels are configured so that the TX channel
// will copy data from a buffer to the UART TX output.  And the uDMA RX channel
// will receive any incoming data into a pair of buffers in ping-pong mode.
//
//*****************************************************************************
void
InitUART0Transfer(void)
{
    uint_fast16_t ui16Idx;

    //
    // Fill the TX buffer with a simple data pattern.
    //
    for(ui16Idx = 0; ui16Idx < UART_TXBUF_SIZE; ui16Idx++)
    {
        g_ui8TxBuf[ui16Idx] = ui16Idx;
    }

    //
    // Enable the UART peripheral, and configure it to operate even if the CPU
    // is in sleep.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure the UART communication parameters.
    //
    ROM_UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, 115200,
                            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                            UART_CONFIG_PAR_NONE);

    //
    // Set both the TX and RX trigger thresholds to 4.  This will be used by
    // the uDMA controller to signal when more data should be transferred.  The
    // uDMA TX and RX channels will be configured so that it can transfer 4
    // bytes in a burst when the UART is ready to transfer more data.
    //
    ROM_UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX4_8, UART_FIFO_RX4_8);

    //
    // Enable the UART for operation, and enable the uDMA interface for both TX
    // and RX channels.
    //
    ROM_UARTEnable(UART0_BASE);
    ROM_UARTDMAEnable(UART0_BASE, UART_DMA_RX | UART_DMA_TX);

    //
    // This register write will set the UART to operate in loopback mode.  Any
    // data sent on the TX output will be received on the RX input.
    //
    HWREG(UART0_BASE + UART_O_CTL) |= UART_CTL_LBE;

    //
    // Put the attributes in a known state for the uDMA UART0RX channel.  These
    // should already be disabled by default.
    //
    ROM_uDMAChannelAttributeDisable(UDMA_CHANNEL_UART0RX,
                                    UDMA_ATTR_ALTSELECT | UDMA_ATTR_USEBURST |
                                    UDMA_ATTR_HIGH_PRIORITY |
                                    UDMA_ATTR_REQMASK);

    //
    // Configure the control parameters for the primary control structure for
    // the UART RX channel.  The primary contol structure is used for the "A"
    // part of the ping-pong receive.  The transfer data size is 8 bits, the
    // source address does not increment since it will be reading from a
    // register.  The destination address increment is byte 8-bit bytes.  The
    // arbitration size is set to 4 to match the RX FIFO trigger threshold.
    // The uDMA controller will use a 4 byte burst transfer if possible.  This
    // will be somewhat more effecient that single byte transfers.
    //
    ROM_uDMAChannelControlSet(UDMA_CHANNEL_UART0RX | UDMA_PRI_SELECT,
                              UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 |
                              UDMA_ARB_4);

    //
    // Configure the control parameters for the alternate control structure for
    // the UART RX channel.  The alternate contol structure is used for the "B"
    // part of the ping-pong receive.  The configuration is identical to the
    // primary/A control structure.
    //
    ROM_uDMAChannelControlSet(UDMA_CHANNEL_UART0RX | UDMA_ALT_SELECT,
                              UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 |
                              UDMA_ARB_4);

    //
    // Set up the transfer parameters for the UART RX primary control
    // structure.  The mode is set to ping-pong, the transfer source is the
    // UART data register, and the destination is the receive "A" buffer.  The
    // transfer size is set to match the size of the buffer.
    //
    ROM_uDMAChannelTransferSet(UDMA_CHANNEL_UART0RX | UDMA_PRI_SELECT,
                               UDMA_MODE_PINGPONG,
                               (void *)(UART0_BASE + UART_O_DR),
                               g_ui8RxBufA, sizeof(g_ui8RxBufA));

    //
    // Set up the transfer parameters for the UART RX alternate control
    // structure.  The mode is set to ping-pong, the transfer source is the
    // UART data register, and the destination is the receive "B" buffer.  The
    // transfer size is set to match the size of the buffer.
    //
    ROM_uDMAChannelTransferSet(UDMA_CHANNEL_UART0RX | UDMA_ALT_SELECT,
                               UDMA_MODE_PINGPONG,
                               (void *)(UART0_BASE + UART_O_DR),
                               g_ui8RxBufB, sizeof(g_ui8RxBufB));

    //
    // Put the attributes in a known state for the uDMA UART0TX channel.  These
    // should already be disabled by default.
    //
    ROM_uDMAChannelAttributeDisable(UDMA_CHANNEL_UART0TX,
                                    UDMA_ATTR_ALTSELECT |
                                    UDMA_ATTR_HIGH_PRIORITY |
                                    UDMA_ATTR_REQMASK);

    //
    // Set the USEBURST attribute for the uDMA UART TX channel.  This will
    // force the controller to always use a burst when transferring data from
    // the TX buffer to the UART.  This is somewhat more effecient bus usage
    // than the default which allows single or burst transfers.
    //
    ROM_uDMAChannelAttributeEnable(UDMA_CHANNEL_UART0TX, UDMA_ATTR_USEBURST);

    //
    // Configure the control parameters for the UART TX.  The uDMA UART TX
    // channel is used to transfer a block of data from a buffer to the UART.
    // The data size is 8 bits.  The source address increment is 8-bit bytes
    // since the data is coming from a buffer.  The destination increment is
    // none since the data is to be written to the UART data register.  The
    // arbitration size is set to 4, which matches the UART TX FIFO trigger
    // threshold.
    //
    ROM_uDMAChannelControlSet(UDMA_CHANNEL_UART0TX | UDMA_PRI_SELECT,
                              UDMA_SIZE_8 | UDMA_SRC_INC_8 |
                              UDMA_DST_INC_NONE |
                              UDMA_ARB_4);

    //
    // Set up the transfer parameters for the uDMA UART TX channel.  This will
    // configure the transfer source and destination and the transfer size.
    // Basic mode is used because the peripheral is making the uDMA transfer
    // request.  The source is the TX buffer and the destination is the UART
    // data register.
    //
    ROM_uDMAChannelTransferSet(UDMA_CHANNEL_UART0TX | UDMA_PRI_SELECT,
                               UDMA_MODE_BASIC, g_ui8TxBuf,
                               (void *)(UART0_BASE + UART_O_DR),
                               sizeof(g_ui8TxBuf));

    //
    // Now both the uDMA UART TX and RX channels are primed to start a
    // transfer.  As soon as the channels are enabled, the peripheral will
    // issue a transfer request and the data transfers will begin.
    //
    ROM_uDMAChannelEnable(UDMA_CHANNEL_UART0RX);
    ROM_uDMAChannelEnable(UDMA_CHANNEL_UART0TX);

    //
    // Enable the UART DMA TX/RX interrupts.
    //
    ROM_UARTIntEnable(UART0_BASE, UART_INT_DMATX | UART_INT_DMATX);

    //
    // Enable the UART peripheral interrupts.
    //
    ROM_IntEnable(INT_UART0);
}

//*****************************************************************************
//
// Initializes the uDMA software channel to perform a memory to memory uDMA
// transfer.
//
//*****************************************************************************
void
InitSWTransfer(void)
{
    uint_fast16_t ui16Idx;

    //
    // Fill the source memory buffer with a simple incrementing pattern.
    //
    for(ui16Idx = 0; ui16Idx < MEM_BUFFER_SIZE; ui16Idx++)
    {
        g_ui32SrcBuf[ui16Idx] = ui16Idx;
    }

    //
    // Enable interrupts from the uDMA software channel.
    //
    ROM_IntEnable(INT_UDMA);

    //
    // Put the attributes in a known state for the uDMA software channel.
    // These should already be disabled by default.
    //
    ROM_uDMAChannelAttributeDisable(UDMA_CHANNEL_SW,
                                    UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT |
                                    (UDMA_ATTR_HIGH_PRIORITY |
                                    UDMA_ATTR_REQMASK));

    //
    // Configure the control parameters for the SW channel.  The SW channel
    // will be used to transfer between two memory buffers, 32 bits at a time.
    // Therefore the data size is 32 bits, and the address increment is 32 bits
    // for both source and destination.  The arbitration size will be set to 8,
    // which causes the uDMA controller to rearbitrate after 8 items are
    // transferred.  This keeps this channel from hogging the uDMA controller
    // once the transfer is started, and allows other channels cycles if they
    // are higher priority.
    //
    ROM_uDMAChannelControlSet(UDMA_CHANNEL_SW | UDMA_PRI_SELECT,
                              UDMA_SIZE_32 | UDMA_SRC_INC_32 | UDMA_DST_INC_32 |
                              UDMA_ARB_8);

    //
    // Set up the transfer parameters for the software channel.  This will
    // configure the transfer buffers and the transfer size.  Auto mode must be
    // used for software transfers.
    //
    ROM_uDMAChannelTransferSet(UDMA_CHANNEL_SW | UDMA_PRI_SELECT,
                               UDMA_MODE_AUTO, g_ui32SrcBuf, g_ui32DstBuf,
                               MEM_BUFFER_SIZE);

    //
    // Now the software channel is primed to start a transfer.  The channel
    // must be enabled.  For software based transfers, a request must be
    // issued.  After this, the uDMA memory transfer begins.
    //
    ROM_uDMAChannelEnable(UDMA_CHANNEL_SW);
    ROM_uDMAChannelRequest(UDMA_CHANNEL_SW);
}

//*****************************************************************************
//
// This example demonstrates how to use the uDMA controller to transfer data
// between memory buffers and to and from a peripheral, in this case a UART.
// The uDMA controller is configured to repeatedly transfer a block of data
// from one memory buffer to another.  It is also set up to repeatedly copy a
// block of data from a buffer to the UART output.  The UART data is looped
// back so the same data is received, and the uDMA controlled is configured to
// continuously receive the UART data using ping-pong buffers.
//
// The processor is put to sleep when it is not doing anything, and this allows
// collection of CPU usage data to see how much CPU is being used while the
// data transfers are ongoing.
//
//*****************************************************************************
int
main(void)
{
    static uint32_t ui32PrevSeconds;
    static uint32_t ui32PrevXferCount;
    static uint32_t ui32PrevUARTCount = 0;
    static char pcStrBuf[40];
    uint32_t ui32CenterX;
    uint32_t ui32XfersCompleted;
    uint32_t ui32BytesTransferred;

    //
    // Run from the PLL at 120 MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(g_ui32SysClock);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&g_sContext, "udma-demo");

    //
    // Enable peripherals to operate when CPU is in sleep.
    //
    ROM_SysCtlPeripheralClockGating(true);

    //
    // Get the center X coordinate of the screen, since it is used a lot.
    //
    ui32CenterX = GrContextDpyWidthGet(&g_sContext) / 2;

    //
    // Show the clock frequency on the display.
    //
    GrContextFontSet(&g_sContext, g_psFontCmss18b);
    usnprintf(pcStrBuf, sizeof(pcStrBuf), "TM4C129X @ %u MHz",
              g_ui32SysClock / 1000000);
    GrStringDrawCentered(&g_sContext, pcStrBuf, -1, ui32CenterX, 40, 0);

    //
    // Show static text and field labels on the display.
    //
    GrContextFontSet(&g_sContext, g_psFontCmss18b);
    GrStringDrawCentered(&g_sContext, "uDMA Mem Transfers", -1,
                         ui32CenterX, 70, 0);
    GrStringDrawCentered(&g_sContext, "uDMA UART Transfers", -1,
                         ui32CenterX, 124, 0);

    //
    // Configure SysTick to occur 100 times per second, to use as a time
    // reference.  Enable SysTick to generate interrupts.
    //
    ROM_SysTickPeriodSet(g_ui32SysClock / SYSTICKS_PER_SECOND);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //
    // Initialize the CPU usage measurement routine.
    //
    CPUUsageInit(g_ui32SysClock, SYSTICKS_PER_SECOND, 2);

    //
    // Enable the uDMA controller at the system level.  Enable it to continue
    // to run while the processor is in sleep.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UDMA);

    //
    // Enable the uDMA controller error interrupt.  This interrupt will occur
    // if there is a bus error during a transfer.
    //
    ROM_IntEnable(INT_UDMAERR);

    //
    // Enable the uDMA controller.
    //
    ROM_uDMAEnable();

    //
    // Point at the control table to use for channel control structures.
    //
    ROM_uDMAControlBaseSet(pui8ControlTable);

    //
    // Initialize the uDMA memory to memory transfers.
    //
    InitSWTransfer();

    //
    // Initialize the uDMA UART transfers.
    //
    InitUART0Transfer();

    //
    // Remember the current SysTick seconds count.
    //
    ui32PrevSeconds = g_ui32Seconds;

    //
    // Remember the current count of memory buffer transfers.
    //
    ui32PrevXferCount = g_ui32MemXferCount;

    //
    // Loop until 10 seconds have passed.  The processor is put to sleep
    // in this loop so that CPU utilization can be measured.
    //
    while(1)
    {
        //
        // Check to see if one second has elapsed.  If so, the make some
        // updates.
        //
        if(g_ui32Seconds != ui32PrevSeconds)
        {
            //
            // Print a message to the display showing the CPU usage percent.
            // The fractional part of the percent value is ignored.
            //
            GrContextFontSet(&g_sContext, g_psFontCmss18b);
            usnprintf(pcStrBuf, sizeof(pcStrBuf), "CPU utilization %2u%%",
                      g_ui32CPUUsage >> 16);
            GrStringDrawCentered(&g_sContext, pcStrBuf, -1,
                                 ui32CenterX, 180, 1);

            //
            // Tell the user how many seconds we have to go before ending.
            //
            usnprintf(pcStrBuf, sizeof(pcStrBuf), " Test ends in %d seconds ",
                      10 - g_ui32Seconds);
            GrStringDrawCentered(&g_sContext, pcStrBuf, -1,
                                 ui32CenterX, 220, 1);

            //
            // Remember the new seconds count.
            //
            ui32PrevSeconds = g_ui32Seconds;

            //
            // Calculate how many memory transfers have occurred since the last
            // second.
            //
            ui32XfersCompleted = g_ui32MemXferCount - ui32PrevXferCount;

            //
            // Remember the new transfer count.
            //
            ui32PrevXferCount = g_ui32MemXferCount;

            //
            // Compute how many bytes were transferred in the memory transfer
            // since the last second.
            //
            ui32BytesTransferred = ui32XfersCompleted * MEM_BUFFER_SIZE * 4;

            //
            // Print a message to the display showing the memory transfer rate.
            //
            GrContextFontSet(&g_sContext, g_psFontCmss16b);
            usnprintf(pcStrBuf, sizeof(pcStrBuf), " %8u Bytes/Sec ",
                      ui32BytesTransferred);
            GrStringDrawCentered(&g_sContext, pcStrBuf, -1,
                                 ui32CenterX, 94, 1);

            //
            // Calculate how many UART transfers have occurred since the last
            // second.  This expression is split to prevent a compiler warning
            // about undefined order of volatile accesses.
            //
            ui32XfersCompleted = g_ui32RxBufACount;
            ui32XfersCompleted += g_ui32RxBufBCount - ui32PrevUARTCount;

            //
            // Remember the new UART transfer count.  This expression is split
            // to prevent a compiler warning about undefined order of volatile
            // accesses.
            //
            ui32PrevUARTCount = g_ui32RxBufACount;
            ui32PrevUARTCount += g_ui32RxBufBCount;

            //
            // Compute how many bytes were transferred by the UART.  The number
            // of bytes received is multiplied by 2 so that the TX bytes
            // transferred are also accounted for.
            //
            ui32BytesTransferred = ui32XfersCompleted * UART_RXBUF_SIZE * 2;

            //
            // Print a message to the display showing the UART transfer rate.
            //
            usnprintf(pcStrBuf, sizeof(pcStrBuf), " %8u Bytes/Sec ",
                      ui32BytesTransferred);
            GrStringDrawCentered(&g_sContext, pcStrBuf, -1,
                                 ui32CenterX, 146, 1);
        }

        //
        // Put the processor to sleep if there is nothing to do.  This allows
        // the CPU usage routine to measure the number of free CPU cycles.
        // If the processor is sleeping a lot, it can be hard to connect to
        // the target with the debugger.
        //
        ROM_SysCtlSleep();

        //
        // See if we have run long enough and exit the loop if so.
        //
        if(g_ui32Seconds >= 10)
        {
            break;
        }
    }

    //
    // Indicate on the display that the example is stopped.
    //
    GrContextFontSet(&g_sContext, g_psFontCmss18b);
    GrContextForegroundSet(&g_sContext, ClrRed);
    GrStringDrawCentered(&g_sContext, "             Stopped             ", -1,
                         ui32CenterX, 220, 1);

    //
    // Disable uDMA and UART interrupts now that the test is complete.
    //
    ROM_IntDisable(INT_UART0);
    ROM_IntDisable(INT_UDMA);

    //
    // Loop forever with the CPU not sleeping, so the debugger can connect.
    //
    while(1)
    {
    }
}
