//*****************************************************************************
//
// nfc_dep.c - used to send packets of P2P
//
// Copyright (c) 2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the Tiva Firmware Development Package.
//
//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nfclib/nfc_dep.h"
#include "nfclib/llcp.h"
#include "nfclib/trf79x0.h"

//*****************************************************************************
//
// Globals
//
//*****************************************************************************

uint8_t g_pui8NFCID3t[10] = {0x01, 0xFE, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                             0x08, 0x09};

uint8_t g_ui8NfcDepPni = 0x00;

uint8_t g_ui8RtoxTransportData;

tPDUBlock tNextPduType = INFORMATION_PDU;

uint8_t * g_pui8DEPBufferPtr;

//*****************************************************************************
//
// NFCDEP_SendATR_REQ -
//
//*****************************************************************************
void NFCDEP_SendATR_REQ(uint8_t * pui8NFCID2_Ptr)
{
    uint8_t ui8Counter = 0;
    uint8_t ui8Offset = 0;

    //
    // Length
    //
    g_pui8DEPBufferPtr[0] = 0x25;

    //
    // Command
    //
    g_pui8DEPBufferPtr[1] = (uint8_t) ((ATR_REQ_CMD & 0xFF00) >> 8);
    g_pui8DEPBufferPtr[2] = (uint8_t) (ATR_REQ_CMD & 0x00FF);

    //
    // NFCID3i
    //
    for(ui8Counter=0;ui8Counter<8;ui8Counter++)
    {
        g_pui8DEPBufferPtr[3+ui8Counter] = pui8NFCID2_Ptr[ui8Counter];
    }
    g_pui8DEPBufferPtr[11] = 0x00;
    g_pui8DEPBufferPtr[12] = 0x00;

    g_pui8DEPBufferPtr[13] = DIDi;
    g_pui8DEPBufferPtr[14] = BSi;
    g_pui8DEPBufferPtr[15] = BRi;
    g_pui8DEPBufferPtr[16] = PPi;     // Max Payload 64 bytes

    //
    // LLCP Magic Number
    //
    g_pui8DEPBufferPtr[17] = LLCP_MAGIC_NUMBER_HIGH;
    g_pui8DEPBufferPtr[18] = LLCP_MAGIC_NUMBER_MIDDLE;
    g_pui8DEPBufferPtr[19] = LLCP_MAGIC_NUMBER_LOW;

    ui8Offset = 20;
    ui8Offset = ui8Offset + LLCP_addTLV(LLCP_VERSION,
                                        &g_pui8DEPBufferPtr[ui8Offset]);
    ui8Offset = ui8Offset + LLCP_addTLV(LLCP_MIUX,&g_pui8DEPBufferPtr[ui8Offset]);
    ui8Offset = ui8Offset + LLCP_addTLV(LLCP_WKS,&g_pui8DEPBufferPtr[ui8Offset]);
    ui8Offset = ui8Offset + LLCP_addTLV(LLCP_LTO,&g_pui8DEPBufferPtr[ui8Offset]);
    ui8Offset = ui8Offset + LLCP_addTLV(LLCP_OPT,&g_pui8DEPBufferPtr[ui8Offset]);

    TRF79x0WriteFIFO(g_pui8DEPBufferPtr,CRC_BIT_ENABLE,ui8Offset);
}

//*****************************************************************************
//
// NFCDEP_SendPSL_REQ -
//
//*****************************************************************************
void NFCDEP_SendPSL_REQ(void)
{
    uint8_t ui8Offset = 1;

    //
    // Command
    //
    g_pui8DEPBufferPtr[ui8Offset++] = (uint8_t) ((PSL_REQ_CMD & 0xFF00) >> 8);
    g_pui8DEPBufferPtr[ui8Offset++] = (uint8_t) (PSL_REQ_CMD & 0x00FF);

    //
    // DID
    //
    g_pui8DEPBufferPtr[ui8Offset++] = 0x00;

    //
    // BRS -
    // B5 B4 B3 (DSI) Initiator to Target
    // B2 B1 B0 (DRI) Target to Initiator
    // 0  0  0  106kbaud
    // 0  0  1  212kbaud
    // 0  1  0  424kbaud (default)
    // 0  1  1  848kbaud
    //
    g_pui8DEPBufferPtr[ui8Offset++] = 0x12;

    //
    // FSL
    // B1-B0 Max Payload Size (11b: Max payload size is 254 bytes)
    //
    g_pui8DEPBufferPtr[ui8Offset++] = 0x03;

    //
    // Length
    //
    g_pui8DEPBufferPtr[0] = ui8Offset;

    TRF79x0WriteFIFO(g_pui8DEPBufferPtr,CRC_BIT_ENABLE,ui8Offset);
}

//*****************************************************************************
//
// NFCDEP_SendATR_RES -
//
//*****************************************************************************
void NFCDEP_SendATR_RES(void)
{
    uint8_t ui8Counter = 0;
    uint8_t ui8Offset = 1;

    //
    // Command
    //
    g_pui8DEPBufferPtr[ui8Offset++] = (uint8_t) ((ATR_RES_CMD & 0xFF00) >> 8);
    g_pui8DEPBufferPtr[ui8Offset++] = (uint8_t) (ATR_RES_CMD & 0x00FF);

    //
    // NFCID3t
    //
    for(ui8Counter=0;ui8Counter<10;ui8Counter++)
    {
        g_pui8DEPBufferPtr[ui8Offset++] = g_pui8NFCID3t[ui8Counter];
    }

    g_pui8DEPBufferPtr[ui8Offset++] = DIDt;
    g_pui8DEPBufferPtr[ui8Offset++] = BSt;
    g_pui8DEPBufferPtr[ui8Offset++] = BRt;
    g_pui8DEPBufferPtr[ui8Offset++] = TO;
    g_pui8DEPBufferPtr[ui8Offset++] = PPt;       // Max Payload 64 bytes

    //
    // LLCP Magic Number
    //
    g_pui8DEPBufferPtr[ui8Offset++] = LLCP_MAGIC_NUMBER_HIGH;
    g_pui8DEPBufferPtr[ui8Offset++] = LLCP_MAGIC_NUMBER_MIDDLE;
    g_pui8DEPBufferPtr[ui8Offset++] = LLCP_MAGIC_NUMBER_LOW;

    ui8Offset = ui8Offset + LLCP_addTLV(LLCP_VERSION,
                                            &g_pui8DEPBufferPtr[ui8Offset]);
    ui8Offset = ui8Offset + LLCP_addTLV(LLCP_MIUX,&g_pui8DEPBufferPtr[ui8Offset]);
    ui8Offset = ui8Offset + LLCP_addTLV(LLCP_WKS,&g_pui8DEPBufferPtr[ui8Offset]);
    ui8Offset = ui8Offset + LLCP_addTLV(LLCP_LTO,&g_pui8DEPBufferPtr[ui8Offset]);
    ui8Offset = ui8Offset + LLCP_addTLV(LLCP_OPT,&g_pui8DEPBufferPtr[ui8Offset]);

    //
    // Length
    //
    g_pui8DEPBufferPtr[0] = ui8Offset;

    TRF79x0WriteFIFO(g_pui8DEPBufferPtr,CRC_BIT_ENABLE,ui8Offset);
}

//*****************************************************************************
//
// NFCDEP_SendRSL_RES -
//
//*****************************************************************************
void NFCDEP_SendRSL_RES(void)
{
    uint8_t ui8Offset = 1;

    //
    // Command
    //
    g_pui8DEPBufferPtr[ui8Offset++] = (uint8_t) ((RSL_RES_CMD & 0xFF00) >> 8);
    g_pui8DEPBufferPtr[ui8Offset++] = (uint8_t) (RSL_RES_CMD & 0x00FF);

    //
    // Length
    //
    g_pui8DEPBufferPtr[0] = ui8Offset;

    TRF79x0WriteFIFO(g_pui8DEPBufferPtr,CRC_BIT_ENABLE,ui8Offset);
}

//*****************************************************************************
//
// NFCDEP_SendPSL_RES -
//
//*****************************************************************************
void NFCDEP_SendPSL_RES(uint8_t did_value)
{
    uint8_t ui8Offset = 1;

    //
    // Command
    //
    g_pui8DEPBufferPtr[ui8Offset++] = (uint8_t) ((PSL_RES_CMD & 0xFF00) >> 8);
    g_pui8DEPBufferPtr[ui8Offset++] = (uint8_t) (PSL_RES_CMD & 0x00FF);

    g_pui8DEPBufferPtr[ui8Offset++] = 0x00;

    //
    // Length
    //
    g_pui8DEPBufferPtr[0] = ui8Offset;

    TRF79x0WriteFIFO(g_pui8DEPBufferPtr,CRC_BIT_ENABLE,ui8Offset);
}

//*****************************************************************************
//
// NFCDEP_ProcessReceivedRequest -
//
//*****************************************************************************
tStatus NFCDEP_ProcessReceivedRequest(uint8_t * pui8RxBuffer , \
                                        uint8_t * pui8NFCID2_Ptr,
                                        bool bActiveResponse)
{
    volatile uint8_t ui8CommandLength;
    uint16_t ui16Command;
    tStatus eNfcDepStatus = STATUS_SUCCESS;
    uint8_t ui8PFBValue;
    uint8_t ui8Counter;

    ui8CommandLength = pui8RxBuffer[0];
    ui16Command = pui8RxBuffer[2]  + (pui8RxBuffer[1] << 8);

    ui8PFBValue = pui8RxBuffer[3];

    // Check if chaining is enabled
    if((ui8PFBValue & 0xF0) == 0x00)
    {
        tNextPduType = INFORMATION_PDU;
    }
    else if((ui8PFBValue & 0xF0) == 0x10)
    {
        tNextPduType = ACK_PDU;
    }
    else if((ui8PFBValue & 0xF0) == 0x90)
    {
        tNextPduType = RTOX_REQ_PDU;
    }
    else if((ui8PFBValue & 0xF0) == 0x80)
    {
        tNextPduType = ATN_PDU;
    }


    if(ui16Command == ATR_REQ_CMD)
    {
        if((pui8NFCID2_Ptr[0] == pui8RxBuffer[3]  && \
           pui8NFCID2_Ptr[1] == pui8RxBuffer[4]  && \
           pui8NFCID2_Ptr[2] == pui8RxBuffer[5]  && \
           pui8NFCID2_Ptr[3] == pui8RxBuffer[6]  && \
           pui8NFCID2_Ptr[4] == pui8RxBuffer[7]  && \
           pui8NFCID2_Ptr[5] == pui8RxBuffer[8]  && \
           pui8NFCID2_Ptr[6] == pui8RxBuffer[9]  && \
           pui8NFCID2_Ptr[7] == pui8RxBuffer[10]) || bActiveResponse == true)
        {
            ui8Counter = 0;
            while(ui8CommandLength > (ui8Counter+20))
            {
                //
                // Process the TLV - pass the starting address of the TLV
                //
                LLCP_processTLV(&pui8RxBuffer[ui8Counter+20]);

                //
                // Increment ui8Counter by the length+ 2 (type and length) of
                // the current TLV
                //
                ui8Counter = ui8Counter+ pui8RxBuffer[ui8Counter+21] + 2;
    		}
            NFCDEP_SendATR_RES();
            // Reset the PNI
            g_ui8NfcDepPni = 0x00;
            //UARTprintf("CMD : D400\n");
        }
        else
            eNfcDepStatus = STATUS_FAIL;
    }
    else if(ui16Command == PSL_REQ_CMD)
    {
        // Check if the DSI (Bits 5-3) == 010b => 424kbaud (Init. to Target)
        //  if the DRI (2-0) == 010b => 424kbaud (Target to Initiator)
        if(((pui8RxBuffer[4] & 0x38) == 0x10) && \
           ((pui8RxBuffer[4] & 0x07) == 0x02))
        {
            NFCDEP_SendPSL_RES(pui8RxBuffer[3]);
            TRF79x0SetMode(P2P_PASSIVE_TARGET_MODE,FREQ_424_KBPS);
        }

    }
    else if(ui16Command == DEP_REQ_CMD)
    {
        //
        // LLCP Packet Handler
        //
        if(tNextPduType == INFORMATION_PDU)
        {
            LLCP_processReceivedData(&pui8RxBuffer[4], (ui8CommandLength-4));
        }

        NFCDEP_SendDEP_RES();
    }
    else if(ui16Command == DSL_REQ_CMD)
    {
        //
        // Debug
        //
        while(1);
    }
    else if(ui16Command == RSL_REQ_CMD)
    {
        //UARTprintf("CMD : D40A\n");
        if(ui8CommandLength == 0x03)
            NFCDEP_SendRSL_RES();
    }
    else
    {
        eNfcDepStatus = STATUS_FAIL;

    }
    return eNfcDepStatus;
}

//*****************************************************************************
//
// NFCDEP_ProcessReceivedData -
//
//*****************************************************************************
tStatus NFCDEP_ProcessReceivedData(uint8_t * pui8RxBuffer)
{
    volatile uint8_t ui8CommandLength;
    uint16_t ui16Command;
    uint8_t ui8Counter;
    tStatus eNfcDepStatus = STATUS_SUCCESS;
    uint8_t ui8PFBValue;

    ui8CommandLength = pui8RxBuffer[0];
    ui16Command = pui8RxBuffer[2]  + (pui8RxBuffer[1] << 8);

    if(ui16Command == ATR_RES_CMD)
    {
        //
        // Store the g_pui8NFCID3t
        //
        for(ui8Counter = 0; ui8Counter < 10; ui8Counter++)
        {
            g_pui8NFCID3t[ui8Counter] = pui8RxBuffer[3+ui8Counter];
        }
        //
        // LLCP Decoding - RFU
        //
        if(pui8RxBuffer[18] == LLCP_MAGIC_NUMBER_HIGH && \
           pui8RxBuffer[19] == LLCP_MAGIC_NUMBER_MIDDLE && \
           pui8RxBuffer[20] == LLCP_MAGIC_NUMBER_LOW)
        {
            ui8Counter = 0;
            while(ui8CommandLength > (ui8Counter+21))
            {
                //
                // Process the TLV - pass the starting address of the TLV
                //
                LLCP_processTLV(&pui8RxBuffer[ui8Counter+21]);

                //
                // Increment ui8Counter by the length+ 2 (type and length) of
                // the current TLV
                //
                ui8Counter = ui8Counter+ pui8RxBuffer[ui8Counter+22] + 2;
    		}
            //
            // Set the next PDU for LLCP - SYMM PDU
            //
            LLCP_setNextPDU(LLCP_SYMM_PDU);

            //
            // Reset the PNI
            //
            g_ui8NfcDepPni = 0x00;
            tNextPduType = INFORMATION_PDU;
        }
        else
        {
            eNfcDepStatus = STATUS_FAIL;
        }
    }
    else if(ui16Command == PSL_RES_CMD)
    {
        //
        // Check if DID is correct
        //
        if(pui8RxBuffer[3] != 0x00)
            eNfcDepStatus = STATUS_FAIL;

    }
    else if(ui16Command == DEP_RES_CMD)
    {
        ui8PFBValue = pui8RxBuffer[3];

        if((ui8PFBValue & 0xF0) == 0x00)
        {
            tNextPduType = INFORMATION_PDU;
        }
        else if((ui8PFBValue & 0xF0) == 0x10)
        {
            //
            // Check if chaining is enabled
            //
            tNextPduType = ACK_PDU;
        }
        else if((ui8PFBValue & 0xF0) == 0x90)
        {
            tNextPduType = RTOX_REQ_PDU;
            g_ui8RtoxTransportData = (0x3F & pui8RxBuffer[4]);
        }

        if(tNextPduType == INFORMATION_PDU)
        //
        // LLCP Packet Handler
        //
            eNfcDepStatus = LLCP_processReceivedData(&pui8RxBuffer[4],
                                                    (ui8CommandLength-4));

    }
    else if(ui16Command == DSL_RES_CMD)
    {

    }
    else if(ui16Command == RSL_RES_CMD)
    {

    }
    else
    {
        eNfcDepStatus = STATUS_FAIL;
    }

    return eNfcDepStatus;
}

//*****************************************************************************
//
// NFCDEP_SendDEP_REQ - Send DEP_REQ to TRF79x0
//
//*****************************************************************************
void NFCDEP_SendDEP_REQ(uint8_t * pui8RxBuffer)
{
    uint8_t ui8TotalLength = 0;

    if(tNextPduType == INFORMATION_PDU)
    {
        //
        // Total = 1 byte Length + 2 bytes Command + 1 byte PFB + n PDU
        //
        ui8TotalLength = 4 + LLCP_stateMachine(&g_pui8DEPBufferPtr[4]);

        //
        // PFB Byte
        //
        g_pui8DEPBufferPtr[3] = ((tNextPduType | (g_ui8NfcDepPni++)) & 0x03);
    }
    else if(tNextPduType == RTOX_REQ_PDU)
    {
        //
        // PFB Byte
        //
        g_pui8DEPBufferPtr[3] = (tNextPduType);

        g_pui8DEPBufferPtr[4] = g_ui8RtoxTransportData;
        ui8TotalLength = 5;
    }
    else if(tNextPduType == ACK_PDU)
    {
        //
        // PFB Byte
        //
        g_pui8DEPBufferPtr[3] = ((tNextPduType | (g_ui8NfcDepPni++)) & 0x03);

        ui8TotalLength = 4;
    }

    //
    // Length
    //
    g_pui8DEPBufferPtr[0] = ui8TotalLength;

    //
    // Command
    //
    g_pui8DEPBufferPtr[1] = (uint8_t) ((DEP_REQ_CMD & 0xFF00) >> 8);
    g_pui8DEPBufferPtr[2] = (uint8_t) (DEP_REQ_CMD & 0x00FF);

    TRF79x0WriteFIFO(g_pui8DEPBufferPtr,CRC_BIT_ENABLE,ui8TotalLength);

    if(tNextPduType == RTOX_REQ_PDU)
        if(TRF79x0IRQHandler(((2<<g_ui8RtoxTransportData)/3)) ==
                                                        IRQ_STATUS_RX_COMPLETE)
        {
            NFCDEP_ProcessReceivedData(pui8RxBuffer);
            NFCDEP_SendDEP_REQ(pui8RxBuffer);
        }
}

//*****************************************************************************
//
// NFCDEP_SendDEP_RES -Send DEP_RES to TRF79x0
//
//*****************************************************************************
void NFCDEP_SendDEP_RES(void)
{
    uint8_t ui8TotalLength = 0;

    if(tNextPduType == INFORMATION_PDU)
    {
        //
        // PFB Byte
        //
        g_pui8DEPBufferPtr[3] = ((tNextPduType | (g_ui8NfcDepPni++)) & 0x03);

        //
        // Total = 1 byte Length + 2 bytes Command + 1 byte PFB + n PDU
        //
        ui8TotalLength = 4 + LLCP_stateMachine(&g_pui8DEPBufferPtr[4]);
    }
    else if(tNextPduType == ATN_PDU)
    {
        //
        // PFB Byte
        //
        g_pui8DEPBufferPtr[3] = (tNextPduType);

        ui8TotalLength = 4;
    }

    //
    // Length
    //
    g_pui8DEPBufferPtr[0] = ui8TotalLength;

    //
    // Command
    //
    g_pui8DEPBufferPtr[1] = (uint8_t) ((DEP_RES_CMD & 0xFF00) >> 8);
    g_pui8DEPBufferPtr[2] = (uint8_t) (DEP_RES_CMD & 0x00FF);

    TRF79x0WriteFIFO(g_pui8DEPBufferPtr,CRC_BIT_ENABLE,ui8TotalLength);
}

//*****************************************************************************
//
// NFCDEP_SetBufferPtr - set global buffer pointer to input pointer value
//
//*****************************************************************************
void NFCDEP_SetBufferPtr(uint8_t * buffer_ptr)
{
    g_pui8DEPBufferPtr = buffer_ptr;
}

