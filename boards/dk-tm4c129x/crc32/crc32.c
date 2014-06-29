//*****************************************************************************
//
// crc32.c - Simple CRC-32 demo
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
#include "inc/hw_ccm.h"
#include "inc/hw_memmap.h"
#include "driverlib/crc.h"
#include "driverlib/debug.h"
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
//! <h1>CRC-32 Demo (crc32)</h1>
//!
//! Simple demo showing an CRC-32 operation using the CCM module.
//!
//! Please note that the use of uDMA is not required for the operation of the
//! CRC module.  It is for demostration purposes only.
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
// Random data for use with the test vectors.
//
//*****************************************************************************
uint32_t g_pui32RandomData[] =
{
    0x8a5f1b22, 0xcb935d29, 0xcc1ac092, 0x5dad8c9e, 0x6a83b39f, 0x8607dc60,
    0xda0ba4d2, 0xf49b0fa2, 0xaf35d524, 0xffa8001d, 0xbcc931e8, 0x4a2c99ef,
    0x7fa297ab, 0xab943bae, 0x07c61cc4, 0x47c8627d
};

//*****************************************************************************
//
// Test Vector Structure
//
//*****************************************************************************
typedef struct CRCTestVectorStruct
{
    uint32_t ui32TestNumber;
    uint32_t ui32Seed;
    uint32_t ui32Result;
}
tCRCTestVector;

//*****************************************************************************
//
// CRC-32 Test Vectors
//
//*****************************************************************************
tCRCTestVector g_psCRC4C11DB7TestVectors[3] =
{
    {
        0,
        0x00000000,
        0xbcc90d0d
    },
    {
        1,
        0xffffffff,
        0x2ff0435c
    },
    {
        2,
        0xa5a5a5a5,
        0x75fd6f5c
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
// Process data to produce a CRC-32 checksum.
//
//*****************************************************************************
uint32_t
CRC32DataProcess(uint32_t *pui32Data, uint32_t ui32Length, uint32_t ui32Seed,
                 bool bUseDMA)
{
    uint32_t ui32Result;

    //
    // Perform a soft reset.
    //
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_CCM0);
    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_CCM0))
    {
    }

    //
    // Configure the CRC engine.
    //
    ROM_CRCConfigSet(CCM0_BASE, (CRC_CFG_INIT_SEED | CRC_CFG_TYPE_P4C11DB7 |
                                 CRC_CFG_SIZE_32BIT));

    //
    // Write the seed.
    //
    ROM_CRCSeedSet(CCM0_BASE, ui32Seed);

    //
    // Generate CRC using uDMA to copy data.
    //
    if(bUseDMA)
    {
        //
        // Setup the DMA module to copy data in.
        //
        ROM_uDMAChannelAssign(UDMA_CH30_SW);
        ROM_uDMAChannelAttributeDisable(UDMA_CH30_SW,
                                        UDMA_ATTR_ALTSELECT |
                                        UDMA_ATTR_USEBURST |
                                        UDMA_ATTR_HIGH_PRIORITY |
                                        UDMA_ATTR_REQMASK);
        ROM_uDMAChannelControlSet(UDMA_CH30_SW | UDMA_PRI_SELECT,
                                  UDMA_SIZE_32 | UDMA_SRC_INC_32 |
                                  UDMA_DST_INC_NONE | UDMA_ARB_1 |
                                  UDMA_DST_PROT_PRIV);
        ROM_uDMAChannelTransferSet(UDMA_CH30_SW | UDMA_PRI_SELECT,
                                   UDMA_MODE_AUTO, (void *)g_pui32RandomData,
                                   (void *)(CCM0_BASE + CCM_O_CRCDIN),
                                   ui32Length);
        ROM_uDMAChannelEnable(UDMA_CH30_SW);
        UARTprintf(" Data in DMA request enabled.\n");

        //
        // Start the uDMA.
        //
        ROM_uDMAChannelRequest(UDMA_CH30_SW | UDMA_PRI_SELECT);

        //
        // Wait for the transfer to finish.
        //
        while(ROM_uDMAChannelIsEnabled(UDMA_CH30_SW))
        {
        }

        //
        // Read the result.
        //
        ui32Result = ROM_CRCResultRead(CCM0_BASE, false);
    }

    //
    // Generate CRC using CPU to copy data.
    //
    else
    {
        ui32Result = ROM_CRCDataProcess(CCM0_BASE, g_pui32RandomData,
                                        ui32Length, false);
    }

    return(ui32Result);
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
// Initialize the CRC and CCM modules.
//
//*****************************************************************************
bool
CRCInit(void)
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
// This example performs a CRC-32 operation on an array of data using a
// number of starting seeds.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32Result, ui32Errors, ui32Idx, ui32SysClock;
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
    FrameDraw(&sContext, "crc32-demo");

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
    // Enable debug output on UART0 and print a welcome message.
    //
    ConfigureUART();
    UARTprintf("Starting CRC-32 demo.\n");
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
    // Initialize the CRC and CCM modules.
    //
    if(!CRCInit())
    {
        UARTprintf("Initialization of the CRC  module failed.\n");
        ui32Errors |= 0x00000001;
    }

    //
    // Run through the test vectors.
    //
    for(ui32Idx = 0;
        ui32Idx < (sizeof(g_psCRC4C11DB7TestVectors) / sizeof(tCRCTestVector));
        ui32Idx++)
    {
        UARTprintf("Starting vector #%d\n", ui32Idx);

        //
        // Generate the checksum without uDMA.
        //
        UARTprintf("Generating CRC-32 checksum without uDMA.\n");
        ui32Result =
            CRC32DataProcess(g_pui32RandomData,
                             (sizeof(g_pui32RandomData) / 4),
                             g_psCRC4C11DB7TestVectors[ui32Idx].ui32Seed,
                             false);

        if(ui32Result != g_psCRC4C11DB7TestVectors[ui32Idx].ui32Result)
        {
            UARTprintf("CRC result mismatch - Exp: 0x%08x, Act: 0x%08x\n",
                       g_psCRC4C11DB7TestVectors[ui32Idx].ui32Result,
                       ui32Result);
        }

        //
        // Generate the checksum with uDMA.
        //
        UARTprintf("Generating CRC-32 checksum with uDMA.\n");
        ui32Result =
            CRC32DataProcess(g_pui32RandomData,
                             (sizeof(g_pui32RandomData) / 4),
                             g_psCRC4C11DB7TestVectors[ui32Idx].ui32Seed,
                             true);

        if(ui32Result != g_psCRC4C11DB7TestVectors[ui32Idx].ui32Result)
        {
            UARTprintf("CRC result mismatch - Exp: 0x%08x, Act: 0x%08x\n",
                       g_psCRC4C11DB7TestVectors[ui32Idx].ui32Result,
                       ui32Result);
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
