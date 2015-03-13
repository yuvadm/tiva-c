//*****************************************************************************
//
// nfc_p2p.c - contains implementation of p2p over NFC
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
#include "nfclib/nfc_p2p.h"
#include "nfclib/nfc_f.h"
#include "nfclib/nfc_dep.h"
#include "nfclib/llcp.h"
#include "nfclib/snep.h"
#include "nfclib/debug.h"

//*****************************************************************************
//! \addtogroup nfc_p2p_api NFC P2P API Functions
//! @{
//! This module implements the encoding and decoding of NFC P2P messages
//! and records.
//!
//! It is assumed that users of this module have a functional knowledge of NFC
//! P2P messages and record types as defined by the NFC specification at
//! <a href="http://www.nfc-forum.org/specs/spec_list/">
//! http://www.nfc-forum.org/specs/spec_list</a> .
//!
//! The functions in this module assume that the NFCP2P_proccessStateMachine()
//! is being called every 77ms or less as defined by the Digital
//! Protocol Technical Specification requirement 197. Before any of the
//! functions in this module are called, TRF79x0Init() and NFCP2P_init() must be
//! called to initialize the transceiver and the NFCP2P state machine.
//
//*****************************************************************************

//*****************************************************************************
//
// Globals
//
//*****************************************************************************

//
// Global pointer to recieve data, used by NFCP2PStateMachine().
//
uint8_t *g_ui8RxDataPtr;

//
// Flag to keep track of when to transmit data. Used by NFCP2PStateMachine().
//
bool g_bTxDataAvailable = false;

//
// Timout value aquired from lower level in NFC Stack, used by
// NFCP2PStateMachine()
//
uint16_t g_ui16TargetTimeout = 0;

//*****************************************************************************
//
// State used by NFCP2PStateMachine.
//
// Options are:
// - NFC_P2P_PROTOCOL_ACTIVATION
// - NFC_P2P_PARAMETER_SELECTION
// - NFC_P2P_DATA_EXCHANGE_PROTOCOL
// - NFC_P2P_DEACTIVATION
//
//*****************************************************************************
tNFCP2PState g_eNFCP2PState = NFC_P2P_PROTOCOL_ACTIVATION;

//*****************************************************************************
//
// Global for what mode the TRF79x0 operates in.
//
// Options are:
// - BOARD_INIT
// - P2P_INITATIOR_MODE
// - P2P_PASSIVE_TARGET_MODE
// - P2P_ACTIVE_TARGET_MODE
// - CARD_EMULATION_TYPE_A
// - CARD_EMULATION_TYPE_B
//
//*****************************************************************************
tTRF79x0TRFMode g_eP2PMode;

//*****************************************************************************
//
// Global for TRF79x0 operating frequency.
//
// Options are:
// - FREQ_STAND_BY
// - FREQ_106_KBPS
// - FREQ_212_KBPS
// - FREQ_424_KBPS
//
//*****************************************************************************
tTRF79x0Frequency g_eP2PFrequency;

//*****************************************************************************
//! Initialize the variables used by the NFC Stack.
//!
//! \param eMode is the mode which to initialize the TRF79x0
//! \param eFrequency is the frequency which to initialize the TRF79x0
//!
//! This function must be called before any other NFCP2P function is called.
//! It can be called at any point to change the mode or frequency of the
//! TRF79x0 transceiver. This function initializes either the initiator or the
//! target mode.
//!
//! The \e eMode parameter can be any of the following:
//!
//! - \b BOARD_INIT                 - Initial Mode.
//! - \b P2P_INITATIOR_MODE         - P2P Initiator Mode.
//! - \b P2P_PASSIVE_TARGET_MODE    - P2P Passive Target Mode.
//! - \b P2P_ACTIVE_TARGET_MODE     - P2P Active Target Mode.
//! - \b CARD_EMULATION_TYPE_A      - Card Emulation for Type A cards.
//! - \b CARD_EMULATION_TYPE_B      - Card Emulation for Type B cards.
//!
//! The \e eFrequency parameter can be any of the following:
//!
//! - \b FREQ_STAND_BY - Used for Board Initialization.
//! - \b FREQ_106_KBPS - Frequency of 106 kB per second.
//! - \b FREQ_212_KBPS - Frequency of 212 kB per second.
//! - \b FREQ_424_KBPS - Frequency of 424 kB per second.
//!
//! \return None.
//
//*****************************************************************************
void
NFCP2P_init(tTRF79x0TRFMode eMode,tTRF79x0Frequency eFrequency)
{
    //
    // Reset Default Values
    //
    g_eNFCP2PState = NFC_P2P_PROTOCOL_ACTIVATION;
    g_eP2PMode = eMode;
    g_eP2PFrequency = eFrequency;
    g_bTxDataAvailable = false;
    g_ui16TargetTimeout = 0;

    //
    // Store the nfc_buffer ptr in g_ui8RxDataPtr
    //
    g_ui8RxDataPtr = TRF79x0GetNFCBuffer();

    //
    // Initialize NFC DEP Global Pointer to use the g_ui8RxDataPtr pointer -
    // the pointer is used to send responses/commands to the other Peer to
    // Peer device. This implementation allows to reduce the RAM consumption.
    //
    NFCDEP_SetBufferPtr(g_ui8RxDataPtr);
}

//*****************************************************************************
//
//!
//! Processes low level stack.
//!
//! \return This function returns the current NFCP2P state.
//!
//! The \e \b tNFCP2PState return parameter can be any of the following
//! - \b NFC_P2P_PROTOCOL_ACTIVATION    - Polling/Listening for SENSF_REQ / SENSF_RES.
//! - \b NFC_P2P_PARAMETER_SELECTION    - Setting the NFCIDs and bit rate
//! - \b NFC_P2P_DATA_EXCHANGE_PROTOCOL - Data exchange using the LLCP layer
//! - \b NFC_P2P_DEACTIVATION           - Technology deactivation.
//!
//! This function must be executed every 77 ms or less as
//! defined by requirement 197 inside the Digital Protocol Technical
//! Specification. When the g_eP2PMode is set to P2P_INITATIOR_MODE, this
//! function sends a SENSF_REQ to check if there is a Target in the field,
//! while blocking the main application. If there is no target in the field,
//! it exits. When the g_eP2PMode is set to P2P_PASSIVE_TARGET_MODE, this
//! function waits for command for 495 ms, while blocking the main
//! application. If no commands are received or if any errors occurred, this
//! function exits.  Once a technology is activated for either
//! P2P_INITATIOR_MODE or P2P_PASSIVE_TARGET_MODE, the main application can use
//! g_eNFCP2PState when equal to NFC_P2P_DATA_EXCHANGE_PROTOCOL, to then call
//! NFCP2P_sendPacket() to send data from the TRF7970A to a target/initiator.
//! Furthermore when g_eNFCP2PState is  NFC_P2P_DATA_EXCHANGE_PROTOCOL,
//! the main application must check the receive state with the function
//! NFCP2P_getReceiveState() each time NFCP2P_proccessStateMachine() is
//! executed to ensure it handles the data as it is received.
//!
//! \return g_eNFCP2PState, which is the current P2P state.
//
//*****************************************************************************
tNFCP2PState
NFCP2P_proccessStateMachine(void)
{
    uint8_t *pui8NFCID2_Ptr=0;

    tTRF79x0IRQFlag eIRQStatus = IRQ_STATUS_IDLE;

    switch(g_eNFCP2PState)
    {
        case NFC_P2P_PROTOCOL_ACTIVATION:
        {
            if (g_eP2PMode == P2P_INITATIOR_MODE)
            {
                //
                // Initialize the TRF7970A Registers for P2P Initiator Mode -
                // in the case there is an external field enabled, the function
                // will return STATUS_FAIL, the TRF7970 field will be disabled,
                // and the program should switch to Target Mode.
                //
                if(TRF79x0Init2(P2P_INITATIOR_MODE, g_eP2PFrequency) ==
                    STATUS_FAIL)
                    break;

                //
                // Send SENSF_REQ
                //
                NFCTypeF_SendSENSF_REQ();

                //
                // Check if IRQ is triggered - timeout of 20 mS
                //
                if(TRF79x0IRQHandler(20) == IRQ_STATUS_RX_COMPLETE)
                {
                    //
                    // Process the received data - check for valid SENSF_RES
                    //
                    if (NFCTypeF_ProcessReceivedData(g_ui8RxDataPtr) ==
                        STATUS_SUCCESS)
                    {
                        g_eNFCP2PState = NFC_P2P_PARAMETER_SELECTION;
                        #ifdef DEBUG_PRINT
                        //UARTprintf("\nInitiator Activated \n");
                        //UARTprintf("Exit PROT ACT \n");
                        #endif

                        break;
                    }
                    else
                    {
                        TRF79x0DisableTransmitter();
                        break;
                    }
                }
                else
                {
                    TRF79x0DisableTransmitter();
                    break;
                }

            }
            else if (g_eP2PMode == P2P_PASSIVE_TARGET_MODE)
            {
                TRF79x0Init2(P2P_PASSIVE_TARGET_MODE, g_eP2PFrequency);

                //
                // Poll the IRQ flag for 495 mS.
                //
                while(eIRQStatus != IRQ_STATUS_TIME_OUT)
                {
                    eIRQStatus = TRF79x0IRQHandler(495);

                    //
                    // Process the received data - check for valid SENSF_REQ
                    //
                    if((eIRQStatus == IRQ_STATUS_RX_COMPLETE) &&
                        (NFCTypeF_ProcessReceivedData(g_ui8RxDataPtr) ==
                        STATUS_SUCCESS))
                    {
                        g_eNFCP2PState = NFC_P2P_PARAMETER_SELECTION;
                        break;
                        #ifdef DEBUG_PRINT
                        //UARTprintf("\nTarget Activated \n");
                        //UARTprintf("Exit PROT ACT \n");
                        #endif

                    }
                }
                break;

            }
            else if (g_eP2PMode == P2P_ACTIVE_TARGET_MODE)
            {
                TRF79x0Init2(P2P_ACTIVE_TARGET_MODE, g_eP2PFrequency);

                //
                // Poll the IRQ flag for 495 mS.
                //
                while(eIRQStatus != IRQ_STATUS_TIME_OUT)
                {
                    eIRQStatus = TRF79x0IRQHandler(495);

                    //
                    // Process the received data - check for valid ATR_REQ
                    //
                    if((eIRQStatus == IRQ_STATUS_RX_COMPLETE) &&
                        (NFCDEP_ProcessReceivedRequest(g_ui8RxDataPtr,0,true) ==
                        STATUS_SUCCESS))
                    {
                        g_eNFCP2PState = NFC_P2P_DATA_EXCHANGE_PROTOCOL;
                        break;
                        #ifdef DEBUG_PRINT
                        //UARTprintf("\nTarget Activated \n");
                        //UARTprintf("Exit PROT ACT \n");
                        #endif


                    }
                }
                break;
            }
        }
        case NFC_P2P_PARAMETER_SELECTION:
        {
            //
            // Reset the LLCP Parameters
            //
            LLCP_init();
            if (g_eP2PMode == P2P_INITATIOR_MODE)
            {
                pui8NFCID2_Ptr = NFCTypeF_GetNFCID2();
                NFCDEP_SendATR_REQ(pui8NFCID2_Ptr);
                //
                // Check if IRQ is triggered - timeout of 100 mS
                //
                if (TRF79x0IRQHandler(1000) == IRQ_STATUS_RX_COMPLETE)
                {
                    //
                    // Process the received data - check for valid ATR_RES
                    //
                    if (NFCDEP_ProcessReceivedData(g_ui8RxDataPtr)
                            == STATUS_SUCCESS)
                    {
                        //
                        // If Current Frequency is 212 request to go to a higher
                        // baud rate
                        //
                        if(g_eP2PFrequency == FREQ_212_KBPS)
                        {
                            NFCDEP_SendPSL_REQ();

                            if (TRF79x0IRQHandler(1000) ==
                                IRQ_STATUS_RX_COMPLETE)
                            {
                                if (NFCDEP_ProcessReceivedData(g_ui8RxDataPtr)==
                                    STATUS_SUCCESS)
                                {
                                    //
                                    // If the function returns successful then
                                    // the returned DID was correct.
                                    //
                                    TRF79x0SetMode(g_eP2PMode,FREQ_424_KBPS);
                                }
                            }
                            else
                            {
                                #ifdef DEBUG_PRINT
                                //UARTprintf("\nMCU Timed Out\n");
                                //UARTprintf("Exit PARAM SEL\n");
                                #endif
                                g_eNFCP2PState = NFC_P2P_PROTOCOL_ACTIVATION;
                                TRF79x0DisableTransmitter();
                                break;
                            }
                        }

                        g_eNFCP2PState = NFC_P2P_DATA_EXCHANGE_PROTOCOL;
                        #ifdef DEBUG_PRINT
                        //UARTprintf("Exit P2P PARM SEL\n");
                        #endif

                        g_ui16TargetTimeout = LLCP_getLinkTimeOut();

                        #ifdef DEBUG_PRINT
                        //UARTprintf("Time out is: %d",g_ui16TargetTimeout);
                        #endif
                    }
                    else
                    {
                        g_eNFCP2PState = NFC_P2P_PROTOCOL_ACTIVATION;
                        TRF79x0DisableTransmitter();
                        break;
                    }
                }
                else
                {
                    #ifdef DEBUG_PRINT
                    //UARTprintf("\nMCU Timed Out\n");
                    //UARTprintf("Exit PARAM SEL\n");
                    #endif
                    g_eNFCP2PState = NFC_P2P_PROTOCOL_ACTIVATION;
                    TRF79x0DisableTransmitter();
                    break;
                }
            }
            else if (g_eP2PMode == P2P_PASSIVE_TARGET_MODE)
            {
                //
                // Check if IRQ is triggered - timeout of 100 mS
                //
                if (TRF79x0IRQHandler(1000) == IRQ_STATUS_RX_COMPLETE)
                {
                    pui8NFCID2_Ptr = NFCTypeF_GetNFCID2();
                    //
                    // Process the received data - check for valid ATR_REQ
                    //
                    if (NFCDEP_ProcessReceivedRequest(g_ui8RxDataPtr,
                                                        pui8NFCID2_Ptr,false)
                            == STATUS_SUCCESS)
                    {
                        g_eNFCP2PState = NFC_P2P_DATA_EXCHANGE_PROTOCOL;
                        #ifdef DEBUG_PRINT
                        //UARTprintf("Exit P2P PARM SEL\n");
                        #endif
                    }
                    else
                    {
                        g_eNFCP2PState = NFC_P2P_PROTOCOL_ACTIVATION;
                        #ifdef DEBUG_PRINT
                        //UARTprintf("\nMCU Invalid ATR REQ\n");
                        //UARTprintf("Exit P2P PARM SEL\n");
                        #endif
                        break;
                    }
                }
                else
                {
                    g_eNFCP2PState = NFC_P2P_PROTOCOL_ACTIVATION;
                    #ifdef DEBUG_PRINT
                    //UARTprintf("\nMCU Timed Out\n");
                    //UARTprintf("Exit PARAM SEL\n");
                    #endif
                    break;
                    //TRF79x0DisableTransmitter();
                }
            }
            else if (g_eP2PMode == P2P_ACTIVE_TARGET_MODE)
            {
                //TODO
                break;
            }
        }
        case NFC_P2P_DATA_EXCHANGE_PROTOCOL:
        {
            if (g_eP2PMode == P2P_INITATIOR_MODE)
            {
                NFCDEP_SendDEP_REQ(g_ui8RxDataPtr);
                //
                // Check if IRQ is triggered - timeout of 100 mS
                //
                if (TRF79x0IRQHandler(g_ui16TargetTimeout) ==
                        IRQ_STATUS_RX_COMPLETE)
                {
                    //
                    // Process the received data - check for valid DEP_RES
                    //
                    if (NFCDEP_ProcessReceivedData(g_ui8RxDataPtr) ==
                            STATUS_FAIL)
                    {
                        //DebugPrintf("Exit DATA EXCHANGE\n");
                        g_eNFCP2PState = NFC_P2P_PROTOCOL_ACTIVATION;
                        break;
                    }

                    //
                    // Check if there is data to send to the Target.
                    //
                    if (g_bTxDataAvailable == true)
                    {
                        //
                        // Set the Connect PDU as the next command to the Target
                        //
                        if (LLCP_setNextPDU(LLCP_CONNECT_PDU) == STATUS_SUCCESS)
                        {
                            //
                            // If there was no ongoing connection, then clear
                            // the g_data_available flag
                            //
                            g_bTxDataAvailable = false;
                        }
                    }
                }
                else
                {
                    #ifdef DEBUG_PRINT
                    //UARTprintf("\nMCU Timed Out \n");
                    //UARTprintf("Exit DATA EXCHANGE\n");
                    #endif
                    g_eNFCP2PState = NFC_P2P_PROTOCOL_ACTIVATION;
                    TRF79x0DisableTransmitter();
                    break;
                }
            }
            else if ((g_eP2PMode == P2P_PASSIVE_TARGET_MODE) ||
                     (g_eP2PMode == P2P_ACTIVE_TARGET_MODE))
            {
                //
                // Check if IRQ is triggered - timeout of 100 mS
                //
                eIRQStatus = IRQ_STATUS_IDLE;
                while((eIRQStatus == IRQ_STATUS_IDLE) ||
                      (eIRQStatus == IRQ_STATUS_RF_FIELD_CHANGE) )
                {
                    eIRQStatus = TRF79x0IRQHandler(1000);
                }

                if (eIRQStatus == IRQ_STATUS_RX_COMPLETE)
                {
                    //
                    // Check if there is data to send to the Target.
                    //
                    if (g_bTxDataAvailable == true)
                    {
                        //
                        // Set the Connect PDU as the next command to the Target
                        //
                        if (LLCP_setNextPDU(LLCP_CONNECT_PDU) == STATUS_SUCCESS)
                        {
                            //
                            // If there was no ongoing connection, then clear
                            //the g_data_available flag
                            //
                            g_bTxDataAvailable = false;
                        }
                    }

                    //
                    // Process the received data - check for valid DEP_REQ
                    //
                    if (NFCDEP_ProcessReceivedRequest(g_ui8RxDataPtr,
                                                        pui8NFCID2_Ptr,false)
                            == STATUS_FAIL)
                    {
                        g_eNFCP2PState = NFC_P2P_PROTOCOL_ACTIVATION;
                        #ifdef DEBUG_PRINT
                        //UARTprintf("Exit DATA EXCHANGE\n");
                        #endif
                        break;
                    }
                }
                else if (eIRQStatus
                     == (IRQ_STATUS_RX_COMPLETE | IRQ_STATUS_FIFO_HIGH_OR_LOW))
                {
                    // Wait to receive the complete payload
                }
                else
                {
                    g_eNFCP2PState = NFC_P2P_PROTOCOL_ACTIVATION;
                    #ifdef DEBUG_PRINT
                    //UARTprintf("\nMCU Timed Out \n");
                    //UARTprintf("Exit DATA EXCHANGE\n");
                    #endif

                    break;
                }
            }
            else if (g_eP2PMode == P2P_ACTIVE_TARGET_MODE)
            {
                //TODO
                break;
            }
        }
        case NFC_P2P_DEACTIVATION:
        {
            break;
        }
    }

    return g_eNFCP2PState;

}

//*****************************************************************************
//
//! Sends a raw buffer of data to the SNEP stack to be transmitted.
//!
//! \param pui8DataPtr is a pointer to the raw data to be sent.
//! \param ui32DataLength is the length of the raw data.
//!
//! This function is used to send a data stream over NFC. The buffer resulting
//! from a call to NFCP2P_NDEFMessageEncoder() should be fed to this function.
//!
//! \return Status of sent packet.
//!
//! The \e \b tStatus parameter can be any of the following:
//!
//! - \b STATUS_FAIL    - The function exited with a failure.
//! - \b STATUS_SUCCESS - The function ended in succes.
//
//*****************************************************************************
tStatus
NFCP2P_sendPacket(uint8_t *pui8DataPtr, uint32_t ui32DataLength)
{
    g_bTxDataAvailable = true;
    return SNEP_setupPacket(pui8DataPtr,ui32DataLength);
}

//*****************************************************************************
//
//! NFCP2P_getReceiveState - Gets the receive state from the low level SNEP
//! stack.
//!
//! Description: This function is used to get the receive payload  status
//! from the SNEP layer.
//!
//! \return This function returns the receive state.
//
//*****************************************************************************
sNFCP2PRxStatus
NFCP2P_getReceiveState(void)
{
    sNFCP2PRxStatus eReceiveStatus;

     SNEP_getReceiveStatus(&eReceiveStatus.eDataReceivedStatus,
                           &eReceiveStatus.ui8DataReceivedLength,
                           &eReceiveStatus.pui8RxDataPtr);

    return eReceiveStatus;
}

//*****************************************************************************
//
//! Encodes NFC Message meta-data and payload information.
//!
//! \param sNDEFDataToSend is a sNDEFMessageData structure filled out with the
//! NDEF message to send.
//! \param pui8Buffer is a pointer to the buffer where the raw encoded data will
//! be stored
//! \param ui16BufferMaxLength is the maximum number of bytes the buffer
//! can hold. This parameter is used to prevent writing past the end of the
//! buffer.
//! \param pui32BufferLength is a pointer to an integer that is filled with
//! the length of the raw data encoded to the \b pui8Buffer.
//!
//! This function takes a filled sNDEFMessageData structure and encodes it to
//! the provided buffer. The length, in bytes, of the data encoded to the buffer
//! is stored into the integer pointer provided.
//!
//! \return This function returns \b STATUS_SUCCESS (1) or \b STATUS_FAIL (0).
//!
//
// Note: for an explanation of the fields please see the Programmers Note in
//       nfc_p2p.h
//*****************************************************************************
bool
NFCP2P_NDEFMessageEncoder(sNDEFMessageData sNDEFDataToSend,
                                        uint8_t *pui8Buffer,
                                        uint16_t ui16BufferMaxLength,
                                        uint32_t *pui32BufferLength)
{
    uint32_t ui32HeaderSize = 0;
    uint32_t x;
    sNDEFMessageData sMessage = sNDEFDataToSend;

    //
    // Check Arguements, ASSERT / return STATUS_FAIL as appropriate
    //
    ASSERT(ui16BufferMaxLength > 0);
    ASSERT(pui8Buffer != 0);
    ASSERT(sNDEFDataToSend.ui8TypeLength > 0);
    ASSERT(sNDEFDataToSend.ui32PayloadLength > 0);
    ASSERT(sNDEFDataToSend.pui8PayloadPtr != 0);
    ASSERT(sNDEFDataToSend.ui32PayloadLength < ui16BufferMaxLength);
    if(
        (ui16BufferMaxLength == 0)                  ||
        (pui8Buffer == 0)                           ||
        (sNDEFDataToSend.ui8TypeLength == 0)        ||
        (sNDEFDataToSend.ui32PayloadLength == 0 )   ||
        (sNDEFDataToSend.pui8PayloadPtr == 0)       ||
        (sNDEFDataToSend.ui32PayloadLength > ui16BufferMaxLength)
      )
    {
        DebugPrintf("   ERR: NDEFMessageEncoder: Invalid Input\n");
        return STATUS_FAIL;
    }
    if(ui16BufferMaxLength < 25)
    {
        DebugPrintf("Warning: NDEFMessageEncoder : You need a bigger buffer\n");
    }

    //
    // Fill STATUS_BYTE field
    //
    pui8Buffer[ui32HeaderSize] = (
                            NDEF_STATUSBYTE_SET_MB(sMessage.sStatusByte.MB) |
                            NDEF_STATUSBYTE_SET_ME(sMessage.sStatusByte.ME) |
                            NDEF_STATUSBYTE_SET_CF(sMessage.sStatusByte.CF) |
                            NDEF_STATUSBYTE_SET_SR(sMessage.sStatusByte.SR) |
                            NDEF_STATUSBYTE_SET_IL(sMessage.sStatusByte.IL) |
                            NDEF_STATUSBYTE_SET_TNF(sMessage.sStatusByte.TNF)
                            );
    ui32HeaderSize++;

    //
    // Fill TYPE_LENGTH field
    //
    pui8Buffer[ui32HeaderSize] = sMessage.ui8TypeLength;
    ui32HeaderSize++;

    //
    // Fill PAYLOAD_LENGTH field.
    // based on StatusByte.SR field. May truncate if improperly set.
    //
    switch(sMessage.sStatusByte.SR)
    {
        //
        // PAYLOAD_LENGTH is 1 byte long
        //
        case NDEF_STATUSBYTE_SR_1BYTEPAYLOADSIZE:
        {
            pui8Buffer[ui32HeaderSize] = (sMessage.ui32PayloadLength & 0xFF);
            ui32HeaderSize++;
            break;
        }

        //
        // PAYLOAD_LENGTH is 4 bytes long, inverted order (NFC Standard)
        //
        case NDEF_STATUSBYTE_SR_4BYTEPAYLOADSIZE:
        {
            pui8Buffer[ui32HeaderSize+0] = ((sMessage.ui32PayloadLength >> 3*8)
                                                & 0xFF);
            pui8Buffer[ui32HeaderSize+1] = ((sMessage.ui32PayloadLength >> 2*8)
                                                & 0xFF);
            pui8Buffer[ui32HeaderSize+2] = ((sMessage.ui32PayloadLength >> 1*8)
                                                & 0xFF);
            pui8Buffer[ui32HeaderSize+3] = ((sMessage.ui32PayloadLength >> 0*8)
                                                & 0xFF);
            ui32HeaderSize = ui32HeaderSize + 4;
            break;
        }

        //
        // default case, should never get here, if you do its an error
        //
        default:
        {
            DebugPrintf("ERR: NFC Header Encoder fn PAYLOAD_LENGTH field\n");
            return STATUS_FAIL;
            break;
        }
    }

    //
    // Fill ID_LENGTH field.
    // depends on Statusbyte.IL, if IL not set but data given in ui8IDLength
    // the data will be ignored.
    //
    switch(sMessage.sStatusByte.IL)
    {
        //
        // No ID_LENGTH field included
        //
        case NDEF_STATUSBYTE_IL_IDLENGTHABSENT:
        {
            // do nothing
            break;
        }

        //
        // ID_LENGTH field present, fill data, incriment buffer pointer
        //
        case NDEF_STATUSBYTE_IL_IDLENGTHPRESENT:
        {
            pui8Buffer[ui32HeaderSize] = sMessage.ui8IDLength;
            ui32HeaderSize++;
            break;
        }

        //
        // default case, should never get here, if you do its an error.
        //
        default:
        {
            DebugPrintf("ERR: NFC Header Encoder fn ID_LENGTH field\n");
            return STATUS_FAIL;
            break;
        }
    }

    //
    // Fill TYPE field. If TYPE_LENGTH > NDEF_TYPE_MAXSIZE then TYPE will be
    // truncated to MAXSIZE
    //
    if(0 == sMessage.ui8TypeLength)
    {
        //
        // do nothing
        // TYPE_LENGTH = 0, so there is nothing to put in the TYPE field
        //
    }
    else
    {
        for(x = 0;(x < sMessage.ui8TypeLength) && (x < NDEF_TYPE_MAXSIZE); x++)
        {
            pui8Buffer[ui32HeaderSize] = sMessage.pui8Type[x];
            ui32HeaderSize++;
        }
    }

    //
    // Fill ID field. If ID_LENGTH > NDEF_ID_MAXSIZE then ID will be truncated
    // to MAXSIZE.
    //
    switch(sMessage.sStatusByte.IL)
    {
        //
        // StatusByte.IL says no ID_LENGTH field, thus no ID field.
        //
        case NDEF_STATUSBYTE_IL_IDLENGTHABSENT:{
            //do nothing.
            break;
        }

        //
        // StatusByte.IL says ID_LENGTH Exists, so add the ID.
        //
        case NDEF_STATUSBYTE_IL_IDLENGTHPRESENT:
        {
            if(0 == sMessage.ui8IDLength)
            {
                //
                // Do nothing. ID_LENGTH = 0 so there is no ID to add.
                //
            }
            else
            {
                for(x = 0;(x < sMessage.ui8IDLength) && (x < NDEF_ID_MAXSIZE);
                    x++)
                {
                    pui8Buffer[ui32HeaderSize] = sMessage.pui8ID[x];
                    ui32HeaderSize++;
                }
            }
            break;
        }
    }

    //
    // Make sure we wont overflow the buffer with the payload in the next step.
    //
    if((ui32HeaderSize + sMessage.ui32PayloadLength) > ui16BufferMaxLength)
    {
        ASSERT(0);
        DebugPrintf("ERR:NDEFMessageEncoder: BufferOverflow Payload too big\n");
        return STATUS_FAIL;
    }

    //
    // Fill PAYLOAD buffer.
    //
    if(sMessage.sStatusByte.SR == NDEF_STATUSBYTE_SR_1BYTEPAYLOADSIZE)
    {
        //
        // 1 byte PAYLOAD_LENGTH.
        //
        for(x = 0;x < (sMessage.ui32PayloadLength & 0xFF);x++)
        {
            pui8Buffer[ui32HeaderSize] =  sMessage.pui8PayloadPtr[x];
            ui32HeaderSize++;
        }
    }
    else
    {
        //
        // 4 byte PAYLOAD_LENGTH.
        // (treat Payload length as a 32bit number)
        //
        for(x = 0;x < sMessage.ui32PayloadLength; x++)
        {
            pui8Buffer[ui32HeaderSize] = sMessage.pui8PayloadPtr[x];
            ui32HeaderSize++;
        }
    }

    //
    // Fill BufferLength variable.
    //
    *pui32BufferLength = ui32HeaderSize;


    return STATUS_SUCCESS;

}

//*****************************************************************************
//
//! Decodes NFC Message meta-data and payload information.
//!
//! \param psNDEFDataDecoded is a pointer to the sNDEFMessageData structure to
//! be filled.
//! \param pui8Buffer is a pointer to the raw NFC data buffer from which to
//! decode the data.
//! \param ui16BufferMaxLength is the maximum number of bytes the buffer
//! can hold. This parameter is used to prevent reading past the end of the
//! buffer.
//!
//! This function takes in a buffer of raw NFC data and fills up an
//! sNDEFMessageData structure. This function is the first step to decoding an
//! NFC Message. The next step is to decode the Message Payload, which is the record.
//! The decoded sNDEFMessageData structure has a field named \b pui8Type. The
//! \b pui8Type field defines the record type and therefore indicates which
//! RecordDecoder function to use on the Message Payload.
//!
//! \return This function returns \b STATUS_SUCCESS (1) or \b STATUS_FAIL (0).
//
// Note: local variables are used to break out fields from the header for
//       clarity. ui32HeaderSize is used to keep track of how large the header
//       is in bytes. It is used to computer the size of the payload at the end.
//       (length of Buffer - HeaderSize = Payload length)
//
//*****************************************************************************
bool
NFCP2P_NDEFMessageDecoder(sNDEFMessageData *psNDEFDataDecoded,
                                        uint8_t *pui8Buffer,
                                        uint16_t ui16BufferMaxLength)
{
    sNDEFMessageData *psMessage;
    uint8_t ui8StatusByte,ui8TypeLength,ui8IDLength;
    uint8_t *pui8PayloadPtr;
    uint32_t ui32HeaderSize = 0;
    uint32_t ui32PayloadLength=0;
    uint32_t x;

    //
    // Check Input for Validity
    //
    ASSERT(pui8Buffer != 0);
    ASSERT(ui16BufferMaxLength > 0);

    //
    // Minimum length of header is 5 bytes.
    //
    if(ui16BufferMaxLength <= 5)
    {
        DebugPrintf("ERR: NDEFMessageDecoder: Invalid Input\n");
        return STATUS_FAIL;
    }

    psMessage = psNDEFDataDecoded;

    //
    // Load Status Byte into NDEF Structure.
    //
    ui8StatusByte = pui8Buffer[ui32HeaderSize];
    psMessage->sStatusByte.MB = NDEF_STATUSBYTE_GET_MB(ui8StatusByte);
    psMessage->sStatusByte.ME = NDEF_STATUSBYTE_GET_ME(ui8StatusByte);
    psMessage->sStatusByte.CF = NDEF_STATUSBYTE_GET_CF(ui8StatusByte);
    psMessage->sStatusByte.SR = NDEF_STATUSBYTE_GET_SR(ui8StatusByte);
    psMessage->sStatusByte.IL = NDEF_STATUSBYTE_GET_IL(ui8StatusByte);
    psMessage->sStatusByte.TNF = NDEF_STATUSBYTE_GET_TNF(ui8StatusByte);

    //
    // Increment size of header (+1 for the size of the Status Byte).
    //
    ui32HeaderSize++;

    //
    // Load TypeLength byte into NDEF Structure.
    //
    ui8TypeLength = pui8Buffer[ui32HeaderSize];
    psMessage->ui8TypeLength = ui8TypeLength;

    //
    // Increment size of header (+1 for the size of the Status Byte).
    //
    ui32HeaderSize++;

    //
    // Determine the payload size based upon the SR field in the header.
    //
    switch (psMessage->sStatusByte.SR)
    {
        //
        // Short Record (PAYLOAD_LENGTH field is 1 byte).
        //
        case NDEF_STATUSBYTE_SR_1BYTEPAYLOADSIZE:
        {
            ui32PayloadLength = pui8Buffer[ui32HeaderSize];

            //
            // Validate Data
            //
            if((ui32HeaderSize + ui32PayloadLength) > ui16BufferMaxLength)
            {
                ASSERT(0);
                DebugPrintf(
                    "ERR: NFCP2P_NDEFMessageDecoder: ui32PayloadLength > ui16BufferMaxLength\n");
                DebugPrintf("\tYou Need a bigger buffer to hold this message.\n");
                return STATUS_FAIL;
            }
            else
            {
                //
                // Set Payload Length
                //
                psMessage->ui32PayloadLength = ui32PayloadLength;
                ui32HeaderSize++;
            }
            break;
        }

        //
        // Normal Record (PAYLOAD_LENGTH field is 4 bytes).
        //
        case NDEF_STATUSBYTE_SR_4BYTEPAYLOADSIZE:
        {
            ui32PayloadLength =
                                (
                                (pui8Buffer[ui32HeaderSize + 3] << 0*8) |
                                (pui8Buffer[ui32HeaderSize + 2] << 1*8) |
                                (pui8Buffer[ui32HeaderSize + 1] << 2*8) |
                                (pui8Buffer[ui32HeaderSize + 0] << 3*8)
                                );

            //
            // Validate Data
            //
            if((ui32HeaderSize + ui32PayloadLength) > ui16BufferMaxLength)
            {
                ASSERT(0);
                DebugPrintf(
                    "ERR: NFCP2P_NDEFMessageDecoder: ui32PayloadLength > ui16BufferMaxLength\n");
                DebugPrintf("\tYou Need a bigger buffer to hold this message.\n");
                return STATUS_FAIL;
            }
            else
            {
                //
                // Set Payload Length
                //
                psMessage->ui32PayloadLength = ui32PayloadLength;
                ui32HeaderSize = ui32HeaderSize + 4;
            }
            break;
        }

        //
        // This should never happen. return error.
        //
        default:
        {
            DebugPrintf("NDEFMessageDecoder: ERR decoding SR bit \n");
            ASSERT(0);
            return STATUS_FAIL;
            break;
        }
    }

    //
    // Load ID_LENGTH field, if it exists. Depends on StatusByte.IL.
    //
    switch (psMessage->sStatusByte.IL)
    {
        //
        // ID_LENGTH field exists. Load it to the NDEF structure.
        //
        case NDEF_STATUSBYTE_IL_IDLENGTHPRESENT:
        {
            ui8IDLength = pui8Buffer[ui32HeaderSize];
            psMessage->ui8IDLength = ui8IDLength;
            ui32HeaderSize++;
            break;
        }

        //
        // ID_LENGTH field does not exist and thus the ID field doesnt exists.
        // Load 0 to NDEF structure to express this
        //
        case NDEF_STATUSBYTE_IL_IDLENGTHABSENT:
        {
            psMessage->ui8IDLength = 0;
            break;
        }

        //
        // This should never happen. return error.
        //
        default:
        {
            DebugPrintf(
               "ERR: Invalid ID_LENGTH field Detected in NDEFMessageDecoder\n");
            ASSERT(0);
            return STATUS_FAIL;
            break;
        }
    }

    //
    // Load TYPE field based on length in TYPE_LENGTH field
    //
    // If TYPE_LENGTH value is larger than NDEF_TYPE_MAXSIZE truncate to MAXSIZE
    // and adjust index in buffer to end of TYPE so as to not lose data / skew
    // pointer.
    //
    if(psMessage->ui8TypeLength > NDEF_TYPE_MAXSIZE)
    {
        #ifdef DEBUG_PRINT
        ASSERT(0);
        UARTprintf("ERR: MessageDecode: TYPE > NDEF_TYPE_MAXSIZE, truncating to %d bytes\n",
                        NDEF_TYPE_MAXSIZE);
        UARTprintf("    Orig Type = ");
        for(x = 0;x < psMessage->ui8TypeLength;x++)
        {
            UARTprintf("%c",pui8Buffer[ui32HeaderSize + x]);
        }
        UARTprintf("\n");
        #endif

        //
        // Copy across truncated TYPE
        //
        for(x = 0;x < NDEF_TYPE_MAXSIZE;x++)
        {
            psMessage->pui8Type[x] = pui8Buffer[ui32HeaderSize];
            ui32HeaderSize++;
        }

        //
        // Adjust index appropriately.
        //
        ui32HeaderSize = ui32HeaderSize +
                            (psMessage->ui8TypeLength - NDEF_TYPE_MAXSIZE);
        psMessage->ui8TypeLength = NDEF_TYPE_MAXSIZE;
    }
    else
    {
        //
        // No problem
        // Load Type field into NDEF structure
        //
        for(x = 0;x < psMessage->ui8TypeLength;x++)
        {
            psMessage->pui8Type[x] = pui8Buffer[ui32HeaderSize];
            ui32HeaderSize++;
        }
    }

    //
    // Load ID field into NDEF structure. Depends on length in ID_LENGTH field.
    // if ID field is > NDEF_ID_MAXSIZE truncate to MAXSIZE
    //
    if(psMessage->ui8IDLength > NDEF_ID_MAXSIZE)
    {
        #ifdef DEBUG_PRINT
        ASSERT(0);
        UARTprintf("ERR: ID_LENGTH > NDEF_ID_MAXSIZE, trucating to %d bytes\n",
            NDEF_ID_MAXSIZE);
        UARTprintf("    Orig ID = ");
        for(x = 0;x < psMessage->ui8IDLength;x++)
        {
            UARTprintf("%c",pui8Buffer[ui32HeaderSize + x]);
        }
        UARTprintf("\n");
        #endif

        //
        // Copy across truncated ID
        //
        for(x = 0;x < NDEF_ID_MAXSIZE;x++)
        {
            psMessage->pui8ID[x] = pui8Buffer[ui32HeaderSize];
            ui32HeaderSize++;
        }

        //
        // adjust index appropriately
        //
        ui32HeaderSize = ui32HeaderSize + (psMessage->ui8IDLength -
                                                            NDEF_ID_MAXSIZE);
        psMessage->ui8IDLength = NDEF_ID_MAXSIZE;
    }
    else
    {
        //
        // No problem
        // Load ID field into NDEF structure
        //
        for(x = 0;x < psMessage->ui8IDLength;x++)
        {
            psMessage->pui8ID[x] = pui8Buffer[ui32HeaderSize];
            ui32HeaderSize++;
        }
    }

    //
    // Error Check
    // Check to make sure we didnt overrun the buffer / read beyond its bounds.
    //
    if((ui32HeaderSize + psMessage->ui32PayloadLength) > ui16BufferMaxLength)
    {
        ASSERT(0);
        DebugPrintf("ERR: NDEFMessageDecode: Buffer OverRun / OverRead\n");

        //
        // Clear all data out of datastrucutre, dont return invalid data.
        //
        psMessage->ui32PayloadLength=0;
        psMessage->pui8PayloadPtr=0;

        return STATUS_FAIL;
    }

    //
    // Calculate Payload Pointer (payload is located after the header)
    //
    pui8PayloadPtr = pui8Buffer + ui32HeaderSize;

    //
    // Set the Message Payload Pointer
    //
    psMessage->pui8PayloadPtr = pui8PayloadPtr;

    return STATUS_SUCCESS;
}

//*****************************************************************************
//
//! Encode NDEF Text Records.
//!
//! \param sTextRecord is the Text Record Structure to be encoded.
//! \param pui8Buffer is a pointer to the buffer to fill with the raw NFC data.
//! \param ui16BufferMaxLength is the maximum number of bytes the buffer
//! can hold. This parameter is used to prevent writing past the end of the
//! buffer.
//! \param pui32BufferLength is a pointer to the integer to hold the length of
//! the raw NFC data buffer.
//!
//! This function takes a TextRecord structure and encodes it into a provided
//! buffer in the raw NFC data format. The length of the data stored in the
//! buffer is stored in \e ui32BufferLength.
//!
//! \return This function returns \b STATUS_SUCCESS (1) or \b STATUS_FAIL (0).
//
//*****************************************************************************
bool
NFCP2P_NDEFTextRecordEncoder(sNDEFTextRecord sTextRecord,
                                uint8_t *pui8Buffer,
                                uint16_t ui16BufferMaxLength,
                                uint32_t *pui32BufferLength)
{
    uint8_t  x;
    uint32_t ui32RecordIndex = 0;

    //
    // Validate Input
    //
    ASSERT(pui8Buffer != 0);
    ASSERT(ui16BufferMaxLength > 0);
    ASSERT(pui32BufferLength != 0);
    ASSERT(sTextRecord.pui8Text != 0);
    ASSERT(sTextRecord.ui32TextLength > 0);
    ASSERT(sTextRecord.ui32TextLength < ui16BufferMaxLength);
    if( (pui8Buffer == 0)                   ||
        (ui16BufferMaxLength == 0)          ||
        (pui32BufferLength == 0)            ||
        (sTextRecord.pui8Text == 0)         ||
        (sTextRecord.ui32TextLength == 0)   ||
        (sTextRecord.ui32TextLength > ui16BufferMaxLength))
    {
        DebugPrintf("ERR: NDEFTextRecordEncoder: Invalid Input\n");
        return STATUS_FAIL;
    }

    //
    // Fill StatusByte in buffer
    //
    pui8Buffer[ui32RecordIndex] =
        (
        NDEF_TEXTRECORD_STATUSBYTE_SET_UTF(sTextRecord.sStatusByte.bUTFcode) |
        NDEF_TEXTRECORD_STATUSBYTE_SET_RFU(sTextRecord.sStatusByte.bRFU    ) |
        NDEF_TEXTRECORD_STATUSBYTE_SET_LENGTHLANGCODE(
                                    sTextRecord.sStatusByte.ui5LengthLangCode)
        );
    ui32RecordIndex++;

    //
    // Validate LanguageCode Length
    //
    if(sTextRecord.sStatusByte.ui5LengthLangCode >
        NDEF_TEXTRECORD_LANGUAGECODE_MAXSIZE)
    {
        ASSERT(0);
        DebugPrintf("Err: TextRecordEncoder: ui5LengthLanguageCode > ");
        DebugPrintf("NDEF_TEXTRECORD_LANGUAGECODE_MAXSIZE\n");
        DebugPrintf("\t Truncating from %d to MaxSize of %d.\n",
                        sTextRecord.sStatusByte.ui5LengthLangCode,
                        NDEF_TEXTRECORD_LANGUAGECODE_MAXSIZE);
    }

    //
    // Fill LanguageCode in buffer
    //
    for(x = 0;x < sTextRecord.sStatusByte.ui5LengthLangCode;x++)
    {
        pui8Buffer[ui32RecordIndex] = sTextRecord.pui8LanguageCode[x];
        ui32RecordIndex++;
    }

    //
    // Error Check
    //
    if((ui32RecordIndex + sTextRecord.ui32TextLength) > ui16BufferMaxLength)
    {
        ASSERT(0);
        DebugPrintf("ERR: NDEFTextRecordEncode: Buffer Overflow Immenant\n");
        return STATUS_FAIL;
    }

    //
    // Fill Text in buffer
    //
    for(x = 0;x < sTextRecord.ui32TextLength;x++)
    {
        pui8Buffer[ui32RecordIndex] = sTextRecord.pui8Text[x];
        ui32RecordIndex++;
    }

    //
    // Set buffer length
    //
    *pui32BufferLength = ui32RecordIndex;

    return STATUS_SUCCESS;
}

//*****************************************************************************
//
//! Decode NDEF Text Records.
//!
//! \param psTextDataDecoded is a pointer to the TextRecord structure to decode
//!  the data into.
//! \param pui8Buffer is a pointer to the raw NFC data buffer to be decoded.
//! \param ui32BufferLength is the length of the raw NFC data buffer.
//!
//! This function takes a raw NFC data buffer and decodes the data into a Text
//! record data structure. It is assumed that the raw data buffer contains a
//! text record.
//!
//! \return This function returns \b STATUS_SUCCESS (1) or \b STATUS_FAIL (0).
//
//*****************************************************************************
bool
NFCP2P_NDEFTextRecordDecoder(sNDEFTextRecord *psTextDataDecoded,
                                uint8_t *pui8Buffer,
                                uint32_t ui32BufferLength)
{
    sNDEFTextRecord *psTextRecord;
    uint8_t ui8StatusByte, ui8LengthLangCode, x = 0;
    uint32_t ui32RecordIndex = 0;

    //
    // Validate Input
    //
    ASSERT(pui8Buffer != 0);
    ASSERT(psTextDataDecoded != 0);
    if(
        (pui8Buffer == 0)           ||
        (psTextDataDecoded == 0)
      )
    {
        DebugPrintf("ERR: TextRecordDecoder: Invalid Input\n");
        return STATUS_FAIL;
    }

    //
    // Initialize (done to insure 0 as sentinel in language Code)
    //
    psTextRecord = psTextDataDecoded;
    for(x = 0;x < NDEF_TEXTRECORD_LANGUAGECODE_MAXSIZE;x++)
    {
        psTextRecord->pui8LanguageCode[x] = 0;
    }
    psTextRecord->ui32TextLength = 0;

    //
    // Load STATUSBYTE field
    //
    ui8StatusByte = pui8Buffer[ui32RecordIndex];
    psTextRecord->sStatusByte.bUTFcode =
                            NDEF_TEXTRECORD_STATUSBYTE_GET_UTF(ui8StatusByte);
    psTextRecord->sStatusByte.bRFU =
                            NDEF_TEXTRECORD_STATUSBYTE_GET_RFU(ui8StatusByte);
    ui8LengthLangCode =
                NDEF_TEXTRECORD_STATUSBYTE_GET_LENGTHLANGCODE(ui8StatusByte);
    psTextRecord->sStatusByte.ui5LengthLangCode = ui8LengthLangCode;
    ui32RecordIndex++;

    //
    // The StatusByte.RFU should always be 0, if this is not the case return
    //  failure
    //
    if(psTextRecord->sStatusByte.bRFU != 0)
    {
        ASSERT(0);
        DebugPrintf("Err: NDEF TextRecord Decoder: StatusByte.RFU !=0\n");
        return STATUS_FAIL;
    }

    //
    // LengthLangCode must be > 0
    //
    if(ui8LengthLangCode <= 0)
    {
        ASSERT(0);
        DebugPrintf("ERR: NDEFTextRecordDecoder: LengthLangCode <= 0\n");
        return STATUS_FAIL;

    }

    //
    // Load LANGUAGE_CODE field
    //
    for(x = 0;x < ui8LengthLangCode;x++)
    {
        //
        // If space left in LanguageCode field put character in, otherwise
        // truncate. (dont copy across, but do incriment through raw buffer)
        //
        if(x < NDEF_TEXTRECORD_LANGUAGECODE_MAXSIZE)
        {
            psTextRecord->pui8LanguageCode[x] = pui8Buffer[ui32RecordIndex];
            ui32RecordIndex++;
        }
        else
        {
            ui32RecordIndex++;
        }
    }

    //
    // Validate Data
    //
    if(ui8LengthLangCode > NDEF_TEXTRECORD_LANGUAGECODE_MAXSIZE)
    {
        DebugPrintf("ERR: TextRecordDecoder: LengthLangCode > ");
        DebugPrintf("NDEF_TEXTRECORD_LANGUAGECODE_MAXSIZE, truncating %d to %d",
            ui8LengthLangCode,NDEF_TEXTRECORD_LANGUAGECODE_MAXSIZE);
        DebugPrintf("\n");
        psTextRecord->sStatusByte.ui5LengthLangCode =
                                NDEF_TEXTRECORD_LANGUAGECODE_MAXSIZE;
    }

    //
    // Load pointer to Text
    //
    psTextRecord->pui8Text = pui8Buffer + ui32RecordIndex;

    //
    // Validate Data - make sure we dont overrun the buffer
    //
    if(ui32RecordIndex > ui32BufferLength)
    {
        ASSERT(0);
        DebugPrintf("ERR: TextRecordDecoder: Text Length longer than payload.");
        DebugPrintf("\n");
        return STATUS_FAIL;
    }
    else
    {
        //
        // Calculate Length of Text
        // Length of text =  Length of Record - RecordIndex to this point.
        //
        psTextRecord->ui32TextLength = ui32BufferLength-ui32RecordIndex;
    }

    return STATUS_SUCCESS;
}

//*****************************************************************************
//
//! Encode NDEF URI Records.
//!
//! \param sURIRecord is the URI Record Structure to be encoded.
//! \param pui8Buffer is a pointer to the buffer to fill with the raw NFC data.
//! \param ui16BufferMaxLength is the maximum number of bytes the buffer
//! can hold. This parameter is used to prevent writing past the end of the
//! buffer.
//! \param pui32BufferLength is a pointer to the integer to hold the length of
//! the raw NFC data buffer.
//!
//! This function takes a URI Record structure and encodes it into a provided
//! buffer in a raw NFC data format. The length of the data stored in the buffer
//! is stored in \e \b pui32BufferLength.
//!
//! \return This function returns \b STATUS_SUCCESS (1) or \b STATUS_FAIL (0).
//
//*****************************************************************************
bool
NFCP2P_NDEFURIRecordEncoder(sNDEFURIRecord sURIRecord,
                                uint8_t *pui8Buffer,
                                uint16_t ui16BufferMaxLength,
                                uint32_t *pui32BufferLength)
{
    uint32_t ui32RecordIndex = 0;
    uint8_t x = 0;

    //
    // Validate Input
    //
    ASSERT(pui8Buffer != 0);
    ASSERT(ui16BufferMaxLength !=0);
    ASSERT((sURIRecord.ui32URILength +1) < ui16BufferMaxLength);
    if(
        (pui8Buffer == 0) ||
        (ui16BufferMaxLength ==0) ||
        ((sURIRecord.ui32URILength +1) > ui16BufferMaxLength)
      )
    {
        ASSERT(0);
        DebugPrintf("ERR: URIRecordEncoder: Invalid Input\n");
        return STATUS_FAIL;
    }

    //
    // Fill IDCode field in buffer
    //
    pui8Buffer[ui32RecordIndex] = sURIRecord.eIDCode;
    ui32RecordIndex++;

    //
    // Fill UTF8 string into buffer
    //
    for(x = 0;x < sURIRecord.ui32URILength;x++)
    {
        pui8Buffer[ui32RecordIndex] = sURIRecord.puiUTF8String[x];
        ui32RecordIndex++;
    }

    //
    // Set Buffer Length
    //
    *pui32BufferLength = ui32RecordIndex;

    return STATUS_SUCCESS;

}

//*****************************************************************************
//
//! Decode NDEF URI Records.
//!
//! \param sURIRecord is a pointer to the URIRecord structure into which to
//! decode the data.
//! \param pui8Buffer is a pointer to the raw NFC data buffer to be decoded.
//! \param ui32BufferLength is the length of the raw NFC data buffer.
//!
//! This function takes a raw NFC data buffer and decodes the data into a URI
//! record data structure. It is assumed that the raw data buffer contains a
//! URI record.
//!
//! \return This function returns \b STATUS_SUCCESS (1) or \b STATUS_FAIL (0).
//
//*****************************************************************************
bool
NFCP2P_NDEFURIRecordDecoder(sNDEFURIRecord *sURIRecord,
                                uint8_t *pui8Buffer,
                                uint32_t ui32BufferLength)
{
    uint32_t ui32RecordIndex = 0;

    //
    // Validate Input
    //
    ASSERT(pui8Buffer != 0);
    ASSERT(sURIRecord != 0);
    if(
        (pui8Buffer == 0)           ||
        (sURIRecord == 0)
      )
    {
        DebugPrintf("ERR: URIRecordDecoder: Invalid Input\n");
        return STATUS_FAIL;
    }

    //
    // Load eIDCode field into struct
    // error check that the ID code is valid.
    //
    if(pui8Buffer[ui32RecordIndex] >= NDEF_URIRECORD_IDCODE_RFU)
    {
        //
        // IDCode not recognized, skip it.
        // (can add codes in nfc_p2p.h eNDEF_URIRecord_IDCode enumeration)
        //
        DebugPrintf("ERR: URI Record Decoder: URI ID Code Not Recognized: 0x%x\n"
                    ,pui8Buffer[ui32RecordIndex]);
        sURIRecord->eIDCode = RFU;
        ui32RecordIndex++;
        //return STATUS_FAIL;
    }
    else
    {
        //
        // ID Code is Valid, set it.
        //
        sURIRecord->eIDCode = pui8Buffer[ui32RecordIndex];
        ui32RecordIndex++;
    }

    //
    // Load UTF8 String Pointer into struct
    //
    sURIRecord->puiUTF8String = pui8Buffer + ui32RecordIndex;

    //
    // Load URI string Length into struct
    //
    sURIRecord->ui32URILength = ui32BufferLength-ui32RecordIndex;

    return STATUS_SUCCESS;
}

//*****************************************************************************
//
//! Encode NDEF SmartPoster Records.
//!
//! \param sSmartPoster is the SmartPoster Record Structure to be encoded.
//! \param pui8Buffer is a pointer to the buffer to fill with the raw NFC data.
//! \param ui16BufferMaxLength is the maximum number of bytes the buffer
//! can hold. This parameter is used to prevent writing past the end of the
//! buffer.
//! \param pui32BufferLength is a pointer to the integer to hold the length of
//! the raw NFC data buffer.
//!
//! This function takes a SmartPoster record structure and encodes it into a
//! provided buffer in a raw NFC data format. The length of the data stored in
//! the buffer is stored in \e \b pui32BufferLength.
//!
//! \note It is assumed that all smart poster messages have a Text record and a
//!       URI record.
//!
//! \return This function returns \b STATUS_SUCCESS (1) or \b STATUS_FAIL (0).
//
// Note: This function works by first encoding the Record, then the Header.
//       The Header comes before the Record. Thus space is allocated in the
//       buffer for the Header before the buffer is passed to the encoder. The
//       extra space will be taken care of by the Header encoder function
//       (aka NDEFMessageEncoder).
//
//
//*****************************************************************************
bool
NFCP2P_NDEFSmartPosterRecordEncoder(sNDEFSmartPosterRecord sSmartPoster,
                                    uint8_t *pui8Buffer,
                                    uint16_t ui16BufferMaxLength,
                                    uint32_t *pui32BufferLength)
{
    //
    // RECORD_OFFSET is the max size of the header. The magic number 7 comes
    // from the size of the Statusbyte[1]+PayloadLength[4]+IDLength[1]+
    // TypeLength[1]. This is done to ensure that there is space
    // left in the buffer for the header while the record is encoding.
    //
    #define RECORD_OFFSET (NDEF_TYPE_MAXSIZE+NDEF_ID_MAXSIZE+7)

    bool bStatus = STATUS_SUCCESS;

    uint32_t ui32TotalLength = 0;
    uint32_t ui32RecordLength = 0;

    uint8_t *pui8CurrHeaderPt = pui8Buffer;
    uint8_t *pui8CurrRecordPt = pui8CurrHeaderPt + RECORD_OFFSET;

    //
    // Validate Data
    //
    ASSERT(ui16BufferMaxLength != 0);
    ASSERT(pui8Buffer != 0);

    //
    // Encode TextMessage, Update Payload Ptr and Payload Length in Header,
    // Encode TextHeader (included TextPayload)
    //
    bStatus = NFCP2P_NDEFTextRecordEncoder(sSmartPoster.sTextPayload,
                                            pui8CurrRecordPt,
                                            (ui16BufferMaxLength -
                                               (pui8CurrRecordPt - pui8Buffer)),
                                            &ui32RecordLength);
    sSmartPoster.sTextHeader.ui32PayloadLength = ui32RecordLength;
    sSmartPoster.sTextHeader.pui8PayloadPtr = pui8CurrRecordPt;
    if(STATUS_FAIL == bStatus)
    {
        DebugPrintf("    ERR: SmartPoster TextRecord Encode FAIL.\n");
        return bStatus;
    }

    bStatus = NFCP2P_NDEFMessageEncoder(sSmartPoster.sTextHeader,
                                        pui8CurrHeaderPt,
                                        (ui16BufferMaxLength -
                                            (pui8CurrHeaderPt - pui8Buffer)),
                                        &ui32RecordLength);
    pui8CurrHeaderPt = pui8CurrHeaderPt + ui32RecordLength;
    pui8CurrRecordPt = pui8CurrHeaderPt + RECORD_OFFSET;
    if(STATUS_FAIL == bStatus)
    {
        DebugPrintf("    ERR: SmartPoster TextRecord Header Encode FAIL.\n");
        return bStatus;
    }
    ui32TotalLength += ui32RecordLength;

    //
    // Encode URIMessage, Update Payload Ptr and Payload Length in Header,
    // Encode URIHeader (included URIPayload)
    //
    bStatus = NFCP2P_NDEFURIRecordEncoder(sSmartPoster.sURIPayload,
                                        pui8CurrRecordPt,
                                        (ui16BufferMaxLength -
                                            (pui8CurrRecordPt - pui8Buffer)),
                                        &ui32RecordLength);
    sSmartPoster.sURIHeader.ui32PayloadLength = ui32RecordLength;
    sSmartPoster.sURIHeader.pui8PayloadPtr = pui8CurrRecordPt;
    if(STATUS_FAIL == bStatus)
    {
        DebugPrintf("    ERR: SmartPoster URIRecord Encode FAIL.\n");
        return bStatus;
    }
    bStatus = NFCP2P_NDEFMessageEncoder(sSmartPoster.sURIHeader,
                                        pui8CurrHeaderPt,
                                        (ui16BufferMaxLength -
                                            (pui8CurrHeaderPt - pui8Buffer)),
                                        &ui32RecordLength);
    pui8CurrHeaderPt = pui8CurrHeaderPt + ui32RecordLength;
    pui8CurrRecordPt = pui8CurrHeaderPt + RECORD_OFFSET;
    ui32TotalLength += ui32RecordLength;
    if(STATUS_FAIL == bStatus)
    {
        DebugPrintf("    ERR: SmartPoster URIRecord Header Encode FAIL.\n");
        return bStatus;
    }

    //
    // Encode ActionMessage, Update Payload Ptr and Payload Length in Header,
    // Encode ActionHeader (included ActionPayload)
    //
    if(sSmartPoster.bActionExists)
    {
        //
        // The Action Record has no Encoder / Decoder because it is just 1 byte
        // of data. So it is hard coded into the Smart Poster Encoder / Decoder
        //
        pui8CurrRecordPt[0] = sSmartPoster.sActionPayload.eAction;
        sSmartPoster.sActionHeader.ui32PayloadLength = 1;
        sSmartPoster.sActionHeader.pui8PayloadPtr = pui8CurrRecordPt;
        bStatus = NFCP2P_NDEFMessageEncoder(sSmartPoster.sActionHeader,
                                            pui8CurrHeaderPt,
                                            (ui16BufferMaxLength -
                                               (pui8CurrHeaderPt - pui8Buffer)),
                                            &ui32RecordLength);
        pui8CurrHeaderPt = pui8CurrHeaderPt + ui32RecordLength;
        pui8CurrRecordPt = pui8CurrHeaderPt + RECORD_OFFSET;
        ui32TotalLength += ui32RecordLength;
        if(STATUS_FAIL == bStatus)
        {
            DebugPrintf("    ERR: SmartPoster ActionRecord Encode FAIL.\n");
            return bStatus;
        }
    }

    //
    // Check for buffer overflow. This should be caught in the lower level
    // encode functions, but just to be safe we check for it here as well.
    //
    if(ui32TotalLength > ui16BufferMaxLength)
    {
        DebugPrintf("   ERR: SmartPosterRecordEncoder : Buffer Overflow.\n");
        return STATUS_FAIL;
    }

    //
    // Return Buffer Length
    //
    *pui32BufferLength = ui32TotalLength;

    return STATUS_SUCCESS;
}

//*****************************************************************************
//
//! Decode NDEF SmartPoster Records.
//!
//! \param sSmartPoster is a pointer to the SmartPosterRecord structure into
//! which to decode the data.
//! \param pui8Buffer is a pointer to the raw NFC data buffer to be decoded.
//! \param ui16BufferMaxLength is the maximum number of bytes the buffer
//! can hold. This parameter is used to prevent reading past the end of the
//! buffer.
//! \param ui32BufferLength is the length of the raw NFC data buffer.
//!
//! This function takes a raw NFC data buffer and decodes the data into a
//! SmartPoster record data structure. It is assumed that the raw data buffer
//! contains a SmartPoster record.
//!
//! \return This function returns \b STATUS_SUCCESS (1) or \b STATUS_FAIL (0).
//!
//! \note Currently only Title, Action and URI records are supported.
//!       Other records are skipped and ignored.
//
//*****************************************************************************
bool
NFCP2P_NDEFSmartPosterRecordDecoder(sNDEFSmartPosterRecord *sSmartPoster,
                                uint8_t *pui8Buffer,
                                uint16_t ui16BufferMaxLength,
                                uint32_t ui32BufferLength)
{
    sNDEFMessageData sCurrentHeader; //temp Header Info
    uint32_t ui32RecordIndex = 0;
    uint8_t *pui8CurrHeaderPt;
    uint8_t x = 0;
    bool bCheck = STATUS_SUCCESS;
    uint64_t TypeID = 0;

    //
    // Initialize
    //
    sSmartPoster->bActionExists = false;

    //
    // Process through Payload for Smart Poster.
    // Assume first header at pui8Buffer[0]
    // Process and fill
    //
    while(ui32RecordIndex < ui32BufferLength)
    {
        //
        // Pointer to Header
        //
        pui8CurrHeaderPt = pui8Buffer+ui32RecordIndex;

        //
        // Decode Current Header, in this case the
        //
        bCheck = NFCP2P_NDEFMessageDecoder(&sCurrentHeader,
                                            pui8CurrHeaderPt,
                                            (ui16BufferMaxLength -
                                                (pui8CurrHeaderPt - pui8Buffer))
                                            );
        if(STATUS_FAIL == bCheck)
        {
            DebugPrintf("ERR: SPDecoder: SP NDEFMessageDecoder Failed\n");
            return STATUS_FAIL;
        }
        //
        // Check for buffer read overrun. This would be caused by bad data.
        // This goes off when you try to read past the end of the buffer.
        //
        if((sCurrentHeader.ui32PayloadLength +
            (sCurrentHeader.pui8PayloadPtr - pui8Buffer))
            > ui16BufferMaxLength)
        {
            DebugPrintf("ERR: SPDecoder: BufferRead Overrun. Bad Data.");
            return STATUS_FAIL;
        }

        //
        // Calculate Record Type
        //
        for(x = 0,TypeID = 0;x < sCurrentHeader.ui8TypeLength;x++)
        {
            TypeID = (TypeID << 8) + sCurrentHeader.pui8Type[x];
        }

        //
        // Decode Header into appropriate part of SmartPoster struct
        // Call decoder function for each header type
        //
        switch(TypeID)
        {
            //
            // Text Record
            //
            case NDEF_TYPE_TEXT:
            {
                bCheck = NFCP2P_NDEFMessageDecoder(&sSmartPoster->sTextHeader,
                                            pui8CurrHeaderPt,
                                            (ui16BufferMaxLength -
                                               (pui8CurrHeaderPt - pui8Buffer))
                                            );
                if(STATUS_FAIL == bCheck)
                {
                    DebugPrintf(
                        "   ERR: SPDecoder: Text NDEFMessageDecoder Failed\n");
                    return STATUS_FAIL;
                }
                bCheck = NFCP2P_NDEFTextRecordDecoder(
                                    &sSmartPoster->sTextPayload,
                                    sSmartPoster->sTextHeader.pui8PayloadPtr,
                                    sSmartPoster->sTextHeader.ui32PayloadLength
                                    );
                if(STATUS_FAIL == bCheck)
                {
                    DebugPrintf(
                        "   ERR: SPDecoder: NDEFTextRecordDecoder Failed\n");
                    return STATUS_FAIL;
                }
                break;
            }

            //
            // URI Record
            //
            case NDEF_TYPE_URI:
            {
                bCheck = NFCP2P_NDEFMessageDecoder(&sSmartPoster->sURIHeader,
                                            pui8CurrHeaderPt,
                                            (ui16BufferMaxLength -
                                               (pui8CurrHeaderPt - pui8Buffer))
                                            );
                if(STATUS_FAIL == bCheck)
                {
                    DebugPrintf(
                        "   ERR: SPDecoder: URI NDEFMessageDecoder Failed\n");
                    return STATUS_FAIL;
                }
                bCheck = NFCP2P_NDEFURIRecordDecoder(
                                &sSmartPoster->sURIPayload,
                                sSmartPoster->sURIHeader.pui8PayloadPtr,
                                sSmartPoster->sURIHeader.ui32PayloadLength
                                );
                if(STATUS_FAIL == bCheck)
                {
                    DebugPrintf(\
                        "   ERR: SPDecoder: NDEFURIMessageDecoder Failed\n");
                    return STATUS_FAIL;
                }
                break;
            }

            //
            // Action Record (built in type to SmartPoster, no need for external
            // functions)
            //
            case NDEF_TYPE_ACTION:
            {
                sSmartPoster->bActionExists = true;
                bCheck = NFCP2P_NDEFMessageDecoder(&sSmartPoster->sActionHeader,
                                            pui8CurrHeaderPt,
                                            (ui16BufferMaxLength -
                                                (pui8CurrHeaderPt - pui8Buffer))
                                            );
                if(STATUS_FAIL == bCheck)
                {
                    DebugPrintf(
                        "   ERR: SPDecoder: Action NDEFMessageDecoder Failed\n");
                    return STATUS_FAIL;
                }
                sSmartPoster->sActionPayload.eAction =
                                sSmartPoster->sActionHeader.pui8PayloadPtr[0];
                break;
            }

            //
            // Other record type, not supported, so skip it.
            //
            default:
            {
                DebugPrintf("NDEFSmartPosterDecode: Record Not recognized: 0x%x\n"
                                ,TypeID);
                break;
            }
        }

        //
        // Incriment ui32RecordIndex
        // (sCurrentHeader.pui8PayloadPtr-pui8CurrHeaderPt) = size of header
        //  when added to payload length this gives the total record size
        //
        ui32RecordIndex += (sCurrentHeader.pui8PayloadPtr-pui8CurrHeaderPt)
                         + sCurrentHeader.ui32PayloadLength;
    }

    return STATUS_SUCCESS;
}


//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
