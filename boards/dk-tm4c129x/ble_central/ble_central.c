//*****************************************************************************
//
// ble_central.c - Demonstration of BLE central device.
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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/pin_map.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/frame.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "hci.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>BLE Central Device Demonstration (ble_central)</h1>
//!
//! This application provides a demonstration use of Bluetooth Low Energy
//! central device by utilizing TI’s BLE CC2540 Evaluation Module and
//! SesorTags.
//!
//! By connecting a CC2540 EM board to the EM header on the TM4C129X development
//! board, the TM4C129X can communicate with the CC2540 by means of
//! vendor-specific HCI commands using the UART interface. This application can
//! discover up to three SensorTag devices, it can connect to any one of them,
//! perform pairing and bonding, read some sensor data and RSSI data from the
//! slave, and display the information on the LCD display.
//!
//! This application can discover any BLE device although it may not be
//! able to make a connection to a device other than a SensorTag/CC2540 as the
//! bonding process will likely fail due to the use of a default passcode.
//! We have tested this application with SensorTag and CC2540 Development
//! boards (SmartRF05EB + CC2540 EM) with the SimpleBLEPeripheral
//! sample application programmed. It can successfully bond with both boards
//! since the same default passcode is expected. Both SensorTag and CC2540
//! devices have BLE Stack 1.4.0 release code programmed.
//!
//! CC2540 device should be programmed with the HostTestRelease
//! (Network Processor) application. A hex file containing the this application
//! can be found in the BLE stack 1.4.0 release under
//! C:\\...\\BLE-CC254x-1.4.0\\Accessories\\HexFiles\\
//! CC2540_SmartRF_HostTestRelease_All.hex. Please refer to the Bluetooth Low
//! Energy CC2540 Development Kit User's Guide for information on how to load
//! the hex file to the CC2540. This User's Guide can be found in
//! http://www.ti.com/lit/ug/swru301a/swru301a.pdf
//!
//! On the TM4C129X development board, make sure that jumpers PJ0 and PJ1
//! are connected to the "EM_UART" side which allows UART3 TX and RX signals to
//! be routed to the EM header. UART3 is used as the communication channel
//! between the CC2540 device and the TM4C129X device.
//!
//! Once the application starts, it will verify the serial connection by
//! sending the CC2540 device a vendor-specific HCI command, and waiting for the
//! expected responses within a short time period. Once the physical connection
//! between TM4C129X and CC2540 is verified, the device will automatically
//! start to discover BLE peripheral device. If no devices are found within 20
//! seconds, the application will timeout and display "No Device Found",
//! otherwise the discovered device names will be shown on the display.
//! Touching any of the device names will start the process of establishing a
//! connection with that device.  This sample application always tries to make
//! a secure connection by pairing the device with default passcode "00000".
//! Upon successfully linking and pairing, application will start querying
//! sensor data, including IR temperature, ambient temperature, humidity and
//! RSSI.
//!
//! Whe run inside, IR and ambient temperature should typically be in the low
//! 20s (Celsius).  You can place the SensorTag near a hot object (such as a
//! cup of coffee) to verify that the IR temperature increases. You can move
//! the SensorTag further from the TM4C129X development board to verify that
//! its RSSI reading will decrease.
//!
//! At any time after the connection is established, you can touch the
//! "disconnect" button on the bottom of the screen to terminate the
//! connection with the peripheral device.
//!
//! In order to make the SensorTag discoverable by a central device, the
//! SensorTag needs to be in the discovery mode. The LED in the middle of the
//! board will blink periodically if the SensorTag is in discovery mode. If the
//! LED is not blinking, pressing the side button on the SensorTag should
//! place it in discovery mode.  Once it is connected to a central device,
//! the LED should be off, pressing the side button while it is connected will
//! terminate the connection and put the SensorTag in discovery mode again.
//! For more information on SensorTag, please visit
//! http://processors.wiki.ti.com/index.php/Bluetooth_SensorTag
//!
//! Every HCI command and event are output to the UART console for
//! debugging purpose. The UART terminal should be configured in 115,200 baud,
//! 8-n-1 mode.
//!
//
//*****************************************************************************


//*****************************************************************************
//
// A set of flags.  The flag bits are defined as follows:
//
//     0 -> An indicator that a second has occurred.
//     1 -> A complete RX Packet has been received.
//     2 -> Whether to draw a circle or not on the display.
//     3 -> Sensors on the SensorTag are configured or not.
//
//*****************************************************************************
#define FLAG_EVERY_SECOND            0
#define FLAG_HCI_MSG_COMPLETE        1
#define FLAG_DRAW_CIRCLE             2
#define FLAG_SENSOR_CFGD             3
static volatile uint32_t g_ui32Flags;

//*****************************************************************************
//
// A system tick counter, incremented every SYSTICKMS.
//
//*****************************************************************************
volatile uint32_t g_ui32TickCounter = 0;

//*****************************************************************************
//
// The delay count for timeout. It decrements to 1 to indicate timeout. Setting
// it to 0 means no timeout has been set.
//
//*****************************************************************************
volatile uint32_t g_ui32Delay;

//*****************************************************************************
//
// The application's graphics context.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// Flag that informs that the user has requested an action via GUI.
//
//*****************************************************************************
static volatile bool g_bDiscoveryReq = false;
static volatile bool g_bEstLinkReq   = false;
static volatile bool g_bTermLinkReq  = false;

//*****************************************************************************
//
// Flag to enable pairing, change to false to diable the pairing.
//
//*****************************************************************************
static bool g_bInitPairReq = true;

//*****************************************************************************
//
// The width and height of the LCD display
//
//*****************************************************************************
uint32_t g_ui32Width, g_ui32Height;

//*****************************************************************************
//
// The screen offset of the upper left hand corner where we start to draw.
//
//*****************************************************************************
#define X_OFFSET                8
#define Y_OFFSET                24


//*****************************************************************************
//
// The maximum number of slaves that we can discover.
//
//*****************************************************************************
#define MAX_SLAVE_NUM           3

//*****************************************************************************
//
// The RX receive circular buffer size.
//
//*****************************************************************************
#define BUF_SIZE                128


//*****************************************************************************
//
// The circular buffer used to store the received HCI message from the CC2540.
//
//*****************************************************************************
typedef struct
{
    volatile uint8_t pui8RXBuf[BUF_SIZE];
    volatile uint8_t ui8Rd;
    volatile uint8_t ui8Wr;
    volatile uint8_t ui8Count;
} tCirBuf;

tCirBuf g_sRxBuf;

//*****************************************************************************
//
// The state defination of BLE statemachine.
//
//*****************************************************************************
volatile enum
{
    STATE_DEV_INIT,
    STATE_GET_PARAM,
    STATE_START_DISCOVERY,
    STATE_SET_PARAM,
    STATE_READY_FOR_LINK_REQ,
    STATE_LINK,
    STATE_LINKED,
    STATE_SEND_PASSKEY,
    STATE_IDLE,
    STATE_TERM,
    STATE_TERMED,
    STATE_ERROR,
}g_iState = STATE_IDLE;

//*****************************************************************************
//
// The paramers of the central device.
//
//*****************************************************************************
uint16_t g_ui16Param[4];
uint8_t  g_ui8ParamWrIdx = 0;

//*****************************************************************************
//
// The complete HCI message and its length that we have received from BLE
// stack.
//
//*****************************************************************************
uint8_t  g_pui8Msg[200];
uint8_t  g_ui8MsgLen;

//*****************************************************************************
//
// Received Signal Strength Indication(RSSI).
//
//*****************************************************************************
int8_t   g_i8RSSI = 0;

//*****************************************************************************
//
// Sensor data, IR temperature, humidity etc. Only available on SensorTag
//
//*****************************************************************************
uint8_t g_pui8IRTemp[4]; // RAW data
uint8_t g_pui8Humidity[4]; // RAW data
double  g_dIRTemp = 0;
double  g_dAmbTemp = 0;
double  g_dHumidity = 0;

//*****************************************************************************
//
// The HCI message information, used for verifying expected response from
// stack.
//
//*****************************************************************************
uint16_t g_ui16CmdStatusOpcode;
uint16_t g_ui16Event;
uint16_t g_ui16Handle;

//*****************************************************************************
//
// The structure of BLE slave information.
//
//*****************************************************************************
typedef struct
{
    //
    // Device Address
    //
    uint8_t  pui8Addr[HCI_BDADDR_LEN];
    //
    // Address Type
    //
    uint8_t  ui8AddrType;
    //
    // Device Name
    //
    char     pcName[32];
    //
    // Long Term Key Data, used for bonding.
    //
    tLTKData sSaveKey;
} tBLEDeviceInfo;

tBLEDeviceInfo g_psDev[MAX_SLAVE_NUM];

//*****************************************************************************
//
// The number of devices discovered.
//
//*****************************************************************************
uint8_t        g_ui8DevFound;

//*****************************************************************************
//
// The index of device to be connected.
//
//*****************************************************************************
uint8_t        g_ui8DevConnect;

//*****************************************************************************
//
// The positions of the circles in the animation used while discovering devices
//
//*****************************************************************************
const int32_t g_ppi32CirclePos[][2] =
{
    {
        12, 0
    },
    {
        8, -9
    },
    {
        0, -12
    },
    {
        -8, -9
    },
    {
        -12, 0
    },
    {
        -8, 9
    },
    {
        0, 12
    },
    {
        8, 9
    }
};

//*****************************************************************************
//
// The colors of the circles in the animation used while discovering devices
//
//*****************************************************************************
const uint32_t g_pui32CircleColor[] =
{
    0x111111,
    0x333333,
    0x555555,
    0x777777,
    0x999999,
    0xbbbbbb,
    0xdddddd,
    0xffffff,
};

//*****************************************************************************
//
// The current color index for the animation used while discovering devices
//
//*****************************************************************************
uint32_t g_ui32ColorIdx;

//*****************************************************************************
//
// The strings that are displayed in the center and bottom of the display
//
//*****************************************************************************
typedef enum
{
    iInitializing = 0,
    iScanning,
    iScan,
    iNoBLE,
    iConnect,
    iConnecting,
    iDisconnect,
    iDisconnecting
} eDisplayUpdateIdx;

//
// eDisplayUpdateIdx is used as index to the following table
//
static char *ppcString[][2] =
{   // Middle of screen, Bottom of screen
    {"Initializing",          0},
    {"Discovering",           "timeout in 20s"},
    {"No Device Found",       "scan"},
    {"CC2540 EM not present", 0},
    {0,                       "scan again"},
    {0,                       "connecting" },
    {0,                       "disconnect" },
    {0,                       "disconnecting"},
};

//*****************************************************************************
//
// Forward reference to local functions.
//
//*****************************************************************************
void DrawCircle(void);
bool MessageComplete(uint8_t *pui8Buf, uint8_t *pui8Len);
void HandleTemp(void);
void HandleHumidity(void);
void DisplayTemp(uint32_t ui32X, uint32_t ui32Y);
void DisplayHumidity(uint32_t ui32X, uint32_t ui32Y);
void DisplayRSSI(uint32_t ui32X, uint32_t ui32Y);

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
// The interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Increment the system tick count.
    //
    g_ui32TickCounter++;

    //
    // After the current message has been processed,
    // check the message buffer to see if we have received another complete
    // message.
    //
    if(HWREGBITW(&g_ui32Flags, FLAG_HCI_MSG_COMPLETE) == 0)
    {
        if(MessageComplete(g_pui8Msg, &g_ui8MsgLen) == true)
        {
            HWREGBITW(&g_ui32Flags, FLAG_HCI_MSG_COMPLETE) = 1;
        }
    }

    //
    // Decrement g_ui32Delay to 1, if it is 1 already,
    // meaning it has timed out.
    //
    if(g_ui32Delay > 1)
    {
        g_ui32Delay--;
    }

    //
    // Draw a circle every 100ms
    //
    if(HWREGBITW(&g_ui32Flags, FLAG_DRAW_CIRCLE) == 1)
    {
        if(g_ui32TickCounter % 10 == 0)
        {
           DrawCircle();
        }
    }

    //
    // set FLAG_EVERY_SECOND every second.
    // Systick interrupt is every 10ms, so a second is every 100 interrupts.
    //
    if(g_ui32TickCounter % 100 == 0)
    {
        HWREGBITW(&g_ui32Flags, FLAG_EVERY_SECOND) = 1;
    }
}

//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************
void
UART3IntHandler(void)
{
    uint32_t ui32Status;

    //
    // Get the interrrupt status.
    //
    ui32Status = ROM_UARTIntStatus(UART3_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    ROM_UARTIntClear(UART3_BASE, ui32Status);

    //
    // Loop while there are characters in the receive FIFO.
    //
    while(ROM_UARTCharsAvail(UART3_BASE))
    {
        //
        // Check for buffer overflow case, this shouldn't happen
        //
        if((g_sRxBuf.ui8Wr == g_sRxBuf.ui8Rd) && g_sRxBuf.ui8Count)
        {
            UARTprintf("\nOF!!! Wr %d, Rd %d, Count %d\n", g_sRxBuf.ui8Wr, g_sRxBuf.ui8Rd, g_sRxBuf.ui8Count);
        }

        //
        // Read the next character from the UART and place it in the RX buffer.
        //
        g_sRxBuf.pui8RXBuf[g_sRxBuf.ui8Wr++] =  UARTCharGetNonBlocking(UART3_BASE);

        //
        // Check for the RX buffer wrap.
        //
        if(g_sRxBuf.ui8Wr >= BUF_SIZE)
        {
            g_sRxBuf.ui8Wr = 0;
        }

        //
        // Increment the total count.
        //
        g_sRxBuf.ui8Count++;

    }
}

//*****************************************************************************
//
// The touch screen driver calls this function to report all state changes.
//
//*****************************************************************************
static int32_t
TouchCallback(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    uint8_t ui8Loop;

    if(ui32Message == WIDGET_MSG_PTR_UP)
    {
        //
        // Check if the bottom of the screen is touched.
        //
        if(i32Y >= (200 - 8) && i32Y < (200 + 8))
        {
            if( (g_iState == STATE_READY_FOR_LINK_REQ) ||
                (g_iState == STATE_START_DISCOVERY))
            {
                g_bDiscoveryReq = true;
            }
            else if(g_iState == STATE_LINKED)
            {
                g_bTermLinkReq = true;
            }
        }


        //
        // Check if any of three device names is touched when it is ready
        // to connect
        //
        if(g_iState == STATE_READY_FOR_LINK_REQ)
        {
            for(ui8Loop = 0; ui8Loop < g_ui8DevFound; ui8Loop++)
            {
                if((g_ui8DevFound >= ui8Loop + 1) &&
                   (i32Y >= (60 + 40*ui8Loop - 20)) &&
                   (i32Y <  (60 + 40*ui8Loop + 20)))
                {
                    //
                    // save the device index and
                    // set flag to connect the device.
                    //
                    g_ui8DevConnect = ui8Loop;
                    g_bEstLinkReq = true;
                    break;
                }
            }
        }
    }

    return(0);
}

//*****************************************************************************
//
// Send a command to the UART.
//
//*****************************************************************************
void
UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    //
    // Loop while there are more characters to send.
    //
    while(ui32Count--)
    {
        //
        // Write the next character to the UART.
        //
        ROM_UARTCharPut(UART3_BASE, *pui8Buffer++);
    }
}

//*****************************************************************************
//
// Simple function to dump the buffer contents to the UART debug port.
//
//*****************************************************************************
void
DumpBuffer(uint8_t *pui8Buf, uint32_t ui32Len, bool bTX)
{
    uint32_t ui32Idx = 0;

    UARTprintf("\n%s: %d\n", (bTX?"TX":"RX"), ui32Len);
    for(ui32Idx = 0; ui32Idx < ui32Len; ui32Idx++)
    {
        if(ui32Idx && (ui32Idx % 16 == 0))
        {
            UARTprintf("\n");
        }
        UARTprintf("%02x ", pui8Buf[ui32Idx]);
    }
    UARTprintf("\n\n");
}

//*****************************************************************************
//
// This function looks in the RX buffer and returns true if there is a complete
// message, otherwise it returns false. The complete message will be taken out
// from the RX buffer, and message itself and its length will be returned to
// to the caller.
// This function doesn't block, can be called from interrupt context.
//
//*****************************************************************************
bool
MessageComplete(uint8_t *pui8Buf, uint8_t *pui8Len)
{
    uint8_t ui8Idx;

    if(g_sRxBuf.ui8Count < 7 )
    {
        //
        // Minimum size of a message is 7+ bytes
        //
        return false;
    }

    //
    // Byte 0: Type
    //      1: EventCode
    //      2: Data Length
    //      3: Event LSB
    //      4: Event MSB
    //      5: Status
    //      ...
    //

    //
    //  Get the index of the Data Length, it is the byte[2]
    //
    ui8Idx = g_sRxBuf.ui8Rd+2;

    //
    // Check for buffer wrap
    //
    if(ui8Idx > BUF_SIZE -1)
    {
        ui8Idx -= BUF_SIZE;
    }

    //
    // The whole message length should 3+Datalength
    //
    if(g_sRxBuf.ui8Count < (g_sRxBuf.pui8RXBuf[ui8Idx] + 3))
    {
        return false;
    }

    //
    // Got a complete message.
    // Return the length of the message
    //
    *pui8Len = g_sRxBuf.pui8RXBuf[ui8Idx] + 3;

    //
    // Take the rx data out of the buffer and return to caller.
    //
    for(ui8Idx = 0; ui8Idx < *pui8Len; ui8Idx++)
    {
        pui8Buf[ui8Idx] = g_sRxBuf.pui8RXBuf[g_sRxBuf.ui8Rd++];
        //
        // Check for buffer wrap
        //
        if(g_sRxBuf.ui8Rd >= BUF_SIZE)
        {
            g_sRxBuf.ui8Rd = 0;
        }
    }
    g_sRxBuf.ui8Count -= *pui8Len;

    return true;
}

//*****************************************************************************
//
// This function validates and parses the received message and return true
// if it is valid message, it also return the command status of the message.
// If the message is not valid, the function returns false.
//
//*****************************************************************************
bool
ProcessRxData( uint8_t *pui8CmdStatus)
{
    uint8_t ui8Idx;
    uint16_t ui16Val;

    //
    // Printout the complete message
    //
    DumpBuffer(g_pui8Msg, g_ui8MsgLen, false);

    //
    // Quick check on the first and third bytes.
    // The first byte should be 0x4(EVENT)
    // The third byte should be the data length.
    //
    if((g_pui8Msg[0] != HCI_EVENT_PACKET) ||
       ((g_pui8Msg[2] + 3) != g_ui8MsgLen))
    {
        //
        // Invalid message, toss it
        //
        return false;
    }

    //
    // Parse the message:
    // The second byte is event code, it is either vendor specific event code
    // 0xFF(HCI_VE_EVENT_CODE) or any BLE event code.
    // 3rd and 4th byte are the event
    //
    if(g_pui8Msg[1] == HCI_VE_EVENT_CODE)
    {
        //BLE Ext Event
        g_ui16Event = (g_pui8Msg[3] | (g_pui8Msg[4]<<8));
    }
    else
    {
        //BLE Event
        g_ui16Event = g_pui8Msg[1];
    }

    //
    // Get the status in the response(5th byte)
    //
    if(g_pui8Msg[1] == HCI_VE_EVENT_CODE)
    {
        //BLE Ext Event
        *pui8CmdStatus = g_pui8Msg[5];
    }
    else
    {
        //BLE Event
        *pui8CmdStatus = g_pui8Msg[6];
    }

    switch(g_ui16Event)
    {
        case GAP_HCI_EVENT_EXT_CMD_STATUS:
            //
            // CommandStatus Event
            // Get the command opcode
            //
            g_ui16CmdStatusOpcode = (g_pui8Msg[6] | (g_pui8Msg[7]<<8));
            if(g_ui16CmdStatusOpcode == HCI_VE_GAP_GET_PARAM_OPCODE)
            {
                //
                // Save the parameters.
                //
                g_ui16Param[g_ui8ParamWrIdx++] = (g_pui8Msg[9] | (g_pui8Msg[10]<<8));
            }
            break;

        case GAP_HCI_EVENT_EXT_DEVICE_INIT_DONE:
            //
            // DeviceInitDone Event
            //
            break;

        case GAP_HCI_EVENT_EXT_DEVICE_INFO:
            //
            // Device Information Event
            // parse the scan response
            //
            if(g_pui8Msg[6] == GAP_ADTYPE_SCAN_RSP_IND)
            {
                if(g_ui8DevFound < MAX_SLAVE_NUM)
                {
                    //
                    // Save the device address
                    //
                    g_psDev[g_ui8DevFound].ui8AddrType = g_pui8Msg[7];
                    memcpy(g_psDev[g_ui8DevFound].pui8Addr, g_pui8Msg + 8, HCI_BDADDR_LEN);

                    //
                    // Get the device name
                    // Skip the first ht(0x9) char.
                    //
                    memcpy(g_psDev[g_ui8DevFound].pcName, &g_pui8Msg[18], (g_pui8Msg[16]-1));

                    //
                    // Null terminate the name string
                    //
                    g_psDev[g_ui8DevFound].pcName[g_pui8Msg[16] -1] = 0;

                    //
                    // Increment the number of devices found.
                    //
                    g_ui8DevFound++;
                }
                else
                {
                    UARTprintf("More than %d device found, ignor this device\n", MAX_SLAVE_NUM);
                }

            }
            break;

        case GAP_HCI_EVENT_EXT_DEVICE_DISC_DONE:
            //
            // Device Discover Done Event
            // print out devices' info
            //
            UARTprintf("%d Devices found\n", g_pui8Msg[6]);
            for(ui8Idx = 0; ui8Idx < g_ui8DevFound; ui8Idx++)
            {
                UARTprintf("Device Name: %s\n", g_psDev[ui8Idx].pcName);
                UARTprintf(" -Addr Type: %02x\n", g_psDev[ui8Idx].ui8AddrType);
                UARTprintf(" -Addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
                           g_psDev[ui8Idx].pui8Addr[5],
                           g_psDev[ui8Idx].pui8Addr[4],
                           g_psDev[ui8Idx].pui8Addr[3],
                           g_psDev[ui8Idx].pui8Addr[2],
                           g_psDev[ui8Idx].pui8Addr[1],
                           g_psDev[ui8Idx].pui8Addr[0]);
            }
            break;

        case GAP_HCI_EVENT_EXT_DEVICE_LINK_DONE:
            //
            // Device EstablishLink Event
            //
            g_ui16Handle = (g_pui8Msg[13] | (g_pui8Msg[14] << 8));
            UARTprintf("Device connected: %s\n", g_psDev[g_ui8DevConnect].pcName);
            UARTprintf(" -Handle: %04x\n", g_ui16Handle);
            UARTprintf(" -Addr Type: %04x\n", g_psDev[g_ui8DevConnect].ui8AddrType);
            UARTprintf(" -Addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
                       g_psDev[g_ui8DevConnect].pui8Addr[5],
                       g_psDev[g_ui8DevConnect].pui8Addr[4],
                       g_psDev[g_ui8DevConnect].pui8Addr[3],
                       g_psDev[g_ui8DevConnect].pui8Addr[2],
                       g_psDev[g_ui8DevConnect].pui8Addr[1],
                       g_psDev[g_ui8DevConnect].pui8Addr[0]);

            break;

        case GAP_HCI_EVENT_EXT_DEVICE_TERM_LINK_DONE:
            //
            // Device TerminateLink Event
            //
            UARTprintf("Device Disconnected, reason: %02x\n", g_pui8Msg[8]);
            break;
        case GAP_HCI_EVENT_EXT_DEVICE_PASSKEY_NEEDED:
            break;

        case GAP_HCI_EVENT_EXT_DEVICE_AUTHENTICATE_DONE:
            //
            // Save the LongTermKey for bond request
            //
            g_psDev[g_ui8DevConnect].sSaveKey.bValid = true;
            ui8Idx = 37; // The DSInf.Enable field is at index 37 in the byte string.
            g_psDev[g_ui8DevConnect].sSaveKey.bAuth = g_pui8Msg[ui8Idx++];

            //
            // Next byte is LTK size, followed by the LTK
            //
            g_psDev[g_ui8DevConnect].sSaveKey.ui8LTKSize =  g_pui8Msg[ui8Idx++];
            memcpy(g_psDev[g_ui8DevConnect].sSaveKey.pui8LTK,
                   g_pui8Msg + ui8Idx,
                   g_psDev[g_ui8DevConnect].sSaveKey.ui8LTKSize);
            ui8Idx += g_psDev[g_ui8DevConnect].sSaveKey.ui8LTKSize;

            //
            // 2 bytes of DIV, followed by 8 bytes of Random number
            //
            g_psDev[g_ui8DevConnect].sSaveKey.pui8DIV[0] = g_pui8Msg[ui8Idx++];
            g_psDev[g_ui8DevConnect].sSaveKey.pui8DIV[1] = g_pui8Msg[ui8Idx++];
            memcpy(g_psDev[g_ui8DevConnect].sSaveKey.pui8Rand, g_pui8Msg + ui8Idx, 8);

            break;
        case GAP_HCI_EVENT_EXT_DEVICE_BOND_DONE:
            UARTprintf("Bonded\n");
            break;

        case GAP_HCI_EVENT_EXT_ATT_WRITE_RSP:
            break;

        case GAP_HCI_EVENT_EXT_ATT_READ_RSP:
            //
            // this is the IR temperature response
            //
            memcpy(g_pui8IRTemp, g_pui8Msg + 9, 4);
            HandleTemp();
            break;

        case HCI_ATT_ERROR_RSP_EVENT:
            UARTprintf("Error Response event: \n");
            break;

        case GAP_HCI_EVENT_CMD_COMPLETE:
            //
            // Get the command opcode
            //
            g_ui16CmdStatusOpcode = (g_pui8Msg[4] | (g_pui8Msg[5]<<8));
            if(g_ui16CmdStatusOpcode == HCI_READ_RSSI_OPCODE)
            {
                //
                // RSSI reading response
                //
                g_i8RSSI = (int8_t)g_pui8Msg[9];
                UARTprintf("RSSI = %d 0x%x\n", g_i8RSSI, g_pui8Msg[9]);

                //
                // Display the data when it is only in LINKED state
                //
                if(g_iState == STATE_LINKED)
                {
                    DisplayRSSI(g_ui32Width / 2 + 80, 80 + 40);
                }
            }
            break;

        case GAP_HCI_EVENT_HANDLE_VALUE_NOTIFY:
            if(g_pui8Msg[8] > 2) //PduLen
            {
                ui16Val = g_pui8Msg[9] | (g_pui8Msg[10] <<8);
                switch(ui16Val)
                {
                    case GATT_IRTEMP_DATA_UUID_HANDLE:
                        //
                        // Temperatur Sensor data
                        //
                        memcpy(g_pui8IRTemp, g_pui8Msg + 11, 4);

                        //
                        // Display the data when it is LINKED state
                        //
                        if(g_iState == STATE_LINKED)
                        {
                            HandleTemp();
                        }
                        break;

                    case GATT_HUMIDITY_DATA_UUID_HANDLE:
                        //
                        // Humidity Sensor data
                        //
                        memcpy(g_pui8Humidity, g_pui8Msg + 11, 4);

                        //
                        // Display the data when it is LINKED state
                        //
                        if(g_iState == STATE_LINKED)
                        {
                            HandleHumidity();
                        }
                        break;

                    default:
                        UARTprintf("unexpected handle: %04x, fix me\n", ui16Val);
                        break;

                }
            }
            break;

        default:
            UARTprintf("unexpected event: %04x, fix me\n", g_ui16Event);
            break;

    }

    return true;
}

//*****************************************************************************
//
// This function verifies if the received response is expected, and return
// true if it is, false otherwise.
//
//*****************************************************************************
bool
VerifyMsg(uint16_t ui16ExpectedEvent, uint16_t ui16ExpectedEventParam,
          uint8_t *pui8Status)
{
    uint8_t ui8CmdStatus;

    if(ProcessRxData(&ui8CmdStatus) == 0)
    {
        //
        // Message is not valid
        //
        return false;
    }

    if(ui8CmdStatus != SUCCESS)
    {
        //
        // Message return non-success status code
        //
        UARTprintf("Command Status return failure: %02x\n", ui8CmdStatus);
    }

    //
    // Pass the status code to the caller if a location is provided.
    if(pui8Status)
    {
        *pui8Status = ui8CmdStatus;
    }

    //
    // check the received event
    //
    UARTprintf("RX: event 0x%04x\n", g_ui16Event);
    if(ui16ExpectedEvent)
    {
        //
        // We are expecting a specific event, check it.
        //
        if(g_ui16Event == ui16ExpectedEvent)
        {
            //
            // Validate Event param if supplied.
            //
            if(ui16ExpectedEventParam)
            {
                if(ui16ExpectedEventParam == g_ui16CmdStatusOpcode)
                {
                    return true;
                }
            }
            else
            {
                //
                // No need to verify EventParam
                //
                return true;
            }
        }
    }
    else
    {
        //
        // We are NOT expecting any specific event, just return true.
        //
        return true;
    }

    return false;
}

//*****************************************************************************
//
// This function sets the timeout and waits for the given event, the function
// will return true when the expected event has been received before the
// timeout; or return false when the expected event has not been received
// before the timeout.
//
//*****************************************************************************
bool
WaitForRsp(uint16_t ui16ExpectedEvent, uint16_t ui16ExpectedParam,
           uint32_t ui32TimeoutMs, uint8_t *pui8Status)
{
    bool bRet = false;

    //
    // Set the timeout if non zero
    // i32Timeout is in ms, convert it to number of systicks.
    // Systick timer is every 10ms, so ui32TimeoutMs/10 + 1
    //
    g_ui32Delay = ui32TimeoutMs?(ui32TimeoutMs/10 + 1):0;

    //
    // Block until there is a response or timeout
    //
    while(g_ui32Delay > 1)
    {
        if(HWREGBITW(&g_ui32Flags, FLAG_HCI_MSG_COMPLETE) == 1)
        {
            //
            // Got a response
            //
            bRet = VerifyMsg(ui16ExpectedEvent, ui16ExpectedParam, pui8Status);
            if(bRet)
            {
                //
                // Got what we expected, disable timeout, and exit the loop
                //
                g_ui32Delay = 0;
            }

            //
            // Clear the flag to indicate we are ready to receive the next
            // complete message
            //
            HWREGBITW(&g_ui32Flags, FLAG_HCI_MSG_COMPLETE) = 0;
        }
    }

    //
    // Check for timeout
    //
    if(g_ui32Delay == 1)
    {
        UARTprintf("\nTimeout waiting for response..%04x.\n",
                   ui16ExpectedEvent);
    }

    return bRet;
}

//*****************************************************************************
//
// This function checks if there are any events in the queue to be processed.
// Such as notify event, it will be sent from slave periodically.
//
//*****************************************************************************
bool
CheckForMsg()
{
    bool bSuccess = false;

    if(HWREGBITW(&g_ui32Flags, FLAG_HCI_MSG_COMPLETE) == 1)
    {
        //
        // Got a response
        //
        bSuccess = VerifyMsg(0, 0, 0);
        if(g_ui16Event == GAP_HCI_EVENT_EXT_DEVICE_TERM_LINK_DONE)
        {
            UARTprintf("Slave terminated the link\n");

            //
            // Sensor will be turned off by the slave
            //
            HWREGBITW(&g_ui32Flags, FLAG_SENSOR_CFGD) = 0;

            //
            // Go back to TERMED state
            //
            g_iState = STATE_TERMED;
        }

        //
        // Clear the flag to indicate we are ready to receive the next
        // complete message
        //
        HWREGBITW(&g_ui32Flags, FLAG_HCI_MSG_COMPLETE) = 0;
    }

    return bSuccess;

}

//*****************************************************************************
//
// This function configures the sensor profiles.
//
//*****************************************************************************
void
ConfigureSensors(void)
{
    uint8_t pui8Byte[2];
    uint8_t ui8Status;
    bool    bSuccess;

    //
    // Enable IR Sensor and Measurements if it is not yet configured.
    //
    if(HWREGBITW(&g_ui32Flags, FLAG_SENSOR_CFGD) == 1)
    {
        return;
    }

    UARTprintf("Send Temp sensor wake cmd...\n");
    pui8Byte[0] = 0x01;
    GAPWriteCharValue(g_ui16Handle, GATT_IRTEMP_CFG_UUID_HANDLE, pui8Byte, 1);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_WRITE_CHAR_VAL_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("ConfigureSensors: Wait for CommandStatus error\n");
    }

    //
    // Wait for ATT_WriteRsp event, timeout after 500ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_ATT_WRITE_RSP, 0, 500, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("ConfigureSensors: Wait for ATT_WriteRsp error\n");
    }

    //
    // Humidity Sensor
    //
    UARTprintf("Send humidity sensor wake cmd...\n");
    GAPWriteCharValue(g_ui16Handle, GATT_HUMIDITY_CFG_UUID_HANDLE, pui8Byte, 1);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_WRITE_CHAR_VAL_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("ConfigureSensors: Wait for CommandStatus error\n");
    }

    //
    // Wait for ATT_WriteRsp event, timeout after 500ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_ATT_WRITE_RSP, 0, 500, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("ConfigureSensors: Wait for ATT_WriteRsp error\n");
    }

    UARTprintf("Send Temp sensor notify cmd...\n");
    pui8Byte[0] = 0x01;
    pui8Byte[1] = 0x00;
    GAPWriteCharValue(g_ui16Handle, GATT_IRTEMP_NOTIFY_UUID_HANDLE, pui8Byte, 2);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_WRITE_CHAR_VAL_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("ConfigureSensors: Wait for CommandStatus error\n");
    }

    //
    // Wait for ATT_WriteRsp event, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_ATT_WRITE_RSP, 0, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("ConfigureSensors: Wait for ATT_WriteRsp error\n");
    }

    //
    // Humidity Sensor
    //
    UARTprintf("Send Humidity sensor notify cmd...\n");
    GAPWriteCharValue(g_ui16Handle, GATT_HUMIDITY_NOTIDY_UUID_HANDLE, pui8Byte, 2);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_WRITE_CHAR_VAL_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("ConfigureSensors: Wait for CommandStatus error\n");
    }

    //
    // Wait for ATT_WriteRsp event, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_ATT_WRITE_RSP, 0, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("ConfigureSensors: Wait for ATT_WriteRsp error\n");
    }

    //
    // Set the flag that sensors are configured.
    //
    HWREGBITW(&g_ui32Flags, FLAG_SENSOR_CFGD) = 1;
}

//*****************************************************************************
//
// This function deconfigures the sensor profiles.
//
//*****************************************************************************
void
DeConfigureSensors(void)
{
    uint8_t pui8Byte[2];
    uint8_t ui8Status;
    bool    bSuccess;

    if(HWREGBITW(&g_ui32Flags, FLAG_SENSOR_CFGD) == 0)
    {
        // Do nothing if the sensors have not been configured.
        return;
    }

    //
    // TM006 IR and Ambient temperature sensor
    //
    UARTprintf("Send Temp sensor stop notify cmd...\n");
    pui8Byte[0] = 0x00;
    pui8Byte[1] = 0x00;
    GAPWriteCharValue(g_ui16Handle, GATT_IRTEMP_NOTIFY_UUID_HANDLE, pui8Byte, 2);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_WRITE_CHAR_VAL_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("DeConfigureSensors: Wait for CommandStatus error\n");
    }

    //
    // Humidity Sensor
    //
    UARTprintf("Send Humidity sensor stop notify cmd...\n");
    GAPWriteCharValue(g_ui16Handle, GATT_HUMIDITY_NOTIDY_UUID_HANDLE, pui8Byte, 2);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_WRITE_CHAR_VAL_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("DeConfigureSensors: Wait for CommandStatus error\n");
    }

    //
    // Wait for ATT_WriteRsp event, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_ATT_WRITE_RSP, 0, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("DeConfigureSensors: Wait for ATT_WriteRsp error\n");
    }

    //
    // Put IR Sensor and Measurements to sleep
    //
    UARTprintf("Send IR Temp sensor sleep cmd...\n");
    pui8Byte[0] = 0x00;
    GAPWriteCharValue(g_ui16Handle, GATT_IRTEMP_CFG_UUID_HANDLE, pui8Byte, 1);

    //
    // Wait for CommandStatus response, timeout after 500ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_WRITE_CHAR_VAL_OPCODE, 500, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("DeConfigureSensors: Wait for CommandStatus error\n");
    }

    //
    // Wait for ATT_WriteRsp event, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_ATT_WRITE_RSP, 0, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("DeConfigureSensors: Wait for ATT_WriteRsp error\n");
    }

    UARTprintf("Send Humidity sensor sleep cmd...\n");
    GAPWriteCharValue(g_ui16Handle, GATT_HUMIDITY_CFG_UUID_HANDLE, pui8Byte, 1);

    //
    // Wait for CommandStatus response, timeout after 500ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_WRITE_CHAR_VAL_OPCODE, 500, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("DeConfigureSensors: Wait for CommandStatus error\n");
    }

    //
    // Wait for ATT_WriteRsp event, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_ATT_WRITE_RSP, 0, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("DeConfigureSensors: Wait for ATT_WriteRsp error\n");
    }
    HWREGBITW(&g_ui32Flags, FLAG_SENSOR_CFGD) = 0;

}

//*****************************************************************************
//
// This function queries the parameters of the central device.
//
//*****************************************************************************
bool
GetParam(void)
{
    uint8_t ui8Status;
    bool    bSuccess;

    UARTprintf("Get Param...\n");

    //
    // Start Parameter write index with 0
    //
    g_ui8ParamWrIdx = 0;

    //
    // Get the Minimum Link Layer connection interval
    //
    GAPGetParam(TGAP_CONN_EST_INT_MIN);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_GET_PARAM_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("Get Param %x failed\n", TGAP_CONN_EST_INT_MIN);
    }

    //
    // Get the Maximum Link Layer connection interval
    //
    GAPGetParam(TGAP_CONN_EST_INT_MAX);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_GET_PARAM_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("Get Param %x failed\n", TGAP_CONN_EST_INT_MAX);
    }

    //
    // Get the Link Layer connection slave latency
    //
    GAPGetParam(TGAP_CONN_EST_LATENCY);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_GET_PARAM_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("Get Param %x failed\n", TGAP_CONN_EST_LATENCY);
    }

    //
    // Get the Link Layer connection supervision timeout
    //
    GAPGetParam(TGAP_CONN_EST_SUPERV_TIMEOUT);
    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_GET_PARAM_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {
        UARTprintf("Get Param %x failed\n", TGAP_CONN_EST_SUPERV_TIMEOUT);
    }

    if(g_ui16Param[0] != 0 && g_ui16Param[1] != 0 && g_ui16Param[3] != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//*****************************************************************************
//
// This function set the parameters of the central device.
//
//*****************************************************************************
bool
SetParam(void)
{
    uint8_t ui8Status;
    bool    bSuccess;

    UARTprintf("Set Param...\n");

    //
    // Set the Minimum Link Layer connection interval
    //
    GAPSetParam(TGAP_CONN_EST_INT_MIN, g_ui16Param[0]);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_SET_PARAM_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {

        UARTprintf("Set Param %x failed\n", TGAP_CONN_EST_INT_MIN);
    }

    //
    // Set the Maximum Link Layer connection interval
    //
    GAPSetParam(TGAP_CONN_EST_INT_MAX, g_ui16Param[1]);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_SET_PARAM_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {

        UARTprintf("Set Param %x failed\n", TGAP_CONN_EST_INT_MAX);
    }

    //
    // Set the Link Layer connection slave latency
    //
    GAPSetParam(TGAP_CONN_EST_LATENCY, g_ui16Param[2]);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_SET_PARAM_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {

        UARTprintf("Set Param %x failed\n", TGAP_CONN_EST_LATENCY);
    }

    //
    // Set the Link Layer connection supervision timeout
    //
    GAPSetParam(TGAP_CONN_EST_SUPERV_TIMEOUT, g_ui16Param[3]);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_SET_PARAM_OPCODE, 200, &ui8Status);
    if(!bSuccess || ui8Status)
    {

        UARTprintf("Set Param %x failed\n", TGAP_CONN_EST_SUPERV_TIMEOUT);
    }

    return bSuccess;
}

//*****************************************************************************
//
// This function authenticates with the slave device.
//
//*****************************************************************************
bool
Authenticate(void)
{
    bool bSuccess = false;
    uint8_t ui8Status;

    if(g_psDev[g_ui8DevConnect].sSaveKey.bValid == false)
    {
        UARTprintf("Initiate Pairing Request...\n");
        GAPAuthenticate(g_ui16Handle);

        //
        // Wait for CommandStatus response, timeout after 100ms
        //
        bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_AUTHENTICATE_OPCODE, 100, &ui8Status);
        if(bSuccess && ui8Status == SUCCESS)
        {
            //
            // Wait for PasskeyNeeded event, timeout after 1s
            //
            bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_DEVICE_PASSKEY_NEEDED, 0, 1000, &ui8Status);
            if(bSuccess && ui8Status == SUCCESS)
            {
                //
                // Got PasskeyNeeded event, send the key
                //
                GAPPassKeyUpdate(g_ui16Handle, "000000");

                //
                // Wait for CommandStatus response, timeout after 200ms
                //
                bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_PASSKEY_UPDATE_OPCODE, 200, &ui8Status);
                if(bSuccess && ui8Status == SUCCESS)
                {
                    //
                    // Wait for AuthenticationComplete event, timeout after 5s
                    //
                    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_DEVICE_AUTHENTICATE_DONE, 0, 5000, &ui8Status);
                }
            }
        }
    }
    else
    {
        UARTprintf("Bond Request...\n");
        GAPBond(g_ui16Handle, &g_psDev[g_ui8DevConnect].sSaveKey);

        //
        // Wait for CommandStatus response, timeout after 100ms
        //
        bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_BOND_OPCODE, 100, &ui8Status);
        if(bSuccess && ui8Status == SUCCESS)
        {
            //
            // Wait for BondComplete event, timeout after 1s
            //
            bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_DEVICE_BOND_DONE, 0, 1000, &ui8Status);
        }
    }

    if(ui8Status != SUCCESS)
    {
        UARTprintf("Authenticate failure 0x%x\n", ui8Status);
        return false;
    }
    return bSuccess;
}

//*****************************************************************************
//
// This function establishes link to the slave.
//
//*****************************************************************************
bool
EstablishLink(uint8_t ui8DevIdx)
{
    bool bSuccess = false;
    uint8_t ui8Status;

    UARTprintf("Link Request on device %d...\n", ui8DevIdx);

    GAPEstLinkReq(false, false, g_psDev[ui8DevIdx].ui8AddrType, g_psDev[ui8DevIdx].pui8Addr);

    //
    // Wait for CommandStatus response, timeout after 100ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_EST_LINK_REQ_OPCODE, 100, &ui8Status);
    if(bSuccess && (ui8Status == SUCCESS || ui8Status == bleAlreadyInRequestedMode))
    {
        //
        // Wait for EstablishLink response, timeout after 15s
        //
        bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_DEVICE_LINK_DONE, 0, 15000, &ui8Status);
        if(bSuccess && ui8Status == SUCCESS)
        {
            //
            // Got DeviceInitDone response, go to the next state
            //
            if(g_bInitPairReq == true)
            {
                bSuccess = Authenticate();
            }
        }
    }

    if(ui8Status != SUCCESS)
    {
        return false;
    }
    return bSuccess;
}

//*****************************************************************************
//
// This function terminates the link.
//
//*****************************************************************************
bool
TerminateLink(void)
{
    bool bSuccess = false;
    uint8_t ui8Status;

    UARTprintf("Terminate Link Request...\n");

    DeConfigureSensors();

    //
    // Send TeminateLink request
    //
    GAPTerLinkReq(g_ui16Handle, HCI_DISCONNECT_REMOTE_USER_TERM);

    //
    // Wait for CommandStatus response, timeout after 200ms
    //
    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_TER_LINK_REQ_OPCODE, 200, &ui8Status);
    if(bSuccess && ui8Status == SUCCESS)
    {
        //
        // Wait for TerminateLink event, timeout after 1s
        //
        bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_DEVICE_TERM_LINK_DONE, 0, 1000, &ui8Status);
    }

    if(ui8Status != SUCCESS)
    {
        return false;
    }
    return bSuccess;
}

//*****************************************************************************
//
// This function draws the animated circle during discovery state
//
//*****************************************************************************
void
DrawCircle(void)
{
    uint32_t ui32Idx;

    //
    // Loop through the circles in the animation.
    //
    for(ui32Idx = 0; ui32Idx < 8; ui32Idx++)
    {
        //
        // Draw this circle.
        //
        GrContextForegroundSet(&g_sContext,
                               g_pui32CircleColor[(g_ui32ColorIdx +
                                                   ui32Idx) & 7]);
        GrCircleFill(&g_sContext,
                     (g_ui32Width / 2) + g_ppi32CirclePos[ui32Idx][0],
                     (g_ui32Height / 2) + g_ppi32CirclePos[ui32Idx][1] + 24,
                     2);
    }

    //
    // Increment the color index.
    //
    g_ui32ColorIdx++;
}

//*****************************************************************************
//
// Clear the screen
//
//*****************************************************************************
void ClearScreen(void)
{
    tRectangle sRect;

    //
    // Clear the display.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = g_ui32Width - 1;
    sRect.i16YMax = g_ui32Height - 1;
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);
    GrContextForegroundSet(&g_sContext, ClrWhite);
}

//*****************************************************************************
//
// Update the display.
//
//*****************************************************************************
void
UpdateDisplay(eDisplayUpdateIdx eUpdate)
{
    uint8_t ui8Loop;
    char pcBuf[20];

    //
    // Clear the middle section of the display
    //
    ClearScreen();

    //
    // Update the middle of screen
    //
    if(eUpdate == iConnecting)
    {
        //
        // show the to be connected device only
        //
        GrStringDrawCentered(&g_sContext, g_psDev[g_ui8DevConnect].pcName, -1,
                             g_ui32Width / 2, 60 + (40*g_ui8DevConnect), false);

        //
        // Convert the device address into a string.
        //
        usprintf(pcBuf, "(%02x:%02x:%02x:%02x:%02x:%02x)",
                 g_psDev[g_ui8DevConnect].pui8Addr[5],
                 g_psDev[g_ui8DevConnect].pui8Addr[4],
                 g_psDev[g_ui8DevConnect].pui8Addr[3],
                 g_psDev[g_ui8DevConnect].pui8Addr[2],
                 g_psDev[g_ui8DevConnect].pui8Addr[1],
                 g_psDev[g_ui8DevConnect].pui8Addr[0]);
        GrContextFontSet(&g_sContext, g_psFontCm14);
        GrStringDrawCentered(&g_sContext, pcBuf, -1,
                             g_ui32Width / 2, 60 + (40*g_ui8DevConnect + 16), false);
        GrContextFontSet(&g_sContext, g_psFontCmss16b);

    }
    else if(eUpdate == iDisconnect)
    {
        GrStringDraw(&g_sContext, "IR Temperature:", -1,
                     (g_ui32Width / 2) - 110, 80, false);
        GrStringDraw(&g_sContext, "Ambient Temperature:", -1,
                     (g_ui32Width / 2) - 110, 80 + 20, false);
        GrStringDraw(&g_sContext, "RSSI:", -1,
                     (g_ui32Width / 2) - 110, 80 + 40, false);
        GrStringDraw(&g_sContext, "Humidity:", -1,
                     (g_ui32Width / 2) - 110, 80 + 60, false);

        //
        // clear the sensor date
        //
        g_dIRTemp   = 0;
        g_dAmbTemp  = 0;
        g_dHumidity = 0;
        g_i8RSSI    = 0;

        DisplayTemp(g_ui32Width / 2 + 80, 80);
        DisplayRSSI(g_ui32Width / 2 + 80, 80 + 40);
        DisplayHumidity(g_ui32Width / 2 + 80, 80 + 60);
    }
    else
    {
        if(ppcString[eUpdate][0])
        {
            GrStringDrawCentered(&g_sContext, ppcString[eUpdate][0], -1,
                                 g_ui32Width / 2, (g_ui32Height / 2) - 18, false);
        }
        else
        {

            for(ui8Loop = 0; ui8Loop < g_ui8DevFound; ui8Loop++)
            {
                GrStringDrawCentered(&g_sContext, g_psDev[ui8Loop].pcName, -1,
                                     g_ui32Width / 2, 60 + (40*ui8Loop), false);

                //
                // Convert the device address into a string.
                //
                usprintf(pcBuf, "(%02x:%02x:%02x:%02x:%02x:%02x)",
                         g_psDev[ui8Loop].pui8Addr[5],
                         g_psDev[ui8Loop].pui8Addr[4],
                         g_psDev[ui8Loop].pui8Addr[3],
                         g_psDev[ui8Loop].pui8Addr[2],
                         g_psDev[ui8Loop].pui8Addr[1],
                         g_psDev[ui8Loop].pui8Addr[0]);
                GrContextFontSet(&g_sContext, g_psFontCm14);
                GrStringDrawCentered(&g_sContext, pcBuf, -1,
                                     g_ui32Width / 2, 60 + (40*ui8Loop + 16), false);
                GrContextFontSet(&g_sContext, g_psFontCmss16b);
            }
        }
    }

    //
    // Update the bottom text.
    //
    if(ppcString[eUpdate][1])
    {
        GrStringDrawCentered(&g_sContext, ppcString[eUpdate][1], -1,
                             g_ui32Width / 2, 200, false);
    }
}

//*****************************************************************************
//
// Calculate the object temperature from TMP006 reading.
// Refer to TMP006 data sheet.
//
//*****************************************************************************
double
calculateTemp(int16_t i16Tdie, int16_t i16Vobj)
{
    double Vobj2 = (double)i16Vobj*.00000015625;
    double i16Tdie2 = (double)i16Tdie*.03125 + 273.15;
    double S0 = 6.40*pow(10,-14);
    double a1 = 1.75*pow(10,-3);
    double a2 = -1.678*pow(10,-5);
    double b0 = -2.94*pow(10,-5);
    double b1 = -5.70*pow(10,-8);
    double b2 = 4.63*pow(10,-10);
    double c2 = 13.4;
    double Tref = 298.15;
    double S = S0*(1+a1*(i16Tdie2 - Tref)+a2*pow((i16Tdie2 - Tref),2));
    double Vos = b0 + b1*(i16Tdie2 - Tref) + b2*pow((i16Tdie2 - Tref),2);
    double fObj = (Vobj2 - Vos) + c2*pow((Vobj2 - Vos),2);
    double Tobj = pow(pow(i16Tdie2,4) + (fObj/S),.25);
    return (Tobj - 273.15);
}

//*****************************************************************************
//
// This function converts the raw temperature reading to actual temperature in
// C and displays the temperatures on the display.
//
//*****************************************************************************
void
HandleTemp(void)
{
    int16_t i16VObj, i16TDie;

    UARTprintf("IR %02x %02x %02x %02x\n", g_pui8IRTemp[0], g_pui8IRTemp[1],
               g_pui8IRTemp[2], g_pui8IRTemp[3]);
    //
    // The first two bytes are Object Voltage,
    // the last two bytes are Die temperature
    //
    i16VObj = (int16_t)(g_pui8IRTemp[0] | (g_pui8IRTemp[1]<<8));
    i16TDie = (int16_t)(g_pui8IRTemp[2] | (g_pui8IRTemp[3]<<8));
    if(i16VObj && i16TDie)
    {
        g_dIRTemp = calculateTemp(i16TDie>>2, i16VObj);
        g_dAmbTemp = (double)i16TDie /128.0;
        DisplayTemp(g_ui32Width / 2 + 80, 80);
    }
}

//*****************************************************************************
//
// This function converts the raw humidity to humidity in rH and displays it
// on the display.
//
//*****************************************************************************
void
HandleHumidity(void)
{
    uint16_t ui16RawH;

    //
    // The first two bytes are temperature(ignored)
    // the last two bytes are humitity
    //
    ui16RawH = (g_pui8Humidity[2] | (g_pui8Humidity[3]<<8));
    UARTprintf("Humidity %04x\n", ui16RawH);

    //
    // Conversion algorithm for Humidity
    //
    ui16RawH &= ~0x0003;  // clear bits [1..0] (status bits)
    g_dHumidity = -6.0 + (125.0 *(double)ui16RawH)/65536; // RH= -6 + 125 * SRH/2^16
    DisplayHumidity(g_ui32Width / 2 + 80, 80 + 60);
}

//*****************************************************************************
//
// Display IR and Ambient temperatures on the display.
//
//*****************************************************************************
void
DisplayTemp(uint32_t ui32X, uint32_t ui32Y)
{
    char pcBuf[16];
    int16_t i16IRInt, i16IRFrac;
    tRectangle sRect;

    i16IRInt = ((int16_t)(g_dIRTemp*100))/100;
    i16IRFrac = (int16_t)(g_dIRTemp*100) - i16IRInt*100;
    UARTprintf("IR temp = %d.%d\n", i16IRInt, i16IRFrac);

    //
    // Convert the temperature into a string.
    //
    usprintf(pcBuf, "%d.%dC", i16IRInt, i16IRFrac);

    //
    // Clear the previous reading.
    //
    sRect.i16XMin = ui32X;
    sRect.i16YMin = ui32Y;
    sRect.i16XMax = ui32X + 60;
    sRect.i16YMax = ui32Y + 20;
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);
    GrContextForegroundSet(&g_sContext, ClrWhite);

    //
    // Display the IR temperature.
    //
    GrStringDraw(&g_sContext, pcBuf, -1, ui32X, ui32Y, false);

    i16IRInt = ((int16_t)(g_dAmbTemp*100))/100;
    i16IRFrac = (int16_t)(g_dAmbTemp*100) - i16IRInt*100;
    UARTprintf("Ambient temp = %d.%d\n", i16IRInt, i16IRFrac);

    //
    // Convert the temperature into a string.
    //
    usprintf(pcBuf, "%d.%dC", i16IRInt, i16IRFrac);

    //
    // Clear the previous temperature.
    //
    sRect.i16XMin = ui32X;
    sRect.i16YMin = ui32Y + 20;
    sRect.i16XMax = ui32X + 60;
    sRect.i16YMax = ui32Y + 40;
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);
    GrContextForegroundSet(&g_sContext, ClrWhite);

    //
    // Display the Ambient temperatur.
    //
    GrStringDraw(&g_sContext, pcBuf, -1, ui32X, ui32Y + 20, false);
}

//*****************************************************************************
//
// Display RSSI on the display.
//
//*****************************************************************************
void
DisplayRSSI(uint32_t ui32X, uint32_t ui32Y)
{
    char pcBuf[16];
    tRectangle sRect;

    //
    // Convert the RSSI data into a string.
    //
    if(g_i8RSSI >= 0)
    {
        //
        // RSSI has to be negative value, discard the nonvalid data.
        //
        return;
    }
    usprintf(pcBuf, "%ddBm", g_i8RSSI);

    //
    // Clear the previous reading.
    //
    sRect.i16XMin = ui32X;
    sRect.i16YMin = ui32Y;
    sRect.i16XMax = ui32X + 60;
    sRect.i16YMax = ui32Y + 20;
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);
    GrContextForegroundSet(&g_sContext, ClrWhite);

    //
    // Display on the screen.
    //
    GrStringDraw(&g_sContext, pcBuf, -1, ui32X, ui32Y, false);
}

//*****************************************************************************
//
// Display Humidity on the display.
//
//*****************************************************************************
void
DisplayHumidity(uint32_t ui32X, uint32_t ui32Y)
{
    char pcBuf[16];
    tRectangle sRect;
    uint16_t ui8Int, ui8Frac;

    ui8Int = (uint16_t)(g_dHumidity);
    ui8Frac = (uint16_t)(g_dHumidity*10) - ui8Int*10;
    UARTprintf("Humidity = %02d.%01d\n", ui8Int, ui8Frac);

    //
    // Convert the humidity into a string.
    //
    usprintf(pcBuf, "%d.%01d%%rH", ui8Int, ui8Frac);

    //
    // Clear the previous reading.
    //
    sRect.i16XMin = ui32X;
    sRect.i16YMin = ui32Y;
    sRect.i16XMax = ui32X + 70;
    sRect.i16YMax = ui32Y + 20;
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);
    GrContextForegroundSet(&g_sContext, ClrWhite);

    //
    // Display on the screen.
    //
    GrStringDraw(&g_sContext, pcBuf, -1, ui32X, ui32Y, false);
}

//*****************************************************************************
//
// This example demonstrates how to communicate a BLE slave using TI's CC2540
// EM on TM4C129X Development board.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32SysClock;
    bool bSuccess;
    uint8_t ui8Status;
    uint8_t pui8IRKOrCSRK[16];

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
    FrameDraw(&g_sContext, "ble-central");
    GrContextFontSet(&g_sContext, g_psFontCmss16b);

    //
    // Initialize the touch screen driver
    //
    TouchScreenInit(ui32SysClock);
    TouchScreenCallbackSet(TouchCallback);

    //
    // UART 0 is used for debugging message console.
    //
    UARTStdioConfig(0, 115200, ui32SysClock);

    UARTprintf("\nBLE Central demo running...\n");

    //
    // Get the width and height of the display.
    //
    g_ui32Width = GrContextDpyWidthGet(&g_sContext);
    g_ui32Height = GrContextDpyHeightGet(&g_sContext);

    //
    // UART3 is used to communicate with CC2540, configure the pins.
    // PJ0, 1, 4, 5 are used for UART3.
    //
    ROM_GPIOPinConfigure(GPIO_PJ0_U3RX);
    ROM_GPIOPinConfigure(GPIO_PJ1_U3TX);
    ROM_GPIOPinConfigure(GPIO_PJ4_U3RTS);
    ROM_GPIOPinConfigure(GPIO_PJ5_U3CTS);
    ROM_GPIOPinTypeUART(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1 |
                                         GPIO_PIN_4 | GPIO_PIN_5);

    //
    // EnableUART3
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);

    //
    // Configure the UART3 for 115,200, 8-N-1 operation.
    //
    ROM_UARTConfigSetExpClk(UART3_BASE, ui32SysClock, 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    //
    // Configure UART3 to use hardware flow control.
    //
    UARTFlowControlSet(UART3_BASE, UART_FLOWCONTROL_TX | UART_FLOWCONTROL_RX);

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Enable the UART interrupt.
    //
    ROM_IntEnable(INT_UART3);
    ROM_UARTIntEnable(UART3_BASE, UART_INT_RX | UART_INT_RT);


    //
    // Clear timeout value
    //
    g_ui32Delay = 0;

    //
    // Configure SysTick for a periodic interrupt at 10ms.
    //
    ROM_SysTickPeriodSet(ui32SysClock / 100);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // clear the device info and RX buffer
    //
    memset(g_psDev, 0, sizeof(g_psDev));
    memset(&g_sRxBuf, 0, sizeof(tCirBuf));

    //
    // Start the state machine with initial state
    //
    g_iState = STATE_DEV_INIT;

    //
    // Display "Initializing" on the bottom of screen
    //
    UpdateDisplay(iInitializing);

    while(1)
    {
        switch(g_iState)
        {
            case STATE_DEV_INIT:

                UARTprintf("Device Init...\n");

                //
                // Send GAP_DeviceInit command
                //
                memset(pui8IRKOrCSRK, 0, 16);
                GAPDeviceInit(GAP_PROFILE_CENTRAL, 5, pui8IRKOrCSRK, pui8IRKOrCSRK, 1);

                //
                // Wait for CommandStatus response, timeout after 500ms
                //
                bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_INIT_OPCODE, 500, &ui8Status);
                if(bSuccess && ui8Status == SUCCESS)
                {
                    //
                    // Wait for DeviceInitDone response, timeout after 500ms
                    //
                    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_DEVICE_INIT_DONE, 0, 500, &ui8Status);
                    if(bSuccess && ui8Status == SUCCESS)
                    {
                        //
                        // Got DeviceInitDone response, go to the next state
                        //
                        g_iState = STATE_GET_PARAM;
                    }
                }
                else
                {
                    UARTprintf("CC2540 EM board is not connected to the DK\n");
                    UpdateDisplay(iNoBLE);

                    //
                    // Hang forever
                    //
                    while(1);
                }
                break;

            case STATE_GET_PARAM:
                if(GetParam())
                {
                    //
                    // Query parameter successful, go to the discovery state
                    //
                    g_bDiscoveryReq = true;
                    g_iState = STATE_START_DISCOVERY;
                    UARTprintf("Ready to scan devices\n");

                    //
                    // Update display
                    //
                    UpdateDisplay(iScanning);
                }
                else
                {
                    //
                    // cannot query parameters on CC2540, something wrong,
                    // go to the error state
                    //
                    g_iState = STATE_ERROR;
                }
                break;

            case STATE_START_DISCOVERY:
                if(g_bDiscoveryReq)
                {
                    UARTprintf("Start Discovery...\n");

                    //
                    // Clear the number of devices discovered
                    //
                    g_ui8DevFound = 0;

                    //
                    // Start to draw circle periodically
                    //
                    HWREGBITW(&g_ui32Flags, FLAG_DRAW_CIRCLE) = 1;

                    //
                    // Send Discovery command
                    //
                    GAPDiscoveryReq(DEVDISC_MODE_ALL, true, false);

                    //
                    // Wait for CommandStatus response, timeout after 100ms
                    //
                    bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_CMD_STATUS, HCI_VE_GAP_DEVICE_DISC_REQ_OPCODE, 100, &ui8Status);
                    if(bSuccess && ui8Status == SUCCESS)
                    {
                        //
                        // Wait for DiscoveryDone response, timeout after 20s
                        //
                        bSuccess = WaitForRsp(GAP_HCI_EVENT_EXT_DEVICE_DISC_DONE, 0, 20000, &ui8Status);
                        if(bSuccess && ui8Status == SUCCESS)
                        {
                            //
                            // Have we discovered any device?
                            //
                            if(g_ui8DevFound)
                            {
                                //
                                // Got DiscoveryDone response, go to the next state
                                //
                                g_iState = STATE_SET_PARAM;
                            }
                            else
                            {
                                //
                                // No device found:
                                // Stop drawing the circles.
                                //
                                HWREGBITW(&g_ui32Flags, FLAG_DRAW_CIRCLE) = 0;

                                //
                                // show the scan button on the bottom of the
                                // screen in order to repeat the scanning.
                                //
                                UpdateDisplay(iScan);
                            }
                            g_bDiscoveryReq = false;
                        }
                    }
                }
                break;

            case STATE_SET_PARAM:
                if(SetParam())
                {
                    //
                    // Configure parameters are successful,
                    // go to the next state
                    //
                    g_iState = STATE_READY_FOR_LINK_REQ;
                    UARTprintf("Discovery done\n");

                    HWREGBITW(&g_ui32Flags, FLAG_DRAW_CIRCLE) = 0;
                    UpdateDisplay(iConnect);
                }
                else
                {
                    //
                    // Failed to configure the parameters,
                    // go to the error state
                    //
                    g_iState = STATE_ERROR;
                }
                break;

            case STATE_READY_FOR_LINK_REQ:
                //
                // Wait for user to connect any device
                //
                if(g_bEstLinkReq == true)
                {
                    //
                    // Received connect command from user,
                    // go to the next state to connect the device
                    //
                    UpdateDisplay(iConnecting);
                    g_iState = STATE_LINK;
                    g_bEstLinkReq = false;
                }

                //
                // Wait for user to do discovery/scan again
                //
                if(g_bDiscoveryReq == true)
                {
                    //
                    // Discovery/scan is requested by user
                    //
                    g_iState = STATE_START_DISCOVERY;

                    //
                    // Update display
                    //
                    UpdateDisplay(iScanning);
                }
                break;

            case STATE_LINK:
                //
                // Connect to the device
                //
                if( EstablishLink(g_ui8DevConnect))
                {
                    //
                    // Connected the device without errors.
                    // go to the next state.
                    //
                    g_iState = STATE_LINKED;

                    //
                    // Display the sensor information.
                    //
                    UpdateDisplay(iDisconnect);
                }
                else
                {
                    //
                    // Link failed, go back to STATE_READY_FOR_LINK_REQ.
                    //
                    UARTprintf("Link failed, go back to ready for link state\n");
                    g_iState = STATE_READY_FOR_LINK_REQ;
                    UpdateDisplay(iConnect);
                }
                break;

            case STATE_LINKED:
                //
                // Handle Terminate request if any
                //
                if(g_bTermLinkReq == true)
                {
                    g_iState = STATE_TERM;
                    UpdateDisplay(iDisconnecting);
                    break;
                }

                //
                // Configure the sensor profiles.
                //
                ConfigureSensors();

                //
                // Check for any sensor notify event
                //
                CheckForMsg();

                //
                // We will read the device's RSSI every second.
                //
                if(HWREGBITW(&g_ui32Flags, FLAG_EVERY_SECOND) == 1)
                {
                    HWREGBITW(&g_ui32Flags, FLAG_EVERY_SECOND) = 0;

                    //
                    // Read RSSI
                    //
                    HCIReadRSSI(g_ui16Handle);
                }
                break;

            case STATE_TERM:
                //
                // We are told to termniate the link.
                //
                if(TerminateLink())
                {
                    //
                    // Terminate success, go to the next state
                    //
                    g_iState = STATE_TERMED;
                    g_bTermLinkReq = false;
                }
                else
                {
                    //TODO
                    g_iState = STATE_TERMED;
                    g_bTermLinkReq = false;

                }
                break;

            case STATE_TERMED:
                //
                // Terminated, go to the next state
                //
                g_iState = STATE_READY_FOR_LINK_REQ;
                UpdateDisplay(iConnect);
                break;

            case STATE_IDLE:
            default:
                //
                // Check any messages.
                //
                CheckForMsg();
                break;
        }
    }
}
