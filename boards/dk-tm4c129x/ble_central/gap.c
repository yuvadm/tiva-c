//*****************************************************************************
//
// gap.c - GAP configuration and control APIs
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
#include <string.h>
#include "hci.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
// TX buffer used by all APIs.
//
//*****************************************************************************
uint8_t g_pui8TXBuf[64];
uint8_t g_ui8TXLen;

//*****************************************************************************
//
// Forward reference of functions used in this file.
//
//*****************************************************************************
extern void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count);
extern void DumpBuffer(uint8_t *pui8Buf, uint32_t ui32Len, bool bTX);

//*****************************************************************************
//
// GAP Device Initialization Request
//
//*****************************************************************************
void
GAPDeviceInit(uint8_t  ui8ProfileRole,
              uint8_t  ui8MaxScanRsps,
              uint8_t  *pui8IRK,
              uint8_t  *pui8SRK,
              uint32_t ui32SignCounter)
{
    g_ui8TXLen = 0;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_CMD_PACKET;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_VE_GAP_DEVICE_INIT_OPCODE & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (HCI_VE_GAP_DEVICE_INIT_OPCODE >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++]  = 38;
    g_pui8TXBuf[g_ui8TXLen++]  = ui8ProfileRole;
    g_pui8TXBuf[g_ui8TXLen++]  = ui8MaxScanRsps;
    memcpy(g_pui8TXBuf + g_ui8TXLen, pui8IRK, 16);
    g_ui8TXLen +=16;
    memcpy(g_pui8TXBuf + g_ui8TXLen, pui8SRK, 16);
    g_ui8TXLen +=16;
    g_pui8TXBuf[g_ui8TXLen++] = ui32SignCounter & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (ui32SignCounter >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (ui32SignCounter >> 16) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (ui32SignCounter >> 24) & 0xFF;

    DumpBuffer((uint8_t*)g_pui8TXBuf, g_ui8TXLen, true);
    UARTSend((const uint8_t*)g_pui8TXBuf, g_ui8TXLen);
    return;
}

//*****************************************************************************
//
// GAP Get Parameter Request.
//
//*****************************************************************************
void
GAPGetParam(uint8_t ui8ParamID)
{
    g_ui8TXLen = 0;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_CMD_PACKET;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_VE_GAP_GET_PARAM_OPCODE & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (HCI_VE_GAP_GET_PARAM_OPCODE >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++]  = 1;
    g_pui8TXBuf[g_ui8TXLen++]  = ui8ParamID;

    DumpBuffer((uint8_t*)g_pui8TXBuf, g_ui8TXLen, true);
    UARTSend((const uint8_t*)g_pui8TXBuf, g_ui8TXLen);
    return;
}

//*****************************************************************************
//
// GAP Set Parameter request.
//
//*****************************************************************************
void
GAPSetParam(uint8_t ui8ParamID, uint16_t ui16Value)
{
    g_ui8TXLen = 0;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_CMD_PACKET;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_VE_GAP_SET_PARAM_OPCODE & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (HCI_VE_GAP_SET_PARAM_OPCODE >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = 3;
    g_pui8TXBuf[g_ui8TXLen++] = ui8ParamID;
    g_pui8TXBuf[g_ui8TXLen++] = ui16Value & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (ui16Value >> 8) & 0xFF;

    DumpBuffer((uint8_t*)g_pui8TXBuf, g_ui8TXLen, true);
    UARTSend((const uint8_t*)g_pui8TXBuf, g_ui8TXLen);
    return;
}

//*****************************************************************************
//
// Start Discovery Request.
//
//*****************************************************************************
void
GAPDiscoveryReq(uint8_t ui8Mode, bool bActiveScan, bool bWhiteList)
{
    g_ui8TXLen = 0;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_CMD_PACKET;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_VE_GAP_DEVICE_DISC_REQ_OPCODE & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (HCI_VE_GAP_DEVICE_DISC_REQ_OPCODE >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++]  = 3;
    g_pui8TXBuf[g_ui8TXLen++]  = ui8Mode;
    g_pui8TXBuf[g_ui8TXLen++]  = bActiveScan;
    g_pui8TXBuf[g_ui8TXLen++]  = bWhiteList;

    DumpBuffer((uint8_t*)g_pui8TXBuf, g_ui8TXLen, true);
    UARTSend((const uint8_t*)g_pui8TXBuf, g_ui8TXLen);
    return;
}

//*****************************************************************************
//
// Establish Link Request.
//
//*****************************************************************************
void
GAPEstLinkReq(bool bHighDutyCycle, bool bWhiteList,
                   uint8_t ui8AddrType, uint8_t *pui8DevAddr)
{
    uint8_t ui8Idx;

    g_ui8TXLen = 0;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_CMD_PACKET;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_VE_GAP_DEVICE_EST_LINK_REQ_OPCODE & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = 
        (HCI_VE_GAP_DEVICE_EST_LINK_REQ_OPCODE >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = 9;
    g_pui8TXBuf[g_ui8TXLen++] = bHighDutyCycle;
    g_pui8TXBuf[g_ui8TXLen++] = bWhiteList;
    g_pui8TXBuf[g_ui8TXLen++] = ui8AddrType;
    for(ui8Idx = 0; ui8Idx < HCI_BDADDR_LEN; ui8Idx++)
    {
        g_pui8TXBuf[g_ui8TXLen++] = pui8DevAddr[ui8Idx];
    }

    DumpBuffer((uint8_t*)g_pui8TXBuf, g_ui8TXLen, true);
    UARTSend((const uint8_t*)g_pui8TXBuf, g_ui8TXLen);
    return;
}

//*****************************************************************************
//
// Terminate Link Request.
//
//*****************************************************************************
void
GAPTerLinkReq(uint16_t ui16ConnHandle, uint8_t ui8Reason)
{
    g_ui8TXLen = 0;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_CMD_PACKET;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_VE_GAP_DEVICE_TER_LINK_REQ_OPCODE & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = 
        (HCI_VE_GAP_DEVICE_TER_LINK_REQ_OPCODE >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = 3;
    g_pui8TXBuf[g_ui8TXLen++] = ui16ConnHandle & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (ui16ConnHandle >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = ui8Reason;

    DumpBuffer((uint8_t*)g_pui8TXBuf, g_ui8TXLen, true);
    UARTSend((const uint8_t*)g_pui8TXBuf, g_ui8TXLen);
    return;
}

//*****************************************************************************
//
// Start Pairing Request.
//
//*****************************************************************************
void
GAPAuthenticate(uint16_t ui16ConnHandle)
{
    g_ui8TXLen = 0;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_CMD_PACKET;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_VE_GAP_DEVICE_AUTHENTICATE_OPCODE & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] =
        (HCI_VE_GAP_DEVICE_AUTHENTICATE_OPCODE >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = 29;
    g_pui8TXBuf[g_ui8TXLen++] = ui16ConnHandle & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (ui16ConnHandle >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = 0x04; //KeyboardDisplay
    g_pui8TXBuf[g_ui8TXLen++] = 0x00; //oob false
    memset(&g_pui8TXBuf[g_ui8TXLen], 0, 16);
    g_ui8TXLen +=16;
    g_pui8TXBuf[g_ui8TXLen++] = 0x05; //AuthReq
    g_pui8TXBuf[g_ui8TXLen++] = 0x10; // MaxEKeySize
    g_pui8TXBuf[g_ui8TXLen++] = 0x3F; // KeyDist
    g_pui8TXBuf[g_ui8TXLen++] = 0x00; // Pair.enable
    g_pui8TXBuf[g_ui8TXLen++] = 0x03; // Pair.ioCaps
    g_pui8TXBuf[g_ui8TXLen++] = 0x00; // Pair.oobDFlag
                                       
    g_pui8TXBuf[g_ui8TXLen++] = 0x01; // Pair.authReq
    g_pui8TXBuf[g_ui8TXLen++] = 0x10; // MaxEKeySize
    g_pui8TXBuf[g_ui8TXLen++] = 0x3F; // KeyDist

    DumpBuffer((uint8_t*)g_pui8TXBuf, g_ui8TXLen, true);
    UARTSend((const uint8_t*)g_pui8TXBuf, g_ui8TXLen);
    return;
}

//*****************************************************************************
//
// PASS key during Authentication.
//
//*****************************************************************************
void
GAPPassKeyUpdate(uint16_t ui16ConnHandle, char *pcPassCode)
{
    g_ui8TXLen = 0;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_CMD_PACKET;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_VE_GAP_DEVICE_PASSKEY_UPDATE_OPCODE & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] =
        (HCI_VE_GAP_DEVICE_PASSKEY_UPDATE_OPCODE >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = 8;
    g_pui8TXBuf[g_ui8TXLen++] = ui16ConnHandle & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (ui16ConnHandle >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = pcPassCode[0];
    g_pui8TXBuf[g_ui8TXLen++] = pcPassCode[1];
    g_pui8TXBuf[g_ui8TXLen++] = pcPassCode[2];
    g_pui8TXBuf[g_ui8TXLen++] = pcPassCode[3];
    g_pui8TXBuf[g_ui8TXLen++] = pcPassCode[4];
    g_pui8TXBuf[g_ui8TXLen++] = pcPassCode[5];

    DumpBuffer((uint8_t*)g_pui8TXBuf, g_ui8TXLen, true);
    UARTSend((const uint8_t*)g_pui8TXBuf, g_ui8TXLen);
    return;
}

//*****************************************************************************
//
// GAP Bonding Request.
//
//*****************************************************************************
void
GAPBond(uint16_t ui16ConnHandle, tLTKData *psSavedKey)
{
    uint8_t ui8Loop;

    g_ui8TXLen = 0;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_CMD_PACKET;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_VE_GAP_DEVICE_BOND_OPCODE & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (HCI_VE_GAP_DEVICE_BOND_OPCODE >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = 30;
    g_pui8TXBuf[g_ui8TXLen++] = ui16ConnHandle & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (ui16ConnHandle >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = psSavedKey->bAuth;
    for(ui8Loop = 0; ui8Loop < psSavedKey->ui8LTKSize; ui8Loop++)
    {
        g_pui8TXBuf[g_ui8TXLen++] = psSavedKey->pui8LTK[ui8Loop];
    }
    g_pui8TXBuf[g_ui8TXLen++] = psSavedKey->pui8DIV[0];
    g_pui8TXBuf[g_ui8TXLen++] = psSavedKey->pui8DIV[1];
    for(ui8Loop = 0; ui8Loop < 8; ui8Loop++)
    {
        g_pui8TXBuf[g_ui8TXLen++] = psSavedKey->pui8Rand[ui8Loop];
    }
    g_pui8TXBuf[g_ui8TXLen++] = psSavedKey->ui8LTKSize;

    DumpBuffer((uint8_t*)g_pui8TXBuf, g_ui8TXLen, true);
    UARTSend((const uint8_t*)g_pui8TXBuf, g_ui8TXLen);
    return;
}

//*****************************************************************************
//
// Read Characteristic Value.
//
//*****************************************************************************
void
GAPReadCharValue(uint16_t ui16ConnHandle, uint16_t ui16Handle)
{
    g_ui8TXLen = 0;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_CMD_PACKET;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_VE_GAP_DEVICE_READ_CHAR_VAL_OPCODE & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] =
        (HCI_VE_GAP_DEVICE_READ_CHAR_VAL_OPCODE >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = 4;
    g_pui8TXBuf[g_ui8TXLen++] = ui16ConnHandle & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = ui16ConnHandle & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = ui16Handle & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (ui16Handle >> 8) & 0xFF;

    DumpBuffer((uint8_t*)g_pui8TXBuf, g_ui8TXLen, true);
    UARTSend((const uint8_t*)g_pui8TXBuf, g_ui8TXLen);
    return;
    
}

//*****************************************************************************
//
// Write Characteristic Value request.
//
//*****************************************************************************
void
GAPWriteCharValue(uint16_t ui16ConnHandle, uint16_t ui16Handle,
                       uint8_t *pui8Buf, uint8_t ui8Len)
{
    uint8_t ui8Loop;

    g_ui8TXLen = 0;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_CMD_PACKET;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_VE_GAP_DEVICE_WRITE_CHAR_VAL_OPCODE & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] =
        (HCI_VE_GAP_DEVICE_WRITE_CHAR_VAL_OPCODE >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = 4 + ui8Len;
    g_pui8TXBuf[g_ui8TXLen++] = ui16ConnHandle & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = ui16ConnHandle & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = ui16Handle & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (ui16Handle >> 8) & 0xFF;
    for(ui8Loop = 0; ui8Loop < ui8Len; ui8Loop++)
    {
        g_pui8TXBuf[g_ui8TXLen++] = pui8Buf[ui8Loop];
    }

    DumpBuffer((uint8_t*)g_pui8TXBuf, g_ui8TXLen, true);
    UARTSend((const uint8_t*)g_pui8TXBuf, g_ui8TXLen);
    return;
}

//*****************************************************************************
//
// Read RSSI Value Request.
//
//*****************************************************************************
void
HCIReadRSSI(uint16_t ui16ConnHandle)
{
    g_ui8TXLen = 0;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_CMD_PACKET;
    g_pui8TXBuf[g_ui8TXLen++] = HCI_READ_RSSI_OPCODE & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (HCI_READ_RSSI_OPCODE >> 8) & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = 2;
    g_pui8TXBuf[g_ui8TXLen++] = ui16ConnHandle & 0xFF;
    g_pui8TXBuf[g_ui8TXLen++] = (ui16ConnHandle >>8) & 0xFF;

    DumpBuffer((uint8_t*)g_pui8TXBuf, g_ui8TXLen, true);
    UARTSend((const uint8_t*)g_pui8TXBuf, g_ui8TXLen);
    return;
}
