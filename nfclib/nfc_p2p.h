//*****************************************************************************
//
// nfc_p2p.h - contains P2P State Machine NDEF P2P Record Type Structures
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
#ifndef __NFC_P2P_H__
#define __NFC_P2P_H__

//*****************************************************************************
//
//! \addtogroup nfc_p2p_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// NFC Protocol Headers
//
//*****************************************************************************
#include "nfc_f.h"
#include "nfc_dep.h"
#include "llcp.h"
#include "snep.h"

//*****************************************************************************
//
// TRF7970 Header
//
//*****************************************************************************
#include "trf79x0.h"

//*****************************************************************************
//
//! Enumeration for 4 possible states for NFC P2P State Machine.
//
//*****************************************************************************
typedef enum {
    //
    //! Polling/Listening for SENSF_REQ / SENSF_RES.
    //
    NFC_P2P_PROTOCOL_ACTIVATION = 0,

    //
    //! Setting the NFCIDs and bit rate
    //
    NFC_P2P_PARAMETER_SELECTION,

    //
    //! Data exchange using the LLCP layer
    //
    NFC_P2P_DATA_EXCHANGE_PROTOCOL,

    //
    //! Technology deactivation
    //
    NFC_P2P_DEACTIVATION

} tNFCP2PState;

//*****************************************************************************
//
//! This structure defines the status of the received payload.
//
//*****************************************************************************
typedef struct{

    //
    //! SNEP RX Packet Status
    //
    tPacketStatus eDataReceivedStatus;

    //
    //! SNEP Number of bytes received
    //
    uint8_t ui8DataReceivedLength;

    //
    //! Pointer to data received
    //
    uint8_t *pui8RxDataPtr;

}sNFCP2PRxStatus;

//*****************************************************************************
//
// Programmers Note: NDEF message layout
//
// The fields in an NDEF header are as follows:
//  ______________________________
// | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0|  Notes:
// |------------------------------|
// | MB| ME| CF| SR| IL|    TNF   |  NDEF StatusByte
// |------------------------------|
// |        TYPE_LENGTH           |  1 byte, hex value
// |------------------------------|
// |        PAYLOAD_LENGTH        |  1 or 4 bytes (determined by SR) (LSB first)
// |------------------------------|
// |        ID_LENGTH             |  0 or 1 bytes (determined by IL)
// |------------------------------|
// |        TYPE                  |  2 or 5 bytes (determined by TYPE_LENGTH)
// |------------------------------|
// |        ID                    |  0 or 1 byte  (determined by IL & ID_LENGTH)
// |------------------------------|
// |        PAYLOAD               |  X bytes (determined by PAYLOAD_LENGTH)
// |------------------------------|
// NDEF messages NDEF messages can be considered as two parts:
// The Header (everything except the last field), and the Payload.
//
// **********
//   HEADER
// **********
// The Header encompases Everything in the above diagram except the PAYLOAD
// The Header can vary in length from 5-13 bytes.
// The PAYLOAD_LENGTH, ID_LENGTH, ID, and TYPE fields can all very in length.
//
// Field Name      |   Length Depends On | Length
// ------------------------------------------------
// PAYLOAD LENGTH  |   SR                | SR = 1 => 1 byte , SR = 0 => 4 bytes
// ID_LENGTH       |   IL                | IL = 0 (if IL = 0 Both ID_LENGTH and
//                                                  ID fields are excluded.
//                                                  If IL = 1 then ID_LENGTH
//                                                  exists. If ID_LENGTH = 0x0
//                                                  then the ID field is
//                                                  not included)
// TYPE            |   TYPE_LENGTH       | 2-5 bytes (hex value of TYPE_LENGTH)
//                                               (In special cases there can be
//                                                a TYPE_LENGTH of 0, in which
//                                                case there is no TYPE field.)
//
// Note: PAYLOAD_LENGTH only gives the length of the PAYLOAD in its message.
//       The PAYLOAD_LENGTH does NOT give the length of the record across
//       multiple messages..
//
// ***********
//   PAYLOAD
// ***********
// The Payload can have a wide range of formats depending on the TNF and TYPE
// specified. (IE a TNF of 0x01 aka WELL_KNOWN_TYPE and a TYPE of 'T' would
// indicate a plain text payload, which has its own syntax). The user can even
// implement their own PAYLOAD type, providing handlers are provided on both the
// sending and receiving devices.
//
//*****************************************************************************

//*****************************************************************************
//
// NDEF message header definitions
// SET macros are used to set the bit (encoder)
// GET macros are used to read the bit (decoder)
//
//*****************************************************************************

//
//! This macro is used to set the MB field in the StatusByte of the NFC message header by
//! shifting a bit into position. This define should be ORed together with other StatusByte
//! Fields.
//!
//! \param ui8x is the binary value to be shifted into place
//!
//! \b Example: Set the MB field in a StatusByte
//!
//! <tt>sNDEFMessage.sStatusByte = sNDEFMessage.sStatusByte |
//! NDEF_STATUSBYTE_SET_MB(0x1) </tt>
//!
//! \b Example: Clear the MB field in a StatusByte
//!
//! <tt>sNDEFMessage.sStatusByte = sNDEFMessage.sStatusByte &
//! NDEF_STATUSBYTE_SET_MB(0x0) </tt>
//!
//
#define NDEF_STATUSBYTE_SET_MB(ui8x)         ((ui8x & 0x01) << 7)

//
//! This macro is used to set the ME field in the StatusByte of the NFC message
//! header by shifting a bit into position. This define should be ORed together
//! with other StatusByte Fields.
//!
//! \param ui8x is the binary value to be shifted into place
//!
//! \b Example: Set the ME field in a StatusByte
//!
//! <tt>sNDEFMessage.sStatusByte = sNDEFMessage.sStatusByte |
//! NDEF_STATUSBYTE_SET_ME(0x1) </tt>
//!
//! \b Example: Clear the ME field in a StatusByte
//!
//! <tt>sNDEFMessage.sStatusByte = sNDEFMessage.sStatusByte &
//! NDEF_STATUSBYTE_SET_ME(0x0) </tt>
//!
//
#define NDEF_STATUSBYTE_SET_ME(ui8x)         ((ui8x & 0x01) << 6)

//
//! This Macro is used to set the CF field in the StatusByte of the NFC message
//! header by shifting a bit into position. This define should be ORed together
//! with other StatusByte Fields.
//!
//! \param ui8x is the binary value to be shifted into place
//!
//! \b Example: Set the CF field in a StatusByte
//!
//! <tt>sNDEFMessage.sStatusByte = sNDEFMessage.sStatusByte |
//! NDEF_STATUSBYTE_SET_CF(0x1) </tt>
//!
//! \b Example: Clear the CF field in a StatusByte
//!
//! <tt>sNDEFMessage.sStatusByte = sNDEFMessage.sStatusByte &
//! NDEF_STATUSBYTE_SET_CF(0x0) </tt>
//!
//
#define NDEF_STATUSBYTE_SET_CF(ui8x)         ((ui8x & 0x01) << 5)

//
//! This macro is used to set the SR field in the StatusByte of the NFC message
//! header by shifting a bit into position. This define should be ORed together
//! with other StatusByte Fields.
//!
//! \param ui8x is the binary value to be shifted into place
//!
//! \b Example: Set the SR field in a StatusByte
//!
//! <tt>sNDEFMessage.sStatusByte = sNDEFMessage.sStatusByte |
//! NDEF_STATUSBYTE_SET_SR(0x1) </tt>
//!
//! \b Example: Clear the SR field in a StatusByte
//!
//! <tt>sNDEFMessage.sStatusByte = sNDEFMessage.sStatusByte &
//! NDEF_STATUSBYTE_SET_SR(0x0) </tt>
//!
//
#define NDEF_STATUSBYTE_SET_SR(ui8x)         ((ui8x & 0x01) << 4)

//
//! This macro is used to set the IL field in the StatusByte of the NFC message header by
//! shifting a bit into position. This define should be ORed together with other StatusByte
//! Fields.
//!
//! \param ui8x is the binary value to be shifted into place
//!
//! \b Example: Set the IL field in a StatusByte
//!
//! <tt>sNDEFMessage.sStatusByte = sNDEFMessage.sStatusByte |
//! NDEF_STATUSBYTE_SET_IL(0x1) </tt>
//!
//! \b Example: Clear the IL field in a StatusByte
//!
//! <tt>sNDEFMessage.sStatusByte = sNDEFMessage.sStatusByte &
//! NDEF_STATUSBYTE_SET_IL(0x0) </tt>
//!
//
#define NDEF_STATUSBYTE_SET_IL(ui8x)         ((ui8x & 0x01) << 3)

//
//! This macro is used to set the TNF field in the StatusByte of the NFC message
//! header by shifting a bit into position. This define should be ORed together
//! with other StatusByte Fields.
//!
//! \param ui8x is the 3-bit value to be shifted into place
//!
//! \b Example: Set the TNF field to Well Known Type in a StatusByte
//!
//! <tt>NsNDEFMessage.sStatusByte = sNDEFMessage.sStatusByte |
//! NDEF_STATUSBYTE_SET_TNF(0x1) </tt>
//!
//! \b Example: Set the TNF field to Unknown Type in a StatusByte
//!
//! <tt>sNDEFMessage.sStatusByte = sNDEFMessage.sStatusByte &
//! NDEF_STATUSBYTE_SET_TNF(0x5) </tt>
//!
//
#define NDEF_STATUSBYTE_SET_TNF(ui8x)         ((ui8x & 0x07) << 0)

//
//! Macro used to get the MB field value from the StatusByte of the NFC message
//! header.
//!
//! \param ui8x is the 8-bit StatusByte
//!
//! \b Example: Get the MB field from the StatusByte into variable x
//!
//! <tt>x = NDEF_STATUSBYTE_GET_MB(sNDEFMessageData.sStatusByte) </tt>
//!
//
#define NDEF_STATUSBYTE_GET_MB(ui8x)         ((ui8x >> 7) & 0x01)

//
//! Macro used to get the ME field value from the StatusByte of the NFC message
//! header.
//!
//! \param ui8x is the 8-bit StatusByte
//!
//! \b Example: Get the ME field from the StatusByte into variable x
//!
//! <tt>x = NDEF_STATUSBYTE_GET_ME(sNDEFMessageData.sStatusByte) </tt>
//!
//
#define NDEF_STATUSBYTE_GET_ME(ui8x)         ((ui8x >> 6) & 0x01)

//
//! Macro used to get the CF field value from the StatusByte of the NFC message
//! header.
//!
//! \param ui8x is the 8-bit StatusByte
//!
//! \b Example: Get the CF field from the StatusByte into variable x
//!
//! <tt>x = NDEF_STATUSBYTE_GET_CF(sNDEFMessageData.sStatusByte) </tt>
//!
//
#define NDEF_STATUSBYTE_GET_CF(ui8x)         ((ui8x >> 5) & 0x01)

//
//! Macro used to get the SR field value from the StatusByte of the NFC message
//! header.
//!
//! \param ui8x is the 8-bit StatusByte
//!
//! \b Example: Get the SR field from the StatusByte into variable x
//!
//! <tt>x = NDEF_STATUSBYTE_GET_SR(sNDEFMessageData.sStatusByte) </tt>
//!
//
#define NDEF_STATUSBYTE_GET_SR(ui8x)         ((ui8x >> 4) & 0x01)

//
//! Macro used to get the IL field value from the StatusByte of the NFC message
//! header.
//!
//! \param ui8x is the 8-bit StatusByte
//!
//! \b Example: Get the IL field from the StatusByte into variable x
//!
//! <tt>x = NDEF_STATUSBYTE_GET_IL(sNDEFMessageData.sStatusByte) </tt>
//!
//
#define NDEF_STATUSBYTE_GET_IL(ui8x)         ((ui8x >> 3) & 0x01)

//
//! Macro used to get the TNF field value from the StatusByte of the NFC message
//! header.
//!
//! \param ui8x is the 8-bit StatusByte
//!
//! \b Example: Get the TNF field from the StatusByte into variable x
//!
//! <tt>x = NDEF_STATUSBYTE_GET_TNF(sNDEFMessageData.sStatusByte) </tt>
//!
//
#define NDEF_STATUSBYTE_GET_TNF(ui8x)         ((ui8x >> 0) & 0x07)

//*****************************************************************************
//
// Defines to check Header StatusByte field meaning. Some cases left out
// because they are irrelevant (ie only care when MB is 1 or not 1, dont care
// about 0)
//
//*****************************************************************************

//
//! Flag used to check the MB field in the StatusByte. If MB is set then this is
//! the first Record.
//!
//! \b Example: Check for Message Begin flag
//!
//! <tt>if(NDEF_STATUSBYTE_GET_MB(ui8StatusByte) ==
//! NDEF_STATUSBYTE_MB_FIRSTBYTE){} </tt>
//
#define NDEF_STATUSBYTE_MB_FIRSTBYTE        1

//
//! Flag used to check the ME field in the StatusByte. If ME is set, then this
//! is the last Record.
//!
//! \b Example: Check for Message End flag
//!
//! <tt>if(NDEF_STATUSBYTE_GET_ME(ui8StatusByte) == NDEF_STATUSBYTE_ME_LASTBYTE)
//!         {} </tt>
//
#define NDEF_STATUSBYTE_ME_LASTBYTE         1

//
//! Flag used to check the CF field in the StatusByte. If CF is set, then the
//! message is a chunked message spread out across multiple transactions.
//!
//! \b Example: Check for Chunked Flag
//!
//! <tt>if(NDEF_STATUSBYTE_GET_CF(ui8StatusByte) == NDEF_STATUSBYTE_CF_CHUNK)
//!         {} </tt>
//
#define NDEF_STATUSBYTE_CF_CHUNK            1

//
//! Flag used to check the SR field in the StatusByte. If SR is set, then
//! the message is a short record with a payload length field of 1 byte instead
//! of 4 bytes.
//!
//! \b Example: Check the Short Record flag
//!
//! <tt>if(NDEF_STATUSBYTE_GET_SR(ui8StatusByte) ==
//!         NDEF_STATUSBYTE_SR_1BYTEPAYLOADSIZE){} </tt>
//
#define NDEF_STATUSBYTE_SR_1BYTEPAYLOADSIZE         1

//
//! Flag used to check the SR field in the StatusByte. If SR is not set, then
//! the message is a normal record with a payload length field of 4 bytes
//! instead of 1 byte.
//!
//! \b Example: Check the Short Record flag
//!
//! <tt>if(NDEF_STATUSBYTE_GET_SR(ui8StatusByte) ==
//!         NDEF_STATUSBYTE_SR_4BYTEPAYLOADSIZE){} </tt>
//
#define NDEF_STATUSBYTE_SR_4BYTEPAYLOADSIZE         0

//
//! Flag used to check the IL field in the StatusByte. If IL is set, then the ID
//! and IDLength fields are present in the message.
//!
//! \b Example: Check for the presence of the ID Length and ID name field
//!
//! <tt>if(NDEF_STATUSBYTE_GET_IL(ui8StatusByte) ==
//!                                 NDEF_STATUSBYTE_IL_IDLENGTHPRESENT) {} </tt>
//
#define NDEF_STATUSBYTE_IL_IDLENGTHPRESENT         1

//
//! Flag used to check the IL field in the StatusByte. If IL is not set, then
//! there is no ID or IDLength fields included in the message.
//!
//! \b Example: Check for the presence of the ID Length and ID name field
//!
//! <tt>if(NDEF_STATUSBYTE_GET_IL(ui8StatusByte) ==
//!                                 NDEF_STATUSBYTE_IL_IDLENGTHABSENT) {} </tt>
//
#define NDEF_STATUSBYTE_IL_IDLENGTHABSENT         0

//*****************************************************************************
//
// Defines to set maximum field lengths in bytes
//
//*****************************************************************************

//
//! Maximum size of Type field in StatusByte. This define is used to declare the
//! length of the buffer in the structure and thus can be changed to allow
//! larger Type names.
//!
//! \b Example: Copy the Type from raw buffer to structure using
//!             NDEF_TYPE_MAXSIZE to prevent overflowing buffer in the
//!             structure
//!
//! <tt>
//! \verbatim
//! //
//! // Assume that TypeLength is already decoded from the raw buffer and is
//! // stored in sNDEFMessageData.ui8TypeLength. Assume ui8RawBuffer is a
//! // pointer to the beginning of the Type field in the raw data stream.
//! //
//! int x = 0;
//! for(x = 0; (x<NDEF_TYPE_MAXSIZE) & (x<sNDEFMessageData.ui8TypeLength); x++)
//! {
//!     sNDEFMessageData.pui8Type[x]=ui8RawBuffer[x];
//! }
//! \endverbatim
//! </tt>
//
#define NDEF_TYPE_MAXSIZE                   10           // can be changed

//
//! Maximum size of the ID field in StatusByte, which can be modified to support
//! larger ID names.
//!
//! \b Example: Copy the ID from the raw buffer to the structure using
//!             NDEF_ID_MAXSIZE to prevent overflowing buffer in the structure
//!
//! <tt>
//! \verbatim
//! //
//! // Assume IDLength already decoded from the raw buffer is stored in
//! // sNDEFMessageData.ui8IDLength. Assume ui8RawBuffer is a pointer to
//! // the beginning of the ID field in the raw data stream.
//! //
//! int x = 0;
//! for(x = 0; (x<NDEF_ID_MAXSIZE) & (x<sNDEFMessageData.ui8IDLength); x++)
//! {
//!     sNDEFMessageData.pui8pui8ID[x] = ui8RawBuffer[x];
//! }
//! \endverbatim
//! </tt>
//
#define NDEF_ID_MAXSIZE                     10           // can be changed

//*****************************************************************************
//
// Defines to check NDEF ID type
//
//*****************************************************************************

//
//! NFC Message TypeID hex representation for TEXT records.
//! 0x54 == 'T'   in UTF-8
//!
//! \b Example: Check if tag type is TEXT
//!
//! <tt>
//! \verbatim
//! //
//! // Assume the Tag Type has already decoded into sNDEFMessageData.pui8Type
//! //
//! if(sNDEFMessageData.pui8Type == NDEF_TYPE_TEXT)
//! {
//!     // The Tag is a TEXT record, handle it appropriately
//! }
//! \endverbatim
//! </tt>
//
#define NDEF_TYPE_TEXT                      0x54        // 'T'   in UTF-8

//
//! NFC Message TypeID hex representation for URI records.
//! 0x55 == 'U'   in UTF-8
//!
//! \b Example: Check if tag type is URI
//!
//! <tt>
//! \verbatim
//! //
//! // Assume the Tag Type has already decoded into sNDEFMessageData.pui8Type
//! //
//! if(sNDEFMessageData.pui8Type == NDEF_TYPE_URI)
//! {
//!     // The Tag is a URI record, handle it appropriately
//! }
//! \endverbatim
//! </tt>
//
#define NDEF_TYPE_URI                       0x55        // 'U'   in UTF-8

//
//! NFC Message TypeID hex representation for SMARTPOSTER records.
//! 0x5370 == "Sp"   in UTF-8
//!
//! \b Example: Check if tag type is SMARTPOSTER
//!
//! <tt>
//! \verbatim
//! //
//! // Assume the Tag Type has already decoded into sNDEFMessageData.pui8Type
//! //
//! if(sNDEFMessageData.pui8Type == NDEF_TYPE_SMARTPOSTER)
//! {
//!     // The Tag is a SMARTPOSTER record, handle it appropriately
//! }
//! \endverbatim
//! </tt>
//
#define NDEF_TYPE_SMARTPOSTER               0x5370      //"Sp"   in UTF-8

//
//! NFC Message TypeID hex representation for SIGNATURE records.
//! 0x536967 == "Sig"  in UTF-8
//!
//! \b Example: Check if tag type is SIGNATURE
//!
//! <tt>
//! \verbatim
//! //
//! // Assume the Tag Type has already decoded into sNDEFMessageData.pui8Type
//! //
//! if(sNDEFMessageData.pui8Type == NDEF_TYPE_SIGNATURE)
//! {
//!     // The Tag is a SIGNATURE record, handle it appropriately
//! }
//! \endverbatim
//! </tt>
//
#define NDEF_TYPE_SIGNATURE                 0x536967    //"Sig"  in UTF-8

//
//! NFC Message TypeID hex representation for SIZE records.
//! 0x73 == 's'   in UTF-8
//!
//! \b Example: Check if tag type is SIZE
//!
//! <tt>
//! \verbatim
//! //
//! // Assume the Tag Type has already decoded into sNDEFMessageData.pui8Type
//! //
//! if(sNDEFMessageData.pui8Type == NDEF_TYPE_SIZE)
//! {
//!     // The Tag is a SIZE record. Handle it appropriately
//! }
//! \endverbatim
//! </tt>
//
#define NDEF_TYPE_SIZE                      0x73        // 's'   in UTF-8

//
//! NFC Message TypeID hex representation for ACTION records.
//! 0x616374 == "act"  in UTF-8
//!
//! \b Example: Check if tag type is ACTION
//!
//! <tt>
//! \verbatim
//! //
//! // Assume the Tag Type has already decoded into sNDEFMessageData.pui8Type
//! //
//! if(sNDEFMessageData.pui8Type == NDEF_TYPE_ACTION)
//! {
//!     // The Tag is a ACTION record, handle it appropriately
//! }
//! \endverbatim
//! </tt>
//
#define NDEF_TYPE_ACTION                    0x616374    //"act"  in UTF-8



//*****************************************************************************
//
//! Enumeration for Type Name Format (TNF) field in NDEF header StatusByte.
//! TNF values are 3 bits. Most records are of the Well Known Type format
//! (0x01).
//
// TNF = Type Name Format: 3bit field, indicates structure of TYPE field
//          Acceptable Values are:
//          0x00 Empty
//          0x01 NFC Forum well-known type [NFC RTD]
//              NDEF Record Type    Description         Full URI Reference
//                      'Sp'        Smart Poster        urn:nfc:wkt:Sp
//                      'T'         Text                urn:nfc:wkt:T
//                      'U'         URI                 urn:nfc:wkt:U
//                      'Hr'        Handover Request    urn:nfc:wkt:Hr
//                      'Hs'        Handover Select     urn:nfc:wkt:Hs
//                      'Hc'        Handover Carrier    urn:nfc:wkt:Hc
//                      'Sig'       Signature           urn:nfc:wkt:Sig
//          0x02 Media-type as defined in RFC 2046 [RFC 2046]
//          0x03 Absolute URI as defined in RFC 3986 [RFC 3986]
//          0x04 NFC Forum external type [NFC RTD]
//          0x05 Unknown
//          0x06 Unchanged (used with single message across multiple chunks)
//          0x07 Reserved
//
//*****************************************************************************
typedef enum
{
    //
    //! Empty Format
    //
    TNF_EMPTY           = 0x00,

    //
    //! NFC Forum Well Known Type [NFC RTD]
    //
    TNF_WELLKNOWNTYPE   = 0x01,

    //
    //! Media-type as defined in RFC 2046 [RFC 2046]
    //
    TNF_MEDIA_TYPE      = 0x02,

    //
    //! Absolute URI as defined in RFC 3986 [RFC 3986]
    //
    TNF_ABSOLUTE_URI    = 0x03,

    //
    //! NFC Forum external type [NFC RTD]
    //
    TNF_EXTERNAL_TYPE   = 0x04,

    //
    //! Unknown
    //
    TNF_UNKNOWN         = 0x05,

    //
    //! Unchanged (used with single message across multiple chunks)
    //
    TNF_UNCHANGED       = 0x06,

    //
    //! Reserved
    //
    TNF_RESERVED        = 0x07
} tTNF;

//*****************************************************************************
//
//! NFC NDEF message header StatusByte structure. Included in this structure
//! are fields for Message Begin (MB), Message End (ME), Chunk Flag (CF), Short
//! Record (SR), IDLength (IL) and Type Name Format (TNF). The purpose of this
//! structure is to make the fields readily available for message processing.
//  ______________________________
// | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0|
// |------------------------------|
// | MB| ME| CF| SR| IL|    TNF   |
// |------------------------------|
//
// MB = Message Begin : marks start of NDEF message
// ME = Message End   : marks end of NDEF message
// CF = Chunk Flag    : indicate first or middle record chunk of chunked payload
// SR = Short Record  : if == 1 PAYLOAD_LENGTH is 1 byte, else it is 4 bytes
// IL = ID Length     : indicate presence of ID_LENGTH byte
//                      (1 =included, 0 = not)
// TNF = Type Name Format: 3bit field, indicates structure of TYPE field
//
// Note: for a record that only takes up 1 NDEF message both the MB and ME
//       fields would be set on the same message. It is likely that the SR
//       field would be set as well to save space, but not required.
//
//*****************************************************************************
typedef struct
{
    //
    //! Message Begin flag
    //
    bool MB;

    //
    //! Message End flag
    //
    bool ME;

    //
    //! Chunk Flag
    //
    bool CF;

    //
    //! Short Record flag
    //
    bool SR;

    //
    //! ID Length flag
    //
    bool IL;

    //
    //! Type Name Field. An enumeration specifying the general tag type.
    //
    tTNF TNF;

} sNDEFStatusByte;

//*****************************************************************************
//
//! Structure to hold NDEF Message header data. The message header encapsulates
//! and contains metadata about the payload message. This structure is used
//! with the NFCP2P_NDEFMessageEncoder and NFCP2P_NDEFMessageDecoder functions.
//! For detailed information on the NDEF message header data, please see the NFC
//! specification.
//
// NDEF Record Layout
//  _________________
// |   StatusByte    |  1 byte
// |-----------------|
// |   TYPE_LENGTH   |  1 byte, hex value
// |---------------- |
// |  PAYLOAD_LENGTH |  1 or 4 bytes
// |---------------- |
// |   ID_LENGTH     |  0 or 1 bytes
// |---------------- |
// |     TYPE        |  2 or 5 bytes
// |---------------- |
// |      ID         |  0 or 1 byte
// |---------------- |
// |                 |
// |     PAYLOAD     |  Multiple Bytes
// |                 |
// |-----------------|
//
// Note: The Type and ID field lengths are arbitrarily set and can be expanded
// if desired. The PayloadLength field is set to the standard maximum.
// The Payload is set as a pointer into the received buffer.
//
//*****************************************************************************
typedef struct
{

    //
    //! Metadata about the message
    //
    sNDEFStatusByte sStatusByte;

    //
    //! Length of the Type field in bytes
    //
    uint8_t   ui8TypeLength;

    //
    //! Length of the payload in bytes
    //
    uint32_t  ui32PayloadLength;

    //
    //! Length of ID field in bytes. Optional field
    //
    uint8_t   ui8IDLength;

    //
    //! Contains message type
    //
    uint8_t   pui8Type[NDEF_TYPE_MAXSIZE];

    //
    //! Contains message ID. Optional field
    //
    uint8_t   pui8ID[NDEF_ID_MAXSIZE];

    //
    //! Pointer to the encoded payload buffer
    //
    uint8_t *pui8PayloadPtr;

} sNDEFMessageData;

//*****************************************************************************
//
// General defines used to interpret data / set limits on buffer sizes
//
//*****************************************************************************
//
//! Check text record bit in the StatusByte to determine if text record is UTF8
//! format.
//!
//! \b Example: Check Text Record for UTF8 format
//!
//! <tt> if(sNDEFTextRecord.bUTFcode == NDEF_TEXTRECORD_STATUSBYTE_UTF8){}</tt>
//
#define NDEF_TEXTRECORD_STATUSBYTE_UTF8         0       // DO NOT CHANGE

//
//! Check text record bit in the StatusByte to determine if text record is UTF16
//! format.
//!
//! \b Example: Check Text Record for UTF16 format
//!
//! <tt> if(sNDEFTextRecord.bUTFcode == NDEF_TEXTRECORD_STATUSBYTE_UTF16){}</tt>
//
#define NDEF_TEXTRECORD_STATUSBYTE_UTF16        1       // DO NOT CHANGE

//
//! Define the size of the Text Record Language Code Buffer. This can be changed
//! by the user to fit larger language codes that may develop in the future.
//! Current language codes are 2 or 5 bits, but users can use larger sizes
//! if they are adopted in the future.
//
#define NDEF_TEXTRECORD_LANGUAGECODE_MAXSIZE    5       // can be changed

//*****************************************************************************
//
// Set Values into Raw StatusByte by | together
//
//*****************************************************************************

//
//! Set UTF bit field in TextRecord StatusByte field. This define should be ORed
//! together with other StatusByte fields and set into StatusByte.
//!
//! \param ui8x is the 8-bit StatusByte
//!
//! \b Example: Set UTF bit field to UTF8
//!
//! <tt> ui8StatusByte = (NDEF_TEXTRECORD_STATUSBYTE_SET_UTF(
//!                                     NDEF_TEXTRECORD_STATUSBYTE_UTF8) |
//!                       NDEF_TEXTRECORD_STATUSBYTE_SET_LENGTHLANGCODE(...))
//! </tt>
//!
//
#define NDEF_TEXTRECORD_STATUSBYTE_SET_UTF(ui8x)            ((ui8x & 0x01) << 7)

//
//! Set the RFU bit field in the TextRecord StatusByte field. Should be ORed
//! together with other StatusByte fields and set into StatusByte. The RFU field
//! is reserved for future use by the NFC specification and should not be used
//! by normal applications.
//!
//! \param ui8x is the 8-bit StatusByte
//!
//! <tt> ui8StatusByte = (NDEF_TEXTRECORD_STATUSBYTE_SET_RFU(0)|
//!                      (NDEF_TEXTRECORD_STATUSBYTE_SET_LENGTHLANGCODE(...)))
//! </tt>
//!
//
#define NDEF_TEXTRECORD_STATUSBYTE_SET_RFU(ui8x)            ((ui8x & 0x01) << 6)

//
//! Set the Language Code Length field in the TextRecord StatusByte field.
//! This define should be ORed together with other StatusByte fields and set
//! into StatusByte.
//!
//! \param ui8x is the 8-bit StatusByte
//!
//! <tt> ui8StatusByte = (NDEF_TEXTRECORD_STATUSBYTE_SET_LENGTHLANGCODE(5) |
//!                       NDEF_TEXTRECORD_STATUSBYTE_SET_LENGTHLANGCODE(...))
//! </tt>
//!
//
#define NDEF_TEXTRECORD_STATUSBYTE_SET_LENGTHLANGCODE(ui8x) ((ui8x & 0x3F) << 0)

//*****************************************************************************
//
// Get values from Raw StatusByte
//
//*****************************************************************************

//
//! This macro extracts the UTF bit value from the raw StatusByte.
//!
//! \param ui8x is the 8-bit StatusByte
//!
//! \b Example: Fill the UTF boolean value in the data structure from the raw
//! buffer byte
//!
//! <tt> sNDEFTextRecord.bUTFcode = NDEF_TEXTRECORD_STATUSBYTE_GET_UTF(
//!                                             ui8StatusByte)
//! </tt>
//!
//
#define NDEF_TEXTRECORD_STATUSBYTE_GET_UTF(ui8x)            ((ui8x >> 7) & 0x01)

//
//! This macro extracts the RFU bit value from raw StatusByte. According to the
//! NFC specification, this value must be zero.
//!
//! \param ui8x is the 8-bit StatusByte
//!
//! \b Example: Fill the RFU boolean value in the data structure from the raw
//! buffer byte
//!
//! <tt> sNDEFTextRecord.bRFU = NDEF_TEXTRECORD_STATUSBYTE_GET_RFU(
//!                                             ui8StatusByte)
//! </tt>
//!
//
#define NDEF_TEXTRECORD_STATUSBYTE_GET_RFU(ui8x)            ((ui8x >> 6) & 0x01)

//
//! This macro extracts the Language Code Length field from the raw StatusByte.
//!
//! \param ui8x is the 8-bit StatusByte
//!
//! \b Example: Fill the Language Code Length field in the data structure from
//! the raw buffer byte
//!
//! <tt>
//! sNDEFTextRecord.ui5LengthLangCode =
//! NDEF_TEXTRECORD_STATUSBYTE_GET_LENGTHLANGCODE(ui8StatusByte)
//! </tt>
//!
//
#define NDEF_TEXTRECORD_STATUSBYTE_GET_LENGTHLANGCODE(ui8x) ((ui8x >> 0) & 0x3F)

//*****************************************************************************
//
//! This structure defines the text record status byte.
//! bUTFcode determines if the Text Record is encoded with UTF8 (0) or
//! UTF16 (1). bRFU is reserved for future use by the NFC specification.
//! ui5LengthLangCode holds the length of the language code. Currently
//! language code lengths are either 2 or 5 bytes.
//  ______________________________
// | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0|
// |------------------------------|
// |UTF|RFU|  Length of Lang Code | = StatusByte
// |------------------------------|
//
// UTF = UTF8 or UTF16 text string formatting (0 = UTF8, 1 = UTF16)
// RFU = 0, no exceptions, its reserved for future use
// LenLangCode = 6 bytes to determine the Length of Language Code (next field)
//
//*****************************************************************************
typedef struct
{
    //
    //! Flag for UTF Code. 0 = UTF8, 1 = UTF16
    //
    bool bUTFcode;

    //
    //! Reserved for future use by NFC specification
    //
    bool bRFU;

    //
    //! Length of Text Record language code
    //
    uint8_t ui5LengthLangCode;

} sNDEFTextRecordStatusByte;

//*****************************************************************************
//
//! This structure defines the text record. sStatusByte contains the length of
//! the language code and the formatting for the Text (UTF8/UTF16).
//! pui8LanguageCode is a buffer that contains the language code; the buffer
//! size can be changed at compile time by modifying the
//! NDEF_TEXTRECORD_LANGUAGECODE_MAXSIZE define. pui8Text is a pointer to the
//! text payload of the Text Record. These three fields are defined in the
//! NFC specification. In addition, ui32TextLength has been added for
//! convenience to keep track of the Text buffer length. For example, a
//! text record with the Text "hello world" would have a StatusByte of 0x02
//! (UTF = 0 (UTF8), LenLangCode = 0x2), a Language Code of "en"
//! (for English, note that it is 2 bytes long just as the ui5LengthLangCode
//! field of the Text Record StatusByte denoted), pui8Text points to
//! a buffer holding "hello world", and ui32TextLength has a value of 11,
//! which is the number of chars in "hello world".
//
// NDEF message Text Record Payload Layout
//  _________________
// |   StatusByte    |  = 1 byte
// |-----------------|
// |   Language Code |  = 2-5 bytes
// |-----------------|
// |                 |
// |      Text       |  = Multiple Bytes
// |                 |
// |-----------------|
//
// Note: the contents of a text record are freeform plain text in either
//       UTF8 or UTF16 format.
//
// Note: the TextRecordLength is used in lieu of a terminiating sentinel on
//       the puiText buffer.
//
//*****************************************************************************
typedef struct
{

    //
    //! Structure to hold StatusByte information
    //
    sNDEFTextRecordStatusByte sStatusByte;

    //
    //! Buffer that holds the Language Code
    //
    uint8_t   pui8LanguageCode[NDEF_TEXTRECORD_LANGUAGECODE_MAXSIZE];

    //
    //! Pointer to the Text Buffer
    //
    uint8_t *pui8Text;

    //
    //! Length of text in Text Buffer
    //
    uint32_t  ui32TextLength;

} sNDEFTextRecord;

//*****************************************************************************
//
//! Define used to mark end of well-defined URI Record ID Codes. Any code
//! greater than this value is not defined by the NFC specification.
//!
//! \b Example: Check if ID Code of Tag is known defined value
//!
//! <tt>
//! \verbatim
//! if(sNDEFURIRecord.eIDCode < NDEF_URIRECORD_IDCODE_RFU)
//!         {
//!             //process tag
//!         }
//! \endverbatim
//! </tt>
//
//*****************************************************************************
#define NDEF_URIRECORD_IDCODE_RFU               0x24

//*****************************************************************************
//
//! Enumeration of all possible URI Record ID Codes defined by the NFC
//! specification.
//! For the complete list, please see the enumeration definition in nfc_p2p.h.
//! Defined values range from 0x00 (no prepending) to 0x23 ('urn:nfc:').
//! Values 0x24 and above are reserved for future use.
//
//      Acceptable Prepending values are:
//          0x00  N/A. No prepending is done
//          0x01  http://www.
//          0x02  https://www.
//          0x03  http://
//          0x04  https://
//          0x05  tel:
//          0x06  mailto:
//          0x07  ftp://anonymous:anonymous@
//          0x08  ftp://ftp.
//          0x09  ftps://
//          0x0A  sftp://
//          0x0B  smb://
//          0x0C  nfs://
//          0x0D  ftp://
//          0x0E  dav://
//          0x0F  news:
//          0x10  telnet://
//          0x11  imap:
//          0x12  rtsp://
//          0x13  urn:
//          0x14  pop:
//          0x15  sip:
//          0x16  sips:
//          0x17  tftp:
//          0x18  btspp://
//          0x19  btl2cap://
//          0x1A  btgoep://
//          0x1B  tcpobex://
//          0x1C  irdaobex://
//          0x1D  file://
//          0x1E  urn:epc:id:
//          0x1F  urn:epc:tag:
//          0x20  urn:epc:pat:
//          0x21  urn:epc:raw:
//          0x22  urn:epc:
//          0x23  urn:nfc:
//
//          0x24-0xFF RFU Reserved for Future Use, Not Valid Inputs
//
//*****************************************************************************
typedef enum
{

    //
    //! Nothing is prepended to puiUTF8String
    //
    unabridged      =       0x00,

    //
    //! 'http://www.' is prepended to puiUTF8String
    //
    http_www        =       0x01,

    //
    //! 'https://www.' is prepended to puiUTF8String
    //
    https_www       =       0x02,

    //
    //! 'http://' is prepended to puiUTF8String
    //
    http            =       0x03,

    //
    //! 'https://' is prepended to puiUTF8String
    //
    https           =       0x04,

    //
    //! 'tel:' is prepended to puiUTF8String
    //
    tel             =       0x05,

    //
    //! 'mailto:' is prepended to puiUTF8String
    //
    mailto          =       0x06,

    //
    //! 'ftp://anonymous:anonymous@' is prepended to puiUTF8String
    //
    ftp_anonymous   =       0x07,

    //
    //! 'ftp://ftp.' is prepended to puiUTF8String
    //
    ftp_ftp         =       0x08,

    //
    //! 'ftps://' is prepended to puiUTF8String
    //
    ftps            =       0x09,

    //
    //! 'sftp://' is prepended to puiUTF8String
    //
    sftp            =       0x0A,

    //
    //! 'smb://' is prepended to puiUTF8String
    //
    smb             =       0x0B,

    //
    //! 'nfs://' is prepended to puiUTF8String
    //
    nfs             =       0x0C,

    //
    //! 'ftp://' is prepended to puiUTF8String
    //
    ftp             =       0x0D,

    //
    //! 'dav://' is prepended to puiUTF8String
    //
    dav             =       0x0E,

    //
    //! 'news:' is prepended to puiUTF8String
    //
    news            =       0x0F,

    //
    //! 'telnet://' is prepended to puiUTF8String
    //
    telnet          =       0x10,

    //
    //! 'imap:' is prepended to puiUTF8String
    //
    imap            =       0x11,

    //
    //! 'rtsp://' is prepended to puiUTF8String
    //
    rtsp            =       0x12,

    //
    //! 'urn:' is prepended to puiUTF8String
    //
    urn             =       0x13,

    //
    //! 'pop:' is prepended to puiUTF8String
    //
    pop             =       0x14,

    //
    //! 'sip:' is prepended to puiUTF8String
    //
    sip             =       0x15,

    //
    //! 'sips:' is prepended to puiUTF8String
    //
    sips            =       0x16,

    //
    //! 'tftp:' is prepended to puiUTF8String
    //
    tftp            =       0x17,

    //
    //! 'btspp://' is prepended to puiUTF8String
    //
    btspp           =       0x18,

    //
    //! 'btl2cap://' is prepended to puiUTF8String
    //
    btl2cap         =       0x19,

    //
    //! 'btgoep://' is prepended to puiUTF8String
    //
    btgoep          =       0x1A,

    //
    //! 'tcpobex://' is prepended to puiUTF8String
    //
    tcpobex         =       0x1B,

    //
    //! 'irdaobex://' is prepended to puiUTF8String
    //
    irdaobex        =       0x1C,

    //
    //! 'file://' is prepended to puiUTF8String
    //
    file            =       0x1D,

    //
    //! 'urn:epc:id:' is prepended to puiUTF8String
    //
    urn_epc_id      =       0x1E,

    //
    //! 'urn:epc:tag:' is prepended to puiUTF8String
    //
    urn_epc_tag     =       0x1F,

    //
    //! 'urn:epc:pat:' is prepended to puiUTF8String
    //
    urn_epc_pat     =       0x20,

    //
    //! 'urn:epc:raw:' is prepended to puiUTF8String
    //
    urn_epc_raw     =       0x21,

    //
    //! 'urn:epc:' is prepended to puiUTF8String
    //
    urn_epc         =       0x22,

    //
    //! 'urn:nfc:' is prepended to puiUTF8String
    //
    urn_nfc         =       0x23,

    //
    //! Values equal to and above this are reserved for future use (RFU)
    //
    RFU             =       0x24

} eNDEF_URIRecord_IDCode;

//*****************************************************************************
//
//! This structure defines the URI record type. The URI Record Type has two
//! fields; the ID code and the UTF8 URI string. The IDCode is used to
//! determine the URI type. For example, IDcode of 0x06 is 'mailto:'
//! and usually triggers an email event. IDcode 0x01 is 'http://www.' and
//! usually triggers a webpage to open. The IDcode values are prepended to
//! the UTF8 string. ui32URILength is used to determine the length of the
//! puiUTF8String buffer. For example, to direct a user to 'http://www.ti.com'
//! the IDcode is 0x01, the UTF8 string is 'ti.com', and the ui32URILength is
//! 0x6.
//
// NDEF message URI Record Payload Layout
//  _________________
// |     ID Code     |  1 byte
// |-----------------|
// |                 |
// |   UTF8 String   |  Multiple Bytes
// |                 |
// |-----------------|
//
// The URI string is multiple bytes of UTF8 format text with a possible
// prepended value depending on the ID Code
//
//*****************************************************************************
typedef struct
{
    //
    //! Enumeration of all possible ID codes
    //
    eNDEF_URIRecord_IDCode eIDCode;

    //
    //! Buffer that holds the URI character string
    //
    uint8_t *puiUTF8String;

    //
    //! Length of URI Character String
    //
    uint32_t  ui32URILength;

} sNDEFURIRecord;

//*****************************************************************************
//
//! Enumeration of the three actions that can be associated with an Action
//! Record.
//
//*****************************************************************************
typedef enum
{

    //
    //! Do Action on Record
    //
    DO_ACTION           = 0x00,

    //
    //! Save Record for Later
    //
    SAVE_FOR_LATER      = 0x01,

    //
    //! Open Record for Editing
    //
    OPEN_FOR_EDITING    = 0x02

} tAction;

//*****************************************************************************
//
//! This structure defines an Action Record
//
//*****************************************************************************
typedef struct
{

    //
    //! Action Record type enumeration
    //
    tAction eAction;

} sNDEFActionRecord;

//*****************************************************************************
//
//! This structure defines the SmartPoster record type.
//! The SmartPoster Record is essentially
//! a URI Record with other records included for metadata. Thus
//! the SmartPoster must include at least a URI Record and may also include a
//! Text Record for a Title record, an Action record to do actions on the URI,
//! an Icon Record with a small icon, a Size record that holds the size of the
//! externally referenced entity, and a Type record that denotes the type of the
//! externally referenced entity. It should be noted that while the SmartPoster
//! specification can include all these records, this library only provides
//! support for Title, URI and Action records. All other records are ignored
//! by the default handler.
//
// NDEF message SmartPoster Record Payload consists of multiple fully wrapped
// NDEF records. The basic layout is a URI record with subsequent records as
// metadata on size, type, icon, title, and action associated with record.
//
// The possible record types are :
//  Title Record  : multiple possible in different languages (Text Record)
//  URI Record    : 1 and only 1, core of Smart Poster record
//  Action Record : how to treat the URI (Do, Save for later, Open for edit)
//  Icon Record   : MIME type image record [optional]
//  Size Record   : size of external referenced entity (web link) [optional]
//  Type Record   : MIME type of external referenced entity [optional
//
//
// Note: Currently only Title,URI, and Action records are supported.
// Image, Type and size records are not implemented.
//
//*****************************************************************************
typedef struct
{

    //
    //! message header for Text Record
    //
    sNDEFMessageData    sTextHeader;

    //
    //! Text Record payload structure
    //
    sNDEFTextRecord     sTextPayload;

    //
    //! message header for URI Record
    //
    sNDEFMessageData    sURIHeader;

    //
    //! URI Record payload strucutre
    //
    sNDEFURIRecord      sURIPayload;

    //
    //! Flag to signal if Action Record is part of Smart Poster
    //
    bool bActionExists;

    //
    //! message header for Action Record
    //
    sNDEFMessageData    sActionHeader;

    //
    //! Action Record payload strucutre
    //
    sNDEFActionRecord   sActionPayload;

} sNDEFSmartPosterRecord;

//*****************************************************************************
//
// Function Prototypes
//
//*****************************************************************************
void NFCP2P_init(tTRF79x0TRFMode eMode,tTRF79x0Frequency eFrequency);
tNFCP2PState NFCP2P_proccessStateMachine(void);
tStatus NFCP2P_sendPacket(uint8_t *pui8DataPtr, uint32_t ui32DataLength);
sNFCP2PRxStatus NFCP2P_getReceiveState(void);

bool NFCP2P_NDEFMessageEncoder(sNDEFMessageData sNDEFDataToSend,
                                        uint8_t  *pui8Buffer,
                                        uint16_t ui16BufferMaxLength,
                                        uint32_t *pui32BufferLength);
bool NFCP2P_NDEFMessageDecoder(sNDEFMessageData *psNDEFDataDecoded,
                                        uint8_t *pui8Buffer,
                                        uint16_t ui16BufferMaxLength);
bool NFCP2P_NDEFTextRecordEncoder(sNDEFTextRecord sTextRecord,
                                        uint8_t  *pui8Buffer,
                                        uint16_t ui16BufferMaxLength,
                                        uint32_t *ui32BufferLength);
bool NFCP2P_NDEFTextRecordDecoder(sNDEFTextRecord *sTextRecord,
                                        uint8_t *pui8Buffer,
                                        uint32_t  ui32BufferLength);
bool NFCP2P_NDEFURIRecordEncoder(sNDEFURIRecord sURIRecord,
                                        uint8_t  *pui8Buffer,
                                        uint16_t ui16BufferMaxLength,
                                        uint32_t *ui32BufferLength);
bool NFCP2P_NDEFURIRecordDecoder(sNDEFURIRecord *sURIRecord,
                                        uint8_t *pui8Buffer,
                                        uint32_t  ui32BufferLength);
bool NFCP2P_NDEFSmartPosterRecordEncoder(sNDEFSmartPosterRecord sSmartPoster,
                                        uint8_t  *pui8Buffer,
                                        uint16_t ui16BufferMaxLength,
                                        uint32_t *ui32BufferLength);
bool NFCP2P_NDEFSmartPosterRecordDecoder(sNDEFSmartPosterRecord *sSmartPoster,
                                        uint8_t *pui8Buffer,
                                        uint16_t ui16BufferMaxLength,
                                        uint32_t  ui32BufferLength);

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif //__NFC_P2P_H__
