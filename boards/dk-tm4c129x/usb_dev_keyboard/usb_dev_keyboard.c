//*****************************************************************************
//
// usb_dev_keyboard.c - Main routines for the keyboard example.
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
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/usb.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidkeyb.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"
#include "usb_keyb_structs.h"
#ifdef DEBUG
#include "utils/uartstdio.h"
#endif

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB HID Keyboard Device (usb_dev_keyboard)</h1>
//!
//! This example application turns the evaluation board into a USB keyboard
//! supporting the Human Interface Device class.  The color LCD display shows a
//! virtual keyboard and taps on the touchscreen will send appropriate key
//! usage codes back to the USB host. Modifier keys (Shift, Ctrl and Alt) are
//! ``sticky'' and tapping them toggles their state. The board status LED is
//! used to indicate the current Caps Lock state and is updated in response to
//! pressing the ``Caps'' key on the virtual keyboard or any other keyboard
//! attached to the same USB host system.
//!
//! The device implemented by this application also supports USB remote wakeup
//! allowing it to request the host to reactivate a suspended bus.  If the bus
//! is suspended (as indicated on the application display), touching the
//! display will request a remote wakeup assuming the host has not
//! specifically disabled such requests.
//
//*****************************************************************************

//*****************************************************************************
//
// Notes about the virtual keyboard definition
//
// The virtual keyboard is defined in terms of rows of keys.  Each row of
// keys may be either a normal alphanumeric row in which all keys are the
// same size and handled in exactly the say way, or a row of "special keys"
// which may have different widths and which have a handler function defined
// for each key.  In the definition used here, g_psKeyboard[] contains 6 rows
// and defines the keyboard at the top level.
//
// The keyboard can be in 1 of 4 states defined by the current shift and
// caps lock state.  For alphanumeric rows, the row definition (tAlphaKeys)
// contain strings representing the key cap characters for each of the keys
// in each of the four states.  Function DrawVirtualKeyboard uses these
// strings and the current state to display the correct key caps.
//
//*****************************************************************************

//*****************************************************************************
//
// Hardware resources related to the LED we use to show the CAPSLOCK state.
//
//*****************************************************************************
#define CAPSLOCK_GPIO_BASE      GPIO_PORTQ_BASE
#define CAPSLOCK_GPIO_PIN       GPIO_PIN_4
#define CAPSLOCK_ACTIVE         CAPSLOCK_GPIO_PIN
#define CAPSLOCK_INACTIVE       0

//*****************************************************************************
//
// The system tick timer period.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 100
#define SYSTICK_PERIOD_MS   (1000 / SYSTICKS_PER_SECOND)

//*****************************************************************************
//
// A structure describing special keys which are not handled the same way as
// all the alphanumeric keys.
//
//*****************************************************************************
typedef struct
{
    //
    // The label string for the key.
    //
    const char *pcLabel;

    //
    // The width of the displayed key in pixels.
    //
    int16_t i16Width;

    //
    // The usage code (if any) associated with this key.
    //
    char cUsageCode;

    //
    // A function to be called when the user presses or releases this key.
    //
    uint32_t (*pfnPressHandler)(int16_t i16Row, int16_t i16Col, bool bPress);

    //
    // A function to be called to redraw the special key. If NULL, the
    // default redraw handler is used.
    //
    void (*pfnRedrawHandler)(int16_t i16Col, int16_t i16Row, bool bFocus,
                             bool bPressed, bool bBorder);
} tSpecialKey;

//*****************************************************************************
//
// A list of the states that the keyboard can be in.
//
//*****************************************************************************
typedef enum
{
    //
    // Neither shift nor caps lock is active.
    //
    KEY_STATE_NORMAL,

    //
    // Shift is active, caps lock is not.
    //
    KEY_STATE_SHIFT,

    //
    // Shift is not active, caps lock is active.
    //
    KEY_STATE_CAPS,

    //
    // Both shift and caps lock are active.
    //
    KEY_STATE_BOTH,

    //
    // State counter member.
    //
    NUM_KEY_STATES
} tKeyState;

tKeyState g_eVirtualKeyState = KEY_STATE_NORMAL;

//*****************************************************************************
//
// A structure describing typical alphanumeric keys.
//
//*****************************************************************************
typedef struct
{
    //
    // Strings containing the unshifted, shifted and caps representations of
    // each of the keys in the row.
    //
    const char *pcKey[NUM_KEY_STATES];
    const char *pcUsageCodes;
} tAlphaKeys;

//*****************************************************************************
//
// A structure describing a single row of the virtual keyboard.
//
//*****************************************************************************
typedef struct
{
    //
    // Does this row consist of alphanumeric keys or special keys?
    //
    bool bSpecial;

    //
    // Pointer to data describing this row of keys.  If bSpecial is true,
    // this points to an array of tSpecialKey structures.  If bSpecial is
    // false, it points to a single tAlphaKeys structure.
    //
    void *pvKeys;

    //
    // The number of keys in the row.
    //
    int16_t i16NumKeys;

    //
    // The horizontal offset to apply when drawing the characters in this
    // row to the screen.  This allows us to offset the rows slightly as they
    // would look on a normal keyboard.
    //
    int16_t i16LeftOffset;
} tRow;

//*****************************************************************************
//
// Labels defining the layout of the virtual keyboard on the display.
//
//*****************************************************************************
#define NUM_KEYBOARD_ROWS       6
#define KEYBOARD_TOP            60
#define KEYBOARD_KEY_WIDTH      26
#define KEYBOARD_KEY_HEIGHT     24
#define KEYBOARD_COL_SPACING    2
#define KEYBOARD_ROW_SPACING    4

#define KEYBOARD_CELL_WIDTH     (KEYBOARD_KEY_WIDTH + KEYBOARD_COL_SPACING)
#define KEYBOARD_CELL_HEIGHT    (KEYBOARD_KEY_HEIGHT + KEYBOARD_ROW_SPACING)

//*****************************************************************************
//
// Colors used to draw various parts of the virtual keyboard.
//
//*****************************************************************************
#define FOCUS_COLOR             ClrRed
#define BACKGROUND_COLOR        ClrBlack
#define HIGHLIGHT_COLOR         ClrWhite
#define SHADOW_COLOR            ClrGray
#define KEY_COLOR               0x00E0E0E0
#define KEY_BRIGHT_COLOR        0x00E0E000
#define HIGHLIGHT_BRIGHT_COLOR  ClrYellow
#define SHADOW_BRIGHT_COLOR     0x00808000
#define KEY_TEXT_COLOR          ClrBlack

//*****************************************************************************
//
// Keys on the top row of the virtual keyboard.  Strings are defined showing
// the keycaps in unshifted, shifted and caps states.
//
//*****************************************************************************
#define NUM_ROW0_KEYS 10

const char g_pcRow0UsageCodes[NUM_ROW0_KEYS] =
{
    HID_KEYB_USAGE_1, HID_KEYB_USAGE_2, HID_KEYB_USAGE_3, HID_KEYB_USAGE_4,
    HID_KEYB_USAGE_5, HID_KEYB_USAGE_6, HID_KEYB_USAGE_7, HID_KEYB_USAGE_8,
    HID_KEYB_USAGE_9, HID_KEYB_USAGE_0
};

const tAlphaKeys g_sRow0 =
{
    {"1234567890",  // Normal
     "!@#$%^&*()",  // Shift
     "1234567890",  // Caps
     "!@#$%^&*()"}, // Shift + Caps
    g_pcRow0UsageCodes
};

//*****************************************************************************
//
// Keys on the second row of the virtual keyboard.  Strings are defined showing
// the keycaps in unshifted, shifted and caps states.
//
//*****************************************************************************
#define NUM_ROW1_KEYS 10

const char g_pcRow1UsageCodes[NUM_ROW1_KEYS] =
{
    HID_KEYB_USAGE_Q, HID_KEYB_USAGE_W, HID_KEYB_USAGE_E, HID_KEYB_USAGE_R,
    HID_KEYB_USAGE_T, HID_KEYB_USAGE_Y, HID_KEYB_USAGE_U, HID_KEYB_USAGE_I,
    HID_KEYB_USAGE_O, HID_KEYB_USAGE_P
};

const tAlphaKeys g_sRow1 =
{
    {"qwertyuiop",   // Normal
     "QWERTYUIOP",   // Shift
     "QWERTYUIOP",   // Caps
     "qwertyuiop"},  // Shift + Caps
    g_pcRow1UsageCodes
};

//*****************************************************************************
//
// Keys on the third row of the virtual keyboard.  Strings are defined showing
// the keycaps in unshifted, shifted and caps states.
//
//*****************************************************************************
#define NUM_ROW2_KEYS 10

const char g_pcRow2UsageCodes[NUM_ROW2_KEYS] =
{
    HID_KEYB_USAGE_A, HID_KEYB_USAGE_S, HID_KEYB_USAGE_D, HID_KEYB_USAGE_F,
    HID_KEYB_USAGE_G, HID_KEYB_USAGE_H, HID_KEYB_USAGE_J, HID_KEYB_USAGE_K,
    HID_KEYB_USAGE_L, HID_KEYB_USAGE_SEMICOLON
};

const tAlphaKeys g_sRow2 =
{
    {"asdfghjkl;",  // Normal
     "ASDFGHJKL:",  // Shift
     "ASDFGHJKL;",  // Caps
     "asdfghjkl;"}, // Shift + Caps
    g_pcRow2UsageCodes
};

//*****************************************************************************
//
// Keys on the fourth row of the virtual keyboard.  Strings are defined showing
// the keycaps in unshifted, shifted and caps states.
//
//*****************************************************************************
#define NUM_ROW3_KEYS 10

const char g_pcRow3UsageCodes[NUM_ROW3_KEYS] =
{
 HID_KEYB_USAGE_Z, HID_KEYB_USAGE_X, HID_KEYB_USAGE_C, HID_KEYB_USAGE_V,
 HID_KEYB_USAGE_B, HID_KEYB_USAGE_N, HID_KEYB_USAGE_M, HID_KEYB_USAGE_COMMA,
 HID_KEYB_USAGE_PERIOD, HID_KEYB_USAGE_FSLASH
};

const tAlphaKeys g_sRow3 =
{
    {"zxcvbnm,./",   // Normal
     "ZXCVBNM<>?",   // Shift
     "ZXCVBNM,./",   // Caps
     "zxcvbnm<>?"},  // Shift + Caps
    g_pcRow3UsageCodes
};

//*****************************************************************************
//
// Prototypes for special key handlers
//
//*****************************************************************************
uint32_t CapsLockHandler(int16_t i16Col, int16_t i16Row, bool bPress);
uint32_t ShiftLockHandler(int16_t i16Col, int16_t i16Row, bool bPress);
uint32_t CtrlHandler(int16_t i16Col, int16_t i16Row, bool bPress);
uint32_t AltHandler(int16_t i16Col, int16_t i16Row, bool bPress);
uint32_t GUIHandler(int16_t i16Col, int16_t i16Row, bool bPress);
uint32_t DefaultSpecialHandler(int16_t i16Col, int16_t i16Row, bool bPress);
void CapsLockRedrawHandler(int16_t i16Col, int16_t i16Row, bool bFocus,
                           bool bPressed, bool bBorder);
void ShiftLockRedrawHandler(int16_t i16Col, int16_t i16Row, bool bFocus,
                            bool bPressed, bool bBorder);
void CtrlRedrawHandler(int16_t i16Col, int16_t i16Row, bool bFocus,
                       bool bPressed, bool bBorder);
void AltRedrawHandler(int16_t i16Col, int16_t i16Row, bool bFocus,
                      bool bPressed, bool bBorder);
void GUIRedrawHandler(int16_t i16Col, int16_t i16Row, bool bFocus,
                      bool bPressed, bool bBorder);

//*****************************************************************************
//
// The bottom 2 rows of the virtual keyboard contains special keys which are
// handled differently from the basic, alphanumeric keys.
//
//*****************************************************************************
const tSpecialKey g_psRow4[] =
{
    {"Cap", 38, HID_KEYB_USAGE_CAPSLOCK, CapsLockHandler,
     CapsLockRedrawHandler},
    {"Shift", 54 , 0, ShiftLockHandler, ShiftLockRedrawHandler},
    {" ", 80, HID_KEYB_USAGE_SPACE, DefaultSpecialHandler, 0},
    {"Ent", 54, HID_KEYB_USAGE_ENTER, DefaultSpecialHandler, 0},
    {"BS", 38, HID_KEYB_USAGE_BACKSPACE, DefaultSpecialHandler, 0}
};

#define NUM_ROW4_KEYS (sizeof(g_psRow4) / sizeof(tSpecialKey))

//*****************************************************************************
//
// Keys on the fifth row of the virtual keyboard.  Strings are defined showing
// the keycaps in unshifted, shifted and caps states.  This row contains only
// cursor keys so the key caps are the same for each state.
//
//*****************************************************************************
const tSpecialKey g_psRow5[] =
{
    {"Alt", 54, 0, AltHandler, AltRedrawHandler},
    {"Ctrl", 54, 0, CtrlHandler, CtrlRedrawHandler},
    {"GUI", 36, 0, GUIHandler, GUIRedrawHandler},
    {"<", 26, HID_KEYB_USAGE_LEFT_ARROW, DefaultSpecialHandler, 0},
    {">", 26, HID_KEYB_USAGE_RIGHT_ARROW, DefaultSpecialHandler, 0},
    {"^", 26, HID_KEYB_USAGE_UP_ARROW, DefaultSpecialHandler, 0},
    {"v", 26, HID_KEYB_USAGE_DOWN_ARROW, DefaultSpecialHandler, 0},
};

#define NUM_ROW5_KEYS (sizeof(g_psRow5) / sizeof(tSpecialKey))

//*****************************************************************************
//
// Define the rows of the virtual keyboard.
//
//*****************************************************************************
const tRow g_psKeyboard[NUM_KEYBOARD_ROWS] =
{
    {false, (void *)&g_sRow0, NUM_ROW0_KEYS, 10},
    {false, (void *)&g_sRow1, NUM_ROW1_KEYS, 10 + (KEYBOARD_CELL_WIDTH / 3)},
    {false, (void *)&g_sRow2, NUM_ROW2_KEYS, 10 +
     ((2 * KEYBOARD_CELL_WIDTH) / 3)},
    {false, (void *)&g_sRow3, NUM_ROW3_KEYS, 20},
    {true, (void *)g_psRow4, NUM_ROW4_KEYS, 20},
    {true, (void *)g_psRow5, NUM_ROW5_KEYS, 20 + (KEYBOARD_CELL_WIDTH / 4)}
};

//*****************************************************************************
//
// The current active key in the virtual keyboard.
//
//*****************************************************************************
int16_t g_i16FocusRow = 0;
int16_t g_i16FocusCol = 0;

//*****************************************************************************
//
// The coordinates of the last touchscreen press.
//
//*****************************************************************************
int16_t g_i16XPress = 0;
int16_t g_i16YPress = 0;

//*****************************************************************************
//
// Flags used to indicate events requiring attention from the main loop.
//
//*****************************************************************************
uint32_t g_ui32Command = 0;

//*****************************************************************************
//
// Values ORed into g_ui32Command to indicate screen press and release events.
//
//*****************************************************************************
#define COMMAND_PRESS       0x01
#define COMMAND_RELEASE     0x02

//*****************************************************************************
//
// SysCtlDelay takes 3 clock cycles so calculate the number of loops
// per millisecond.
//
// = ((120000000 cycles/sec) / (1000 ms/sec)) / 3 cycles/loop
//
// = (120000000 / (1000 * 3)) loops
//
//*****************************************************************************
#define SYSDELAY_1_MS (120000000 / (1000 * 3))

//*****************************************************************************
//
// This global indicates whether or not we are connected to a USB host.
//
//*****************************************************************************
volatile bool g_bConnected = false;

//*****************************************************************************
//
// This global indicates whether or not the USB bus is currently in the suspend
// state.
//
//*****************************************************************************
volatile bool g_bSuspended = false;

//*****************************************************************************
//
// Global system tick counter holds elapsed time since the application started
// expressed in 100ths of a second.
//
//*****************************************************************************
volatile uint32_t g_ui32SysTickCount;

//*****************************************************************************
//
// The number of system ticks to wait for each USB packet to be sent before
// we assume the host has disconnected.  The value 50 equates to half a second.
//
//*****************************************************************************
#define MAX_SEND_DELAY          50

//*****************************************************************************
//
// This global is set to true if the host sends a request to set or clear
// any keyboard LED.
//
//*****************************************************************************
volatile bool g_bDisplayUpdateRequired;

//*****************************************************************************
//
// This global holds the current state of the keyboard LEDs as sent by the
// host.
//
//*****************************************************************************
volatile uint8_t g_ui8LEDStates;

//*****************************************************************************
//
// This global is set by the USB data handler if the host reports a change in
// the keyboard LED states.  The main loop uses it to update the virtual
// keyboard state.
//
//*****************************************************************************
volatile bool g_bLEDStateChanged;

//*****************************************************************************
//
// This enumeration holds the various states that the keyboard can be in during
// normal operation.
//
//*****************************************************************************
volatile enum
{
    //
    // Unconfigured.
    //
    STATE_UNCONFIGURED,

    //
    // No keys to send and not waiting on data.
    //
    STATE_IDLE,

    //
    // Waiting on data to be sent out.
    //
    STATE_SENDING
} g_eKeyboardState = STATE_UNCONFIGURED;

//*****************************************************************************
//
// The current state of the modifier key flags which form the first byte of
// the report to the host.  This indicates the state of the shift, control,
// alt and GUI keys on the keyboard.
//
//*****************************************************************************
static uint8_t g_ui8Modifiers = 0;

//*****************************************************************************
//
// Graphics context used to show text on the color STN display.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// Debug-related definitions and declarations.
//
// Debug output is available via UART0 if DEBUG is defined during build.
//
//*****************************************************************************
#ifdef DEBUG
//*****************************************************************************
//
// Map all debug print calls to UARTprintf in debug builds.
//
//*****************************************************************************
#define DEBUG_PRINT UARTprintf

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
void
__error__(char *pcFilename, uint32_t ui32Line)
{
    while(1)
    {
    }
}
#else

//*****************************************************************************
//
// Compile out all debug print calls in release builds.
//
//*****************************************************************************
#define DEBUG_PRINT while(0) ((int (*)(char *, ...))0)

#endif

//*****************************************************************************
//
// This function is called by the touchscreen driver whenever there is a
// change in press state or position.
//
//*****************************************************************************
static int32_t
KeyboardTouchHandler(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    switch(ui32Message)
    {
        //
        // The touchscreen has been pressed.  Remember the coordinates and
        // set the flag indicating that the main loop should process some new
        // input.
        //
        case WIDGET_MSG_PTR_DOWN:
            g_i16XPress = (int16_t)i32X;
            g_i16YPress = (int16_t)i32Y;
            g_ui32Command |= COMMAND_PRESS;
            break;

        //
        // The touchscreen is no longer being pressed.  Release any key which
        // was previously pressed.
        //
        case WIDGET_MSG_PTR_UP:
            g_ui32Command |= COMMAND_RELEASE;
            break;

        //
        // We have nothing to do on pointer move events.
        //
        case WIDGET_MSG_PTR_MOVE:
            break;
    }

    return(0);
}

//*****************************************************************************
//
// Handles asynchronous events from the HID keyboard driver.
//
// \param pvCBData is the event callback pointer provided during
// USBDHIDKeyboardInit().  This is a pointer to our keyboard device structure
// (&g_sKeyboardDevice).
// \param ui32Event identifies the event we are being called back for.
// \param ui32MsgData is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the HID keyboard driver to inform the application
// of particular asynchronous events related to operation of the keyboard HID
// device.
//
// \return Returns 0 in all cases.
//
//*****************************************************************************
uint32_t
KeyboardHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgData,
                void *pvMsgData)
{
    switch (ui32Event)
    {
        //
        // The host has connected to us and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            g_bConnected = true;
            g_bSuspended = false;
            break;
        }

        //
        // The host has disconnected from us.
        //
        case USB_EVENT_DISCONNECTED:
        {
            g_bConnected = false;
            break;
        }

        //
        // We receive this event every time the host acknowledges transmission
        // of a report. It is used here purely as a way of determining whether
        // the host is still talking to us or not.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            //
            // Enter the idle state since we finished sending something.
            //
            g_eKeyboardState = STATE_IDLE;
            break;
        }

        //
        // This event indicates that the host has suspended the USB bus.
        //
        case USB_EVENT_SUSPEND:
        {
            g_bSuspended = true;
            break;
        }

        //
        // This event signals that the host has resumed signaling on the bus.
        //
        case USB_EVENT_RESUME:
        {
            g_bSuspended = false;
            break;
        }

        //
        // This event indicates that the host has sent us an Output or
        // Feature report and that the report is now in the buffer we provided
        // on the previous USBD_HID_EVENT_GET_REPORT_BUFFER callback.
        //
        case USBD_HID_KEYB_EVENT_SET_LEDS:
        {

            //
            // Remember the new LED state.
            //
            g_ui8LEDStates = (uint8_t)(ui32MsgData & 0xFF);

            //
            // Set a flag to tell the main loop that the LED state changed.
            //
            g_bLEDStateChanged = true;

            break;
        }

        //
        // We ignore all other events.
        //
        default:
        {
            break;
        }
    }
    return (0);
}

//***************************************************************************
//
// Wait for a period of time for the state to become idle.
//
// \param ulTimeoutTick is the number of system ticks to wait before
// declaring a timeout and returning \b false.
//
// This function polls the current keyboard state for ui32TimeoutTicks system
// ticks waiting for it to become idle.  If the state becomes idle, the
// function returns true.  If it ui32TimeoutTicks occur prior to the state
// becoming idle, false is returned to indicate a timeout.
//
// \return Returns \b true on success or \b false on timeout.
//
//***************************************************************************
bool WaitForSendIdle(uint_fast32_t ui32TimeoutTicks)
{
    uint32_t ui32Start;
    uint32_t ui32Now;
    uint32_t ui32Elapsed;

    ui32Start = g_ui32SysTickCount;
    ui32Elapsed = 0;

    while (ui32Elapsed < ui32TimeoutTicks)
    {
        //
        // Is the keyboard is idle, return immediately.
        //
        if (g_eKeyboardState == STATE_IDLE)
        {
            return (true);
        }

        //
        // Determine how much time has elapsed since we started waiting.  This
        // should be safe across a wrap of g_ui32SysTickCount.  I suspect you
        // won't likely leave the app running for the 497.1 days it will take
        // for this to occur but you never know...
        //
        ui32Now = g_ui32SysTickCount;
        ui32Elapsed = (ui32Start < ui32Now) ? (ui32Now - ui32Start) :
                      (((uint32_t)0xFFFFFFFF - ui32Start) + ui32Now + 1);
    }

    //
    // If we get here, we timed out so return a bad return code to let the
    // caller know.
    //
    return (false);
}

//***************************************************************************
//
// Determine the X position on the screen for a given key in the virtual
// keyboard.
//
// \param i16Col is the column number of the key whose position is being
//  queried.
// \param i16Row is the row number of the key whose position is being queried.
//
// \return Returns the horizontal pixel coordinate of the left edge of the
// key. Note that this is 1 greater than you would expect since we allow
// space for the focus border round the character.
//
//***************************************************************************
int16_t GetVirtualKeyX(int16_t i16Col, int16_t i16Row)
{
    int16_t i16X;
    int16_t i16Count;
    tSpecialKey *psKey;

    //
    // Is this a row of special keys?
    //
    if (g_psKeyboard[i16Row].bSpecial)
    {
        //
        // Yes - we need to walk along the row of keys since the widths can
        // vary by key.
        //
        i16X = g_psKeyboard[i16Row].i16LeftOffset;
        psKey = (tSpecialKey *)(g_psKeyboard[i16Row].pvKeys);

        for (i16Count = 0; i16Count < i16Col; i16Count++)
        {
            i16X += (psKey[i16Count].i16Width + KEYBOARD_COL_SPACING);
        }

        //
        // Return the calculated X position for the key.
        //
        return (i16X + 1);
    }
    else
    {
        //
        // This is a normal alphanumeric row so the keys are all the same
        // width.
        //
        return (g_psKeyboard[i16Row].i16LeftOffset +
                (i16Col * KEYBOARD_CELL_WIDTH) + 1);
    }
}

//***************************************************************************
//
// Find a key on one row closest the a key on another row.
//
// \param i16FromCol
// \param i16FromRow
// \param i16ToRow
//
// This function is called during processing of the up and down keys while
// navigating the virtual keyboard.  It finds the key in row i16ToRow that
// sits closest to key index i16FromCol in row i16FromRow.
//
// \return Returns the index (column number) of the closest key in row
// i16ToRow.
//
//***************************************************************************
int16_t VirtualKeyboardFindClosestKey(int16_t i16FromCol, int16_t i16FromRow,
                                      int16_t i16ToRow)
{
    int16_t i16Index;
    int16_t i16X;

    //
    // If moving between 2 alphanumeric rows, just move to the same key
    // index in the new row (taking care to pass back a valid key index).
    //
    if (!g_psKeyboard[i16FromRow].bSpecial &&
        !g_psKeyboard[i16ToRow].bSpecial)
    {
        i16Index = i16FromCol;
        if (i16Index > g_psKeyboard[i16ToRow].i16NumKeys)
        {
            i16Index = g_psKeyboard[i16ToRow].i16NumKeys - 1;
        }

        return (i16Index);
    }

    //
    // Determine the x position of the key we are moving from.
    //
    i16X = GetVirtualKeyX(i16FromCol, i16FromRow);

    //
    // Check for cases where the supplied x coordinate is at or to the left of
    // any key in this row.  In this case, we always pass back index 0.
    //
    if (i16X <= g_psKeyboard[i16ToRow].i16LeftOffset)
    {
        return (0);
    }

    //
    // The x coordinate is not to the left of any key so we need to determine
    // which particular key it relates to.  The position is associated with a
    // key if it falls within the width of the key and the following space.
    //
    if (g_psKeyboard[i16ToRow].bSpecial)
    {
        //
        // This is a special key so the keys on this row can all have different
        // widths.  We walk through them looking for a hit.
        //
        for (i16Index = 1; i16Index < g_psKeyboard[i16ToRow].i16NumKeys;
             i16Index++)
        {
            //
            // If the passed coordinate is less than the leftmost position of
            // this key, we've overshot.  Drop out since we've found our
            // answer.
            //
            if (i16X < GetVirtualKeyX(i16Index, i16ToRow))
            {
                break;
            }
        }

        //
        // Return the index of the key one before the one we last looked at
        // since this is the key which contains the supplied x coordinate.
        // Since we end the loop above on the last key this handles cases
        // where the x coordinate passed is further right than any key on the
        // row.
        //
        return (i16Index - 1);
    }
    else
    {
        //
        // This is an alphanumeric row so we determine the index based on
        // the fixed character cell width.
        //
        i16Index = (i16X - g_psKeyboard[i16ToRow].i16LeftOffset) /
                   KEYBOARD_CELL_WIDTH;

        //
        // If we calculated an index higher than the number of keys on the
        // row, return the largest index supported.
        //
        if (i16Index >= g_psKeyboard[i16ToRow].i16NumKeys)
        {
            i16Index = g_psKeyboard[i16ToRow].i16NumKeys - 1;
        }
    }

    //
    // Return the column index we calculated.
    //
    return (i16Index);
}

//***************************************************************************
//
// Draw a single key of the virtual keyboard.
//
// \param i16Col contains the column number for the key to be drawn.
// \param i16Row contains the row  number for the key to be drawn.
// \param bFocus is \b true if the red focus border is to be drawn around this
//  key or \b false if the border is to be erased.
// \param bPressed is \b true of the key is to be drawn in the pressed state
//  or \b false if it is to be drawn in the released state.
// \param bBorder is \b true if the whole key is to be redrawn or \b false
//  if only the key cap text is to be redrawn.
// \param bBright is \b true if the key is to be drawn in the bright (yellow)
//  color or \b false if drawn in the normal (grey) color.
//
// This function draws a single key, varying the look depending upon whether
// the key is pressed or released and whether it has the input focus or not.
// If the bBorder parameter is false, only the key label is refreshed.  If
// true, the whole key is redrawn.
//
// This is the lowest level function used to refresh the display of both
// alphanumeric and special keys.
//
// \return None.
//
//***************************************************************************
void DrawKey(int16_t i16Col, int16_t i16Row, bool bFocus, bool bPressed,
             bool bBorder, bool bBright)
{
    tRectangle sRectOutline;
    tRectangle sFocusBorder;
    tSpecialKey *psSpecial;
    tAlphaKeys *psAlpha;
    int16_t i16X;
    int16_t i16Y;
    int16_t i16Width;
    char pcBuffer[2];
    char *pcLabel;
    uint32_t ui32Highlight;
    uint32_t ui32Shadow;

    //
    // Determine the position, width and text label for this key.
    //
    i16X = GetVirtualKeyX(i16Col, i16Row);
    i16Y = KEYBOARD_TOP + (i16Row * KEYBOARD_CELL_HEIGHT);
    if (g_psKeyboard[i16Row].bSpecial)
    {
        psSpecial = (tSpecialKey *)g_psKeyboard[i16Row].pvKeys;
        i16Width = psSpecial[i16Col].i16Width;
        pcLabel = (char *)psSpecial[i16Col].pcLabel;
    }
    else
    {
        i16Width = KEYBOARD_KEY_WIDTH;

        psAlpha = (tAlphaKeys *)g_psKeyboard[i16Row].pvKeys;
        pcBuffer[1] = (char)0;
        pcBuffer[0] = (psAlpha->pcKey[g_eVirtualKeyState])[i16Col];
        pcLabel = pcBuffer;
    }

    //
    // Determine the bounding rectangle for the key.  This rectangle is the
    // area containing the key background color and label text.  It excludes
    // the 1 line border.
    //
    sRectOutline.i16XMin = i16X + 1;
    sRectOutline.i16YMin = i16Y + 1;
    sRectOutline.i16XMax = (i16X + i16Width) - 2;
    sRectOutline.i16YMax = (i16Y + KEYBOARD_KEY_HEIGHT) - 2;

    //
    // If the key has focus, we will draw a 1 pixel red line around it
    // outside the actual key cell.  Set up the rectangle for this here.
    //
    sFocusBorder.i16XMin = i16X - 1;
    sFocusBorder.i16YMin = i16Y - 1;
    sFocusBorder.i16XMax = i16X + i16Width;
    sFocusBorder.i16YMax = i16Y + KEYBOARD_KEY_HEIGHT;

    //
    // Pick the relevant highlight and shadow colors depending upon the button
    // state.
    //
    if (!bBright)
    {
        //
        // The key is not bright so just pick the normal (grey)
        //
        ui32Highlight = bPressed ? SHADOW_COLOR : HIGHLIGHT_COLOR;
        ui32Shadow = bPressed ? HIGHLIGHT_COLOR : SHADOW_COLOR;
    }
    else
    {
        ui32Highlight = bPressed ? SHADOW_BRIGHT_COLOR :
                        HIGHLIGHT_BRIGHT_COLOR;
        ui32Shadow = bPressed ? HIGHLIGHT_BRIGHT_COLOR : SHADOW_BRIGHT_COLOR;
    }

    //
    // Are we drawing the whole key or merely updating the label?
    //
    if (bBorder)
    {
        //
        // Draw the focus border in the relevant color.
        //
        GrContextForegroundSet(&g_sContext, bFocus ? FOCUS_COLOR
                        : BACKGROUND_COLOR);
        GrRectDraw(&g_sContext, &sFocusBorder);

        //
        // Draw the key border.
        //
        GrContextForegroundSet(&g_sContext, ui32Highlight);
        GrLineDrawH(&g_sContext, i16X, i16X + i16Width - 1, i16Y);
        GrLineDrawV(&g_sContext, i16X, i16Y, i16Y + KEYBOARD_KEY_HEIGHT - 1);
        GrContextForegroundSet(&g_sContext, ui32Shadow);
        GrLineDrawH(&g_sContext, i16X + 1, i16X + i16Width - 1, i16Y
                        + KEYBOARD_KEY_HEIGHT - 1);
        GrLineDrawV(&g_sContext, i16X + i16Width - 1, i16Y + 1, i16Y
                        + KEYBOARD_KEY_HEIGHT - 1);
    }

    //
    // Fill the button with the main button color
    //
    GrContextForegroundSet(&g_sContext, bBright ? KEY_BRIGHT_COLOR :
                           KEY_COLOR);
    GrRectFill(&g_sContext, &sRectOutline);

    //
    // Update the key label.  We center the text in the key, moving it one
    // pixel down and to the right if the key is in the pressed state.
    //
    GrContextForegroundSet(&g_sContext, KEY_TEXT_COLOR);
    GrContextBackgroundSet(&g_sContext, bBright ? KEY_BRIGHT_COLOR :
                           KEY_COLOR);
    GrContextClipRegionSet(&g_sContext, &sRectOutline);
    GrStringDrawCentered(&g_sContext, pcLabel, -1, (bPressed ? 1 : 0)
                    + ((sRectOutline.i16XMax + sRectOutline.i16XMin) / 2),
                          (bPressed ? 1 : 0) + ((sRectOutline.i16YMax
                                         + sRectOutline.i16YMin) / 2), true);

    //
    // Revert to the previous clipping region.
    //
    sRectOutline.i16XMin = 0;
    sRectOutline.i16YMin = 0;
    sRectOutline.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRectOutline.i16YMax = GrContextDpyHeightGet(&g_sContext) - 1;
    GrContextClipRegionSet(&g_sContext, &sRectOutline);

    //
    // Revert to the usual background and foreground colors.
    //
    GrContextBackgroundSet(&g_sContext, BACKGROUND_COLOR);
    GrContextForegroundSet(&g_sContext, ClrWhite);
}

//***************************************************************************
//
// Call the appropriate handler to draw a single key on the virtual
// keyboard. This top level function handles both alphanumeric and special
// keys.
//
// \param i16Col contains the column number for the key to be drawn.
// \param i16Row contains the row  number for the key to be drawn.
// \param bFocus is \b true if the red focus border is to be drawn around this
//  key or \b false if the border is to be erased.
// \param bPressed is \b true of the key is to be drawn in the pressed state
//  or \b false if it is to be drawn in the released state.
// \param bBorder is \b true if the whole key is to be redrawn or \b false
//  if only the key cap text is to be redrawn.
//
// This function draws a single key on the keyboard, varying the look depending
// upon whether the key is pressed or released and whether it has the input
// focus or not.  If the bBorder parameter is \b false, only the key label is
// refreshed.  If \b true, the whole key is redrawn.
//
// If the specific key is a special key with a redraw handler set, the
// handler function is called to update the display.  If not, the basic
// DrawKey() function is used.
//
// \return None.
//
//***************************************************************************
void DrawVirtualKey(int16_t i16Col, int16_t i16Row, bool bFocus,
                    bool bPressed, bool bBorder)
{
    tSpecialKey *psSpecial;

    //
    // Get a pointer to the array of special keys for this row (even though
    // we are not yet sure if this is a special row).
    //
    psSpecial = (tSpecialKey *)g_psKeyboard[i16Row].pvKeys;

    //
    // Is this a special row and, if so, does the current key have a redraw
    // handler installed?
    //
    if (g_psKeyboard[i16Row].bSpecial && psSpecial[i16Col].pfnRedrawHandler)
    {
        //
        // Yes - call the special handler for this key.
        //
        psSpecial[i16Col].pfnRedrawHandler(i16Col, i16Row, bFocus, bPressed,
                                           bBorder);
    }
    else
    {
        //
        // The key has no redraw handler so just treat it as a normal
        // key.
        //
        DrawKey(i16Col, i16Row, bFocus, bPressed, bBorder, false);
    }
}

//***************************************************************************
//
// Draw or update the virtual keyboard on the display.
//
// \param bBorder is \b true if the whole virtual keyboard is to be drawn or
// \b false if only the key caps have to be updated.
//
// Draw the virtual keyboard on the display.  The bBorder parameter controls
// whether the whole keyboard is drawn (true) or whether only the key labels
// are replaced (false).
//
// \return None.
//
//***************************************************************************
void DrawVirtualKeyboard(bool bBorder)
{
    int16_t i16Col;
    int16_t i16Row;

    //
    // Select the font we use for the keycaps.
    //
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);

    //
    // Loop through each row, drawing each to the display
    //
    for (i16Row = 0; i16Row < NUM_KEYBOARD_ROWS; i16Row++)
    {
        //
        // Loop through each key on this row of the keyboard.
        //
        for (i16Col = 0; i16Col < g_psKeyboard[i16Row].i16NumKeys; i16Col++)
        {
            //
            // Draw a single key.
            //
            DrawVirtualKey(i16Col, i16Row, false, false, bBorder);
        }
    }
}

//****************************************************************************
//
// This function is called by the main loop if it receives a signal from the
// USB data handler telling it that the host has changed the state of the
// keyboard LEDs.  We update the state and display accordingly.
//
//****************************************************************************
void KeyboardLEDsChanged(void)
{
    bool bCapsOn;

    //
    // Clear the flag indicating a state change occurred.
    //
    g_bLEDStateChanged = false;

    //
    // Is CAPSLOCK on or off?
    //
    bCapsOn = (g_ui8LEDStates & HID_KEYB_CAPS_LOCK) ? true : false;

    //
    // Update the state to ensure that the communicated CAPSLOCK state is
    // incorporated.
    //
    switch (g_eVirtualKeyState)
    {
        //
        // Are we in an unshifted state?
        //
        case KEY_STATE_NORMAL:
        case KEY_STATE_CAPS:
        {
            if (bCapsOn)
            {
                g_eVirtualKeyState = KEY_STATE_CAPS;
            }
            else
            {
                g_eVirtualKeyState = KEY_STATE_NORMAL;
            }
            break;
        }

            //
            // Are we in a shifted state?
            //
        case KEY_STATE_SHIFT:
        case KEY_STATE_BOTH:
        {
            if (bCapsOn)
            {
                g_eVirtualKeyState = KEY_STATE_BOTH;
            }
            else
            {
                g_eVirtualKeyState = KEY_STATE_SHIFT;
            }
            break;
        }

        default:
        {
            //
            // Do nothing.  This default case merely prevents a compiler
            // warning related to the NUM_KEY_STATES enum member not having
            // a handler.
            //
            break;
        }
    }

    //
    // Redraw the virtual keyboard keycaps with the appropriate characters.
    //
    DrawVirtualKeyboard(false);

    //
    // Set the CAPSLOCK LED appropriately.
    //
    ROM_GPIOPinWrite(CAPSLOCK_GPIO_BASE, CAPSLOCK_GPIO_PIN,
    bCapsOn ? CAPSLOCK_ACTIVE : CAPSLOCK_INACTIVE);
}

//***************************************************************************
//
// Special key handler for the Caps virtual key.
//
// \param i16Col is the column number of the key which has been pressed.
// \param i16Row is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the user presses the "Select" button
// when the CapsLock key on the virtual keyboard has input focus.
//
// \returns Returns \b KEYB_SUCCESS on success or a non-zero value to
// indicate failure.
//
//***************************************************************************
uint32_t
CapsLockHandler(int16_t i16Col, int16_t i16Row, bool bPress)
{
    uint32_t ui32Retcode;

    //
    // Note that we don't set the state or redraw the keyboard here since the
    // host is expected to send us an update telling is that the CAPSLOCK
    // state changed.  We trigger the keyboard redrawing and LED setting off
    // this message instead.  In this function, we only redraw the CAPSLOCK
    // key itself to provide user feedback.
    //
    DrawKey(i16Col, i16Row, bPress ? true : false, bPress, true,
            (g_ui8LEDStates & HID_KEYB_CAPS_LOCK) ? true : false);

    //
    // Send the CAPSLOCK key code back to the host.
    //
    g_eKeyboardState = STATE_SENDING;
    ui32Retcode = USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                                g_ui8Modifiers,
                                                HID_KEYB_USAGE_CAPSLOCK,
                                                bPress);

    return (ui32Retcode);
}

//***************************************************************************
//
// Special key handler for the Ctrl virtual key.
//
// \param i16Col is the column number of the key which has been pressed.
// \param i16Row is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the user presses the "Select" button
// when the Ctrl key on the virtual keyboard has input focus.
//
// \returns Returns \b KEYB_SUCCESS on success or a non-zero value to
// indicate failure.
//
//***************************************************************************
uint32_t
CtrlHandler(int16_t i16Col, int16_t i16Row, bool bPress)
{
    uint32_t ui32Retcode;

    //
    // Ignore key release messages.
    //
    if(bPress)
    {
        //
        // Toggle the modifier bit for the left control key.
        //
        g_ui8Modifiers ^= HID_KEYB_LEFT_CTRL;

        //
        // Update the host with the new modifier state.  Sending usage code
        // HID_KEYB_USAGE_RESERVED indicates no key press so this changes only
        // the modifiers.
        //
        g_eKeyboardState = STATE_SENDING;
        ui32Retcode = USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                                    g_ui8Modifiers,
                                                    HID_KEYB_USAGE_RESERVED,
                                                    true);
    }
    else
    {
        //
        // We are ignoring key release but tell the caller that all is well.
        //
        ui32Retcode = KEYB_SUCCESS;
    }

    //
    // Redraw the key in the appropriate state.
    //
    DrawKey(i16Col, i16Row, bPress ? true : false, bPress, true,
            (g_ui8Modifiers & HID_KEYB_LEFT_CTRL) ? true : false);

    return(ui32Retcode);
}

//***************************************************************************
//
// Special key handler for the Alt virtual key.
//
// \param i16Col is the column number of the key which has been pressed.
// \param i16Row is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the user presses the "Select" button
// when the Alt key on the virtual keyboard has input focus.
//
// \returns Returns \b KEYB_SUCCESS on success or a non-zero value to
// indicate failure.
//
//***************************************************************************
uint32_t
AltHandler(int16_t i16Col, int16_t i16Row, bool bPress)
{
    uint32_t ui32Retcode;

    //
    // Ignore key release messages.
    //
    if(bPress)
    {
        //
        // Toggle the modifier bit for the left ALT key.
        //
        g_ui8Modifiers ^= HID_KEYB_LEFT_ALT;

        //
        // Update the host with the new modifier state.  Sending usage code
        // HID_KEYB_USAGE_RESERVED indicates no key press so this changes only
        // the modifiers.
        //
        g_eKeyboardState = STATE_SENDING;
        ui32Retcode = USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                                    g_ui8Modifiers,
                                                    HID_KEYB_USAGE_RESERVED,
                                                    true);
    }
    else
    {
        //
        // We are ignoring key release but tell the caller that all is well.
        //
        ui32Retcode = KEYB_SUCCESS;
    }

    //
    // Redraw the key in the appropriate state.
    //
    DrawKey(i16Col, i16Row, bPress ? true : false, bPress, true,
            (g_ui8Modifiers & HID_KEYB_LEFT_ALT) ? true : false);

    return(ui32Retcode);
}

//***************************************************************************
//
// Special key handler for the GUI virtual key.
//
// \param i16Col is the column number of the key which has been pressed.
// \param i16Row is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the user presses the "Select" button
// when the GUI key on the virtual keyboard has input focus.
//
// \returns Returns \b KEYB_SUCCESS on success or a non-zero value to
// indicate failure.
//
//***************************************************************************
uint32_t
GUIHandler(int16_t i16Col, int16_t i16Row, bool bPress)
{
    uint32_t ui32Retcode;

    //
    // Ignore key release messages.
    //
    if(bPress)
    {
        //
        // Toggle the modifier bit for the left GUI key.
        //
        g_ui8Modifiers ^= HID_KEYB_LEFT_GUI;

        //
        // Update the host with the new modifier state.  Sending usage code
        // HID_KEYB_USAGE_RESERVED indicates no key press so this changes only
        // the modifiers.
        //
        g_eKeyboardState = STATE_SENDING;
        ui32Retcode = USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                                    g_ui8Modifiers,
                                                    HID_KEYB_USAGE_RESERVED,
                                                    true);
    }
    else
    {
        //
        // We are ignoring key release but tell the caller that all is well.
        //
        ui32Retcode = KEYB_SUCCESS;
    }

    //
    // Redraw the key in the appropriate state.
    //
    DrawKey(i16Col, i16Row, bPress ? true : false, bPress, true,
            (g_ui8Modifiers & HID_KEYB_LEFT_GUI) ? true : false);

    return(ui32Retcode);
}

//***************************************************************************
//
// Special key handler for the Shift virtual key.
//
// \param i16Col is the column number of the key which has been pressed.
// \param i16Row is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the user presses the "Select" button
// when the ShiftLock key on the virtual keyboard has input focus.
//
// \returns Returns \b true on success or \b false on failure.
//
//***************************************************************************
uint32_t
ShiftLockHandler(int16_t i16Col, int16_t i16Row, bool bPress)
{
    //
    // We ignore key release for the shift lock.
    //
    if(bPress)
    {
        //
        // Set the new state by toggling the shift component.
        //
        switch(g_eVirtualKeyState)
        {
            case KEY_STATE_NORMAL:
            {
                g_eVirtualKeyState = KEY_STATE_SHIFT;
                g_ui8Modifiers |= HID_KEYB_LEFT_SHIFT;
                break;
            }

            case KEY_STATE_SHIFT:
            {
                g_eVirtualKeyState = KEY_STATE_NORMAL;
                g_ui8Modifiers &= ~HID_KEYB_LEFT_SHIFT;
                break;
            }

            case KEY_STATE_CAPS:
            {
                g_eVirtualKeyState = KEY_STATE_BOTH;
                g_ui8Modifiers |= HID_KEYB_LEFT_SHIFT;
                break;
            }

            case KEY_STATE_BOTH:
            {
                g_eVirtualKeyState = KEY_STATE_CAPS;
                g_ui8Modifiers &= ~HID_KEYB_LEFT_SHIFT;
                break;
            }

            default:
            {
                //
                // Do nothing.  This default case merely prevents a compiler
                // warning related to the NUM_KEY_STATES enum member not having
                // a handler.
                //
                break;
            }
        }

        //
        // Redraw the keycaps to show the shifted characters.
        //
        DrawVirtualKeyboard(false);
    }

    //
    // Redraw the SHIFT key in the appropriate state.
    //
    DrawKey(i16Col, i16Row, bPress ? true : false, bPress, true,
            (g_ui8Modifiers & HID_KEYB_LEFT_SHIFT) ? true : false);

    return(KEYB_SUCCESS);
}

//***************************************************************************
//
// Redraw the caps lock key. This is a thin layer over the usual DrawKey
// function which merely sets the key into bright or normal mode depending
// upon the current caps lock state.
//
//***************************************************************************
void
CapsLockRedrawHandler(int16_t i16Col, int16_t i16Row, bool bFocus,
                      bool bPressed, bool bBorder)
{
    //
    // Draw the key in either normal color if the CAPS lock is not active
    // or in the bright color if it is.
    //
    DrawKey(i16Col, i16Row, bFocus, bPressed, bBorder,
            ((g_eVirtualKeyState == KEY_STATE_BOTH) ||
             (g_eVirtualKeyState == KEY_STATE_CAPS)) ? true : false);
}

//***************************************************************************
//
// Redraw the Shift lock key. This is a thin layer over the usual DrawKey
// function which merely sets the key into bright or normal mode depending
// upon the current shift state.
//
//***************************************************************************
void
ShiftLockRedrawHandler(int16_t i16Col, int16_t i16Row, bool bFocus,
                       bool bPressed, bool bBorder)
{
    //
    // Draw the key in either normal color if the shift lock is not active
    // or in the bright color if it is.
    //
    DrawKey(i16Col, i16Row, bFocus, bPressed, bBorder,
            (g_ui8Modifiers & HID_KEYB_LEFT_SHIFT) ? true : false);
}

//***************************************************************************
//
// Redraw the Ctrl sticky key. This is a thin layer over the usual DrawKey
// function which merely sets the key into bright or normal mode depending
// upon the current key state.
//
//***************************************************************************
void
CtrlRedrawHandler(int16_t i16Col, int16_t i16Row, bool bFocus,
                  bool bPressed, bool bBorder)
{
    //
    // Draw the key in either normal color if CTRL is not active
    // or in the bright color if it is.
    //
    DrawKey(i16Col, i16Row, bFocus, bPressed, bBorder,
            (g_ui8Modifiers & HID_KEYB_LEFT_CTRL) ? true : false);
}

//***************************************************************************
//
// Redraw the Alt sticky key. This is a thin layer over the usual DrawKey
// function which merely sets the key into bright or normal mode depending
// upon the current key state.
//
//***************************************************************************
void
AltRedrawHandler(int16_t i16Col, int16_t i16Row, bool bFocus,
                 bool bPressed, bool bBorder)
{
    //
    // Draw the key in either normal color if CTRL is not active
    // or in the bright color if it is.
    //
    DrawKey(i16Col, i16Row, bFocus, bPressed, bBorder,
            (g_ui8Modifiers & HID_KEYB_LEFT_ALT) ? true : false);
}

//***************************************************************************
//
// Redraw the GUI sticky key. This is a thin layer over the usual DrawKey
// function which merely sets the key into bright or normal mode depending
// upon the current key state.
//
//***************************************************************************
void
GUIRedrawHandler(int16_t i16Col, int16_t i16Row, bool bFocus,
                 bool bPressed, bool bBorder)
{
    //
    // Draw the key in either normal color if CTRL is not active
    // or in the bright color if it is.
    //
    DrawKey(i16Col, i16Row, bFocus, bPressed, bBorder,
           (g_ui8Modifiers & HID_KEYB_LEFT_GUI) ? true : false);
}

//***************************************************************************
//
// Special key handler for the space, enter, backspace and cursor control
// virtual keys.
//
// \param i16Col is the column number of the key which has been pressed.
// \param i16Row is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the user presses the "Select" button
// when the space, backspace, enter or cursor control keys on the virtual
// keyboard have input focus.  These keys are like any other alpha key in that
// they merely send a single usage code back to the host.  We need a special
// handler for them, however, since they are on the bottom row of the virtual
// keyboard and this row contains other special keys.
//
// \returns Returns \b true on success or \b false on failure.
//
//***************************************************************************
uint32_t
DefaultSpecialHandler(int16_t i16Col, int16_t i16Row, bool bPress)
{
    tSpecialKey *psKey;
    uint32_t ui32Retcode;

    //
    // Get a pointer to the array of keys for this row.
    //
    psKey = (tSpecialKey *)g_psKeyboard[i16Row].pvKeys;

    //
    // Send the usage code for this key back to the USB host.
    //
    g_eKeyboardState = STATE_SENDING;
    ui32Retcode = USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                                g_ui8Modifiers,
                                                psKey[i16Col].cUsageCode,
                                                bPress);

    //
    // Redraw the key in the appropriate state.
    //
    DrawKey(i16Col, i16Row, bPress ? true : false, bPress, true, false);

    return(ui32Retcode);
}

//*****************************************************************************
//
// Processes a single key press on the virtual keyboard.
//
// \param i16Col is the column number of the key which has been pressed.
// \param i16Row is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the "Select" button is pressed or released.
// Depending upon the specific key, this will either call a special key handler
// function or send a report back to the USB host indicating the change of
// state.
//
// \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
bool
VirtualKeyboardKeyPress(int16_t i16Col, int16_t i16Row, bool bPress)
{
    tSpecialKey *psKey;
    tAlphaKeys *psAlphaKeys;
    uint32_t ui32Retcode;
    bool bSuccess;

    //
    // Are we dealing with a special key?
    //
    if(g_psKeyboard[i16Row].bSpecial)
    {
        //
        // Yes - call the handler for this special key.
        //
        psKey = (tSpecialKey *)g_psKeyboard[i16Row].pvKeys;
        ui32Retcode = psKey[i16Col].pfnPressHandler(i16Col, i16Row, bPress);

        DEBUG_PRINT("Key \"%s\" %s\n", psKey[i16Col].pcLabel,
                    (bPress ? "pressed" : "released"));
    }
    else
    {
        //
        // Normal key - add or remove this key from the list of keys currently
        // pressed and pass the latest report back to the host.
        //
        psAlphaKeys = (tAlphaKeys *)g_psKeyboard[i16Row].pvKeys;
        g_eKeyboardState = STATE_SENDING;
        ui32Retcode = USBDHIDKeyboardKeyStateChange(
                                           (void *)&g_sKeyboardDevice,
                                           g_ui8Modifiers,
                                           psAlphaKeys->pcUsageCodes[i16Col],
                                           bPress);
        DEBUG_PRINT("Key \"%c\" %s\n",
                    psAlphaKeys->pcKey[g_eVirtualKeyState][i16Col],
                    (bPress ? "pressed" : "released"));

        //
        // Redraw the key in the appropriate state.
        //
        DrawKey(i16Col, i16Row, bPress ? true : false, bPress, true, false);
    }

    //
    // Did we schedule the report for transmission?
    //
    if(ui32Retcode == KEYB_SUCCESS)
    {
        //
        // Wait for the host to acknowledge the transmission if all went well.
        //
        bSuccess = WaitForSendIdle(MAX_SEND_DELAY);

        //
        // Did we time out waiting for the packet to be sent?
        //
        if (!bSuccess)
        {
            //
            // Yes - assume the host disconnected and go back to
            // waiting for a new connection.
            //
            g_bConnected = 0;
        }
    }
    else
    {
        //
        // An error was reported when trying to send the character.
        //
        bSuccess = false;
    }
    return(bSuccess);
}

//*****************************************************************************
//
// Map a screen coordinate to the column and row of a virtual key.
//
// \param i16X is the screen X coordinate that is to be mapped.
// \param i16Y is the screen Y coordinate that is to be mapped.
// \param pusCol is a pointer to the variable which will be written with the
// column number of the virtual key at screen position (i16X, i16Y).
// \param pusRow is a pointer to the variable which will be written with the
// row number of the virtual key at screen position (i16X, i16Y).
//
// \return Returns \b true if a virtual key exists at the position provided or
// \b false otherwise.  If \b false is returned, pointers \e pusCol and \e
// pusRow will not be written.
//
//*****************************************************************************
static bool
FindVirtualKey(int16_t i16X, int16_t i16Y, int16_t *psCol, int16_t *psRow)
{
    uint32_t ui32Row, ui32Col, ui32NumKeys;
    int16_t i16KeyX, i16KeyWidth;
    tSpecialKey *psKey;

    //
    // Initialize the column value.
    //
    ui32Col = 0;

    //
    // Determine which row the coordinates occur in.
    //
    for(ui32Row = 0; ui32Row < NUM_KEYBOARD_ROWS; ui32Row++)
    {
        if((i16Y > (KEYBOARD_TOP + (ui32Row * KEYBOARD_CELL_HEIGHT))) &&
           (i16Y < (KEYBOARD_TOP + (ui32Row * KEYBOARD_CELL_HEIGHT) +
            KEYBOARD_KEY_HEIGHT)))
        {
            //
            // If this is a standard alphanumeric row, we can determine the
            // mapping arithmetically since all the keys are the same width.
            if(!g_psKeyboard[ui32Row].bSpecial)
            {
                //
                // First check to make sure that the press is not to the left
                // of the first key in the row.
                //
                if(i16X < g_psKeyboard[ui32Row].i16LeftOffset)
                {
                    return(false);
                }

                //
                // This includes presses that occur in the space between
                // keys but, given that the touchscreen is not hugely accurate
                // and that fingers or styli will likely cover more than a
                // couple of pixels, this is probably perfectly fine.
                //
                ui32Col = ((i16X - g_psKeyboard[ui32Row].i16LeftOffset) /
                         KEYBOARD_CELL_WIDTH);

                //
                // If we calculated an out of range column, this means no key
                // exists under the press position so return false to indicate
                // this.
                //
                if(ui32Col >= g_psKeyboard[ui32Row].i16NumKeys)
                {
                    return(false);
                }
            }
            else
            {
                //
                // The touch is somewhere within this row of keys.  How many keys
                // are in this row?
                //
                ui32NumKeys = g_psKeyboard[ui32Row].i16NumKeys;
                i16KeyX = g_psKeyboard[ui32Row].i16LeftOffset;

                //
                // Walk through the keys in this row.
                //
                for(ui32Col = 0; ui32Col < ui32NumKeys; ui32Col++)
                {
                    i16KeyX = GetVirtualKeyX(ui32Col, ui32Row);
                    psKey = (tSpecialKey *)(g_psKeyboard[ui32Row].pvKeys);
                    i16KeyWidth = psKey[ui32Col].i16Width + KEYBOARD_COL_SPACING;

                    if((i16X >= i16KeyX) && (i16X < (i16KeyX + i16KeyWidth)))
                    {
                        //
                        // We found a matching key so drop out of the loop.
                        //
                        break;
                    }
                }

                //
                // If we get here and ui32Col has reached ui32NumKeys, we didn't
                // find a key under the press position.
                //
                if(ui32Col == ui32NumKeys)
                {
                    return(false);
                }
            }
            break;
        }
    }

    //
    // If we end up here and the row number is equal to the number of rows in
    // the keyboard, the press was not in any keyboard row so return false.
    //
    if(ui32Row == NUM_KEYBOARD_ROWS)
    {
        return(false);
    }

    //
    // At this point, we found a key beneath the press so we return the
    // information to the caller.
    //
    *psCol = (int16_t)ui32Col;
    *psRow = (int16_t)ui32Row;
    return(true);
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int
main(void)
{
    tRectangle sRect;
    int32_t int32CenterX;
    uint32_t ui32LastTickCount, ui32Processing;
    bool bRetcode, bLastSuspend, bKeyPressed;
    uint32_t ui32SysClock;

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
    FrameDraw(&g_sContext, "usb-dev-keyboard");


    //
    // Configure GPIO pin which controls the CAPSLOCK LED and turn it off
    // initially.  Note that PinoutSet() already enabled the GPIO peripheral
    // containing this pin
    //
    ROM_GPIOPinTypeGPIOOutput(CAPSLOCK_GPIO_BASE, CAPSLOCK_GPIO_PIN);
    ROM_GPIOPinWrite(CAPSLOCK_GPIO_BASE, CAPSLOCK_GPIO_PIN, CAPSLOCK_INACTIVE);

#ifdef DEBUG
    //
    // Open UART0 for debug output.
    //
    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, ui32SysClock);
#endif

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(ui32SysClock);

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(KeyboardTouchHandler);

    //
    // Set the system tick to fire 100 times per second.
    //
    ROM_SysTickPeriodSet(ui32SysClock / SYSTICKS_PER_SECOND);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //
    // Not configured initially.
    //
    g_bConnected = false;
    g_bSuspended = false;
    bLastSuspend = false;

    //
    // Initialize the USB stack for device mode.
    //
    USBStackModeSet(0, eUSBModeDevice, 0);

    //
    // Pass our device information to the USB HID device class driver,
    // initialize the USB
    // controller and connect the device to the bus.
    //
    USBDHIDKeyboardInit(0, &g_sKeyboardDevice);

    //
    // find the middle X coordinate.
    //
    int32CenterX = GrContextDpyWidthGet(&g_sContext) / 2;

    //
    // The main loop starts here.  We begin by waiting for a host connection
    // then drop into the main keyboard handling section.  If the host
    // disconnects, we return to the top and wait for a new connection.
    //
    while(1)
    {
        //
        // Fill all but the top 24 rows of the screen with black to erase the
        // keyboard.
        //
        sRect.i16XMin = 10;
        sRect.i16YMin = 24;
        sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 10;
        sRect.i16YMax = GrContextDpyHeightGet(&g_sContext) - 10;
        GrContextForegroundSet(&g_sContext, ClrBlack);
        GrRectFill(&g_sContext, &sRect);

        //
        // Tell the user what we are doing and provide some basic instructions.
        //
        GrContextFontSet(&g_sContext, g_psFontCmss20b);
        GrContextForegroundSet(&g_sContext, ClrWhite);
        GrStringDrawCentered(&g_sContext, " Waiting for host... ", -1,
                        int32CenterX, 40, true);
        GrContextFontSet(&g_sContext, g_psFontFixed6x8);
        DEBUG_PRINT("Waiting for host connection...\n");

        //
        // Wait for USB configuration to complete. Even in this state, we look
        // for key presses and, if any occur while the bus is suspended, we
        // issue a remote wakeup request.
        //
        while(!g_bConnected)
        {
            //
            // Remember the current time.
            //
            ui32LastTickCount = g_ui32SysTickCount;

            //
            // Has the suspend state changed since last time we checked?
            //
            if(bLastSuspend != g_bSuspended)
            {
                //
                // Yes - the state changed so update the display.
                //
                bLastSuspend = g_bSuspended;
                GrContextFontSet(&g_sContext, g_psFontCmss20b);
                GrStringDrawCentered(&g_sContext,
                                     (bLastSuspend ? "   Bus suspended...   ":
                                                     " Waiting for host... "),
                                     -1, int32CenterX, 40, true);
                DEBUG_PRINT(bLastSuspend ? "Bus suspended.\n" :
                            "Bus resumed.\n");

            }

            //
            // Wait for at least 1 system tick to have gone by before we poll
            // the buttons again.
            //
            while(g_ui32SysTickCount == ui32LastTickCount)
            {
                //
                // Hang around doing nothing.
                //
            }
        }

        //
        // Update the status.
        //
        GrContextFontSet(&g_sContext, g_psFontCmss20b);
        GrStringDrawCentered(&g_sContext, " Host connected... ", -1,
                        int32CenterX, 40, true);
        DEBUG_PRINT("Host connected.\n");

        //
        // Enter the idle state.
        //
        g_eKeyboardState = STATE_IDLE;

        //
        // Draw the keyboard on the display.
        //
        DrawVirtualKeyboard(true);

        //
        // Assume that the bus is not currently suspended if we have just been
        // configured.
        //
        bLastSuspend = false;

        //
        // Start with the assumption that no keys are pressed.
        //
        bKeyPressed = false;

        //
        // Keep transfering characters from the UART to the USB host for as
        // long as we are connected to the host.
        //
        while(g_bConnected)
        {
            //
            // Remember the current time.
            //
            ui32LastTickCount = g_ui32SysTickCount;

            //
            // Has the suspend state changed since last time we checked?
            //
            if(bLastSuspend != g_bSuspended)
            {
                //
                // Yes - the state changed so update the display.
                //
                bLastSuspend = g_bSuspended;
                GrContextFontSet(&g_sContext, g_psFontCmss20b);
                GrStringDrawCentered(&g_sContext,
                                     (bLastSuspend ? " Bus suspended...  ":
                                                     " Host connected... "),
                                     -1, int32CenterX, 40, true);
                DEBUG_PRINT(bLastSuspend ? "Bus suspended.\n" :
                            "Bus resumed.\n");
            }

            //
            // Do we have any touchscreen input to process?
            //
            if(g_ui32Command)
            {
                //
                // Take a snapshot of the commands we were sent then clear
                // the global command flags.
                //
                ui32Processing = g_ui32Command;
                g_ui32Command = 0;

                //
                // Is the bus currently suspended?
                //
                if(g_bSuspended)
                {
                    //
                    // We are suspended so request a remote wakeup.
                    //
                    USBDHIDKeyboardRemoteWakeupRequest(
                                                   (void *)&g_sKeyboardDevice);
                }

                //
                // Process the command unless we got simultaneous press and
                // release commands in which case we ignore them.
                //
                if(!((ui32Processing & (COMMAND_PRESS | COMMAND_RELEASE)) ==
                     (COMMAND_PRESS | COMMAND_RELEASE)))
                {
                    //
                    // Was the touchscreen pressed?
                    //
                    if(ui32Processing & COMMAND_PRESS)
                    {
                        //
                        // Map the touchscreen press to an actual key in the
                        // virtual keyboard.
                        //
                        bRetcode = FindVirtualKey(g_i16XPress, g_i16YPress,
                                                 &g_i16FocusCol, &g_i16FocusRow);
                        if(!bRetcode)
                        {
                            //
                            // The press was outside any key on the virtual
                            // keyboard so just go back and wait for something
                            // else to happen.
                            //
                            continue;
                        }

                        //
                        // A key is pressed.
                        //
                        bKeyPressed = true;
                    }

                    //
                    // Pass information on the press or release to the host,
                    // making sure we only send a message if we really saw a
                    // change of state.
                    //
                    if(bKeyPressed)
                    {
                        bRetcode = VirtualKeyboardKeyPress(g_i16FocusCol, g_i16FocusRow,
                                           ((ui32Processing == COMMAND_PRESS) ?
                                           true : false));
                    }
                    else
                    {
                        bRetcode = true;
                    }

                    //
                    // Remember that no key is currently pressed.
                    //
                    if(ui32Processing & COMMAND_RELEASE)
                    {
                        bKeyPressed = false;
                    }

                    //
                    // If the key press generated an error, this likely
                    // indicates that the host has disconnected so drop out of
                    // the loop and go back to looking for a new connection.
                    //
                    if(!bRetcode)
                    {
                        break;
                    }
                }
            }

            //
            // Update the state if the host set the LEDs since we last looked.
            //
            if(g_bLEDStateChanged)
            {
                KeyboardLEDsChanged();
            }

            //
            // Wait for at least 1 system tick to have gone by before we poll
            // the buttons again.
            //
            while(g_ui32SysTickCount == ui32LastTickCount)
            {
                //
                // Hang around doing nothing.
                //
            }
        }

        //
        // Dropping out of the previous loop indicates that the host has
        // disconnected so go back and wait for reconnection.
        //
        DEBUG_PRINT("Host disconnected.\n");
    }
}

//*****************************************************************************
//
// This is the interrupt handler for the SysTick interrupt.  It is used to
// update our local tick count which, in turn, is used to check for transmit
// timeouts.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    g_ui32SysTickCount++;
}
