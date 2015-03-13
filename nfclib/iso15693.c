//*****************************************************************************
//
// iso15693.c - The top level API used to communicate with ISO15063 cards.
//
// Copyright (c) 2010-2014 Texas Instruments Incorporated.  All rights reserved.
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
//*****************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "trf79x0.h"
#include "nfclib/iso15693.h"

extern struct
{
    //
    // The actual string of bytes in the UID.
    //
    unsigned char pucUID[UID_SIZE];

    //
    // The number of valid bytes in the pucUID variable.
    //
    unsigned long ulUIDSize;

    //
    // The ASCII string that is used to display the UID on the screen.
    //
    char pcUIDStr[CARD_LABEL_SIZE];

    unsigned char ucSlot;
}g_sCard_15693[16];

//*****************************************************************************
//
// Command/Response and transmit/receive buffer.
//
//*****************************************************************************
static unsigned char g_pucCmd[16];

//*****************************************************************************
//
// The value that is written to the block if the "Erase" button is pressed.
// This will invalidate the block.
//
//*****************************************************************************
static const unsigned char g_pucValueEmpty[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static unsigned char ucCardFound = 0;

//*****************************************************************************
//
// Set up registers for ISO 15693 operation.  This function must
// be called after initializing the TRF79x0 (for example with TRF79x0Init()
// or TRF79x0Command() with argument \b TRF79X0_SOFT_INIT_CMD) and before
// calling any of the other ISO15963 functions.
//
//*****************************************************************************
void
ISO15693SetupRegisters(void)
{
	// actually, we can just use the default setting
#if 0
    //
    // Set the TX pulse to 9.44us (0x80 * 73.7ns).
    //
    TRF79x0WriteRegister(TRF79X0_TX_PULSE_LENGTH_CTRL_REG, 0x80);

    //
    // Set the RX No response wait time to 529us (0xe * 37.76us).
    //
    TRF79x0WriteRegister(TRF79X0_RX_NO_RESPONSE_WAIT_REG, 0x0e);

    //
    // Set the RX wait time to 293us (0x20 * 9.44us).
    //
    TRF79x0WriteRegister(TRF79X0_RX_WAIT_TIME_REG, 0x20);

    //
    // Configure the Special Settings Register.
    //
    TRF79x0WriteRegister(TRF79X0_RX_SPECIAL_SETTINGS_REG,
       (TRF79x0ReadRegister(TRF79X0_RX_SPECIAL_SETTINGS_REG) & 0x0f) |
       TRF79X0_RX_SP_SET_C424);

    //
    // Configure the Test Settings Register.
    //
    TRF79x0WriteRegister(TRF79X0_TEST_SETTING1_REG, 0x20);
#endif

    //
    // Set the SYSCLK to 6.78MHz and the Modulation Depth to OOK.
    //
    TRF79x0WriteRegister(TRF79X0_MODULATOR_CONTROL_REG,
                         (TRF79X0_MOD_CTRL_SYS_CLK_6_78MHZ |
                          TRF79X0_MOD_CTRL_MOD_ASK_10));

    //
    // Set the regulator voltage to be automatic.
    //
    TRF79x0WriteRegister(TRF79X0_REGULATOR_CONTROL_REG,
                         TRF79X0_REGULATOR_CTRL_AUTO_REG);

    //
    // Set the regulator voltage to be automatic.
    //
    TRF79x0WriteRegister(TRF79X0_CHIP_STATUS_CTRL_REG,
    					 TRF79X0_STATUS_CTRL_RF_ON | TRF79X0_STATUS_CTRL_RF_PWR_FULL
    					 | TRF79X0_STATUS_CTRL_5V_OPERATION);

    //
    // Set the ISO format to ISO15693 high bit rate, 26.48 kbps, one subcarrier, 1 out of 4
    //
    TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG,
    		TRF79X0_ISO_CONTROL_15693_HIGH_1SUB_1OUT4);
}

//*****************************************************************************
//
//! Initializes the ISO15693 utility functions.
//!
//! This function prepares the ISO1593 utility functions so that they are
//! prepared for the remaining ISO1593 calls.  This function must be called once
//! before calling any other ISO1593 functions.
//!
//! \return None.
//
//*****************************************************************************
void
ISO15693Init(void)
{
    //
    // Initialize RFID hardware.
    //
    TRF79x0Init();

    //
    // Set up ISO 15693 operation.
    //
    ISO15693SetupRegisters();
}

void
ISO15693NextSlot(void)
{
    TRF79x0Command(TRF79X0_STOP_DECODERS_CMD);
    TRF79x0Command(TRF79X0_RUN_DECODERS_CMD);
    TRF79x0Command(TRF79X0_RESET_FIFO_CMD);
    TRF79x0Command(TRF79X0_TRANSMIT_NEXT_SLOT_CMD);
}

//
//	\ucSubCarrier,
//	0		A single sub-carrier frequency shall be used by the VICC
//	1		Two sub-carriers shall be used by the VICC
//	\ucDataRate
//	0        Low data rate shall be used
//	1        High data rate shall be used
//	\ucNbSlots
//	0        16 slots
//	1        1 slot
//
int
ISO15693InventoryAFI(unsigned char ucSubCarrier, unsigned char ucDataRate,
            	  unsigned char ucAfi, unsigned char ucNbSlots,
            	  unsigned char *pucMask, unsigned char ucMaskLen)
{
    unsigned char pucResponse[10];
    unsigned int uiRxSize, i, slot;

    uiRxSize = 10;

    //
    // Prepare Inventory command.
    //
	//	b5 	AFI_flag
	//	0          AFI Field is not present
	//	1          AFI Field is present
    g_pucCmd[0] = (ucNbSlots << 5) | (0x1 << 4) | (0x1 << 2) | (ucDataRate << 1) | ucSubCarrier;
    g_pucCmd[1] = 0x01;		// Command Code = 0x01 ---> Inventory
    g_pucCmd[2] = ucAfi;
    g_pucCmd[3] = ucMaskLen;

    //
    // Transmit Inventory, receive response
    //
    TRF79x0Transceive(g_pucCmd, 4, 0, pucResponse, &uiRxSize, 0, TRF79X0_TRANSCEIVE_CRC);

	//
	// check if needing to scan slot
	//
    slot = 1;
	while((uiRxSize != 10) && (slot < 16))
	{
	    uiRxSize = sizeof(pucResponse);

	    TRF79x0IRQClearCauses(TRF79X0_WAIT_RXEND);
	    //
	    // the order of the two function must not reversed, because the TRF79x0ReceiveAgain() called
	    // TRF79x0Command(TRF79X0_RESET_FIFO_CMD); to reset receive FIFO
	    // the most important action TRF79x0ReceiveAgain() done is to set g_sRXState.uiMaxLength as uiRxSize
	    // in order to enable the TRF7970A interrupt to continue receive data
	    //
		ISO15693NextSlot();
		TRF79x0ReceiveAgain(pucResponse, &uiRxSize);
		slot++;
	};

    if(uiRxSize == 10 )
    {
        //
        // Valid answer to Inventory command received, return it as an char, uiRxSize NOT including two CRC bytes
        // the first 2 byte in pucResponse is Flags & DSFI, the last 8 bytes is UID
		//
        if(pucMask != NULL)
        {
        	for(i = 0; i < 8; i++)
        		pucMask[i] = pucResponse[2 + i];
        }
        return(uiRxSize);
    }
    else
    {
    	return(0);
    }
}


//
//	\ucSubCarrier,
//	0		A single sub-carrier frequency shall be used by the VICC
//	1		Two sub-carriers shall be used by the VICC
//	\ucDataRate
//	0        Low data rate shall be used
//	1        High data rate shall be used
//	\ucNbSlots
//	0        16 slots
//	1        1 slot
//
int
ISO15693Inventory(unsigned char ucSubCarrier, unsigned char ucDataRate,
            	  unsigned char ucNbSlots, unsigned char *pucMask, unsigned char ucMaskLen)
{
    unsigned char pucResponse[10];
    unsigned char uiRxSize, i, slot;

    uiRxSize = sizeof(pucResponse);

    //
    // Prepare Inventory command.
    //
	//	b5 	AFI_flag
	//	0          AFI Field is not present
	//	1          AFI Field is present
    g_pucCmd[0] = (ucNbSlots << 5) | (0x1 << 2) | (ucDataRate << 1) | ucSubCarrier;
    g_pucCmd[1] = 0x01;		// Command Code = 0x01 ---> Inventory
    g_pucCmd[3] = ucMaskLen;

    //
    // Transmit Inventory, receive response
    //
    TRF79x0Transceive(g_pucCmd, 3, 0, pucResponse, &uiRxSize, 0, TRF79X0_TRANSCEIVE_CRC);

	//
	// check if needing to scan slot
	//
    slot = 1;
	while((uiRxSize != 10) && (slot < 16))
	{
	    uiRxSize = sizeof(pucResponse);

	    TRF79x0IRQClearCauses(TRF79X0_WAIT_RXEND);
	    //
	    // the order of the two function must not reversed, because the TRF79x0ReceiveAgain() called
	    // TRF79x0Command(TRF79X0_RESET_FIFO_CMD); to reset receive FIFO
	    // the most important action TRF79x0ReceiveAgain() done is to set g_sRXState.uiMaxLength as uiRxSize
	    // in order to enable the TRF7970A interrupt to continue receive data
	    //
		ISO15693NextSlot();
		TRF79x0ReceiveAgain(pucResponse, &uiRxSize);
		slot++;
	};

    if(uiRxSize == 10 )
    {
        //
        // Valid answer to Inventory command received, return it as an char, uiRxSize NOT including two CRC bytes
        //
        if(pucMask != NULL)
        {
        	for(i = 0; i < uiRxSize; i++)
        		pucMask[i] = pucResponse[i];
        }
        return(uiRxSize);
    }
    else
    {
    	return(0);
    }
}

int
ISO15693Anticollision16Slots(unsigned char ucSubCarrier, unsigned char ucDataRate,
            	  	  	  	 unsigned char *pucMask, unsigned char ucMaskLen)
{
    unsigned char pucResponse[10], ucMaskNew[8];
    unsigned int uiRxSize, uiTxSize, i, slot = 0;
    unsigned int uiFlagCollision = 0, uiSlotCollision = 0;

    uiRxSize = sizeof(pucResponse);

    //
    // Prepare Inventory command.
    //
	//	b5 	AFI_flag
	//	0          AFI Field is not present
	//	1          AFI Field is present
    g_pucCmd[0] = (0x1 << 2) | (ucDataRate << 1) | ucSubCarrier;
    g_pucCmd[1] = 0x01;		// Command Code = 0x01 ---> Inventory
    g_pucCmd[2] = ucMaskLen;

	uiTxSize = 3 + (((ucMaskLen >> 2) + 1) >> 1);

    if(uiTxSize > 3)
    {
    	for(i = 0; i < (uiTxSize - 3); i++)
    		g_pucCmd[3 + i] = pucMask[i];
    }

    //
    // Transmit Inventory, receive response
    //
    TRF79x0Transceive(g_pucCmd, uiTxSize, 0, pucResponse, &uiRxSize, 0, TRF79X0_TRANSCEIVE_CRC);

	//
	// check if needing to scan slot
	//
	while(slot < 16)
	{
	    if(TRF79x0IsCollision() == 1)
	    {
	    	uiFlagCollision = 1;
	    	uiSlotCollision = slot -1;
	    }
	    else if(uiRxSize == 10)
	    {
	    	for(i = 0; i < 8; i++)
	    		g_sCard_15693[ucCardFound + 1].pucUID[i] = pucResponse[2 + i];
	    	g_sCard_15693[ucCardFound + 1].ucSlot = slot;
	    	ucCardFound++;
	    }

	    uiRxSize = sizeof(pucResponse);

	    TRF79x0IRQClearCauses(TRF79X0_WAIT_RXEND);

	    //
	    // the order of the two function must not reversed, because the TRF79x0ReceiveAgain() called
	    // TRF79x0Command(TRF79X0_RESET_FIFO_CMD); to reset receive FIFO
	    // the most important action TRF79x0ReceiveAgain() done is to set g_sRXState.uiMaxLength as uiRxSize
	    // in order to enable the TRF7970A interrupt to continue receive data
	    //
		ISO15693NextSlot();
		TRF79x0ReceiveAgain(pucResponse, &uiRxSize);
		slot++;
	};

	//
	// only do ones cascade
	// TODO: NEED refer to the msp430 version to make more cascade anticollision function
	//
	if(uiFlagCollision && (ucMaskLen < 4))
	{
		uiFlagCollision = 0;
		ucMaskNew[0] = uiSlotCollision;
		ISO15693Anticollision16Slots(0, 1, &ucMaskNew[0], ucMaskLen + 4);
	}

    if(ucCardFound)
    {
    	ucCardFound = 0;
        return(1);
    }
    else
    {
    	return(0);
    }
}

int
ISO15693StayQuiet(unsigned char  *pucUID)
{
	int i;

    //
    // Prepare Stay Quiet command.
    //
	//	b6 	Address_flag		the bit sequence start from b1 not b0!
	//	1          Request is addressed. UID field is included. It shall be	executed only
	//			   by the VICC whose UID matches the UID specified in the request.
    g_pucCmd[0] = (1 << 5) | (1 << 1);
    // Command Code = 0x02 ---> Stay Quiet
    g_pucCmd[1] = 0x02;
    for(i = 0; i < 8; i++)
    	g_pucCmd[2 + i] = pucUID[i];

    //
    // Transmit Stay Quiet command, receive response
    //
    TRF79x0Transceive(g_pucCmd, 10, 0, 0, 0, 0, TRF79X0_TRANSCEIVE_TX_CRC);
}

//*****************************************************************************
//
// ! Reads a single block data from the selected card.
// !
// ! \param uiBlock is the address of the block to read.
// ! \param pucBuf is the output buffer to store the raw block contents into.
// ! This buffer must be able to store at least 32 bytes.
// !
// ! This function reads a ISO15693 block and returns the full contents
// ! of the block with no interpretation of the bytes. The function will
// ! return the number of valid bytes stored in the \e pucBuf parameter.
// !
//
//*****************************************************************************
int
BlockReadSingleUID(unsigned char  *pucUID, unsigned int uiBlock, unsigned char *pucBuf)
{
    unsigned char pucCmd[11];
    unsigned int uiRxBytes;
    unsigned int uiRxBits;
    int i;

    //
    // Reading 32 bytes and 0 bits.
    //
    uiRxBytes = 32;
    uiRxBits = 0;

    //
    // Prepare Read Single Block command.
    //
	//	b6 	Address_flag		the bit sequence start from b1 not b0!
	//	1          Request is addressed. UID field is included. It shall be	executed only
	//			   by the VICC whose UID matches the UID specified in the request.
    // b7	Option_flag
    // 1			Meaning is defined by the command description
    pucCmd[0] = (1 << 6) |(1 << 5) | (1 << 1);
    // Command Code = 0x20 ---> Read Single Block
    pucCmd[1] = 0x20;
    for(i = 0; i < 8; i++)
    	pucCmd[2 + i] = pucUID[i];
    pucCmd[10] = uiBlock;

    //
    // Transmit Read Single Block, receive response
    //
    TRF79x0Transceive(pucCmd, sizeof(pucCmd), 0, pucBuf, &uiRxBytes, &uiRxBits, TRF79X0_TRANSCEIVE_CRC);
    if(uiRxBytes == 0)
    {
        return(0);
    }

    return(uiRxBytes);
}

int
BlockReadSingle(unsigned int uiBlock, unsigned char *pucBuf)
{
    unsigned char pucCmd[3];
    unsigned int uiRxBytes;
    unsigned int uiRxBits;
    int i;

    //
    // Reading 32 bytes and 0 bits.
    //
    uiRxBytes = 32;
    uiRxBits = 0;

    //
    // Prepare Read Single Block command.
    //
    // b7	Option_flag
    // 1			Meaning is defined by the command description
    pucCmd[0] = (1 << 6) | (1 << 1);
    // Command Code = 0x20 ---> Read Single Block
    pucCmd[1] = 0x20;
    pucCmd[2] = uiBlock;

    //
    // Transmit Read Single Block, receive response
    //
    TRF79x0Transceive(pucCmd, sizeof(pucCmd), 0, pucBuf, &uiRxBytes, &uiRxBits, TRF79X0_TRANSCEIVE_CRC);
    if(uiRxBytes == 0)
    {
        return(0);
    }

    return(uiRxBytes);
}
//*****************************************************************************
//
// ! Write a single block data to the selected card.
// !
// ! \param uiBlock is the address of the block to write.
// ! \param pucBuf is the input buffer to store the raw block contents into.
// ! This buffer must be able to store at least 32 bytes.
// !
// ! This function write a ISO15693 block
// !
//
//*****************************************************************************
int
BlockWriteSingleUID(unsigned char  *pucUID, unsigned int uiBlock, unsigned char ucValueLen, unsigned char *pucBuf)
{
    unsigned char pucCmd[43];
    unsigned char pucResponse[2];
    unsigned int uiRxBytes;
    int i;

    //
    // transmit  bytes as most
    //
    uiRxBytes = 2;

    //
    // Prepare Write Single Block command.
    //
	//	b6 	Address_flag		the bit sequence start from b1 not b0!
	//	1          Request is addressed. UID field is included. It shall be	executed only
	//			   by the VICC whose UID matches the UID specified in the request.

    // b7	Option_flag			must be set for Write & Lock command
    // 1			Meaning is defined by the command description
    pucCmd[0] = (1 << 6) | (1 << 5) | (1 << 1);
    // Command Code = 0x21 ---> Write Single Block
    pucCmd[1] = 0x21;
    for(i = 0; i < 8; i++)
    	pucCmd[2 + i] = pucUID[i];
    pucCmd[10] = uiBlock;
    for(i = 0; i < ucValueLen; i++)
    	pucCmd[11 + i] = pucBuf[i];

    //
    // Transmit Read Single Block, receive response
    //
    TRF79x0TransceiveISO15693(pucCmd, 11 + ucValueLen, 0, pucResponse, &uiRxBytes, 0, TRF79X0_TRANSCEIVE_CRC);
    if(uiRxBytes == 0)
    {
        return(0);
    }

    return(uiRxBytes);
}

int
BlockWriteSingle(unsigned int uiBlock, unsigned char ucValueLen, unsigned char *pucBuf)
{
    unsigned char pucCmd[7];
    unsigned char pucResponse[2];
    unsigned int uiRxBytes;
    int i;

    //
    // transmit  bytes as most
    //
    uiRxBytes = 2;

    //
    // Prepare Write Single Block command.
    //
	//	b6 	Address_flag		the bit sequence start from b1 not b0!
	//	1          Request is addressed. UID field is included. It shall be	executed only
	//			   by the VICC whose UID matches the UID specified in the request.

    // b7	Option_flag			must be set for Write & Lock command
    // 1			Meaning is defined by the command description
    pucCmd[0] = (1 << 6) | (1 << 1);
    // Command Code = 0x21 ---> Write Single Block
    pucCmd[1] = 0x21;
    pucCmd[2] = uiBlock;
    for(i = 0; i < ucValueLen; i++)
    	pucCmd[3 + i] = pucBuf[i];

    //
    // Transmit Read Single Block, receive response
    //
    TRF79x0TransceiveISO15693(pucCmd, 3 + ucValueLen, 0, pucResponse, &uiRxBytes, 0, TRF79X0_TRANSCEIVE_CRC);
    if(uiRxBytes == 0)
    {
        return(0);
    }

    return(uiRxBytes);
}

int
BlockLockSingleUID(unsigned char  *pucUID, unsigned int uiBlock, unsigned char *pucResponse)
{
    unsigned char pucCmd[11];
    unsigned int uiRxBytes;
    unsigned int uiRxBits;
    int i;

    //
    // Reading 32 bytes and 0 bits.
    //
    uiRxBytes = 2;
    uiRxBits = 0;

    //
    // Prepare Read Single Block command.
    //
	//	b6 	Address_flag		the bit sequence start from b1 not b0!
	//	1          Request is addressed. UID field is included. It shall be	executed only
	//			   by the VICC whose UID matches the UID specified in the request.
    // b7	Option_flag			must be set for Write & Lock command
    // 1			Meaning is defined by the command description
    pucCmd[0] = (1 << 6) | (1 << 5) | (1 << 1);
    // Command Code = 0x22 ---> Lock Single Block
    pucCmd[1] = 0x22;
    for(i = 0; i < 8; i++)
    	pucCmd[2 + i] = pucUID[i];
    pucCmd[10] = uiBlock;

    //
    // Transmit Read Single Block, receive response
    //
    TRF79x0TransceiveISO15693(pucCmd, sizeof(pucCmd), 0, pucResponse, &uiRxBytes, &uiRxBits, TRF79X0_TRANSCEIVE_CRC);
    if(uiRxBytes == 0)
    {
        return(0);
    }

    return(uiRxBytes);
}

int
BlockLockSingle(unsigned int uiBlock, unsigned char *pucResponse)
{
    unsigned char pucCmd[3];
    unsigned int uiRxBytes;
    unsigned int uiRxBits;
    int i;

    //
    // Reading 32 bytes and 0 bits.
    //
    uiRxBytes = 2;
    uiRxBits = 0;

    //
    // Prepare Read Single Block command.
    //

    pucCmd[0] = (1 << 6)  | (1 << 1);
    // Command Code = 0x22 ---> Lock Single Block
    pucCmd[1] = 0x22;
    pucCmd[2] = uiBlock;

    //
    // Transmit Read Single Block, receive response
    //
    TRF79x0TransceiveISO15693(pucCmd, sizeof(pucCmd), 0, pucResponse, &uiRxBytes, &uiRxBits, TRF79X0_TRANSCEIVE_CRC);
    if(uiRxBytes == 0)
    {
        return(0);
    }

    return(uiRxBytes);
}
