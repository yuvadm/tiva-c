//*****************************************************************************
//
// smbus.h - Prototypes for the SMBus driver.
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
// This is part of revision 2.1.0.12573 of the Tiva Utility Library.
//
//*****************************************************************************

#ifndef __SMBUS_H__
#define __SMBUS_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
//! \addtogroup smbus_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! This structure holds the SMBus Unique Device ID (UDID).  For detailed
//! information, please refer to the SMBus Specification.
//
//*****************************************************************************
typedef struct
{
    //
    //! Device capabilities field.  This 8-bit field reports generic SMBus
    //! capabilities such as address type for ARP.
    //
    uint8_t ui8DeviceCapabilities;

    //
    //! Version Revision field.  This 8-bit field reports UDID revision
    //! information as well as some vendor-specific things such as silicon
    //! revision.
    //
    uint8_t ui8Version;

    //
    //! Vendor ID.  This 16-bit field contains the manufacturer's ID as
    //! assigned by the SBS Implementers' Forum of the PCI SIG.
    //
    uint16_t ui16VendorID;

    //
    //! Device ID.  This 16-bit field contains the device ID assigned by the
    //! device manufacturer.
    //
    uint16_t ui16DeviceID;

    //
    //! Interface.  This 16-bit field identifies the protocol layer interfaces
    //! supported over the SMBus connection.
    //
    uint16_t ui16Interface;

    //
    //! Subsystem Vendor ID.  This 16-bit field holds additional information
    //! that may be derived from the vendor ID or other information.
    //
    uint16_t ui16SubSystemVendorID;

    //
    //! Subsystem Device ID.  This 16-bit field holds additional information
    //! that may be derived from the device ID or other information.
    //
    uint16_t ui16SubSystemDeviceID;

    //
    //! Vendor-specific ID.  This 32-bit field contains a unique number that
    //! can be assigned per device by the manufacturer.
    //
    uint32_t ui32VendorSpecificID;
}
tSMBusUDID;

//*****************************************************************************
//
//! This structure contains the state of a single instance of an SMBus module.
//! Master and slave instances require unique configuration structures.
//
//*****************************************************************************
typedef struct
{
    //
    //! The SMBus Unique Device ID (UDID) for this SMBus instance.  If
    //! operating as a host, master-only, or on a bus that does not use Address
    //! Resolution Protocol (ARP), this is not required.  This member can be
    //! set via a direct structure access or using the SMBusSlaveInit
    //! function.  For detailed information about the UDID, refer to the SMBus
    //! spec.
    //
    tSMBusUDID *pUDID;

    //
    //! The base address of the I2C master peripheral.  This member can be set
    //! via a direct structure access or using the SMBusMasterInit or
    //! SMBusSlaveInit functions.
    //
    uint32_t ui32I2CBase;

    //
    //! The address of the data buffer used for transmit operations.  For
    //! master operations, this member is set by the SMBusMasterxxxx functions
    //! that pass a buffer pointer (for example, SMBusMasterBlockWrite).  For
    //! slave operations, this member can be set via direct structure access or
    //! using the SMBusSlaveTxBufferSet function.
    //
    uint8_t *pui8TxBuffer;

    //
    //! The address of the data buffer used for receive operations.  For master
    //! operations, this member is set by the SMBusMasterxxxx functions that
    //! pass a buffer pointer (for example, SMBusMasterBlockRead).  For slave
    //! operations, this member can be set via direct structure access or using
    //! the SMBusSlaveRxBufferSet function.
    //
    uint8_t *pui8RxBuffer;

    //
    //! The amount of data to transmit from pui8TxBuffer.  For master
    //! operations this member is set by the SMBusMasterxxxx functions either
    //! via an input argument (example SMBusMasterByteWordWrite) or explicitly
    //! (example SMBusMasterSendByte).  In master mode, this member should not
    //! be accessed or modified by the application.  For slave operations, this
    //! member can be set via direct structure access of using the
    //! SMBusSlaveTxBufferSet function.
    //
    uint8_t ui8TxSize;

    //
    //! The current index in the transmit buffer.  This member should not be
    //! accessed or modified by the application.
    //
    uint8_t ui8TxIndex;

    //
    //! The amount of data to receive into pui8RxBuffer.  For master
    //! operations, this member is set by the SMBusMasterxxxx functions either
    //! via an input argument (example SMBusMasterByteWordRead), explicitly
    //! (example SMBusMasterReceiveByte), or by the slave (example
    //! SMBusMasterBlockRead).  In master mode, this member should not be
    //! accessed or modified by the application.  For slave operations, this
    //! member can be set via direct structure access of using the
    //! SMBusSlaveRxBufferSet function.
    //
    uint8_t ui8RxSize;

    //
    //! The current index in the receive buffer.  This member should not be
    //! accessed or modified by the application.
    //
    uint8_t ui8RxIndex;

    //
    //! The active slave address of the I2C peripheral on the device.
    //! When using dual address in slave mode, the active address is store
    //! here.  In master mode, this member is not used.  This member is updated
    //! as requests come in from the master.
    //
    uint8_t ui8OwnSlaveAddress;

    //
    //! The address of the targeted slave device.  In master mode, this member
    //! is set by the ui8TargetSlaveAddress argument in the SMBusMasterxxxx
    //! transfer functions.  In slave mode, it is not used.  This member should
    //! not be modified by the application.
    //
    uint8_t ui8TargetSlaveAddress;

    //
    //! The last used command.  In master mode, this member is set by the
    //! ui8Command argument in the SMBusMasterxxxx transfer functions.  In
    //! slave mode, the first received byte will always be considered the
    //! command.  This member should not be modified by the application.
    //
    uint8_t ui8CurrentCommand;

    //
    //! The running CRC calculation used for transfers that require Packet
    //! Error Checking (PEC).  This member is updated by the SMBus software and
    //! should not be modified by the application.
    //
    uint8_t ui8CalculatedCRC;

    //
    //! The received CRC calculation used for transfers that require Packet
    //! Error Checking (PEC).  This member is updated by the SMBus software and
    //! should not be modified by the application.
    //
    uint8_t ui8ReceivedCRC;

    //
    //! The current state of the SMBusMasterISRProcess state machine.  This
    //! member should not be accessed or modified by the application.
    //
    uint8_t ui8MasterState;

    //
    //! The current state of the SMBusSlaveISRProcess state machine.  This
    //! member should not be accessed or modified by the application.
    //
    uint8_t ui8SlaveState;

    //
    //! Flags used for various items in the SMBus state machines for different
    //! transaction types and status.
    //!
    //! FLAG_PEC can be modified via the SMBusPECEnable or SMBusPECDisable
    //! functions or via direct structure access.
    //!
    //! FLAG_BLOCK_TRANSFER can be set via the SMBusSlaveBlockTransferEnable
    //! function and is cleared automatically by the SMBusSlaveTransferInit
    //! function or manually using the SMBusSlaveBlockTransferDisable function.
    //!
    //! FLAG_RAW_I2C can be modified via the SMBusSlaveI2CEnable or
    //! SMBusSlaveI2CDisable functions or via direct structure access.
    //!
    //! FLAG_TRANSFER_IN_PROGRESS should not be modified by the application,
    //! but can be read via the SMBusStatusGet function.
    //!
    //! FLAG_PROCESS_CALL can be set via the SMBusSlaveProcessCallEnable
    //! function and is cleared automatically by the SMBusSlaveTransferInit
    //! function or manually using the SMBusSlaveProcessCallDisable function.
    //!
    //! FLAG_ADDRESS_RESOLVED is only used by an SMBus Slave that supports ARP.
    //! This flag can be modified via the SMBusSlaveARPFlagARSet function and
    //! read via SMBusSlaveARPFlagARGet.  It can also be modified by direct
    //! structure access.
    //!
    //! FLAG_ADDRESS_VALID is only used by an SMBus Slave that supports ARP.
    //! This flag can be modified via the SMBusSlaveARPFlagAVSet function and
    //! read via SMBusSlaveARPFlagAVGet.  It can also be modified by direct
    //! structure access.
    //!
    //! FLAG_ARP is used to indicate that ARP is currently active.  This flag
    //! is not used by the SMBus stack and can (optionally) be used by the
    //! application to keep track of the ARP session.
    //
    uint16_t ui16Flags;
}
tSMBus;

//*****************************************************************************
//
// ! Return codes.
//
//*****************************************************************************
typedef enum
{
    SMBUS_OK = 0,               // General "OK" return code
    SMBUS_TIMEOUT,              // Master detected bus timeout from slave
    SMBUS_PERIPHERAL_BUSY,      // The I2C peripheral is currently in use
    SMBUS_BUS_BUSY,             // The I2C bus is currently in use
    SMBUS_ARB_LOST,             // Bus arbitration was lost (master mode)
    SMBUS_ADDR_ACK_ERROR,       // In master mode, the address was NAK'd
    SMBUS_DATA_ACK_ERROR,       // Data transfer was NAK'd by receiver
    SMBUS_PEC_ERROR,            // PEC mismatch occurred
    SMBUS_DATA_SIZE_ERROR,      // Data size error has occurred
    SMBUS_MASTER_ERROR,         // Error occurred in the master ISR
    SMBUS_SLAVE_ERROR,          // Error occurred in the slave ISR
    SMBUS_SLAVE_QCMD_0,         // Slave transaction is Quick Command with
                                // data value 0.
    SMBUS_SLAVE_QCMD_1,         // Slave transaction is Quick Command with
                                // data value 1.
    SMBUS_SLAVE_FIRST_BYTE,     // The first byte has been received
    SMBUS_SLAVE_ADDR_PRIMARY,   // Primary address was detected
    SMBUS_SLAVE_ADDR_SECONDARY, // Secondary address was detected
    SMBUS_TRANSFER_IN_PROGRESS, // A transfer is currently in progress
    SMBUS_TRANSFER_COMPLETE,    // The last active transfer is complete
    SMBUS_SLAVE_NOT_READY,      // A slave transmit has been requested, but is
                                // not ready (TX buffer not set).
    SMBUS_FIFO_ERROR,           // A master receive operation did not receive
                                // enough data from the slave.
}
tSMBusStatus;

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// ARP Commands
//
//*****************************************************************************
#define SMBUS_CMD_PREPARE_TO_ARP            0x01
#define SMBUS_CMD_ARP_RESET_DEVICE          0x02
#define SMBUS_CMD_ARP_GET_UDID              0x03
#define SMBUS_CMD_ARP_ASSIGN_ADDRESS        0x04

//*****************************************************************************
//
// Fixed addresses defined by the SMBus specification.
//
//*****************************************************************************
#define SMBUS_ADR_HOST                      0x08
#define SMBUS_ADR_SMART_BATTERY_CHARGER     0x09
#define SMBUS_ADR_SMART_BATTERY_SELECTOR    0x0A
#define SMBUS_ADR_SMART_BATTERY             0x0B
#define SMBUS_ADR_DEFAULT_DEVICE            0x61

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void SMBusPECEnable(tSMBus *psSMBus);
extern void SMBusPECDisable(tSMBus *psSMBus);
extern void SMBusARPEnable(tSMBus *psSMBus);
extern void SMBusARPDisable(tSMBus *psSMBus);
extern tSMBusStatus SMBusStatusGet(tSMBus *psSMBus);
extern void SMBusARPUDIDPacketEncode(tSMBusUDID *pUDID,
                                     uint8_t ui8Address,
                                     uint8_t *pui8Data);
extern void SMBusARPUDIDPacketDecode(tSMBusUDID *pUDID,
                                     uint8_t *pui8Address,
                                     uint8_t *pui8Data);
extern uint8_t SMBusRxPacketSizeGet(tSMBus *psSMBus);
extern void SMBusUDIDDataGet(tSMBus *psSMBus, tSMBusUDID *pUDID);
extern tSMBusStatus SMBusMasterQuickCommand(tSMBus *psSMBus,
                                            uint8_t ui8TargetAddress,
                                            bool bData);
extern tSMBusStatus SMBusMasterByteSend(tSMBus *psSMBus,
                                        uint8_t ui8TargetAddress,
                                        uint8_t ui8Data);
extern tSMBusStatus SMBusMasterByteReceive(tSMBus *psSMBus,
                                           uint8_t ui8TargetAddress,
                                           uint8_t *pui8Data);
extern tSMBusStatus SMBusMasterByteWordWrite(tSMBus *psSMBus,
                                             uint8_t ui8TargetAddress,
                                             uint8_t ui8Command,
                                             uint8_t *pui8Data,
                                             uint8_t ui8Size);
extern tSMBusStatus SMBusMasterBlockWrite(tSMBus *psSMBus,
                                          uint8_t ui8TargetAddress,
                                          uint8_t ui8Command,
                                          uint8_t *pui8Data,
                                          uint8_t ui8Size);
extern tSMBusStatus SMBusMasterByteWordRead(tSMBus *psSMBus,
                                            uint8_t ui8TargetAddress,
                                            uint8_t ui8Command,
                                            uint8_t *pui8Data,
                                            uint8_t ui8Size);
extern tSMBusStatus SMBusMasterBlockRead(tSMBus *psSMBus,
                                         uint8_t ui8TargetAddress,
                                         uint8_t ui8Command,
                                         uint8_t *pui8Data);
extern tSMBusStatus SMBusMasterProcessCall(tSMBus *psSMBus,
                                           uint8_t ui8TargetAddress,
                                           uint8_t ui8Command,
                                           uint8_t *pui8TxData,
                                           uint8_t *pui8RxData);
extern tSMBusStatus SMBusMasterBlockProcessCall(tSMBus *psSMBus,
                                                uint8_t ui8TargetAddress,
                                                uint8_t ui8Command,
                                                uint8_t *pui8TxData,
                                                uint8_t ui8TxSize,
                                                uint8_t *pui8RxData);
extern tSMBusStatus SMBusMasterHostNotify(tSMBus *psSMBus,
                                          uint8_t ui8OwnSlaveAddress,
                                          uint8_t *pui8Data);
extern tSMBusStatus SMBusMasterI2CWrite(tSMBus *psSMBus,
                                        uint8_t ui8TargetAddress,
                                        uint8_t *pui8Data,
                                        uint8_t ui8Size);
extern tSMBusStatus SMBusMasterI2CRead(tSMBus *psSMBus,
                                       uint8_t ui8TargetAddress,
                                       uint8_t *pui8Data,
                                       uint8_t ui8Size);
extern tSMBusStatus SMBusMasterI2CWriteRead(tSMBus *psSMBus,
                                            uint8_t ui8TargetAddress,
                                            uint8_t *pui8TxData,
                                            uint8_t ui8TxSize,
                                            uint8_t *pui8RxData,
                                            uint8_t ui8RxSize);
extern tSMBusStatus SMBusMasterARPGetUDIDGen(tSMBus *psSMBus,
                                             uint8_t *pui8Data);
extern tSMBusStatus SMBusMasterARPGetUDIDDir(tSMBus *psSMBus,
                                             uint8_t ui8TargetAddress,
                                             uint8_t *pui8Data);
extern tSMBusStatus SMBusMasterARPResetDeviceGen(tSMBus *psSMBus);
extern tSMBusStatus SMBusMasterARPResetDeviceDir(tSMBus *psSMBus,
                                                 uint8_t ui8TargetAddress);
extern tSMBusStatus SMBusMasterARPAssignAddress(tSMBus *psSMBus,
                                                uint8_t *pui8Data);
extern tSMBusStatus SMBusMasterARPNotifyMaster(tSMBus *psSMBus,
                                               uint8_t *pui8Data);
extern tSMBusStatus SMBusMasterARPPrepareToARP(tSMBus *psSMBus);
extern tSMBusStatus SMBusMasterIntProcess(tSMBus *psSMBus);
extern void SMBusMasterIntEnable(tSMBus *psSMBus);
extern void SMBusMasterInit(tSMBus *psSMBus, uint32_t ui32I2CBase,
                            uint32_t ui32SMBusClock);
extern void SMBusSlaveTxBufferSet(tSMBus *psSMBus, uint8_t *pui8Data,
                                  uint8_t ui8Size);
extern void SMBusSlaveRxBufferSet(tSMBus *psSMBus, uint8_t *pui8Data,
                                  uint8_t ui8Size);
extern uint8_t SMBusSlaveCommandGet(tSMBus *psSMBus);
extern void SMBusSlaveProcessCallEnable(tSMBus *psSMBus);
extern void SMBusSlaveProcessCallDisable(tSMBus *psSMBus);
extern void SMBusSlaveBlockTransferEnable(tSMBus *psSMBus);
extern void SMBusSlaveBlockTransferDisable(tSMBus *psSMBus);
extern void SMBusSlaveI2CEnable(tSMBus *psSMBus);
extern void SMBusSlaveI2CDisable(tSMBus *psSMBus);
extern void SMBusSlaveARPFlagARSet(tSMBus *psSMBus, bool bValue);
extern bool SMBusSlaveARPFlagARGet(tSMBus *psSMBus);
extern void SMBusSlaveARPFlagAVSet(tSMBus *psSMBus, bool bValue);
extern bool SMBusSlaveARPFlagAVGet(tSMBus *psSMBus);
extern void SMBusSlaveTransferInit(tSMBus *psSMBus);
extern tSMBusStatus SMBusSlaveIntProcess(tSMBus *psSMBus);
extern tSMBusStatus SMBusSlaveDataSend(tSMBus *psSMBus);
extern void SMBusSlaveACKSend(tSMBus *psSMBus, bool bACK);
extern void SMBusSlaveManualACKEnable(tSMBus *psSMBus);
extern void SMBusSlaveManualACKDisable(tSMBus *psSMBus);
extern bool SMBusSlaveManualACKStatusGet(tSMBus *psSMBus);
extern tSMBusStatus SMBusSlaveIntAddressGet(tSMBus *psSMBus);
extern void SMBusSlaveIntEnable(tSMBus *psSMBus);
extern void SMBusSlaveUDIDSet(tSMBus *psSMBus, tSMBusUDID *pUDID);
extern void SMBusSlaveAddressSet(tSMBus *psSMBus, uint8_t ui8AddressNum,
                                 uint8_t ui8SlaveAddress);
extern void SMBusSlaveInit(tSMBus *psSMBus, uint32_t ui32I2CBase);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __SMBUS_H__
