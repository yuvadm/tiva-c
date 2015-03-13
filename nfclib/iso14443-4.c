//*****************************************************************************
//
// iso14443-4.c - ISO 14443-4 implementation.
//
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

#include <string.h>

#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "trf7960.h"

//*****************************************************************************
//
// Transceive ISO 14443-4 RATS command.
//
//*****************************************************************************
int
ISO14443RATS(unsigned char ucFSDI, unsigned char ucCID, unsigned char *pucATS)
{
    unsigned char pucResponse[16];
    unsigned int uiRxSize;
    unsigned char pucRATS[2];
    int i;

    uiRxSize = sizeof(pucResponse);

    //
    // RATS command.
    //
    pucRATS[0] = 0xE0;
    pucRATS[1] = (ucFSDI << 4) || ucCID;

    //
    // Transmit RATS, receive ATS.
    //
    TRF7960Transceive(pucRATS, sizeof(pucRATS), 0, pucResponse, &uiRxSize, NULL,
                      TRF7960_TRANSCEIVE_CRC);

    if(uiRxSize >= 3)
    {
        //
        // Valid ATS received, return it as an char buffer. Was transmitted LSByte first.
    	// pucResponse[0] is the length of the transmitted ATS, including TL byte, NOT including
    	// two CRC bytes
        //
		for(i = 0; i < pucResponse[0]; i++)
			pucATS[i] = pucResponse[i];

        return(uiRxSize);
    }
    else
    {
    	return(0);
    }
}

//*****************************************************************************
//
// Transceive ISO 14443-4 PPS command.
// ucCID must between 0~14, ucDRI & ucDSI must between 0~3
//
//*****************************************************************************
int
ISO14443PPS(unsigned char ucCID, unsigned char ucDRI, unsigned char ucDSI)
{
    unsigned char pucResponse[3];
    unsigned int uiRxSize;
    unsigned char pucPPS[3];

    uiRxSize = sizeof(pucResponse);

    //
    // PPS command.
    //
    pucPPS[0] = (0xD << 4) | ucCID;
    pucPPS[1] = 0x11;					// PPS1 is transmitted
    pucPPS[2] = (ucDSI << 2) | ucDRI;

    //
    // Transmit PPS, receive PPS response.
    //
    TRF7960Transceive(pucPPS, sizeof(pucPPS), 0, pucResponse, &uiRxSize, NULL,
                      TRF7960_TRANSCEIVE_CRC);

	//
	// check if receive the first byte of the response is PPSS
	//
	if(pucResponse[0] == pucPPS[0])
		return(1);
	else
	{
		//
		// Not valid response
		//
		return(0);
	}
}

//*****************************************************************************
//
// Transceive ISO 14443-4 DESELECT command.
// ucCID must between 0~14, ucDRI & ucDSI must between 0~3
//
//*****************************************************************************
int
ISO14443DESELECT(unsigned char ucCID)
{
    unsigned char pucResponse[2];
    unsigned int uiRxSize;
    unsigned char pucDESELECT[2];

    uiRxSize = sizeof(pucResponse);

    pucResponse[0] = 0;
    pucResponse[1] = 0;

    //
    // DESELECT command.
    //
    pucDESELECT[0] = 0xCA;				// S-block with DESELECT set and CID following
    pucDESELECT[1] = ucCID & 0x0F;

    //
    // Transmit DESELECT, receive DESELECT response.
    //
    TRF7960Transceive(pucDESELECT, sizeof(pucDESELECT), 0, pucResponse, &uiRxSize, NULL,
                      TRF7960_TRANSCEIVE_CRC);

	//
	// check if receive the first byte of the response is DESELECT
	// check if the second byte contains the same CID
	//
	if(pucResponse[0] == pucDESELECT[0])
	{
		return(1);
	}
	else
	{
		//
		// Not valid response
		//
		return(0);
	}
}
