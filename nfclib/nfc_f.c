//*****************************************************************************
//
// nfc_f.c - contains implementation of NFC Type F (Felica) protocol
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
#include "nfclib/nfc_f.h"
#include "nfclib/trf79x0.h"

//*****************************************************************************
//
// NFC ID for TYPE F cards
//
//*****************************************************************************
uint8_t g_ui8NFCID2[8] = {0x01 , 0xFE, 0x88 , 0x77, 0x66 , 0x55, 0x44 , 0x33};

//*****************************************************************************
//
// Pointer to buffer
//
//*****************************************************************************
uint8_t * g_pui8NFC_F_BufferPtr;

//*****************************************************************************
//
//  Sens SENSF_REQ (request)
//
//*****************************************************************************
void NFCTypeF_SendSENSF_REQ(void)
{
	uint8_t ui8NFC_F_Packet[6];
	//
	// Length
	//
	ui8NFC_F_Packet[0] = 0x06;
	//
	// Command
	//
	ui8NFC_F_Packet[1] = SENSF_REQ_CMD;

	ui8NFC_F_Packet[2] = 0xFF;		// System Code (SC) 7:0
	ui8NFC_F_Packet[3] = 0xFF;		// System Code (SC) 15:8

	ui8NFC_F_Packet[4] = 0x00;		// Request Code (RC)

	ui8NFC_F_Packet[5] = 0x03;		// Time Slot Number (TSN) (DP, Table 42, 4 time slots)
	TRF79x0WriteFIFO(ui8NFC_F_Packet,CRC_BIT_ENABLE,6);
}

//*****************************************************************************
//
// Send SENSF_RES (response)
//
//*****************************************************************************
void NFCTypeF_SendSENSF_RES(void)
{
	uint8_t ui8NFC_F_Packet[18];
	uint8_t ui8Offset = 0;
	uint8_t ui8Counter = 0;
	//
	// Length
	//
	ui8NFC_F_Packet[ui8Offset++] = 0x12;
	//
	// Command
	//
	ui8NFC_F_Packet[ui8Offset++] = SENSF_RES_CMD;

	for(ui8Counter = 0; ui8Counter < 8; ui8Counter++)
	{
		ui8NFC_F_Packet[ui8Offset++] = g_ui8NFCID2[ui8Counter];
	}

	// PAD 0
	ui8NFC_F_Packet[ui8Offset++] = 0xC0;
	ui8NFC_F_Packet[ui8Offset++] = 0xC1;
	// PAD 1
	ui8NFC_F_Packet[ui8Offset++] = 0xC2;
	ui8NFC_F_Packet[ui8Offset++] = 0xC3;
	ui8NFC_F_Packet[ui8Offset++] = 0xC4;
	// MRTI CHECK
	ui8NFC_F_Packet[ui8Offset++] = 0xC5;
	// MRTI UPDATE
	ui8NFC_F_Packet[ui8Offset++] = 0xC6;
	// PAD2
	ui8NFC_F_Packet[ui8Offset++] = 0xC7;

	TRF79x0WriteFIFO(ui8NFC_F_Packet,CRC_BIT_ENABLE,ui8Offset);
}
//*****************************************************************************
//
// Process data received in buffer
//
//*****************************************************************************
tStatus NFCTypeF_ProcessReceivedData(uint8_t * pui8RxBuffer)
{
	volatile uint8_t ui8CommandLength;
	uint8_t ui8Command;
	uint8_t ui8Counter;
	tStatus eNFCFStatus = STATUS_SUCCESS;

	ui8CommandLength = pui8RxBuffer[0];
	ui8Command = pui8RxBuffer[1];

//	//UARTprintf("NFC_F CMD: %d \n",ui8Command);

	if(ui8Command == SENSF_RES_CMD)
	{
		//
		// Store the g_ui8NFCID2
		//
		for(ui8Counter = 0; ui8Counter < 8; ui8Counter++)
		{
			g_ui8NFCID2[ui8Counter] = pui8RxBuffer[2+ui8Counter];
		}
	}
	else if(ui8Command == SENSF_REQ_CMD && ui8CommandLength == 0x06 )
	{
		if(pui8RxBuffer[2] == 0xFF && pui8RxBuffer[3] == 0xFF)
		{
			// Valid SENSF_REQ received - thus send a SENSF Response
			NFCTypeF_SendSENSF_RES();
		}
		else
			eNFCFStatus = STATUS_FAIL;
	}
	else
	{
		eNFCFStatus = STATUS_FAIL;
	}
	return eNFCFStatus;
}

//*****************************************************************************
//
// Return the NFCID
//
//*****************************************************************************
uint8_t * NFCTypeF_GetNFCID2(void)
{
	return g_ui8NFCID2;
}




