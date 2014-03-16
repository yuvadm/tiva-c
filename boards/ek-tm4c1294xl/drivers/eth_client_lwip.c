//*****************************************************************************
//
// eth_client.c - This is the portion of the ethernet client.
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
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "eth_client_lwip.h"
#include "utils/lwiplib.h"
#include "lwip/dns.h"
#include "lwipopts.h"

#if RTOS_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#endif


//*****************************************************************************
//
// Flag indexes for g_sEnet.ui32Flags
//
//*****************************************************************************
#define FLAG_TIMER_DHCP_EN      0
#define FLAG_TIMER_DNS_EN       1
#define FLAG_TIMER_TCP_EN       2
#define FLAG_DHCP_STARTED       3
#define FLAG_DNS_ADDRFOUND      4

//*****************************************************************************
//
// The current state of the Ethernet connection.
//
//*****************************************************************************
struct
{
    volatile uint32_t ui32Flags;

    //
    // Array to hold the MAC addresses.
    //
    uint8_t pui8MACAddr[8];

    //
    // Global define of the TCP structure used.
    //
    struct tcp_pcb *psTCP;

    //
    // Global IP structure to hold a copy of the IP address.
    //
    struct ip_addr sIPAddr;

    //
    // Global IP structure to hold a copy of the DNS resolved address.
    //
    struct ip_addr sResolvedIP;

    //
    // The saved proxy name as a text string.
    //
    const char *pcProxyName;

    //
    // The port number for the proxy server.
    //
    uint16_t ui16ProxyPort;

    //
    // The saved host name as a text string.
    //
    const char *pcHostName;

    //
    // The port number on the host.
    //
    uint16_t ui16HostPort;

    //
    // The number of bytes to be sent.
    //
    uint32_t ui32SendSize;

    //
    // The index into the send buffer.
    //
    uint32_t ui32SendIndex;

    //
    // Event handler.
    //
    tEventFunction pfnEvent;

    //
    // States.
    //
    volatile enum
    {
        iEthNoConnection,
            iEthDHCPWait,
            iEthDNSWait,
            iEthTCPOpen,
            iEthTCPWait,
            iEthSend,
            iEthIdle
    } eState;
}
g_sEnet;

//*****************************************************************************
//
// Maximum size of a request.
//
//*****************************************************************************
#define MAX_REQUEST             256

//*****************************************************************************
//
// Send buffer config.
//
//*****************************************************************************
uint8_t g_pui8SendBuff[SEND_BUFFER_SIZE];

//*****************************************************************************
//
// Reset the state to a non-connected state to restart dhcp and dns.
//
//*****************************************************************************
static void
ResetConnection(void)
{
    //
    // Nothing to do if already not connected.
    //
    if(g_sEnet.eState != iEthNoConnection)
    {
        //
        // No longer have a link.
        //
        g_sEnet.eState = iEthNoConnection;

        //
        // Reset the flags to just enable the lwIP timer.
        //
        g_sEnet.ui32Flags = (1 << FLAG_TIMER_DHCP_EN);
    }

    //
    // Deallocate the TCP structure if it was already allocated.
    //
    if(g_sEnet.psTCP)
    {
        //
        // Clear out all of the TCP callbacks.
        //
        tcp_sent(g_sEnet.psTCP, NULL);
        tcp_recv(g_sEnet.psTCP, NULL);
        tcp_err(g_sEnet.psTCP, NULL);

        //
        // Close the TCP connection.
        //
        tcp_close(g_sEnet.psTCP);
        g_sEnet.psTCP = 0;
    }
}

//*****************************************************************************
//
// Handler function when the DNS server gets a response or times out.
//
// \param pcName is DNS server name.
// \param psIPAddr is the DNS server's IP address.
// \param vpArg is the configurable argument.
//
// This function is called when the DNS server resolves an IP or times out.
// If the DNS server returns an IP structure that is not NULL, add the IP to
// to the g_sEnet.sResolvedIP IP structure.
//
// \return None.
//
//*****************************************************************************
static void
DNSServerFound(const char *pcName, struct ip_addr *psIPAddr, void *vpArg)
{
    //
    // Check if a valid DNS server address was found.
    //
    if((psIPAddr) && (psIPAddr->addr))
    {
        //
        // Copy the returned IP address into a global IP address.
        //
        g_sEnet.sResolvedIP = *psIPAddr;

        //
        // Tell the main program that a DNS address was found.
        //
        HWREGBITW(&g_sEnet.ui32Flags, FLAG_DNS_ADDRFOUND) = 1;
    }
    else
    {
        //
        // Disable the DNS timer.
        //
        HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_DNS_EN) = 0;
    }
}

//*****************************************************************************
//
//! Handles lwIP TCP/IP errors.
//!
//! \param vPArg is the state data for this connection.
//! \param iErr is the error that was detected.
//!
//! This function is called when the lwIP TCP/IP stack has detected an error.
//! The connection is no longer valid.
//!
//! \return None.
//
//*****************************************************************************
void
TCPError(void *vPArg, err_t iErr)
{
    //
    // Signal event handler that there was an error.
    //
    g_sEnet.pfnEvent(ETH_CLIENT_EVENT_ERROR, 0, (uint32_t)iErr);
}

//*****************************************************************************
//
//! Finalizes the TCP connection in client mode.
//!
//! \param pvArg is the state data for this connection.
//! \param psPcb is the pointer to the TCP control structure.
//! \param psBuf
//! \param iErr is not used in this implementation.
//!
//! This function is called when the lwIP TCP/IP stack has completed a TCP
//! connection.
//!
//! \return This function will return an lwIP defined error code.
//
//*****************************************************************************
err_t
TCPReceived(void *pvArg, struct tcp_pcb *psPcb, struct pbuf *psBuf, err_t iErr)
{
    struct pbuf *psBufCur;

    //
    // Signal event handler that data is available.
    //
    g_sEnet.pfnEvent(ETH_CLIENT_EVENT_RECEIVE, (void *)psBuf->payload,
            (uint32_t)psBuf->len);

    //
    // Indicate that you have received and processed this set of TCP data.
    //
    tcp_recved(psPcb, psBuf->len);

    //
    // Initialize the linked list pointer to parse.
    //
    psBufCur = psBuf;

    //
    // Free the buffers used since they have been processed.
    //
    while(psBufCur->len != 0)
    {
        //
        // Indicate that you have received and processed this set of TCP data.
        //
        tcp_recved(psPcb, psBufCur->len);

        //
        // Go to the next buffer.
        //
        psBufCur = psBufCur->next;

        //
        // Terminate if there are no more buffers.
        //
        if(psBufCur == 0)
        {
            break;
        }
    }

    //
    // Free the memory space allocated for this receive.
    //
    pbuf_free(psBuf);

    //
    // Return.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
//! Handles acknowledgment of data transmitted via Ethernet.
//!
//! \param pvArg is the state data for this connection.
//! \param psPcb is the pointer to the TCP control structure.
//! \param ui16Len is the length of the data transmitted.
//!
//! This function is called when the lwIP TCP/IP stack has received an
//! acknowledgment for data that has been transmitted.
//!
//! \return This function will return an lwIP defined error code.
//
//*****************************************************************************
err_t
TCPSent(void *pvArg, struct tcp_pcb *psPcb, u16_t ui16Len)
{
    //
    // Signal the event handler that data was sent.
    //
    g_sEnet.pfnEvent(ETH_CLIENT_EVENT_SEND, 0, (uint32_t)ui16Len);

    //
    // Return OK.
    //
    return (ERR_OK);
}

//*****************************************************************************
//
//! Finalizes the TCP connection in client mode.
//!
//! \param pvArg is the state data for this connection.
//! \param psPcb is the pointer to the TCP control structure.
//! \param iErr is not used in this implementation.
//!
//! This function is called when the lwIP TCP/IP stack has completed a TCP
//! connection.
//!
//! \return This function will return an lwIP defined error code.
//
//*****************************************************************************
err_t
TCPConnected(void *pvArg, struct tcp_pcb *psPcb, err_t iErr)
{
    //
    // Check if there was a TCP error.
    //
    if(iErr != ERR_OK)
    {
        //
        // Clear out all of the TCP callbacks.
        //
        tcp_sent(psPcb, NULL);
        tcp_recv(psPcb, NULL);
        tcp_err(psPcb, NULL);

        //
        // Close the TCP connection.
        //
        tcp_close(psPcb);

        if(psPcb == g_sEnet.psTCP)
        {
            g_sEnet.psTCP = 0;
        }

        //
        // And return.
        //
        return(ERR_CONN);
    }

    //
    // Setup the TCP receive function.
    //
    tcp_recv(psPcb, TCPReceived);

    //
    // Setup the TCP error function.
    //
    tcp_err(psPcb, TCPError);

    //
    // Setup the TCP sent callback function.
    //
    tcp_sent(psPcb, TCPSent);

    //
    // Signal event handler that connection is established.
    //
    g_sEnet.pfnEvent(ETH_CLIENT_EVENT_CONNECT, 0, 0);

    //
    // Return a success code.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
//! TCP connect
//!
//! This function attempts to connect to a TCP endpoint.
//!
//! \return None.
//
//*****************************************************************************
int32_t
EthClientTCPConnect(void)
{
    err_t eTCPReturnCode;

    //
    // Enable the TCP timer function calls.
    //
    HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_TCP_EN) = 1;

    if(g_sEnet.psTCP)
    {
        //
        // Initially clear out all of the TCP callbacks.
        //
        tcp_sent(g_sEnet.psTCP, NULL);
        tcp_recv(g_sEnet.psTCP, NULL);
        tcp_err(g_sEnet.psTCP, NULL);

        //
        // Make sure there is no lingering TCP connection.
        //
        tcp_close(g_sEnet.psTCP);
    }

    //
    // Create a new TCP socket.
    //
    g_sEnet.psTCP = tcp_new();

    //
    // Check if you need to go through a proxy.
    //
    if(g_sEnet.pcProxyName != 0)
    {
        //
        // Attempt to connect through the proxy server.
        //
        eTCPReturnCode = tcp_connect(g_sEnet.psTCP, &g_sEnet.sResolvedIP,
                g_sEnet.ui16ProxyPort, TCPConnected);
    }
    else
    {
        //
        // Attempt to connect to the server directly.
        //
        eTCPReturnCode = tcp_connect(g_sEnet.psTCP, &g_sEnet.sResolvedIP,
                g_sEnet.ui16HostPort, TCPConnected);
    }

    if((eTCPReturnCode == ERR_OK) || (eTCPReturnCode == ERR_INPROGRESS))
    {
        return(0);
    }
    else
    {
        return(1);
    }
}

//*****************************************************************************
//
//! TCP discconnect
//!
//! This function attempts to disconnect a TCP endpoint.
//!
//! \return None.
//
//*****************************************************************************
void
EthClientTCPDisconnect(void)
{
    g_sEnet.eState = iEthIdle;

    //
    // Reset connection.
    //
    ResetConnection();

    //
    // Disable the TCP timer function calls.
    //
    HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_TCP_EN) = 0;
}

//*****************************************************************************
//
//! Send a request to the server
//!
//! \param pi8Request request to be sent
//! \param ui32Size length of the request to be sent. This is usually the size
//! of the request minus the termination character.
//!
//! This function will send the request to the connected server
//!
//! \return the lwIP error code.
//
//*****************************************************************************
int32_t
EthClientSend(int8_t *pi8Request, uint32_t ui32Size)
{
    uint32_t ui32Index, ui32SendSize, ui32CurrentState;

    //
    // Save off the current number of bytes used in the buffer and the current
    // state.
    //
    ui32SendSize = g_sEnet.ui32SendSize;
    ui32CurrentState = g_sEnet.eState;

    //
    // Check that we have room in the buffer.
    //
    if (ui32SendSize + ui32Size <= SEND_BUFFER_SIZE)
    {
        //
        // Fill the send buffer.
        //
        for (ui32Index = 0; ui32Index < ui32Size; ui32Index++)
        {
            g_pui8SendBuff[ui32SendSize + ui32Index] =
                pi8Request[ui32Index];
        }

        //
        // Increment the number of bytes we have to send.
        //
        g_sEnet.ui32SendSize += ui32Size;

        //
        // Check if we have already sent some data in the buffer.
        // This is determined by checking the number of bytes left to be sent
        // and the current state. If we have then we need to update the index
        // into the buffer.
        //
        if (g_sEnet.ui32SendSize != ui32SendSize &&
            g_sEnet.eState != ui32CurrentState &&
            ui32CurrentState == iEthSend)
        {
            g_sEnet.ui32SendIndex = ui32SendSize;
        }
        else
        {
            g_sEnet.ui32SendIndex = 0;
        }

        //
        // Set the state to Send and send on the next Tick.
        //
        g_sEnet.eState = iEthSend;

        return(ERR_OK);
    }
    else
    {
        //
        // Tell the app we dont have enough memory.
        //
        return(ERR_MEM);
    }
}

//*****************************************************************************
//
//! DHCP connect
//!
//! This function obtains the MAC address from the User registers, starts the
//! DHCP timer and blocks until an IP address is obtained.
//!
//! \return None.
//
//*****************************************************************************
err_t
EthClientDHCPConnect(void)
{
    //
    // Check if the DHCP has already been started.
    //
    if(HWREGBITW(&g_sEnet.ui32Flags, FLAG_DHCP_STARTED) == 0)
    {
        //
        // Set the DCHP started flag.
        //
        HWREGBITW(&g_sEnet.ui32Flags, FLAG_DHCP_STARTED) = 1;
    }
    else
    {
        //
        // If DHCP has already been started, we need to clear the IPs and
        // switch to static.  This forces the LWIP to get new IP address
        // and retry the DHCP connection.
        //
        lwIPNetworkConfigChange(0, 0, 0, IPADDR_USE_STATIC);

        //
        // Restart the DHCP connection.
        //
        lwIPNetworkConfigChange(0, 0, 0, IPADDR_USE_DHCP);
    }

    return ERR_OK;
}

//*****************************************************************************
//
//! Handler function when the DNS server gets a response or times out.
//!
//! This function is called when the DNS server resolves an IP or times out.
//! If the DNS server returns an IP structure that is not NULL, add the IP to
//! to the g_sEnet.sResolvedIP IP structure.
//!
//! \return None.
//
//*****************************************************************************
int32_t
EthClientDNSResolve(void)
{
    err_t iRet;

    if(HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_DNS_EN))
    {
        return(ERR_INPROGRESS);
    }

    //
    // Set DNS config timer to true.
    //
    HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_DNS_EN) = 1;

    //
    // Initialize the host name IP address found flag to false.
    //
    HWREGBITW(&g_sEnet.ui32Flags, FLAG_DNS_ADDRFOUND) = 0;

    //
    // Set state.
    //
    g_sEnet.eState = iEthDNSWait;

    //
    // Resolve host name.
    //
    if(g_sEnet.pcProxyName != 0)
    {
        iRet = dns_gethostbyname(g_sEnet.pcProxyName, &g_sEnet.sResolvedIP,
                DNSServerFound, 0);
    }
    else
    {
        iRet = dns_gethostbyname(g_sEnet.pcHostName, &g_sEnet.sResolvedIP,
                DNSServerFound, 0);
    }

    //
    // If ERR_OK is returned, the local DNS table resolved the host name.  If
    // ERR_INPROGRESS is returned, the DNS request has been queued and will be
    // sent to the DNS server.
    //
    if(iRet == ERR_OK)
    {
        //
        // Stop calling the DNS timer function.
        //
        HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_DNS_EN) = 0;
    }

    //
    // Return host name not found.
    //
    return(iRet);
}

//*****************************************************************************
//
// Returns the weather server IP address for this interface.
//
// This function will read and return the server IP address that is currently
// in use.  This could be the proxy server if the Internet proxy is enabled.
//
// \return Returns the weather server IP address for this interface.
//
//*****************************************************************************
uint32_t
EthClientServerAddrGet(void)
{
    //
    // Return IP.
    //
    return((uint32_t)g_sEnet.sResolvedIP.addr);
}

//*****************************************************************************
//
//! Returns the IP address for this interface.
//!
//! This function will read and return the currently assigned IP address for
//! the Tiva Ethernet interface.
//!
//! \return Returns the assigned IP address for this interface.
//
//*****************************************************************************
uint32_t
EthClientAddrGet(void)
{
    //
    // Return IP.
    //
    return(lwIPLocalIPAddrGet());
}

//*****************************************************************************
//
// Returns the MAC address for the Tiva Ethernet controller.
//
// \param pui8MACAddr is the 6 byte MAC address assigned to the Ethernet
// controller.
//
// This function will read and return the MAC address for the Ethernet
// controller.
//
// \return Returns the weather server IP address for this interface.
//
//*****************************************************************************
void
EthClientMACAddrGet(uint8_t *pui8MACAddr)
{
    int32_t iIdx;

    for(iIdx = 0; iIdx < 6; iIdx++)
    {
        pui8MACAddr[iIdx] = g_sEnet.pui8MACAddr[iIdx];
    }
}

//*****************************************************************************
//
// Set the proxy string for the Ethernet connection.
//
// \param pi8ProxyName is the string used as the proxy server name.
//
// This function sets the current proxy used by the Ethernet connection.  The
// \e pi8ProxyName value can be 0 to indicate that no proxy is in use or it can
// be a pointer to a string that holds the name of the proxy server to use.
// The content of the pointer passed to \e pi8ProxyName should not be changed
// after this call as this function only stores the pointer and does not copy
// the data from this pointer.
//
// \return None.
//
//*****************************************************************************
void
EthClientProxySet(const char *pcProxyName, uint16_t ui16Port)
{
    //
    // Save the new proxy string.
    //
    g_sEnet.pcProxyName = pcProxyName;
    g_sEnet.ui16ProxyPort = ui16Port;

    //
    // Reset the connection on any change to the proxy.
    //
    ResetConnection();
}

void
EthClientHostSet(const char *pcHostName, uint16_t ui16Port)
{
    //
    // Save the new host setting.
    //
    g_sEnet.pcHostName = pcHostName;
    g_sEnet.ui16HostPort = ui16Port;

    //
    // Reset the connection on any change to the host.
    //
    ResetConnection();
}

//*****************************************************************************
//
// Initialize the Ethernet client
//
// This function initializes all the Ethernet components to not configured.
// This tells the SysTick interrupt which timer modules to call.
//
// \return None.
//
//*****************************************************************************
void
EthClientInit(uint32_t ui32SysClock, tEventFunction pfnEvent)
{
    uint32_t ui32User0, ui32User1;

    //
    // Initialize all the Ethernet components to not configured.  This tells
    // the SysTick interrupt which timer modules to call.
    //
    HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_DHCP_EN) = 0;
    HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_DNS_EN) = 0;
    HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_TCP_EN) = 0;

    g_sEnet.eState = iEthNoConnection;
    g_sEnet.pfnEvent = pfnEvent;
    g_sEnet.pcProxyName = 0;

    //
    // Convert the 24/24 split MAC address from NV ram into a 32/16 split MAC
    // address needed to program the hardware registers, then program the MAC
    // address into the Ethernet Controller registers.
    //
    FlashUserGet(&ui32User0, &ui32User1);

    g_sEnet.pui8MACAddr[0] = ((ui32User0 >> 0) & 0xff);
    g_sEnet.pui8MACAddr[1] = ((ui32User0 >> 8) & 0xff);
    g_sEnet.pui8MACAddr[2] = ((ui32User0 >> 16) & 0xff);
    g_sEnet.pui8MACAddr[3] = ((ui32User1 >> 0) & 0xff);
    g_sEnet.pui8MACAddr[4] = ((ui32User1 >> 8) & 0xff);
    g_sEnet.pui8MACAddr[5] = ((ui32User1 >> 16) & 0xff);

    //
    // Initialize lwIP with the system clock, MAC and use DHCP.
    //
    lwIPInit(ui32SysClock, g_sEnet.pui8MACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Start lwIP tick interrupt.
    //
    HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_DHCP_EN) = 1;
}

//*****************************************************************************
//
// Periodic Tick for the Ethernet client
//
// This function is the needed periodic tick for the Ethernet client. It needs
// to be called periodically through the use of a timer or systick.
//
// \return None.
//
//*****************************************************************************
#if NO_SYS
void
EthClientTick(uint32_t ui32TickMS)
{
    if(HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_DHCP_EN))
    {
        lwIPTimer(ui32TickMS);
    }
}
#endif // #if NO_SYS
//*****************************************************************************
//
// Required by lwIP library to support any host-related timer functions.
//
//*****************************************************************************

void
lwIPHostTimerHandler(void)
{
    uint32_t ui32IPAddr;
    err_t eError;
#if NO_SYS
    if(HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_DNS_EN))
    {
        dns_tmr();
    }

    if(HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_TCP_EN))
    {
        tcp_tmr();
    }
#endif // #if NO_SYS

    //
    // Check if we need to send.
    //
    if(g_sEnet.eState == iEthSend)
    {
        //
        // Queue the send.
        //
        eError = tcp_write(g_sEnet.psTCP,
                           g_pui8SendBuff + g_sEnet.ui32SendIndex,
                           g_sEnet.ui32SendSize, TCP_WRITE_FLAG_COPY);

        //
        //  Write data for sending (but does not send it immediately).
        //
        if(eError == ERR_OK)
        {
            //
            // Find out what we can send and send it.
            //
            tcp_output(g_sEnet.psTCP);

            //
            // No more data to send.
            //
            g_sEnet.ui32SendSize = 0;

        }

        //
        // Set state to Idle
        //
        g_sEnet.eState = iEthIdle;
    }

    //
    // Check for loss of link.
    //
    else if((g_sEnet.eState != iEthNoConnection) &&
            (lwIPLocalIPAddrGet() == 0xffffffff))
    {
        //
        // Reset the connection due to a loss of link.
        //
        ResetConnection();

        //
        // Signal a disconnect event.
        //
        g_sEnet.pfnEvent(ETH_CLIENT_EVENT_DISCONNECT, 0, 0);
    }
    else if(g_sEnet.eState == iEthNoConnection)
    {
        //
        // Once link is detected start DHCP.
        //
        if(lwIPLocalIPAddrGet() != 0xffffffff)
        {
            EthClientDHCPConnect();
            g_sEnet.eState = iEthDHCPWait;
        }
    }
    else if(g_sEnet.eState == iEthDHCPWait)
    {
        //
        // Get IP address.
        //
        ui32IPAddr = lwIPLocalIPAddrGet();

        //
        // If IP Address has not yet been assigned, update the display
        // accordingly.
        //
        if((ui32IPAddr != 0xffffffff) && (ui32IPAddr != 0))
        {
            //
            // Update the DHCP IP address.
            //
            g_sEnet.sIPAddr.addr = ui32IPAddr;
            g_sEnet.eState = iEthIdle;

            //
            // Stop DHCP timer since an address has been provided.
            //
            HWREGBITW(&g_sEnet.ui32Flags, FLAG_DHCP_STARTED) = 0;

            //
            // Signal a connect event.
            //
            g_sEnet.pfnEvent(ETH_CLIENT_EVENT_DHCP, &g_sEnet.sIPAddr.addr, 4);
        }
    }
    else if(g_sEnet.eState == iEthDNSWait)
    {
        //
        // Check to see if the DNS timer has been turned off, which signals
        // that the DNS lookup has failed.
        //
        if(HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_DNS_EN) == 0)
        {
            //
            // Go back to idle.
            //
            g_sEnet.eState = iEthIdle;

            //
            // Signal failure.
            //
            g_sEnet.pfnEvent(ETH_CLIENT_EVENT_DNS, 0, 0);
        }

        //
        // Check if the host name was resolved.
        //
        if(HWREGBITW(&g_sEnet.ui32Flags, FLAG_DNS_ADDRFOUND))
        {
            //
            // Stop calling the DNS timer function.
            //
            HWREGBITW(&g_sEnet.ui32Flags, FLAG_TIMER_DNS_EN) = 0;

            //
            // Go back to idle.
            //
            g_sEnet.eState = iEthIdle;

            //
            // Notify the main routine of the new Ethernet connection.
            //
            g_sEnet.pfnEvent(ETH_CLIENT_EVENT_DNS, &g_sEnet.sResolvedIP.addr,
                             4);
        }
    }
}
