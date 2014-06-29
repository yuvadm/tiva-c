//*****************************************************************************
//
// lwip_task.c - Tasks to serve web pages over Ethernet using lwIP.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C129X Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "utils/lwiplib.h"
#include "utils/locator.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/httpd.h"
#include "freertos_demo.h"
#include "cgifuncs.h"
#include "led_task.h"
#include "idle_task.h"
#include "lwip_task.h"
#include "spider_task.h"
//*****************************************************************************
//
// This structure contains the details of a SSI tag.
//
//*****************************************************************************
typedef struct
{
    //
    // The text name of the tag.  If the name is "foo", it will appear in the
    // HTML source as "<!--#foo-->".
    //
    const char *pcName;

    //
    // A pointer to the variable that contains the value of this tag.
    //
    uint32_t *pui32Value;
}
tSSITag;

//*****************************************************************************
//
// The list of tags.
//
//*****************************************************************************
const tSSITag g_psTags[] =
{
    { "linksent", (uint32_t *) &lwip_stats.link.xmit },
    { "linkrecv", (uint32_t *) &lwip_stats.link.recv },
    { "linkdrop", (uint32_t *) &lwip_stats.link.drop },
    { "linkcksm", (uint32_t *) &lwip_stats.link.chkerr },
    { "linklen",  (uint32_t *) &lwip_stats.link.lenerr },
    { "linkmem",  (uint32_t *) &lwip_stats.link.memerr },
    { "linkrte",  (uint32_t *) &lwip_stats.link.rterr },
    { "linkprot", (uint32_t *) &lwip_stats.link.proterr },
    { "linkopt",  (uint32_t *) &lwip_stats.link.opterr },
    { "linkmisc", (uint32_t *) &lwip_stats.link.err },
    { "arpsent",  (uint32_t *) &lwip_stats.etharp.xmit },
    { "arprecv",  (uint32_t *) &lwip_stats.etharp.recv },
    { "arpdrop",  (uint32_t *) &lwip_stats.etharp.drop },
    { "arpcksm",  (uint32_t *) &lwip_stats.etharp.chkerr },
    { "arplen",   (uint32_t *) &lwip_stats.etharp.lenerr },
    { "arpmem",   (uint32_t *) &lwip_stats.etharp.memerr },
    { "arprte",   (uint32_t *) &lwip_stats.etharp.rterr },
    { "arpprot",  (uint32_t *) &lwip_stats.etharp.proterr },
    { "arpopt",   (uint32_t *) &lwip_stats.etharp.opterr },
    { "arpmisc",  (uint32_t *) &lwip_stats.etharp.err },
    { "icmpsent", (uint32_t *) &lwip_stats.icmp.xmit },
    { "icmprecv", (uint32_t *) &lwip_stats.icmp.recv },
    { "icmpdrop", (uint32_t *) &lwip_stats.icmp.drop },
    { "icmpcksm", (uint32_t *) &lwip_stats.icmp.chkerr },
    { "icmplen",  (uint32_t *) &lwip_stats.icmp.lenerr },
    { "icmpmem",  (uint32_t *) &lwip_stats.icmp.memerr },
    { "icmprte",  (uint32_t *) &lwip_stats.icmp.rterr },
    { "icmpprot", (uint32_t *) &lwip_stats.icmp.proterr },
    { "icmpopt",  (uint32_t *) &lwip_stats.icmp.opterr },
    { "icmpmisc", (uint32_t *) &lwip_stats.icmp.err },
    { "ipsent",   (uint32_t *) &lwip_stats.ip.xmit },
    { "iprecv",   (uint32_t *) &lwip_stats.ip.recv },
    { "ipdrop",   (uint32_t *) &lwip_stats.ip.drop },
    { "ipcksm",   (uint32_t *) &lwip_stats.ip.chkerr },
    { "iplen",    (uint32_t *) &lwip_stats.ip.lenerr },
    { "ipmem",    (uint32_t *) &lwip_stats.ip.memerr },
    { "iprte",    (uint32_t *) &lwip_stats.ip.rterr },
    { "ipprot",   (uint32_t *) &lwip_stats.ip.proterr },
    { "ipopt",    (uint32_t *) &lwip_stats.ip.opterr },
    { "ipmisc",   (uint32_t *) &lwip_stats.ip.err },
    { "tcpsent",  (uint32_t *) &lwip_stats.tcp.xmit },
    { "tcprecv",  (uint32_t *) &lwip_stats.tcp.recv },
    { "tcpdrop",  (uint32_t *) &lwip_stats.tcp.drop },
    { "tcpcksm",  (uint32_t *) &lwip_stats.tcp.chkerr },
    { "tcplen",   (uint32_t *) &lwip_stats.tcp.lenerr },
    { "tcpmem",   (uint32_t *) &lwip_stats.tcp.memerr },
    { "tcprte",   (uint32_t *) &lwip_stats.tcp.rterr },
    { "tcpprot",  (uint32_t *) &lwip_stats.tcp.proterr },
    { "tcpopt",   (uint32_t *) &lwip_stats.tcp.opterr },
    { "tcpmisc",  (uint32_t *) &lwip_stats.tcp.err },
    { "udpsent",  (uint32_t *) &lwip_stats.udp.xmit },
    { "udprecv",  (uint32_t *) &lwip_stats.udp.recv },
    { "udpdrop",  (uint32_t *) &lwip_stats.udp.drop },
    { "udpcksm",  (uint32_t *) &lwip_stats.udp.chkerr },
    { "udplen",   (uint32_t *) &lwip_stats.udp.lenerr },
    { "udpmem",   (uint32_t *) &lwip_stats.udp.memerr },
    { "udprte",   (uint32_t *) &lwip_stats.udp.rterr },
    { "udpprot",  (uint32_t *) &lwip_stats.udp.proterr },
    { "udpopt",   (uint32_t *) &lwip_stats.udp.opterr },
    { "udpmisc",  (uint32_t *) &lwip_stats.udp.err },
    { "ledrate",  (uint32_t *) &g_ui32LEDDelay },
    { "spider",   (uint32_t *) g_pui32SpiderDelay }
};

//*****************************************************************************
//
// The number of tags.
//
//*****************************************************************************
#define NUM_TAGS                (sizeof(g_psTags) / sizeof(g_psTags[0]))

//*****************************************************************************
//
// An array of names for the tags, as required by the web server.
//
//*****************************************************************************
static const char *g_ppcSSITags[NUM_TAGS];

//*****************************************************************************
//
// The CGI handler for changing the toggle rate of the LED task.
//
//*****************************************************************************
static const char *
ToggleRateCGIHandler(int32_t i32Index, int32_t i32NumParams, char *ppcParam[],
                     char *ppcValue[])
{
    bool bParamError;
    int32_t i32Rate;

    //
    // Get the value of the time parameter.
    //
    bParamError = false;
    i32Rate = GetCGIParam("time", ppcParam, ppcValue, i32NumParams,
                          &bParamError);

    //
    // Return a parameter error if the time parameter was not supplied or was
    // out of range.
    //
    if(bParamError || (i32Rate < 1) || (i32Rate > 10000))
    {
        return("/perror.htm");
    }

    //
    // Update the delay between toggles of the LED.
    //
    g_ui32LEDDelay = i32Rate;

    //
    // Refresh the current page.
    //
    return("/io.ssi");
}

//*****************************************************************************
//
// The CGI handler for changing the spider speed.
//
//*****************************************************************************
static const char *
SpiderSpeedCGIHandler(int32_t i32Index, int32_t i32NumParams, char *ppcParam[],
                      char *ppcValue[])
{
    bool bParamError;
    int32_t i32Rate;

    //
    // Get the value of the time parameter.
    //
    bParamError = false;
    i32Rate = GetCGIParam("time", ppcParam, ppcValue, i32NumParams,
                          &bParamError);

    //
    // Return a parameter error if the time parameter was not supplied or was
    // out of range.
    //
    if(bParamError)
    {
        return("/perror.htm");
    }

    //
    // Update the speed of the spiders.
    //
    SpiderSpeedSet(i32Rate);

    //
    // Refresh the current page.
    //
    return("/io.ssi");
}

//*****************************************************************************
//
// The array of CGI handlers, as required by the web server.
//
//*****************************************************************************
static const tCGI g_psCGIs[] =
{
    { "/toggle_rate.cgi", ToggleRateCGIHandler },
    { "/spider_rate.cgi", SpiderSpeedCGIHandler }
};

//*****************************************************************************
//
// The number of CGI handlers.
//
//*****************************************************************************
#define NUM_CGIS                (sizeof(g_psCGIs) / sizeof(g_psCGIs[0]))

//*****************************************************************************
//
// The handler for server side includes.
//
//*****************************************************************************
static uint16_t
SSIHandler(int32_t i32Index, char *pcInsert, int32_t i32InsertLen)
{
    //
    // Replace the tag with an appropriate value.
    //
    if(i32Index < NUM_TAGS)
    {
        //
        // This is a valid tag, so print out its value.
        //
        usnprintf(pcInsert, i32InsertLen, "%d",
                  *(g_psTags[i32Index].pui32Value));
    }
    else
    {
        //
        // This is an unknown tag.
        //
        usnprintf(pcInsert, i32InsertLen, "??");
    }

    //
    // Determine the length of the replacement text.
    //
    for(i32Index = 0; pcInsert[i32Index] != 0; i32Index++)
    {
    }

    //
    // Return the length of the replacement text.
    //
    return(i32Index);
}

//*****************************************************************************
//
// Sets up the additional lwIP raw API services provided by the application.
//
//*****************************************************************************
void
SetupServices(void *pvArg)
{
    uint8_t pui8MAC[6];
    uint32_t ui32Idx;

    //
    // Setup the device locator service.
    //
    LocatorInit();
    lwIPLocalMACGet(pui8MAC);
    LocatorMACAddrSet(pui8MAC);

    LocatorAppTitleSet("DK-TM4C129X freertos_demo");

    //
    // Initialize the sample httpd server.
    //
    httpd_init();

    //
    // Initialize the tag array used by the web server's SSI processing.
    //
    for(ui32Idx = 0; ui32Idx < NUM_TAGS; ui32Idx++)
    {
        g_ppcSSITags[ui32Idx] = g_psTags[ui32Idx].pcName;
    }

    //
    // Register the SSI tags and handler with the web server.
    //
    http_set_ssi_handler(SSIHandler, g_ppcSSITags, NUM_TAGS);

    //
    // Register the CGI handlers with the web server.
    //
    http_set_cgi_handlers(g_psCGIs, NUM_CGIS);
}

//*****************************************************************************
//
// Initializes the lwIP tasks.
//
//*****************************************************************************
uint32_t
lwIPTaskInit(void)
{
    uint32_t ui32User0, ui32User1;
    uint8_t pui8MAC[6];

    //
    // Get the MAC address from the user registers.
    //
    ROM_FlashUserGet(&ui32User0, &ui32User1);
    if((ui32User0 == 0xffffffff) || (ui32User1 == 0xffffffff))
    {
        return(1);
    }

    //
    // Convert the 24/24 split MAC address from NV ram into a 32/16 split MAC
    // address needed to program the hardware registers, then program the MAC
    // address into the Ethernet Controller registers.
    //
    pui8MAC[0] = ((ui32User0 >>  0) & 0xff);
    pui8MAC[1] = ((ui32User0 >>  8) & 0xff);
    pui8MAC[2] = ((ui32User0 >> 16) & 0xff);
    pui8MAC[3] = ((ui32User1 >>  0) & 0xff);
    pui8MAC[4] = ((ui32User1 >>  8) & 0xff);
    pui8MAC[5] = ((ui32User1 >> 16) & 0xff);

    //
    // Lower the priority of the Ethernet interrupt handler.  This is required
    // so that the interrupt handler can safely call the interrupt-safe
    // FreeRTOS functions (specifically to send messages to the queue).
    //
    ROM_IntPrioritySet(INT_EMAC0, 0xC0);

    //
    // Initialize lwIP.
    //
    lwIPInit(g_ui32SysClock, pui8MAC, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the remaining services inside the TCP/IP thread's context.
    //
    tcpip_callback(SetupServices, 0);

    //
    // Success.
    //
    return(0);
}
