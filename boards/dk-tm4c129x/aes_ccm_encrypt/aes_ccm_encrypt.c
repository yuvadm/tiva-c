//*****************************************************************************
//
// aes_ccm_encrypt.c - Simple AES128 CCM encryption demo.
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
//! <h1>AES128 and AES256 CCM Encryption Demo (aes_ccm_encrypt)</h1>
//!
//! Simple demo showing an encryption operation using the AES128 and AES256
//! modules in CCM mode.  A set of test cases are encrypted.
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
// Test cases from the NIST SP 800-38C document and proposals for IEEE P1619.1
// Test Vectors
//
//*****************************************************************************
typedef struct AESTestVectorStruct
{
    uint32_t ui32KeySize;
    uint32_t pui32Key[8];
    uint32_t ui32NonceLength;
    uint32_t pui32Nonce[4];
    uint32_t ui32PayloadLength;
    uint32_t pui32Payload[16];
    uint32_t ui32AuthDataLength;
    uint32_t pui32AuthData[16];
    uint32_t pui32CipherText[16];
    uint32_t ui32TagLength;
    uint32_t pui32Tag[4];
}

tAESCCMTestVector;

tAESCCMTestVector g_psAESCCMTestVectors[] = 
{
    //
    // Test Case #1
    // The data in these test cases have been modified to be in big endian 
    // format as required by the AES module.  This was done to simplify writes
    // and comparisons. 
    // Also, The test vector is formatted in the document in a way that the 
    // ciphertext is the concatenation of the ciphertext and the MAC.  they
    // have been separated to match the operation of the AES module.
    //
    {
        AES_CFG_KEY_SIZE_128BIT,
        { 0x43424140, 0x47464544, 0x4b4a4948, 0x4f4e4d4c }, // Key
        7,                                                  // Nonce Length
        { 0x13121110, 0x00161514, 0x00000000, 0x00000000 }, // Nonce
        4,                                                  // Payload Length
        { 0x23222120, 0x00000000, 0x00000000, 0x00000000 }, // Payload
        8,                                                  // Auth Data Length
        { 0x03020100, 0x07060504, 0x00000000, 0x00000000 }, // Auth Data
        { 0x5b016271, 0x00000000, 0x00000000, 0x00000000 }, // CipherText
        4,                                                  // Tag Length
        { 0x5d25ac4d, 0x00000000, 0x00000000, 0x00000000 }  // Tag
    },
    
    //
    // Test Case #2
    //
    {
        AES_CFG_KEY_SIZE_128BIT,
        { 0x43424140, 0x47464544, 0x4b4a4948, 0x4f4e4d4c }, // Key
        8,                                                  // Nonce Length
        { 0x13121110, 0x17161514, 0x00000000, 0x00000000 }, // Nonce
        16,                                                 // Payload Length
        { 0x23222120, 0x27262524, 0x2b2a2928, 0x2f2e2d2c }, // Payload
        16,                                                 // Auth Data Length
        { 0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c }, // Auth Data
        { 0xe0f0a1d2, 0x625fea51, 0x92771a08, 0x3d593d07 }, // CipherText
        6,                                                  // Tag Length
        { 0xbf4fc61f, 0x0000cdac, 0x00000000, 0x00000000 }  // Tag
    },

    //
    // Test Case #3
    //
    {
        AES_CFG_KEY_SIZE_128BIT,
        { 0x43424140, 0x47464544, 0x4b4a4948, 0x4f4e4d4c }, // Key
        12,                                                 // Nonce Length
        { 0x13121110, 0x17161514, 0x1b1a1918, 0x00000000 }, // Nonce
        24,                                                 // Payload Length
        { 0x23222120, 0x27262524, 0x2b2a2928, 0x2f2e2d2c,   // Payload
          0x33323130, 0x37363534, 0x00000000, 0x00000000 },
        20,                                                 // Auth Data Length
        { 0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,   // Auth Data
          0x13121110, 0x00000000, 0x00000000, 0x00000000 },
        { 0xa901b2e3, 0x7a1ab7f5, 0xecea1c9b, 0x0be797cd,   // CipherText
          0xd9aa7661, 0xa58a42a4, 0x00000000, 0x00000000 },
        8,                                                  // Tag Length
        { 0xfb924348, 0x5199b0c1, 0x00000000, 0x00000000 }  // Tag
    },

    //
    // The following test cases use 256bit key, and they are taken from
    // proposals for IEEE P1619.1 Test Vectors.
    // 
    // Test Case #4
    //
    {
        AES_CFG_KEY_SIZE_256BIT,
        { 0xb21576fb, 0x1d89803d, 0x0b9870d4, 0xc88495c7, // Key
          0xce64fbb2, 0x4d8f9760, 0x5ae4fc17, 0xb730e849 },
        12,                                                 // Nonce Length
        { 0x63a3d1db, 0xb4b72460, 0x6f7dda02, 0x00000000 }, // Nonce
        16,                                                 // Payload Length
        { 0x8e3445a8, 0xf1b5c5c8, 0x760ef526, 0x1e1bfdfe,   // Payload
          0x00000000, 0x00000000, 0x00000000, 0x00000000 },
        0,                                                 // Auth Data Length
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000,   // Auth Data
          0x00000000, 0x00000000, 0x00000000, 0x00000000 },
        { 0x611288cc, 0x72faa7c6, 0x39176ab9, 0x7f276b17,   // CipherText
          0x00000000, 0x00000000, 0x00000000, 0x00000000 },
        16,                                                 // Tag Length
        { 0x14e17234, 0xbe0c2c5f, 0x06496314, 0x23e4f02c }  // Tag
    },

    // 
    // Test Case #5
    //
    {
        AES_CFG_KEY_SIZE_256BIT,
        { 0x43424140, 0x47464544, 0x4b4a4948, 0x4f4e4d4c, // Key
          0x53525150, 0x57565554, 0x5b5a5958, 0x5f5e5d5c },
        12,                                                 // Nonce Length
        { 0x13121110, 0x17161514, 0x1b1a1918, 0x00000000 }, // Nonce
        24,                                                 // Payload Length
        { 0x23222120, 0x27262524, 0x2b2a2928, 0x2f2e2d2c,   // Payload
          0x33323130, 0x37363534, 0x00000000, 0x00000000 },
        20,                                                 // Auth Data Length
        { 0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,   // Auth Data
          0x13121110, 0x00000000, 0x00000000, 0x00000000 },
        { 0xae83f804, 0x3007bdb3, 0xb60bf5ea, 0x21a24fde,   // CipherText
          0xe4e43420, 0xe5750e1b, 0x00000000, 0x00000000 },
        16,                                                 // Tag Length
        { 0x3a3fba9b, 0x39327f10, 0x299063bd, 0x7103f823 }  // Tag
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
// Perform an CCM encryption operation.
// 
//*****************************************************************************
bool
AESCCMEncrypt(uint32_t ui32Keysize, uint32_t *pui32Key,
              uint32_t *pui32Src, uint32_t *pui32Dst,
              uint32_t ui32DataLength, uint32_t *pui32Nonce, 
              uint32_t ui32NonceLength, uint32_t *pui32AuthData, 
              uint32_t ui32AuthDataLength, uint32_t *pui32Tag, 
              uint32_t ui32TagLength, bool bUseDMA)
{
    uint32_t pui32IV[4], ui32Idx;
    uint32_t ui32M, ui32L;
    uint8_t *pui8Nonce, *pui8IV;

    //
    // Determine the value of M.  It is determined using 
    // the tag length.
    //
    if(ui32TagLength == 4)
    {
        ui32M = AES_CFG_CCM_M_4;
    }
    else if(ui32TagLength == 6)
    {
        ui32M = AES_CFG_CCM_M_6;
    }
    else if(ui32TagLength == 8)
    {
        ui32M = AES_CFG_CCM_M_8;
    }
    else if(ui32TagLength == 10)
    {
        ui32M = AES_CFG_CCM_M_10;
    }
    else if(ui32TagLength == 12)
    {
        ui32M = AES_CFG_CCM_M_12;
    }
    else if(ui32TagLength == 14)
    {
        ui32M = AES_CFG_CCM_M_14;
    }
    else if(ui32TagLength == 16)
    {
        ui32M = AES_CFG_CCM_M_16;
    }
    else
    {
        UARTprintf("Unexpected tag length.\n");
        return(false);
    }

    //
    // Determine the value of L. This is determined by using
    // the value of q from the NIST document:  n + q = 15
    //
    if(ui32NonceLength == 7)
    {   
        ui32L = AES_CFG_CCM_L_8;
    }
    else if(ui32NonceLength == 8)
    {   
        ui32L = AES_CFG_CCM_L_7;
    }
    else if(ui32NonceLength == 9)
    {   
        ui32L = AES_CFG_CCM_L_6;
    }
    else if(ui32NonceLength == 10)
    {   
        ui32L = AES_CFG_CCM_L_5;
    }
    else if(ui32NonceLength == 11)
    {
        ui32L = AES_CFG_CCM_L_4;
    }
    else if(ui32NonceLength == 12)
    {
        ui32L = AES_CFG_CCM_L_3;
    }
    else if(ui32NonceLength == 13)
    {
        ui32L = AES_CFG_CCM_L_2;
    }
    else if(ui32NonceLength == 14)
    {
        ui32L = AES_CFG_CCM_L_1;
    }
    else
    {
        UARTprintf("Unexpected nonce length.\n");
        return(false);
    }

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
    ROM_AESConfigSet(AES_BASE, (ui32Keysize | AES_CFG_DIR_ENCRYPT |
                                AES_CFG_CTR_WIDTH_128 |
                                AES_CFG_MODE_CCM | ui32L | ui32M));

    //
    // Determine the value to be written in the initial value registers.  It is 
    // the concatenation of 5 bits of zero, 3 bits of L, nonce, and the counter
    // value.  First, clear the contents of the IV.
    //
    for(ui32Idx = 0; ui32Idx < 4; ui32Idx++)
    {
        pui32IV[ui32Idx] = 0;
    }

    //
    // Now find the binary value of L.
    //
    if(ui32L == AES_CFG_CCM_L_8)
    {
        pui32IV[0] = 0x7;
    }
    else if(ui32L == AES_CFG_CCM_L_7)
    {
        pui32IV[0] = 0x6;
    }
    else if(ui32L == AES_CFG_CCM_L_6)
    {
        pui32IV[0] = 0x5;
    }
    else if(ui32L == AES_CFG_CCM_L_5)
    {
        pui32IV[0] = 0x4;
    }
    else if(ui32L == AES_CFG_CCM_L_4)
    {
        pui32IV[0] = 0x3;
    }
    else if(ui32L == AES_CFG_CCM_L_3)
    {
        pui32IV[0] = 0x2;
    }
    else if(ui32L == AES_CFG_CCM_L_2)
    {
        pui32IV[0] = 0x1;
    }

    //
    // Finally copy the contents of the nonce into the IV.  Convert
    // the pointers to simplify the copying.
    //
    pui8Nonce = (uint8_t *)pui32Nonce; 
    pui8IV = (uint8_t *)pui32IV;
    for(ui32Idx = 0; ui32Idx < ui32NonceLength; ui32Idx++)
    {
        pui8IV[ui32Idx + 1] = pui8Nonce[ui32Idx];
    }

    //
    // Write the initial value.
    //
    ROM_AESIVSet(AES_BASE, pui32IV);

    //
    // Write the key.
    //
    ROM_AESKey1Set(AES_BASE, pui32Key, ui32Keysize);
 
    //
    // Depending on the argument, perform the encryption
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
        // Setup the DMA module to copy auth data in.
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

        if(ui32AuthDataLength)
        {
            ROM_uDMAChannelTransferSet(UDMA_CH14_AES0DIN | UDMA_PRI_SELECT,
                                       UDMA_MODE_BASIC, (void *)pui32AuthData,
                                       (void *)(AES_BASE + AES_O_DATA_IN_0),
                                       LengthRoundUp(ui32AuthDataLength) / 4);
        }
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
                                   (void *)pui32Dst,
                                   LengthRoundUp(ui32DataLength) / 4);
        UARTprintf("Data out DMA request enabled.\n");

        //
        // Write the length registers.
        //
        ROM_AESLengthSet(AES_BASE, (uint64_t)ui32DataLength);

        //
        // Write the auth length registers to start the process.
        //
        ROM_AESAuthLengthSet(AES_BASE, ui32AuthDataLength);
        
        //
        // Enable the DMA channels to start the transfers.  This must be done after
        // writing the length to prevent data from copying before the context is 
        // truly ready.
        // 
        ROM_uDMAChannelEnable(UDMA_CH14_AES0DIN);
        ROM_uDMAChannelEnable(UDMA_CH15_AES0DOUT);

        //
        // Enable DMA requests.
        //
        ROM_AESDMAEnable(AES_BASE, AES_DMA_DATA_IN | AES_DMA_DATA_OUT);

        //
        // Wait for the data in DMA done interrupt.
        //
        while(!g_bDataInDMADoneIntFlag)
        {
        }

        //
        // Setup the uDMA to copy the plaintext data.
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
                                   UDMA_MODE_BASIC, (void *)pui32Src,
                                   (void *)(AES_BASE + AES_O_DATA_IN_0),
                                   LengthRoundUp(ui32DataLength) / 4);
        ROM_uDMAChannelEnable(UDMA_CH14_AES0DIN);
        UARTprintf("Data in DMA request enabled.\n");

        //
        // Wait for the data out DMA done interrupt.
        //
        while(!g_bDataOutDMADoneIntFlag)
        {
        }

        //
        // Read the tag out.
        //
        ROM_AESTagRead(AES_BASE, pui32Tag);
    }
    else
    {
        //
        // Perform the encryption.
        //
        ROM_AESDataProcessAuth(AES_BASE, pui32Src, pui32Dst, ui32DataLength,
                               pui32AuthData, ui32AuthDataLength, pui32Tag);
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
// This example encrypts a block of payload using AES128 in CCM mode.  It
// does the encryption first without uDMA and then with uDMA.  The results
// are checked after each operation.
//
//*****************************************************************************
int
main(void)
{
    uint32_t pui32CipherText[16], pui32Tag[4], ui32Errors, ui32Idx;
    uint32_t ui32PayloadLength, ui32TagLength;
    uint32_t ui32NonceLength, ui32AuthDataLength;
    uint32_t *pui32Nonce, *pui32AuthData, ui32SysClock;
    uint32_t *pui32Key, *pui32Payload, *pui32ExpCipherText;
    uint32_t ui32Keysize;
    uint8_t ui8Vector;
    uint8_t *pui8ExpTag, *pui8Tag;
    tContext sContext;
    
    //
    // Run from the PLL at 120 MHz.
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN |
                                           SYSCTL_USE_PLL |
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
    FrameDraw(&sContext, "aes-ccm-encrypt");
    
    //
    // Show some instructions on the display
    //
    GrContextFontSet(&sContext, g_psFontCm20);
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
    for(ui32Idx = 0; ui32Idx < 4; ui32Idx++)
    {
        pui32Tag[ui32Idx] = 0;
    }
    pui8Tag = (uint8_t *)pui32Tag;

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
    UARTprintf("Starting AES CCM encryption demo.\n");
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
    // Loop through all the given vectors.
    //
    for(ui8Vector = 0;
        (ui8Vector <
         (sizeof(g_psAESCCMTestVectors) / sizeof(g_psAESCCMTestVectors[0]))) &&
        (ui32Errors == 0);
        ui8Vector++)
    {
        UARTprintf("Starting vector #%d\n", ui8Vector);

        //
        // Get the current vector's data members.
        //
        ui32Keysize = g_psAESCCMTestVectors[ui8Vector].ui32KeySize;
        pui32Key = g_psAESCCMTestVectors[ui8Vector].pui32Key;
        pui32Payload = g_psAESCCMTestVectors[ui8Vector].pui32Payload;
        ui32PayloadLength = 
            g_psAESCCMTestVectors[ui8Vector].ui32PayloadLength;
        pui32AuthData = g_psAESCCMTestVectors[ui8Vector].pui32AuthData;
        ui32AuthDataLength = 
            g_psAESCCMTestVectors[ui8Vector].ui32AuthDataLength;
        pui32ExpCipherText = 
            g_psAESCCMTestVectors[ui8Vector].pui32CipherText;
        pui8ExpTag = (uint8_t *)g_psAESCCMTestVectors[ui8Vector].pui32Tag;
        ui32TagLength = g_psAESCCMTestVectors[ui8Vector].ui32TagLength;
        pui32Nonce = g_psAESCCMTestVectors[ui8Vector].pui32Nonce;
        ui32NonceLength =
            g_psAESCCMTestVectors[ui8Vector].ui32NonceLength;

        //
        // Perform the encryption without uDMA.
        //
        UARTprintf("Performing encryption without uDMA.\n");
        AESCCMEncrypt(ui32Keysize, pui32Key, pui32Payload, pui32CipherText,
                      ui32PayloadLength, pui32Nonce, ui32NonceLength,
                      pui32AuthData, ui32AuthDataLength, pui32Tag,
                      ui32TagLength, false);
            
        //
        // Check the result.
        //
        for(ui32Idx = 0; ui32Idx < (ui32PayloadLength / 4); ui32Idx++)
        {
            if(pui32CipherText[ui32Idx] != pui32ExpCipherText[ui32Idx])
            {
                UARTprintf("Ciphertext mismatch on word %d. Exp: 0x%x, Act: "
                           "0x%x\n", ui32Idx, pui32ExpCipherText[ui32Idx],
                           pui32CipherText[ui32Idx]);
                ui32Errors |= (ui32Idx << 16) | 0x00000002;
            }
        }
        for(ui32Idx = 0; ui32Idx < ui32TagLength; ui32Idx++)
        {
            if(pui8Tag[ui32Idx] != pui8ExpTag[ui32Idx])
            {
                UARTprintf("Tag mismatch on byte %d. Exp: 0x%x, Act: "
                           "0x%x\n", ui32Idx, pui8ExpTag[ui32Idx],
                           pui8Tag[ui32Idx]);
                ui32Errors |= (ui32Idx << 16) | 0x00000004;
            }
        }

        //
        // Clear the array containing the ciphertext.
        //
        for(ui32Idx = 0; ui32Idx < 16; ui32Idx++)
        {
            pui32CipherText[ui32Idx] = 0;
        }
        for(ui32Idx = 0; ui32Idx < 4; ui32Idx++)
        {
            pui32Tag[ui32Idx] = 0;
        }

        //
        // Perform the encryption with uDMA.
        //
        UARTprintf("Performing encryption with uDMA.\n");
        AESCCMEncrypt(ui32Keysize, pui32Key, pui32Payload, pui32CipherText,
                      ui32PayloadLength, pui32Nonce, ui32NonceLength,
                      pui32AuthData, ui32AuthDataLength, pui32Tag,
                      ui32TagLength, true);
        
        //
        // Check the result.
        //
        for(ui32Idx = 0; ui32Idx < (ui32PayloadLength / 4); ui32Idx++)
        {
            if(pui32CipherText[ui32Idx] != pui32ExpCipherText[ui32Idx])
            {
                UARTprintf("Ciphertext mismatch on word %d. Exp: 0x%x, Act: "
                           "0x%x\n", ui32Idx, pui32ExpCipherText[ui32Idx],
                           pui32CipherText[ui32Idx]);
                ui32Errors |= (ui32Idx << 16) | 0x00000002;
            }
        }
        for(ui32Idx = 0; ui32Idx < ui32TagLength; ui32Idx++)
        {
            if(pui8Tag[ui32Idx] != pui8ExpTag[ui32Idx])
            {
                UARTprintf("Tag mismatch on byte %d. Exp: 0x%x, Act: "
                           "0x%x\n", ui32Idx, pui8ExpTag[ui32Idx],
                           pui8Tag[ui32Idx]);
                ui32Errors |= (ui32Idx << 16) | 0x00000004;
            }
        }
        
        //
        // Clear the array containing the ciphertext.
        //
        for(ui32Idx = 0; ui32Idx < 16; ui32Idx++)
        {
            pui32CipherText[ui32Idx] = 0;
        }
        for(ui32Idx = 0; ui32Idx < 4; ui32Idx++)
        {
            pui32Tag[ui32Idx] = 0;
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
