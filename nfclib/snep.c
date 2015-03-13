//*****************************************************************************
//
// snep.c - implementation of Simple NDEF Exchange Protocol, uses LLCP
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
#include "nfclib/snep.h"
#include "nfclib/debug.h"

//*****************************************************************************
//
//! \addtogroup nfc_snep_api NFC SNEP API Functions
//! @{
//! Simple NDEF Exchange Protocol is an application protocol used by the LLCP
//! layer to send / receive NDEFs between two NFC Forum Devices operating
//! in Peer-to-Peer Mode (1 Target and 1 Initiator). For more information
//! on SNEP, please read the NFC Simple NDEF Exchange Protocol Specification
//! Version 1.0.
//
//*****************************************************************************

//*****************************************************************************
//
// Stores the length of the Tx/Rx packet.
//
//****************************************************************************
uint32_t g_ui32SNEPPacketLength;

//*****************************************************************************
//
// g_pui8SNEPTxPacketPtr points to the first location of the data to be
// transferred
//
//*****************************************************************************
uint8_t * g_pui8SNEPTxPacketPtr;
uint8_t * g_pui8SNEPRxPacketPtr;

//*****************************************************************************
//
// Stores the remaining rx byte count
//
//*****************************************************************************
uint32_t g_ui32SNEPRemainingRxPayloadBytes = 0;

//*****************************************************************************
//
// Stores the bytes received in the current I-PDU transaction
//
//*****************************************************************************
uint8_t g_ui8SNEPReceivedBytes = 0;

//*****************************************************************************
//
// Stores the status of the incoming packet
//
//*****************************************************************************
tPacketStatus g_eRxPacketStatus = RECEIVED_NO_FRAGMENT;

//*****************************************************************************
//
// Stores the status of the SNEP communication
//
//*****************************************************************************
tSNEPConnectionStatus g_eSNEPConnectionStatus = SNEP_CONNECTION_IDLE;
//*****************************************************************************
//
// Stores the maximum size of each SNEP packet.
//
//*****************************************************************************
uint8_t g_ui8MaxPayload = SNEP_MAX_BUFFER;

//*****************************************************************************
//
// Stores the index of the current transaction
//
//*****************************************************************************
uint32_t g_ui32TxIndex = 0;

//*****************************************************************************
//
//! Initialize the Simple NDEF Exchange Protocol driver.
//!
//! This function must be called prior to any other function offered by the
//! SNEP driver. This function initializes the SNEP status, Tx/Rx packet length
//! and maximum payload size. This function must be called by the LLCP_init().
//!
//! \return None.
//
//*****************************************************************************
void SNEP_init(void)
{
    g_eSNEPConnectionStatus = SNEP_CONNECTION_IDLE;
    g_eRxPacketStatus = RECEIVED_NO_FRAGMENT;
    g_ui32SNEPRemainingRxPayloadBytes = 0;
    g_ui8SNEPReceivedBytes = 0;
	g_ui8MaxPayload = SNEP_MAX_BUFFER;
	g_ui32TxIndex = 0;
}

//*****************************************************************************
//
//! Set the Maximum size of each fragment.
//!
//! \param ui8MaxPayload is the maximum size of each fragment.
//!
//! This function must be called inside LLCP_processTLV() to define the maxium
//! size of each fragment based on the Maximum Information Unit (MIU) supported
//! by the target/initiator.
//!
//! \return None.
//
//*****************************************************************************
void SNEP_setMaxPayload(uint8_t ui8MaxPayload)
{
	if(ui8MaxPayload <= SNEP_MAX_BUFFER)
	{
		g_ui8MaxPayload = ui8MaxPayload;
		if(g_ui8MaxPayload == 0x80)
		{
			ui8MaxPayload = 0;
		}
	}
}

//*****************************************************************************
//
//! Set the global SNEP Packet Pointer and Length
//!
//! \param pui8PacketPtr is the pointer to the first payload to be transmitted.
//! \param ui32PacketLength is the length of the total packet.
//!
//! This function must be called by the main application to initialize the
//! packet to be sent to the SNEP server.
//!
//! \return This function returns \b STATUS_SUCCESS (1) if the packet was
//! queued, else it returns \b STATUS_FAIL (0).
//
//*****************************************************************************
tStatus SNEP_setupPacket(uint8_t * pui8PacketPtr, uint32_t ui32PacketLength)
{
    tStatus ePacketSetupStatus;

    if(g_eSNEPConnectionStatus == SNEP_CONNECTION_IDLE )
    {
        g_pui8SNEPTxPacketPtr = pui8PacketPtr;
		// Reset TX Index
		g_ui32TxIndex = 0;
        g_ui32SNEPPacketLength = ui32PacketLength;
        g_eSNEPConnectionStatus = SNEP_CONNECTION_IDLE;

        ePacketSetupStatus = STATUS_SUCCESS;
    }
    else
        ePacketSetupStatus = STATUS_FAIL;

    return ePacketSetupStatus;
}


//*****************************************************************************
//
//! Sends request to the server.
//!
//! \param pui8DataPtr is the start pointer where the request is written.
//! \param eRequestCmd is the request command to be sent.
//!
//! The \e eRequestCmd parameter can be any of the following:
//!
//! - \b SNEP_REQUEST_CONTINUE          - Send remaining fragments
//! - \b SNEP_REQUEST_GET               - Return an NDEF message
//! - \b SNEP_REQUEST_PUT               - Accept an NDEF message
//! - \b SNEP_REQUEST_REJECT            - Do not send remaining fragments
//!
//! This function sends an SNEP request from the SNEP client to an SNEP server.
//! It must be called from the LLCP_sendI() function.
//!
//! \return \b ui8offset, which is the length of the request written starting at
//!  \b pui8DataPtr.
//
//*****************************************************************************
uint8_t SNEP_sendRequest(uint8_t * pui8DataPtr, tSNEPCommands eRequestCmd)
{
    uint8_t ui8PacketLength;
    uint8_t ui8offset = 0;
    static uint8_t * pui8SNEPPacketPtr;
    volatile uint8_t ui8counter;

    switch(eRequestCmd)
    {
        case SNEP_REQUEST_CONTINUE:
        {
            if(g_eSNEPConnectionStatus == SNEP_CONNECTION_IDLE)
            break;
        }
        case SNEP_REQUEST_GET:
        {
            break;
        }
        case SNEP_REQUEST_PUT:
        {
            if(g_eSNEPConnectionStatus == SNEP_CONNECTION_IDLE)
            {
                //
                // Set sneP_packet_ptr to first address
                //
                pui8SNEPPacketPtr = g_pui8SNEPTxPacketPtr;

                //
                // SNEP Protocol Version
                //
                pui8DataPtr[ui8offset++] = SNEP_VERSION;

                //
                // Request Field
                //
                pui8DataPtr[ui8offset++] = (uint8_t) SNEP_REQUEST_PUT;

                //
                // Length (4 bytes)
                //
                pui8DataPtr[ui8offset++] =
                        (uint8_t) ((g_ui32SNEPPacketLength & 0xFF000000) >> 24);
                pui8DataPtr[ui8offset++] =
                        (uint8_t) ((g_ui32SNEPPacketLength & 0x00FF0000) >> 16);
                pui8DataPtr[ui8offset++] =
                        (uint8_t) ((g_ui32SNEPPacketLength & 0x0000FF00) >> 8);
                pui8DataPtr[ui8offset++] =
                        (uint8_t) (g_ui32SNEPPacketLength & 0x000000FF);

                //
                // The PUT Request has 6 bytes of overhead (Version (1) Request
                // Field (1) Length (4)).
                //
                if( g_ui32SNEPPacketLength > (g_ui8MaxPayload - 6))
                {
                    //
                    // Remaining bytes = Total Length - (SNEP_MAX_BUFFER - 13)
                    //
                    g_ui32SNEPPacketLength  = g_ui32SNEPPacketLength -
                                                    (g_ui8MaxPayload - 6);
                    ui8PacketLength = (g_ui8MaxPayload - 6);

                    //
                    // Change connection status to waiting for continue
                    //
                    g_eSNEPConnectionStatus =
                        SNEP_CONNECTION_WAITING_FOR_CONTINUE;
                }
                else
                {
                    ui8PacketLength = g_ui32SNEPPacketLength;
                    g_ui32SNEPPacketLength = 0;

                    //
                    // Change connection status to waiting for success
                    //
                    g_eSNEPConnectionStatus =
                        SNEP_CONNECTION_WAITING_FOR_SUCCESS;
                }

                //
                // Copy the snep_packet buffer into the pui8DataPtr
                //
                for(ui8counter = 0; ui8counter < ui8PacketLength; ui8counter++)
                {
                    pui8DataPtr[ui8offset++] =  pui8SNEPPacketPtr[g_ui32TxIndex++];

                }
            }
            else if(g_eSNEPConnectionStatus ==
                        SNEP_CONNECTION_SENDING_N_FRAGMENTS)
            {
                if( g_ui32SNEPPacketLength > g_ui8MaxPayload)
                {
                    //
                    // Remaining bytes = Total Length - SNEP_MAX_BUFFER
                    //
                    g_ui32SNEPPacketLength  = g_ui32SNEPPacketLength -
                                                    g_ui8MaxPayload;
                    ui8PacketLength = g_ui8MaxPayload;
                }
                else
                {
                	ui8PacketLength = g_ui32SNEPPacketLength;
                    //
                    // Remaining bytes = 0
                    //
                    g_ui32SNEPPacketLength = 0;
                	g_eSNEPConnectionStatus = SNEP_CONNECTION_WAITING_FOR_SUCCESS;
                }

                //
                // Copy the snep_packet buffer into the pui8DataPtr
                //
                for(ui8counter = 0; ui8counter < ui8PacketLength; ui8counter++)
                {
					pui8DataPtr[ui8offset++] =  pui8SNEPPacketPtr[g_ui32TxIndex++];
                }

            }
        break;
        }
        case SNEP_REQUEST_REJECT:
        {
            break;
        }
        default:
        {
            break;
        }
    }

    return ui8offset;
}

//*****************************************************************************
//
//! Sends response to the client.
//!
//! \param pui8DataPtr is the start pointer where the response is written.
//! \param eResponseCmd is the response command to be sent.
//!
//! The \e eResponseCmd parameter can be any of the following:
//!
//! - \b SNEP_RESPONSE_CONTINUE         - Continue send remaining fragments
//! - \b SNEP_RESPONSE_SUCCESS          - Operation succeeded
//! - \b SNEP_RESPONSE_NOT_FOUND        - Resource not found
//! - \b SNEP_RESPONSE_EXCESS_DATA      - Resource exceeds data size limit
//! - \b SNEP_RESPONSE_BAD_REQUEST      - Malformed request not understood
//! - \b SNEP_RESPONSE_NOT_IMPLEMENTED  - Unsupported functionality requested
//! - \b SNEP_RESPONSE_UNSUPPORTED_VER  - Unsupported protocol version
//! - \b SNEP_RESPONSE_REJECT           - Do not send remaining fragments
//!
//! This function sends an SNEP response from the SNEP server to an SNEP client.
//! It must be called from the LLCP_sendI() function.
//!
//! \return \b ui8offset is the length of the response written starting at
//! \b pui8DataPtr.
//
//*****************************************************************************
uint8_t SNEP_sendResponse(uint8_t * pui8DataPtr, tSNEPCommands eResponseCmd)
{
    uint8_t ui8offset = 0;

    switch(eResponseCmd)
    {
        case SNEP_RESPONSE_CONTINUE:
        {
            if(g_eSNEPConnectionStatus == SNEP_CONNECTION_RECEIVED_FIRST_PACKET)
            {
                //
                // SNEP Protocol Version
                //
                pui8DataPtr[ui8offset++] = SNEP_VERSION;

                //
                // Response Field
                //
                pui8DataPtr[ui8offset++] = (uint8_t) SNEP_RESPONSE_CONTINUE;

                //
                // Length (4 bytes)
                //
                pui8DataPtr[ui8offset++] = 0x00;
                pui8DataPtr[ui8offset++] = 0x00;
                pui8DataPtr[ui8offset++] = 0x00;
                pui8DataPtr[ui8offset++] = 0x00;
                g_eSNEPConnectionStatus = SNEP_CONNECTION_RECEIVING_N_FRAGMENTS;
            }
            break;
        }
        case SNEP_RESPONSE_SUCCESS:
        {
            if(g_eSNEPConnectionStatus ==  SNEP_CONNECTION_RECEIVE_COMPLETE)
            {
                //
                // SNEP Protocol Version
                //
                pui8DataPtr[ui8offset++] = SNEP_VERSION;

                //
                // Response Field
                //
                pui8DataPtr[ui8offset++] = (uint8_t) SNEP_RESPONSE_SUCCESS;

                //
                // Length (4 bytes)
                //
                pui8DataPtr[ui8offset++] = 0x00;
                pui8DataPtr[ui8offset++] = 0x00;
                pui8DataPtr[ui8offset++] = 0x00;
                pui8DataPtr[ui8offset++] = 0x00;
                g_eSNEPConnectionStatus = SNEP_CONNECTION_IDLE;
            }
            break;
        }
        case SNEP_RESPONSE_NOT_FOUND:
        {
            break;
        }
        case SNEP_RESPONSE_EXCESS_DATA:
        {
            break;
        }
        case SNEP_RESPONSE_BAD_REQUEST:
        {
            break;
        }
        case SNEP_RESPONSE_NOT_IMPLEMENTED:
        {
            break;
        }
        case SNEP_RESPONSE_UNSUPPORTED_VER:
        {
            break;
        }
        case SNEP_RESPONSE_REJECT:
        {
            if(g_eSNEPConnectionStatus ==  SNEP_CONNECTION_EXCESS_SIZE)
            {
                //
                // SNEP Protocol Version
                //
                pui8DataPtr[ui8offset++] = SNEP_VERSION;

                //
                // Response Field
                //
                pui8DataPtr[ui8offset++] = (uint8_t) SNEP_RESPONSE_REJECT;

                //
                // Length (4 bytes)
                //
                pui8DataPtr[ui8offset++] = 0x00;
                pui8DataPtr[ui8offset++] = 0x00;
                pui8DataPtr[ui8offset++] = 0x00;
                pui8DataPtr[ui8offset++] = 0x00;
                g_eSNEPConnectionStatus = SNEP_CONNECTION_IDLE;
            }
            break;
        }
        default:
        {
            break;
        }
    }
    return ui8offset;
}

//*****************************************************************************
//
//! Processes the data received from a client/server.
//!
//! \param pui8RxBuffer is the starting pointer of the SNEP request/response
//! received.
//! \param ui8RxLength is the length of the SNEP request/response received.
//!
//! This function handles the requests/responses received inside an I-PDU
//! in the LLCP layer. This function must be called inside
//! LLCP_processReceivedData().
//!
//! \return None
//
//*****************************************************************************
void SNEP_processReceivedData(uint8_t * pui8RxBuffer, uint8_t ui8RxLength)
{
    volatile uint8_t ui8SNEPversion;
    tSNEPCommands eCommandField;

    eCommandField = (tSNEPCommands) pui8RxBuffer[1];

    if((g_eSNEPConnectionStatus == SNEP_CONNECTION_RECEIVED_FIRST_PACKET) ||
        (g_eSNEPConnectionStatus == SNEP_CONNECTION_RECEIVING_N_FRAGMENTS))
    {
        if(g_ui32SNEPRemainingRxPayloadBytes > ui8RxLength)
        {
            g_ui8SNEPReceivedBytes = ui8RxLength;
            g_eSNEPConnectionStatus = SNEP_CONNECTION_RECEIVING_N_FRAGMENTS;
            g_eRxPacketStatus = RECEIVED_N_FRAGMENT;
        }
        else
        {
            g_ui8SNEPReceivedBytes = (uint8_t)g_ui32SNEPRemainingRxPayloadBytes;
            g_eSNEPConnectionStatus = SNEP_CONNECTION_RECEIVE_COMPLETE;
            g_eRxPacketStatus = RECEIVED_FRAGMENT_COMPLETED;
        }
        g_ui32SNEPRemainingRxPayloadBytes = g_ui32SNEPRemainingRxPayloadBytes -
                                                g_ui8SNEPReceivedBytes;
        g_pui8SNEPRxPacketPtr = &pui8RxBuffer[0];
    }
    else if(eCommandField >= 0x80)
    {
        //
        // Process Responses
        //
        switch(eCommandField)
        {
            case SNEP_RESPONSE_CONTINUE:
            {
                if(g_eSNEPConnectionStatus ==
                                        SNEP_CONNECTION_WAITING_FOR_CONTINUE)
                {
                    g_eSNEPConnectionStatus =
                                            SNEP_CONNECTION_SENDING_N_FRAGMENTS;
                }
                break;
            }
            case SNEP_RESPONSE_SUCCESS:
            {
                if(g_eSNEPConnectionStatus ==
                    SNEP_CONNECTION_WAITING_FOR_SUCCESS)
                {
                    g_eSNEPConnectionStatus = SNEP_CONNECTION_SEND_COMPLETE;
                }
                break;
            }
            case SNEP_RESPONSE_NOT_FOUND:
            {
                break;
            }
            case SNEP_RESPONSE_EXCESS_DATA:
            {
                break;
            }
            case SNEP_RESPONSE_BAD_REQUEST:
            {
                break;
            }
            case SNEP_RESPONSE_NOT_IMPLEMENTED:
            {
                break;
            }
            case SNEP_RESPONSE_UNSUPPORTED_VER:
            {
                break;
            }
            case SNEP_RESPONSE_REJECT:
            {
                break;
            }
            default :
            {
                break;
            }
        }
    }
    else
    {
        //
        // Process Requests
        //
        switch(eCommandField)
        {
            case SNEP_REQUEST_CONTINUE:
            {
                break;
            }
            case SNEP_REQUEST_GET:
            {
                break;
            }
            case SNEP_REQUEST_PUT:
            {
                ui8SNEPversion = pui8RxBuffer[0];
                if(ui8SNEPversion == SNEP_VERSION)
                {
                    //
                    // Update remaining payload bytes
                    //
                    g_ui32SNEPRemainingRxPayloadBytes =
                                    (
                                    (uint32_t) (pui8RxBuffer[5] & 0xFF)         +
                                    ((uint32_t) (pui8RxBuffer[4] & 0xFF) << 8)  +
                                    ((uint32_t) (pui8RxBuffer[3] & 0xFF) << 16) +
                                    ((uint32_t) (pui8RxBuffer[2] & 0xFF) << 24)
                                    );
                    if(g_ui32SNEPRemainingRxPayloadBytes > SNEP_MAX_PAYLOAD)
                    {
                        g_eSNEPConnectionStatus = SNEP_CONNECTION_EXCESS_SIZE;
                    }
                    else
                    {
                        if (g_ui32SNEPRemainingRxPayloadBytes
                                > (ui8RxLength - 6)) {
                            g_ui8SNEPReceivedBytes = (ui8RxLength - 6);
                            g_eSNEPConnectionStatus =
                                    SNEP_CONNECTION_RECEIVED_FIRST_PACKET;
                            g_eRxPacketStatus = RECEIVED_FIRST_FRAGMENT;
                        }
                        else
                        {
                            //
                            // Packet Length
                            //
                            g_ui8SNEPReceivedBytes =
                                    (uint8_t) g_ui32SNEPRemainingRxPayloadBytes;
                            g_eSNEPConnectionStatus =
                                    SNEP_CONNECTION_RECEIVE_COMPLETE;
                            g_eRxPacketStatus = RECEIVED_FRAGMENT_COMPLETED;
                        }
                        //
                        // Update remaining payload bytes
                        //
                        g_ui32SNEPRemainingRxPayloadBytes =
                                g_ui32SNEPRemainingRxPayloadBytes
                                        - g_ui8SNEPReceivedBytes;

                        //
                        // Set the g_pui8SNEPRxPacketPtr to the start of payload
                        //
                        g_pui8SNEPRxPacketPtr = &pui8RxBuffer[6];

                    }
                }
                else
                {
                    g_eSNEPConnectionStatus = SNEP_WRONG_VERSION_RECEIVED;
                }
                break;
            }
            case SNEP_REQUEST_REJECT:
            {
                break;
            }
            default :
            {
                break;
            }
        }
    }
}

//*****************************************************************************
//
//! Get RxStatus flag, Clear packet status flag,retrieve length of data and
//! retrieve data
//!
//! \param peReceiveFlag is a pointer to store the RX state status.
//! \param pui8length is a pointer to store the number of received bytes.
//! \param pui8DataPtr is a double pointer to store the pointer of data
//! received.
//!
//! The \e peReceiveFlag parameter can be any of the following:
//!
//! - \b RECEIVED_NO_FRAGMENT        - No Fragment has been received
//! - \b RECEIVED_FIRST_FRAGMENT     - First fragment has been received.
//! - \b RECEIVED_N_FRAGMENT         - N Fragment has been received.
//! - \b RECEIVED_FRAGMENT_COMPLETED - End of the fragment has been received.
//!
//! This function must be called in the main application after the
//! NFCP2P_proccessStateMachine() is called to ensure the data received is moved
//! to another buffer and handled when a fragment is received.
//!
//! \return None
//
//*****************************************************************************
void SNEP_getReceiveStatus(tPacketStatus * peReceiveFlag, uint8_t * pui8length,
     uint8_t ** pui8DataPtr)
{
    //
    // Save RX Packet Status Flag
    //
    *peReceiveFlag = g_eRxPacketStatus;

    //
    // Clear the packet status flag
    //
    g_eRxPacketStatus = RECEIVED_NO_FRAGMENT;

    //
    // Save Number of Byted received
    //
    *pui8length = g_ui8SNEPReceivedBytes;

    //
    // Set Data = ReceivedPacket
    //
    *pui8DataPtr = g_pui8SNEPRxPacketPtr;

    return;
}

//*****************************************************************************
//
//! Returns current SNEP Connection Status enumeration
//!
//! This function returns the current SNEP status flag. It must be called inside
//! LLCP_processReceivedData() to determine if further I-PDUs are required,
//! which is when there are requests/responses queued.
//!
//! \return g_eSNEPConnectionStatus the current connection status flag.
//
//*****************************************************************************
tSNEPConnectionStatus SNEP_getProtocolStatus(void)
{
    return g_eSNEPConnectionStatus;
}

//*****************************************************************************
//
//! Sets current SNEP Connection Status enumeration
//!
//! \param eProtocolStatus is the status flag used by the SNEP state machine
//! SNEP_processReceivedData() to send request/response.
//! New sent transactions are allowed only when eProtocolStatus is set
//! to \b SNEP_CONNECTION_IDLE.
//!
//! The \e eProtocolStatus parameter can be any of the following:
//!
//! - \b SNEP_CONNECTION_IDLE                   - No ongoing Tx/Rx
//! - \b SNEP_WRONG_VERSION_RECEIVED            - Wrong Version Received
//! - \b SNEP_CONNECTION_RECEIVED_FIRST_PACKET  - Received First Fragment
//! - \b SNEP_CONNECTION_RECEIVING_N_FRAGMENTS  - Received N Fragment
//! - \b SNEP_CONNECTION_WAITING_FOR_CONTINUE   - Waiting for Continue response
//! - \b SNEP_CONNECTION_WAITING_FOR_SUCCESS    - Waiting for Success response
//! - \b SNEP_CONNECTION_SENDING_N_FRAGMENTS    - Sending N Fragment
//! - \b SNEP_CONNECTION_SEND_COMPLETE          - Send Completed
//! - \b SNEP_CONNECTION_RECEIVE_COMPLETE       - Receive Completed
//! - \b SNEP_CONNECTION_EXCESS_SIZE            - Received Excess Size request
//!
//! This function is called inside LLCP_processReceivedData(), to set the
//! \e g_eSNEPConnectionStatus flag to \b SNEP_CONNECTION_IDLE after
//! a send transaction is completed to allow for further send transactions.
//!
//! \return None
//
//*****************************************************************************
void SNEP_setProtocolStatus(tSNEPConnectionStatus eProtocolStatus)
{
    g_eSNEPConnectionStatus = eProtocolStatus;
    return;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

