//*****************************************************************************
//
// snep.h - Simple NDEF Exchange Protocol deffinitions
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
#ifndef __NFC_SNEP_H__
#define __NFC_SNEP_H__

#include "types.h"

//*****************************************************************************
//
//! \addtogroup nfc_snep_api NFC SNEP API Functions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// This size is used to limit the maximum size of the incoming NDEF message.
// The maximum is dependent on the Maximum Information Units (MIU) defined in
// the LLCP layer.
// For example for MIU = 248, SNEP_MAX_BUFFER = 248
//
//*****************************************************************************
//
//! This is the maximum size of a fragment that is sent/received.
//
#define SNEP_MAX_BUFFER             248

//
//! Maximum size of the incoming payload.
//
#define SNEP_MAX_PAYLOAD			20000

//
//! Simple NDEF protocol version specified in the specification.
//
#define SNEP_VERSION                0x10

//*****************************************************************************
//
// List of SNEP Commands
//
//*****************************************************************************

//*****************************************************************************
//
//! SNEPCommand request / responses enumeration.
//
//*****************************************************************************
typedef enum
{
    //
    // SNEP request field value
    //

    //! See SNEP V1.0 Section 4.1
    SNEP_REQUEST_CONTINUE =          0x00,

    //! See SNEP V1.0 Section 4.2
    SNEP_REQUEST_GET =               0x01,

    //! See SNEP V1.0 Section 4.3
    SNEP_REQUEST_PUT =               0x02,
    // 03h-7Eh Reserved for future use

    //! See SNEP V1.0 Section 4.4
    SNEP_REQUEST_REJECT =            0x7F,
    // 80h-FFh Reserved for response field values

    //
    // See SNEP Response Field Values
    //
    // 00h-7Fh Reserved for request field values

    //! See SNEP V1.0 Section 5.1
    SNEP_RESPONSE_CONTINUE =         0x80,

    //! See SNEP V1.0 Section 5.2
    SNEP_RESPONSE_SUCCESS =          0x81,

    //! See SNEP V1.0 Section 5.3
    SNEP_RESPONSE_NOT_FOUND =        0xC0,

    //! See SNEP V1.0 Section 5.4
    SNEP_RESPONSE_EXCESS_DATA =      0xC1,

    //! See SNEP V1.0 Section 5.5
    SNEP_RESPONSE_BAD_REQUEST =      0xC2,

    //! See SNEP V1.0 Section 5.6
    SNEP_RESPONSE_NOT_IMPLEMENTED =  0xE0,

    //! See SNEP V1.0 Section 5.7
    SNEP_RESPONSE_UNSUPPORTED_VER =  0xE1,

    //! See SNEP V1.0 Section 5.8
    SNEP_RESPONSE_REJECT =           0xFF
}tSNEPCommands;

//*****************************************************************************
//
//! SNEP Connection Status Enumeration.
//
//*****************************************************************************
typedef enum
{
    //! No ongoing transaction to/from the client
    SNEP_CONNECTION_IDLE                    = 0x00,

    //! Wrong version received
    SNEP_WRONG_VERSION_RECEIVED,

    //! Received first fragment
    SNEP_CONNECTION_RECEIVED_FIRST_PACKET,

    //! Received n fragment
    SNEP_CONNECTION_RECEIVING_N_FRAGMENTS,

    //! Waiting for continue response
    SNEP_CONNECTION_WAITING_FOR_CONTINUE,

    //! Waiting for success response
    SNEP_CONNECTION_WAITING_FOR_SUCCESS,

    //! Sending n fragment
    SNEP_CONNECTION_SENDING_N_FRAGMENTS,

    //! Send completed
    SNEP_CONNECTION_SEND_COMPLETE,

    //! Receive completed
    SNEP_CONNECTION_RECEIVE_COMPLETE,

    //! Received excess size request.
    SNEP_CONNECTION_EXCESS_SIZE
}tSNEPConnectionStatus;

//*****************************************************************************
//
//! RX packet status enumeration.
//
//*****************************************************************************
typedef enum
{
    //
    //! No pending received data
    //
    RECEIVED_NO_FRAGMENT = 0,

    //
    //! First fragment received from the client
    //
    RECEIVED_FIRST_FRAGMENT,

    //
    //! N fragment received from the client
    RECEIVED_N_FRAGMENT,

    //! Last fragment received from the client - packet completed
    RECEIVED_FRAGMENT_COMPLETED
}tPacketStatus;

//*****************************************************************************
//
// Function Prototypes
//
//*****************************************************************************
void SNEP_init(void);
void SNEP_setMaxPayload(uint8_t ui8MaxPayload);
tStatus SNEP_setupPacket(uint8_t * pui8PacketPtr, uint32_t ui32PacketLength);
uint8_t SNEP_sendRequest(uint8_t * pui8DataPtr, tSNEPCommands eRequestCmd);
uint8_t SNEP_sendResponse(uint8_t * pui8DataPtr, tSNEPCommands eResponseCmd);
void SNEP_processReceivedData(uint8_t * pui8RxBuffer, uint8_t ui8RxLength);
void SNEP_getReceiveStatus(tPacketStatus * peReceiveFlag, uint8_t * length,
                            uint8_t ** pui8DataPtr);
tSNEPConnectionStatus SNEP_getProtocolStatus(void);
void SNEP_setProtocolStatus(tSNEPConnectionStatus eProtocolStatus);

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif // __NFC_SNEP_H__
