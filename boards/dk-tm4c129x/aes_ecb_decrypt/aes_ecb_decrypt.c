//*****************************************************************************
//
// aes_ecb_decrypt.c - Simple AES128 and AES256 ECB decryption demo.
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
#include "inc/hw_aes.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/aes.h"
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
//! <h1>AES128 and AES256 ECB Decryption Demo (aes_ecb_decrypt)</h1>
//!
//! Simple demo showing an decryption operation using the AES128 and AES256
//! modules in ECB mode.  A single block of data is decrypted.
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
// Sample plaintext, ciphertext, and key from the NIST SP 800-38A document.
//
//*****************************************************************************
uint32_t g_pui32AESPlainText[16] =
{
    0xe2bec16b, 0x969f402e, 0x117e3de9, 0x2a179373,
    0x578a2dae, 0x9cac031e, 0xac6fb79e, 0x518eaf45,
    0x461cc830, 0x11e45ca3, 0x19c1fbe5, 0xef520a1a,
    0x45249ff6, 0x179b4fdf, 0x7b412bad, 0x10376ce6
};

uint32_t g_pui32AES128Key[4] =
{
    0x16157e2b, 0xa6d2ae28, 0x8815f7ab, 0x3c4fcf09
};

uint32_t g_pui32AES256Key[8] =
{
    0x10eb3d60, 0xbe71ca15, 0xf0ae732b, 0x81777d85, 
    0x072c351f, 0xd708613b, 0xa310982d, 0xf4df1409
};

uint32_t g_pui32AES128CipherText[16] =
{
    0xb47bd73a, 0x60367a0d, 0xf3ca9ea8, 0x97ef6624,
    0x85d5d3f5, 0x9d69b903, 0x5a8985e7, 0xafbafd96,
    0x7fcdb143, 0x23ce8e59, 0xe3001b88, 0x880603ed,
    0x5e780c7b, 0x3fade827, 0x71202382, 0xd45d7204
};

uint32_t g_pui32AES256CipherText[16] =
{
    0xbdd1eef3, 0x3ca0d2b5, 0x7e5a4b06, 0xf881b13d,
    0x10cb1c59, 0x26ed10d4, 0x4aa75bdc, 0x70283631,
    0xb921edb6, 0xf9f4a69c, 0xb1e753f1, 0x1dedafbe,
    0x7a4b3023, 0xfff3f939, 0x8f8d7d06, 0xc7ec249e
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
// Round up length to nearest 16 byte boundary.  This is needed because all
// four data registers must be written at once.  This is handled in the AES
// driver, but if using uDMA, the length must rounded up.
//
//*****************************************************************************
uint32_t
LengthRoundUp(uint32_t ui32Length)
{
    uint32_t ui32Remainder;

    ui32Remainder = ui32Length % 16;
    if(ui32Remainder == 0)
    {
        return(ui32Length);
    }
    else
    {
        return(ui32Length + (16 - ui32Remainder));
    }
}

//*****************************************************************************
//
// The AES interrupt handler and interrupt flags.
//
//*****************************************************************************
static volatile bool g_bContextInIntFlag;
static volatile bool g_bDataInIntFlag;
static volatile bool g_bContextOutIntFlag;
static volatile bool g_bDataOutIntFlag;
static volatile bool g_bContextInDMADoneIntFlag;
static volatile bool g_bDataInDMADoneIntFlag;
static volatile bool g_bContextOutDMADoneIntFlag;
static volatile bool g_bDataOutDMADoneIntFlag;

void
AESIntHandler(void)
{
    uint32_t ui32IntStatus;

    //
    // Read the AES masked interrupt status.
    //
    ui32IntStatus = ROM_AESIntStatus(AES_BASE, true);

    //
    // Print a different message depending on the interrupt source.
    //
    if(ui32IntStatus & AES_INT_CONTEXT_IN)
    {
        ROM_AESIntDisable(AES_BASE, AES_INT_CONTEXT_IN);
        g_bContextInIntFlag = true;
        UARTprintf(" Context input registers are ready.\n");
    }
    if(ui32IntStatus & AES_INT_DATA_IN)
    {
        ROM_AESIntDisable(AES_BASE, AES_INT_DATA_IN);
        g_bDataInIntFlag = true;
        UARTprintf(" Data FIFO is ready to receive data.\n");
    }
    if(ui32IntStatus & AES_INT_CONTEXT_OUT)
    {
        ROM_AESIntDisable(AES_BASE, AES_INT_CONTEXT_OUT);
        g_bContextOutIntFlag = true;
        UARTprintf(" Context output registers are ready.\n");
    }
    if(ui32IntStatus & AES_INT_DATA_OUT)
    {
        ROM_AESIntDisable(AES_BASE, AES_INT_DATA_OUT);
        g_bDataOutIntFlag = true;
        UARTprintf(" Data FIFO is ready to provide data.\n");
    }
    if(ui32IntStatus & AES_INT_DMA_CONTEXT_IN)
    {
        ROM_AESIntClear(AES_BASE, AES_INT_DMA_CONTEXT_IN);
        g_bContextInDMADoneIntFlag = true;
        UARTprintf(" DMA completed a context write to the internal\n");
        UARTprintf(" registers.\n");
    }
    if(ui32IntStatus & AES_INT_DMA_DATA_IN)
    {
        ROM_AESIntClear(AES_BASE, AES_INT_DMA_DATA_IN);
        g_bDataInDMADoneIntFlag = true;
        UARTprintf(" DMA has written the last word of input data to\n");
        UARTprintf(" the internal FIFO of the engine.\n");
    }
    if(ui32IntStatus & AES_INT_DMA_CONTEXT_OUT)
    {
        ROM_AESIntClear(AES_BASE, AES_INT_DMA_CONTEXT_OUT);
        g_bContextOutDMADoneIntFlag = true;
        UARTprintf(" DMA completed the output context movement from\n");
        UARTprintf(" the internal registers.\n");
    }
    if(ui32IntStatus & AES_INT_DMA_DATA_OUT)
    {
        ROM_AESIntClear(AES_BASE, AES_INT_DMA_DATA_OUT);
        g_bDataOutDMADoneIntFlag = true;
        UARTprintf(" DMA has written the last word of process result.\n");
    }
}

//*****************************************************************************
//
// Perform an decryption operation.
//
//*****************************************************************************
bool
AESECBDecrypt(uint32_t ui32Keysize, uint32_t *pui32Src, uint32_t *pui32Dst,
              uint32_t *pui32Key, uint32_t ui32Length, bool bUseDMA)
{
    //
    // Perform a soft reset.
    //
    ROM_AESReset(AES_BASE);

    //
    // Clear the interrupt flags.
    //
    g_bContextInIntFlag = false;
    g_bDataInIntFlag = false;
    g_bContextOutIntFlag = false;
    g_bDataOutIntFlag = false;
    g_bContextInDMADoneIntFlag = false;
    g_bDataInDMADoneIntFlag = false;
    g_bContextOutDMADoneIntFlag = false;
    g_bDataOutDMADoneIntFlag = false;

    //
    // Enable all interrupts.
    //
    ROM_AESIntEnable(AES_BASE, (AES_INT_CONTEXT_IN | AES_INT_CONTEXT_OUT |
                                AES_INT_DATA_IN | AES_INT_DATA_OUT));

    //
    // Configure the AES module.
    //
    ROM_AESConfigSet(AES_BASE, (ui32Keysize | AES_CFG_DIR_DECRYPT |
                                AES_CFG_MODE_ECB));

    //
    // Write the key.
    //
    ROM_AESKey1Set(AES_BASE, pui32Key, ui32Keysize);

    //
    // Depending on the argument, perform the decryption
    // with or without uDMA.
    //
    if(bUseDMA)
    {
        //
        // Enable DMA interrupts.
        //
        ROM_AESIntEnable(AES_BASE, (AES_INT_DMA_CONTEXT_IN |
                                    AES_INT_DMA_DATA_IN |
                                    AES_INT_DMA_CONTEXT_OUT |
                                    AES_INT_DMA_DATA_OUT));

        //
        // Setup the DMA module to copy data in.
        //
        ROM_uDMAChannelAssign(UDMA_CH14_AES0DIN);
        ROM_uDMAChannelAttributeDisable(UDMA_CH14_AES0DIN,
                                        UDMA_ATTR_ALTSELECT |
                                        UDMA_ATTR_USEBURST |
                                        UDMA_ATTR_HIGH_PRIORITY |
                                        UDMA_ATTR_REQMASK);
        ROM_uDMAChannelControlSet(UDMA_CH14_AES0DIN | UDMA_PRI_SELECT,
                                  UDMA_SIZE_32 | UDMA_SRC_INC_32 |
                                  UDMA_DST_INC_NONE | UDMA_ARB_4 |
                                  UDMA_DST_PROT_PRIV);
        ROM_uDMAChannelTransferSet(UDMA_CH14_AES0DIN | UDMA_PRI_SELECT,
                                   UDMA_MODE_BASIC, (void *)pui32Dst,
                                   (void *)(AES_BASE + AES_O_DATA_IN_0),
                                   LengthRoundUp(ui32Length) / 4);
        UARTprintf("Data in DMA request enabled.\n");

        //
        // Setup the DMA module to copy the data out.
        //
        ROM_uDMAChannelAssign(UDMA_CH15_AES0DOUT);
        ROM_uDMAChannelAttributeDisable(UDMA_CH15_AES0DOUT,
                                        UDMA_ATTR_ALTSELECT |
                                        UDMA_ATTR_USEBURST |
                                        UDMA_ATTR_HIGH_PRIORITY |
                                        UDMA_ATTR_REQMASK);
        ROM_uDMAChannelControlSet(UDMA_CH15_AES0DOUT | UDMA_PRI_SELECT,
                                  UDMA_SIZE_32 | UDMA_SRC_INC_NONE |
                                  UDMA_DST_INC_32 | UDMA_ARB_4 |
                                  UDMA_SRC_PROT_PRIV);
        ROM_uDMAChannelTransferSet(UDMA_CH15_AES0DOUT | UDMA_PRI_SELECT,
                                   UDMA_MODE_BASIC,
                                   (void *)(AES_BASE + AES_O_DATA_IN_0),
                                   (void *)pui32Src,
                                   LengthRoundUp(ui32Length) / 4);
        UARTprintf("Data out DMA request enabled.\n");

        //
        // Write the length registers to start the process.
        //
        ROM_AESLengthSet(AES_BASE, (uint64_t)ui32Length);
        
        //
        // Enable the DMA channels to start the transfers.  This must be done after
        // writing the length to prevent data from copying before the context is 
        // truly ready.
        // 
        ROM_uDMAChannelEnable(UDMA_CH14_AES0DIN);
        ROM_uDMAChannelEnable(UDMA_CH15_AES0DOUT);

        //
        // Enable DMA requests
        //
        ROM_AESDMAEnable(AES_BASE, AES_DMA_DATA_IN | AES_DMA_DATA_OUT);

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
        // Perform the decryption.
        //
        ROM_AESDataProcess(AES_BASE, pui32Src, pui32Dst, ui32Length);
    }

    return(true);
}

//*****************************************************************************
//
// Initialize the AES and CCM modules.
//
//*****************************************************************************
bool
AESInit(void)
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
// This example decrypts blocks of plaintext using AES128 in ECB mode.  It
// does the decryption first without uDMA and then with uDMA.  The results
// are checked after each operation.
//
//*****************************************************************************
int
main(void)
{
    uint32_t pui32PlainText[16], ui32Errors, ui32Idx, ui32SysClock;
    uint32_t ui32Keysize;
    tContext sContext;
    uint8_t  ui8Loop;
    
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
    FrameDraw(&sContext, "aes-ecb-decrypt");
    
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

    //
    // Enable stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPUStackingEnable();

    //
    // Configure the system clock to run off the internal 16MHz oscillator.
    //
    MAP_SysCtlClockFreqSet(SYSCTL_OSC_INT | SYSCTL_USE_OSC, 16000000);

    //
    // Enable AES interrupts.
    //
    ROM_IntEnable(INT_AES0);

    //
    // Enable debug output on UART0 and print a welcome message.
    //
    ConfigureUART();
    UARTprintf("Starting AES ECB decryption demo.\n");
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
    // Initialize the CCM and AES modules.
    //
    if(!AESInit())
    {
        UARTprintf("Initialization of the AES module failed.\n");
        ui32Errors |= 0x00000001;
    }

    //
    // Perform the same operation with 128bit key first, then 256bit key.
    //
    for(ui8Loop = 0; ui8Loop < 2; ui8Loop++)
    {

        ui32Keysize = (ui8Loop == 0)?AES_CFG_KEY_SIZE_128BIT:
                                     AES_CFG_KEY_SIZE_256BIT;
        UARTprintf("\nKey Size: %sbit\n", ((ui8Loop == 0)?"128":"256"));

        //
        // Clear the array containing the plaintext.
        //
        for(ui32Idx = 0; ui32Idx < 16; ui32Idx++)
        {
            pui32PlainText[ui32Idx] = 0;
        }

        //
        // Perform the decryption without uDMA.
        //
        UARTprintf("Performing decryption without uDMA.\n");
        AESECBDecrypt(ui32Keysize,
               (ui8Loop == 0)?g_pui32AES128CipherText:g_pui32AES256CipherText,
               pui32PlainText,
               (ui8Loop == 0)?g_pui32AES128Key:g_pui32AES256Key,
               64, false);

        //
        // Check the result.
        //
        for(ui32Idx = 0; ui32Idx < 16; ui32Idx++)
        {
            if(pui32PlainText[ui32Idx] != g_pui32AESPlainText[ui32Idx])
            {
                UARTprintf("Plaintext mismatch on word %d. Exp: 0x%x, Act: 0x%x\n",
                           ui32Idx, g_pui32AESPlainText[ui32Idx],
                           pui32PlainText[ui32Idx]);
                ui32Errors |= (ui32Idx << 16) | 0x00000002;
            }
        }

        //
        // Clear the array containing the plaintext.
        //
        for(ui32Idx = 0; ui32Idx < 16; ui32Idx++)
        {
            pui32PlainText[ui32Idx] = 0;
        }

        //
        // Perform the decryption with uDMA.
        //
        UARTprintf("Performing decryption with uDMA.\n");
        AESECBDecrypt(ui32Keysize, pui32PlainText,
               (ui8Loop == 0)?g_pui32AES128CipherText:g_pui32AES256CipherText,
               (ui8Loop == 0)?g_pui32AES128Key:g_pui32AES256Key,
               64, true);

        //
        // Check the result.
        //
        for(ui32Idx = 0; ui32Idx < 16; ui32Idx++)
        {
            if(pui32PlainText[ui32Idx] != g_pui32AESPlainText[ui32Idx])
            {
                UARTprintf("Plaintext mismatch on word %d. Exp: 0x%x, Act: 0x%x\n",
                           ui32Idx, g_pui32AESPlainText[ui32Idx],
                           pui32PlainText[ui32Idx]);
                ui32Errors |= (ui32Idx << 16) | 0x00000004;
            }
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
