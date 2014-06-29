//*****************************************************************************
//
// usb_dev_msc.c - Main routines for the device mass storage class example.
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
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/udma.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdmsc.h"
#include "utils/ustdlib.h"
#include "fatfs/src/diskio.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/mx66l51235f.h"
#include "drivers/pinout.h"
#include "usbdspiflash.h"
#include "usb_msc_structs.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB MSC Device (usb_dev_msc)</h1>
//!
//! This example application turns the evaluation board into a USB mass storage
//! class device.  The application will use the onboard flash memory for the
//! storage media for the mass storage device.  The screen will display the
//! current action occurring on the device ranging from disconnected, reading,
//! writing and idle.
//
//*****************************************************************************

//*****************************************************************************
//
// The number of ticks to wait before falling back to the idle state.  Since
// the tick rate is 100Hz this is approximately 1 second.
//
//*****************************************************************************
#define USBMSC_ACTIVITY_TIMEOUT 100

//*****************************************************************************
//
// This enumeration holds the various states that the device can be in during
// normal operation.
//
//*****************************************************************************
volatile enum
{
    //
    // Unconfigured.
    //
    MSC_DEV_DISCONNECTED,

    //
    // Connected and fully enumerated but not currently handling a command.
    //
    MSC_DEV_IDLE,

    //
    // Currently reading the device.
    //
    MSC_DEV_READ,

    //
    // Currently writing the device.
    //
    MSC_DEV_WRITE,
}
g_eMSCState;

//*****************************************************************************
//
// The Flags that handle updates to the status area to avoid drawing when no
// updates are required..
//
//*****************************************************************************
#define FLAG_UPDATE_STATUS      1
static uint32_t g_ui32Flags;
static uint32_t g_ui32IdleTimeout;

//*****************************************************************************
//
// Graphics context used to show text on the display.
//
//*****************************************************************************
tContext g_sContext;

//******************************************************************************
//
// The DMA control structure table.
//
//******************************************************************************
#ifdef ewarm
#pragma data_alignment=1024
tDMAControlTable psDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(psDMAControlTable, 1024)
tDMAControlTable psDMAControlTable[64];
#else
tDMAControlTable psDMAControlTable[64] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// A buffer used for updating the read/write counts.
//
//*****************************************************************************
static char g_pcBuffer[32];

//*****************************************************************************
//
// The system clock frequency in Hz.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Handles bulk driver notifications related to the receive channel (data from
// the USB host).
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ui32Event identifies the event we are being notified about.
// \param ui32MsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the bulk driver to notify us of any events
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
    return(0);
}

//*****************************************************************************
//
// Handles bulk driver notifications related to the transmit channel (data to
// the USB host).
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ui32Event identifies the event we are being notified about.
// \param ui32MsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the bulk driver to notify us of any events
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
    return(0);
}

//*****************************************************************************
//
// This function updates the status area of the screen.  It uses the current
// state of the application to print the status area.
//
//*****************************************************************************
void
UpdateStatus(char *pcString)
{
    //
    // Write the current state to the bottom of screen.
    //
    GrStringDrawCentered(&g_sContext, pcString, -1, 160, 78, true);
}

//*****************************************************************************
//
// This function updates the read/write count area of the screen.
//
//*****************************************************************************
void
UpdateCount(uint32_t ui32Count, uint32_t ui32Y)
{
    //
    // See if the count is at least a billion.
    //
    if(ui32Count > 999999999)
    {
        //
        // Format the value with commas separating the billions, millions,
        // thousands, and ones.
        //
        usnprintf(g_pcBuffer, sizeof(g_pcBuffer), "  %d,%03d,%03d,%03d  ",
                  ui32Count / 1000000000, (ui32Count / 1000000) % 1000,
                  (ui32Count / 1000) % 1000, ui32Count % 1000);
    }

    //
    // See if the count is at least a million.
    //
    else if(ui32Count > 999999)
    {
        //
        // Format the value with commas separating the millions, thousands, and
        // ones.
        //
        usnprintf(g_pcBuffer, sizeof(g_pcBuffer), "  %d,%03d,%03d  ",
                  (ui32Count / 1000000) % 1000, (ui32Count / 1000) % 1000,
                  ui32Count % 1000);
    }

    //
    // See if the count is at least a thousand.
    //
    else if(ui32Count > 999)
    {
        //
        // Format the value with a comma separating the thousands and ones.
        //
        usnprintf(g_pcBuffer, sizeof(g_pcBuffer), "  %d,%03d  ",
                  (ui32Count / 1000) % 1000, ui32Count % 1000);
    }

    //
    // Otherwise the value is less than a thousand.
    //
    else
    {
        //
        // Simply print the value.  This will occur in two cases: at startup
        // when the count is 0 and after the 32-bit count has rolled-over.  In
        // the later case, a large number of spaces are needed before and after
        // the actual value in order to erase the text from the large value
        // that preceeded the roll-over.
        //
        usnprintf(g_pcBuffer, sizeof(g_pcBuffer), "          %d          ",
                  ui32Count);
    }

    //
    // Draw the formatted string.
    //
    GrStringDrawCentered(&g_sContext, g_pcBuffer, -1, 160, ui32Y, true);
}

//*****************************************************************************
//
// This function is the call back notification function provided to the USB
// library's mass storage class.
//
//*****************************************************************************
uint32_t
USBDMSCEventCallback(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam,
                     void *pvMsgData)
{
    //
    // Reset the time out every time an event occurs.
    //
    g_ui32IdleTimeout = USBMSC_ACTIVITY_TIMEOUT;

    switch(ui32Event)
    {
        //
        // Writing to the device.
        //
        case USBD_MSC_EVENT_WRITING:
        {
            //
            // Only update if this is a change.
            //
            if(g_eMSCState != MSC_DEV_WRITE)
            {
                //
                // Go to the write state.
                //
                g_eMSCState = MSC_DEV_WRITE;

                //
                // Cause the main loop to update the screen.
                //
                g_ui32Flags |= FLAG_UPDATE_STATUS;
            }

            break;
        }

        //
        // Reading from the device.
        //
        case USBD_MSC_EVENT_READING:
        {
            //
            // Only update if this is a change.
            //
            if(g_eMSCState != MSC_DEV_READ)
            {
                //
                // Go to the read state.
                //
                g_eMSCState = MSC_DEV_READ;

                //
                // Cause the main loop to update the screen.
                //
                g_ui32Flags |= FLAG_UPDATE_STATUS;
            }

            break;
        }

        //
        // The USB host has disconnected from the device.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // Go to the disconnected state.
            //
            g_eMSCState = MSC_DEV_DISCONNECTED;

            //
            // Cause the main loop to update the screen.
            //
            g_ui32Flags |= FLAG_UPDATE_STATUS;

            break;
        }

        //
        // The USB host has connected to the device.
        //
        case USB_EVENT_CONNECTED:
        {
            //
            // Go to the idle state to wait for read/writes.
            //
            g_eMSCState = MSC_DEV_IDLE;

            break;
        }

        case USBD_MSC_EVENT_IDLE:
        default:
        {
            break;
        }
    }

    return(0);
}

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.  This is to detect idle
// state for the SPI flash.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    if(g_ui32IdleTimeout != 0)
    {
        g_ui32IdleTimeout--;
    }
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32Read, ui32Write;

    //
    // Run from the PLL at 120 MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(g_ui32SysClock);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Draw the application frame.
    //
    FrameDraw(&g_sContext, "usb-dev-msc");

    //
    // Place the static status text on the display.
    //
    GrStringDrawCentered(&g_sContext, "Status", -1, 160, 58, false);
    GrStringDrawCentered(&g_sContext, "Bytes Read", -1, 160, 118, false);
    GrStringDrawCentered(&g_sContext, "Bytes Written", -1, 160, 178, false);
    GrContextForegroundSet(&g_sContext, ClrGray);
    UpdateCount(0, 138);
    UpdateCount(0, 198);

    //
    // Configure SysTick for a 100Hz interrupt.  This is to detect idle state
    // every 10ms after a state change.
    //
    ROM_SysTickPeriodSet(g_ui32SysClock / 100);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Configure and enable uDMA
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);
    ROM_uDMAControlBaseSet(&psDMAControlTable[0]);
    ROM_uDMAEnable();

    //
    // Initialize the idle timeout and reset all flags.
    //
    g_ui32IdleTimeout = 0;
    g_ui32Flags = 0;

    //
    // Initialize the state to idle.
    //
    g_eMSCState = MSC_DEV_DISCONNECTED;

    //
    // Draw the status bar and set it to idle.
    //
    UpdateStatus("Disconnected");

    //
    // Enable the USB controller.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);

    //
    // Enable the SSI3 used by SPI flash.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_SSI3);

    //
    // Set the USB stack mode to Device mode with VBUS monitoring.
    //
    USBStackModeSet(0, eUSBModeDevice, 0);

    //
    // Pass our device information to the USB library and place the device
    // on the bus.
    //
    USBDMSCInit(0, (tUSBDMSCDevice*)&g_sMSCDevice);

    //
    // Initialize the SD card, if present.  This prevents it from interfering
    // with accesses to the SPI flash.
    //
    disk_initialize(0);

    //
    // Initialize MX66L51235F Flash memory.
    //
    MX66L51235FInit(g_ui32SysClock);

    //
    // Drop into the main loop.
    //
    ui32Read = g_ui32ReadCount;
    ui32Write = g_ui32WriteCount;
    while(1)
    {
        switch(g_eMSCState)
        {
            case MSC_DEV_READ:
            {
                //
                // Update the screen if necessary.
                //
                if(g_ui32Flags & FLAG_UPDATE_STATUS)
                {
                    UpdateStatus("        Reading        ");
                    g_ui32Flags &= ~FLAG_UPDATE_STATUS;
                }

                //
                // If there is no activity then return to the idle state.
                //
                if(g_ui32IdleTimeout == 0)
                {
                    UpdateStatus("        Idle        ");
                    g_eMSCState = MSC_DEV_IDLE;
                }

                break;
            }

            case MSC_DEV_WRITE:
            {
                //
                // Update the screen if necessary.
                //
                if(g_ui32Flags & FLAG_UPDATE_STATUS)
                {
                    UpdateStatus("        Writing        ");
                    g_ui32Flags &= ~FLAG_UPDATE_STATUS;
                }

                //
                // If there is no activity then return to the idle state.
                //
                if(g_ui32IdleTimeout == 0)
                {
                    UpdateStatus("        Idle        ");
                    g_eMSCState = MSC_DEV_IDLE;
                }
                break;
            }

            case MSC_DEV_DISCONNECTED:
            {
                //
                // Update the screen if necessary.
                //
                if(g_ui32Flags & FLAG_UPDATE_STATUS)
                {
                    UpdateStatus("        Disconnected        ");
                    g_ui32Flags &= ~FLAG_UPDATE_STATUS;
                }
                break;
            }

            case MSC_DEV_IDLE:
            {
                break;
            }

            default:
            {
                break;
            }
        }

        //
        // Update the read count if it has changed.
        //
        if(g_ui32ReadCount != ui32Read)
        {
            ui32Read = g_ui32ReadCount;
            UpdateCount(ui32Read, 138);
        }

        //
        // Update the write count if it has changed.
        //
        if(g_ui32WriteCount != ui32Write)
        {
            ui32Write = g_ui32WriteCount;
            UpdateCount(ui32Write, 198);
        }
    }
}
