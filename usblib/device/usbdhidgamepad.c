//*****************************************************************************
//
// usbdhidgame.c - USB HID Gamepad device class driver
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

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/usb.h"
#include "usblib/usblib.h"
#include "usblib/usblibpriv.h"
#include "usblib/device/usbdevice.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidgamepad.h"

//*****************************************************************************
//
//! \addtogroup hid_gamepad_device_class_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// HID device configuration descriptor.
//
// It is vital that the configuration descriptor bConfigurationValue field
// (byte 6) is 1 for the first configuration and increments by 1 for each
// additional configuration defined here.  This relationship is assumed in the
// device stack for simplicity even though the USB 2.0 specification imposes
// no such restriction on the bConfigurationValue values.
//
//*****************************************************************************
static uint8_t g_pui8GameDescriptor[] =
{
    //
    // Configuration descriptor header.
    //
    9,                          // Size of the configuration descriptor.
    USB_DTYPE_CONFIGURATION,    // Type of this descriptor.
    USBShort(24),               // The total size of this full structure.
    1,                          // The number of interfaces in this
                                // configuration.
    1,                          // The unique value for this configuration.
    5,                          // The string identifier that describes this
                                // configuration.
    USB_CONF_ATTR_SELF_PWR,     // Self Powered.
    0,                          // The maximum power in 2mA increments.
};

//*****************************************************************************
//
// This is the HID interface descriptor for the gamepad device.
//
//*****************************************************************************
static uint8_t g_pui8HIDInterface[HIDINTERFACE_SIZE] =
{
    //
    // HID Device Class Interface Descriptor.
    //
    9,                          // Size of the interface descriptor.
    USB_DTYPE_INTERFACE,        // Type of this descriptor.
    0,                          // The index for this interface.
    0,                          // The alternate setting for this interface.
    1,                          // The number of endpoints used by this
                                // interface.
    USB_CLASS_HID,              // The interface class
    0,                          // The interface sub-class.
    0,                          // The interface protocol for the sub-class
                                // specified above.
    4,                          // The string index for this interface.
};

//*****************************************************************************
//
// This is the HID IN endpoint descriptor for the gamepad device.
//
//*****************************************************************************
static const uint8_t g_pui8HIDInEndpoint[HIDINENDPOINT_SIZE] =
{
    //
    // Interrupt IN endpoint descriptor
    //
    7,                          // The size of the endpoint descriptor.
    USB_DTYPE_ENDPOINT,         // Descriptor type is an endpoint.
    USB_EP_DESC_IN | USBEPToIndex(USB_EP_1),
    USB_EP_ATTR_INT,            // Endpoint is an interrupt endpoint.
    USBShort(USBFIFOSizeToBytes(USB_FIFO_SZ_64)),
                                // The maximum packet size.
    1,                          // The polling interval for this endpoint.
};

//*****************************************************************************
//
// The following is the HID report structure definition that is passed back
// to the host.
//
//*****************************************************************************
static const uint8_t g_pui8GameReportDescriptor[] =
{
    UsagePage(USB_HID_GENERIC_DESKTOP),
    Usage(USB_HID_JOYSTICK),
    Collection(USB_HID_APPLICATION),
        //
        // The axis for the controller.
        //
        UsagePage(USB_HID_GENERIC_DESKTOP),
        Usage (USB_HID_POINTER),
        Collection (USB_HID_PHYSICAL),

            //
            // The X, Y and Z values which are specified as 8-bit absolute
            // position values.
            //
            Usage (USB_HID_X),
            Usage (USB_HID_Y),
            Usage (USB_HID_Z),

            //
            // 3 8-bit absolute values.
            //
            ReportSize(8),
            ReportCount(3),
            Input(USB_HID_INPUT_DATA | USB_HID_INPUT_VARIABLE |
                  USB_HID_INPUT_ABS),

            //
            // The 8 buttons.
            //
            UsagePage(USB_HID_BUTTONS),
            UsageMinimum(1),
            UsageMaximum(8),
            LogicalMinimum(0),
            LogicalMaximum(1),
            PhysicalMinimum(0),
            PhysicalMaximum(1),

            //
            // 8 - 1 bit values for the buttons.
            //
            ReportSize(1),
            ReportCount(8),
            Input(USB_HID_INPUT_DATA | USB_HID_INPUT_VARIABLE |
                  USB_HID_INPUT_ABS),

        EndCollection,
    EndCollection
};

//*****************************************************************************
//
// The HID descriptor for the gamepad device.
//
//*****************************************************************************
static tHIDDescriptor g_sGameHIDDescriptor =
{
    9,                              // bLength
    USB_HID_DTYPE_HID,              // bDescriptorType
    0x111,                          // bcdHID (version 1.11 compliant)
    0,                              // bCountryCode (not localized)
    1,                              // bNumDescriptors
    {
        {
            USB_HID_DTYPE_REPORT,   // Report descriptor
            sizeof(g_pui8GameReportDescriptor)
                                    // Size of report descriptor
        }
    }
};

//*****************************************************************************
//
// The HID configuration descriptor is defined as four sections.
// These sections are:
//
// 1.  The 9 byte configuration descriptor.
// 2.  The interface descriptor.
// 3.  The HID report and physical descriptors, provided by the application
//     or the default can be used.
// 4.  The mandatory interrupt IN endpoint descriptor.
//
//*****************************************************************************
static const tConfigSection g_sHIDConfigSection =
{
    sizeof(g_pui8GameDescriptor),
    g_pui8GameDescriptor
};

static const tConfigSection g_sHIDInterfaceSection =
{
    sizeof(g_pui8HIDInterface),
    g_pui8HIDInterface
};

static const tConfigSection g_sHIDInEndpointSection =
{
    sizeof(g_pui8HIDInEndpoint),
    g_pui8HIDInEndpoint
};

//*****************************************************************************
//
// Place holder for the user's HID descriptor block.
//
//*****************************************************************************
static tConfigSection g_sHIDDescriptorSection =
{
   sizeof(g_sGameHIDDescriptor),
   (const uint8_t *)&g_sGameHIDDescriptor
};

//*****************************************************************************
//
// This array lists all the sections that must be concatenated to make a
// single, complete HID configuration descriptor.
//
//*****************************************************************************
static const tConfigSection *g_psHIDSections[] =
{
    &g_sHIDConfigSection,
    &g_sHIDInterfaceSection,
    &g_sHIDDescriptorSection,
    &g_sHIDInEndpointSection,
};

#define NUM_HID_SECTIONS        ((sizeof(g_psHIDSections) /                   \
                                  sizeof(tConfigSection *)))

//*****************************************************************************
//
// The header for the single configuration supported.  This is the root of
// the data structure that defines all the bits and pieces that are pulled
// together to generate the configuration descriptor.  Note that this must be
// in RAM since we need to include or exclude the final section based on
// client supplied initialization parameters.
//
//*****************************************************************************
static tConfigHeader g_sHIDConfigHeader =
{
    NUM_HID_SECTIONS,
    g_psHIDSections
};

//*****************************************************************************
//
// Configuration Descriptor.
//
//*****************************************************************************
static const tConfigHeader * const g_ppsHIDConfigDescriptors[] =
{
    &g_sHIDConfigHeader
};

//*****************************************************************************
//
// The HID class descriptor table.  For the gamepad class there is only a
// single report descriptor.
//
//*****************************************************************************
static const uint8_t *g_ppui8GameClassDescriptors[] =
{
    g_pui8GameReportDescriptor
};

//*****************************************************************************
//
// HID gamepad transmit channel event handler function.
//
// \param pvGameDevice is the event callback pointer provided during
// USBDHIDInit().  This is a pointer to the HID gamepad device structure
// of the type tUSBDHIDGamepadDevice.
// \param ui32Event identifies the event we are being called back for.
// \param ui32MsgData is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the lower level HID device class driver to inform
// the application of particular asynchronous events related to report events
// related to using the interrupt IN endpoint.
//
// \return Returns a value which is event-specific.
//
//*****************************************************************************
static uint32_t
HIDGamepadTxHandler(void *pvGameDevice, uint32_t ui32Event,
                    uint32_t ui32MsgData, void *pvMsgData)
{
    tUSBDGamepadInstance *psInst;
    tUSBDHIDGamepadDevice *psGamepad;

    //
    // Make sure we did not get a NULL pointer.
    //
    ASSERT(pvGameDevice);

    //
    // Get a pointer to our instance data
    //
    psGamepad = (tUSBDHIDGamepadDevice *)pvGameDevice;
    psInst = &psGamepad->sPrivateData;

    //
    // Which event were we sent?
    //
    switch (ui32Event)
    {
        //
        // A report transmitted via the interrupt IN endpoint was acknowledged
        // by the host.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            //
            // The last transmission is complete so return to the idle state.
            //
            psInst->iState = eHIDGamepadStateIdle;

            //
            // Pass the event on to the application.
            //
            psGamepad->pfnCallback(psGamepad->pvCBData, USB_EVENT_TX_COMPLETE,
                                   ui32MsgData, (void *)0);

            break;
        }

        //
        // Ignore all other events related to transmission of reports via
        // the interrupt IN endpoint.
        //
        default:
        {
            break;
        }
    }

    return(0);
}

//*****************************************************************************
//
// Main HID device class event receive handler function.
//
// \param pvGameDevice is the event callback pointer provided during
// USBDHIDInit().  This is a pointer to the HID gamepad device structure
// of the type tUSBDHIDGamepadDevice.
// \param ui32Event identifies the event we are being called back for.
// \param ui32MsgData is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the lower level HID device class driver to inform
// the application of particular asynchronous events related to operation of
// the gamepad HID device.
//
// \note This function also receive all generic events as well such as
// \b USB_EVENT_CONNECTED and USB_EVENT_DISCONNECTED.
//
// \return Returns a value which is event-specific.
//
//*****************************************************************************
static uint32_t
HIDGamepadRxHandler(void *pvGamepad, uint32_t ui32Event, uint32_t ui32MsgData,
                    void *pvMsgData)
{
    tUSBDGamepadInstance *psInst;
    tUSBDHIDGamepadDevice *psGamepad;
    uint32_t ui32Ret;

    //
    // Make sure we did not get a NULL pointer.
    //
    ASSERT(pvGamepad);

    //
    // Return zero by default.
    //
    ui32Ret = 0;

    //
    // Get a pointer to our instance data
    //
    psGamepad = (tUSBDHIDGamepadDevice *)pvGamepad;
    psInst = &psGamepad->sPrivateData;

    //
    // Which event were we sent?
    //
    switch(ui32Event)
    {
        //
        // The host has connected to us and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            //
            // Now in the idle state.
            //
            psInst->iState = eHIDGamepadStateIdle;

            //
            // Pass the information on to the application.
            //
            psGamepad->pfnCallback(psGamepad->pvCBData, USB_EVENT_CONNECTED, 0,
                                   (void *)0);

            break;
        }

        //
        // The host has disconnected from us.
        //
        case USB_EVENT_DISCONNECTED:
        {
            psInst->iState = eHIDGamepadStateNotConnected;

            //
            // Pass the information on to the application.
            //
            ui32Ret = psGamepad->pfnCallback(psGamepad->pvCBData,
                                             USB_EVENT_DISCONNECTED, 0,
                                             (void *)0);

            break;
        }

        //
        // This handles the Set Idle command.
        //
        case USBD_HID_EVENT_IDLE_TIMEOUT:
        {
            //
            // Give the pointer to the idle report structure.
            //
            *(void **)pvMsgData = (void *)&psInst->sReportIdle;

            ui32Ret = sizeof(psInst->sReportIdle);

            break;
        }

        //
        // The host is polling for a particular report and the HID driver
        // is asking for the latest version to transmit.
        //
        case USBD_HID_EVENT_GET_REPORT:
        {
            //
            // If this is an IN request then pass the request on to the
            // application.  All other requests are ignored.
            //
            if(ui32MsgData == USB_HID_REPORT_IN)
            {
                ui32Ret = psGamepad->pfnCallback(psGamepad->pvCBData,
                                                 USBD_HID_EVENT_GET_REPORT, 0,
                                                 pvMsgData);
            }

            break;
        }

        //
        // The device class driver has completed sending a report to the
        // host in response to a Get_Report request.
        //
        case USBD_HID_EVENT_REPORT_SENT:
        {
            //
            // We have nothing to do here.
            //
            break;
        }

        //
        // Pass these events to the client unchanged.
        //
        case USB_EVENT_ERROR:
        case USB_EVENT_SUSPEND:
        case USB_EVENT_RESUME:
        case USB_EVENT_LPM_RESUME:
        case USB_EVENT_LPM_SLEEP:
        case USB_EVENT_LPM_ERROR:
        {
            ui32Ret = psGamepad->pfnCallback(psGamepad->pvCBData, ui32Event,
                                             ui32MsgData, pvMsgData);

            break;
        }

        //
        // This event is sent in response to a host Set_Report request which
        // is not supported for gamepads.
        //
        case USBD_HID_EVENT_GET_REPORT_BUFFER:

        //
        // We ignore all other events.
        //
        default:
        {
            break;
        }
    }
    return(ui32Ret);
}

//*****************************************************************************
//
//! Initializes HID gamepad device operation for a given USB controller.
//!
//! \param ui32Index is the index of the USB controller that is to be
//! initialized for HID gamepad device operation.
//! \param psGamepad points to a structure containing parameters
//! customizing the operation of the HID gamepad device.
//!
//! An application that enables a USB HID gamepad interface to a USB host
//! must call this function to initialize the USB controller and attach the
//! gamepad device to the USB bus.  This function performs all required USB
//! initialization, and the device is ready for operation on the function
//! return.
//!
//! On successful completion, this function returns the modified \e psGamepad
//! pointer passed to it or returns a NULL pointer if there was a problem.
//! This pointer must be passed on all future calls to the HID gamepad device
//! driver.
//!
//! When a host connects and configures the device, the application callback
//! receives \b USB_EVENT_CONNECTED, after which calls can be made to
//! USBDHIDGamepadSendReport() to report changes to the gamepad interface to
//! the USB host when it requests them.
//!
//! \note The application must not make any calls to the lower level USB device
//! interfaces if interacting with USB via the USB HID gamepad device class
//! API.
//!
//! \return Returns NULL on failure or the \e psGamepad pointer on success.
//
//*****************************************************************************
tUSBDHIDGamepadDevice *
USBDHIDGamepadInit(uint32_t ui32Index, tUSBDHIDGamepadDevice *psGamepad)
{
    void *pvRetcode;
    tUSBDHIDDevice *psHIDDevice;
    tConfigDescriptor *pConfigDesc;

    //
    // Check basic parameter validity.
    //
    ASSERT(psGamepad);
    ASSERT(psGamepad->ppui8StringDescriptors);
    ASSERT(psGamepad->pfnCallback);

    //
    // Get a pointer to the HID device data.
    //
    psHIDDevice = &psGamepad->sPrivateData.sHIDDevice;

    //
    // Call the common initialization routine.
    //
    pvRetcode = USBDHIDGamepadCompositeInit(ui32Index, psGamepad, 0);

    pConfigDesc = (tConfigDescriptor *)g_pui8GameDescriptor;
    pConfigDesc->bmAttributes = psGamepad->ui8PwrAttributes;
    pConfigDesc->bMaxPower =  (uint8_t)(psGamepad->ui16MaxPowermA / 2);

    //
    // If we initialized the HID layer successfully, pass our device pointer
    // back as the return code, otherwise return NULL to indicate an error.
    //
    if(pvRetcode)
    {
        //
        // Initialize the lower layer HID driver and pass it the various
        // structures and descriptors necessary to declare that we are a
        // gamepad.
        //
        pvRetcode = USBDHIDInit(ui32Index, psHIDDevice);

        return(psGamepad);
    }
    else
    {
        return((tUSBDHIDGamepadDevice *)0);
    }
}

//*****************************************************************************
//
//! Initializes HID gamepad device operation for a given USB controller.
//!
//! \param ui32Index is the index of the USB controller that is to be
//! initialized for HID gamepad device operation.
//! \param psGamepad points to a structure containing parameters
//! customizing the operation of the HID gamepad device.
//! \param psCompEntry is the composite device entry to initialize when
//! creating a composite device.
//!
//! This call is very similar to USBDHIDGamepadInit() except that it is used
//! for initializing an instance of the HID gamepad device for use in a
//! composite device.  If this HID gamepad is part of a composite device, then
//! the \e psCompEntry should point to the composite device entry to
//! initialize.  This entry is part of the array that is passed to the
//! USBDCompositeInit() function to start up and complete configuration of a
//! composite USB device.
//!
//! \return Returns NULL on failure or the \e psGamepad value that should be
//! used with the remaining USB HID gamepad APIs.
//
//*****************************************************************************
tUSBDHIDGamepadDevice *
USBDHIDGamepadCompositeInit(uint32_t ui32Index,
                            tUSBDHIDGamepadDevice *psGamepad,
                            tCompositeEntry *psCompEntry)
{
    tUSBDGamepadInstance *psInst;
    tUSBDHIDDevice *psHIDDevice;

    //
    // Check parameter validity.
    //
    ASSERT(psGamepad);
    ASSERT(psGamepad->ppui8StringDescriptors);
    ASSERT(psGamepad->pfnCallback);

    //
    // Get a pointer to our instance data
    //
    psInst = &psGamepad->sPrivateData;

    //
    // Initialize the various fields in our instance structure.
    //
    psInst->iState = eHIDGamepadStateNotConnected;

    //
    // Get a pointer to the HID device data.
    //
    psHIDDevice = &psInst->sHIDDevice;

    //
    // Initialize the HID device class instance structure based on input from
    // the caller.
    //
    psHIDDevice->ui16PID = psGamepad->ui16PID;
    psHIDDevice->ui16VID = psGamepad->ui16VID;
    psHIDDevice->ui16MaxPowermA = psGamepad->ui16MaxPowermA;
    psHIDDevice->ui8PwrAttributes = psGamepad->ui8PwrAttributes;
    psHIDDevice->ui8Subclass = 0;
    psHIDDevice->ui8Protocol = 0;
    psHIDDevice->ui8NumInputReports = 1;
    psHIDDevice->psReportIdle = &psInst->sReportIdle;
    psInst->sReportIdle.ui8Duration4mS = 125;
    psInst->sReportIdle.ui8ReportID = 0;
    psInst->sReportIdle.ui32TimeSinceReportmS = 0;
    psInst->sReportIdle.ui16TimeTillNextmS = 0;
    psHIDDevice->pfnTxCallback = HIDGamepadTxHandler;
    psHIDDevice->pvRxCBData = (void *)psGamepad;
    psHIDDevice->pfnRxCallback = HIDGamepadRxHandler;
    psHIDDevice->pvTxCBData = (void *)psGamepad;
    psHIDDevice->bUseOutEndpoint = false,
    psHIDDevice->psHIDDescriptor = &g_sGameHIDDescriptor;
    psHIDDevice->ppui8ClassDescriptors = g_ppui8GameClassDescriptors;
    psHIDDevice->ppui8StringDescriptors = psGamepad->ppui8StringDescriptors;
    psHIDDevice->ui32NumStringDescriptors =
                                          psGamepad->ui32NumStringDescriptors;
    psHIDDevice->ppsConfigDescriptor = g_ppsHIDConfigDescriptors;

    //
    // If there was an override for the report descriptor then use it.
    //
    if(psGamepad->pui8ReportDescriptor)
    {
        //
        // Save the report descriptor in the list of report descriptors.
        //
        g_ppui8GameClassDescriptors[0] = psGamepad->pui8ReportDescriptor;

        //
        // Override the report descriptor size.
        //
        g_sGameHIDDescriptor.sClassDescriptor[0].wDescriptorLength =
                                                    psGamepad->ui32ReportSize;
    }

    //
    // Initialize the lower layer HID driver and pass it the various structures
    // and descriptors necessary to declare that we are a gamepad.
    //
    return(USBDHIDCompositeInit(ui32Index, psHIDDevice, psCompEntry));
}

//*****************************************************************************
//
//! Schedules a report to be sent once the host requests more data.
//!
//! \param psHIDGamepad is the structure pointer that is returned from the
//! USBDHIDGamepadCompositeInit() or USBDHIDGamepadInit() functions.
//! \param pvReport is the data to send to the host.
//! \param ui32Size is the number of bytes in the \e pvReport buffer.
//!
//! This call is made by an application to schedule data to be sent to the
//! host when the host requests an update from the device.  The application
//! must then wait for a \b USB_EVENT_TX_COMPLETE event in the function
//! provided in the \e pfnCallback pointer in the tUSBDHIDGamepadDevice
//! structure before being able to send more data with this function.  The
//! pointer passed in the \e pvReport can be updated once this call returns as
//! the data has been copied from the buffer.  The function returns
//! \b USBDGAMEPAD_SUCCESS if the transmission was successfully scheduled or
//! \b USBDGAMEPAD_TX_ERROR if the report could not be sent at this time.
//! If the call is made before the device is connected or ready to communicate
//! with the host, then the function can return \b USBDGAMEPAD_NOT_CONFIGURED.
//!
//! \return The function returns one of the \b USBDGAMEPAD_* values.
//
//*****************************************************************************
uint32_t
USBDHIDGamepadSendReport(tUSBDHIDGamepadDevice *psHIDGamepad, void *pvReport,
                         uint32_t ui32Size)
{
    uint32_t ui32Retcode, ui32Count;
    tUSBDGamepadInstance *psInst;
    tUSBDHIDDevice *psHIDDevice;

    //
    // Get a pointer to the HID device data.
    //
    psHIDDevice = &psHIDGamepad->sPrivateData.sHIDDevice;

    //
    // Get a pointer to our instance data
    //
    psInst = &psHIDGamepad->sPrivateData;

    //
    // If we are not configured, return an error here before trying to send
    // anything.
    //
    if(psInst->iState == eHIDGamepadStateNotConnected)
    {
        return(USBDGAMEPAD_NOT_CONFIGURED);
    }

    //
    // Only send a report if the transmitter is currently free.
    //
    if(USBDHIDTxPacketAvailable((void *)psHIDDevice))
    {
        //
        // Send the report to the host.
        //
        psInst->iState = eHIDGamepadStateSending;
        ui32Count = USBDHIDReportWrite((void *)psHIDDevice, pvReport, ui32Size,
                                       true);

        //
        // Did we schedule a packet for transmission correctly?
        //
        if(ui32Count == 0)
        {
            //
            // No - report the error to the caller.
            //
            ui32Retcode = USBDGAMEPAD_TX_ERROR;
        }
        else
        {
            ui32Retcode = USBDGAMEPAD_SUCCESS;
        }
    }
    else
    {
        ui32Retcode = USBDGAMEPAD_TX_ERROR;
    }

    //
    // Return the relevant error code to the caller.
    //
    return(ui32Retcode);
}

//*****************************************************************************
//
//! Shuts down the HID gamepad device.
//!
//! \param psGamepad is the pointer to the device instance structure
//! as returned by USBDHIDGamepadInit() or USBDHIDGamepadCompositeInit().
//!
//! This function terminates HID gamepad operation for the instance supplied
//! and removes the device from the USB bus.  Following this call, the
//! \e psGamepad instance may not me used in any other call to the HID
//! gamepad device other than to reinitialize by calling USBDHIDGamepadInit()
//! or USBDHIDGamepadCompositeInit().
//!
//! \return None.
//
//*****************************************************************************
void
USBDHIDGamepadTerm(tUSBDHIDGamepadDevice *psGamepad)
{
    tUSBDHIDDevice *psHIDDevice;

    ASSERT(psGamepad);

    //
    // Get a pointer to the HID device data.
    //
    psHIDDevice = &psGamepad->sPrivateData.sHIDDevice;

    //
    // Mark the device as no longer connected.
    //
    psGamepad->sPrivateData.iState = eHIDGamepadStateNotConnected;

    //
    // Terminate the low level HID driver.
    //
    USBDHIDTerm(psHIDDevice);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
