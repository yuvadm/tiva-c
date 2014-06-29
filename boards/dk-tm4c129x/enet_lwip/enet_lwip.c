//*****************************************************************************
//
// enet_lwip.c - Sample WebServer Application using lwIP.
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
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/httpd.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "enet_fs.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Ethernet with lwIP (enet_lwip)</h1>
//!
//! This example application demonstrates the operation of the Tiva
//! Ethernet controller using the lwIP TCP/IP Stack configured to operate as
//! an HTTP file server (web server).  DHCP is used to obtain an Ethernet
//! address.  If DHCP times out without obtaining an address, AUTOIP will be
//! used to obtain a link-local address.  The address that is selected will be
//! shown on the QVGA display.
//!
//! The file system code will first check to see if an SD card has been plugged
//! into the microSD slot.  If so, all file requests from the web server will
//! be directed to the SD card.  Otherwise, a default set of pages served up
//! by an internal file system will be used.  Source files for the internal
//! file system image can be found in the ``fs'' directory.  If any of these
//! files are changed, the file system image (enet_fsdata.h) should be
//! rebuilt using the command:
//!
//!     ../../../../tools/bin/makefsfile -i fs -o enet_fsdata.h -r -h -q
//!
//! For additional details on lwIP, refer to the lwIP web page at:
//! http://savannah.nongnu.org/projects/lwip/
//
//*****************************************************************************

//*****************************************************************************
//
// Defines for setting up the system clock.
//
//*****************************************************************************
#define SYSTICKHZ               100
#define SYSTICKMS               (1000 / SYSTICKHZ)

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
// The positions of the circles in the animation used while waiting for an IP
// address.
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
// The colors of the circles in the animation used while waiting for an IP
// address.
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
// The current color index for the animation used while waiting for an IP
// address.
//
//*****************************************************************************
uint32_t g_ui32ColorIdx;

//*****************************************************************************
//
// The current IP address.
//
//*****************************************************************************
uint32_t g_ui32IPAddress;

//*****************************************************************************
//
// The application's graphics context.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// The system clock frequency.  Used by the SD card driver.
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
// Display an lwIP type IP Address.
//
//*****************************************************************************
void
DisplayIPAddress(uint32_t ui32Addr, uint32_t ui32Col, uint32_t ui32Row)
{
    char pcBuf[16];

    //
    // Convert the IP Address into a string.
    //
    usprintf(pcBuf, "%d.%d.%d.%d", ui32Addr & 0xff, (ui32Addr >> 8) & 0xff,
             (ui32Addr >> 16) & 0xff, (ui32Addr >> 24) & 0xff);

    //
    // Display the string.
    //
    GrStringDraw(&g_sContext, pcBuf, -1, ui32Col, ui32Row, false);
}

//*****************************************************************************
//
// Required by lwIP library to support any host-related timer functions.
//
//*****************************************************************************
void
lwIPHostTimerHandler(void)
{
    uint32_t ui32Idx, ui32NewIPAddress, ui32Width, ui32Height;
    tRectangle sRect;

    //
    // Get the width and height of the display.
    //
    ui32Width = GrContextDpyWidthGet(&g_sContext);
    ui32Height = GrContextDpyHeightGet(&g_sContext);

    //
    // Get the current IP address.
    //
    ui32NewIPAddress = lwIPLocalIPAddrGet();

    //
    // See if the IP address has changed.
    //
    if(ui32NewIPAddress != g_ui32IPAddress)
    {
        //
        // Clear the display.
        //
        sRect.i16XMin = 0;
        sRect.i16YMin = 0;
        sRect.i16XMax = ui32Width - 1;
        sRect.i16YMax = ui32Height - 1;
        GrContextForegroundSet(&g_sContext, ClrBlack);
        GrRectFill(&g_sContext, &sRect);
        GrContextForegroundSet(&g_sContext, ClrWhite);

        //
        // See if there is an IP address assigned.
        //
        if(ui32NewIPAddress == 0xffffffff)
        {
            //
            // Indicate that there is no link.
            //
            GrStringDrawCentered(&g_sContext, "Waiting for link", -1,
                                 ui32Width / 2, (ui32Height / 2) - 18, false);
        }
        else if(ui32NewIPAddress == 0)
        {
            //
            // There is no IP address, so indicate that the DHCP process is
            // running.
            //
            GrStringDrawCentered(&g_sContext, "Waiting for IP address", -1,
                                 ui32Width / 2, (ui32Height / 2) - 18, false);
        }
        else
        {
            //
            // Display the new IP address information.
            //
            GrStringDraw(&g_sContext, "IP Address:", -1, (ui32Width / 2) - 110,
                         (ui32Height / 2) + 8 - 10 - 20, false);
            GrStringDraw(&g_sContext, "Subnet Mask:", -1,
                         (ui32Width / 2) - 110, (ui32Height / 2) + 8 - 10,
                         false);
            GrStringDraw(&g_sContext, "Gateway:", -1, (ui32Width / 2) - 110,
                         (ui32Height / 2) + 8 - 10 + 20, false);
            DisplayIPAddress(ui32NewIPAddress, ui32Width / 2,
                             (ui32Height / 2) + 8 - 10 - 20);
            DisplayIPAddress(lwIPLocalNetMaskGet(), ui32Width / 2,
                             (ui32Height / 2) + 8 - 10);
            DisplayIPAddress(lwIPLocalGWAddrGet(), ui32Width / 2,
                             (ui32Height / 2) + 8 - 10 + 20);
        }

        //
        // Save the new IP address.
        //
        g_ui32IPAddress = ui32NewIPAddress;
    }

    //
    // If there is not an IP address, draw the animated circle.
    //
    if((ui32NewIPAddress == 0) || (ui32NewIPAddress == 0xffffffff))
    {
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
                         (ui32Width / 2) + g_ppi32CirclePos[ui32Idx][0],
                         (ui32Height / 2) + g_ppi32CirclePos[ui32Idx][1] + 24,
                         2);
        }

        //
        // Increment the color index.
        //
        g_ui32ColorIdx++;
    }
}

//*****************************************************************************
//
// The interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Call the lwIP timer handler.
    //
    lwIPTimer(SYSTICKMS);

    //
    // Run the file system tick handler.
    //
    fs_tick(SYSTICKMS);
}

//*****************************************************************************
//
// This example demonstrates the use of the Ethernet Controller.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32User0, ui32User1;
    uint8_t pui8MACArray[8];

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
    FrameDraw(&g_sContext, "enet-lwip");

    //
    // Configure SysTick for a periodic interrupt.
    //
    ROM_SysTickPeriodSet(g_ui32SysClock / SYSTICKHZ);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Initialize the file system.
    //
    fs_init();

    //
    // Configure the hardware MAC address for Ethernet Controller filtering of
    // incoming packets.  The MAC address will be stored in the non-volatile
    // USER0 and USER1 registers.
    //
    ROM_FlashUserGet(&ui32User0, &ui32User1);
    if((ui32User0 == 0xffffffff) || (ui32User1 == 0xffffffff))
    {
        //
        // We should never get here.  This is an error if the MAC address has
        // not been programmed into the device.  Exit the program.
        //
        GrContextForegroundSet(&g_sContext, ClrRed);
        GrStringDrawCentered(&g_sContext, "MAC Address", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2,
                             (GrContextDpyHeightGet(&g_sContext) / 2) - 4,
                             false);
        GrStringDrawCentered(&g_sContext, "Not Programmed!", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2,
                             (GrContextDpyHeightGet(&g_sContext) / 2) + 16,
                             false);
        while(1)
        {
        }
    }

    //
    // Convert the 24/24 split MAC address from NV ram into a 32/16 split MAC
    // address needed to program the hardware registers, then program the MAC
    // address into the Ethernet Controller registers.
    //
    pui8MACArray[0] = ((ui32User0 >>  0) & 0xff);
    pui8MACArray[1] = ((ui32User0 >>  8) & 0xff);
    pui8MACArray[2] = ((ui32User0 >> 16) & 0xff);
    pui8MACArray[3] = ((ui32User1 >>  0) & 0xff);
    pui8MACArray[4] = ((ui32User1 >>  8) & 0xff);
    pui8MACArray[5] = ((ui32User1 >> 16) & 0xff);

    //
    // Initialze the lwIP library, using DHCP.
    //
    lwIPInit(g_ui32SysClock, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pui8MACArray);
    LocatorAppTitleSet("DK-TM4C129X enet_lwip");

    //
    // Initialize the sample httpd server.
    //
    httpd_init();

    //
    // Set the interrupt priorities.  We set the SysTick interrupt to a higher
    // priority than the Ethernet interrupt to ensure that the file system
    // tick is processed if SysTick occurs while the Ethernet handler is being
    // processed.  This is very likely since all the TCP/IP and HTTP work is
    // done in the context of the Ethernet interrupt.
    //
    ROM_IntPrioritySet(INT_EMAC0, ETHERNET_INT_PRIORITY);
    ROM_IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);

    //
    // Loop forever.  All the work is done in interrupt handlers.
    //
    while(1)
    {
    }
}
