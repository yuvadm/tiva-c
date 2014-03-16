//*****************************************************************************
//
// usbdhidgame.h - The header information for using the USB libraries game pad
// device class.
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
// This is part of revision 2.1.0.12573 of the Tiva USB Library.
//
//*****************************************************************************

#ifndef __USBDHIDGAME_H__
#define __USBDHIDGAME_H__

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
//! \addtogroup hid_gamepad_device_class_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// PRIVATE
//
// This enumeration holds the various states that the game pad can be in during
// normal operation.  This should not be used by applications and is only
// here for memory allocation purposes.
//
//*****************************************************************************
typedef enum
{
    //
    // Not yet configured.
    //
    eHIDGamepadStateNotConnected,

    //
    // Nothing to transmit and not waiting on data to be sent.
    //
    eHIDGamepadStateIdle,

    //
    // Waiting on data to be sent.
    //
    eHIDGamepadStateSending
}
tGamepadState;

//*****************************************************************************
//
// PRIVATE
//
// This is the structure for an instance of a USB game pad device. This should
// not be used by applications and is only here for memory allocation purposes.
//
//*****************************************************************************
typedef struct
{
    //
    // This is needed for the lower level HID driver.
    //
    tUSBDHIDDevice sHIDDevice;

    //
    // The current state of the game pad device.
    //
    tGamepadState iState;

    //
    // The idle timeout control structure for our input report.  This is
    // required by the lower level HID driver.
    //
    tHIDReportIdle sReportIdle;
} tUSBDGamepadInstance;

//*****************************************************************************
//
//! This structure is used by the application to define operating parameters
//! for the HID game device.
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
    //! The maximum power consumption of the device, expressed in milliamps.
    //
    const uint16_t ui16MaxPowermA;

    //
    //! Indicates whether the device is self- or bus-powered and whether or not
    //! it supports remote wake up.  Valid values are \b USB_CONF_ATTR_SELF_PWR
    //! or \b USB_CONF_ATTR_BUS_PWR, optionally ORed with
    //! \b USB_CONF_ATTR_RWAKE.
    //
    const uint8_t ui8PwrAttributes;

    //
    //! A pointer to the callback function that is called to notify
    //! the application of general events.  This pointer must point to a valid
    //! function.
    //
    const tUSBCallback pfnCallback;

    //
    //! A client-supplied pointer that is sent as the first parameter in all
    //! calls made to the pfnCallback gamedevice callback function.
    //
    void *pvCBData;

    //
    //! A pointer to the string descriptor array for this device.  This array
    //! must contain the following string descriptor pointers in this order:
    //! Language descriptor, Manufacturer name string (language 1), Product
    //! name string (language 1), Serial number string (language 1),HID
    //! Interface description string (language 1), Configuration description
    //! string (language 1).
    //!
    //! If supporting more than 1 language, the descriptor block (except for
    //! string descriptor 0) must be repeated for each language defined in the
    //! language descriptor.
    //
    const uint8_t * const *ppui8StringDescriptors;

    //
    //! The number of descriptors provided in the \e ppStringDescriptors
    //! array, which must be (1 + (5 * (number of languages))).
    //
    const uint32_t ui32NumStringDescriptors;

    //
    //! Optional report descriptor if the application wants to use a custom
    //! descriptor.
    //
    const uint8_t *pui8ReportDescriptor;

    //
    //! The size of the optional report descriptor define in
    //! pui8ReportDescriptor.
    //
    const uint32_t ui32ReportSize;

    //
    //! The private instance data for this device.  This memory must
    //! remain accessible for as long as the game device is in use and
    //! must not be modified by any code outside the HID game device driver.
    //
    tUSBDGamepadInstance sPrivateData;
}
tUSBDHIDGamepadDevice;

//*****************************************************************************
//
//! The USBDHIDGamepadSendReport() call successfully scheduled the report.
//
//*****************************************************************************
#define USBDGAMEPAD_SUCCESS     0

//*****************************************************************************
//
//! The USBDHIDGamepadSendReport() function could not send the report at this
//! time.
//
//*****************************************************************************
#define USBDGAMEPAD_TX_ERROR    1

//*****************************************************************************
//
//! The device is not currently configured and cannot perform any operations.
//
//*****************************************************************************
#define USBDGAMEPAD_NOT_CONFIGURED \
                                2

//*****************************************************************************
//
//! This structure is the default packed report structure that is sent to the
//! host.  The application can provide its own structure if the default report
//! descriptor is overridden by the application.  This structure or an
//! application-defined structure is passed to the USBDHIDGamepadSendReport
//! function to send gamepad updates to the host.
//
//*****************************************************************************
typedef struct
{
    //
    //! Signed 8-bit value (-128 to 127).
    //
    int8_t i8XPos;

    //
    //! Signed 8-bit value (-128 to 127).
    //
    int8_t i8YPos;

    //
    //! Signed 8-bit value (-128 to 127).
    //
    int8_t i8ZPos;

    //
    //! 8-bit button mapping with button 1 in the LSB.
    //
    uint8_t ui8Buttons;
}
PACKED tGamepadReport;

//*****************************************************************************
//
// API Function Prototypes
//
//*****************************************************************************
extern tUSBDHIDGamepadDevice *USBDHIDGamepadInit(uint32_t ui32Index,
                                        tUSBDHIDGamepadDevice *psHIDGamepad);
extern tUSBDHIDGamepadDevice *USBDHIDGamepadCompositeInit(uint32_t ui32Index,
                                         tUSBDHIDGamepadDevice *psHIDGamepad,
                                         tCompositeEntry *psCompEntry);
extern void USBDHIDGamepadTerm(tUSBDHIDGamepadDevice *psCompEntry);

extern uint32_t USBDHIDGamepadSendReport(tUSBDHIDGamepadDevice *psHIDGamepad,
                                         void *pvReport, uint32_t ui32Size);

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
