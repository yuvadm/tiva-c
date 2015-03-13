//*****************************************************************************
//
// nfc.c - NFC implementation.
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
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "trf79x0.h"
#include "iso14443b.h"

unsigned char g_ucNFCID[11] = "\x80\x12\x34\x56"; //NFC ID (PUPI = 80123456)

//*****************************************************************************
//
// Set up registers for ISO 14443 B 106Kbit/s operation.  This function must
// be called after initializing the TRF79x0 (for example with TRF79x0Init()
// or TRF79x0DirectCommand() with argument \b TRF79X0_SOFT_INIT_CMD) and before
// calling any of the other ISO14443B functions.
//
//*****************************************************************************
void
NfcTagType4BSetupRegisters(void)
{
    TRF79x0DirectCommand(TRF79X0_SOFT_INIT_CMD);
    TRF79x0DirectCommand(TRF79X0_IDLE_CMD);

    //
    // TODO:check why have to set as this?
    //
    TRF79x0WriteRegister(TRF79X0_MODULATOR_CONTROL_REG,
    					TRF79X0_MOD_CTRL_MOD_OOK_100);

    //
    // Set the ISO format to NFC Card Emulation, Type B
    //
    TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x25);

    //
    // Set the regulator voltage to be automatic.
    //
    TRF79x0WriteRegister(TRF79X0_REGULATOR_CONTROL_REG,
    					TRF79X0_REGULATOR_CTRL_VRS_2_8V);

    //
    // RX Special Settings for ISO14443B
    //
    TRF79x0WriteRegister(TRF79X0_RX_SPECIAL_SETTINGS_REG, 0x3C);

    //
    // Set the Target Detection Level to Max; use SDD
    //
//    TRF79x0WriteRegister(TRF79X0_NFC_TARGET_LEVEL_REG, 0x27);
        TRF79x0WriteRegister(TRF79X0_NFC_TARGET_LEVEL_REG, 0x07);

    //
    // Set the NFCID to be sent during SDD
    //
    TRF79x0WriteRegisterContinuous(TRF79X0_NFC_ID_REG, g_ucNFCID, 4);

    TRF79x0WriteRegister(TRF79X0_NFC_LO_FIELD_LEVEL_REG, 0x03);

    //
    // ISO14443B TX Options
    //
    TRF79x0WriteRegister(TRF79X0_ISO14443B_OPTIONS_REG, 0x00);

    TRF79x0WriteRegister(TRF79X0_CHIP_STATUS_CTRL_REG, 0x21);

    TRF79x0ResetFifoCommand();
    TRF79x0DirectCommand(TRF79X0_STOP_DECODERS_CMD);
    TRF79x0DirectCommand(TRF79X0_RUN_DECODERS_CMD);
}

//*****************************************************************************
//
// Set up registers for ISO 14443 A 106Kbit/s operation.  This function must
// be called after initializing the TRF79x0 (for example with TRF79x0Init()
// or TRF79x0DirectCommand() with argument \b TRF79X0_SOFT_INIT_CMD) and before
// calling any of the other ISO14443B functions.
//
//*****************************************************************************
void
NfcTagType4ASetupRegisters(void)
{
	unsigned char Data[11] = "\x08\x12\x34\x56";

	//Examples of start byte of Type A UID Values and MFGs seen by Nexus S.
    //These are just a few found by using the TagInfo app
	//0x01, 0x05, 0x07, 0x09, 0x19 = Infineon
	//0x02, 0x03, 0x04, 0x06, 0x0A = NXP
	//0x08 = Unknown, considered to be for random ID
	//0x1C, 0xC2, 0x3E, 0x80 = NXP

	TRF79x0DirectCommand(TRF79X0_SOFT_INIT_CMD);
    TRF79x0DirectCommand(TRF79X0_IDLE_CMD);

    //
    // TODO:check why have to set as this?
    //
    TRF79x0WriteRegister(TRF79X0_MODULATOR_CONTROL_REG,
    					TRF79X0_MOD_CTRL_MOD_OOK_100);

    //
    // Set the ISO format to NFC Card Emulation, Type A
    //
    TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x24);

    //
    // Set the regulator voltage to be automatic.
    //
    TRF79x0WriteRegister(TRF79X0_REGULATOR_CONTROL_REG,
    					TRF79X0_REGULATOR_CTRL_AUTO_REG);

    //
    // RX Special Settings for ISO14443A
    //
    TRF79x0WriteRegister(TRF79X0_RX_SPECIAL_SETTINGS_REG, 0x30);

    //
    // Set the Target Detection Level to Max; use SDD
    //
    TRF79x0WriteRegister(TRF79X0_NFC_TARGET_LEVEL_REG, 0x27);

    //
    // Set the NFCID to be sent during SDD
    //
    TRF79x0WriteRegisterContinuous(TRF79X0_NFC_ID_REG, Data, 4);

    TRF79x0WriteRegister(TRF79X0_NFC_LO_FIELD_LEVEL_REG, 0x83);

    // SDD need this
    TRF79x0WriteRegister(TRF79X0_ISO14443B_OPTIONS_REG, 0x01);

    //
    // ISO14443B TX Options
    //
//    TRF79x0WriteRegister(TRF79X0_ISO14443B_OPTIONS_REG, 0x01);
    TRF79x0WriteRegister(TRF79X0_ISO14443A_OPTIONS_REG, 0x00);

//    //
//    // Set the TX pulse to 106ns (0x20 * 73.7ns).
//    //
//    TRF79x0WriteRegister(TRF79X0_TX_PULSE_LENGTH_CTRL_REG, 0x20);
//
//    //
//    // Set the RX No response wait time to 529us (0xe * 37.76us).
//    //
//    TRF79x0WriteRegister(TRF79X0_RX_NO_RESPONSE_WAIT_REG, 0x0e);
//
//    //
//    // Set the RX wait time to 66us (7 * 9.44us).
//    //
//    TRF79x0WriteRegister(TRF79X0_RX_WAIT_TIME_REG, 0x07);
    TRF79x0WriteRegister(TRF79X0_TEST_SETTING1_REG, 0x40);

    TRF79x0WriteRegister(TRF79X0_CHIP_STATUS_CTRL_REG, 0x21);

    TRF79x0ResetFifoCommand();
    TRF79x0DirectCommand(TRF79X0_STOP_DECODERS_CMD);
    TRF79x0DirectCommand(TRF79X0_RUN_DECODERS_CMD);

    //
    // Delay 5ms before initializing the TRF79x0.
    //
    SysCtlDelay((SysCtlClockGet()/3000) * 2);
}

//*****************************************************************************
//
//
//
//*****************************************************************************
int
ISO14443BATQB(unsigned char *pucPUPI,unsigned char ucAFI, unsigned char ucBitRate,
			  unsigned char ucMaxFrameSize, unsigned char ucProtocolType,
			  unsigned char ucFWI, unsigned char ucADC, unsigned char ucFO)
{
    unsigned char pucATQB[12];

//
//    // Protocol Info Bytes
//    buffer[10] = 0x80;    // Date Rate Capability ( Only Support 106 kbps)
//    // Max Frame/Protocol type (128 bytes / PICC compliant to -4)
//    buffer[11] = 0x71;
//    // (FWI/ADC/FO) ( FWT = 77.3mSec, ADC = coded according to AFI, CID supported)
//    buffer[12] = 0x85;

    //
    // ATQB response.
    //
    pucATQB[0] = 0x50;
    pucATQB[1] = pucPUPI[0];
    pucATQB[2] = pucPUPI[1];
    pucATQB[3] = pucPUPI[2];
    pucATQB[4] = pucPUPI[3];
    pucATQB[5] = ucAFI;
    pucATQB[6] = 0xE2;			// CRC_B
    pucATQB[7] = 0xAF;			// CRC_B
    pucATQB[8] = 0x11;     		// # of applications (1)
    pucATQB[9] = ucBitRate;
    pucATQB[10] = (ucMaxFrameSize << 4) | ucProtocolType;
    pucATQB[11] = (ucFWI << 4) | (ucADC << 2) | ucFO;

    //
    // Transmit w/o receive
    //
	TRF79x0Transceive(pucATQB, sizeof(pucATQB), 0, 0, 0, 0, TRF79X0_TRANSCEIVE_TX_CRC);

	//
	// Return true
	//
	return(1);
}

//*****************************************************************************
//
// NFC P2P Functions
//
//*****************************************************************************
