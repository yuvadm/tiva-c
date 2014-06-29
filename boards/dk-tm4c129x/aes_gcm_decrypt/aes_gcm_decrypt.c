//*****************************************************************************
//
// aes_gcm_decrypt.c - Simple AES GCM decryption demo.
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
#include "driverlib/interrupt.h"
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
//! <h1>AES128 and AES256 GCM Decryption Demo (aes_gcm_decrypt)</h1>
//!
//! Simple demo showing authenticated decryption operations using the AES
//! module in GCM mode.  The test vectors are from the gcm_revised_spec.pdf
//! document.
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
// Structure for NIST AES GCM tests
//
//*****************************************************************************
typedef struct AESTestVectorStruct
{
    uint32_t ui32KeySize;
    uint32_t pui32Key[8];
    uint32_t ui32IVLength;
    uint32_t pui32IV[64];
    uint32_t ui32DataLength;
    uint32_t pui32PlainText[64];
    uint32_t ui32AuthDataLength;
    uint32_t pui32AuthData[64];
    uint32_t pui32CipherText[64];
    uint32_t pui32Tag[4];
}
tAESGCMTestVector;

//*****************************************************************************
//
// Test Cases from NIST GCM Revised Spec.
//
//*****************************************************************************
tAESGCMTestVector g_psAESGCMTestVectors[] =
{
    //
    // Test Case #1
    // This is a special case that cannot use the GCM mode because the
    // data and AAD lengths are both zero.  The work around is to perform
    // an ECB encryption on Y0.
    //
    {
        AES_CFG_KEY_SIZE_128BIT,
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
        12,
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
        0,
        { 0 },
        0,
        { 0 },
        { 0 },
        { 0xcefce258, 0x61307efa, 0x571d7f36, 0x5a45e7a4 }
    },

    //
    // Test Case #2
    // This is the first test in which the AAD length is zero.
    //
    {
        AES_CFG_KEY_SIZE_128BIT,
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
        12,
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
        16,
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
        0,
        { 0 },
        { 0xceda8803, 0x92a3b660, 0xb9c228f3, 0x78feb271 },
        { 0xd4476eab, 0xbd13ec2c, 0xb2673af5, 0xdfbd5712 }
    },

    //
    // Test Case #3
    //
    {
        AES_CFG_KEY_SIZE_128BIT,
        { 0x92e9fffe, 0x1c736586, 0x948f6a6d, 0x08833067 },
        12,
        { 0xbebafeca, 0xaddbcefa, 0x88f8cade, 0x00000000 },
        64,
        { 0x253231d9, 0xe50684f8, 0xc50959a5, 0x9a26f5af,
          0x53a9a786, 0xdaf73415, 0x3d304c2e, 0x728a318a,
          0x950c3c1c, 0x53096895, 0x240ecf2f, 0x25b5a649,
          0xf5ed6ab1, 0x57e60daa, 0x397b63ba, 0x55d2af1a },
        0,
        { 0 },
        { 0xc21e8342, 0x24747721, 0xb721724b, 0x9cd4d084,
          0x2f21aae3, 0xe0a4022c, 0x237ec135, 0x2ea1ac29,
          0xb214d521, 0x1c936654, 0x5a6a8f7d, 0x05aa84ac,
          0x390ba31b, 0x97ac0a6a, 0x91e0583d, 0x85593f47 },
        { 0xf32a5c4d, 0xa664cd27, 0xbd5af32c, 0xb4faa62b }
    },

    //
    // Test Case #4
    // When the data lengths do not align with the block
    // boundary, we need to pad with zeros to ensure unknown
    // data is not copied with uDMA.
    //
    {
        AES_CFG_KEY_SIZE_128BIT,
        { 0x92e9fffe, 0x1c736586, 0x948f6a6d, 0x08833067 },
        12,
        { 0xbebafeca, 0xaddbcefa, 0x88f8cade, 0x00000000 },
        60,
        { 0x253231d9, 0xe50684f8, 0xc50959a5, 0x9a26f5af,
          0x53a9a786, 0xdaf73415, 0x3d304c2e, 0x728a318a,
          0x950c3c1c, 0x53096895, 0x240ecf2f, 0x25b5a649,
          0xf5ed6ab1, 0x57e60daa, 0x397b63ba, 0x00000000 },
        20,
        { 0xcefaedfe, 0xefbeadde, 0xcefaedfe, 0xefbeadde,
          0xd2daadab, 0x00000000, 0x00000000, 0x00000000 },
        { 0xc21e8342, 0x24747721, 0xb721724b, 0x9cd4d084,
          0x2f21aae3, 0xe0a4022c, 0x237ec135, 0x2ea1ac29,
          0xb214d521, 0x1c936654, 0x5a6a8f7d, 0x05aa84ac,
          0x390ba31b, 0x97ac0a6a, 0x91e0583d, 0x00000000 },
        { 0xbc4fc95b, 0xdba52132, 0x5ae9fa94, 0x471a12e7 }
    },

    //
    // Test Case #5
    // This is the first case in which IV is less than
    // 96 bits.
    //
    {
        AES_CFG_KEY_SIZE_128BIT,
        { 0x92e9fffe, 0x1c736586, 0x948f6a6d, 0x08833067 },
        8,
        { 0xbebafeca, 0xaddbcefa, 0x00000000, 0x00000000 },
        60,
        { 0x253231d9, 0xe50684f8, 0xc50959a5, 0x9a26f5af,
          0x53a9a786, 0xdaf73415, 0x3d304c2e, 0x728a318a,
          0x950c3c1c, 0x53096895, 0x240ecf2f, 0x25b5a649,
          0xf5ed6ab1, 0x57e60daa, 0x397b63ba, 0x00000000 },
        20,
        { 0xcefaedfe, 0xefbeadde, 0xcefaedfe, 0xefbeadde,
          0xd2daadab, 0x00000000, 0x00000000, 0x00000000 },
        { 0x4c3b3561, 0x4a930628, 0x1ff57f77, 0x55472aa2,
          0x712a9b69, 0xf8c6cd4f, 0xf9e56637, 0x23746c7b,
          0x00698073, 0xb2249fe4, 0x4475092b, 0x426b89d4,
          0xe1b58949, 0x070faceb, 0x98453fc2, 0x00000000 },
        { 0xe7d21236, 0x85073b9e, 0x4ae11b56, 0xcbfca2ac }
    },

    //
    // Test Case #6
    // This is the first case in which IV is more than
    // 96 bits.
    //
    {
        AES_CFG_KEY_SIZE_128BIT,
        { 0x92e9fffe, 0x1c736586, 0x948f6a6d, 0x08833067 },
        60,
        { 0x5d221393, 0xe50684f8, 0x5a9c9055, 0xaa6952ff,
          0x38957a6a, 0xa17d4f53, 0xd203c3e4, 0x28a718a3,
          0x51c9c0c3, 0x39958056, 0x42e2f0fc, 0x54526b9a,
          0xf5dbae16, 0x576adea0, 0x9bb337a6, 0x00000000 },
        60,
        { 0x253231d9, 0xe50684f8, 0xc50959a5, 0x9a26f5af,
          0x53a9a786, 0xdaf73415, 0x3d304c2e, 0x728a318a,
          0x950c3c1c, 0x53096895, 0x240ecf2f, 0x25b5a649,
          0xf5ed6ab1, 0x57e60daa, 0x397b63ba, 0x00000000 },
        20,
        { 0xcefaedfe, 0xefbeadde, 0xcefaedfe, 0xefbeadde,
          0xd2daadab },
        { 0x9849e28c, 0xb6155662, 0xac33a003, 0x94b83fa1,
          0xa51291be, 0xa811a2c3, 0x3c2a26ba, 0xa72c7eca,
          0xa4a9e401, 0x903ca4fb, 0x81b2dccc, 0x6f7c8cd4,
          0xd27528d6, 0x0317a4ac, 0xe5ae344c, 0x00000000 },
        { 0xaec59c61, 0xfa0bfeff, 0x3cf42a46, 0x50d09916 }
    },

    //
    // The following test cases use 256bit Keys.
    // 
    // Test Case #7 - Test Case 13 from the doc
    // This is a special case that cannot use the GCM mode because the
    // data and AAD lengths are both zero.  The work around is to perform
    // an ECB encryption on Y0.
    //
    {
        AES_CFG_KEY_SIZE_256BIT,
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000,
          0x00000000, 0x00000000, 0x00000000, 0x00000000 },
        12,
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
        0,
        { 0 },
        0,
        { 0 },
        { 0 },
        { 0xfb8a0f53, 0xb93645c7, 0xf1b463a9, 0x8b73cbc4 }
    },

    //
    // Test Case #8, - Test Case 14 from the doc
    // This is the first test in which the AAD length is zero.
    //
    {
        AES_CFG_KEY_SIZE_256BIT,
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000,
          0x00000000, 0x00000000, 0x00000000, 0x00000000 },
        12,
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
        16,
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
        0,
        { 0 },
        { 0x3d40a7ce, 0x6e6b604d, 0xd3c54e07, 0x189df3ba },
        { 0xa7c8d1d0, 0xf06b9999, 0xb5985b26, 0x19b98ad4 }
    },

    //
    // Test Case #9, - Test Case 15 from the doc
    //
    {
        AES_CFG_KEY_SIZE_256BIT,
        { 0x92e9fffe, 0x1c736586, 0x948f6a6d, 0x08833067,
          0x92e9fffe, 0x1c736586, 0x948f6a6d, 0x08833067 },
        12,
        { 0xbebafeca, 0xaddbcefa, 0x88f8cade, 0x00000000 },
        64,
        { 0x253231d9, 0xe50684f8, 0xc50959a5, 0x9a26f5af,
          0x53a9a786, 0xdaf73415, 0x3d304c2e, 0x728a318a,
          0x950c3c1c, 0x53096895, 0x240ecf2f, 0x25b5a649,
          0xf5ed6ab1, 0x57e60daa, 0x397b63ba, 0x55d2af1a },
        0,
        { 0 },
        { 0xf0c12d52, 0x077d5699, 0xa3377ff4, 0x7d42842a,
          0xdc8c3a64, 0xc9c0e5bf, 0xbda29875, 0xaad15525,
          0x488eb08c, 0x3dbb0d59, 0x108bb0a7, 0x38888256,
          0x631ef6c5, 0x0a7aba93, 0x62f6c9bc, 0xad158089 },
        { 0xc5da94b0, 0xbd7134d9, 0x22501aec, 0x6ccce370 }
    },

    //
    // Test Case #10 - Test Case 16 from the doc
    // When the data lengths do not align with the block
    // boundary, we need to pad with zeros to ensure unknown
    // data is not copied with uDMA.
    //
    {
        AES_CFG_KEY_SIZE_256BIT,
        { 0x92e9fffe, 0x1c736586, 0x948f6a6d, 0x08833067,
          0x92e9fffe, 0x1c736586, 0x948f6a6d, 0x08833067 },
        12,
        { 0xbebafeca, 0xaddbcefa, 0x88f8cade, 0x00000000 },
        60,
        { 0x253231d9, 0xe50684f8, 0xc50959a5, 0x9a26f5af,
          0x53a9a786, 0xdaf73415, 0x3d304c2e, 0x728a318a,
          0x950c3c1c, 0x53096895, 0x240ecf2f, 0x25b5a649,
          0xf5ed6ab1, 0x57e60daa, 0x397b63ba, 0x00000000 },
        20,
        { 0xcefaedfe, 0xefbeadde, 0xcefaedfe, 0xefbeadde,
          0xd2daadab, 0x00000000, 0x00000000, 0x00000000 },
        { 0xf0c12d52, 0x077d5699, 0xa3377ff4, 0x7d42842a,
          0xdc8c3a64, 0xc9c0e5bf, 0xbda29875, 0xaad15525,
          0x488eb08c, 0x3dbb0d59, 0x108bb0a7, 0x38888256,
          0x631ef6c5, 0x0a7aba93, 0x62f6c9bc, 0x00000000 },
        { 0xce6efc76, 0x68174e0f, 0x5388dfcd, 0x1b552dbb }
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
        UARTprintf("Context input registers are ready.\n");
    }
    if(ui32IntStatus & AES_INT_DATA_IN)
    {
        ROM_AESIntDisable(AES_BASE, AES_INT_DATA_IN);
        g_bDataInIntFlag = true;
        UARTprintf("Data FIFO is ready to receive data.\n");
    }
    if(ui32IntStatus & AES_INT_CONTEXT_OUT)
    {
        ROM_AESIntDisable(AES_BASE, AES_INT_CONTEXT_OUT);
        g_bContextOutIntFlag = true;
        UARTprintf("Context output registers are ready.\n");
    }
    if(ui32IntStatus & AES_INT_DATA_OUT)
    {
        ROM_AESIntDisable(AES_BASE, AES_INT_DATA_OUT);
        g_bDataOutIntFlag = true;
        UARTprintf("Data FIFO is ready to provide data.\n");
    }
    if(ui32IntStatus & AES_INT_DMA_CONTEXT_IN)
    {
        ROM_AESIntClear(AES_BASE, AES_INT_DMA_CONTEXT_IN);
        g_bContextInDMADoneIntFlag = true;
        UARTprintf("DMA completed a context write to the internal\n");
        UARTprintf("registers.\n");
    }
    if(ui32IntStatus & AES_INT_DMA_DATA_IN)
    {
        ROM_AESIntClear(AES_BASE, AES_INT_DMA_DATA_IN);
        g_bDataInDMADoneIntFlag = true;
        UARTprintf("DMA has written the last word of input data to\n");
        UARTprintf("the internal FIFO of the engine.\n");
    }
    if(ui32IntStatus & AES_INT_DMA_CONTEXT_OUT)
    {
        ROM_AESIntClear(AES_BASE, AES_INT_DMA_CONTEXT_OUT);
        g_bContextOutDMADoneIntFlag = true;
        UARTprintf("DMA completed the output context movement from\n");
        UARTprintf("the internal registers.\n");
    }
    if(ui32IntStatus & AES_INT_DMA_DATA_OUT)
    {
        ROM_AESIntClear(AES_BASE, AES_INT_DMA_DATA_OUT);
        g_bDataOutDMADoneIntFlag = true;
        UARTprintf("DMA has written the last word of process result.\n");
    }
}

//*****************************************************************************
//
// Perform an ECB encryption operation.
//
//*****************************************************************************
bool
AESECBEncrypt(uint32_t ui32Keysize, uint32_t *pui32Src, uint32_t *pui32Dst,
              uint32_t *pui32Key, uint32_t ui32Length)
{
    //
    // Perform a soft reset.
    //
    ROM_AESReset(AES_BASE);

    //
    // Configure the AES module.
    //
    ROM_AESConfigSet(AES_BASE, (ui32Keysize | AES_CFG_DIR_ENCRYPT |
                                AES_CFG_MODE_ECB));

    //
    // Write the key.
    //
    ROM_AESKey1Set(AES_BASE, pui32Key, ui32Keysize);

    //
    // Perform the encryption.
    //
    ROM_AESDataProcess(AES_BASE, pui32Src, pui32Dst, ui32Length);

    return(true);
}

//*****************************************************************************
//
// Calculate hash subkey with the given key.
// This is performed by encrypting 128 zeroes with the key.
//
//*****************************************************************************
void
AESHashSubkeyGet(uint32_t ui32Keysize, uint32_t *pui32Key,
                 uint32_t *pui32HashSubkey)
{
    uint32_t pui32ZeroArray[8];

    //
    // Put zeroes into the first 4 words of the array.
    //
    pui32ZeroArray[0] = 0x0;
    pui32ZeroArray[1] = 0x0;
    pui32ZeroArray[2] = 0x0;
    pui32ZeroArray[3] = 0x0;

    //
    // Put zeroes into the next 4 words if the key size is 256bit
    //
    if(ui32Keysize == AES_CFG_KEY_SIZE_256BIT)
    {
        pui32ZeroArray[4] = 0x0;
        pui32ZeroArray[5] = 0x0;
        pui32ZeroArray[6] = 0x0;
        pui32ZeroArray[7] = 0x0;
    }

    //
    // Perform the encryption.
    //
    AESECBEncrypt(ui32Keysize, pui32ZeroArray, pui32HashSubkey, pui32Key,
                  (ui32Keysize == AES_CFG_KEY_SIZE_128BIT?16:32));
}

//*****************************************************************************
//
// Perform a basic GHASH operation with the hashsubkey and IV.  This is
// used to get Y0 when the IV is not 96 bits.  To use this GCM mode, the
// operation direction must not be set and the counter should be disabled.
//
//*****************************************************************************
void
AESGHASH(uint32_t ui32Keysize, uint32_t *pui32HashSubkey, uint32_t *pui32IV,
         uint32_t ui32IVLength, uint32_t *pui32Result)
{
    uint32_t ui32Count;

    //
    // Perform a soft reset.
    //
    ROM_AESReset(AES_BASE);

    //
    // Configure the AES module.
    //
    ROM_AESConfigSet(AES_BASE, (ui32Keysize | AES_CFG_MODE_GCM_HLY0ZERO));

    //
    // Set the hash subkey.
    //
    ROM_AESKey2Set(AES_BASE, pui32HashSubkey, ui32Keysize);

    //
    // Write the lengths
    //
    ROM_AESLengthSet(AES_BASE, (uint64_t)ui32IVLength);
    ROM_AESAuthLengthSet(AES_BASE, 0);

    //
    // Write the data.
    //
    for(ui32Count = 0; ui32Count < ui32IVLength; ui32Count += 16)
    {
        //
        // Write the data registers.
        //
        ROM_AESDataWrite(AES_BASE, pui32IV + (ui32Count / 4));
    }

    //
    // Read the hash tag value.
    //
    AESTagRead(AES_BASE, pui32Result);
}

//*****************************************************************************
//
// Calculate the Y0 value that needs to be written into the IV registers.
// Note: Y0 will always be 128 bits.
//
//*****************************************************************************
void
AESGCMY0Get(uint32_t ui32Keysize, uint32_t *pui32IV, uint32_t ui32IVLength,
            uint32_t *pui32Key, uint32_t *pui32Y0)
{
    uint32_t pui32HashSubkey[8];

    //
    // If the length is 96 bits, then just set the last bit of the IV to 1.
    //
    if(ui32IVLength == 12)
    {
        pui32Y0[0] = pui32IV[0];
        pui32Y0[1] = pui32IV[1];
        pui32Y0[2] = pui32IV[2];
        pui32Y0[3] = 0x01000000;
    }

    //
    // If the length is not 96 bits, then peform a basic GHASH on the IV.
    //
    else
    {
        //
        // First, get the hash subkey or H.
        //
        AESHashSubkeyGet(ui32Keysize, pui32Key, pui32HashSubkey);

        //
        // Next, perform the GHASH operation.
        //
        AESGHASH(ui32Keysize, pui32HashSubkey, pui32IV, ui32IVLength, pui32Y0);
    }
}

//*****************************************************************************
//
// Perform an GCM decryption operation.
//
//*****************************************************************************
bool
AESGCMDecrypt(uint32_t ui32Keysize, uint32_t *pui32Src, uint32_t *pui32Dst,
              uint32_t ui32Length, uint32_t *pui32Key, uint32_t *pui32IV,
              uint32_t *pui32AAD, uint32_t ui32AADLength, uint32_t *pui32Tag,
              bool bUseDMA)
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
    // Wait for the context in flag.
    //
    while(!g_bContextInIntFlag)
    {
    }

    //
    // Configure the AES module.
    //
    ROM_AESConfigSet(AES_BASE, (ui32Keysize | AES_CFG_DIR_DECRYPT |
                                AES_CFG_MODE_GCM_HY0CALC));

    //
    // Write the initialization value
    //
    ROM_AESIVSet(AES_BASE, pui32IV);

    //
    // Write the keys.
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

        if(ui32AADLength != 0)
        {
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
            ROM_uDMAChannelTransferSet(UDMA_CH14_AES0DIN | UDMA_PRI_SELECT,
                                       UDMA_MODE_BASIC, (void *)pui32AAD,
                                       (void *)(AES_BASE + AES_O_DATA_IN_0),
                                       LengthRoundUp(ui32AADLength) / 4);
            UARTprintf("Data in DMA request enabled.\n");
        }

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
                                   LengthRoundUp(ui32Length) / 4);
        UARTprintf("Data out DMA request enabled.\n");

        //
        // Write the plaintext length
        //
        ROM_AESLengthSet(AES_BASE, (uint64_t)ui32Length);

        //
        // Write the auth length registers to start the process.
        //
        ROM_AESAuthLengthSet(AES_BASE, ui32AADLength);
        
        //
        // Enable the DMA channels to start the transfers.  This must be done after
        // writing the length to prevent data from copying before the context is 
        // truly ready.
        // 
        if(ui32AADLength != 0)
        {
            ROM_uDMAChannelEnable(UDMA_CH14_AES0DIN);
        }
        ROM_uDMAChannelEnable(UDMA_CH15_AES0DOUT);

        //
        // Enable DMA requests
        //
        ROM_AESDMAEnable(AES_BASE, AES_DMA_DATA_IN | AES_DMA_DATA_OUT);

        if(ui32AADLength != 0)
        {
            //
            // Wait for the data in DMA done interrupt.
            //
            while(!g_bDataInDMADoneIntFlag)
            {
            }
        }

        if(ui32Length != 0)
        {
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
                                       LengthRoundUp(ui32Length) / 4);
            ROM_uDMAChannelEnable(UDMA_CH14_AES0DIN);
            UARTprintf("Data in DMA request enabled.\n");

            //
            // Wait for the data out DMA done interrupt.
            //
            while(!g_bDataOutDMADoneIntFlag)
            {
            }
        }

        //
        // Read out the tag.
        //
        AESTagRead(AES_BASE, pui32Tag);
    }
    else
    {
        //
        // Perform the decryption.
        //
        ROM_AESDataProcessAuth(AES_BASE, pui32Src, pui32Dst, ui32Length,
                               pui32AAD, ui32AADLength, pui32Tag);
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
// This example decrypts blocks ciphertext using AES128 and AES256 in GCM
// mode.  It does the decryption first without uDMA and then with uDMA.
// The results are checked after each operation.
//
//*****************************************************************************
int
main(void)
{
    uint32_t pui32PlainText[64], pui32Tag[4], pui32Y0[4], ui32Errors, ui32Idx;
    uint32_t *pui32Key, ui32IVLength, *pui32IV, ui32DataLength;
    uint32_t *pui32ExpPlainText, ui32AuthDataLength, *pui32AuthData;
    uint32_t *pui32CipherText, *pui32ExpTag;
    uint32_t ui32KeySize;
    uint32_t ui32SysClock;
    uint8_t ui8Vector;
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
    FrameDraw(&sContext, "aes-gcm-decrypt");
    
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
        pui32PlainText[ui32Idx] = 0;
    }
    for(ui32Idx = 0; ui32Idx < 4; ui32Idx++)
    {
        pui32Tag[ui32Idx] = 0;
    }

    //
    // Enable stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPUStackingEnable();

    //
    // Enable AES interrupts.
    //
    ROM_IntEnable(INT_AES0);

    //
    // Enable debug output on UART0 and print a welcome message.
    //
    ConfigureUART();
    UARTprintf("Starting AES GCM decryption demo.\n");
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
         (sizeof(g_psAESGCMTestVectors) / sizeof(g_psAESGCMTestVectors[0]))) &&
        (ui32Errors == 0);
        ui8Vector++)
    {
        UARTprintf("Starting vector #%d\n", ui8Vector);

        //
        // Get the current vector's data members.
        //
        ui32KeySize = g_psAESGCMTestVectors[ui8Vector].ui32KeySize;
        pui32Key = g_psAESGCMTestVectors[ui8Vector].pui32Key;
        ui32IVLength = g_psAESGCMTestVectors[ui8Vector].ui32IVLength;
        pui32IV = g_psAESGCMTestVectors[ui8Vector].pui32IV;
        ui32DataLength = g_psAESGCMTestVectors[ui8Vector].ui32DataLength;
        pui32ExpPlainText = g_psAESGCMTestVectors[ui8Vector].pui32PlainText;
        ui32AuthDataLength =
            g_psAESGCMTestVectors[ui8Vector].ui32AuthDataLength;
        pui32AuthData = g_psAESGCMTestVectors[ui8Vector].pui32AuthData;
        pui32CipherText = g_psAESGCMTestVectors[ui8Vector].pui32CipherText;
        pui32ExpTag = g_psAESGCMTestVectors[ui8Vector].pui32Tag;

        //
        // If both the data lengths are zero, then it's a special case.
        //
        if((ui32DataLength == 0) && (ui32AuthDataLength == 0))
        {
            UARTprintf("Performing decryption without uDMA.\n");

            //
            // Figure out the value of Y0 depending on the IV length.
            //
            AESGCMY0Get(ui32KeySize, pui32IV, ui32IVLength, pui32Key, pui32Y0);

            //
            // Perform the basic encryption.
            //
            AESECBEncrypt(ui32KeySize, pui32Y0, pui32Tag, pui32Key, 16);
        }
        else
        {
            //
            // Figure out the value of Y0 depending on the IV length.
            //
            AESGCMY0Get(ui32KeySize, pui32IV, ui32IVLength, pui32Key, pui32Y0);

            //
            // Perform the decryption without uDMA.
            //
            UARTprintf("Performing decryption without uDMA.\n");
            AESGCMDecrypt(ui32KeySize, pui32CipherText, pui32PlainText,
                          ui32DataLength, pui32Key, pui32Y0, pui32AuthData,
                          ui32AuthDataLength, pui32Tag, false);
        }

        //
        // Check the results.
        //
        for(ui32Idx = 0; ui32Idx < (ui32DataLength / 4); ui32Idx++)
        {
            if(pui32ExpPlainText[ui32Idx] != pui32PlainText[ui32Idx])
            {
                UARTprintf("Plaintext mismatch on word %d. Exp: 0x%x, Act: "
                           "0x%x\n", ui32Idx, pui32ExpPlainText[ui32Idx],
                           pui32PlainText[ui32Idx]);
                ui32Errors |= (ui32Idx << 16) | 0x00000002;
            }
        }
        for(ui32Idx = 0; ui32Idx < 4; ui32Idx++)
        {
            if(pui32ExpTag[ui32Idx] != pui32Tag[ui32Idx])
            {
                UARTprintf("Tag mismatch on word %d. Exp: 0x%x, Act: 0x%x\n",
                           ui32Idx, pui32ExpTag[ui32Idx], pui32Tag[ui32Idx]);
                ui32Errors |= (ui32Idx << 16) | 0x00000003;
            }
        }

        //
        // Clear the arrays containing the ciphertext and tag to ensure things
        // are working correctly.
        //
        for(ui32Idx = 0; ui32Idx < 16; ui32Idx++)
        {
            pui32PlainText[ui32Idx] = 0;
        }
        for(ui32Idx = 0; ui32Idx < 4; ui32Idx++)
        {
            pui32Tag[ui32Idx] = 0;
        }

        //
        // Only use DMA with the vectors that have data.
        //
        if((ui32DataLength != 0) || (ui32AuthDataLength != 0))
        {
            //
            // Perform the decryption with uDMA.
            //
            UARTprintf("Performing decryption with uDMA.\n");
            AESGCMDecrypt(ui32KeySize, pui32CipherText, pui32PlainText,
                          ui32DataLength, pui32Key, pui32Y0, pui32AuthData,
                          ui32AuthDataLength, pui32Tag, true);

            //
            // Check the result.
            //
            for(ui32Idx = 0; ui32Idx < (ui32DataLength / 4); ui32Idx++)
            {
                if(pui32ExpPlainText[ui32Idx] != pui32PlainText[ui32Idx])
                {
                    UARTprintf("Plaintext mismatch on word %d. Exp: 0x%x, "
                               "Act: 0x%x\n", ui32Idx,
                               pui32ExpPlainText[ui32Idx],
                               pui32PlainText[ui32Idx]);
                    ui32Errors |= (ui32Idx << 16) | 0x00000002;
                }
            }
            for(ui32Idx = 0; ui32Idx < 4; ui32Idx++)
            {
                if(pui32ExpTag[ui32Idx] != pui32Tag[ui32Idx])
                {
                    UARTprintf("Tag mismatch on word %d. Exp: 0x%x, Act: "
                               "0x%x\n", ui32Idx, pui32ExpTag[ui32Idx],
                               pui32Tag[ui32Idx]);
                    ui32Errors |= (ui32Idx << 16) | 0x00000003;
                }
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

    //
    // Wait forever.
    //
    while(1)
    {
    }
}
