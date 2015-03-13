//*****************************************************************************
//
// nfc_dep.h - Defines for sending packets of P2P
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
#ifndef __NFC_DEP_H__
#define __NFC_DEP_H__
#include "types.h"

//*****************************************************************************
//
// List of Commands
//
//*****************************************************************************
//        REQUESTS			//
#define	ATR_REQ_CMD		0xD400
#define PSL_REQ_CMD		0xD404
#define DEP_REQ_CMD		0xD406
#define DSL_REQ_CMD		0xD408
#define RSL_REQ_CMD		0xD40A

//        RESPONSES			//
#define ATR_RES_CMD		0xD501
#define PSL_RES_CMD		0xD505
#define DEP_RES_CMD		0xD507
#define DSL_RES_CMD		0xD509
#define RSL_RES_CMD		0xD50B


#define		DIDi			0x00
#define		BSi				0x00
#define		BRi				0x00
//*****************************************************************************
//
// Initiator Maximum payload size + General bytes available (BIT1)
// B6 B5 - '00' Max Payload 64 bytes
// B6 B5 - '01' Max Payload 128 bytes
// B6 B5 - '10' Max Payload 192 bytes
// B6 B5 - '11' Max Payload 254 bytes (default)
//
//*****************************************************************************
#define		PPi				0x32

#define		DIDt			0x00
#define		BSt				0x00
#define		BRt				0x00
#define		TO				0x07
//*****************************************************************************
//
// Target Maximum payload size + General bytes available (BIT1)
// B6 B5 - '00' Max Payload 64 bytes
// B6 B5 - '01' Max Payload 128 bytes
// B6 B5 - '10' Max Payload 192 bytes
// B6 B5 - '11' Max Payload 254 bytes (default)
//
//*****************************************************************************
#define		PPt				0x32

//*****************************************************************************
//
//
//
//*****************************************************************************
typedef enum
{
	ACK_PDU	= 				0x40,
	INFORMATION_PDU = 		0x00,
	NACK_PDU =	 			0x50,
	ATN_PDU = 				0x80,
	RTOX_REQ_PDU =	 		0x90,

}tPDUBlock;

//*****************************************************************************
//
// Function Prototypes
//
//*****************************************************************************
void NFCDEP_SendATR_REQ(uint8_t * pui8NFCID2_Ptr);
void NFCDEP_SendPSL_REQ(void);
void NFCDEP_SendATR_RES(void);
void NFCDEP_SendRSL_RES(void);
void NFCDEP_SendPSL_RES(uint8_t did_value);

tStatus NFCDEP_ProcessReceivedRequest(uint8_t * rx_buffer ,uint8_t * pui8NFCID2_Ptr, bool bActiveResponse);
tStatus NFCDEP_ProcessReceivedData(uint8_t * rx_buffer);
void NFCDEP_SendDEP_REQ(uint8_t * rx_buffer);
void NFCDEP_SendDEP_RES(void);
void NFCDEP_SetBufferPtr(uint8_t * buffer_ptr);

#endif //__NFC_DEP_H__
