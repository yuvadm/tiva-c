//*****************************************************************************
//
// llcp.c - Logic Link Control Protocol : used to send packets via NPP or SNEP
//          NPP:  NDEF Push Protocol
//          SNEP: Simple NDEF Exchange protocol
// NOTE: currently only SNEP is Supported
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
#include "utils/uartstdio.h"
#include "nfclib/llcp.h"
#include "nfclib/snep.h"

//*****************************************************************************
//
//! \addtogroup nfc_llcp_api NFC LLCP API Functions
//! @{
//! Logical Link Control Protocol is the NFC transport layer used to open and
//! close a virtual link used to transfer NDEFs between two devices in
//! peer-to-peer mode via the Simple NDEF Exchange Protocol.
//! For more information on LLCP, please read the Logical Link Control Protocol
//! Specification Version 1.1.
//
//*****************************************************************************

//*****************************************************************************
//
// The next PDU to be sent to the destination device - updated based on
//  incoming packets or by LLCPSetNextPDU().
//
//*****************************************************************************
tLLCPPduPtype g_eNextPduQueue = LLCP_SYMM_PDU;

//*****************************************************************************
//
// Connection Status
//
//*****************************************************************************
tLLCPConnectionStatus g_eLLCPConnectionStatus = LLCP_CONNECTION_IDLE;

//*****************************************************************************
//
// Destination Service Access Point Address
//
//*****************************************************************************
uint8_t g_ui8dsapValue;

//*****************************************************************************
//
// Source Service Access Point Address
//
//*****************************************************************************
uint8_t g_ui8ssapValue;

//*****************************************************************************
//
// Service Name - SNEP by default
//
//*****************************************************************************
tServiceName g_eCurrentServiceEnabled = SNEP_SERVICE;

//*****************************************************************************
//
// Acknowledged packets
//
//*****************************************************************************
uint8_t g_ui8NSNR = 0x00;

//*****************************************************************************
//
// Disconnected Mode Reason
//
//*****************************************************************************
tDisconnectModeReason g_eDMReason;

//*****************************************************************************
//
// LLCP Link Time Out
//
//*****************************************************************************
uint16_t g_ui16LLCPlto = 0x00;

//*****************************************************************************
//
// LLCP MIUX of the initiator / target communicating with the TRF7970A.
// 248 by default.
//
//*****************************************************************************
uint8_t g_ui8LLCPmiu = 248;

//*****************************************************************************
//
//! Initializes the Logical Link Control Protocol layer.
//!
//! This function must be called prior to any other function offer by the LLCP
//! driver. This function initializes the acknowledge packets, the current
//! service enabled, and the next PDU for the LLCP_stateMachine(), and also
//! initializes the SNEP layer with SNEP_init().
//!
//! \return None
//!
//
//*****************************************************************************
void LLCP_init(void)
{
    //
    // Reset NS and NR
    //
    g_ui8NSNR = 0x00;
    g_ui8LLCPmiu = 128;
    g_eLLCPConnectionStatus = LLCP_CONNECTION_IDLE;
    g_eCurrentServiceEnabled = SNEP_SERVICE;
    g_eNextPduQueue = LLCP_SYMM_PDU;
    SNEP_init();
    SNEP_setMaxPayload(g_ui8LLCPmiu);
}

//*****************************************************************************
//
//! Gets link timeout
//!
//! This function returns the Link Timeout, which may be modified if the
//! LLCP_processTLV() function processes a LLCP_LTO TLV.
//!
//! \return \e \b g_ui16LLCPlto the link timeout.
//
//*****************************************************************************
uint16_t LLCP_getLinkTimeOut(void)
{
    return g_ui16LLCPlto;
}

//*****************************************************************************
//
//! Adds a  LLCP parameter to the LLCP PDU with the Type Length
//! Value (TLV) format.
//!
//! \param eLLCPparam is the LLCP type that will be added.
//! \param pui8TLVBufferPtr is the pointer where the TLV is written
//!
//! The \e \b eLLCPparam parameter can be any of the following:
//!
//! - \b LLCP_VERSION    - Version Number
//! - \b LLCP_MIUX       - Maximum Information Unit Extension
//! - \b LLCP_WKS        - Well-Known Service List
//! - \b LLCP_LTO        - Link Timeout
//! - \b LLCP_RW         - Receive Window Size
//! - \b LLCP_SN         - Service Name
//! - \b LLCP_OPT        - Option
//! - \b LLCP_SDREQ      - Service Discovery Request
//! - \b LLCP_SDRES      - Service Discovery Response
//! - \b LLCP_ERROR      - Reserved (used ro return length of 0)
//!
//! This function is used to add a LLCP Parameter to the LLCP PDU to include
//! more information about the LLCP layer. This function must be called inside
//! LLCP_sendCONNECT(), LLCP_sendCC(), NFCDEP_sendATR_REQ() and
//! NFCDEP_sendATR_RES().
//!
//! \return ui8PacketLength Length of the LLCP Parameter added to the LLCP.
//
//*****************************************************************************
uint8_t LLCP_addTLV(tLLCPParamaeter eLLCPparam, uint8_t * pui8TLVBufferPtr)
{
    uint8_t ui8PacketLength = 0;

    switch(eLLCPparam)
    {
    case LLCP_VERSION:
        // Type
        pui8TLVBufferPtr[0] = (uint8_t) LLCP_VERSION;
        // Length
        pui8TLVBufferPtr[1] = 0x01;
        // Value
        pui8TLVBufferPtr[2] = 0x11;     // Version 1.1
        break;
    case LLCP_MIUX:
        // Type
        pui8TLVBufferPtr[0] = (uint8_t) LLCP_MIUX;
        // Length
        pui8TLVBufferPtr[1] = 0x02;
        // Value
        // 128 + MIUX (120) = MIU (248)
        pui8TLVBufferPtr[2] = (LLCP_MIUX_SIZE >> 8) & 0xFF;     // MIUX 15:8
        pui8TLVBufferPtr[3] = (uint8_t) LLCP_MIUX_SIZE;        // MIUX 7:0
        break;
    case LLCP_WKS:
        // Type
        pui8TLVBufferPtr[0] = (uint8_t) LLCP_WKS;
        // Length
        pui8TLVBufferPtr[1] = 0x02;
        // Value
        pui8TLVBufferPtr[2] = 0x00;
        pui8TLVBufferPtr[3] = 0x03;
        break;
    case LLCP_LTO:
        // Type
        pui8TLVBufferPtr[0] = (uint8_t) LLCP_LTO;
        // Length
        pui8TLVBufferPtr[1] = 0x01;
        // Value
        pui8TLVBufferPtr[2] = 0x64;     // (100 (0x64) * 10 mS = 1000 mS timeout, Figure 22, LLP)
        break;
    case LLCP_RW:
        // Type
        pui8TLVBufferPtr[0] = (uint8_t) LLCP_RW;
        // Length
        pui8TLVBufferPtr[1] = 0x01;
        // Value
        // Section 5.6.2.2 LLP
        // A receive window size of zero indicates that the local LLC will not
        // accept I PDUs on that data link connection. A receive window size of
        // one indicates that the local LLC will acknowledge every I PDU before
        // accepting additional I PDUs.
        //
        pui8TLVBufferPtr[2] = 0x04;
        break;
    case LLCP_SN:
        // Type
        pui8TLVBufferPtr[0] = (uint8_t) LLCP_SN;
        if(g_eCurrentServiceEnabled == NPP_SERVICE)
        {
            // Length
            pui8TLVBufferPtr[1] = 0x0F;
            // Value
            pui8TLVBufferPtr[2] = 'c';
            pui8TLVBufferPtr[3] = 'o';
            pui8TLVBufferPtr[4] = 'm';
            pui8TLVBufferPtr[5] = '.';
            pui8TLVBufferPtr[6] = 'a';
            pui8TLVBufferPtr[7] = 'n';
            pui8TLVBufferPtr[8] = 'd';
            pui8TLVBufferPtr[9] = 'r';
            pui8TLVBufferPtr[10] = 'o';
            pui8TLVBufferPtr[11] = 'i';
            pui8TLVBufferPtr[12] = 'd';
            pui8TLVBufferPtr[13] = '.';
            pui8TLVBufferPtr[14] = 'n';
            pui8TLVBufferPtr[15] = 'p';
            pui8TLVBufferPtr[16] = 'p';
        }
        else if(g_eCurrentServiceEnabled == SNEP_SERVICE)
        {
            // Length
            pui8TLVBufferPtr[1] = 0x0F;
            // Value
            pui8TLVBufferPtr[2] = 'u';
            pui8TLVBufferPtr[3] = 'r';
            pui8TLVBufferPtr[4] = 'n';
            pui8TLVBufferPtr[5] = ':';
            pui8TLVBufferPtr[6] = 'n';
            pui8TLVBufferPtr[7] = 'f';
            pui8TLVBufferPtr[8] = 'c';
            pui8TLVBufferPtr[9] = ':';
            pui8TLVBufferPtr[10] = 's';
            pui8TLVBufferPtr[11] = 'n';
            pui8TLVBufferPtr[12] = ':';
            pui8TLVBufferPtr[13] = 's';
            pui8TLVBufferPtr[14] = 'n';
            pui8TLVBufferPtr[15] = 'e';
            pui8TLVBufferPtr[16] = 'p';

        }
        break;
    case LLCP_OPT:
        // Type
        pui8TLVBufferPtr[0] = (uint8_t) LLCP_OPT;
        // Length
        pui8TLVBufferPtr[1] = 0x01;
        // Value
        pui8TLVBufferPtr[2] = 0x03;     // (Class 3) (Table 7, LLP)
        break;
    case LLCP_SDREQ:
        break;
    case LLCP_SDRES:
        break;
    default:
        pui8TLVBufferPtr[0] = LLCP_ERROR;
        break;
    }

    if(pui8TLVBufferPtr[0] == LLCP_ERROR)
        ui8PacketLength = 0x00;
    else
        ui8PacketLength = pui8TLVBufferPtr[1] + 2;

    return ui8PacketLength;
}

//*****************************************************************************
//
//! Processes the LLCP Parameter TLV.
//!
//! \param pui8TLVBufferPtr is the pointer to the Type value of the TLV.
//!
//! This function processes the LLCP Parameters included in the ATR_RES. This
//! function must be called inside the NFCDEP_processReceivedData(), to
//! initialize the g_ui8LLCPmiu and g_ui16LLCPlto if they are included as part
//! of ATR_RES.
//!
//! \return None
//
//*****************************************************************************
void LLCP_processTLV(uint8_t * pui8TLVBufferPtr)
{
    uint16_t ui16Miu;
    switch(pui8TLVBufferPtr[0])
    {
    case LLCP_VERSION:
        break;
    case LLCP_MIUX:
        // MIU = 128 + MIUX
        ui16Miu = (pui8TLVBufferPtr[2] << 8)+pui8TLVBufferPtr[3]+128;
        // Check if the received MIU is less than 248, the modify the current MIU to it
        if(ui16Miu < 248)
        {
            // Modify MIU to be less
            g_ui8LLCPmiu = (uint8_t) ui16Miu;

        }
        else
        {
            // Maximum supported MIU is 248
            g_ui8LLCPmiu = 248;
        }

        SNEP_setMaxPayload(g_ui8LLCPmiu);
        break;
    case LLCP_WKS:
        break;
    case LLCP_LTO:
        g_ui16LLCPlto = pui8TLVBufferPtr[2] * 10;
        break;
    case LLCP_RW:
        break;
    case LLCP_SN:
        break;
    case LLCP_OPT:
        break;
    case LLCP_SDREQ:
        break;
    case LLCP_SDRES:
        break;
    default:
        break;
    }
}

//*****************************************************************************
//
//! Prepares the LLCP packet to be transmitted.
//!
//! \param pui8PduBufferPtr is the start pointer to add the LLCP PDU.
//!
//! This function is used to add the LLCP portion of the DEP_REQ / DEP_RES PDU.
//! This function must be called inside NFCDEP_sendDEP_REQ() and
//! NFCDEP_sendDEP_RES(). It currently does not support sending the following
//! PDUs : LLCP_PAX_PDU, LLCP_AGF_PDU, LLCP_UI_PDU, LLCP_FRMR_PDU, LLCP_SNL_PDU,
//! LLCP_RNR_PDU, and LLCP_RESERVED_PDU.
//!
//! \return ui8PacketLength is the length of the LLCP PDU added to the
//! pui8PduBufferPtr.
//
//*****************************************************************************
uint8_t LLCP_stateMachine(uint8_t * pui8PduBufferPtr)
{
    uint8_t ui8PacketLength=0;

    switch(g_eNextPduQueue)
    {
        case LLCP_SYMM_PDU:
        {
                //UARTprintf("TX: SYMM\n");
            ui8PacketLength = LLCP_sendSYMM(pui8PduBufferPtr);
            break;
        }
        case LLCP_PAX_PDU:
        {
            break;
        }
        case LLCP_AGF_PDU:
        {
            break;
        }
        case LLCP_UI_PDU:
        {
            break;
        }
        case LLCP_CONNECT_PDU:
        {
            //UARTprintf("TX: CONNECT\n");
            ui8PacketLength = LLCP_sendCONNECT(pui8PduBufferPtr);
            g_eNextPduQueue = LLCP_SYMM_PDU;
            break;
        }
        case LLCP_DISC_PDU:
        {
            if(g_eCurrentServiceEnabled == HANDOVER_SERVICE)
            {
                g_eCurrentServiceEnabled = SNEP_SERVICE;
            }
            //UARTprintf("TX: DISC\n");
            ui8PacketLength = LLCP_sendDISC(pui8PduBufferPtr);
            break;
        }
        case LLCP_CC_PDU:
        {
            //UARTprintf("TX: CC\n");
            ui8PacketLength = LLCP_sendCC(pui8PduBufferPtr);
            break;
        }
        case LLCP_DM_PDU:
        {
            //UARTprintf("TX: DM\n");
            g_eLLCPConnectionStatus = LLCP_CONNECTION_IDLE;
            ui8PacketLength = LLCP_sendDM(pui8PduBufferPtr,g_eDMReason);
            g_eNextPduQueue = LLCP_SYMM_PDU;
            break;
        }
        case LLCP_FRMR_PDU:
        {
            break;
        }
        case LLCP_SNL_PDU:
        {
            break;
        }
        case LLCP_I_PDU:
        {
            //UARTprintf("TX: I\n");
            ui8PacketLength = LLCP_sendI(pui8PduBufferPtr);
            break;
        }
        case LLCP_RR_PDU:
        {
            //UARTprintf("TX: RR\n");
            if(g_eCurrentServiceEnabled == HANDOVER_SERVICE)
            {
                g_eLLCPConnectionStatus = LLCP_CONNECTION_IDLE;
            }
            ui8PacketLength = LLCP_sendRR(pui8PduBufferPtr);
            break;
        }
        case LLCP_RNR_PDU:
        {
            break;
        }
        case LLCP_RESERVED_PDU:
        {
            break;
        }
        default:
        {
            break;
        }
    }

    return ui8PacketLength;
}

//*****************************************************************************
//
//! Processes LLCP Data Received.
//!
//! \param pui8RxBuffer is the start pointer of the LLCP data received.
//! \param ui8PduLength is the length of the LLCP PDU received.
//!
//! This function is used to handle the LLCP portion of the DEP_REQ / DEP_RES PDU.
//! This function must be called inside NFCDEP_processReceivedRequest() and
//! NFCDEP_processReceivedData().It currently does not support to handle the
//! following PDUs : LLCP_PAX_PDU, LLCP_AGF_PDU, LLCP_UI_PDU, LLCP_FRMR_PDU,
//! LLCP_SNL_PDU, LLCP_RNR_PDU, and LLCP_RESERVED_PDU.
//!
//! \return \b eLLCPStatus is the boolean status if the command was processed
//! (1) or not (0).
//
//*****************************************************************************
tStatus LLCP_processReceivedData(uint8_t * pui8RxBuffer, uint8_t ui8PduLength)
{
    tLLCPPduPtype ePduType;
    tStatus eLLCPStatus = STATUS_SUCCESS;
    tSNEPConnectionStatus eSnepProtocolStatus;

    ePduType = (tLLCPPduPtype) ( ((pui8RxBuffer[0] & 0x03) << 2) +
                                    ((pui8RxBuffer[1] & 0xC0) >> 6));

    switch(ePduType)
    {
    case LLCP_SYMM_PDU:
        if(g_eCurrentServiceEnabled == SNEP_SERVICE)
        {
            eSnepProtocolStatus = SNEP_getProtocolStatus();
            if((g_eNextPduQueue == LLCP_CONNECT_PDU) ||
                (g_eNextPduQueue == LLCP_I_PDU))
            {
                //
                // Do no modify the next PDU
                //
            }
            else if(eSnepProtocolStatus == SNEP_CONNECTION_SEND_COMPLETE)
            {
                SNEP_setProtocolStatus(SNEP_CONNECTION_IDLE);
                //UARTprintf("RX: SYMM ");
                g_eNextPduQueue = LLCP_DISC_PDU;
            }
            else if((eSnepProtocolStatus ==
                                    SNEP_CONNECTION_RECEIVED_FIRST_PACKET) ||
                    (eSnepProtocolStatus ==
                                    SNEP_CONNECTION_RECEIVE_COMPLETE) ||
                    (eSnepProtocolStatus ==
                                    SNEP_CONNECTION_EXCESS_SIZE) ||
                    (eSnepProtocolStatus ==
                                    SNEP_CONNECTION_SENDING_N_FRAGMENTS))
            {
                //UARTprintf("RX: SYMM ");
                g_eNextPduQueue = LLCP_I_PDU;
            }
            else
            {
                if(g_eLLCPConnectionStatus != LLCP_CONNECTION_IDLE)
                {
                    g_eNextPduQueue = LLCP_SYMM_PDU;
                }
            }
        }
        else if(g_eCurrentServiceEnabled == HANDOVER_SERVICE)
        {
            if(g_eLLCPConnectionStatus == LLCP_CONNECTION_IDLE)
            {
                g_eNextPduQueue = LLCP_DISC_PDU;
            }
            else
                g_eNextPduQueue = LLCP_SYMM_PDU;
        }
        else
        {
            if(g_eLLCPConnectionStatus != LLCP_CONNECTION_IDLE)
            {
                //UARTprintf("RX: SYMM ");
                g_eNextPduQueue = LLCP_SYMM_PDU;
            }
        }
        break;
    case LLCP_PAX_PDU:
        // Not Supported
        break;
    case LLCP_AGF_PDU:
        // Not Supported
        break;
    case LLCP_UI_PDU:
        // Not Supported
        break;
    case LLCP_CONNECT_PDU:
        g_ui8dsapValue = (pui8RxBuffer[1] & 0x3F);

            // Check Service Name TLV
            if(pui8RxBuffer[2] == 0x06)
            {
                if (pui8RxBuffer[3] == 0x0F && pui8RxBuffer[4] == 'u'
                        && pui8RxBuffer[5] == 'r' && pui8RxBuffer[6] == 'n'
                        && pui8RxBuffer[7] == ':' && pui8RxBuffer[8] == 'n'
                        && pui8RxBuffer[9] == 'f' && pui8RxBuffer[10] == 'c'
                        && pui8RxBuffer[11] == ':' && pui8RxBuffer[12] == 's'
                        && pui8RxBuffer[13] == 'n' && pui8RxBuffer[14] == ':'
                        && pui8RxBuffer[15] == 's' && pui8RxBuffer[16] == 'n'
                        && pui8RxBuffer[17] == 'e' && pui8RxBuffer[18] == 'p')
                {
                    // SNEP
                    g_eCurrentServiceEnabled = SNEP_SERVICE;
                }
                else if (pui8RxBuffer[3] == 0x0F && pui8RxBuffer[4] == 'c'
                        && pui8RxBuffer[5] == 'o' && pui8RxBuffer[6] == 'm'
                        && pui8RxBuffer[7] == '.' && pui8RxBuffer[8] == 'a'
                        && pui8RxBuffer[9] == 'n' && pui8RxBuffer[10] == 'd'
                        && pui8RxBuffer[11] == 'r' && pui8RxBuffer[12] == 'o'
                        && pui8RxBuffer[13] == 'i' && pui8RxBuffer[14] == 'd'
                        && pui8RxBuffer[15] == '.' && pui8RxBuffer[16] == 'n'
                        && pui8RxBuffer[17] == 'p' && pui8RxBuffer[18] == 'p')
                {
                    // NPP
                    g_eCurrentServiceEnabled = NPP_SERVICE;
                }
                else if (pui8RxBuffer[3] == 0x13 && pui8RxBuffer[4] == 'u'
                        && pui8RxBuffer[5] == 'r' && pui8RxBuffer[6] == 'n'
                        && pui8RxBuffer[7] == ':' && pui8RxBuffer[8] == 'n'
                        && pui8RxBuffer[9] == 'f' && pui8RxBuffer[10] == 'c'
                        && pui8RxBuffer[11] == ':' && pui8RxBuffer[12] == 's'
                        && pui8RxBuffer[13] == 'n' && pui8RxBuffer[14] == ':'
                        && pui8RxBuffer[15] == 'h' && pui8RxBuffer[16] == 'a'
                        && pui8RxBuffer[17] == 'n' && pui8RxBuffer[18] == 'd'
                        && pui8RxBuffer[19] == 'o' && pui8RxBuffer[20] == 'v'
                        && pui8RxBuffer[21] == 'e' && pui8RxBuffer[22] == 'r')
                {
                    // Handover
                    g_eCurrentServiceEnabled = HANDOVER_SERVICE;
                }
            else if(ui8PduLength == 2)
            {
                // SNEP
                g_eCurrentServiceEnabled = SNEP_SERVICE;
            }
                else
                {
    //              // Debug Incoming Request
    //              while(1);
                    // Ignore the command
                    g_eNextPduQueue = LLCP_SYMM_PDU;
                    break;
                }
            }
            else
                g_eCurrentServiceEnabled = SNEP_SERVICE;

            g_eNextPduQueue = LLCP_CC_PDU;
            break;
        case LLCP_DISC_PDU:
            //UARTprintf("RX: DISC ");
            g_eDMReason = DM_REASON_LLCP_RECEIVED_DISC_PDU;
            g_eNextPduQueue = LLCP_DM_PDU;
            break;
        case LLCP_CC_PDU:
            //UARTprintf("RX: CC ");
            g_ui8dsapValue = (pui8RxBuffer[1] & 0x3F);
            g_eNextPduQueue = LLCP_I_PDU;
            break;
        case LLCP_DM_PDU:
            //UARTprintf("RX: DM ");
            g_eLLCPConnectionStatus = LLCP_CONNECTION_IDLE;
            // Reset the snep communication
            g_eNextPduQueue = LLCP_SYMM_PDU;
            break;
        case LLCP_FRMR_PDU:
            //UARTprintf("RX: FRMR ");
            break;
        case LLCP_SNL_PDU:
            //UARTprintf("RX: SNL ");
            break;
        case LLCP_I_PDU:
            //UARTprintf("RX: I ");
            if(g_eCurrentServiceEnabled == SNEP_SERVICE)
                SNEP_processReceivedData(&pui8RxBuffer[3],ui8PduLength-3);
            else if(g_eCurrentServiceEnabled == NPP_SERVICE)
            {
                // Not Supported
            }
            else if(g_eCurrentServiceEnabled == HANDOVER_SERVICE)
            {
                // Not Supported
            }
            if(g_eLLCPConnectionStatus == LLCP_CONNECTION_ESTABLISHED)
            {
                g_eLLCPConnectionStatus = LLCP_CONNECTION_RECEIVING;
            }
            g_eNextPduQueue = LLCP_RR_PDU;
            break;
        case LLCP_RR_PDU:
            //UARTprintf("RX: RR \n");
            g_eNextPduQueue = LLCP_SYMM_PDU;
            eSnepProtocolStatus = SNEP_getProtocolStatus();
            if(g_eLLCPConnectionStatus == LLCP_CONNECTION_SENDING)
            {
                if(
                    (eSnepProtocolStatus ==
                                        SNEP_CONNECTION_WAITING_FOR_CONTINUE) ||
                   (eSnepProtocolStatus ==
                                        SNEP_CONNECTION_WAITING_FOR_SUCCESS))
                {
                   g_eNextPduQueue = LLCP_SYMM_PDU;
                }
                else if(eSnepProtocolStatus ==
                                            SNEP_CONNECTION_SENDING_N_FRAGMENTS)
                {
                    g_eNextPduQueue = LLCP_I_PDU;
                }
                else if(eSnepProtocolStatus == SNEP_CONNECTION_SEND_COMPLETE)
                {
                    g_eNextPduQueue = LLCP_DISC_PDU;
                }
            }
            else
            {
                //
                // Used for debugging
                //
                g_eNextPduQueue = LLCP_SYMM_PDU;
            }
            break;
        case LLCP_RNR_PDU:
            //UARTprintf("RX: RNR ");
            break;
        case LLCP_RESERVED_PDU:
            //UARTprintf("RX: RESERVED ");
            break;
        default:
            //UARTprintf("RX: UNKNOWN LLCP ");
            eLLCPStatus = STATUS_FAIL;
            break;
    }

    return eLLCPStatus;
}

//*****************************************************************************
//
//! Set next PDU, return SUCCESS or FAIL
//!
//! \param eNextPdu is the LLCP PDU to set next.
//!
//! The \e eNextPdu parameter can be any of the following:
//!
//! - \b LLCP_SYMM_PDU       - See LLCP standard document section 4.3.1
//! - \b LLCP_PAX_PDU        - See LLCP standard document section 4.3.2
//! - \b LLCP_AGF_PDU        - See LLCP standard document section 4.3.3
//! - \b LLCP_UI_PDU         - See LLCP standard document section 4.3.4
//! - \b LLCP_CONNECT_PDU    - See LLCP standard document section 4.3.5
//! - \b LLCP_DISC_PDU       - See LLCP standard document section 4.3.6
//! - \b LLCP_CC_PDU         - See LLCP standard document section 4.3.7
//! - \b LLCP_DM_PDU         - See LLCP standard document section 4.3.8
//! - \b LLCP_FRMR_PDU       - See LLCP standard document section 4.3.9
//! - \b LLCP_SNL_PDU        - See LLCP standard document section 4.3.10
//! - \b LLCP_I_PDU          - See LLCP standard document section 4.3.11
//! - \b LLCP_RR_PDU         - See LLCP standard document section 4.3.12
//! - \b LLCP_RNR_PDU        - See LLCP standard document section 4.3.13
//! - \b LLCP_RESERVED_PDU   - See LLCP standard document section 4.3.14
//! - \b LLCP_ERROR_PDU      - Unknown PDU
//!
//! This function is used to modify the next LLCP PDU. For example
//! when we need to set the next PDU to be LLCP_CONNECT_PDU, to initiate
//! a transfer. For more information please see the LLCP document from the
//! NFC Forum.
//!
//! \return eSetNextPduStatus SUCCESS if g_eNextPduQueue was modified, else
//! return FAIL
//
//*****************************************************************************
tStatus LLCP_setNextPDU(tLLCPPduPtype eNextPdu)
{
    tStatus eSetNextPduStatus;
    if(g_eLLCPConnectionStatus == LLCP_CONNECTION_IDLE ||
        g_eLLCPConnectionStatus == LLCP_CONNECTION_ESTABLISHED)
    {
        g_eNextPduQueue = eNextPdu;
        if(eNextPdu == LLCP_CONNECT_PDU)
            g_eCurrentServiceEnabled = SNEP_SERVICE;
        eSetNextPduStatus = STATUS_SUCCESS;
    }
    else
    {
        eSetNextPduStatus = STATUS_FAIL;
    }
    return eSetNextPduStatus;
}

//*****************************************************************************
//
//! Send SYMM message
//!
//! \param pui8PduBufferPtr is the start pointer to store the SYMM PDU.
//!
//! This function adds a SYMM PDU starting at pui8PduBufferPtr.For more
//! details on this PDU read LLCP V1.1 Section 4.3.1.
//!
//! \return ui8IndexTemp is the length of the SYMM PDU.
//
//*****************************************************************************
uint8_t LLCP_sendSYMM(uint8_t * pui8PduBufferPtr)
{
    uint8_t ui8IndexTemp = 0;
    // DSAP (6 bits)  PTYPE (4 bits)  SSAP (6 bits)
    pui8PduBufferPtr[ui8IndexTemp++] = ( (LLCP_SYMM_PDU & 0xFC) >> 2);
    pui8PduBufferPtr[ui8IndexTemp++] = ( (LLCP_SYMM_PDU & 0x03) << 6);
    return ui8IndexTemp;
}

//*****************************************************************************
//
//! Send CONNECT message
//!
//! \param pui8PduBufferPtr is the start pointer to store the CONNECT PDU.
//!
//! This function adds a CONNECT PDU starting at pui8PduBufferPtr.For more
//! details on this PDU read LLCP V1.1 Section 4.3.5.
//!
//! \return ui8IndexTemp is the length of the CONNECT PDU.
//
//*****************************************************************************
uint8_t LLCP_sendCONNECT(uint8_t * pui8PduBufferPtr)
{
    uint8_t ui8IndexTemp = 0;

    g_eCurrentServiceEnabled = SNEP_SERVICE;

    //
    // Reset NR and NS
    //
    g_ui8NSNR = 0x00;

    g_eLLCPConnectionStatus = LLCP_CONNECTION_SENDING;

    g_ui8ssapValue = LLCP_SSAP_CONNECT_SEND;
    g_ui8dsapValue = DSAP_SERVICE_DISCOVERY_PROTOCOL;

    // DSAP (6 bits)  PTYPE (4 bits)  SSAP (6 bits)
    pui8PduBufferPtr[ui8IndexTemp++] = (g_ui8dsapValue << 2) |
                                        ( (LLCP_CONNECT_PDU & 0xFC) >> 2);
    pui8PduBufferPtr[ui8IndexTemp++] = ( (LLCP_CONNECT_PDU & 0x03) << 6) |
                                        g_ui8ssapValue;

    //
    // TLV Fields
    //
    ui8IndexTemp = ui8IndexTemp +
                        LLCP_addTLV(LLCP_SN, &pui8PduBufferPtr[ui8IndexTemp]);
    ui8IndexTemp = ui8IndexTemp +
                        LLCP_addTLV(LLCP_MIUX, &pui8PduBufferPtr[ui8IndexTemp]);
    ui8IndexTemp = ui8IndexTemp +
                        LLCP_addTLV(LLCP_RW, &pui8PduBufferPtr[ui8IndexTemp]);

    return ui8IndexTemp;
}

//*****************************************************************************
//
//! Send DISC message
//!
//! \param pui8PduBufferPtr is the start pointer to store the DISC PDU.
//!
//! This function adds a DISC PDU starting at pui8PduBufferPtr.For more details
//! on this PDU read LLCP V1.1 Section 4.3.6.
//!
//! \return ui8IndexTemp is the length of the DISC PDU.
//
//*****************************************************************************
uint8_t LLCP_sendDISC(uint8_t * pui8PduBufferPtr)
{
    uint8_t ui8IndexTemp = 0;

    //
    // DSAP (6 bits)  PTYPE (4 bits)  SSAP (6 bits)
    //
    pui8PduBufferPtr[ui8IndexTemp++] = (g_ui8dsapValue << 2) |
                                        ( (LLCP_DISC_PDU & 0xFC) >> 2);
    pui8PduBufferPtr[ui8IndexTemp++] = ( (LLCP_DISC_PDU & 0x03) << 6) |
                                            g_ui8ssapValue;

    return ui8IndexTemp;
}

//*****************************************************************************
//
//! Send CC message
//!
//! \param pui8PduBufferPtr is the start pointer to store the CC PDU.
//!
//! This function adds a CC PDU starting at pui8PduBufferPtr. For more details
//! on this PDU, read LLCP V1.1 Section 4.3.7.
//!
//! \return \b ui8IndexTemp is the length of the CC PDU.
//
//*****************************************************************************
uint8_t LLCP_sendCC(uint8_t * pui8PduBufferPtr)
{
    uint8_t ui8IndexTemp = 0;

    g_eLLCPConnectionStatus = LLCP_CONNECTION_ESTABLISHED;

    //
    // Reset NR and NS
    //
    g_ui8NSNR = 0x00;

    g_ui8ssapValue = LLCP_SSAP_CONNECT_RECEIVED;

    // DSAP (6 bits)  PTYPE (4 bits)  SSAP (6 bits)
    pui8PduBufferPtr[ui8IndexTemp++] = (g_ui8dsapValue << 2) |
                                            ( (LLCP_CC_PDU & 0xFC) >> 2);
    pui8PduBufferPtr[ui8IndexTemp++] = ( (LLCP_CC_PDU & 0x03) << 6) |
                                            g_ui8ssapValue;

    //
    // TLV Fields
    //
    ui8IndexTemp = ui8IndexTemp +
                        LLCP_addTLV(LLCP_MIUX, &pui8PduBufferPtr[ui8IndexTemp]);
    ui8IndexTemp = ui8IndexTemp +
                        LLCP_addTLV(LLCP_RW, &pui8PduBufferPtr[ui8IndexTemp]);

    return ui8IndexTemp;
}

//*****************************************************************************
//
//! Send DM message
//!
//! \param pui8PduBufferPtr is the start pointer to store the DM PDU.
//! \param eDmReason is the enumeration of the disconnection reason.
//!
//! The \e eDmReason parameter can be any of the following:
//!
//! - \b DM_REASON_LLCP_RECEIVED_DISC_PDU
//! - \b DM_REASON_LLCP_RECEIVED_CONNECTION_ORIENTED_PDU
//! - \b DM_REASON_LLCP_RECEIVED_CONNECT_PDU_NO_SERVICE
//! - \b DM_REASON_LLCP_PROCESSED_CONNECT_PDU_REQ_REJECTED
//! - \b DM_REASON_LLCP_PERMNANTLY_NOT_ACCEPT_CONNECT_WITH_SAME_SSAP
//! - \b DM_REASON_LLCP_PERMNANTLY_NOT_ACCEPT_CONNECT_WITH_ANY_SSAP
//! - \b DM_REASON_LLCP_TEMMPORARILY_NOT_ACCEPT_PDU_WITH_SAME_SSSAPT
//! - \b DM_REASON_LLCP_TEMMPORARILY_NOT_ACCEPT_PDU_WITH_ANY_SSSAPT
//!
//! This function adds a DM PDU starting at pui8PduBufferPtr with a dm_reason.
//! For more details on this PDU read LLCP V1.1 Section 4.3.8.
//!
//! \return ui8IndexTemp is the length of the DM PDU.
//
//*****************************************************************************
uint8_t LLCP_sendDM(uint8_t * pui8PduBufferPtr,tDisconnectModeReason eDmReason)
{
    uint8_t ui8IndexTemp = 0;

    // DSAP (6 bits)  PTYPE (4 bits)  SSAP (6 bits)
    pui8PduBufferPtr[ui8IndexTemp++] = (g_ui8dsapValue << 2) |
                                            ( (LLCP_DM_PDU & 0xFC) >> 2);
    pui8PduBufferPtr[ui8IndexTemp++] = ( (LLCP_DM_PDU & 0x03) << 6) |
                                            g_ui8ssapValue;

    pui8PduBufferPtr[ui8IndexTemp++] = (uint8_t) eDmReason;

    return ui8IndexTemp;
}

//*****************************************************************************
//
//! Send I message
//!
//! \param pui8PduBufferPtr is the start pointer to store the I PDU.
//!
//! This function adds a I PDU starting at pui8PduBufferPtr.For more details
//! on this PDU read LLCP V1.1 Section 4.3.10.
//!
//! \return ui8IndexTemp is the length of the I PDU.
//
//*****************************************************************************
uint8_t LLCP_sendI(uint8_t * pui8PduBufferPtr)
{
    uint8_t ui8IndexTemp = 0;
    tSNEPConnectionStatus eSnepProtocolStatus;

    // DSAP (6 bits)  PTYPE (4 bits)  SSAP (6 bits)
    pui8PduBufferPtr[ui8IndexTemp++] = (g_ui8dsapValue << 2) | ( (LLCP_I_PDU & 0xFC) >> 2);
    pui8PduBufferPtr[ui8IndexTemp++] = ( (LLCP_I_PDU & 0x03) << 6) | g_ui8ssapValue;

    pui8PduBufferPtr[ui8IndexTemp++] = g_ui8NSNR;

    g_ui8NSNR = (g_ui8NSNR & 0x0F) | (((g_ui8NSNR >> 4) + 0x01) << 4); // Increment N(S)

    if(g_eLLCPConnectionStatus == LLCP_CONNECTION_ESTABLISHED)
    {
        g_eLLCPConnectionStatus = LLCP_CONNECTION_SENDING;
    }
    if(g_eCurrentServiceEnabled == SNEP_SERVICE)
    {
        if(g_eLLCPConnectionStatus == LLCP_CONNECTION_SENDING)
        {
            ui8IndexTemp = ui8IndexTemp +
            SNEP_sendRequest(&pui8PduBufferPtr[ui8IndexTemp],SNEP_REQUEST_PUT);
        }
        else if(g_eLLCPConnectionStatus == LLCP_CONNECTION_RECEIVING)
        {
            eSnepProtocolStatus = SNEP_getProtocolStatus();
            if(eSnepProtocolStatus == SNEP_CONNECTION_RECEIVED_FIRST_PACKET)
            {
                ui8IndexTemp = ui8IndexTemp +
                SNEP_sendResponse(&pui8PduBufferPtr[ui8IndexTemp],
                                    SNEP_RESPONSE_CONTINUE);
            }
            else if(eSnepProtocolStatus == SNEP_CONNECTION_RECEIVE_COMPLETE)
            {
                ui8IndexTemp = ui8IndexTemp +
                SNEP_sendResponse(&pui8PduBufferPtr[ui8IndexTemp],
                                    SNEP_RESPONSE_SUCCESS);
            }
            else if(eSnepProtocolStatus == SNEP_CONNECTION_EXCESS_SIZE)
            {
                ui8IndexTemp = ui8IndexTemp +
                SNEP_sendResponse(&pui8PduBufferPtr[ui8IndexTemp],
                                    SNEP_RESPONSE_REJECT);
            }
        }
    }
    else if(g_eCurrentServiceEnabled == NPP_SERVICE)
    {
        // RFU
    }

    return ui8IndexTemp;
}

//*****************************************************************************
//
//! Send RR message
//!
//! \param pui8PduBufferPtr is the start pointer to store the RR PDU.
//!
//! This function adds a RR PDU starting at pui8PduBufferPtr.For more details
//! on this PDU read LLCP V1.1 Section 4.3.11.
//!
//! \return ui8IndexTemp is the length of the RR PDU.
//
//*****************************************************************************
uint8_t LLCP_sendRR(uint8_t * pui8PduBufferPtr)
{
    uint8_t ui8IndexTemp = 0;

    // DSAP (6 bits)  PTYPE (4 bits)  SSAP (6 bits)
    pui8PduBufferPtr[ui8IndexTemp++] = (g_ui8dsapValue << 2) | \
                                      ((LLCP_RR_PDU & 0xFC) >> 2);
    pui8PduBufferPtr[ui8IndexTemp++] = ( (LLCP_RR_PDU & 0x03) << 6) | \
                                         g_ui8ssapValue;

    // Increment N(R)
    g_ui8NSNR = (g_ui8NSNR & 0xF0) | ((g_ui8NSNR + 0x01) & 0x0F);

    pui8PduBufferPtr[ui8IndexTemp++] = (g_ui8NSNR & 0x0F);

    return ui8IndexTemp;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
