//*****************************************************************************
//
// locator.c - A device locator server using UDP in lwIP.
//
// Copyright (c) 2009-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the Tiva Utility Library.
//
//*****************************************************************************

#include <stdint.h>
#include "utils/locator.h"
#include "utils/lwiplib.h"

//*****************************************************************************
//
//! \addtogroup locator_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// These defines are used to describe the device locator protocol.
//
//*****************************************************************************
#define TAG_CMD                 0xff
#define TAG_STATUS              0xfe
#define CMD_DISCOVER_TARGET     0x02

//*****************************************************************************
//
// An array that contains the device locator response data.  The format of the
// data is as follows:
//
//     Byte        Description
//     --------    ------------------------
//      0          TAG_STATUS
//      1          packet length
//      2          CMD_DISCOVER_TARGET
//      3          board type
//      4          board ID
//      5..8       client IP address
//      9..14      MAC address
//     15..18      firmware version
//     19..82      application title
//     83          checksum
//
//*****************************************************************************
static uint8_t g_pui8LocatorData[84];

//*****************************************************************************
//
// This function is called by the lwIP TCP/IP stack when it receives a UDP
// packet from the discovery port.  It produces the response packet, which is
// sent back to the querying client.
//
//*****************************************************************************
static void
LocatorReceive(void *arg, struct udp_pcb *pcb, struct pbuf *p,
               struct ip_addr *addr, u16_t port)
{
    uint8_t *pui8Data;
    uint32_t ui32Idx;

    //
    // Validate the contents of the datagram.
    //
    pui8Data = p->payload;
    if((p->len != 4) || (pui8Data[0] != TAG_CMD) || (pui8Data[1] != 4) ||
       (pui8Data[2] != CMD_DISCOVER_TARGET) ||
       (pui8Data[3] != ((0 - TAG_CMD - 4 - CMD_DISCOVER_TARGET) & 0xff)))
    {
        pbuf_free(p);
        return;
    }

    //
    // The incoming pbuf is no longer needed, so free it.
    //
    pbuf_free(p);

    //
    // Allocate a new pbuf for sending the response.
    //
    p = pbuf_alloc(PBUF_TRANSPORT, sizeof(g_pui8LocatorData), PBUF_RAM);
    if(p == NULL)
    {
        return;
    }

    //
    // Calculate and fill in the checksum on the response packet.
    //
    for(ui32Idx = 0, g_pui8LocatorData[sizeof(g_pui8LocatorData) - 1] = 0;
        ui32Idx < (sizeof(g_pui8LocatorData) - 1); ui32Idx++)
    {
        g_pui8LocatorData[sizeof(g_pui8LocatorData) - 1] -=
            g_pui8LocatorData[ui32Idx];
    }

    //
    // Copy the response packet data into the pbuf.
    //
    pui8Data = p->payload;
    for(ui32Idx = 0; ui32Idx < sizeof(g_pui8LocatorData); ui32Idx++)
    {
        pui8Data[ui32Idx] = g_pui8LocatorData[ui32Idx];
    }

    //
    // Send the response.
    //
    udp_sendto(pcb, p, addr, port);

    //
    // Free the pbuf.
    //
    pbuf_free(p);
}

//*****************************************************************************
//
//! Initializes the locator service.
//!
//! This function prepares the locator service to handle device discovery
//! requests.  A UDP server is created and the locator response data is
//! initialized to all empty.
//!
//! \return None.
//
//*****************************************************************************
void
LocatorInit(void)
{
    uint32_t ui32Idx;
    void *pcb;

    //
    // Clear out the response data.
    //
    for(ui32Idx = 0; ui32Idx < 84; ui32Idx++)
    {
        g_pui8LocatorData[ui32Idx] = 0;
    }

    //
    // Fill in the header for the response data.
    //
    g_pui8LocatorData[0] = TAG_STATUS;
    g_pui8LocatorData[1] = sizeof(g_pui8LocatorData);
    g_pui8LocatorData[2] = CMD_DISCOVER_TARGET;

    //
    // Fill in the MAC address for the response data.
    //
    g_pui8LocatorData[9] = 0;
    g_pui8LocatorData[10] = 0;
    g_pui8LocatorData[11] = 0;
    g_pui8LocatorData[12] = 0;
    g_pui8LocatorData[13] = 0;
    g_pui8LocatorData[14] = 0;

    //
    // Create a new UDP port for listening to device locator requests.
    //
    pcb = udp_new();
    udp_recv(pcb, LocatorReceive, NULL);
    udp_bind(pcb, IP_ADDR_ANY, 23);
}

//*****************************************************************************
//
//! Sets the board type in the locator response packet.
//!
//! \param ui32Type is the type of the board.
//!
//! This function sets the board type field in the locator response packet.
//!
//! \return None.
//
//*****************************************************************************
void
LocatorBoardTypeSet(uint32_t ui32Type)
{
    //
    // Save the board type in the response data.
    //
    g_pui8LocatorData[3] = ui32Type & 0xff;
}

//*****************************************************************************
//
//! Sets the board ID in the locator response packet.
//!
//! \param ui32ID is the ID of the board.
//!
//! This function sets the board ID field in the locator response packet.
//!
//! \return None.
//
//*****************************************************************************
void
LocatorBoardIDSet(uint32_t ui32ID)
{
    //
    // Save the board ID in the response data.
    //
    g_pui8LocatorData[4] = ui32ID & 0xff;
}

//*****************************************************************************
//
//! Sets the client IP address in the locator response packet.
//!
//! \param ui32IP is the IP address of the currently connected client.
//!
//! This function sets the IP address of the currently connected client in the
//! locator response packet.  The IP should be set to 0.0.0.0 if there is no
//! client connected.  It should never be set for devices that do not have a
//! strict one-to-one mapping of client to server (for example, a web server).
//!
//! \return None.
//
//*****************************************************************************
void
LocatorClientIPSet(uint32_t ui32IP)
{
    //
    // Save the client IP address in the response data.
    //
    g_pui8LocatorData[5] = ui32IP & 0xff;
    g_pui8LocatorData[6] = (ui32IP >> 8) & 0xff;
    g_pui8LocatorData[7] = (ui32IP >> 16) & 0xff;
    g_pui8LocatorData[8] = (ui32IP >> 24) & 0xff;
}

//*****************************************************************************
//
//! Sets the MAC address in the locator response packet.
//!
//! \param pui8MACArray is the MAC address of the network interface.
//!
//! This function sets the MAC address of the network interface in the locator
//! response packet.
//!
//! \return None.
//
//*****************************************************************************
void
LocatorMACAddrSet(uint8_t *pui8MACArray)
{
    //
    // Save the MAC address.
    //
    g_pui8LocatorData[9] = pui8MACArray[0];
    g_pui8LocatorData[10] = pui8MACArray[1];
    g_pui8LocatorData[11] = pui8MACArray[2];
    g_pui8LocatorData[12] = pui8MACArray[3];
    g_pui8LocatorData[13] = pui8MACArray[4];
    g_pui8LocatorData[14] = pui8MACArray[5];
}

//*****************************************************************************
//
//! Sets the firmware version in the locator response packet.
//!
//! \param ui32Version is the version number of the device firmware.
//!
//! This function sets the version number of the device firmware in the locator
//! response packet.
//!
//! \return None.
//
//*****************************************************************************
void
LocatorVersionSet(uint32_t ui32Version)
{
    //
    // Save the firmware version number in the response data.
    //
    g_pui8LocatorData[15] = ui32Version & 0xff;
    g_pui8LocatorData[16] = (ui32Version >> 8) & 0xff;
    g_pui8LocatorData[17] = (ui32Version >> 16) & 0xff;
    g_pui8LocatorData[18] = (ui32Version >> 24) & 0xff;
}

//*****************************************************************************
//
//! Sets the application title in the locator response packet.
//!
//! \param pcAppTitle is a pointer to the application title string.
//!
//! This function sets the application title in the locator response packet.
//! The string is truncated at 64 characters if it is longer (without a
//! terminating 0), and is zero-filled to 64 characters if it is shorter.
//!
//! \return None.
//
//*****************************************************************************
void
LocatorAppTitleSet(const char *pcAppTitle)
{
    uint32_t ui32Count;

    //
    // Copy the application title string into the response data.
    //
    for(ui32Count = 0; (ui32Count < 64) && *pcAppTitle; ui32Count++)
    {
        g_pui8LocatorData[ui32Count + 19] = *pcAppTitle++;
    }

    //
    // Zero-fill the remainder of the space in the response data (if any).
    //
    for(; ui32Count < 64; ui32Count++)
    {
        g_pui8LocatorData[ui32Count + 19] = 0;
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
