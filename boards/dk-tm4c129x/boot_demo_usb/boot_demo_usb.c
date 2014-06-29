//*****************************************************************************
//
// boot_demo_usb.c - Main routines for the USB HID/DFU composite device
//                   example.
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
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/usb.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "usblib/device/usbddfu-rt.h"
#include "usblib/device/usbdcomp.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"
#include "usb_hiddfu_structs.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Boot Loader USB Demo (boot_demo_usb)</h1>
//!
//! This example application is used in conjunction with the USB boot loader in
//! ROM and turns the development board into a composite device supporting a
//! mouse via the Human Interface Device class and also publishing runtime
//! Device Firmware Upgrade (DFU) capability.  Dragging a finger or stylus over
//! the touchscreen translates into mouse movement and presses on marked areas
//! at the bottom of the screen indicate mouse button press.  This input is
//! used to generate messages in HID reports sent to the USB host allowing the
//! development board to control the mouse pointer on the host system.
//!
//! Since the device also publishes a DFU interface, host software such as the
//! dfuprog tool can determine that the device is capable of receiving software
//! updates over USB.  The runtime DFU protocol allows such tools to signal the
//! device to switch into DFU mode and prepare to receive a new software image.
//!
//! Runtime DFU functionality requires only that the device listen for a
//! particular request (DETACH) from the host and, when this is received,
//! transfer control to the USB boot loader via the normal means to reenumerate
//! as a pure DFU device capable of uploading and downloading firmware images.
//!
//! Windows device drivers for both the runtime and DFU mode of operation can
//! be found in <tt>C:/TI/TivaWare_C_Series-x.x/windows_drivers</tt> assuming
//! you installed TivaWare in the default directory.
//!
//! To illustrate runtime DFU capability, use the <tt>dfuprog</tt> tool which
//! is part of the Tiva Windows USB Examples package (SW-USB-win-xxxx.msi)
//! Assuming this package is installed in the default location, the
//! <tt>dfuprog</tt> executable can be found in the
//! <tt>C:/Program Files/Texas Instruments/Tiva/usb_examples</tt> or 
//! <tt>C:/Program Files (x86)/Texas Instruments/Tiva/usb_examples</tt>
//! directory.
//!
//! With the device connected to your PC and the device driver installed, enter
//! the following command to enumerate DFU devices:
//!
//! <tt>dfuprog -e</tt>
//!
//! This will list all DFU-capable devices found and you should see that you
//! have one or two devices available which are in ``Runtime'' mode.
//!
//! If you see two devices, it is strongly recommended that you disconnect
//! ICDI debug port from the PC, and power the board either with a 5V external
//! power brick or any usb wall charger which is not plugged in your pc.
//! This way, your PC is connected to the board only through USB OTG port. 
//! The reason for this is that the ICDI chip on the board is DFU-capable
//! device as well as TM4C129X, if not careful, the firmware on the ICDI
//! chip could be accidently erased which can not restored easily. As a
//! result, debug capabilities would be lost!
//!
//! If IDCI debug port is disconnected from your PC, you should see only one
//! device from above command, and its index should be 0, and should be named
//! as ``Mouse with Device Firmware Upgrade''. 
//! If for any reason that you cannot provide the power to the board without
//! connecting ICDI debug port to your PC, the above command should show two
//! devices, the second device is probably named as
//! ``In-Circuit Debug interface'', and we need to be careful not to update
//! the firmware on that device. So please take careful note of the index for
//! the device ``Mouse with Device Firmware Upgrade'', it could be 0 or 1, we
//! will need this index number for the following command. 
//! Entering the following command will switch this device into DFU mode and
//! leave it ready to receive a new firmware image:
//!
//! <tt>dfuprog -i index -m</tt>
//!
//! After entering this command, you should notice that the device disconnects
//! from the USB bus and reconnects again.  Running ``<tt>dfuprog -e</tt>'' a
//! second time will show that the device is now in DFU mode and ready to
//! receive downloads.  At this point, either LM Flash Programmer or dfuprog
//! may be used to send a new application binary to the device.
//
//*****************************************************************************

//*****************************************************************************
//
// The defines used with the g_ui32Commands variable.
//
//*****************************************************************************
#define TOUCH_TICK_EVENT        0x80000000

//*****************************************************************************
//
// The system tick timer rate.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND     50

//*****************************************************************************
//
// This structure defines the area of the display that is devoted to a
// mouse button.  Touchscreen input in this area is translated into press and
// release messages for the given button.
//
//*****************************************************************************
typedef struct
{
    const char *pcLabel;
    uint16_t ui16X;
    uint16_t ui16Width;
    uint8_t ui8ReportFlag;
}
tMouseButtonArea;

//*****************************************************************************
//
// The height of the mouse button bar at the bottom of the display and the
// number of buttons it contains.
//
//*****************************************************************************
#define BUTTON_HEIGHT           30
#define NUM_MOUSE_BUTTONS       3

//*****************************************************************************
//
// Definitions of the positions and labels for each of the three mouse buttons.
//
//*****************************************************************************
static tMouseButtonArea g_sMouseButtons[NUM_MOUSE_BUTTONS] =
{
    { "Button 1", 8,   101, MOUSE_REPORT_BUTTON_1 },
    { "Button 2", 109, 102, MOUSE_REPORT_BUTTON_2 },
    { "Button 3", 211, 101, MOUSE_REPORT_BUTTON_3 }
};

//*****************************************************************************
//
// Holds command bits used to signal the main loop to perform various tasks.
//
//*****************************************************************************
volatile uint32_t g_ui32Commands;

//*****************************************************************************
//
// A flag used to indicate whether or not we are currently connected to the USB
// host.
//
//*****************************************************************************
volatile bool g_bConnected;

//*****************************************************************************
//
// Global system tick counter holds elapsed time since the application started
// expressed in 100ths of a second.
//
//*****************************************************************************
volatile uint32_t g_ui32SysTickCount;

//*****************************************************************************
//
// Holds the previous press position for the touchscreen.
//
//*****************************************************************************
static volatile int32_t g_i32ScreenStartX;
static volatile int32_t g_i32ScreenStartY;

//*****************************************************************************
//
// Holds the current press position for the touchscreen.
//
//*****************************************************************************
static volatile int32_t g_i32ScreenX;
static volatile int32_t g_i32ScreenY;

//*****************************************************************************
//
// Holds the current state of the touchscreen - pressed or not.
//
//*****************************************************************************
static volatile bool g_bScreenPressed;

//*****************************************************************************
//
// Holds the current state of the push button - pressed or not.
//
//*****************************************************************************
static volatile uint8_t g_ui8Buttons;

//*****************************************************************************
//
// This enumeration holds the various states that the mouse can be in during
// normal operation.
//
//*****************************************************************************
volatile enum
{
    //
    // Unconfigured.
    //
    MOUSE_STATE_UNCONFIGURED,

    //
    // No keys to send and not waiting on data.
    //
    MOUSE_STATE_IDLE,

    //
    // Waiting on data to be sent out.
    //
    MOUSE_STATE_SENDING
}
g_eMouseState = MOUSE_STATE_UNCONFIGURED;

//*****************************************************************************
//
// Graphics context used to show text on the display.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// Flag used to tell the main loop that it's time to pass control back to the
// boot loader for an update.
//
//*****************************************************************************
volatile bool g_bUpdateSignalled;

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
// This function is called by the touchscreen driver whenever there is a
// change in press state or position.
//
//*****************************************************************************
static int32_t
MouseTouchHandler(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    uint32_t ui32Loop;

    switch(ui32Message)
    {
        //
        // The touchscreen has been pressed.  Remember where we are so that
        // we can determine how far the pointer moves later.
        //
        case WIDGET_MSG_PTR_DOWN:
        {
            //
            // Save the location of the pointer down event.
            //
            g_i32ScreenStartX = i32X;
            g_i32ScreenStartY = i32Y;
            g_i32ScreenX = i32X;
            g_i32ScreenY = i32Y;
            g_bScreenPressed = true;

            //
            // Is the press within the button area?  If so, determine which
            // button has been pressed.
            //
            if(i32Y >=
               (GrContextDpyHeightGet(&g_sContext) - BUTTON_HEIGHT - 8))
            {
                //
                // Run through the list of buttons to determine which one was
                // pressed.
                //
                for(ui32Loop = 0; ui32Loop < NUM_MOUSE_BUTTONS; ui32Loop++)
                {
                    if((i32X >= g_sMouseButtons[ui32Loop].ui16X) &&
                       (i32X < (g_sMouseButtons[ui32Loop].ui16X +
                        g_sMouseButtons[ui32Loop].ui16Width)))
                    {
                        g_ui8Buttons |=
                            g_sMouseButtons[ui32Loop].ui8ReportFlag;
                        break;
                    }
                }
            }
            break;
        }

        //
        // The touchscreen is no longer being pressed.
        //
        case WIDGET_MSG_PTR_UP:
        {
            g_bScreenPressed = false;

            //
            // Ensure that all buttons are unpressed.
            //
            g_ui8Buttons = 0;
            break;
        }

        //
        // The user is dragging his/her finger/stylus over the touchscreen.
        //
        case WIDGET_MSG_PTR_MOVE:
        {
            g_i32ScreenX = i32X;
            g_i32ScreenY = i32Y;
            break;
        }
    }

    return(0);
}

//*****************************************************************************
//
// This is the callback from the USB DFU runtime interface driver.
//
// \param pvCBData is ignored by this function.
// \param ui32Event is one of the valid events for a DFU device.
// \param ui32MsgParam is defined by the event that occurs.
// \param pvMsgData is a pointer to data that is defined by the event that
// occurs.
//
// This function will be called to inform the application when a change occurs
// during operation as a DFU device.  Currently, the only event passed to this
// callback is USBD_DFU_EVENT_DETACH which tells the recipient that they should
// pass control to the boot loader at the earliest, non-interrupt context
// point.
//
// \return This function will return 0.
//
//*****************************************************************************
uint32_t
DFUDetachCallback(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgData,
                  void *pvMsgData)
{
    if(ui32Event == USBD_DFU_EVENT_DETACH)
    {
        //
        // Set the flag that the main loop uses to determine when it is time
        // to transfer control back to the boot loader.  Note that we
        // absolutely DO NOT call USBDDFUUpdateBegin() here since we are
        // currently in interrupt context and this would cause bad things to
        // happen (and the boot loader to not work).
        //
        g_bUpdateSignalled = true;
    }

    return(0);
}

//*****************************************************************************
//
// This is the callback from the USB composite device class driver.
//
// \param pvCBData is ignored by this function.
// \param ui32Event is one of the valid events for a mouse device.
// \param ui32MsgParam is defined by the event that occurs.
// \param pvMsgData is a pointer to data that is defined by the event that
// occurs.
//
// This function will be called to inform the application when a change occurs
// during operation as a HID class USB mouse device.
//
// \return This function will return 0.
//
//*****************************************************************************
uint32_t
MouseHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgData,
             void *pvMsgData)
{
    switch(ui32Event)
    {
        //
        // The USB host has connected to and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            g_eMouseState = MOUSE_STATE_IDLE;
            g_bConnected = true;
            break;
        }

        //
        // The USB host has disconnected from the device.
        //
        case USB_EVENT_DISCONNECTED:
        {
            g_bConnected = false;
            g_eMouseState = MOUSE_STATE_UNCONFIGURED;
            break;
        }

        //
        // A report was sent to the host. We are not free to send another.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            g_eMouseState = MOUSE_STATE_IDLE;
            break;
        }

    }
    return(0);
}

//*****************************************************************************
//
// This function updates the display to show button state.
//
// This function is called from ButtonHandler to update the display showing
// the state of each of the buttons.
//
// Returns None.
//
//*****************************************************************************
void
UpdateDisplay(uint8_t ui8Buttons, bool bRedraw)
{
    static uint8_t ui8LastButtons;
    tRectangle sRect, sRectOutline;
    uint32_t ui32Loop;

    //
    // Initialize the Y coordinates of the button rectangle.
    //
    sRectOutline.i16YMin =
        GrContextDpyHeightGet(&g_sContext) - BUTTON_HEIGHT - 8;
    sRectOutline.i16YMax = GrContextDpyHeightGet(&g_sContext) - 1 - 8;
    sRect.i16YMin = sRectOutline.i16YMin + 1;
    sRect.i16YMax = sRectOutline.i16YMax - 1;

    //
    // Loop through each of the mouse buttons, drawing each in turn.
    //
    for(ui32Loop = 0; ui32Loop < NUM_MOUSE_BUTTONS; ui32Loop++)
    {
        //
        // Draw the outline if we are redrawing the whole button area.
        //
        if(bRedraw)
        {
            GrContextForegroundSet(&g_sContext, ClrWhite);

            sRectOutline.i16XMin = g_sMouseButtons[ui32Loop].ui16X;
            sRectOutline.i16XMax = (sRectOutline.i16XMin +
                                    g_sMouseButtons[ui32Loop].ui16Width) - 1;

            GrRectDraw(&g_sContext, &sRectOutline);
        }

        //
        // Has the button state changed since we last drew it or are we
        // drawing the buttons unconditionally?
        //
        if(((g_ui8Buttons & g_sMouseButtons[ui32Loop].ui8ReportFlag) !=
           (ui8LastButtons & g_sMouseButtons[ui32Loop].ui8ReportFlag)) ||
           bRedraw)
        {
            //
            // Set the appropriate button color depending upon whether the
            // button is pressed or not.
            //
            GrContextForegroundSet(&g_sContext,
                                   ((g_ui8Buttons &
                                     g_sMouseButtons[ui32Loop].ui8ReportFlag) ?
                                    ClrRed : ClrGreen));

            sRect.i16XMin = g_sMouseButtons[ui32Loop].ui16X + 1;
            sRect.i16XMax = (sRect.i16XMin +
                             g_sMouseButtons[ui32Loop].ui16Width) - 3;
            GrRectFill(&g_sContext, &sRect);

            //
            // Draw the button text.
            //
            GrContextForegroundSet(&g_sContext, ClrWhite);
            GrStringDrawCentered(&g_sContext,
                                 g_sMouseButtons[ui32Loop].pcLabel,
                                 -1, (sRect.i16XMin + sRect.i16XMax) / 2,
                                 (sRect.i16YMin + sRect.i16YMax) / 2, 0);
        }
    }

    //
    // Remember the button state we just drew.
    //
    ui8LastButtons = ui8Buttons;
}

//*****************************************************************************
//
// This function handles updates due to touchscreen input.
//
// This function is called periodically from the main loop to check the
// touchscreen state and, if necessary, send a HID report back to the host
// system.
//
// Returns Returns \b true on success or \b false if an error is detected.
//
//*****************************************************************************
static void
TouchHandler(void)
{
    int32_t i32DeltaX, i32DeltaY;
    static uint8_t ui8Buttons = 0;

    //
    // Is someone pressing the screen or has the button changed state?  If so,
    // we determine how far they have dragged their finger/stylus and use this
    // to calculate mouse position changes to send to the host.
    //
    if(g_bScreenPressed || (ui8Buttons != g_ui8Buttons))
    {
        //
        // Calculate how far we moved since the last time we checked.  This
        // rather odd layout prevents a compiler warning about undefined order
        // of volatile accesses.
        //
        i32DeltaX = g_i32ScreenX;
        i32DeltaX -= g_i32ScreenStartX;
        i32DeltaY = g_i32ScreenY;
        i32DeltaY -= g_i32ScreenStartY;

        //
        // Reset our start position.
        //
        g_i32ScreenStartX = g_i32ScreenX;
        g_i32ScreenStartY = g_i32ScreenY;

        //
        // Was there any movement or change in button state?
        //
        if(i32DeltaX || i32DeltaY || (ui8Buttons != g_ui8Buttons))
        {
            //
            // Yes - send a report back to the host after clipping the deltas
            // to the maximum we can support.
            //
            i32DeltaX = (i32DeltaX > 127) ? 127 : i32DeltaX;
            i32DeltaX = (i32DeltaX < -128) ? -128 : i32DeltaX;
            i32DeltaY = (i32DeltaY > 127) ? 127 : i32DeltaY;
            i32DeltaY = (i32DeltaY < -128) ? -128 : i32DeltaY;

            //
            // Remember the current button state.
            //
            ui8Buttons = g_ui8Buttons;

            //
            // Send the report back to the host.
            //
            USBDHIDMouseStateChange((void *)&g_sMouseDevice,
                                    (char)i32DeltaX, (char)i32DeltaY,
                                    ui8Buttons);
        }

        //
        // Update the button portion of the display.
        //
        UpdateDisplay(ui8Buttons, false);
    }
}

//*****************************************************************************
//
// This is the interrupt handler for the SysTick interrupt.  It is called
// periodically and updates a global tick counter then sets a flag to tell the
// main loop to check the button state.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    g_ui32SysTickCount++;
    g_ui32Commands |= TOUCH_TICK_EVENT;
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32SysClock;
    tRectangle sRect;

    //
    // Run from the PLL at 120 MHz.
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet();

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
    FrameDraw(&g_sContext, "boot-demo-usb");

    //
    // Set the system tick to fire 100 times per second.
    //
    ROM_SysTickPeriodSet(ui32SysClock / SYSTICKS_PER_SECOND);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //
    // Draw the buttons in their initial (unpressed)state.
    //
    UpdateDisplay(g_ui8Buttons, true);

    //
    // Initialize each of the device instances that will form our composite
    // USB device.
    //
    USBDHIDMouseCompositeInit(0, &g_sMouseDevice,
                              &(g_sCompDevice.psDevices[0]));
    USBDDFUCompositeInit(0, &g_sDFUDevice, &(g_sCompDevice.psDevices[1]));

    //
    // Pass the USB library our device information, initialize the USB
    // controller and connect the device to the bus.
    //
    USBDCompositeInit(0, &g_sCompDevice, DESCRIPTOR_BUFFER_SIZE,
                      g_pui8DescriptorBuffer);

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(ui32SysClock);

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(MouseTouchHandler);

    //
    // Drop into the main loop.
    //
    while(!g_bUpdateSignalled)
    {
        //
        // Tell the user what we are doing.
        //
        GrContextForegroundSet(&g_sContext, ClrWhite);
        GrStringDrawCentered(&g_sContext, "   Waiting for host...   ", -1, 160,
                             110, true);

        //
        // Wait for USB configuration to complete.
        //
        while(!g_bConnected)
        {
        }

        //
        // Update the status.
        //
        GrStringDrawCentered(&g_sContext, "   Host connected...   ", -1, 160,
                             110, true);

        //
        // Now keep processing the mouse as long as the host is connected and
        // we've not been told to prepare for a firmware upgrade.
        //
        while(g_bConnected && !g_bUpdateSignalled)
        {
            //
            // If it is time to check the touchscreen state then do so.
            //
            if(g_ui32Commands & TOUCH_TICK_EVENT)
            {
                g_ui32Commands &= ~TOUCH_TICK_EVENT;
                TouchHandler();
            }
        }

        //
        // If we drop out of the previous loop, either the host has
        // disconnected or a firmware upgrade has been signalled.
        //
    }

    //
    // Tell the user what's going on.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext);
    sRect.i16YMax = GrContextDpyHeightGet(&g_sContext);
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrStringDrawCentered(&g_sContext, " Switching to DFU mode ", -1, 160, 118,
                         true);

    //
    // Terminate the USB device and detach from the bus.
    //
    USBDCDTerm(0);

    //
    // Disable all interrupts.
    //
    ROM_IntMasterDisable();

    //
    // Disable SysTick and its interrupt.
    //
    ROM_SysTickIntDisable();
    ROM_SysTickDisable();

    //
    // Disable all processor interrupts.  Instead of disabling them one at a
    // time, a direct write to NVIC is done to disable all peripheral
    // interrupts.
    //
    HWREG(NVIC_DIS0) = 0xffffffff;
    HWREG(NVIC_DIS1) = 0xffffffff;
    HWREG(NVIC_DIS2) = 0xffffffff;
    HWREG(NVIC_DIS3) = 0xffffffff;
    HWREG(NVIC_DIS4) = 0xffffffff;

    //
    // Enable and reset the USB peripheral.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_USB0);
    ROM_USBClockEnable(USB0_BASE, 8, USB_CLOCK_INTERNAL);

    //
    // Wait for about a second.
    //
    ROM_SysCtlDelay(ui32SysClock / 3);

    //
    // Re-enable interrupts at the NVIC level.
    //
    ROM_IntMasterEnable();

    //
    // Call the USB boot loader.
    //
    ROM_UpdateUSB(0);

    //
    // Should never get here, but just in case.
    //
    while(1)
    {
    }
}
