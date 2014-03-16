//*****************************************************************************
//
// usbdmsc.h - USB mass storage device class driver.
//
// Copyright (c) 2009-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the Tiva USB Library.
//
//*****************************************************************************

#ifndef __USBDMSC_H__
#define __USBDMSC_H__

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
//! \addtogroup msc_device_class_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Media Access functions.
//
//*****************************************************************************
typedef struct
{
    //
    //! This function is used to initialize and open the physical drive number
    //! associated with the parameter \e ui32Drive.  The function returns
    //! zero if the drive could not be opened for some reason.  In the case of
    //! removable device like an SD card this function must return zero if
    //! the SD card is not present.
    //! The function returns a pointer to data that should be passed to other
    //! APIs or returns 0 if no drive was found.
    //
    void *(*pfnOpen)(uint32_t ui32Drive);

    //*************************************************************************
    //
    //! This function closes the drive number in use by the mass storage class
    //! device.  The \e pvDrive is the pointer that was returned from a call to
    //! \e pfnOpen. This function is used to close the physical drive
    //! number associated with the parameter \e pvDrive.  This function
    //! returns 0 if the drive was closed successfully and any other value
    //! indicates a failure.
    //
    //*************************************************************************
    void (*pfnClose)(void *pvDrive);

    //*************************************************************************
    //
    //! This function reads a block of data from a device opened by the
    //! \e pfnOpen call.  The \e pvDrive parameter is the pointer that was
    //! returned from the original call to \e pfnOpen.  The \e pui8Data
    //! parameter is the buffer that data will be written into.  The data area
    //! pointed to by \e pui8Data must be at least \e ui32NumBlocks * Block
    //! Size bytes to prevent overwriting data. The \e ui32Sector is the block
    //! address to read and \e ui32NumBlocks is the number of blocks to read.
    //! This function returns the number of bytes that were read from the
    //! and placed into the \e pui8Data buffer..
    //
    //*************************************************************************
    uint32_t (*pfnBlockRead)(void *pvDrive, uint8_t *pui8Data,
                                uint32_t ui32Sector, uint32_t ui32NumBlocks);

    //*************************************************************************
    //
    //! This function is use to write blocks to a physical device from the
    //! buffer pointed to by the \e pui8Data buffer. The \e pvDrive parameter
    //! is the pointer that was returned from the original call to \e pfnOpen.
    //! The \e pui8Data is the pointer to the data to write to the storage
    //! device and \e ui32NumBlocks is the number of blocks to write.  The
    //! \e ui32Sector parameter is the sector number used to write the block.
    //! If the number of blocks is greater than one then the block address
    //! increments and writes to the next block until
    //! \e ui32NumBlocks * Block Size bytes are written.  This function returns
    //! the number of bytes that were written to the device.
    //
    //*************************************************************************
    uint32_t (*pfnBlockWrite)(void *pvDrive, uint8_t *pui8Data,
                              uint32_t ui32Sector, uint32_t ui32NumBlocks);

    //*************************************************************************
    //
    //! This function returns the total number of blocks on a physical device
    //! based on the \e pvDrive parameter.  The \e pvDrive parameter
    //! is the pointer that was returned from the original call to \e pfnOpen.
    //
    //*************************************************************************
    uint32_t (*pfnNumBlocks)(void *pvDrive);

    //*************************************************************************
    //
    //! This function returns the block size for a physical device based on the
    //! \e pvDrive parameter.  The \e pvDrive parameter is the pointer
    //! that was returned from the original call to \e pfnOpen.
    //
    //*************************************************************************
    uint32_t (*pfnBlockSize)(void *pvDrive);

}
tMSCDMedia;

//*****************************************************************************
//
// These defines control the default sizes of USB transfers for data and
// commands.
//
//*****************************************************************************
#define DEVICE_BLOCK_SIZE       512

//*****************************************************************************
//
// USBDMSCMediaChange() tUSBDMSCMediaStatus values.
//
//*****************************************************************************
typedef enum
{
    eUSBDMSCMediaPresent,
    eUSBDMSCMediaNotPresent,
    eUSBDMSCMediaStopped,
    eUSBDMSCMediaUnknown
}
tUSBDMSCMediaStatus;

//*****************************************************************************
//
// PRIVATE
//
// This structure defines the private instance data and state variables for the
// mass storage class.  The memory for this structure is in the the
// sPrivateData field in the tUSBDMSCDevice structure passed on
// USBDMSCInit() and should not be modified by any code outside of the mass
// storage device code.
//
//*****************************************************************************
typedef struct
{
    //
    // Base address for the USB controller.
    //
    uint32_t ui32USBBase;

    //
    // The device info to interact with the lower level DCD code.
    //
    tDeviceInfo sDevInfo;

    //
    // These three values are used to return the current sense data for an
    // instance of the mass storage class.
    //
    uint8_t ui8ErrorCode;
    uint8_t ui8SenseKey;
    uint16_t ui16AddSenseCode;

    //
    // The pointer to the instance returned from the Open call to the media.
    //
    void *pvMedia;

    //
    // The connection status of the device.
    //
    volatile bool bConnected;

    //
    // Holds the flag settings for this instance.
    //
    uint32_t ui32Flags;

    //
    // Holds the current media status.
    //
    tUSBDMSCMediaStatus iMediaStatus;

    //
    // MSC block buffer.
    //
    uint32_t pui32Buffer[0x1000>>2];

    //
    // Current number of bytes to transfer.
    //
    uint32_t ui32BytesToTransfer;

    //
    // The LBA for the current transfer.
    //
    uint32_t ui32CurrentLBA;

    //
    // The IN endpoint number, this is modified in composite devices.
    //
    uint8_t ui8INEndpoint;

    //
    // The IN DMA channel.
    //
    uint8_t ui8INDMA;

    //
    // The OUT endpoint number, this is modified in composite devices.
    //
    uint8_t ui8OUTEndpoint;

    //
    // The OUT DMA channel.
    //
    uint8_t ui8OUTDMA;

    //
    // The bulk class interface number, this is modified in composite devices.
    //
    uint8_t ui8Interface;

    //
    // Active SCSI state.
    //
    uint8_t ui8SCSIState;

    //
    // A copy of the DMA instance data used with calls to USBLibDMA functions.
    //
    tUSBDMAInstance *psDMAInstance;
}
tMSCInstance;

//*****************************************************************************
//
// This is the size of the g_pui8MSCInterface array in bytes.
//
//*****************************************************************************
#define MSCINTERFACE_SIZE       (23)

//*****************************************************************************
//
//! The size of the memory that should be allocated to create a configuration
//! descriptor for a single instance of the USB Audio Device.
//! This does not include the configuration descriptor which is automatically
//! ignored by the composite device class.
//
//
//*****************************************************************************
#define COMPOSITE_DMSC_SIZE     (MSCINTERFACE_SIZE)

//*****************************************************************************
//
//! The structure used by the application to define operating parameters for
//! the mass storage device.
//
//*****************************************************************************
typedef struct
{
    //
    //! The vendor ID that this device is to present in the device descriptor.
    //
    const uint16_t ui16VID;

    //
    //! The product ID that this device is to present in the device descriptor.
    //
    const uint16_t ui16PID;

    //
    //! 8 byte vendor string.
    //
    const uint8_t pui8Vendor[8];

    //
    //! 16 byte vendor string.
    //
    const uint8_t pui8Product[16];

    //
    //! 4 byte vendor string.
    //
    const uint8_t pui8Version[4];

    //
    //! The maximum power consumption of the device, expressed in milliamps.
    //
    const uint16_t ui16MaxPowermA;

    //
    //! Indicates whether the device is self or bus-powered and whether or not
    //! it supports remote wakeup.  Valid values are \b USB_CONF_ATTR_SELF_PWR
    //! or \b USB_CONF_ATTR_BUS_PWR, optionally ORed with
    //! \b USB_CONF_ATTR_RWAKE.
    //
    const uint8_t ui8PwrAttributes;

    //
    //! A pointer to the string descriptor array for this device.  This array
    //! must contain the following string descriptor pointers in this order.
    //! Language descriptor, Manufacturer name string (language 1), Product
    //! name string (language 1), Serial number string (language 1), MSC
    //! Interface description string (language 1), Configuration description
    //! string (language 1).
    //!
    //! If supporting more than 1 language, the descriptor block (except for
    //! string descriptor 0) must be repeated for each language defined in the
    //! language descriptor.
    //!
    //
    const uint8_t * const *ppui8StringDescriptors;

    //
    //! The number of descriptors provided in the \e ppStringDescriptors
    //! array.  This must be 1 + ((5 + (num HID strings)) * (num languages)).
    //
    const uint32_t ui32NumStringDescriptors;

    //
    //! This structure holds the access functions for the media used by this
    //! instance of the mass storage class device.  All of the functions in
    //! this structure are required to be filled out with valid functions.
    //
    const tMSCDMedia sMediaFunctions;

    //
    //! This is the callback function for various events that occur during
    //! mass storage class operation.
    //
    const tUSBCallback pfnEventCallback;

    //
    //! The private instance data for this device.  This memory
    //! must remain accessible for as long as the MSC device is in use and
    //! must not be modified by any code outside the MSC class driver.
    //
    tMSCInstance sPrivateData;
}
tUSBDMSCDevice;

//*****************************************************************************
//
// MSC-specific device class driver events
//
//*****************************************************************************

//*****************************************************************************
//
//! This event indicates that the host has completed other operations and is
//! no longer accessing the device.
//
//*****************************************************************************
#define USBD_MSC_EVENT_IDLE     (USBD_MSC_EVENT_BASE + 0)

//*****************************************************************************
//
//! This event indicates that the host is reading the storage media.
//
//*****************************************************************************
#define USBD_MSC_EVENT_READING  (USBD_MSC_EVENT_BASE + 1)

//*****************************************************************************
//
//! This event indicates that the host is writing to the storage media.
//
//*****************************************************************************
#define USBD_MSC_EVENT_WRITING  (USBD_MSC_EVENT_BASE + 2)

//*****************************************************************************
//
// API Function Prototypes
//
//*****************************************************************************
extern void *USBDMSCInit(uint32_t ui32Index,
                         tUSBDMSCDevice *psMSCDevice);
extern void *USBDMSCCompositeInit(uint32_t ui32Index,
                                  tUSBDMSCDevice *psMSCDevice,
                                  tCompositeEntry *psCompEntry);
extern void USBDMSCTerm(void *pvInstance);
extern void USBDMSCMediaChange(void *pvInstance,
                               tUSBDMSCMediaStatus eMediaStatus);

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif
