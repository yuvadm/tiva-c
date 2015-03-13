//*****************************************************************************
//
// llcp.h - Logic Link Control Protocol header file
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
#ifndef __NFC_LLCP_H__
#define __NFC_LLCP_H__

#include "types.h"

//*****************************************************************************
//
//! \addtogroup nfc_llcp_api NFC LLCP API Functions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// List of Commands
//
//*****************************************************************************

//
// ! LLCP Magic Number is constant 0x46666D
//
#define LLCP_MAGIC_NUMBER_HIGH          0x46
#define LLCP_MAGIC_NUMBER_MIDDLE        0x66
#define LLCP_MAGIC_NUMBER_LOW           0x6D

//*****************************************************************************
//
// Service in local service Environment and is NOT advertised by local SDP
//
//*****************************************************************************

//
//! Source Service Access Point when sending
//
#define LLCP_SSAP_CONNECT_SEND              0x20

//
//! Source Service Access Point when receiving
//
#define LLCP_SSAP_CONNECT_RECEIVED          0x04

//
//! Destination Service Access Point for discovery
//
#define DSAP_SERVICE_DISCOVERY_PROTOCOL     0x01

//
//! The LLCP_MIU is the maximum information unit supported by the LLCP layer.
//! This information unit may be included in each LLCP packet depending on the
//! PDU type. The minimum must be 128.
//
#define LLCP_MIU                            248

//
//! The LLCP_MIUX_SIZE is the value for the LLCP_MIUX TLV used in LLCP_addTLV().
//
#define LLCP_MIUX_SIZE                      (LLCP_MIU - 128)

//*****************************************************************************
//
//! LLCP Parameter Enumerations.
//
//*****************************************************************************
typedef enum
{
    //! See LLCP V1.1 Section 4.5.1
    LLCP_VERSION =   0x01,
    //! See LLCP V1.1 Section 4.5.2
    LLCP_MIUX =      0x02,
    //! See LLCP V1.1 Section 4.5.3
    LLCP_WKS =       0x03,
    //! See LLCP V1.1 Section 4.5.4
    LLCP_LTO =       0x04,
    //! See LLCP V1.1 Section 4.5.5
    LLCP_RW =        0x05,
    //! See LLCP V1.1 Section 4.5.6
    LLCP_SN =        0x06,
    //! See LLCP V1.1 Section 4.5.7
    LLCP_OPT =       0x07,
    //! See LLCP V1.1 Section 4.5.8
    LLCP_SDREQ =     0x08,
    //! See LLCP V1.1 Section 4.5.9
    LLCP_SDRES =     0x09,
    LLCP_ERROR
}tLLCPParamaeter;

//*****************************************************************************
//
//! PDU Type Enumerations.
//
//*****************************************************************************
typedef enum
{
    //! See LLCP V1.1 Section 4.3.1
    LLCP_SYMM_PDU =      0x00,
    //! See LLCP V1.1 Section 4.3.2
    LLCP_PAX_PDU=        0x01,
    //! See LLCP V1.1 Section 4.3.3
    LLCP_AGF_PDU=        0x02,
    //! See LLCP V1.1 Section 4.3.4
    LLCP_UI_PDU =        0x03,
    //! See LLCP V1.1 Section 4.3.5
    LLCP_CONNECT_PDU =   0x04,
    //! See LLCP V1.1 Section 4.3.6
    LLCP_DISC_PDU =      0x05,
    //! See LLCP V1.1 Section 4.3.7
    LLCP_CC_PDU =        0x06,
    //! See LLCP V1.1 Section 4.3.8
    LLCP_DM_PDU =        0x07,
    //! See LLCP V1.1 Section 4.3.9
    LLCP_FRMR_PDU =      0x08,
    //! See LLCP V1.1 Section 4.3.10
    LLCP_SNL_PDU =       0x09,
    //! See LLCP V1.1 Section 4.3.11
    LLCP_I_PDU   =       0x0C,
    //! See LLCP V1.1 Section 4.3.12
    LLCP_RR_PDU  =       0x0D,
    //! See LLCP V1.1 Section 4.3.13
    LLCP_RNR_PDU =       0x0E,
    //! See LLCP V1.1 Section 4.3.14
    LLCP_RESERVED_PDU =  0x0F
}tLLCPPduPtype;

//*****************************************************************************
//
//! LLCP Connection Status Enumeration.
//
//*****************************************************************************
typedef enum
{
    //! No Tx/Rx ongoing.
    LLCP_CONNECTION_IDLE    = 0x00,

    //! When a virtual link is created either when we send a CONNECT PDU and
    //! receive a CC PDU, or when we receive a CONNECT PDU and respond a CC PDU.
    LLCP_CONNECTION_ESTABLISHED,

    //! When sending data via SNEP
    LLCP_CONNECTION_SENDING,

    //! When receiving data via SNEP
    LLCP_CONNECTION_RECEIVING

}tLLCPConnectionStatus;

//*****************************************************************************
//
//! Service Name Enumerations - Only support SNEP_SERVICE
//
//*****************************************************************************
typedef enum
{
    NPP_SERVICE = 0,
    SNEP_SERVICE,
    HANDOVER_SERVICE
}tServiceName;


//*****************************************************************************
//
//! Disconnected Mode Reasons Enumerations.
//
//*****************************************************************************
typedef enum
{
    //! See LLCP Section 4.3.8.
    DM_REASON_LLCP_RECEIVED_DISC_PDU =                               0x00,

    //! See LLCP Section 4.3.8.
    DM_REASON_LLCP_RECEIVED_CONNECTION_ORIENTED_PDU =                0x01,

    //! See LLCP Section 4.3.8.
    DM_REASON_LLCP_RECEIVED_CONNECT_PDU_NO_SERVICE =                 0x02,

    //! See LLCP Section 4.3.8.
    DM_REASON_LLCP_PROCESSED_CONNECT_PDU_REQ_REJECTED =              0x03,

    //! See LLCP Section 4.3.8.
    DM_REASON_LLCP_PERMNANTLY_NOT_ACCEPT_CONNECT_WITH_SAME_SSAP =    0x10,

    //! See LLCP Section 4.3.8.
    DM_REASON_LLCP_PERMNANTLY_NOT_ACCEPT_CONNECT_WITH_ANY_SSAP =     0x11,

    //! See LLCP Section 4.3.8.
    DM_REASON_LLCP_TEMMPORARILY_NOT_ACCEPT_PDU_WITH_SAME_SSSAPT =    0x20,

    //! See LLCP Section 4.3.8.
    DM_REASON_LLCP_TEMMPORARILY_NOT_ACCEPT_PDU_WITH_ANY_SSSAPT =     0x21
}tDisconnectModeReason;



//*****************************************************************************
//
// Function Prototypes
//
//*****************************************************************************
void LLCP_init(void);

uint8_t LLCP_stateMachine(uint8_t * pui8PduBufferPtr);

tStatus LLCP_processReceivedData(uint8_t * pui8RxBuffer, uint8_t ui8PduLength);

uint16_t LLCP_getLinkTimeOut(void);
uint8_t LLCP_addTLV(tLLCPParamaeter eLLCPparam, uint8_t * pui8TLVBufferPtr);
void LLCP_processTLV(uint8_t * pui8TLVBufferPtr);

tStatus LLCP_setNextPDU(tLLCPPduPtype eNextPdu);
void LLCP_setConnectionStatus(tLLCPConnectionStatus eConnectionState);

uint8_t LLCP_sendSYMM(uint8_t * pui8PduBufferPtr);
uint8_t LLCP_sendCONNECT(uint8_t * pui8PduBufferPtr);
uint8_t LLCP_sendDISC(uint8_t * pui8PduBufferPtr);
uint8_t LLCP_sendCC(uint8_t * pui8PduBufferPtr);
uint8_t LLCP_sendDM(uint8_t * pui8PduBufferPtr,tDisconnectModeReason eDmReason);
uint8_t LLCP_sendI(uint8_t * pui8PduBufferPtr);
uint8_t LLCP_sendRR(uint8_t * pui8PduBufferPtr);
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif //__NFC_LLCP_H__
