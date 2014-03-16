//*****************************************************************************
//
// exosite_hal_lwip.c - Abstraction Layer between exosite and eth_client_lwip.
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
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/eeprom.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "drivers/eth_client_lwip.h"
#include "drivers/http.h"
#include "utils/ringbuf.h"
#include "utils/ustdlib.h"
#include "exosite.h"
#include "exosite_hal_lwip.h"
#include "exosite_meta.h"
#include "lwipopts.h"

#if RTOS_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#endif

//*****************************************************************************
//
// Defines for setting up the system clock.
//
//*****************************************************************************
#define SYSTICKHZ 100
#define SYSTICKMS (1000 / SYSTICKHZ)
#define SYSTICKUS (1000000 / SYSTICKHZ)
#define SYSTICKNS (1000000000 / SYSTICKHZ)

//*****************************************************************************
//
// Interrupt priority definitions.  The top 3 bits of these values are
// significant with lower values indicating higher priority interrupts.
//
//*****************************************************************************
#define SYSTICK_INT_PRIORITY    0x80
#define ETHERNET_INT_PRIORITY   0xC0

//*****************************************************************************
//
// Proxy info
//
//*****************************************************************************
bool g_bUseProxy = false;
char g_pcProxyAddress[50];
uint16_t g_ui16ProxyPort = 0;

//*****************************************************************************
//
// System clock speed.
//
//*****************************************************************************
extern uint32_t g_ui32SysClock;

//*****************************************************************************
//
// Buffer used to hold proxy request message, and size of proxy request
// message.
//
//*****************************************************************************
char g_pcRequest[256] = {0};
uint8_t g_ui8RequestSize = 0;

//*****************************************************************************
//
// IP address.
//
//*****************************************************************************
char g_pcIPAddr[20];

//*****************************************************************************
//
// The current state of the exosite connection.
//
//*****************************************************************************
struct
{
    //
    // Flags used by the application.
    //
    volatile uint32_t ui32Flags;

    //
    // Client ID used to identify this client on the server/broker.
    //
    char *pcClientID;

    //
    // Server/broker name.
    //
    char *pcServer;

    //
    // Event handler for EXOSITE events.
    //
    tExositeEventHandler pfnEventHandler;

    //
    // States.
    //
    volatile enum
    {
        EXOSITE_STATE_NOT_CONNECTED,
            EXOSITE_STATE_CONNECTED_IDLE,
            EXOSITE_STATE_PROXY_WAIT,
    } eState;
}
g_sExosite;

//*****************************************************************************
//
// EERPROM status.
//
//*****************************************************************************
uint32_t g_ui32EEStatus = 0;

//*****************************************************************************
//
// Receive buffer config.
//
//*****************************************************************************
tRingBufObject g_sEnetBuffer;
uint8_t g_ui8Data[RECEIVE_BUFFER_SIZE];

//*****************************************************************************
//
// The interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Call into Ethernet client layer.
    //
#if NO_SYS
    EthClientTick(10);
#endif

}

//*****************************************************************************
//
//! Enables the memory used to store any meta data.
//!
//! This function enables the EEPROM to be used to store any meta data.
//!
//! \return None.
//
//*****************************************************************************
void
exoHAL_EnableMeta(void)
{
    //
    // Enable the EEPROM peripheral.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);

    //
    // Initialize the EEPROM
    //
    EEPROMInit();

    //
    // Indicate that the EEPROM is now initalized.
    //
    g_ui32EEStatus = EEPROM_INITALIZED;
}

//*****************************************************************************
//
//! Function is a simple return to maintain compatibility with exostie.c/h.
//!
//! This function just returns.
//!
//! \return None.
//
//*****************************************************************************
void
exoHAL_EraseMeta(void)
{
    return;
}

//*****************************************************************************
//
//! Write meta information to the nonvolatile memory (EEPROM).
//!
//! \param pucBuffer - string buffer containing info to write to meta.
//! \param iLength - size of string in bytes.
//! \param iOffset - offset from base of meta location to store the item.
//!
//! This function stores information to the NV meta structure.
//!
//! \return None.
//
//*****************************************************************************
void
exoHAL_WriteMetaItem(unsigned char * pucBuffer, int iLength,
        int iOffset)
{
    //
    // Make sure EEPROM is initialized.
    //
    if (g_ui32EEStatus == EEPROM_IDLE || g_ui32EEStatus == EEPROM_INITALIZED)
    {
        //
        // Set EEPROM status to erasing.
        //
        g_ui32EEStatus = EEPROM_WRITING;

        //
        // Write the info to the EEPROM.
        //
        EEPROMProgram((uint32_t *)pucBuffer,
                (uint32_t)(EXOMETA_ADDR_OFFSET + iOffset), (uint32_t)iLength);

        //
        // Set EEPROM status to IDLE.
        //
        g_ui32EEStatus = EEPROM_IDLE;
    }
    else
    {
        //
        // Set EEPROM status to ERROR.
        //
        g_ui32EEStatus = EEPROM_ERROR;
    }
}

//*****************************************************************************
//
//! Read meta information from the nonvolatile memory (EEPROM).
//!
//! \param pucBuffer - buffer we can read meta info into.
//! \param iLength - size of the buffer (max 256 bytes).
//! \param iOffset - offset from base of meta to begin reading from.
//!
//! This function reads information from the NV meta structure.
//!
//! \return None.
//
//*****************************************************************************
void
exoHAL_ReadMetaItem(unsigned char * pucBuffer, int iLength,
        int iOffset)
{
    //
    // Make sure the EEPROM is initialized and idle.
    //
    if (g_ui32EEStatus == EEPROM_IDLE || g_ui32EEStatus == EEPROM_INITALIZED)
    {
        //
        // Indicate that the EEPROM is now being read.
        //
        g_ui32EEStatus = EEPROM_READING;

        //
        // Read the requested data.
        //
        EEPROMRead((uint32_t *)pucBuffer,
                (uint32_t)(EXOMETA_ADDR_OFFSET + iOffset), (uint32_t)iLength);

        //
        // Set EEPROM status to IDLE.
        //
        g_ui32EEStatus = EEPROM_IDLE;
    }
    else
    {
        //
        // Set EEPROM status to ERROR.
        //
        g_ui32EEStatus = EEPROM_ERROR;
    }
}

//*****************************************************************************
//
// Reset the connection state.
//
//*****************************************************************************
void
exoHAL_ResetConnection(void)
{
    //
    // Reset flags.
    //
    HWREGBITW(&g_sExosite.ui32Flags, FLAG_CONNECT_WAIT) = 0;
    HWREGBITW(&g_sExosite.ui32Flags, FLAG_CONNECTED) = 0;
    HWREGBITW(&g_sExosite.ui32Flags, FLAG_RECEIVED) = 0;
    HWREGBITW(&g_sExosite.ui32Flags, FLAG_SENT) = 0;
    HWREGBITW(&g_sExosite.ui32Flags, FLAG_BUSY) = 0;
    HWREGBITW(&g_sExosite.ui32Flags, FLAG_PROXY_SET) = 0;

    //
    // Empty the receive buffer.
    //
    RingBufFlush(&g_sEnetBuffer);

    //
    // Reset state.
    //
    g_sExosite.eState = EXOSITE_STATE_NOT_CONNECTED;
}

//*****************************************************************************
//
// Constructs proxy CONNECT request.
//
//*****************************************************************************
static void
exoHAL_ExositeConstructProxyRequest(void)
{
    char pcTemp[128];

    //
    // Construct the request.
    //
    usprintf(pcTemp, "%s:%d" ,EXOSITE_ADDRESS, EXOSITE_PORT);
    HTTPMessageTypeSet(g_pcRequest, HTTP_MESSAGE_CONNECT, pcTemp);

    //
    // Count the number of bytes in the transfer.
    //
    g_ui8RequestSize = strlen(g_pcRequest);
}

//*****************************************************************************
//
// Network events handler.
//
//*****************************************************************************
void
exoHAL_ExositeEnetEvents(uint32_t ui32Event, void *pvData, uint32_t ui32Param)
{
    uint32_t ui32NumHeaders;
    uint8_t *pD = (uint8_t *)pvData;

    //
    // Handle events from the Ethernet client layer.
    //
    switch(ui32Event)
    {
        //
        // Ethernet client has received data.
        //
        case ETH_CLIENT_EVENT_RECEIVE:
        {
            //
            // Set the RECEIVED flag if we have a minimum of 50 bytes received.
            //
            if (ui32Param >= 50)
            {
                HWREGBITW(&g_sExosite.ui32Flags, FLAG_RECEIVED) = 1;
            }

            if (RingBufFree(&g_sEnetBuffer) >= ui32Param)
            {
                //
                // Write to the ring buffer.
                //
                RingBufWrite(&g_sEnetBuffer, pD, ui32Param);
            }
            else
            {
            }

            //
            // Handle the received data based on the state of the EXOSITE
            // transfer.
            //
            switch(g_sExosite.eState)
            {
                //
                // Idle state.
                //
                case EXOSITE_STATE_CONNECTED_IDLE:
                {
                    break;
                }

                //
                // Waiting for the proxy connect request to complete.
                //
                case EXOSITE_STATE_PROXY_WAIT:
                {
                    //
                    // Check the response.
                    //
                    if(!HTTPResponseParse((char *)pD, g_pcRequest, 
                                          (uint32_t *)&ui32NumHeaders))
                    {
                        break;
                    }

                    if (ustrncmp((char *)pD, "200", 3))
                    {
                        //
                        // Empty the receive buffer.
                        //
                        RingBufFlush(&g_sEnetBuffer);

                        //
                        // Set the connected flag.
                        //
                        HWREGBITW(&g_sExosite.ui32Flags, FLAG_CONNECTED) = 1;

                        //
                        // Clear the RECEIVED flag.
                        //
                        HWREGBITW(&g_sExosite.ui32Flags, FLAG_RECEIVED) = 0;

                        //
                        // IDLE state.
                        //
                        g_sExosite.eState = EXOSITE_STATE_CONNECTED_IDLE;

                        break;
                    }
                    break;
                }

                default:
                break;
            }
            break;
        }

        //
        // Ethernet client has connected to the specified server/host and port.
        //
        case ETH_CLIENT_EVENT_CONNECT:
        {
            //
            // If there is a proxy, establish the connection.
            //
            if(g_bUseProxy && HWREGBITW(&g_sExosite.ui32Flags, FLAG_PROXY_SET))
            {
                //
                // Construct the CONNECT request.
                //
                exoHAL_ExositeConstructProxyRequest();
                EthClientSend((int8_t *)g_pcRequest, g_ui8RequestSize);

                //
                // Wait for callback from the CONNECT request.
                //
                g_sExosite.eState = EXOSITE_STATE_PROXY_WAIT;
            }
            else
            {
                //
                // Set the connected flag.
                //
                HWREGBITW(&g_sExosite.ui32Flags, FLAG_CONNECTED) = 1;
            }

            break;
        }
        //
        // Ethernet client has obtained IP address via DHCP.
        //
        case ETH_CLIENT_EVENT_DHCP:
        {
            break;
        }

        //
        // Ethernet client has received DNS response.
        //
        case ETH_CLIENT_EVENT_DNS:
        {
            if(ui32Param != 0)
            {
                //
                // If DNS resolved successfully, initialize the socket and
                // stack.
                //
                EthClientTCPConnect();
            }
            else
            {
                //
                // Clear the busy flag.
                //
                HWREGBITW(&g_sExosite.ui32Flags, FLAG_BUSY) = 0;

                //
                // Clear the DNS flag.
                //
                HWREGBITW(&g_sExosite.ui32Flags, FLAG_DNS_INIT) = 0;

                //
                // Update state machine.
                //
                g_sExosite.eState = EXOSITE_STATE_NOT_CONNECTED;
            }

            break;
        }

        //
        // Ethernet client has disconnected from the server/host.
        //
        case ETH_CLIENT_EVENT_DISCONNECT:
        {
            //
            // Close the socket.
            //
            exoHAL_SocketClose(0);

            break;
        }

        //
        // All other cases are unhandled.
        //
        case ETH_CLIENT_EVENT_SEND:
        {

            break;
        }


        case ETH_CLIENT_EVENT_ERROR:
        {

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
// Exosite init.
//
//*****************************************************************************
void
exoHAL_ExositeInit(void)
{
    if (HWREGBITW(&g_sExosite.ui32Flags, FLAG_ENET_INIT) == 0)
    {

#if NO_SYS
        //
        // Configure SysTick for a periodic interrupt.
        //
        SysTickPeriodSet(g_ui32SysClock / SYSTICKHZ);
        SysTickEnable();
        SysTickIntEnable();

        //
        // Turn on interrupts.
        //
        IntMasterEnable();

        //
        // Set the interrupt priorities.  We set the SysTick interrupt to a
        // higher priority than the Ethernet interrupt to ensure that the file
        // system tick is processed if SysTick occurs while the Ethernet
        // handler is being processed.  This is very likely since all the
        // TCP/IP and HTTP work is done in the context of the Ethernet
        // interrupt.
        //

        IntPriorityGroupingSet(4);
        IntPrioritySet(INT_EMAC0, ETHERNET_INT_PRIORITY);
        IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);
#endif
        //
        // Initialize the structure to known state.
        //
        g_sExosite.ui32Flags = 0;
        g_sExosite.pcClientID = 0;
        g_sExosite.pcServer = 0;
        g_sExosite.eState = EXOSITE_STATE_NOT_CONNECTED;

        //
        // Initialize the Ethernet Client.
        //
        EthClientInit(g_ui32SysClock, &exoHAL_ExositeEnetEvents);

        //
        // Initialize the write pointer for the circular buffer.
        //
        RingBufInit(&g_sEnetBuffer, g_ui8Data, RECEIVE_BUFFER_SIZE);

        //
        // Set the host address.
        //
        EthClientHostSet(EXOSITE_ADDRESS, EXOSITE_PORT);

        //
        // Signal that we have initialized the Ethernet Client.
        //
        HWREGBITW(&g_sExosite.ui32Flags, FLAG_ENET_INIT) = 1;
    }
}

//*****************************************************************************
//
//! Reads and returns the UUID (MAC).
//!
//! \param ucIfNbr - The interface number (1-WiFi).
//! \param pucUUIDBuf - The buffer to return the hexadecimal MAC.
//!
//! This function reads the MAC address from the hardware.
//!
//! \return 0 if failure. Length of UUID if successful.
//
//*****************************************************************************
int
exoHAL_ReadUUID(unsigned char ucIfNbr, unsigned char * pucUUIDBuf)
{
    uint8_t pui8MACAddr[6];

    //
    // Initialize the Client which initializes the MAC.
    //
    exoHAL_ExositeInit();

    //
    // Get the MAC.
    //
    EthClientMACAddrGet(pui8MACAddr);

    //
    // Fill pucUUIDBuf.
    //
    usprintf((char *)pucUUIDBuf,"%02x%02x%02x%02x%02x%02x",
                                                (char)pui8MACAddr[0],
                                                (char)pui8MACAddr[1],
                                                (char)pui8MACAddr[2],
                                                (char)pui8MACAddr[3],
                                                (char)pui8MACAddr[4],
                                                (char)pui8MACAddr[5]);

    //
    // Return the size of the MAC.
    //
    return sizeof(pucUUIDBuf);
}

//*****************************************************************************
//
// Set the proxy server name and port.
//
//*****************************************************************************
void
exoHAL_ExositeProxySet(char *pcProxy, uint16_t ui16Port)
{
    //
    // Set the proxy server.
    //
    EthClientProxySet((const char *)pcProxy, ui16Port);

    //
    // Set the proxy flag appropriately.
    //
    HWREGBITW(&g_sExosite.ui32Flags, FLAG_PROXY_SET) = 1;

}

//*****************************************************************************
//
//! Closes a socket.
//!
//! \param lSocket - socket handle.
//!
//! This function closes a socket by reseting the state flags in the
//! connection.
//!
//! \return None.
//
//*****************************************************************************
void
exoHAL_SocketClose(long lSocket)
{
    //
    // TCP disconnect.
    //
    EthClientTCPDisconnect();

    //
    // Reset the connection state.
    //
    exoHAL_ResetConnection();
}

//*****************************************************************************
//
//! Opens a TCP socket.
//!
//! \param pucServer - socket handle.
//!
//! This function attempts to obtain an IP. Then configures the client
//! proxy, performs a DNS lookup and loops waiting for a connection. After
//! a set number of tries to connect it returns with an error.
//!
//! \return -1 for failure. else return 0.
//
//*****************************************************************************
long
exoHAL_SocketOpenTCP(unsigned char *pucServer)
{
    uint32_t ui32IPAddr, ui32Timeout;
    int32_t i32Connected, i32Error;

    //
    // Variable to track our while() loop count.
    //
    ui32Timeout = 5;

    //
    // Initialize the exosite connection.
    //
    exoHAL_ExositeInit();

    while(ui32Timeout)
    {
        //
        // Decrement timeout count.
        //
        ui32Timeout--;

        //
        // Get the current IP address.
        //
        ui32IPAddr = EthClientAddrGet();

        //
        // If IP is valid, print IP.
        //
        if(ui32IPAddr == 0 || ui32IPAddr == 0xffffffff)
        {
            //
            // Delay, wait for response and continue.
            //
#if NO_SYS
            SysCtlDelay((g_ui32SysClock / SYSTICKMS) * 10);
#elif RTOS_FREERTOS
            vTaskDelay(10 / portTICK_RATE_MS);
#endif // #if NO_SYS
            continue;
        }
        else
        {

        }

        //
        // Set Proxy if not already.
        //
        if (g_bUseProxy && !HWREGBITW(&g_sExosite.ui32Flags, FLAG_PROXY_SET))
        {
            //
            // Set the proxy defined in exosite_hal_lwip.h
            //
            exoHAL_ExositeProxySet(g_pcProxyAddress, g_ui16ProxyPort);
        }

        //
        // If we are already initiated the DNS no need to do it again.
        //
        if (!HWREGBITW(&g_sExosite.ui32Flags, FLAG_DNS_INIT))
        {
            //
            // Resolve the host address.
            //
            i32Error = EthClientDNSResolve();

            //
            // If error, break.
            //
            if (i32Error != 0 && i32Error != -5)
            {
                break;
            }

            //
            // Set the DNS flag.
            //
            HWREGBITW(&g_sExosite.ui32Flags, FLAG_DNS_INIT) = 1;

            //
            // Set the connect wait flag.
            //
            HWREGBITW(&g_sExosite.ui32Flags, FLAG_CONNECT_WAIT) = 1;
        }
        if (HWREGBITW(&g_sExosite.ui32Flags, FLAG_CONNECT_WAIT) == 0)
        {
            //
            // Try to reconnect.
            //
            i32Error = EthClientTCPConnect();

            //
            // If error, break.
            //
            if (i32Error != 0)
            {
                break;
            }

            //
            // Set the connect wait flag.
            //
            HWREGBITW(&g_sExosite.ui32Flags, FLAG_CONNECT_WAIT) = 1;
        }

        //
        // See if we have connected to Exosite.
        //
        i32Connected = exoHAL_ServerConnect(0);

        //
        // If connected, return success.
        // else delay and decrement timeout.
        //
        if (i32Connected != -1)
        {

            return 0;
        }
        else
        {
            //
            // Delay and wait for response.
            //
#if NO_SYS
            SysCtlDelay(g_ui32SysClock / SYSTICKMS);
#elif RTOS_FREERTOS
            vTaskDelay(1000 / portTICK_RATE_MS);
#endif // #if NO_SYS

        }
    }
    //
    // We failed close the connection.
    //
    exoHAL_SocketClose(0);

    return -1;
}

//*****************************************************************************
//
//! Checks the connection to the server.
//!
//! \param lSocket - socket handle.
//!
//! This function checks the connection to the server.
//!
//! \return -1 for failure. else: the socket handle
//
//*****************************************************************************
long
exoHAL_ServerConnect(long lSocket)
{
    //
    // Check if we have connected to Exosite.
    //
    if (HWREGBITW(&g_sExosite.ui32Flags, FLAG_CONNECTED))
    {
        return lSocket;
    }
    else
    {
        return -1;
    }
}

//*****************************************************************************
//
//! Sends data out to the Internet.
//!
//! \param lSocket - socket handle.
//! \param  pcBuffer - string buffer containing info to send.
//! \param iLength - size of string in bytes.
//!
//! This function sends data out to the Internet.
//!
//! \return Number of bytes sent.
//
//*****************************************************************************
unsigned char
exoHAL_SocketSend(long lSocket, char * pcBuffer, int iLength)
{
    uint32_t ui32IPAddr;

    //
    // Get the current IP address.
    //
    ui32IPAddr = EthClientAddrGet();

    //
    // If IP is invalid return error.
    //
    if(ui32IPAddr == 0 || ui32IPAddr == 0xffffffff )
    {
        return 0;
    }

    if (HWREGBITW(&g_sExosite.ui32Flags, FLAG_CONNECTED))
    {
        //
        // Send.
        //
        EthClientSend((int8_t *)pcBuffer, (uint32_t)iLength);

        //
        // Set the SENT flag.
        //
        HWREGBITW(&g_sExosite.ui32Flags, FLAG_SENT) = 1;

        //
        // Return the number of bytes sent.
        //
        return iLength;
    }
    else
    {
        return 0;
    }
}

//*****************************************************************************
//
//! Returns data from the buffer.
//!
//! \param lSocket - socket handle.
//! \param pcBuffer - string buffer to put info we receive.
//! \param iLength - size of buffer in bytes.
//!
//! This function reads data from the receive buffer.
//!
//! \return Number of bytes received.
//
//*****************************************************************************
unsigned char
exoHAL_SocketRecv(long lSocket, char * pcBuffer, int iLength)
{
    uint32_t ui32Used, ui32Timeout;

    ui32Timeout = 10;

    //
    // Block until we receive our response.
    //
    while(HWREGBITW(&g_sExosite.ui32Flags, FLAG_SENT) == 1 &&
            HWREGBITW(&g_sExosite.ui32Flags, FLAG_RECEIVED) == 0 &&
            ui32Timeout)
    {
        //
        // Decrement timeout
        //
        ui32Timeout--;

        //
        // Delay.
        //
#if NO_SYS
        SysCtlDelay(g_ui32SysClock / 10);
#elif RTOS_FREERTOS
        vTaskDelay(300 / portTICK_RATE_MS);
#endif // #if NO_SYS
    }

    if(ui32Timeout != 0)
    {
        //
        // Determine how much of the buffer have we used.
        //
        ui32Used = RingBufUsed(&g_sEnetBuffer);

        //
        // If the number of bytes being requested is greater than what we have,
        // only read the number of bytes out that we have.
        //
        if (ui32Used < iLength)
        {
            //
            // Read from the buffer.
            //
            RingBufRead(&g_sEnetBuffer, (uint8_t *)pcBuffer, ui32Used);
            return (unsigned char)ui32Used;
        }
        else
        {
            //
            // Read from the buffer.
            //
            RingBufRead(&g_sEnetBuffer, (uint8_t *)pcBuffer,
                        (uint32_t)iLength);
            return (unsigned char)iLength;
        }
    }
    else
    {
        //
        // Timeout.
        //
    }

    return 0;
}

//*****************************************************************************
//
//! Delays for specified milliseconds.
//!
//! \param usDelay - milliseconds to delay.
//!
//! This function delays for specified milliseconds.
//!
//! \return None.
//
//*****************************************************************************
void
exoHAL_MSDelay(unsigned short usDelay)
{
#if NO_SYS
    uint32_t ui32Delay;

    //
    // Determine delay based on the current clock speed.
    //
    ui32Delay = usDelay * ((g_ui32SysClock / SYSTICKMS) / 3);

    //
    // Delay
    //
    SysCtlDelay(ui32Delay);

#elif RTOS_FREERTOS
    //
    // Delay.
    //
    vTaskDelay(usDelay / portTICK_RATE_MS);
#endif // #if NO_SYS

}
