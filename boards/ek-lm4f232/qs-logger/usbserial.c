//*****************************************************************************
//
// usbserial.c - Data logger module to handle serial device functions.
//
// Copyright (c) 2011-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "driverlib/debug.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#include "usb_serial_structs.h"
#include "qs-logger.h"

//*****************************************************************************
//
// This module manages the USB serial device function. It is used when the
// eval board is connected to a host PC as a serial device, and can transmit
// data log records to the host PC through a virtual serial port.
//
//*****************************************************************************

//*****************************************************************************
//
// Global flag indicating that a USB device configuration is made.
//
//*****************************************************************************
static volatile bool g_bUSBDevConnected = false;

//*****************************************************************************
//
// The line coding parameters for the virtual serial port.  Since there is
// no physical port this does not have any real effect, but we have a default
// set of values to report if asked, and will remember whatever the host
// configures.
//
//*****************************************************************************
static tLineCoding g_sLineCoding =
{
    115200, USB_CDC_STOP_BITS_1, USB_CDC_PARITY_NONE, 8
};

//*****************************************************************************
//
// Set the communication parameters for the virtual serial port.
//
//*****************************************************************************
static bool
SetLineCoding(tLineCoding *psLineCoding)
{
    //
    // Copy whatever the host passes into our copy of line parameters.
    //
    memcpy(&g_sLineCoding, psLineCoding, sizeof(tLineCoding));

    //
    // ALways return success
    //
    return(true);
}

//*****************************************************************************
//
// Get the communication parameters in use on the UART.
//
//*****************************************************************************
static void
GetLineCoding(tLineCoding *psLineCoding)
{
    //
    // Copy whatever we have stored as the line parameter to the host.
    //
    memcpy(psLineCoding, &g_sLineCoding, sizeof(tLineCoding));
}

//*****************************************************************************
//
// Handles CDC driver notifications related to control and setup of the device.
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ui32Event identifies the event we are being notified about.
// \param ui32MsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to perform control-related
// operations on behalf of the USB host.  These functions include setting
// and querying the serial communication parameters, setting handshake line
// states and sending break conditions.
//
// \return The return value is event-specific.
//
//*****************************************************************************
uint32_t
ControlHandler(void *pvCBData, uint32_t ui32Event,
               uint32_t ui32MsgValue, void *pvMsgData)
{
    //
    // Which event are we being asked to process?
    //
    switch(ui32Event)
    {
        //
        // We are connected to a host and communication is now possible.
        //
        case USB_EVENT_CONNECTED:
        {
            g_bUSBDevConnected = true;

            //
            // Flush our buffers.
            //
            USBBufferFlush(&g_sTxBuffer);
            USBBufferFlush(&g_sRxBuffer);
            break;
        }

        //
        // The host has disconnected.
        //
        case USB_EVENT_DISCONNECTED:
        {
            g_bUSBDevConnected = false;
            break;
        }

        //
        // Return the current serial communication parameters.
        //
        case USBD_CDC_EVENT_GET_LINE_CODING:
        {
            GetLineCoding(pvMsgData);
            break;
        }

        //
        // Set the current serial communication parameters.
        //
        case USBD_CDC_EVENT_SET_LINE_CODING:
        {
            SetLineCoding(pvMsgData);
            break;
        }

        //
        // The following line control events can be ignored because there is
        // no physical serial port to manage.
        //
        case USBD_CDC_EVENT_SET_CONTROL_LINE_STATE:
        case USBD_CDC_EVENT_SEND_BREAK:
        case USBD_CDC_EVENT_CLEAR_BREAK:
        {
            break;
        }

        //
        // Ignore SUSPEND and RESUME for now.
        //
        case USB_EVENT_SUSPEND:
        case USB_EVENT_RESUME:
        {
            break;
        }

        //
        // An unknown event occurred.
        //
        default:
        {
            break;
        }
    }

    //
    // Return control to USB stack
    //
    return(0);
}

//*****************************************************************************
//
// Handles CDC driver notifications related to the transmit channel (data to
// the USB host).
//
// \param ui32CBData is the client-supplied callback pointer for this channel.
// \param ui32Event identifies the event we are being notified about.
// \param ui32MsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the transmit data channel (the IN channel carrying
// data to the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
uint32_t
TxHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue,
          void *pvMsgData)
{
    //
    // Which event have we been sent?
    //
    switch(ui32Event)
    {
        case USB_EVENT_TX_COMPLETE:
        {
            //
            // Since we are using the USBBuffer, we don't need to do anything
            // here.
            //
            break;
        }

        //
        // We don't expect to receive any other events.
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
// Handles CDC driver notifications related to the receive channel (data from
// the USB host).
//
// \param ui32CBData is the client-supplied callback data value for this
// channel.
// \param ui32Event identifies the event we are being notified about.
// \param ui32MsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the receive data channel (the OUT channel carrying
// data from the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
uint32_t
RxHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue,
          void *pvMsgData)
{
    //
    // Which event are we being sent?
    //
    switch(ui32Event)
    {
        //
        // A new packet has been received.
        //
        case USB_EVENT_RX_AVAILABLE:
        {
            //
            // We do not ever expect to receive serial data, so just flush
            // the RX buffer if any data actually comes in.
            //
            USBBufferFlush(&g_sRxBuffer);
            break;
        }

        //
        // We are being asked how much unprocessed data we have still to
        // process.  Since there is no actual serial port and we are not
        // processing any RX data, just return 0.
        //
        case USB_EVENT_DATA_REMAINING:
        {
            return(0);
        }

        //
        // We are being asked to provide a buffer into which the next packet
        // can be read. We do not support this mode of receiving data so let
        // the driver know by returning 0. The CDC driver should not be sending
        // this message but this is included just for illustration and
        // completeness.
        //
        case USB_EVENT_REQUEST_BUFFER:
        {
            return(0);
        }

        //
        // We don't expect to receive any other events.
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
// Initializes the USB serial device.
//
//*****************************************************************************
void
USBSerialInit(void)
{
    //
    // Initialize the transmit and receive buffers.
    //
    USBBufferInit((tUSBBuffer *)&g_sTxBuffer);
    USBBufferInit((tUSBBuffer *)&g_sRxBuffer);

    //
    // Initialize the USB library CDC device function.
    //
    USBDCDCInit(0, (tUSBDCDCDevice *)&g_sCDCDevice);
}

//*****************************************************************************
//
// This is called by the application main loop to perform regular processing.
// This is just a stub here because everything is event or interrupt driven.
//
//*****************************************************************************
void
USBSerialRun(void)
{
}

//*****************************************************************************
//
// Write a data record to the serial port.  An acquired data record is passed
// in and is composed into a binary packet and sent on the serial port.  The
// host PC, if connected will receive this packet via the virtual serial port,
// and can decode and display the data.
//
// The binary packet has the following format:
// - 16-bit header, value 0x5351
// - 32-bit seconds time stamp
// - 16-bit fractional seconds time stamp (1/32768 resolution)
// - 16-bit data item selection mask (which items are included in the record)
// - multiple 16-bit data item values, per selection mask
// - 16-bit checksum which when added to the 16-bit sum of the entire packet
// will result in 0.
//
// The entire packet is transmitted bytewise over the virtual serial port,
// little-endian format.
//
//*****************************************************************************
int32_t
USBSerialWriteRecord(tLogRecord *psRecord)
{
    uint32_t ui32Idx;
    uint16_t ui16Checksum;
    uint32_t ui32ItemCount;
    uint16_t *pui16Buf;

    //
    // Check the arguments
    //
    ASSERT(psRecord);
    if(!psRecord)
    {
        return(1);
    }

    //
    // Check state for ready device
    //
    if(!g_bUSBDevConnected)
    {
        return(1);
    }

    //
    // Determine how many channels are to be logged
    //
    ui32Idx = psRecord->ui16ItemMask;
    ui32ItemCount = 0;
    while(ui32Idx)
    {
        if(ui32Idx & 1)
        {
            ui32ItemCount++;
        }
        ui32Idx >>= 1;
    }

    //
    // Add to item count the equivalent number of 16-bit words for timestamp
    // and selection mask.
    //
    ui32ItemCount += 4;

    //
    // Put the header word in the USB buffer
    //
    ui16Checksum = 0x5351;
    USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, (uint8_t *)&ui16Checksum, 2);

    //
    // Compute the checksum over the entire record
    //
    pui16Buf = (uint16_t *)psRecord;
    for(ui32Idx = 0; ui32Idx < ui32ItemCount; ui32Idx++)
    {
        ui16Checksum += pui16Buf[ui32Idx];
    }

    //
    // Convert item count to bytes.  This now represents the entire record
    // size in bytes, not including the checksum.  The header has already
    // been sent
    //
    ui32ItemCount *= 2;

    //
    // Transmit the record, which includes the time stamp, selection mask
    // and all selected data item, to the USB buffer
    //
    USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, (uint8_t *)pui16Buf,
                   ui32ItemCount);

    //
    // Adjust the checksum so that when added (as uint16_t) to the sum of the
    // rest of the packet, the result will be 0
    //
    ui16Checksum = (uint16_t)(0x10000L - (uint32_t)ui16Checksum);

    //
    // Transmit the checksum which is the end of the packet
    //
    USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, (uint8_t *)&ui16Checksum, 2);

    //
    // Return success to the caller
    //
    return(0);
}
