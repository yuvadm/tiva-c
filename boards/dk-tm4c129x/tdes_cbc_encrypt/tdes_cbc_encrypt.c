//*****************************************************************************
//
// tdes_cbc_encrypt.c - Simple TDES CBC encryption demo.
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
#include "inc/hw_des.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/des.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/udma.h"
#include "grlib/grlib.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>TDES CBC Encryption Demo (tdes_cbc_encrypt)</h1>
//!
//! Simple demo showing an encryption operation using the DES module with
//! triple DES in CBC mode.  A single block of data is encrypted at a time.
//! The module is also capable of performing in DES mode, but this has been
//! proven to be cryptographically insecure.  CBC mode is also not recommended
//! because it will always produce the same ciphertext for a block of
//! plaintext.  CBC and CFB modes are recommended instead.
//!
//! Please note that the use of interrupts and uDMA is not required for the
//! operation of the module.  It is only done for demonstration purposes.
//
//*****************************************************************************

//*****************************************************************************
//
// Configuration defines.
//
//*****************************************************************************
#define CCM_LOOP_TIMEOUT        500000

//*****************************************************************************
//
// The DMA control structure table.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
tDMAControlTable g_psDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(g_psDMAControlTable, 1024)
tDMAControlTable g_psDMAControlTable[64];
#else
tDMAControlTable g_psDMAControlTable[64] __attribute__((aligned(1024)));
#endif

//*****************************************************************************
//
// Triple DES test vector.
// The plaintext is from the AES example. The key was randomly generated.
// The resulting ciphertext was then generated using a Perl script with the
// Crypt::DES_EDE3 module.
//
//*****************************************************************************
uint32_t g_pui32TDESPlainText[16] =
{
    0xe2bec16b, 0x969f402e, 0x117e3de9, 0x2a179373,
    0x578a2dae, 0x9cac031e, 0xac6fb79e, 0x518eaf45,
    0x461cc830, 0x11e45ca3, 0x19c1fbe5, 0xef520a1a,
    0x45249ff6, 0x179b4fdf, 0x7b412bad, 0x10376ce6
};

uint32_t g_pui32TDESKey[6] =
{
    0xc7f51c87, 0x8076211f, 0x5de5c871, 0xa243cf7e,
    0xd25fdb75, 0xad73068f
};

uint32_t g_pui32TDESIV[2] =
{
    0x6d8ecac4, 0x3b27c885
};

uint32_t g_pui32TDESCipherText[16] =
{
    0x24c69385, 0xb338be54, 0x6eeeb276, 0x1a952b4e,
    0x7242ce4b, 0x9ec147cf, 0x765916ee, 0x3d25e685,
    0xfe5865b4, 0xf2238cb8, 0x2a5b68d5, 0x0f79a41a,
    0x6f4a7601, 0x7a57235f, 0xce84d08a, 0x1a34d011
};

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
// Round up length to nearest 8 byte boundary.  This is needed because all
// two data registers must be written at once.  This is handled in the DES
// driver, but if using uDMA, the length must rounded up.
//
//*****************************************************************************
uint32_t
LengthRoundUp(uint32_t ui32Length)
{
    uint32_t ui32Remainder;

    ui32Remainder = ui32Length % 8;
    if(ui32Remainder == 0)
    {
        return(ui32Length);
    }
    else
    {
        return(ui32Length + (8 - ui32Remainder));
    }
}

//*****************************************************************************
//
// The TDES interrupt handler and interrupt flags.
//
//*****************************************************************************
static volatile bool g_bContextInIntFlag;
static volatile bool g_bDataInIntFlag;
static volatile bool g_bDataOutIntFlag;
static volatile bool g_bContextInDMADoneIntFlag;
static volatile bool g_bDataInDMADoneIntFlag;
static volatile bool g_bDataOutDMADoneIntFlag;

void
TDESIntHandler(void)
{
    uint32_t ui32IntStatus;

    //
    // Read the DES masked interrupt status.
    //
    ui32IntStatus = ROM_DESIntStatus(DES_BASE, true);

    //
    // Print a different message depending on the interrupt source.
    //
    if(ui32IntStatus & DES_INT_CONTEXT_IN)
    {
        ROM_DESIntDisable(DES_BASE, DES_INT_CONTEXT_IN);
        g_bContextInIntFlag = true;
        UARTprintf(" Context input registers are ready.\n");
    }
    if(ui32IntStatus & DES_INT_DATA_IN)
    {
        ROM_DESIntDisable(DES_BASE, DES_INT_DATA_IN);
        g_bDataInIntFlag = true;
        UARTprintf(" Data FIFO is ready to receive data.\n");
    }
    if(ui32IntStatus & DES_INT_DATA_OUT)
    {
        ROM_DESIntDisable(DES_BASE, DES_INT_DATA_OUT);
        g_bDataOutIntFlag = true;
        UARTprintf(" Data FIFO is ready to provide data.\n");
    }
    if(ui32IntStatus & DES_INT_DMA_CONTEXT_IN)
    {
        ROM_DESIntClear(DES_BASE, DES_INT_DMA_CONTEXT_IN);
        g_bContextInDMADoneIntFlag = true;
        UARTprintf(" DMA completed a context write to the internal\n");
        UARTprintf(" registers.\n");
    }
    if(ui32IntStatus & DES_INT_DMA_DATA_IN)
    {
        ROM_DESIntClear(DES_BASE, DES_INT_DMA_DATA_IN);
        g_bDataInDMADoneIntFlag = true;
        UARTprintf(" DMA has written the last word of input data to\n");
        UARTprintf(" the internal FIFO of the engine.\n");
    }
    if(ui32IntStatus & DES_INT_DMA_DATA_OUT)
    {
        ROM_DESIntClear(DES_BASE, DES_INT_DMA_DATA_OUT);
        g_bDataOutDMADoneIntFlag = true;
        UARTprintf(" DMA has written the last word of process result.\n");
    }
}

//*****************************************************************************
//
// Perform an encryption operation.
//
//*****************************************************************************
bool
TDESCBCEncrypt(uint32_t *pui32Src, uint32_t *pui32Dst, uint32_t *pui32Key,
                 uint32_t ui32Length, uint32_t *pui32IV, bool bUseDMA)
{
    //
    // Perform a soft reset.
    //
    ROM_DESReset(DES_BASE);

    //
    // Clear the interrupt flags.
    //
    g_bContextInIntFlag = false;
    g_bDataInIntFlag = false;
    g_bDataOutIntFlag = false;
    g_bContextInDMADoneIntFlag = false;
    g_bDataInDMADoneIntFlag = false;
    g_bDataOutDMADoneIntFlag = false;

    //
    // Enable all interrupts.
    //
    ROM_DESIntEnable(DES_BASE, (DES_INT_CONTEXT_IN |
                                DES_INT_DATA_IN | DES_INT_DATA_OUT));

    //
    // Configure the DES module.
    //
    ROM_DESConfigSet(DES_BASE, (DES_CFG_DIR_ENCRYPT | DES_CFG_TRIPLE |
                                DES_CFG_MODE_CBC));

    //
    // Write the key.
    //
    ROM_DESKeySet(DES_BASE, pui32Key);

    //
    // Write the IV.
    //
    ROM_DESIVSet(DES_BASE, pui32IV);

    //
    // Depending on the argument, perform the encryption
    // with or without uDMA.
    //
    if(bUseDMA)
    {
        //
        // Enable DMA interrupts.
        //
        ROM_DESIntEnable(DES_BASE, (DES_INT_DMA_CONTEXT_IN |
                                    DES_INT_DMA_DATA_IN |
                                    DES_INT_DMA_DATA_OUT));

        //
        // Setup the DMA module to copy data in.
        //
        ROM_uDMAChannelAssign(UDMA_CH21_DES0DIN);
        ROM_uDMAChannelAttributeDisable(UDMA_CH21_DES0DIN,
                                        UDMA_ATTR_ALTSELECT |
                                        UDMA_ATTR_USEBURST |
                                        UDMA_ATTR_HIGH_PRIORITY |
                                        UDMA_ATTR_REQMASK);
        ROM_uDMAChannelControlSet(UDMA_CH21_DES0DIN | UDMA_PRI_SELECT,
                                  UDMA_SIZE_32 | UDMA_SRC_INC_32 |
                                  UDMA_DST_INC_NONE | UDMA_ARB_2 |
                                  UDMA_DST_PROT_PRIV);
        ROM_uDMAChannelTransferSet(UDMA_CH21_DES0DIN | UDMA_PRI_SELECT,
                                   UDMA_MODE_BASIC, (void *)pui32Src,
                                   (void *)(DES_BASE + DES_O_DATA_L),
                                   LengthRoundUp(ui32Length) / 4);
        UARTprintf("Data in DMA request enabled.\n");

        //
        // Setup the DMA module to copy the data out.
        //
        ROM_uDMAChannelAssign(UDMA_CH22_DES0DOUT);
        ROM_uDMAChannelAttributeDisable(UDMA_CH22_DES0DOUT,
                                        UDMA_ATTR_ALTSELECT |
                                        UDMA_ATTR_USEBURST |
                                        UDMA_ATTR_HIGH_PRIORITY |
                                        UDMA_ATTR_REQMASK);
        ROM_uDMAChannelControlSet(UDMA_CH22_DES0DOUT | UDMA_PRI_SELECT,
                                  UDMA_SIZE_32 | UDMA_SRC_INC_NONE |
                                  UDMA_DST_INC_32 | UDMA_ARB_2 |
                                  UDMA_SRC_PROT_PRIV);
        ROM_uDMAChannelTransferSet(UDMA_CH22_DES0DOUT | UDMA_PRI_SELECT,
                                   UDMA_MODE_BASIC,
                                   (void *)(DES_BASE + DES_O_DATA_L),
                                   (void *)pui32Dst,
                                   LengthRoundUp(ui32Length) / 4);
        UARTprintf("Data out DMA request enabled.\n");

        //
        // Enable DMA requests
        //
        ROM_DESDMAEnable(DES_BASE, DES_DMA_DATA_IN | DES_DMA_DATA_OUT);

        //
        // Write the length registers to start the process.
        //
        ROM_DESLengthSet(DES_BASE, ui32Length);

        //
        // Enable the DMA channels to start the transfers.  This must be done after
        // writing the length to prevent data from copying before the context is
        // truly ready.
        //
        ROM_uDMAChannelEnable(UDMA_CH21_DES0DIN);
        ROM_uDMAChannelEnable(UDMA_CH22_DES0DOUT);

        //
        // Wait for the data in DMA done interrupt.
        //
        while(!g_bDataInDMADoneIntFlag)
        {
        }

        //
        // Wait for the data out DMA done interrupt.
        //
        while(!g_bDataOutDMADoneIntFlag)
        {
        }
    }
    else
    {
        //
        // Perform the encryption.
        //
        ROM_DESDataProcess(DES_BASE, pui32Src, pui32Dst, ui32Length);
    }

    return(true);
}

//*****************************************************************************
//
// Initialize the DES and CCM modules.
//
//*****************************************************************************
bool
DESInit(void)
{
    uint32_t ui32Loop;

    //
    // Check that the CCM peripheral is present.
    //
    if(!ROM_SysCtlPeripheralPresent(SYSCTL_PERIPH_CCM0))
    {
        UARTprintf("No CCM peripheral found!\n");

        //
        // Return failure.
        //
        return(false);
    }

    //
    // The hardware is available, enable it.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_CCM0);

    //
    // Wait for the peripheral to be ready.
    //
    ui32Loop = 0;
    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_CCM0))
    {
        //
        // Increment our poll counter.
        //
        ui32Loop++;

        if(ui32Loop > CCM_LOOP_TIMEOUT)
        {
            //
            // Timed out, notify and spin.
            //
            UARTprintf("Time out on CCM ready after enable.\n");

            //
            // Return failure.
            //
            return(false);
        }
    }

    //
    // Reset the peripheral to ensure we are starting from a known condition.
    //
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_CCM0);

    //
    // Wait for the peripheral to be ready again.
    //
    ui32Loop = 0;
    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_CCM0))
    {
        //
        // Increment our poll counter.
        //
        ui32Loop++;

        if(ui32Loop > CCM_LOOP_TIMEOUT)
        {
            //
            // Timed out, spin.
            //
            UARTprintf("Time out on CCM ready after reset.\n");

            //
            // Return failure.
            //
            return(false);
        }
    }

    //
    // Return initialization success.
    //
    return(true);
}

//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************
void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    ROM_UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, 16000000);
}

//*****************************************************************************
//
// This example encrypts blocks of plaintext using TDES in CBC mode.  It
// does the encryption first without uDMA and then with uDMA.  The results
// are checked after each operation.
//
//*****************************************************************************
int
main(void)
{
    uint32_t pui32CipherText[16], ui32Errors, ui32Idx, ui32SysClock;
    tContext sContext;

    //
    // Run from the PLL at 120 MHz.
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(ui32SysClock);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&sContext, "tdes-cbc-encrypt");

    //
    // Show some instructions on the display
    //
    GrContextFontSet(&sContext, g_psFontCm20);
    GrContextForegroundSet(&sContext, ClrWhite);
    GrStringDrawCentered(&sContext, "Connect a terminal to", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 60, false);
    GrStringDrawCentered(&sContext, "UART0 (115200,N,8,1)", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 80, false);
    GrStringDrawCentered(&sContext, "for more information.", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 100, false);

    //
    // Initialize local variables.
    //
    ui32Errors = 0;
    for(ui32Idx = 0; ui32Idx < 16; ui32Idx++)
    {
        pui32CipherText[ui32Idx] = 0;
    }

    //
    // Enable stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPUStackingEnable();

    //
    // Enable DES interrupts.
    //
    ROM_IntEnable(INT_DES0);

    //
    // Enable debug output on UART0 and print a welcome message.
    //
    ConfigureUART();
    UARTprintf("Starting TDES CBC encryption demo.\n");
    GrStringDrawCentered(&sContext, "Starting demo...", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 140, false);

    //
    // Enable the uDMA module.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);

    //
    // Setup the control table.
    //
    ROM_uDMAEnable();
    ROM_uDMAControlBaseSet(g_psDMAControlTable);

    //
    // Initialize the CCM and DES modules.
    //
    if(!DESInit())
    {
        UARTprintf("Initialization of the DES module failed.\n");
        ui32Errors |= 0x00000001;
    }

    //
    // Perform the encryption without uDMA.
    //
    UARTprintf("Performing encryption without uDMA.\n");
    TDESCBCEncrypt(g_pui32TDESPlainText, pui32CipherText, g_pui32TDESKey,
                   64, g_pui32TDESIV, false);

    //
    // Check the result.
    //
    for(ui32Idx = 0; ui32Idx < 16; ui32Idx++)
    {
        if(pui32CipherText[ui32Idx] != g_pui32TDESCipherText[ui32Idx])
        {
            UARTprintf("Ciphertext mismatch on word %d. Exp: 0x%x, Act: "
                       "0x%x\n", ui32Idx, g_pui32TDESCipherText[ui32Idx],
                       pui32CipherText[ui32Idx]);
            ui32Errors |= (ui32Idx << 16) | 0x00000002;
        }
    }

    //
    // Clear the array containing the ciphertext.
    //
    for(ui32Idx = 0; ui32Idx < 16; ui32Idx++)
    {
        pui32CipherText[ui32Idx] = 0;
    }

    //
    // Perform the encryption with uDMA.
    //
    UARTprintf("Performing encryption with uDMA.\n");
    TDESCBCEncrypt(g_pui32TDESPlainText, pui32CipherText, g_pui32TDESKey,
                   64, g_pui32TDESIV, true);

    //
    // Check the result.
    //
    for(ui32Idx = 0; ui32Idx < 16; ui32Idx++)
    {
        if(pui32CipherText[ui32Idx] != g_pui32TDESCipherText[ui32Idx])
        {
            UARTprintf("Ciphertext mismatch on word %d. Exp: 0x%x, Act: "
                       "0x%x\n", ui32Idx, g_pui32TDESCipherText[ui32Idx],
                       pui32CipherText[ui32Idx]);
            ui32Errors |= (ui32Idx << 16) | 0x00000004;
        }
    }

    //
    // Finished.
    //
    if(ui32Errors)
    {
        UARTprintf("Demo failed with error code 0x%x.\n", ui32Errors);
        GrStringDrawCentered(&sContext, "Demo failed.", -1,
                             GrContextDpyWidthGet(&sContext) / 2, 180, false);
    }
    else
    {
        UARTprintf("Demo completed successfully.\n");
        GrStringDrawCentered(&sContext, "Demo passed.", -1,
                             GrContextDpyWidthGet(&sContext) / 2, 180, false);
    }

    while(1)
    {
    }
}
