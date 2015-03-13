//*****************************************************************************
//
// iso14443B.c - ISO 14443B implementation.
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
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "trf79x0.h"
#include "iso14443b.h"

//*****************************************************************************
//
// Set up registers for ISO 14443 B 106Kbit/s operation.  This function must
// be called after initializing the TRF79x0 (for example with TRF79x0Init()
// or TRF79x0Command() with argument \b TRF79X0_SOFT_INIT_CMD) and before
// calling any of the other ISO14443B functions.
//
//*****************************************************************************
void
ISO14443BSetupRegisters(void)
{
    //
    // Set the ISO format to ISO1443B 106Kbps.
    //
    TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG,
                         TRF79X0_ISO_CONTROL_14443B_106K);

    //
    // Set the TX pulse to 106ns (0x20 * 73.7ns).
    //
    TRF79x0WriteRegister(TRF79X0_TX_PULSE_LENGTH_CTRL_REG, 0x20);

    //
    // Set the RX No response wait time to 529us (0xe * 37.76us).
    //
    TRF79x0WriteRegister(TRF79X0_RX_NO_RESPONSE_WAIT_REG, 0x0e);

    //
    // Set the RX wait time to 66us (7 * 9.44us).
    //
    TRF79x0WriteRegister(TRF79X0_RX_WAIT_TIME_REG, 0x07);

    //
    // Set the SYSCLK to 6.78MHz and the Modulation 10% ASK.
    //
    TRF79x0WriteRegister(TRF79X0_MODULATOR_CONTROL_REG, TRF79X0_MOD_CTRL_SYS_CLK_6_78MHZ);

    //
    // Configure the Special Settings Register.
    //
    TRF79x0WriteRegister(TRF79X0_RX_SPECIAL_SETTINGS_REG,
       (TRF79x0ReadRegister(TRF79X0_RX_SPECIAL_SETTINGS_REG) & 0x0f) |
        TRF79X0_RX_SP_SET_M848);

    //
    // Configure the Test Settings Register.
    //
    TRF79x0WriteRegister(TRF79X0_TEST_SETTING1_REG, 0x20);


    //
    // Set the regulator voltage to be automatic.
    //
    TRF79x0WriteRegister(TRF79X0_REGULATOR_CONTROL_REG,
                         TRF79X0_REGULATOR_CTRL_AUTO_REG);
}

//*****************************************************************************
//
// Power on the field and wait for a time that is long enough to guarantee
// that all cards in the field will be initialized.
//
//*****************************************************************************
void
ISO14443BPowerOn(void)
{
    unsigned char ucReg;

    //
    // Enable RF field and receiver.
    //
    ucReg = TRF79x0ReadRegister(TRF79X0_CHIP_STATUS_CTRL_REG);
    TRF79x0WriteRegister(TRF79X0_CHIP_STATUS_CTRL_REG,
                         ucReg | TRF79X0_STATUS_CTRL_RF_ON);

    //
    // Wait 5ms (as per ISO 14443-3 clause 5).
    //
    SysCtlDelay(((SysCtlClockGet() / 3) * 5) / 1000);
}

//*****************************************************************************
//
// Power off the field and wait for some time.
//
//*****************************************************************************
void
ISO14443BPowerOff(void)
{
    unsigned char ucReg;

    //
    // Disable RF field and receiver.
    //
    ucReg = TRF79x0ReadRegister(TRF79X0_CHIP_STATUS_CTRL_REG);

    TRF79x0WriteRegister(TRF79X0_CHIP_STATUS_CTRL_REG,
                         ucReg & ~TRF79X0_STATUS_CTRL_RF_ON);

    //
    // Wait 5ms.
    //
    SysCtlDelay(((SysCtlClockGet() / 3) * 5) / 1000);
}

int
ISO14443BHalt(unsigned char *pucPUPI)
{
    unsigned char ucResponse;
    unsigned int uiRxSize;

    //
    // HLTA command.
    //
    unsigned char pucHLTA[5];

    pucHLTA[0] = 0x50;
    pucHLTA[1] = pucPUPI[0];
    pucHLTA[2] = pucPUPI[1];
    pucHLTA[3] = pucPUPI[2];
    pucHLTA[4] = pucPUPI[3];

    TRF79x0Transceive(pucHLTA, sizeof(pucHLTA), 0, &ucResponse, &uiRxSize, NULL,
                      TRF79X0_TRANSCEIVE_CRC);

	//
	// Valid answer to HLTB received.
    if((uiRxSize == 1) && (ucResponse == 0))
    	return(uiRxSize);
    else
    	return(0);
}

//*****************************************************************************
//
// Transceive ISO 14443-B SlotMARKER command.
//
// \param ucSlot is the slot number for the following operations, must between 1~15
//
//*****************************************************************************
void
ISO14443BSlotMARKER(unsigned char ucSlot)
{
    unsigned char ucAPn;

    ucAPn = (ucSlot << 4) | 0x05;

    TRF79x0Transceive(&ucAPn, 1, 0, 0, 0, 0, TRF79X0_TRANSCEIVE_CRC);
//    TRF79x0Send(&ucAPn, 1, 0, TRF79X0_TRANSCEIVE_TX_CRC);
}

//*****************************************************************************
//
// Transceive ISO 14443-B REQB type command.
//
// \param ucCmd is the command, either \b ISO14443B_REQB or \b ISO14443B_WUPB
// \param ucAFI is the application family identifier
// \param ucN is the number of the time slot that was used in anticollision process,
//        must between 0 ~ 4
// \param piATQB is a pointer to an integer to store the received ATQB and
//  will be set to -1 if a collision occurred.
//
// \return true if at least one card responded and false
// otherwise.
//
// \note User code usually does not need to call this function since it is
// implicitly called in ISO14443BSelect(), ISO14443bSelectFirst() or
// ISO14443BSelectNext().
//
//*****************************************************************************
int
ISO14443BREQB(unsigned char ucCmd, unsigned char ucAFI, unsigned char ucN,
		      unsigned char *pucATQB, unsigned int *puiATQBSize)
{
    unsigned char pucREQB[3];
    unsigned char pucResponse[12];
    unsigned int uiRxSize;
    int i, slotTotal, slot;

    switch(ucN)
    {
    	case 0:		slotTotal = 1;		break;
    	case 1:		slotTotal = 2;		break;
    	case 2:		slotTotal = 4;		break;
    	case 3:		slotTotal = 8;		break;
    	case 4:		slotTotal = 16;		break;
    	default:	slotTotal = 1;		break;
    }

    uiRxSize = sizeof(pucResponse);
    pucResponse[0] = 0;

    //
    // Transmit WUPB/REQB, receive ATQB.
    //
    pucREQB[0] = 0x05;
    pucREQB[1] = ucAFI;
    pucREQB[2] = ucCmd | ucN;

    //
    // the first byte of ATQB is 0x50
    //
	TRF79x0Transceive(pucREQB, sizeof(pucREQB), 0, pucResponse, &uiRxSize, 0,
					 TRF79X0_TRANSCEIVE_CRC);

	//
	// check if needing to scan slot
	//
	slot = 1;
	while((pucResponse[0] != 0x50) && (slot < slotTotal))
	{
	    uiRxSize = 12;
	    pucResponse[0] = 0;

	    TRF79x0IRQClearCauses(TRF79X0_WAIT_RXEND);
	    //
	    // the order of the two function must not reversed, because the TRF79x0ReceiveAgain() called
	    // TRF79x0Command(TRF79X0_RESET_FIFO_CMD); to reset receive FIFO
	    // the most important action TRF79x0ReceiveAgain() done is to set g_sRXState.uiMaxLength as uiRxSize
	    // in order to enable the TRF7970A interrupt to continue receive data
	    //
		ISO14443BSlotMARKER(slot++);
		TRF79x0ReceiveAgain(pucResponse, &uiRxSize);
	};

	//
	// Valid ATQB received. Was transmitted LSByte first.
	//
    if(pucResponse[0] == 0x50)
    {
		if(pucATQB != NULL)
		{
			for(i = 0; i < uiRxSize; i++)
				pucATQB[i] = pucResponse[i];
		}
		*puiATQBSize = uiRxSize;

		//
		// Return true
		//
		return(1);
    }
    else
    {
		//
		// No response at all
		//
		return(0);
    }
}

//*****************************************************************************
//
// Transceive ISO 14443-3 ATTRIB command.
// EOF_SOF indicate  the  PCD  capability  to  support  suppression  of  the  EOF  and/or  SOF
// from  PICC  to  PCD, which may reduce communication overhead.
// The suppression of EOF and/or SOF is optional for the PICC.
// SOF/EOF  suppression  applies  only  for  communications  at  fc / 128  (~  106  kbit/s).
// For  bit  rates  higher  than fc / 128 (~ 106 kbit/s) the PICC shall always provide SOF and EOF
// EOF_SOF set 0 indicate SOF&EOF required.
//
// ucTR1 & ucTR0 must between 0~2
//
//*****************************************************************************
int
ISO14443BATTRIB(unsigned char *pucPUPI, unsigned char ucTR0, unsigned char ucTR1, unsigned char ucEOF_SOF,
				unsigned char ucMaxFrameSize, unsigned char ucBitRateD2C, unsigned char ucBitRateC2D,
				unsigned char ucProtocolType, unsigned char ucCID, unsigned char *A2ATTRIB)
{
    unsigned char pucResponse[3];
    unsigned int uiRxSize;
    unsigned char pucATTRIB[9];

    uiRxSize = sizeof(pucResponse);

    //
    // ATTRIB command.
    //
    pucATTRIB[0] = 0x1D;
    pucATTRIB[1] = pucPUPI[0];
    pucATTRIB[2] = pucPUPI[1];
    pucATTRIB[3] = pucPUPI[2];
    pucATTRIB[4] = pucPUPI[3];
    pucATTRIB[5] = (ucTR0 << 6) | (ucTR1 << 4) | (ucEOF_SOF << 3) | (ucEOF_SOF << 2);
    pucATTRIB[6] =  (ucBitRateC2D << 6) | (ucBitRateD2C << 4) | ucMaxFrameSize;
    pucATTRIB[7] = ucProtocolType & 0x0F;
    pucATTRIB[8] = ucCID & 0x0F;

    //
    // Transmit ATTRIB without higher layer INF, receive answer to ATTRIB command without higher layer response
    //
    TRF79x0Transceive(pucATTRIB, sizeof(pucATTRIB), 0, pucResponse, &uiRxSize, NULL,
                      TRF79X0_TRANSCEIVE_CRC);

    if(uiRxSize == 1 )
    {
        //
        // Valid answer to ATTRIB command received, return it as an char, uiRxSize NOT including two CRC bytes
        //
        if(A2ATTRIB != NULL)
        	*A2ATTRIB = pucResponse[0];

        return(uiRxSize);
    }
    else
    {
    	return(0);
    }
}
