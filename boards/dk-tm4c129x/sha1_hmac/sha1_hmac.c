//*****************************************************************************
//
// sha1_hmac.c - Simple SHA1 HMAC demo.
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
#include "inc/hw_shamd5.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/shamd5.h"
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
//! <h1>SHA1 HMAC Demo (sha1_hmac)</h1>
//!
//! Simple example showing SHA1 HMAC generation using a block of random data.
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
// Source data for producing HMACs. This array contains 1024 bytes of
// randomly generated data.
//
//*****************************************************************************
uint32_t g_pui32RandomData[] =
{
    0x7c68c9ec, 0x72af34b3, 0xca0edf2e, 0x60f4860d, 0x50cfa1dc, 0x9a2b538c,
    0x98450274, 0x60f5c272, 0x7317d78e, 0x2361ca0e, 0xfa4a52b1, 0x658f729b,
    0x5267f9d9, 0x1bccd3ca, 0x2f0bb993, 0x1be38a3d, 0x00bd2d2a, 0x97405e63,
    0xe3efd585, 0xb02d1588, 0xe55d71c8, 0x43a27ecf, 0x5fd275db, 0x73ad8f06,
    0x88f55495, 0x68922493, 0x03ea6039, 0xe40a678a, 0x052847ce, 0xf7a28b46,
    0x3b60c73e, 0x3f08dbd4, 0x2a66b3a6, 0xcf398b15, 0xacbfc6d8, 0x6c15a285,
    0x997d0e01, 0xbfd12e26, 0xa26bc485, 0xb8946d2f, 0x0f84742b, 0x5be82a2f,
    0x8d2e2cc7, 0xc7a1dea6, 0xcfaa6cb6, 0xe706434c, 0x079810d0, 0x5eca9400,
    0x7b92dd1c, 0x1ec552e8, 0xa74ae9c3, 0x2e859af5, 0x8d9d1a35, 0x07ff6040,
    0xc0b19670, 0x2e348aa8, 0xed89efea, 0x3262e8f0, 0x45093372, 0x8f8bae5c,
    0x505d64bb, 0x9a172079, 0x327b5f67, 0xa3a12ba8, 0x7f573054, 0xd3d5f778,
    0xbc1bd124, 0x0d0ad1c6, 0x24ac345b, 0x4f50084a, 0x302a5985, 0xfa3e8b86,
    0x2022c497, 0xd297e4b4, 0xd1c53c01, 0x6e541890, 0x93ec53c6, 0x24c5ce2b,
    0xdd38e334, 0x078a0334, 0x2a470b22, 0xadad86b4, 0x7b2041db, 0xc74ce30b,
    0x8e6dc4ca, 0x273b85c8, 0x339d2334, 0x86d1dacc, 0xd588e165, 0xcee15221,
    0x8e11a0a1, 0x9315a6c2, 0x53e9fa9a, 0xf4bb6d7a, 0x421cb9ec, 0x1f370567,
    0xfd8c880f, 0xd20797cd, 0x90aee852, 0x2a2f966a, 0x126ffcdd, 0x44a2f09a,
    0xbac72ac4, 0x77d588c5, 0x77b53c09, 0x275b8828, 0x778a2be5, 0x40167d1e,
    0x550c0c94, 0x14e070e7, 0x597ff5a3, 0xbef40dc2, 0x8306d119, 0x6a8d29a6,
    0xb5d8e740, 0x52a37fe2, 0xdf34ad27, 0x1bb885fd, 0x6dd352f8, 0x8b0d62b5,
    0x5c82d35f, 0x0eb84312, 0xd2c7823a, 0x494f7a00, 0x30680642, 0x01fa9460,
    0xdc63956f, 0x70fa0b53, 0xd0865e78, 0x3a52e983, 0x318a881c, 0x4d113947,
    0xc0f302df, 0x6b2027fb, 0x1078566d, 0xd71d39a6, 0xcdd00388, 0x119e3c4e,
    0x4ddbf1c6, 0xb371eb0f, 0xdcbd768f, 0x2fc5b5e8, 0xc67a2efe, 0x29d18630,
    0xb389d68f, 0x26a71f13, 0x43583b57, 0x56f5eae8, 0x2edc7cd5, 0xcc93d41e,
    0xab691f87, 0x51ab1d8e, 0x37c2966e, 0x19ccd9ec, 0xb782124a, 0xdefc2804,
    0xea3bde3c, 0x46d81e08, 0xf828d58e, 0x757a39d3, 0xc92f1b5f, 0x56a2b368,
    0x1bbbb9b9, 0x46086ac7, 0x8a343144, 0x1675157a, 0x28ac0cf1, 0xb8695178,
    0x25fc4cec, 0x3f23a44e, 0x0a697977, 0x525794ad, 0xf920e15c, 0x49a0a7a7,
    0x1f54cafb, 0x7357b64c, 0x6d3a19c6, 0x5efb526d, 0x3d37f6e2, 0xd4f5835b,
    0x6ff454ee, 0x4f2a311c, 0x83cc4a40, 0x003036e9, 0xd481bf33, 0x38868b3c,
    0x63ee4445, 0x58426a29, 0xa022ae59, 0x07deb8ce, 0xfe3e673d, 0x176aa368,
    0xf2b18641, 0xbadeccd8, 0xea7a72b4, 0x72ccf0a0, 0xcdee3b08, 0x1689c54f,
    0xd577085a, 0xd9d79bd1, 0x089fa69a, 0x03fdaf65, 0x855e5697, 0x5788c00c,
    0x1139e03e, 0x48f4305f, 0x2d8ad2fd, 0x71ab04b5, 0xf5c7871c, 0x76801f21,
    0x329a590e, 0xe8e982a2, 0xdb67783e, 0x26ebf88b, 0x13ac5de7, 0x69b07707,
    0x2bc54e92, 0xc2556f94, 0x6d21bc3b, 0x3a230d0c, 0x4e02eeec, 0x53605beb,
    0x3a31e796, 0x6e186887, 0x8f93356e, 0xfa2342e4, 0xfbf2f519, 0x7ae95455,
    0xad6e9d94, 0xd942c7ab, 0x624f7aed, 0xd4158624, 0x82a0c0a9, 0x6d79b262,
    0xa7b9c84d, 0x2015bfeb, 0x462c7267, 0x44a17743, 0x7d207f71, 0xc2ab7566,
    0xaa833e65, 0x0a6c385e, 0x3b2d85f1, 0x8a4821a8, 0x62bf5742, 0xf55cf0e1,
    0xfc07d0d9, 0x54910235, 0xe8ae66c9, 0x9beb7306, 0xe5671f9e, 0x3332ad03,
    0xdb2343b6, 0x124332ac, 0xf595c7fb, 0xda2c72b0
};

//*****************************************************************************
//
// Sample key for generating an HMAC.  This array contains 64 bytes (512 bits)
// of randomly generated data.
//
//*****************************************************************************
uint32_t g_pui32SHA1HMACKey[] =
{
    0x8a5f1b22, 0xcb935d29, 0xcc1ac092, 0x5dad8c9e, 0x6a83b39f, 0x8607dc60,
    0xda0ba4d2, 0xf49b0fa2, 0xaf35d524, 0xffa8001d, 0xbcc931e8, 0x4a2c99ef,
    0x7fa297ab, 0xab943bae, 0x07c61cc4, 0x47c8627d
};

//*****************************************************************************
//
// Expected HMAC results.  These results have been separately verified using
// a Perl script.
//
//*****************************************************************************
typedef struct SHA1TestVectorStruct
{
    uint32_t pui32HMACResult[5];
    uint32_t ui32DataLength;
}
tSHA1TestVector;

tSHA1TestVector g_psSHA1TestVectors[] =
{
    {
        { 0x06d4db72, 0xa1f8c22a, 0x869efcc5, 0xca8bc8fc, 0x30b77c92 },
        1024
    },
    {
        { 0x5c01f196, 0xbad6b65e, 0x73eed7a2, 0x61665901, 0x7320b932 },
        1000
    },
    {
        { 0xee4dfa06, 0x78f74a98, 0x109a6d09, 0xa9470d90, 0xeb550d5f },
        0
    }
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
//! The SHA/MD5 interrupt handler
//
//*****************************************************************************

//
// Flags to check that interrupts were successfully generated.
//
volatile bool g_bContextReadyFlag;
volatile bool g_bParthashReadyFlag;
volatile bool g_bInputReadyFlag;
volatile bool g_bOutputReadyFlag;
volatile bool g_bContextInDMADoneFlag;
volatile bool g_bDataInDMADoneFlag;
volatile bool g_bContextOutDMADoneFlag;

void
SHAMD5IntHandler(void)
{
    uint32_t ui32IntStatus;

    //
    // Read the SHA/MD5 masked interrupt status.
    //
    ui32IntStatus = ROM_SHAMD5IntStatus(SHAMD5_BASE, true);

    //
    // Print a different message depending on the interrupt source.
    //
    if(ui32IntStatus & SHAMD5_INT_CONTEXT_READY)
    {
        ROM_SHAMD5IntDisable(SHAMD5_BASE, SHAMD5_INT_CONTEXT_READY);
        g_bContextReadyFlag = true;
        UARTprintf("Context input registers are ready.\n");
    }
    if(ui32IntStatus & SHAMD5_INT_PARTHASH_READY)
    {
        ROM_SHAMD5IntDisable(SHAMD5_BASE, SHAMD5_INT_PARTHASH_READY);
        UARTprintf("Context output registers are ready after a\n");
        UARTprintf("context switch.\n");
    }
    if(ui32IntStatus & SHAMD5_INT_INPUT_READY)
    {
        ROM_SHAMD5IntDisable(SHAMD5_BASE, SHAMD5_INT_INPUT_READY);
        g_bInputReadyFlag = true;
        UARTprintf("Data FIFO is ready to receive data.\n");
    }
    if(ui32IntStatus & SHAMD5_INT_OUTPUT_READY)
    {
        ROM_SHAMD5IntDisable(SHAMD5_BASE, SHAMD5_INT_OUTPUT_READY);
        g_bOutputReadyFlag = true;
        UARTprintf("Context output registers are ready.\n");
    }
    if(ui32IntStatus & SHAMD5_INT_DMA_CONTEXT_IN)
    {
        ROM_SHAMD5IntClear(SHAMD5_BASE, SHAMD5_INT_DMA_CONTEXT_IN);
        g_bContextInDMADoneFlag = true;
        UARTprintf("DMA completed a context write to the internal\n");
        UARTprintf("registers.\n");
    }
    if(ui32IntStatus & SHAMD5_INT_DMA_DATA_IN)
    {
        ROM_SHAMD5IntClear(SHAMD5_BASE, SHAMD5_INT_DMA_DATA_IN);
        g_bDataInDMADoneFlag = true;
        UARTprintf("DMA has written the last word of input data to\n");
        UARTprintf("the internal FIFO of the engine.\n");
    }
    if(ui32IntStatus & SHAMD5_INT_DMA_CONTEXT_OUT)
    {
        ROM_SHAMD5IntClear(SHAMD5_BASE, SHAMD5_INT_DMA_CONTEXT_OUT);
        g_bContextOutDMADoneFlag = true;
        UARTprintf("DMA completed the output context movement from\n");
        UARTprintf("the internal registers.\n");
    }
}

//*****************************************************************************
//
// Generate a HMAC for the given data.
//
//*****************************************************************************
void
SHA1HMACGenerate(uint32_t *pui32Data, uint32_t ui32DataLength,
                 uint32_t *pui32HMACKey, uint32_t *pui32HMACResult,
                 bool bUseDMA, bool bPreProcessedKey)
{
    //
    // Perform a soft reset of the SHA module.
    //
    ROM_SHAMD5Reset(SHAMD5_BASE);

    //
    // Clear the flags
    //
    g_bContextReadyFlag = false;
    g_bInputReadyFlag = false;
    g_bDataInDMADoneFlag = false;
    g_bContextOutDMADoneFlag = false;

    //
    // Enable interrupts.
    //
    ROM_SHAMD5IntEnable(SHAMD5_BASE, (SHAMD5_INT_CONTEXT_READY |
                                      SHAMD5_INT_PARTHASH_READY |
                                      SHAMD5_INT_INPUT_READY |
                                      SHAMD5_INT_OUTPUT_READY));

    //
    // Wait for the context ready flag.
    //
    while(!g_bContextReadyFlag)
    {
    }

    //
    // Configure the SHA/MD5 module.
    //
    ROM_SHAMD5ConfigSet(SHAMD5_BASE, SHAMD5_ALGO_HMAC_SHA1);

    //
    // Write the key.
    //
    if(bPreProcessedKey)
    {
        ROM_SHAMD5HMACPPKeySet(SHAMD5_BASE, pui32HMACKey);
    }
    else
    {
        ROM_SHAMD5HMACKeySet(SHAMD5_BASE, pui32HMACKey);
    }

    //
    // Use DMA to write the data into the SHA/MD5 module.
    //
    if(bUseDMA)
    {
        //
        // Enable DMA done interrupts.
        //
        ROM_SHAMD5IntEnable(SHAMD5_BASE, (SHAMD5_INT_DMA_CONTEXT_IN |
                                          SHAMD5_INT_DMA_DATA_IN |
                                          SHAMD5_INT_DMA_CONTEXT_OUT));

        if(ui32DataLength != 0)
        {
            //
            // Setup the DMA module to copy data in.
            //
            ROM_uDMAChannelAssign(UDMA_CH5_SHAMD50DIN);
            ROM_uDMAChannelAttributeDisable(UDMA_CH5_SHAMD50DIN,
                                            UDMA_ATTR_ALTSELECT |
                                            UDMA_ATTR_USEBURST |
                                            UDMA_ATTR_HIGH_PRIORITY |
                                            UDMA_ATTR_REQMASK);
            ROM_uDMAChannelControlSet(UDMA_CH5_SHAMD50DIN | UDMA_PRI_SELECT,
                                      UDMA_SIZE_32 | UDMA_SRC_INC_32 |
                                      UDMA_DST_INC_NONE | UDMA_ARB_16 |
                                      UDMA_DST_PROT_PRIV);
            ROM_uDMAChannelTransferSet(UDMA_CH5_SHAMD50DIN | UDMA_PRI_SELECT,
                                       UDMA_MODE_BASIC, (void *)pui32Data,
                                       (void *)(SHAMD5_BASE +
                                                SHAMD5_O_DATA_0_IN),
                                       ui32DataLength / 4);
            ROM_uDMAChannelEnable(UDMA_CH5_SHAMD50DIN);
            UARTprintf("Data in DMA request enabled.\n");
        }

        //
        // Setup the DMA module to copy the HMAC out.
        //
        ROM_uDMAChannelAssign(UDMA_CH6_SHAMD50COUT);
        ROM_uDMAChannelAttributeDisable(UDMA_CH6_SHAMD50COUT,
                                        UDMA_ATTR_ALTSELECT |
                                        UDMA_ATTR_USEBURST |
                                        UDMA_ATTR_HIGH_PRIORITY |
                                        UDMA_ATTR_REQMASK);
        ROM_uDMAChannelControlSet(UDMA_CH6_SHAMD50COUT | UDMA_PRI_SELECT,
                                  UDMA_SIZE_32 | UDMA_SRC_INC_32 |
                                  UDMA_DST_INC_32 | UDMA_ARB_8 |
                                  UDMA_SRC_PROT_PRIV);
        ROM_uDMAChannelTransferSet(UDMA_CH6_SHAMD50COUT | UDMA_PRI_SELECT,
                                   UDMA_MODE_BASIC,
                                   (void *)(SHAMD5_BASE + SHAMD5_O_IDIGEST_A),
                                   (void *)pui32HMACResult, 5);
        ROM_uDMAChannelEnable(UDMA_CH6_SHAMD50COUT);
        UARTprintf("Context out DMA request enabled.\n");

        //
        // Enable DMA in the SHA/MD5 module.
        //
        ROM_SHAMD5DMAEnable(SHAMD5_BASE);

        //
        // Write the length.
        //
        ROM_SHAMD5HashLengthSet(SHAMD5_BASE, ui32DataLength);

        if(ui32DataLength != 0)
        {
            //
            // Wait for the DMA done interrupt.
            //
            while(!g_bDataInDMADoneFlag)
            {
            }
        }

        //
        // Wait for the next DMA done interrupt.
        //
        while(!g_bContextOutDMADoneFlag)
        {
        }

        //
        // Disable DMA requests.
        //
        ROM_SHAMD5DMADisable(SHAMD5_BASE);
    }

    //
    // Perform hash computation by copying the data with the CPU.
    //
    else
    {
        //
        // Perform the hashing operation
        //
        ROM_SHAMD5HMACProcess(SHAMD5_BASE, pui32Data, ui32DataLength,
                              pui32HMACResult);
    }
}

//*****************************************************************************
//
// Initializes the CCM and SHA/MD5 modules.
//
//*****************************************************************************
bool
SHAMD5Init(void)
{
    uint32_t ui32Loop;

    //
    // Check that the CCM peripheral is present.
    //
    if(!ROM_SysCtlPeripheralPresent(SYSCTL_PERIPH_CCM0))
    {
        UARTprintf(" No CCM peripheral found!\n");

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
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

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
// This example generates HMACs from a random block of data and empty block.
//
//*****************************************************************************
int
main(void)
{
    uint32_t pui32HMACPPKey[16], pui32HMACResult[5], ui32Errors, ui32Vector;
    uint32_t ui32Idx, ui32SysClock;
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
    FrameDraw(&sContext, "sha1-hmac");

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
    // Enable SHA interrupts.
    //
    ROM_IntEnable(INT_SHA0);

    //
    // Enable debug output on UART0 and print a welcome message.
    //
    ConfigureUART();
    UARTprintf("Starting SHA1 HMAC encryption demo.\n");
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
    // Initialize the CCM and SHAMD5 modules.
    //
    if(!SHAMD5Init())
    {
        UARTprintf("Initialization of the SHA module failed.\n");
        ui32Errors |= 0x00000001;
    }

    //
    // Run tests without uDMA.
    //
    for(ui32Vector = 0; ui32Vector < 3; ui32Vector++)
    {
        UARTprintf("Running test #%d without uDMA\n", ui32Vector);

        //
        // Generate the HMAC.
        //
        SHA1HMACGenerate(g_pui32RandomData,
                         g_psSHA1TestVectors[ui32Vector].ui32DataLength,
                         g_pui32SHA1HMACKey, pui32HMACResult, false, false);

        //
        // Check the result.
        //
        for(ui32Idx = 0; ui32Idx < 5; ui32Idx++)
        {
            if(pui32HMACResult[ui32Idx] !=
               g_psSHA1TestVectors[ui32Vector].pui32HMACResult[ui32Idx])
            {
                UARTprintf("HMAC result mismatch - Exp: 0x%x, Act: 0x%x\n",
                           g_psSHA1TestVectors[ui32Vector].
                           pui32HMACResult[ui32Idx], pui32HMACResult[0]);
                ui32Errors |= ((ui32Idx << 16) | 0x2);
            }
        }
    }

    //
    // Run tests with uDMA.
    //
    for(ui32Vector = 0; ui32Vector < 3; ui32Vector++)
    {
        UARTprintf("Running test #%d with uDMA\n", ui32Vector);

        //
        // Generate the HMAC.
        //
        SHA1HMACGenerate(g_pui32RandomData,
                         g_psSHA1TestVectors[ui32Vector].ui32DataLength,
                         g_pui32SHA1HMACKey, pui32HMACResult, true, false);

        //
        // Check the result.
        //
        for(ui32Idx = 0; ui32Idx < 5; ui32Idx++)
        {
            if(pui32HMACResult[ui32Idx] !=
               g_psSHA1TestVectors[ui32Vector].pui32HMACResult[ui32Idx])
            {
                UARTprintf("HMAC result mismatch - Exp: 0x%x, Act: 0x%x\n",
                           g_psSHA1TestVectors[ui32Vector].
                           pui32HMACResult[ui32Idx], pui32HMACResult[0]);
                ui32Errors |= ((ui32Idx << 16) | 0x3);
            }
        }
    }

    //
    // Preprocess the HMAC key.
    //
    UARTprintf("Preprocessing HMAC key with SHA1...\n");
    ROM_SHAMD5Reset(SHAMD5_BASE);
    ROM_SHAMD5ConfigSet(SHAMD5_BASE, SHAMD5_ALGO_HMAC_SHA1);
    ROM_SHAMD5HMACPPKeyGenerate(SHAMD5_BASE, g_pui32SHA1HMACKey,
                                pui32HMACPPKey);

    //
    // Run tests with a preprocessed key and without uDMA.
    //
    for(ui32Vector = 0; ui32Vector < 3; ui32Vector++)
    {
        UARTprintf("Running test #%d without uDMA\n", ui32Vector);

        //
        // Generate the HMAC.
        //
        SHA1HMACGenerate(g_pui32RandomData,
                         g_psSHA1TestVectors[ui32Vector].ui32DataLength,
                         pui32HMACPPKey, pui32HMACResult, false, true);

        //
        // Check the result.
        //
        for(ui32Idx = 0; ui32Idx < 5; ui32Idx++)
        {
            if(pui32HMACResult[ui32Idx] !=
               g_psSHA1TestVectors[ui32Vector].pui32HMACResult[ui32Idx])
            {
                UARTprintf("HMAC result mismatch - Exp: 0x%x, Act: 0x%x\n",
                           g_psSHA1TestVectors[ui32Vector].
                           pui32HMACResult[ui32Idx], pui32HMACResult[0]);
                ui32Errors |= ((ui32Idx << 16) | 0x2);
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
