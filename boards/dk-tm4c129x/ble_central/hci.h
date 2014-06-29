//*****************************************************************************
//
// hci.h - This file contains the types, contants, external functions
//         etc. for the BLE HCI Transport Layer.
//         This file copied most of #defines from BLE-Stack 1.3.2 release
//
// Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C129X Firmware Package.
//
//*****************************************************************************

#ifndef HCI_H
#define HCI_H

#ifdef __cplusplus
extern "C"
{
#endif

//
// Long Term Key info
//
typedef struct
{
    bool    bValid;
    bool    bAuth;
    uint8_t ui8LTKSize;
    uint8_t pui8LTK[16];
    uint8_t pui8DIV[2];
    uint8_t pui8Rand[8];
} tLTKData;

#define HCI_BDADDR_LEN                6

//
// HCI Packet Types
//
#define HCI_CMD_PACKET                 0x01
#define HCI_ACL_DATA_PACKET            0x02
#define HCI_SCO_DATA_PACKET            0x03
#define HCI_EVENT_PACKET               0x04

//
// Vendor Specific Command Opcode
//
#define HCI_VE_GAP_DEVICE_INIT_OPCODE              0xFE00
#define HCI_VE_GAP_DEVICE_DISC_REQ_OPCODE          0xFE04
#define HCI_VE_GAP_DEVICE_EST_LINK_REQ_OPCODE      0xFE09
#define HCI_VE_GAP_DEVICE_TER_LINK_REQ_OPCODE      0xFE0A
#define HCI_VE_GAP_DEVICE_AUTHENTICATE_OPCODE      0xFE0B
#define HCI_VE_GAP_DEVICE_PASSKEY_UPDATE_OPCODE    0xFE0C
#define HCI_VE_GAP_DEVICE_BOND_OPCODE              0xFE0F
#define HCI_VE_GAP_SET_PARAM_OPCODE                0xFE30
#define HCI_VE_GAP_GET_PARAM_OPCODE                0xFE31
#define HCI_VE_GAP_DEVICE_READ_CHAR_VAL_OPCODE     0xFD8A
#define HCI_VE_GAP_DEVICE_WRITE_CHAR_VAL_OPCODE    0xFD92

//
// BLE Command Opcode
//
#define HCI_READ_RSSI_OPCODE                       0x1405

//
// Characteristic Value Handle
//
#define GATT_IRTEMP_DATA_UUID_HANDLE               0x25
#define GATT_IRTEMP_NOTIFY_UUID_HANDLE             0x26
#define GATT_IRTEMP_CFG_UUID_HANDLE                0x29
#define GATT_HUMIDITY_DATA_UUID_HANDLE             0x38
#define GATT_HUMIDITY_NOTIDY_UUID_HANDLE           0x39
#define GATT_HUMIDITY_CFG_UUID_HANDLE              0x3C

// 
// GAP Profile Roles Bit mask values
//
#define GAP_PROFILE_BROADCASTER   0x01 //A device that sends advertising events only.
#define GAP_PROFILE_OBSERVER      0x02 //A device that receives advertising events only.
#define GAP_PROFILE_PERIPHERAL    0x04 //A device that accepts the establishment of an LE physical link using the connection establishment procedure
#define GAP_PROFILE_CENTRAL       0x08 //A device that supports the Central role initiates the establishment of a physical connection


//
// GAP_PARAMETER_ID_DEFINES GAP Parameter IDs
//
#define TGAP_GEN_DISC_ADV_MIN          0  //!< Minimum time to remain advertising, when in Discoverable mode (mSec).  Setting this parameter to 0 turns off the timeout (default).
#define TGAP_LIM_ADV_TIMEOUT           1  //!< Maximum time to remain advertising, when in Limited Discoverable mode. In seconds (default 180 seconds)
#define TGAP_GEN_DISC_SCAN             2  //!< Minimum time to perform scanning, when performing General Discovery proc (mSec)
#define TGAP_LIM_DISC_SCAN             3  //!< Minimum time to perform scanning, when performing Limited Discovery proc (mSec)
#define TGAP_CONN_EST_ADV_TIMEOUT      4  //!< Advertising timeout, when performing Connection Establishment proc (mSec)
#define TGAP_CONN_PARAM_TIMEOUT        5  //!< Link Layer connection parameter update notification timer, connection parameter update proc (mSec)

// Constants
#define TGAP_LIM_DISC_ADV_INT_MIN      6  //!< Minimum advertising interval, when in limited discoverable mode (n * 0.625 mSec)
#define TGAP_LIM_DISC_ADV_INT_MAX      7  //!< Maximum advertising interval, when in limited discoverable mode (n * 0.625 mSec)
#define TGAP_GEN_DISC_ADV_INT_MIN      8  //!< Minimum advertising interval, when in General discoverable mode (n * 0.625 mSec)
#define TGAP_GEN_DISC_ADV_INT_MAX      9  //!< Maximum advertising interval, when in General discoverable mode (n * 0.625 mSec)
#define TGAP_CONN_ADV_INT_MIN         10  //!< Minimum advertising interval, when in Connectable mode (n * 0.625 mSec)
#define TGAP_CONN_ADV_INT_MAX         11  //!< Maximum advertising interval, when in Connectable mode (n * 0.625 mSec)
#define TGAP_CONN_SCAN_INT            12  //!< Scan interval used during Link Layer Initiating state, when in Connectable mode (n * 0.625 mSec)
#define TGAP_CONN_SCAN_WIND           13  //!< Scan window used during Link Layer Initiating state, when in Connectable mode (n * 0.625 mSec)
#define TGAP_CONN_HIGH_SCAN_INT       14  //!< Scan interval used during Link Layer Initiating state, when in Connectable mode, high duty scan cycle scan paramaters (n * 0.625 mSec)
#define TGAP_CONN_HIGH_SCAN_WIND      15  //!< Scan window used during Link Layer Initiating state, when in Connectable mode, high duty scan cycle scan paramaters (n * 0.625 mSec)
#define TGAP_GEN_DISC_SCAN_INT        16  //!< Scan interval used during Link Layer Scanning state, when in General Discovery proc (n * 0.625 mSec)
#define TGAP_GEN_DISC_SCAN_WIND       17  //!< Scan window used during Link Layer Scanning state, when in General Discovery proc (n * 0.625 mSec)
#define TGAP_LIM_DISC_SCAN_INT        18  //!< Scan interval used during Link Layer Scanning state, when in Limited Discovery proc (n * 0.625 mSec)
#define TGAP_LIM_DISC_SCAN_WIND       19  //!< Scan window used during Link Layer Scanning state, when in Limited Discovery proc (n * 0.625 mSec)
#define TGAP_CONN_EST_ADV             20  //!< Advertising interval, when using Connection Establishment proc (n * 0.625 mSec). Obsolete - Do not use.
#define TGAP_CONN_EST_INT_MIN         21  //!< Minimum Link Layer connection interval, when using Connection Establishment proc (n * 1.25 mSec)
#define TGAP_CONN_EST_INT_MAX         22  //!< Maximum Link Layer connection interval, when using Connection Establishment proc (n * 1.25 mSec)
#define TGAP_CONN_EST_SCAN_INT        23  //!< Scan interval used during Link Layer Initiating state, when using Connection Establishment proc (n * 0.625 mSec)
#define TGAP_CONN_EST_SCAN_WIND       24  //!< Scan window used during Link Layer Initiating state, when using Connection Establishment proc (n * 0.625 mSec)
#define TGAP_CONN_EST_SUPERV_TIMEOUT  25  //!< Link Layer connection supervision timeout, when using Connection Establishment proc (n * 10 mSec)
#define TGAP_CONN_EST_LATENCY         26  //!< Link Layer connection slave latency, when using Connection Establishment proc (in number of connection events)
#define TGAP_CONN_EST_MIN_CE_LEN      27  //!< Local informational parameter about min len of connection needed, when using Connection Establishment proc (n * 0.625 mSec)
#define TGAP_CONN_EST_MAX_CE_LEN      28  //!< Local informational parameter about max len of connection needed, when using Connection Establishment proc (n * 0.625 mSec)
#define TGAP_PRIVATE_ADDR_INT         29  //!< Minimum Time Interval between private (resolvable) address changes. In minutes (default 15 minutes)
#define TGAP_CONN_PAUSE_CENTRAL       30  //!< Central idle timer. In seconds (default 1 second)
#define TGAP_CONN_PAUSE_PERIPHERAL    31  //!< Minimum time upon connection establishment before the peripheral starts a connection update procedure. In seconds (default 5 seconds)

// Proprietary
#define TGAP_SM_TIMEOUT               32  //!< SM Message Timeout (milliseconds). Default 30 seconds.
#define TGAP_SM_MIN_KEY_LEN           33  //!< SM Minimum Key Length supported. Default 7.
#define TGAP_SM_MAX_KEY_LEN           34  //!< SM Maximum Key Length supported. Default 16.
#define TGAP_FILTER_ADV_REPORTS       35  //!< Filter duplicate advertising reports. Default TRUE.
#define TGAP_SCAN_RSP_RSSI_MIN        36  //!< Minimum RSSI required for scan responses to be reported to the app. Default -127.

//
// GAP Device Discovery Modes
//
#define DEVDISC_MODE_NONDISCOVERABLE  0x00    //!< No discoverable setting
#define DEVDISC_MODE_GENERAL          0x01    //!< General Discoverable devices
#define DEVDISC_MODE_LIMITED          0x02    //!< Limited Discoverable devices
#define DEVDISC_MODE_ALL              0x03    //!< Not filtered

//
// GAP Address Types
//
#define ADDRTYPE_PUBLIC               0x00  //!< Use the BD_ADDR
#define ADDRTYPE_STATIC               0x01  //!< Static address
#define ADDRTYPE_PRIVATE_NONRESOLVE   0x02  //!< Generate Non-Resolvable Private Address
#define ADDRTYPE_PRIVATE_RESOLVE      0x03  //!< Generate Resolvable Private Address

//
// GAP Advertiser Event Types
//
#define GAP_ADTYPE_ADV_IND                0x00  //!< Connectable undirected advertisement
#define GAP_ADTYPE_ADV_DIRECT_IND         0x01  //!< Connectable directed advertisement
#define GAP_ADTYPE_ADV_DISCOVER_IND       0x02  //!< Discoverable undirected advertisement
#define GAP_ADTYPE_ADV_NONCONN_IND        0x03  //!< Non-Connectable undirected advertisement
#define GAP_ADTYPE_SCAN_RSP_IND           0x04  //!< Only used in gapDeviceInfoEvent_t

// 
// Vendor Specific Event Code
//
#define HCI_VE_EVENT_CODE                 0xFF

// Vendor Specific Event
#define GAP_HCI_EVENT_EXT_CMD_STATUS                         0x067F
#define GAP_HCI_EVENT_EXT_DEVICE_INIT_DONE                   0x0600
#define GAP_HCI_EVENT_EXT_DEVICE_INFO                        0x060D
#define GAP_HCI_EVENT_EXT_DEVICE_DISC_DONE                   0x0601
#define GAP_HCI_EVENT_EXT_DEVICE_LINK_DONE                   0x0605
#define GAP_HCI_EVENT_EXT_DEVICE_TERM_LINK_DONE              0x0606
#define GAP_HCI_EVENT_EXT_DEVICE_PASSKEY_NEEDED              0x060B
#define GAP_HCI_EVENT_EXT_DEVICE_AUTHENTICATE_DONE           0x060A
#define GAP_HCI_EVENT_EXT_DEVICE_BOND_DONE                   0x060E
#define GAP_HCI_EVENT_EXT_ATT_READ_RSP                       0x050B
#define GAP_HCI_EVENT_EXT_ATT_WRITE_RSP                      0x0513
#define GAP_HCI_EVENT_CMD_COMPLETE                           0x0E
#define HCI_ATT_ERROR_RSP_EVENT                              0x0501
#define GAP_HCI_EVENT_HANDLE_VALUE_NOTIFY                    0x051B

/*
** HCI Status
**
** Per the Bluetooth Core Specification, V4.0.0, Vol. 2, Part D.
*/
#define HCI_SUCCESS                                                         0x00
#define HCI_ERROR_CODE_UNKNOWN_HCI_CMD                                      0x01
#define HCI_ERROR_CODE_UNKNOWN_CONN_ID                                      0x02
#define HCI_ERROR_CODE_HW_FAILURE                                           0x03
#define HCI_ERROR_CODE_PAGE_TIMEOUT                                         0x04
#define HCI_ERROR_CODE_AUTH_FAILURE                                         0x05
#define HCI_ERROR_CODE_PIN_KEY_MISSING                                      0x06
#define HCI_ERROR_CODE_MEM_CAP_EXCEEDED                                     0x07
#define HCI_ERROR_CODE_CONN_TIMEOUT                                         0x08
#define HCI_ERROR_CODE_CONN_LIMIT_EXCEEDED                                  0x09
#define HCI_ERROR_CODE_SYNCH_CONN_LIMIT_EXCEEDED                            0x0A
#define HCI_ERROR_CODE_ACL_CONN_ALREADY_EXISTS                              0x0B
#define HCI_ERROR_CODE_CMD_DISALLOWED                                       0x0C
#define HCI_ERROR_CODE_CONN_REJ_LIMITED_RESOURCES                           0x0D
#define HCI_ERROR_CODE_CONN_REJECTED_SECURITY_REASONS                       0x0E
#define HCI_ERROR_CODE_CONN_REJECTED_UNACCEPTABLE_BDADDR                    0x0F
#define HCI_ERROR_CODE_CONN_ACCEPT_TIMEOUT_EXCEEDED                         0x10
#define HCI_ERROR_CODE_UNSUPPORTED_FEATURE_PARAM_VALUE                      0x11
#define HCI_ERROR_CODE_INVALID_HCI_CMD_PARAMS                               0x12
#define HCI_ERROR_CODE_REMOTE_USER_TERM_CONN                                0x13
#define HCI_ERROR_CODE_REMOTE_DEVICE_TERM_CONN_LOW_RESOURCES                0x14
#define HCI_ERROR_CODE_REMOTE_DEVICE_TERM_CONN_POWER_OFF                    0x15
#define HCI_ERROR_CODE_CONN_TERM_BY_LOCAL_HOST                              0x16
#define HCI_ERROR_CODE_REPEATED_ATTEMPTS                                    0x17
#define HCI_ERROR_CODE_PAIRING_NOT_ALLOWED                                  0x18
#define HCI_ERROR_CODE_UNKNOWN_LMP_PDU                                      0x19
#define HCI_ERROR_CODE_UNSUPPORTED_REMOTE_FEATURE                           0x1A
#define HCI_ERROR_CODE_SCO_OFFSET_REJ                                       0x1B
#define HCI_ERROR_CODE_SCO_INTERVAL_REJ                                     0x1C
#define HCI_ERROR_CODE_SCO_AIR_MODE_REJ                                     0x1D
#define HCI_ERROR_CODE_INVALID_LMP_PARAMS                                   0x1E
#define HCI_ERROR_CODE_UNSPECIFIED_ERROR                                    0x1F
#define HCI_ERROR_CODE_UNSUPPORTED_LMP_PARAM_VAL                            0x20
#define HCI_ERROR_CODE_ROLE_CHANGE_NOT_ALLOWED                              0x21
#define HCI_ERROR_CODE_LMP_LL_RESP_TIMEOUT                                  0x22
#define HCI_ERROR_CODE_LMP_ERR_TRANSACTION_COLLISION                        0x23
#define HCI_ERROR_CODE_LMP_PDU_NOT_ALLOWED                                  0x24
#define HCI_ERROR_CODE_ENCRYPT_MODE_NOT_ACCEPTABLE                          0x25
#define HCI_ERROR_CODE_LINK_KEY_CAN_NOT_BE_CHANGED                          0x26
#define HCI_ERROR_CODE_REQ_QOS_NOT_SUPPORTED                                0x27
#define HCI_ERROR_CODE_INSTANT_PASSED                                       0x28
#define HCI_ERROR_CODE_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED                  0x29
#define HCI_ERROR_CODE_DIFFERENT_TRANSACTION_COLLISION                      0x2A
#define HCI_ERROR_CODE_RESERVED1                                            0x2B
#define HCI_ERROR_CODE_QOS_UNACCEPTABLE_PARAM                               0x2C
#define HCI_ERROR_CODE_QOS_REJ                                              0x2D
#define HCI_ERROR_CODE_CHAN_ASSESSMENT_NOT_SUPPORTED                        0x2E
#define HCI_ERROR_CODE_INSUFFICIENT_SECURITY                                0x2F
#define HCI_ERROR_CODE_PARAM_OUT_OF_MANDATORY_RANGE                         0x30
#define HCI_ERROR_CODE_RESERVED2                                            0x31
#define HCI_ERROR_CODE_ROLE_SWITCH_PENDING                                  0x32
#define HCI_ERROR_CODE_RESERVED3                                            0x33
#define HCI_ERROR_CODE_RESERVED_SLOT_VIOLATION                              0x34
#define HCI_ERROR_CODE_ROLE_SWITCH_FAILED                                   0x35
#define HCI_ERROR_CODE_EXTENDED_INQUIRY_RESP_TOO_LARGE                      0x36
#define HCI_ERROR_CODE_SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST                 0x37
#define HCI_ERROR_CODE_HOST_BUSY_PAIRING                                    0x38
#define HCI_ERROR_CODE_CONN_REJ_NO_SUITABLE_CHAN_FOUND                      0x39
#define HCI_ERROR_CODE_CONTROLLER_BUSY                                      0x3A
#define HCI_ERROR_CODE_UNACCEPTABLE_CONN_INTERVAL                           0x3B
#define HCI_ERROR_CODE_DIRECTED_ADV_TIMEOUT                                 0x3C
#define HCI_ERROR_CODE_CONN_TERM_MIC_FAILURE                                0x3D
#define HCI_ERROR_CODE_CONN_FAILED_TO_ESTABLISH                             0x3E
#define HCI_ERROR_CODE_MAC_CONN_FAILED                                      0x3F

//
// Generic Status Return Values
//
#define SUCCESS                         0x00
#define FAILURE                         0x01
#define INVALIDPARAMETER                0x02
#define INVALID_TASK                    0x03
#define MSG_BUFFER_NOT_AVAIL            0x04
#define INVALID_MSG_POINTER             0x05
#define INVALID_EVENT_ID                0x06
#define INVALID_INTERRUPT_ID            0x07
#define NO_TIMER_AVAIL                  0x08
#define NV_ITEM_UNINIT                  0x09
#define NV_OPER_FAILED                  0x0A
#define INVALID_MEM_SIZE                0x0B
#define NV_BAD_ITEM_LEN                 0x0C

#define bleNotReady                     0x10  //!< Not ready to perform task
#define bleAlreadyInRequestedMode       0x11  //!< Already performing that task
#define bleIncorrectMode                0x12  //!< Not setup properly to perform that task
#define bleMemAllocError                0x13  //!< Memory allocation error occurred
#define bleNotConnected                 0x14  //!< Can't perform function when not in a connection
#define bleNoResources                  0x15  //!< There are no resource available
#define blePending                      0x16  //!< Waiting
#define bleTimeout                      0x17  //!< Timed out performing function
#define bleInvalidRange                 0x18  //!< A parameter is out of range
#define bleLinkEncrypted                0x19  //!< The link is already encrypted
#define bleProcedureComplete            0x1A  //!< The Procedure is completed

// GAP Status Return Values - returned as bStatus_t
#define bleGAPUserCanceled              0x30  //!< The user canceled the task
#define bleGAPConnNotAcceptable         0x31  //!< The connection was not accepted
#define bleGAPBondRejected              0x32  //!< The bound information was rejected.

// ATT Status Return Values - returned as bStatus_t
#define bleInvalidPDU                   0x40  //!< The attribute PDU is invalid
#define bleInsufficientAuthen           0x41  //!< The attribute has insufficient authentication
#define bleInsufficientEncrypt          0x42  //!< The attribute has insufficient encryption
#define bleInsufficientKeySize          0x43  //!< The attribute has insufficient encryption key size

// Disconnect Reasons
#define HCI_DISCONNECT_AUTH_FAILURE                    HCI_ERROR_CODE_AUTH_FAILURE
#define HCI_DISCONNECT_REMOTE_USER_TERM                HCI_ERROR_CODE_REMOTE_USER_TERM_CONN
#define HCI_DISCONNECT_REMOTE_DEV_LOW_RESOURCES        HCI_ERROR_CODE_REMOTE_DEVICE_TERM_CONN_LOW_RESOURCES
#define HCI_DISCONNECT_REMOTE_DEV_POWER_OFF            HCI_ERROR_CODE_REMOTE_DEVICE_TERM_CONN_POWER_OFF
#define HCI_DISCONNECT_UNSUPPORTED_REMOTE_FEATURE      HCI_ERROR_CODE_UNSUPPORTED_REMOTE_FEATURE
#define HCI_DISCONNECT_KEY_PAIRING_NOT_SUPPORTED       HCI_ERROR_CODE_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED
#define HCI_DISCONNECT_UNACCEPTABLE_CONN_INTERVAL      HCI_ERROR_CODE_UNACCEPTABLE_CONN_INTERVAL

extern void GAPDeviceInit(uint8_t  ui8ProfileRole,
                          uint8_t  ui8MaxScanRsps,
                          uint8_t  *pui8IRK,
                          uint8_t  *pui8SRK,
                          uint32_t ui32SignCounter);
extern void GAPGetParam(uint8_t ui8ParamID);
extern void GAPSetParam(uint8_t ui8ParamID, uint16_t ui16Value);
extern void GAPDiscoveryReq(uint8_t ui8Mode, bool bActiveScan, bool bWhiteList);
extern void GAPEstLinkReq(bool bHighDutyCycle, bool bWhiteList,
                          uint8_t ui8AddrType, uint8_t *pui8DevAddr);
extern void GAPTerLinkReq(uint16_t ui16ConnHandle, uint8_t ui8Reason);
extern void GAPAuthenticate(uint16_t ui16ConnHandle);
extern void GAPPassKeyUpdate(uint16_t ui16ConnHandle, char *pcPassCode);
extern void GAPBond(uint16_t ui16ConnHandle, tLTKData *psSavedKey);
extern void GAPReadCharValue(uint16_t ui16ConnHandle, uint16_t ui16Handle);
extern void GAPWriteCharValue(uint16_t ui16ConnHandle, uint16_t ui16Handle,
                              uint8_t *pui8Buf, uint8_t ui8Len);
extern void HCIReadRSSI(uint16_t ui16ConnHandle);
#ifdef __cplusplus
}
#endif

#endif /* HCI_H */
