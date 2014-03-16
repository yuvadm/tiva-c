//*****************************************************************************
//
// usb_host_keyboard.c - main application code for the host keyboard example.
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
// This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/usb.h"
#include "driverlib/udma.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhhid.h"
#include "usblib/host/usbhhidkeyboard.h"
#include "utils/uartstdio.h"
#include "drivers/pinout.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB HID Keyboard Host (usb_host_keyboard)</h1>
//!
//! This application demonstrates the handling of a USB keyboard attached to
//! the evaluation kit.  Once attached, text typed on the keyboard will appear
//! on the UART.  Any keyboard that supports the USB HID BIOS protocol is
//! supported.
//!
//! UART0, connected to the ICDI virtual COM port and running at 115,200,
//! 8-N-1, is used to display messages from this application.
//
//*****************************************************************************

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100
#define MS_PER_SYSTICK (1000 / TICKS_PER_SECOND)

//*****************************************************************************
//
// Our running system tick counter and a global used to determine the time
// elapsed since last call to GetTickms().
//
//*****************************************************************************
uint32_t g_ui32SysTickCount;
uint32_t g_ui32LastTick;

//*****************************************************************************
//
// The size of the host controller's memory pool in bytes.
//
//*****************************************************************************
#define HCD_MEMORY_SIZE         128

//*****************************************************************************
//
// The memory pool to provide to the Host controller driver.
//
//*****************************************************************************
uint8_t g_pHCDPool[HCD_MEMORY_SIZE];

//*****************************************************************************
//
// The size of the keyboard device interface's memory pool in bytes.
//
//*****************************************************************************
#define KEYBOARD_MEMORY_SIZE    128

//*****************************************************************************
//
// The memory pool to provide to the keyboard device.
//
//*****************************************************************************
uint8_t g_pui8Buffer[KEYBOARD_MEMORY_SIZE];

//*****************************************************************************
//
// Declare the USB Events driver interface.
//
//*****************************************************************************
DECLARE_EVENT_DRIVER(g_sUSBEventDriver, 0, 0, USBHCDEvents);

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the Keyboard class is loaded.
//
//*****************************************************************************
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
    &g_sUSBHIDClassDriver,
    &g_sUSBEventDriver
};

//*****************************************************************************
//
// This global holds the number of class drivers in the g_ppHostClassDrivers
// list.
//
//*****************************************************************************
static const uint32_t g_ui32NumHostClassDrivers =
    sizeof(g_ppHostClassDrivers) / sizeof(tUSBHostClassDriver *);

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.  In this application uDMA is only used for USB,
// so only the first 6 channels are needed.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
tDMAControlTable g_sDMAControlTable[6];
#elif defined(ccs)
#pragma DATA_ALIGN(g_sDMAControlTable, 1024)
tDMAControlTable g_sDMAControlTable[6];
#else
tDMAControlTable g_sDMAControlTable[6] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// The instance data for the MSC driver.
//
//*****************************************************************************
tUSBHKeyboard *g_psKeyboardInstance = 0;


//*****************************************************************************
//
// This enumerated type is used to hold the states of the keyboard.
//
//*****************************************************************************
enum
{
    //
    // No device is present.
    //
    STATE_NO_DEVICE,

    //
    // Keyboard has been detected and needs to be initialized in the main
    // loop.
    //
    STATE_KEYBOARD_INIT,

    //
    // Keyboard is connected and waiting for events.
    //
    STATE_KEYBOARD_CONNECTED,

    //
    // Keyboard has received a key press that requires updating the keyboard
    // in the main loop.
    //
    STATE_KEYBOARD_UPDATE,

    //
    // An unsupported device has been attached.
    //
    STATE_UNKNOWN_DEVICE,

    //
    // A power fault has occurred.
    //
    STATE_POWER_FAULT
}
g_eUSBState;

extern const tHIDKeyboardUsageTable g_sUSKeyboardMap;

//*****************************************************************************
//
// This variable holds the current status of the modifiers keys.
//
//*****************************************************************************
uint32_t g_ui32Modifiers = 0;

//*****************************************************************************
//
// The current USB operating mode - Host, Device or unknown.
//
//*****************************************************************************
tUSBMode g_eCurrentUSBMode;

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
// This is the handler for this SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Update our tick counter.
    //
    g_ui32SysTickCount++;
}

//*****************************************************************************
//
// This function returns the number of ticks since the last time this function
// was called.
//
//*****************************************************************************
uint32_t
GetTickms(void)
{
    uint32_t ui32RetVal;
    uint32_t ui32Saved;

    ui32RetVal = g_ui32SysTickCount;
    ui32Saved = ui32RetVal;

    if(ui32Saved > g_ui32LastTick)
    {
        ui32RetVal = ui32Saved - g_ui32LastTick;
    }
    else
    {
        ui32RetVal = g_ui32LastTick - ui32Saved;
    }

    //
    // This could miss a few milliseconds but the timings here are on a
    // much larger scale.
    //
    g_ui32LastTick = ui32Saved;

    //
    // Return the number of milliseconds since the last time this was called.
    //
    return(ui32RetVal * MS_PER_SYSTICK);
}

//*****************************************************************************
//
// This is the generic callback from host stack.
//
// \param pvData is actually a pointer to a tEventInfo structure.
//
// This function will be called to inform the application when a USB event has
// occurred that is outside those related to the keyboard device.  At this
// point this is used to detect unsupported devices being inserted and removed.
// It is also used to inform the application when a power fault has occurred.
// This function is required when the g_USBGenericEventDriver is included in
// the host controller driver array that is passed in to the
// USBHCDRegisterDrivers() function.
//
// \return None.
//
//*****************************************************************************
void
USBHCDEvents(void *pvData)
{
    tEventInfo *pEventInfo;

    //
    // Cast this pointer to its actual type.
    //
    pEventInfo = (tEventInfo *)pvData;

    switch(pEventInfo->ui32Event)
    {
        //
        // New keyboard detected.
        //
        case USB_EVENT_CONNECTED:
        {
            //
            // See if this is a HID Keyboard.
            //
            if((USBHCDDevClass(pEventInfo->ui32Instance, 0) == USB_CLASS_HID) &&
               (USBHCDDevProtocol(pEventInfo->ui32Instance, 0) ==
                USB_HID_PROTOCOL_KEYB))
            {
                //
                // Indicate that the keyboard has been detected.
                //
                UARTprintf("\nKeyboard Connected\n");

                //
                // Proceed to the STATE_KEYBOARD_INIT state so that the main
                // loop can finish initialized the mouse since
                // USBHKeyboardInit() cannot be called from within a callback.
                //
                g_eUSBState = STATE_KEYBOARD_INIT;
            }

            break;
        }
        //
        // Unsupported device detected.
        //
        case USB_EVENT_UNKNOWN_CONNECTED:
        {
            UARTprintf("Unsupported Device Class (0x%02x) Connected.\n",
                       pEventInfo->ui32Instance);

            //
            // An unknown device was detected.
            //
            g_eUSBState = STATE_UNKNOWN_DEVICE;

            break;
        }
        //
        // Device has been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // Indicate that the device has been disconnected.
            //
            UARTprintf("\nDevice Disconnected\n");

            //
            // Change the state so that the main loop knows that the device
            // is no longer present.
            //
            g_eUSBState = STATE_NO_DEVICE;

            break;
        }
        //
        // Power Fault has occurred.
        //
        case USB_EVENT_POWER_FAULT:
        {
            UARTprintf("Power Fault\n");

            //
            // No power means no device is present.
            //
            g_eUSBState = STATE_POWER_FAULT;

            break;
        }

        default:
        {
            break;
        }
    }
}

//*****************************************************************************
//
// This is the callback from the USB HID keyboard handler.
//
// \param pvCBData is ignored by this function.
// \param ui32Event is one of the valid events for a keyboard device.
// \param ui32MsgParam is defined by the event that occurs.
// \param pvMsgData is a pointer to data that is defined by the event that
// occurs.
//
// This function will be called to inform the application when a keyboard has
// been plugged in or removed and any time a key is pressed or released.
//
// \return This function will return 0.
//
//*****************************************************************************
void
KeyboardCallback(tUSBHKeyboard *psKbInstance, uint32_t ui32Event,
                 uint32_t ui32MsgParam, void *pvMsgData)
{
    unsigned char ucChar;

    switch(ui32Event)
    {
        //
        // New Key press detected.
        //
        case USBH_EVENT_HID_KB_PRESS:
        {
            //
            // If this was a Caps Lock key then update the Caps Lock state.
            //
            if(ui32MsgParam == HID_KEYB_USAGE_CAPSLOCK)
            {
                //
                // The main loop needs to update the keyboard's Caps Lock
                // state.
                //
                g_eUSBState = STATE_KEYBOARD_UPDATE;

                //
                // Toggle the current Caps Lock state.
                //
                g_ui32Modifiers ^= HID_KEYB_CAPS_LOCK;
            }
            else if(ui32MsgParam == HID_KEYB_USAGE_SCROLLOCK)
            {
                //
                // The main loop needs to update the keyboard's Scroll Lock
                // state.
                //
                g_eUSBState = STATE_KEYBOARD_UPDATE;

                //
                // Toggle the current Scroll Lock state.
                //
                g_ui32Modifiers ^= HID_KEYB_SCROLL_LOCK;
            }
            else if(ui32MsgParam == HID_KEYB_USAGE_NUMLOCK)
            {
                //
                // The main loop needs to update the keyboard's Scroll Lock
                // state.
                //
                g_eUSBState = STATE_KEYBOARD_UPDATE;

                //
                // Toggle the current Num Lock state.
                //
                g_ui32Modifiers ^= HID_KEYB_NUM_LOCK;
            }
            else if(ui32MsgParam == HID_KEYB_USAGE_BACKSPACE)
            {
                //
                // This is the backspace, so move the cursor left and erase
                // that character.
                //
                UARTprintf("\b \b");
            }
            else
            {
                //
                // Print the current key out the UART.
                //
                ucChar = (unsigned char)
                    USBHKeyboardUsageToChar(g_psKeyboardInstance,
                                            &g_sUSKeyboardMap,
                                            (unsigned char)ui32MsgParam);
                //
                // A zero value indicates there was no textual mapping of this
                // usage code.
                //
                if(ucChar != 0)
                {
                    UARTprintf("%c", ucChar);
                }
            }
            break;
        }

        case USBH_EVENT_HID_KB_MOD:
        {
            //
            // This application ignores the state of the shift or control
            // and other special keys.
            //
            break;
        }

        case USBH_EVENT_HID_KB_REL:
        {
            //
            // This applications ignores the release of keys as well.
            //
            break;
        }
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
    uint32_t ui32SysClock;

    //
    // Initially wait for device connection.
    //
    g_eUSBState = STATE_NO_DEVICE;

    //
    // Run from the PLL at 120 MHz.
    //

    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN |
                                           SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Enable the pins and peripherals used by this example.
    //
    PinoutSet(0,1);

    //
    // Enable the UART and print a brief message.
    //
    UARTStdioConfig(0, 115200, ui32SysClock);
    UARTprintf("\033[2J\033[H");
    UARTprintf("Host Keyboard Application\n");

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    ROM_SysTickPeriodSet(ui32SysClock / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable the uDMA controller and set up the control table base.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    ROM_uDMAEnable();
    ROM_uDMAControlBaseSet(g_sDMAControlTable);

    //
    // Initialize the USB stack in host mode. No callback is needed at this
    // time.
    //
    USBStackModeSet(0, eUSBModeHost, 0);

    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ui32NumHostClassDrivers);

    //
    // Open an instance of the keyboard driver.  The keyboard does not need
    // to be present at this time, this just save a place for it and allows
    // the applications to be notified when a keyboard is present.
    //
    g_psKeyboardInstance = USBHKeyboardOpen(KeyboardCallback, g_pui8Buffer,
                                            KEYBOARD_MEMORY_SIZE);

    //
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);

    //
    // Initialize the USB controller for host operation.
    //
    USBHCDInit(0, g_pHCDPool, HCD_MEMORY_SIZE);

    //
    // The main loop for the application.
    //
    while(1)
    {
        //
        // Tell the OTG library code how much time has passed in
        // milliseconds since the last call.
        //
        USBOTGMain(GetTickms());

        switch(g_eUSBState)
        {
            //
            // This state is entered when they keyboard is first detected.
            //
            case STATE_KEYBOARD_INIT:
            {
                //
                // Initialized the newly connected keyboard.
                //
                USBHKeyboardInit(g_psKeyboardInstance);

                //
                // Proceed to the keyboard connected state.
                //
                g_eUSBState = STATE_KEYBOARD_CONNECTED;

                USBHKeyboardModifierSet(g_psKeyboardInstance, g_ui32Modifiers);

                break;
            }

            case STATE_KEYBOARD_UPDATE:
            {
                //
                // If the application detected a change that required an
                // update to be sent to the keyboard to change the modifier
                // state then call it and return to the connected state.
                //
                g_eUSBState = STATE_KEYBOARD_CONNECTED;

                USBHKeyboardModifierSet(g_psKeyboardInstance, g_ui32Modifiers);

                break;
            }

            case STATE_KEYBOARD_CONNECTED:
            {
                //
                // Nothing is currently done in the main loop when the keyboard
                // is connected.
                //
                break;
            }

            case STATE_UNKNOWN_DEVICE:
            {
                //
                // Nothing to do as the device is unknown.
                //
                break;
            }

            case STATE_NO_DEVICE:
            {
                //
                // Nothing is currently done in the main loop when the keyboard
                // is not connected.
                //
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

