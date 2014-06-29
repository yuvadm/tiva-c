//*****************************************************************************
//
// usb_dev_serial.c - Main routines for the USB CDC serial example.
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

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/usb.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#include "utils/ustdlib.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "usb_serial_structs.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Serial Device (usb_dev_serial)</h1>
//!
//! This example application turns the development kit into a virtual serial
//! port when connected to the USB host system.  The application supports the
//! USB Communication Device Class, Abstract Control Model to redirect UART0
//! traffic to and from the USB host system.
//!
//! The application can be recompiled to run using and external USB phy to
//! implement a high speed device using an external USB phy.  To use the
//! external phy the application must be built with \b USE_ULPI defined.  This
//! disables the internal phy and the connector on the DK-TM4C129X board and
//! enables the connections to the external ULPI phy pins on the DK-TM4C129X
//! board.
//!
//! Assuming you installed TivaWare in the default directory, a
//! driver information (INF) file for use with Windows XP, Windows Vista and
//! Windows7 can be found in C:/ti/TivaWare-for-C-Series/windows_drivers.
//! For Windows 2000, the required INF file is in
//! C:/ti/TivaWare-for-C-Series/windows_drivers/win2K.
//
//*****************************************************************************

//*****************************************************************************
//
// Note:
//
// This example is intended to run on Tiva C Series evaluation kit hardware
// where the UARTs are wired solely for TX and RX, and do not have GPIOs
// connected to act as handshake signals.  As a result, this example mimics
// the case where communication is always possible.  It reports DSR, DCD
// and CTS as high to ensure that the USB host recognizes that data can be
// sent and merely ignores the host's requested DTR and RTS states.  "TODO"
// comments in the code indicate where code would be required to add support
// for real handshakes.
//
//*****************************************************************************


//*****************************************************************************
//
// Configuration and tuning parameters.
//
//*****************************************************************************

//*****************************************************************************
//
// The system tick rate expressed both as ticks per second and a millisecond
// period.
//
//*****************************************************************************
#define TICKS_PER_SECOND        100

//*****************************************************************************
//
// The UART peripheral clock rate.
//
//*****************************************************************************
#define UART_CLOCK              16000000

//*****************************************************************************
//
// Variables tracking transmit and receive counts.
//
//*****************************************************************************
static volatile uint32_t g_ui32UARTTxCount;
static volatile uint32_t g_ui32UARTRxCount;
#ifdef DEBUG
uint32_t g_ui32UARTRxErrors;
#endif

//*****************************************************************************
//
// The base address, peripheral ID and interrupt ID of the UART that is to
// be redirected.
//
//*****************************************************************************

//*****************************************************************************
//
// Default line coding settings for the redirected UART.
//
//*****************************************************************************
#define DEFAULT_BIT_RATE        115200
#define DEFAULT_UART_CONFIG     (UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | \
                                 UART_CONFIG_STOP_ONE)

//*****************************************************************************
//
// Global system tick counter
//
//*****************************************************************************
static volatile uint32_t g_ui32SysTickCount = 0;

//*****************************************************************************
//
// Graphics context used to show text on the color STN display.
//
//*****************************************************************************
static tContext g_sContext;
#define TEXT_FONT               g_psFontCmss22b
#define TEXT_HEIGHT             (GrFontHeightGet(TEXT_FONT))
#define BUFFER_METER_HEIGHT     TEXT_HEIGHT
#define BUFFER_METER_WIDTH      150

//*****************************************************************************
//
// Flags used to pass commands from interrupt context to the main loop.
//
//*****************************************************************************
#define FLAG_STATUS_UPDATE      0
#define FLAG_USB_CONFIGURED     1
#define FLAG_SENDING_BREAK      2
static volatile uint32_t g_ui32Flags;

//*****************************************************************************
//
// Global pointer to the current status string.
//
//*****************************************************************************
static char *g_pcStatus;

//*****************************************************************************
//
// Internal function prototypes.
//
//*****************************************************************************
static void USBUARTPrimeTransmit(uint32_t ui32Base);
static void CheckForSerialStateChange(const tUSBDCDCDevice *psDevice,
                                      uint32_t ui32Errors);
static void SetControlLineState(uint16_t ui16State);
static bool SetLineCoding(tLineCoding *psLineCoding, uint32_t ui32UARTClock);
static void GetLineCoding(tLineCoding *psLineCoding, uint32_t ui32UARTClock);
static void SendBreak(bool bSend);

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
    while(1)
    {
    }
}
#endif

//*****************************************************************************
//
// This function is called whenever serial data is received from the UART.
// It is passed the accumulated error flags from each character received in
// this interrupt and determines from them whether or not an interrupt
// notification to the host is required.
//
// If a notification is required and the control interrupt endpoint is idle,
// we send the notification immediately.  If the endpoint is not idle, we
// accumulate the errors in a global variable which will be checked on
// completion of the previous notification and used to send a second one
// if necessary.
//
//*****************************************************************************
static void
CheckForSerialStateChange(const tUSBDCDCDevice *psDevice, uint32_t ui32Errors)
{
    uint16_t ui16SerialState;

    //
    // Clear our USB serial state.  Since we are faking the handshakes, always
    // set the TXCARRIER (DSR) and RXCARRIER (DCD) bits.
    //
    ui16SerialState = USB_CDC_SERIAL_STATE_TXCARRIER |
                      USB_CDC_SERIAL_STATE_RXCARRIER;

    //
    // Are any error bits set?
    //
    if(ui32Errors)
    {
        //
        // At least one error is being notified so translate from our hardware
        // error bits into the correct state markers for the USB notification.
        //
        if(ui32Errors & UART_DR_OE)
        {
            ui16SerialState |= USB_CDC_SERIAL_STATE_OVERRUN;
        }

        if(ui32Errors & UART_DR_PE)
        {
            ui16SerialState |= USB_CDC_SERIAL_STATE_PARITY;
        }

        if(ui32Errors & UART_DR_FE)
        {
            ui16SerialState |= USB_CDC_SERIAL_STATE_FRAMING;
        }

        if(ui32Errors & UART_DR_BE)
        {
            ui16SerialState |= USB_CDC_SERIAL_STATE_BREAK;
        }

        //
        // Call the CDC driver to notify the state change.
        //
        USBDCDCSerialStateChange((void *)psDevice, ui16SerialState);
    }
}

//*****************************************************************************
//
// Read as many characters from the UART FIFO as we can and move them into
// the CDC transmit buffer.
//
// \return Returns UART error flags read during data reception.
//
//*****************************************************************************
static uint32_t
ReadUARTData(void)
{
    int32_t i32Char;
    int8_t ui8Char;
    uint32_t ui32Space, ui32Errors;

    //
    // Clear our error indicator.
    //
    ui32Errors = 0;

    //
    // How much space do we have in the buffer?
    //
    ui32Space = USBBufferSpaceAvailable((tUSBBuffer *)&g_sTxBuffer);

    //
    // Read data from the UART FIFO until there is none left or we run
    // out of space in our receive buffer.
    //
    while(ui32Space && UARTCharsAvail(UART0_BASE))
    {
        //
        // Read a character from the UART FIFO into the ring buffer if no
        // errors are reported.
        //
        i32Char = UARTCharGetNonBlocking(UART0_BASE);

        //
        // If the character did not contain any error notifications,
        // copy it to the output buffer.
        //
        if(!(i32Char & ~0xFF))
        {
            ui8Char = (unsigned char)(i32Char & 0xFF);
            USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,
                           (unsigned char *)&ui8Char, 1);

            //
            // Decrement the number of bytes we know the buffer can accept.
            //
            ui32Space--;
        }
        else
        {
#ifdef DEBUG
            //
            // Increment our receive error counter.
            //
            g_ui32UARTRxErrors++;
#endif
            //
            // Update our error accumulator.
            //
            ui32Errors |= i32Char;
        }

        //
        // Update our count of bytes received via the UART.
        //
        g_ui32UARTRxCount++;
    }

    //
    // Pass back the accumulated error indicators.
    //
    return(ui32Errors);
}

//*****************************************************************************
//
// Take as many bytes from the transmit buffer as we have space for and move
// them into the USB UART's transmit FIFO.
//
//*****************************************************************************
static void
USBUARTPrimeTransmit(uint32_t ui32Base)
{
    uint32_t ui32Read;
    uint8_t ui8Char;

    //
    // If we are currently sending a break condition, don't receive any
    // more data. We will resume transmission once the break is turned off.
    //
    if(HWREGBITW(&g_ui32Flags, FLAG_SENDING_BREAK))
    {
        return;
    }

    //
    // If there is space in the UART FIFO, try to read some characters
    // from the receive buffer to fill it again.
    //
    while(UARTSpaceAvail(ui32Base))
    {
        //
        // Get a character from the buffer.
        //
        ui32Read = USBBufferRead((tUSBBuffer *)&g_sRxBuffer, &ui8Char, 1);

        //
        // Did we get a character?
        //
        if(ui32Read)
        {
            //
            // Place the character in the UART transmit FIFO.
            //
            ROM_UARTCharPutNonBlocking(ui32Base, ui8Char);

            //
            // Update our count of bytes transmitted via the UART.
            //
            g_ui32UARTTxCount++;
        }
        else
        {
            //
            // We ran out of characters so exit the function.
            //
            return;
        }
    }
}

//*****************************************************************************
//
// Interrupt handler for the system tick counter.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Update our system time.
    //
    g_ui32SysTickCount++;
}

//*****************************************************************************
//
// Interrupt handler for the UART which we are redirecting via USB.
//
//*****************************************************************************
void
USBUARTIntHandler(void)
{
    uint32_t ui32Ints, ui32Errors;

    //
    // Get and clear the current interrupt source(s)
    //
    ui32Ints = ROM_UARTIntStatus(UART0_BASE, true);
    ROM_UARTIntClear(UART0_BASE, ui32Ints);

    //
    // Are we being interrupted because the TX FIFO has space available?
    //
    if(ui32Ints & UART_INT_TX)
    {
        //
        // Move as many bytes as we can into the transmit FIFO.
        //
        USBUARTPrimeTransmit(UART0_BASE);

        //
        // If the output buffer is empty, turn off the transmit interrupt.
        //
        if(!USBBufferDataAvailable(&g_sRxBuffer))
        {
            ROM_UARTIntDisable(UART0_BASE, UART_INT_TX);
        }
    }

    //
    // Handle receive interrupts.
    //
    if(ui32Ints & (UART_INT_RX | UART_INT_RT))
    {
        //
        // Read the UART's characters into the buffer.
        //
        ui32Errors = ReadUARTData();

        //
        // Check to see if we need to notify the host of any errors we just
        // detected.
        //
        CheckForSerialStateChange(&g_sCDCDevice, ui32Errors);
    }
}

//*****************************************************************************
//
// Set the state of the RS232 RTS and DTR signals.
//
//*****************************************************************************
static void
SetControlLineState(uint16_t ui16State)
{
    //
    // TODO: If configured with GPIOs controlling the handshake lines,
    // set them appropriately depending upon the flags passed in the wValue
    // field of the request structure passed.
    //
}

//*****************************************************************************
//
// Set the communication parameters to use on the UART.
//
//*****************************************************************************
static bool
SetLineCoding(tLineCoding *psLineCoding, uint32_t ui32UARTClock)
{
    uint32_t ui32Config;
    bool bRetcode;

    //
    // Assume everything is OK until we detect any problem.
    //
    bRetcode = true;

    //
    // Word length.  For invalid values, the default is to set 8 bits per
    // character and return an error.
    //
    switch(psLineCoding->ui8Databits)
    {
        case 5:
        {
            ui32Config = UART_CONFIG_WLEN_5;
            break;
        }

        case 6:
        {
            ui32Config = UART_CONFIG_WLEN_6;
            break;
        }

        case 7:
        {
            ui32Config = UART_CONFIG_WLEN_7;
            break;
        }

        case 8:
        {
            ui32Config = UART_CONFIG_WLEN_8;
            break;
        }

        default:
        {
            ui32Config = UART_CONFIG_WLEN_8;
            bRetcode = false;
            break;
        }
    }

    //
    // Parity.  For any invalid values, we set no parity and return an error.
    //
    switch(psLineCoding->ui8Parity)
    {
        case USB_CDC_PARITY_NONE:
        {
            ui32Config |= UART_CONFIG_PAR_NONE;
            break;
        }

        case USB_CDC_PARITY_ODD:
        {
            ui32Config |= UART_CONFIG_PAR_ODD;
            break;
        }

        case USB_CDC_PARITY_EVEN:
        {
            ui32Config |= UART_CONFIG_PAR_EVEN;
            break;
        }

        case USB_CDC_PARITY_MARK:
        {
            ui32Config |= UART_CONFIG_PAR_ONE;
            break;
        }

        case USB_CDC_PARITY_SPACE:
        {
            ui32Config |= UART_CONFIG_PAR_ZERO;
            break;
        }

        default:
        {
            ui32Config |= UART_CONFIG_PAR_NONE;
            bRetcode = false;
            break;
        }
    }

    //
    // Stop bits.  Our hardware only supports 1 or 2 stop bits whereas CDC
    // allows the host to select 1.5 stop bits.  If passed 1.5 (or any other
    // invalid or unsupported value of ucStop, we set up for 1 stop bit but
    // return an error in case the caller needs to Stall or otherwise report
    // this back to the host.
    //
    switch(psLineCoding->ui8Stop)
    {
        //
        // One stop bit requested.
        //
        case USB_CDC_STOP_BITS_1:
        {
            ui32Config |= UART_CONFIG_STOP_ONE;
            break;
        }

        //
        // Two stop bits requested.
        //
        case USB_CDC_STOP_BITS_2:
        {
            ui32Config |= UART_CONFIG_STOP_TWO;
            break;
        }

        //
        // Other cases are either invalid values of ucStop or values that we
        // cannot support so set 1 stop bit but return an error.
        //
        default:
        {
            ui32Config = UART_CONFIG_STOP_ONE;
            bRetcode |= false;
            break;
        }
    }

    //
    // Set the UART mode appropriately.
    //
    ROM_UARTConfigSetExpClk(UART0_BASE, ui32UARTClock,
                            psLineCoding->ui32Rate, ui32Config);

    //
    // Let the caller know if we had a problem or not.
    //
    return(bRetcode);
}

//*****************************************************************************
//
// Get the communication parameters in use on the UART.
//
//*****************************************************************************
static void
GetLineCoding(tLineCoding *psLineCoding, uint32_t ui32UARTClock)
{
    uint32_t ui32Config, ui32Rate;

    //
    // Get the current line coding set in the UART.
    //
    ROM_UARTConfigGetExpClk(UART0_BASE, ui32UARTClock, &ui32Rate,
                            &ui32Config);
    psLineCoding->ui32Rate = ui32Rate;

    //
    // Translate the configuration word length field into the format expected
    // by the host.
    //
    switch(ui32Config & UART_CONFIG_WLEN_MASK)
    {
        case UART_CONFIG_WLEN_8:
        {
            psLineCoding->ui8Databits = 8;
            break;
        }

        case UART_CONFIG_WLEN_7:
        {
            psLineCoding->ui8Databits = 7;
            break;
        }

        case UART_CONFIG_WLEN_6:
        {
            psLineCoding->ui8Databits = 6;
            break;
        }

        case UART_CONFIG_WLEN_5:
        {
            psLineCoding->ui8Databits = 5;
            break;
        }
    }

    //
    // Translate the configuration parity field into the format expected
    // by the host.
    //
    switch(ui32Config & UART_CONFIG_PAR_MASK)
    {
        case UART_CONFIG_PAR_NONE:
        {
            psLineCoding->ui8Parity = USB_CDC_PARITY_NONE;
            break;
        }

        case UART_CONFIG_PAR_ODD:
        {
            psLineCoding->ui8Parity = USB_CDC_PARITY_ODD;
            break;
        }

        case UART_CONFIG_PAR_EVEN:
        {
            psLineCoding->ui8Parity = USB_CDC_PARITY_EVEN;
            break;
        }

        case UART_CONFIG_PAR_ONE:
        {
            psLineCoding->ui8Parity = USB_CDC_PARITY_MARK;
            break;
        }

        case UART_CONFIG_PAR_ZERO:
        {
            psLineCoding->ui8Parity = USB_CDC_PARITY_SPACE;
            break;
        }
    }

    //
    // Translate the configuration stop bits field into the format expected
    // by the host.
    //
    switch(ui32Config & UART_CONFIG_STOP_MASK)
    {
        case UART_CONFIG_STOP_ONE:
        {
            psLineCoding->ui8Stop = USB_CDC_STOP_BITS_1;
            break;
        }

        case UART_CONFIG_STOP_TWO:
        {
            psLineCoding->ui8Stop = USB_CDC_STOP_BITS_2;
            break;
        }
    }
}

//*****************************************************************************
//
// This function sets or clears a break condition on the redirected UART RX
// line.  A break is started when the function is called with \e bSend set to
// \b true and persists until the function is called again with \e bSend set
// to \b false.
//
//*****************************************************************************
static void
SendBreak(bool bSend)
{
    //
    // Are we being asked to start or stop the break condition?
    //
    if(!bSend)
    {
        //
        // Remove the break condition on the line.
        //
        ROM_UARTBreakCtl(UART0_BASE, false);
        HWREGBITW(&g_ui32Flags, FLAG_SENDING_BREAK) = 0;
    }
    else
    {
        //
        // Start sending a break condition on the line.
        //
        ROM_UARTBreakCtl(UART0_BASE, true);
        HWREGBITW(&g_ui32Flags, FLAG_SENDING_BREAK) = 1;
    }
}

//*****************************************************************************
//
// Shows the status string on the color STN display.
//
// \param psContext is a pointer to the graphics context representing the
// display.
// \param pcStatus is a pointer to the string to be shown.
//
//*****************************************************************************
void
DisplayStatus(tContext *psContext, char *pcStatus)
{
    tRectangle rectLine;
    int32_t i32Y;

    //
    // Calculate the Y coordinate of the top left of the character cell
    // for our line of text.
    //
    i32Y = (GrContextDpyHeightGet(psContext) / 4) -
           (GrFontHeightGet(TEXT_FONT) / 2);

    //
    // Determine the bounding rectangle for this line of text. We add 4 pixels
    // to the height just to ensure that we clear a couple of pixels above and
    // below the line of text.
    //
    rectLine.i16XMin = 0;
    rectLine.i16XMax = GrContextDpyWidthGet(psContext) - 1;
    rectLine.i16YMin = i32Y;
    rectLine.i16YMax = i32Y + GrFontHeightGet(TEXT_FONT) + 3;

    //
    // Clear the line with black.
    //
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(psContext, &rectLine);

    //
    // Draw the new status string
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrStringDrawCentered(psContext, pcStatus, -1,
                         GrContextDpyWidthGet(psContext) / 2,
                         GrContextDpyHeightGet(psContext) / 4 , false);
}

//*****************************************************************************
//
// Draw a horizontal meter at a given position on the display and fill it
// with green.
//
//*****************************************************************************
void
DrawBufferMeter(tContext *psContext, int32_t i32X, int32_t i32Y)
{
    tRectangle sRect;
    int32_t i32CorrectedY;

    //
    // Correct the Y coordinate so that the meter is centered on the same line
    // as the text caption to its left.
    //
    i32CorrectedY = i32Y - ((BUFFER_METER_HEIGHT - TEXT_HEIGHT) / 2);

    //
    // Determine the bounding rectangle of the meter.
    //
    sRect.i16XMin = i32X;
    sRect.i16XMax = i32X + BUFFER_METER_WIDTH - 1;
    sRect.i16YMin = i32CorrectedY;
    sRect.i16YMax = i32CorrectedY + BUFFER_METER_HEIGHT - 1;

    //
    // Fill the meter with green to indicate empty
    //
    GrContextForegroundSet(psContext, ClrGreen);
    GrRectFill(psContext, &sRect);

    //
    // Put a white box around the meter.
    //
    GrContextForegroundSet(psContext, ClrWhite);
    GrRectDraw(psContext, &sRect);
}

//*****************************************************************************
//
// Draw green and red blocks within a graphical meter on the display to
// indicate percentage fullness of some quantity (transmit and receive buffers
// in this case).
//
//*****************************************************************************
void
UpdateBufferMeter(tContext *psContext, uint32_t ui32FullPercent, int32_t i32X,
                  int32_t i32Y)
{
    tRectangle sRect;
    int32_t i32CorrectedY, i32XBreak;

    //
    // Correct the Y coordinate so that the meter is centered on the same line
    // as the text caption to its left and so that we avoid the meter's 1 pixel
    // white border.
    //
    i32CorrectedY = i32Y - ((BUFFER_METER_HEIGHT - TEXT_HEIGHT) / 2) + 1;

    //
    // Determine where the break point between full (red) and empty (green)
    // sections occurs.
    //
    i32XBreak = (i32X + 1) + (ui32FullPercent * (BUFFER_METER_WIDTH - 2)) /
                             100;

    //
    // Determine the bounding rectangle of the full section.
    //
    sRect.i16XMin = i32X + 1;
    sRect.i16XMax = i32XBreak;
    sRect.i16YMin = i32CorrectedY;
    sRect.i16YMax = i32CorrectedY + BUFFER_METER_HEIGHT - 3;

    //
    // Fill the full section with red (if there is anything to draw)
    //
    if(ui32FullPercent)
    {
        GrContextForegroundSet(psContext, ClrRed);
        GrRectFill(psContext, &sRect);
    }

    //
    // Fill the empty section with green.
    //
    sRect.i16XMin = i32XBreak;
    sRect.i16XMax = i32X + BUFFER_METER_WIDTH - 2;
    if(sRect.i16XMax > sRect.i16XMin)
    {
        GrContextForegroundSet(psContext, ClrGreen);
        GrRectFill(psContext, &sRect);
    }

    //
    // Revert to white for text drawing which may occur later.
    //
    GrContextForegroundSet(psContext, ClrWhite);

}

//*****************************************************************************
//
// Handles CDC driver notifications related to control and setup of the device.
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
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
ControlHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue,
               void *pvMsgData)
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
            //
            // Now connected and ready for normal operation.
            //
            HWREGBITW(&g_ui32Flags, FLAG_USB_CONFIGURED) = 1;

            //
            // Flush our buffers.
            //
            USBBufferFlush(&g_sTxBuffer);
            USBBufferFlush(&g_sRxBuffer);

            //
            // Tell the main loop to update the display.
            //
            g_pcStatus = "Host connected.";

            //
            // Set the command status update flag.
            //
            HWREGBITW(&g_ui32Flags, FLAG_STATUS_UPDATE) = 1;

            break;
        }

        //
        // The host has disconnected.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // No longer connected.
            //
            HWREGBITW(&g_ui32Flags, FLAG_USB_CONFIGURED) = 0;

            g_pcStatus = "Host disconnected.";

            //
            // Set the command status update flag.
            //
            HWREGBITW(&g_ui32Flags, FLAG_STATUS_UPDATE) = 1;

            break;
        }

        //
        // Return the current serial communication parameters.
        //
        case USBD_CDC_EVENT_GET_LINE_CODING:
        {
            GetLineCoding(pvMsgData, UART_CLOCK);
            break;
        }

        //
        // Set the current serial communication parameters.
        //
        case USBD_CDC_EVENT_SET_LINE_CODING:
        {
            SetLineCoding(pvMsgData, UART_CLOCK);
            break;
        }

        //
        // Set the current serial communication parameters.
        //
        case USBD_CDC_EVENT_SET_CONTROL_LINE_STATE:
        {
            SetControlLineState((uint16_t)ui32MsgValue);
            break;
        }

        //
        // Send a break condition on the serial line.
        //
        case USBD_CDC_EVENT_SEND_BREAK:
        {
            SendBreak(true);
            break;
        }

        //
        // Clear the break condition on the serial line.
        //
        case USBD_CDC_EVENT_CLEAR_BREAK:
        {
            SendBreak(false);
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
        // We don't expect to receive any other events.  Ignore any that show
        // up in a release build or hang in a debug build.
        //
        default:
        {
#ifdef DEBUG
            while(1);
#else
            break;
#endif
        }

    }

    return(0);
}

//*****************************************************************************
//
// Handles CDC driver notifications related to the transmit channel (data to
// the USB host).
//
// \param pvCBData is the client-supplied callback pointer for this channel.
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
        // We don't expect to receive any other events.  Ignore any that show
        // up in a release build or hang in a debug build.
        //
        default:
        {
#ifdef DEBUG
            while(1);
#else
            break;
#endif
        }
    }
    return(0);
}

//*****************************************************************************
//
// Handles CDC driver notifications related to the receive channel (data from
// the USB host).
//
// \param pvCBData is the client-supplied callback data value for this channel.
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
    uint32_t ui32Count;

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
            // Feed some characters into the UART TX FIFO and enable the
            // interrupt so we are told when there is more space.
            //
            USBUARTPrimeTransmit(UART0_BASE);
            ROM_UARTIntEnable(UART0_BASE, UART_INT_TX);
            break;
        }

        //
        // We are being asked how much unprocessed data we have still to
        // process. We return 0 if the UART is currently idle or 1 if it is
        // in the process of transmitting something. The actual number of
        // bytes in the UART FIFO is not important here, merely whether or
        // not everything previously sent to us has been transmitted.
        //
        case USB_EVENT_DATA_REMAINING:
        {
            //
            // Get the number of bytes in the buffer and add 1 if some data
            // still has to clear the transmitter.
            //
            ui32Count = UARTBusy(UART0_BASE) ? 1 : 0;
            return(ui32Count);
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
        // We don't expect to receive any other events.  Ignore any that show
        // up in a release build or hang in a debug build.
        //
        default:
#ifdef DEBUG
            while(1);
#else
            break;
#endif
    }

    return(0);
}

//*****************************************************************************
//
// This is the main application entry function.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32TxCount, ui32RxCount, ui32Fullness, ui32SysClock, ui32PLLRate;
    tRectangle sRect;
    char pcBuffer[16];
#ifdef USE_ULPI
    uint32_t ui32Setting;
#endif

    //
    // Set the system clock to run at 120MHz from the PLL.
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet();

#ifdef USE_ULPI
    //
    // Switch the USB ULPI Pins over.
    //
    USBULPIPinoutSet();

    //
    // Enable USB ULPI with high speed support.
    //
    ui32Setting = USBLIB_FEATURE_ULPI_HS;
    USBOTGFeatureSet(0, USBLIB_FEATURE_USBULPI, &ui32Setting);

    //
    // Setting the PLL frequency to zero tells the USB library to use the
    // external USB clock.
    //
    ui32PLLRate = 0;
#else
    //
    // Save the PLL rate used by this application.
    //
    ui32PLLRate = 480000000;
#endif

    //
    // Enable the system tick.
    //
    ROM_SysTickPeriodSet(ui32SysClock / TICKS_PER_SECOND);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //
    // Not configured initially.
    //
    g_ui32Flags = 0;

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(ui32SysClock);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&g_sContext, "usb-dev-serial");

    //
    // Fill the top 15 rows of the screen with blue to create the banner.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMax = 23;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Show the various static text elements on the color STN display.
    //
    GrContextFontSet(&g_sContext, TEXT_FONT);
    GrStringDraw(&g_sContext, "Tx bytes:", -1, 8, 80, false);
    GrStringDraw(&g_sContext, "Tx buffer:", -1, 8, 105, false);
    GrStringDraw(&g_sContext, "Rx bytes:", -1, 8, 160, false);
    GrStringDraw(&g_sContext, "Rx buffer:", -1, 8, 185, false);
    DrawBufferMeter(&g_sContext, 150, 105);
    DrawBufferMeter(&g_sContext, 150, 185);

    //
    // Enable the UART that we will be redirecting.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Change the UART clock to the 16 MHz PIOSC.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Set the default UART configuration.
    //
    ROM_UARTConfigSetExpClk(UART0_BASE, UART_CLOCK,
                            DEFAULT_BIT_RATE, DEFAULT_UART_CONFIG);
    ROM_UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX4_8, UART_FIFO_RX4_8);

    //
    // Configure and enable UART interrupts.
    //
    ROM_UARTIntClear(UART0_BASE, ROM_UARTIntStatus(UART0_BASE, false));
    ROM_UARTIntEnable(UART0_BASE, (UART_INT_OE | UART_INT_BE | UART_INT_PE |
                      UART_INT_FE | UART_INT_RT | UART_INT_TX | UART_INT_RX));

    //
    // Tell the user what we are up to.
    //
    DisplayStatus(&g_sContext, " Configuring USB... ");

    //
    // Initialize the transmit and receive buffers.
    //
    USBBufferInit(&g_sTxBuffer);
    USBBufferInit(&g_sRxBuffer);

    //
    // Set the USB stack mode to Device mode with VBUS monitoring.
    //
    USBStackModeSet(0, eUSBModeDevice, 0);

    //
    // Tell the USB library the CPU clock and the PLL frequency.  This is a
    // new requirement for TM4C129 devices.
    //
    USBDCDFeatureSet(0, USBLIB_FEATURE_CPUCLK, &ui32SysClock);
    USBDCDFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

    //
    // Pass our device information to the USB library and place the device
    // on the bus.
    //
    USBDCDCInit(0, (tUSBDCDCDevice *)&g_sCDCDevice);

    //
    // Wait for initial configuration to complete.
    //
    DisplayStatus(&g_sContext, " Waiting for host... ");

    //
    // Clear our local byte counters.
    //
    ui32RxCount = 0;
    ui32TxCount = 0;
    g_ui32UARTTxCount = 0;
    g_ui32UARTRxCount = 0;
#ifdef DEBUG
    g_ui32UARTRxErrors = 0;
#endif

    //
    // Enable interrupts now that the application is ready to start.
    //
    ROM_IntEnable(INT_UART0);

    //
    // Main application loop.
    //
    while(1)
    {
        //
        // Have we been asked to update the status display?
        //
        if(HWREGBITW(&g_ui32Flags, FLAG_STATUS_UPDATE))
        {
            //
            // Clear the command flag
            //
            HWREGBITW(&g_ui32Flags, FLAG_STATUS_UPDATE) = 0;

            DisplayStatus(&g_sContext, g_pcStatus);
        }

        //
        // Has there been any transmit traffic since we last checked?
        //
        if(ui32TxCount != g_ui32UARTTxCount)
        {
            //
            // Take a snapshot of the latest transmit count.
            //
            ui32TxCount = g_ui32UARTTxCount;

            //
            // Update the display of bytes transmitted by the UART.
            //
            usnprintf(pcBuffer, 16, "%d ", ui32TxCount);
            GrStringDraw(&g_sContext, pcBuffer, -1, 150, 80, true);

            //
            // Update the RX buffer fullness. Remember that the buffers are
            // named relative to the USB whereas the status display is from
            // the UART's perspective. The USB's receive buffer is the UART's
            // transmit buffer.
            //
            ui32Fullness = ((USBBufferDataAvailable(&g_sRxBuffer) * 100) /
                          UART_BUFFER_SIZE);

            UpdateBufferMeter(&g_sContext, ui32Fullness, 150, 105);
        }

        //
        // Has there been any receive traffic since we last checked?
        //
        if(ui32RxCount != g_ui32UARTRxCount)
        {
            //
            // Take a snapshot of the latest receive count.
            //
            ui32RxCount = g_ui32UARTRxCount;

            //
            // Update the display of bytes received by the UART.
            //
            usnprintf(pcBuffer, 16, "%d ", ui32RxCount);
            GrStringDraw(&g_sContext, pcBuffer, -1, 150, 160, true);

            //
            // Update the TX buffer fullness. Remember that the buffers are
            // named relative to the USB whereas the status display is from
            // the UART's perspective. The USB's transmit buffer is the UART's
            // receive buffer.
            //
            ui32Fullness = ((USBBufferDataAvailable(&g_sTxBuffer) * 100) /
                          UART_BUFFER_SIZE);

            UpdateBufferMeter(&g_sContext, ui32Fullness, 150, 185);
        }
    }
}
