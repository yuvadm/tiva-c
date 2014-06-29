//*****************************************************************************
//
// enet_uip.c - Sample WebServer Application for Ethernet Demo
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
#include "inc/hw_emac.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/emac.h"
#include "driverlib/flash.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "uip/uip.h"
#include "uip/uip_arp.h"
#include "httpd/httpd.h"
#include "dhcpc/dhcpc.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Ethernet with uIP (enet_uip)</h1>
//!
//! This example application demonstrates the operation of the Tiva C Series
//! Ethernet controller using the uIP TCP/IP Stack.  DHCP is used to obtain
//! an Ethernet address.  A basic web site is served over the Ethernet port.
//! The web site displays a few lines of text, and a counter that increments
//! each time the page is sent.
//!
//! UART0, connected to the ICDI virtual COM port and running at 115,200,
//! 8-N-1, is used to display messages from this application.
//!
//! For additional details on uIP, refer to the uIP web page at:
//! http://www.sics.se/~adam/uip/
//
//*****************************************************************************

//*****************************************************************************
//
// Defines for setting up the system clock.
//
//*****************************************************************************
#define SYSTICKHZ               CLOCK_CONF_SECOND
#define SYSTICKMS               (1000 / SYSTICKHZ)
#define SYSTICKUS               (1000000 / SYSTICKHZ)
#define SYSTICKNS               (1000000000 / SYSTICKHZ)

//*****************************************************************************
//
// Macro for accessing the Ethernet header information in the buffer.
//
//*****************************************************************************
u8_t g_pui8UIPBuffer[UIP_BUFSIZE + 2];
u8_t *uip_buf = g_pui8UIPBuffer;

#define BUF                     ((struct uip_eth_hdr *)uip_buf)

//*****************************************************************************
//
// Ethernet DMA descriptors.
//
// Although uIP uses a single buffer, the MAC hardware needs a minimum of
// 3 receive descriptors to operate.
//
//*****************************************************************************
#define NUM_TX_DESCRIPTORS 3
#define NUM_RX_DESCRIPTORS 3
tEMACDMADescriptor g_psRxDescriptor[NUM_TX_DESCRIPTORS];
tEMACDMADescriptor g_psTxDescriptor[NUM_RX_DESCRIPTORS];

uint32_t g_ui32RxDescIndex;
uint32_t g_ui32TxDescIndex;

//*****************************************************************************
//
// Transmit and receive buffers.
//
//*****************************************************************************
#define RX_BUFFER_SIZE 1536
#define TX_BUFFER_SIZE 1536
uint8_t g_pui8RxBuffer[RX_BUFFER_SIZE];
uint8_t g_pui8TxBuffer[TX_BUFFER_SIZE];

//*****************************************************************************
//
// A set of flags.  The flag bits are defined as follows:
//
//     0 -> An indicator that a SysTick interrupt has occurred.
//     1 -> An RX Packet has been received.
//     2 -> A TX packet DMA transfer is pending.
//     3 -> A RX packet DMA transfer is pending.
//
//*****************************************************************************
#define FLAG_SYSTICK            0
#define FLAG_RXPKT              1
#define FLAG_TXPKT              2
#define FLAG_RXPKTPEND          3
static volatile uint32_t g_ui32Flags;

//*****************************************************************************
//
// A system tick counter, incremented every SYSTICKMS.
//
//*****************************************************************************
volatile uint32_t g_ui32TickCounter = 0;

//*****************************************************************************
//
// Default TCP/IP Settings for this application.
//
// Default to Link Local address ... (169.254.1.0 to 169.254.254.255).  Note:
// This application does not implement the Zeroconf protocol.  No ARP query is
// issued to determine if this static IP address is already in use.
//
// Uncomment the following #define statement to enable STATIC IP
// instead of DHCP.
//
//*****************************************************************************
//#define USE_STATIC_IP

#ifndef DEFAULT_IPADDR0
#define DEFAULT_IPADDR0         169
#endif

#ifndef DEFAULT_IPADDR1
#define DEFAULT_IPADDR1         254
#endif

#ifndef DEFAULT_IPADDR2
#define DEFAULT_IPADDR2         19
#endif

#ifndef DEFAULT_IPADDR3
#define DEFAULT_IPADDR3         63
#endif

#ifndef DEFAULT_NETMASK0
#define DEFAULT_NETMASK0        255
#endif

#ifndef DEFAULT_NETMASK1
#define DEFAULT_NETMASK1        255
#endif

#ifndef DEFAULT_NETMASK2
#define DEFAULT_NETMASK2        0
#endif

#ifndef DEFAULT_NETMASK3
#define DEFAULT_NETMASK3        0
#endif

//*****************************************************************************
//
// UIP Timers (in MS)
//
//*****************************************************************************
#define UIP_PERIODIC_TIMER_MS   500
#define UIP_ARP_TIMER_MS        10000

//*****************************************************************************
//
// The drawing context used to draw to the screen.
//
//*****************************************************************************
static tContext g_sContext;

//*****************************************************************************
//
// The vertical position of the status string on the LCD display.
//
//*****************************************************************************
#define STATUS_Y                128

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
void
__error__(char *pcFilename, uint32_t ui32Line)
{
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
    // Increment the system tick count.
    //
    g_ui32TickCounter++;

    //
    // Indicate that a SysTick interrupt has occurred.
    //
    HWREGBITW(&g_ui32Flags, FLAG_SYSTICK) = 1;
}

//*****************************************************************************
//
// When using the timer module in UIP, this function is required to return
// the number of ticks.  Note that the file "clock-arch.h" must be provided
// by the application, and define CLOCK_CONF_SECONDS as the number of ticks
// per second, and must also define the typedef "clock_time_t".
//
//*****************************************************************************
clock_time_t
clock_time(void)
{
    return((clock_time_t)g_ui32TickCounter);
}

//*****************************************************************************
//
// Display a status string on the LCD and also transmit it via the serial port.
//
//*****************************************************************************
void
UpdateStatus(char *pcStatus)
{
    tRectangle sRect;

    //
    // Dump that status string to the serial port.
    //
    UARTprintf("%s\n", pcStatus);

    //
    // Clear any previous status message.
    //
    sRect.i16XMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMin = STATUS_Y - 16;
    sRect.i16YMax = STATUS_Y + 16;
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);

    //
    // Display the new status string.
    //
    GrContextFontSet(&g_sContext, g_psFontCmss20);
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrStringDrawCentered(&g_sContext, pcStatus, -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, STATUS_Y, 0);
}

//*****************************************************************************
//
// Display the current IP address on the screen and transmit it via the UART.
//
//*****************************************************************************
void
ShowIPAddress(const uip_ipaddr_t sIPAddr)
{
    char pcBuffer[24];

    usprintf(pcBuffer, "IP: %d.%d.%d.%d", sIPAddr[0] & 0xff,
             sIPAddr[0] >> 8, sIPAddr[1] & 0xff, sIPAddr[1] >> 8);
    UpdateStatus(pcBuffer);
}

//*****************************************************************************
//
// The interrupt handler for the Ethernet interrupt.
//
//*****************************************************************************
void
EthernetIntHandler(void)
{
    uint32_t ui32Temp;

    //
    // Read and Clear the interrupt.
    //
    ui32Temp = ROM_EMACIntStatus(EMAC0_BASE, true);
    ROM_EMACIntClear(EMAC0_BASE, ui32Temp);

    //
    // Check to see if an RX Interrupt has occurred.
    //
    if(ui32Temp & EMAC_INT_RECEIVE)
    {
        //
        // Indicate that a packet has been received.
        //
        HWREGBITW(&g_ui32Flags, FLAG_RXPKT) = 1;
    }

    //
    // Has the DMA finished transferring a packet to the transmitter?
    //
    if(ui32Temp & EMAC_INT_TRANSMIT)
    {
        //
        // Indicate that a packet has been sent.
        //
        HWREGBITW(&g_ui32Flags, FLAG_TXPKT) = 0;
    }
}

//*****************************************************************************
//
// Callback for when DHCP client has been configured.
//
//*****************************************************************************
void
dhcpc_configured(const struct dhcpc_state *s)
{
    uip_sethostaddr(&s->ipaddr);
    uip_setnetmask(&s->netmask);
    uip_setdraddr(&s->default_router);
    ShowIPAddress(s->ipaddr);
}

//*****************************************************************************
//
// Read a packet from the DMA receive buffer into the uIP packet buffer.
//
//*****************************************************************************
int32_t
PacketReceive(uint32_t ui32Base, uint8_t *pui8Buf, int32_t i32BufLen)
{
    int_fast32_t i32FrameLen, i32Loop;

    //
    // Check the arguments.
    //
    ASSERT(ui32Base == EMAC0_BASE);
    ASSERT(pui8Buf != 0);
    ASSERT(i32BufLen > 0);

    //
    // By default, we assume we got a bad frame.
    //
    i32FrameLen = 0;

    //
    // Make sure that we own the receive descriptor.
    //
    if(!(g_psRxDescriptor[g_ui32RxDescIndex].ui32CtrlStatus & DES0_RX_CTRL_OWN))
    {
        //
        // We own the receive descriptor so check to see if it contains a valid
        // frame.  Look for a descriptor error, indicating that the incoming
        // packet was truncated or, if this is the last frame in a packet,
        // the receive error bit.
        //
        if(!(g_psRxDescriptor[g_ui32RxDescIndex].ui32CtrlStatus &
             DES0_RX_STAT_ERR))
        {
            //
            // We have a valid frame so copy the content to the supplied
            // buffer. First check that the "last descriptor" flag is set.  We
            // sized the receive buffer such that it can always hold a valid
            // frame so this flag should never be clear at this point but...
            //
            if(g_psRxDescriptor[g_ui32RxDescIndex].ui32CtrlStatus &
               DES0_RX_STAT_LAST_DESC)
            {
                i32FrameLen =
                    ((g_psRxDescriptor[g_ui32RxDescIndex].ui32CtrlStatus &
                      DES0_RX_STAT_FRAME_LENGTH_M) >>
                     DES0_RX_STAT_FRAME_LENGTH_S);

                //
                // Sanity check.  This shouldn't be required since we sized the
                // uIP buffer such that it's the same size as the DMA receive
                // buffer but, just in case...
                //
                if(i32FrameLen > i32BufLen)
                {
                    i32FrameLen = i32BufLen;
                }

                //
                // Copy the data from the DMA receive buffer into the provided
                // frame buffer.
                //
                for(i32Loop = 0; i32Loop < i32FrameLen; i32Loop++)
                {
                    pui8Buf[i32Loop] = g_pui8RxBuffer[i32Loop];
                }
            }
        }

        //
        // Move on to the next descriptor in the chain.
        //
        g_ui32RxDescIndex++;
        if(g_ui32RxDescIndex == NUM_RX_DESCRIPTORS)
        {
            g_ui32RxDescIndex = 0;
        }

        //
        // Mark the next descriptor in the ring as available for the receiver
        // to write into.
        //
        g_psRxDescriptor[g_ui32RxDescIndex].ui32CtrlStatus = DES0_RX_CTRL_OWN;
    }

    //
    // Return the Frame Length
    //
    return(i32FrameLen);
}

//*****************************************************************************
//
// Transmit a packet from the supplied buffer.
//
//*****************************************************************************
static int32_t
PacketTransmit(uint32_t ui32Base, uint8_t *pui8Buf, int32_t i32BufLen)
{
    int_fast32_t i32Loop;

    //
    // Indicate that a packet is being sent.
    //
    HWREGBITW(&g_ui32Flags, FLAG_TXPKT) = 1;

    //
    // Wait for the previous packet to be transmitted.
    //
    while(g_psTxDescriptor[g_ui32TxDescIndex].ui32CtrlStatus &
          DES0_TX_CTRL_OWN)
    {
        //
        // Spin and waste time.
        //
    }

    //
    // Check that we're not going to overflow the transmit buffer.  This
    // shouldn't be necessary since the uIP buffer is smaller than our DMA
    // transmit buffer but, just in case...
    //
    if(i32BufLen > TX_BUFFER_SIZE)
    {
        i32BufLen = TX_BUFFER_SIZE;
    }

    //
    // Copy the packet data into the transmit buffer.
    //
    for(i32Loop = 0; i32Loop < i32BufLen; i32Loop++)
    {
        g_pui8TxBuffer[i32Loop] = pui8Buf[i32Loop];
    }

    //
    // Move to the next descriptor.
    //
    g_ui32TxDescIndex++;
    if(g_ui32TxDescIndex == NUM_TX_DESCRIPTORS)
    {
        g_ui32TxDescIndex = 0;
    }

    //
    // Fill in the packet size and tell the transmitter to start work.
    //
    g_psTxDescriptor[g_ui32TxDescIndex].ui32Count = (uint32_t)i32BufLen;
    g_psTxDescriptor[g_ui32TxDescIndex].ui32CtrlStatus =
        (DES0_TX_CTRL_LAST_SEG | DES0_TX_CTRL_FIRST_SEG |
         DES0_TX_CTRL_INTERRUPT | DES0_TX_CTRL_IP_ALL_CKHSUMS |
         DES0_TX_CTRL_CHAINED | DES0_TX_CTRL_OWN);

    //
    // Tell the DMA to reacquire the descriptor now that we've filled it in.
    //
    ROM_EMACTxDMAPollDemand(EMAC0_BASE);

    //
    // Return the number of bytes sent.
    //
    return(i32BufLen);
}

//*****************************************************************************
//
// Initialize the transmit and receive DMA descriptors.  We apparently need
// a minimum of 3 descriptors in each chain.  This is overkill since uIP uses
// a single, common transmit and receive buffer so we tag each descriptor
// with the same buffer and will make sure we only hand the DMA one descriptor
// at a time.
//
//*****************************************************************************
void
InitDescriptors(uint32_t ui32Base)
{
    uint32_t ui32Loop;

    //
    // Initialize each of the transmit descriptors.  Note that we leave the OWN
    // bit clear here since we have not set up any transmissions yet.
    //
    for(ui32Loop = 0; ui32Loop < NUM_TX_DESCRIPTORS; ui32Loop++)
    {
        g_psTxDescriptor[ui32Loop].ui32Count =
            (DES1_TX_CTRL_SADDR_INSERT |
             (TX_BUFFER_SIZE << DES1_TX_CTRL_BUFF1_SIZE_S));
        g_psTxDescriptor[ui32Loop].pvBuffer1 = g_pui8TxBuffer;
        g_psTxDescriptor[ui32Loop].DES3.pLink =
            (ui32Loop == (NUM_TX_DESCRIPTORS - 1)) ?
            g_psTxDescriptor : &g_psTxDescriptor[ui32Loop + 1];
        g_psTxDescriptor[ui32Loop].ui32CtrlStatus =
            (DES0_TX_CTRL_LAST_SEG | DES0_TX_CTRL_FIRST_SEG |
             DES0_TX_CTRL_INTERRUPT | DES0_TX_CTRL_CHAINED |
             DES0_TX_CTRL_IP_ALL_CKHSUMS);
    }

    //
    // Initialize each of the receive descriptors.  We clear the OWN bit here
    // to make sure that the receiver doesn't start writing anything
    // immediately.
    //
    for(ui32Loop = 0; ui32Loop < NUM_RX_DESCRIPTORS; ui32Loop++)
    {
        g_psRxDescriptor[ui32Loop].ui32CtrlStatus = 0;
        g_psRxDescriptor[ui32Loop].ui32Count =
            (DES1_RX_CTRL_CHAINED |
             (RX_BUFFER_SIZE << DES1_RX_CTRL_BUFF1_SIZE_S));
        g_psRxDescriptor[ui32Loop].pvBuffer1 = g_pui8RxBuffer;
        g_psRxDescriptor[ui32Loop].DES3.pLink =
            (ui32Loop == (NUM_RX_DESCRIPTORS - 1)) ?
            g_psRxDescriptor : &g_psRxDescriptor[ui32Loop + 1];
    }

    //
    // Set the descriptor pointers in the hardware.
    //
    ROM_EMACRxDMADescriptorListSet(ui32Base, g_psRxDescriptor);
    ROM_EMACTxDMADescriptorListSet(ui32Base, g_psTxDescriptor);

    //
    // Start from the beginning of both descriptor chains.  We actually set
    // the transmit descriptor index to the last descriptor in the chain
    // since it will be incremented before use and this means the first
    // transmission we perform will use the correct descriptor.
    //
    g_ui32RxDescIndex = 0;
    g_ui32TxDescIndex = NUM_TX_DESCRIPTORS - 1;
}

//*****************************************************************************
//
// This example demonstrates the use of the Ethernet Controller with the uIP
// TCP/IP stack.
//
//*****************************************************************************
int
main(void)
{
    uip_ipaddr_t sIPAddr;
    static struct uip_eth_addr sTempAddr;
    int32_t i32PeriodicTimer, i32ARPTimer;
#ifdef EXTERNAL_RMII_PHY
    uint16_t ui16Val;
#endif
    uint32_t ui32User0, ui32User1;
    uint32_t ui32Temp, ui32PHYConfig, ui32SysClock;
    uint8_t ui8PHYAddr;

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
    FrameDraw(&g_sContext, "enet-uip");

    //
    // Initialize the UART.
    //
    UARTStdioConfig(0, 115200, ui32SysClock);
    UARTprintf("Ethernet with uIP\n-----------------\n\n");

#ifdef EXTERNAL_RMII_PHY
    UpdateStatus("Using RMII PHY.");
    ui32PHYConfig = EMAC_PHY_TYPE_EXTERNAL_RMII;
    ui8PHYAddr = 0;
#elif defined EXTERNAL_MII_PHY
    UpdateStatus("Using MII PHY.");
    ui32PHYConfig = EMAC_PHY_TYPE_EXTERNAL_MII;
    ui8PHYAddr = 0;
#else
    UpdateStatus("Using Internal PHY.");
    ui32PHYConfig = (EMAC_PHY_TYPE_INTERNAL | EMAC_PHY_INT_MDIX_EN |
                     EMAC_PHY_AN_100B_T_FULL_DUPLEX);
    ui8PHYAddr = 0;
#endif

    //
    // Read the MAC address from the user registers.
    //
    ROM_FlashUserGet(&ui32User0, &ui32User1);
    if((ui32User0 == 0xffffffff) || (ui32User1 == 0xffffffff))
    {
        //
        // We should never get here.  This is an error if the MAC address has
        // not been programmed into the device.  Exit the program.
        //
        UpdateStatus("MAC Address Not Programmed!");
        while(1)
        {
        }
    }

    //
    // Convert the 24/24 split MAC address from NV ram into a 32/16 split MAC
    // address needed to program the hardware registers, then program the MAC
    // address into the Ethernet Controller registers.
    //
    sTempAddr.addr[0] = ((ui32User0 >> 0) & 0xff);
    sTempAddr.addr[1] = ((ui32User0 >> 8) & 0xff);
    sTempAddr.addr[2] = ((ui32User0 >> 16) & 0xff);
    sTempAddr.addr[3] = ((ui32User1 >> 0) & 0xff);
    sTempAddr.addr[4] = ((ui32User1 >> 8) & 0xff);
    sTempAddr.addr[5] = ((ui32User1 >> 16) & 0xff);

    //
    // Configure SysTick for a periodic interrupt.
    //
    ROM_SysTickPeriodSet(ui32SysClock / SYSTICKHZ);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable and reset the Ethernet modules.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_EMAC0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_EPHY0);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_EMAC0);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_EPHY0);

    //
    // Wait for the MAC to be ready.
    //
    UpdateStatus("Waiting for MAC to be ready...");
    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_EMAC0))
    {
    }

    //
    // Configure for use with the internal PHY.
    //
    MAP_EMACPHYConfigSet(EMAC0_BASE, ui32PHYConfig);
    UpdateStatus("MAC ready.");

    //
    // Reset the MAC.
    //
    ROM_EMACReset(EMAC0_BASE);

    //
    // Initialize the MAC and set the DMA mode.
    //
    ROM_EMACInit(EMAC0_BASE, ui32SysClock,
                 EMAC_BCONFIG_MIXED_BURST | EMAC_BCONFIG_PRIORITY_FIXED, 4, 4,
                 0);

#ifdef EXTERNAL_RMII_PHY
    //
    // When using an external RMII PHY, we need to acquire link before we can
    // determine how to configure the MAC.  Unfortunately, the bit signalling
    // on RMII changes depending upon whether the PHY is configured to use
    // 10Mbps or 100Mbps and we need to tell the MAC which signalling method
    // to use.
    //
    // With MII, the signalling is the same for both speeds so we don't need to
    // include code to wait for the link to be acquired.
    //

    //
    // Write the PHY register that tells it to start auto-negotiation.
    //
    UpdateStatus("Starting autonegotiation...");
    ROM_EMACPHYWrite(EMAC0_BASE, ui8PHYAddr, EPHY_BMCR,
                     EPHY_BMCR_ANEN | EPHY_BMCR_RESTARTAN);

    //
    // Poll, waiting for auto-negotiation to complete.
    //
    UpdateStatus("Waiting for autonegotiation to complete...");
    do
    {
        ui16Val = ROM_EMACPHYRead(EMAC0_BASE, ui8PHYAddr, EPHY_BMSR);
        ROM_SysCtlDelay(ui32SysClock / (10 * 3));
    }
    while(!(ui16Val & EPHY_BMSR_ANC));

    //
    // Wait for link to be acquired.
    //
    UpdateStatus("Waiting for link...");
    while(!(ui16Val & EPHY_BMSR_LINKSTAT))
    {
        ui16Val = ROM_EMACPHYRead(EMAC0_BASE, ui8PHYAddr, EPHY_BMSR);
    }

    UpdateStatus("Link acquired.");

    //
    // What's the operating speed?
    //
    ui16Val = ROM_EMACPHYRead(EMAC0_BASE, ui8PHYAddr, EPHY_STS);

    //
    // Set the MAC to PHY communication speed and duplex mode accordingly.
    //
    UARTprintf("Speed is 10%sMbps.\n", (ui16Val & EPHY_STS_SPEED) ? "" : "0");

    ROM_EMACConfigSet(EMAC0_BASE,
                      (((ui16Val & EPHY_STS_SPEED) ? 0 : EMAC_CONFIG_100MBPS) |
                       ((ui16Val & EPHY_STS_DUPLEX) ? EMAC_CONFIG_FULL_DUPLEX :
                        0) |
                       EMAC_CONFIG_FULL_DUPLEX |
                       EMAC_CONFIG_CHECKSUM_OFFLOAD |
                       EMAC_CONFIG_7BYTE_PREAMBLE |
                       EMAC_CONFIG_IF_GAP_96BITS | EMAC_CONFIG_USE_MACADDR0 |
                       EMAC_CONFIG_SA_FROM_DESCRIPTOR |
                       EMAC_CONFIG_BO_LIMIT_1024),
                      (EMAC_MODE_RX_STORE_FORWARD |
                       EMAC_MODE_TX_STORE_FORWARD |
                       EMAC_MODE_TX_THRESHOLD_64_BYTES |
                       EMAC_MODE_RX_THRESHOLD_64_BYTES), 0);
#else
    //
    // Set MAC configuration options.
    //
    ROM_EMACConfigSet(EMAC0_BASE,
                      (EMAC_CONFIG_FULL_DUPLEX | EMAC_CONFIG_CHECKSUM_OFFLOAD |
                       EMAC_CONFIG_7BYTE_PREAMBLE | EMAC_CONFIG_IF_GAP_96BITS |
                       EMAC_CONFIG_USE_MACADDR0 |
                       EMAC_CONFIG_SA_FROM_DESCRIPTOR |
                       EMAC_CONFIG_BO_LIMIT_1024),
                      (EMAC_MODE_RX_STORE_FORWARD |
                       EMAC_MODE_TX_STORE_FORWARD |
                       EMAC_MODE_TX_THRESHOLD_64_BYTES |
                       EMAC_MODE_RX_THRESHOLD_64_BYTES), 0);
#endif

    //
    // Initialize the Ethernet DMA descriptors.
    //
    InitDescriptors(EMAC0_BASE);

    //
    // Program the hardware with its MAC address (for filtering).
    //
    ROM_EMACAddrSet(EMAC0_BASE, 0, (uint8_t *)&sTempAddr);

#ifndef EXTERNAL_RMII_PHY
    //
    // Wait for the link to become active.  If we are using an external
    // RMII PHY, we already waited for link to be acquired prior to
    // configuring the MAC so don't wait again here.
    //
    UpdateStatus("Waiting for Link");
    while((ROM_EMACPHYRead(EMAC0_BASE, ui8PHYAddr, EPHY_BMSR) &
           EPHY_BMSR_LINKSTAT) == 0)
    {
    }
    UpdateStatus("Link Established");
#endif

    //
    // Set MAC filtering options.  We receive all broadcast and multicast
    // packets along with those addressed specifically for us.
    //
    ROM_EMACFrameFilterSet(EMAC0_BASE, (EMAC_FRMFILTER_SADDR |
                                        EMAC_FRMFILTER_PASS_MULTICAST |
                                        EMAC_FRMFILTER_PASS_NO_CTRL));

    //
    // Clear any pending interrupts.
    //
    ROM_EMACIntClear(EMAC0_BASE, EMACIntStatus(EMAC0_BASE, false));

    //
    // Initialize the uIP TCP/IP stack.
    //
    uip_init();

    //
    // Set the local MAC address (for uIP).
    //
    uip_setethaddr(sTempAddr);
#ifdef USE_STATIC_IP
    uip_ipaddr(sIPAddr, DEFAULT_IPADDR0, DEFAULT_IPADDR1, DEFAULT_IPADDR2,
               DEFAULT_IPADDR3);
    uip_sethostaddr(sIPAddr);
    ShowIPAddress(sIPAddr);
    uip_ipaddr(sIPAddr, DEFAULT_NETMASK0, DEFAULT_NETMASK1, DEFAULT_NETMASK2,
               DEFAULT_NETMASK3);
    uip_setnetmask(sIPAddr);
#else
    uip_ipaddr(sIPAddr, 0, 0, 0, 0);
    uip_sethostaddr(sIPAddr);
    UpdateStatus("Waiting for IP address...");
    uip_ipaddr(sIPAddr, 0, 0, 0, 0);
    uip_setnetmask(sIPAddr);
#endif

    //
    // Enable the Ethernet MAC transmitter and receiver.
    //
    ROM_EMACTxEnable(EMAC0_BASE);
    ROM_EMACRxEnable(EMAC0_BASE);

    //
    // Enable the Ethernet interrupt.
    //
    ROM_IntEnable(INT_EMAC0);

    //
    // Enable the Ethernet RX Packet interrupt source.
    //
    ROM_EMACIntEnable(EMAC0_BASE, EMAC_INT_RECEIVE);

    //
    // Mark the first receive descriptor as available to the DMA to start
    // the receive processing.
    //
    g_psRxDescriptor[g_ui32RxDescIndex].ui32CtrlStatus |= DES0_RX_CTRL_OWN;

    //
    // Initialize the TCP/IP Application (e.g. web server).
    //
    httpd_init();

#ifndef USE_STATIC_IP
    //
    // Initialize the DHCP Client Application.
    //
    dhcpc_init(&sTempAddr.addr[0], 6);
    dhcpc_request();
#endif

    //
    // Main Application Loop.
    //
    i32PeriodicTimer = 0;
    i32ARPTimer = 0;
    while(true)
    {
        //
        // Wait for an event to occur.  This can be either a System Tick event,
        // or an RX Packet event.
        //
        while(!g_ui32Flags)
        {
        }

        //
        // If SysTick, Clear the SysTick interrupt flag and increment the
        // timers.
        //
        if(HWREGBITW(&g_ui32Flags, FLAG_SYSTICK) == 1)
        {
            HWREGBITW(&g_ui32Flags, FLAG_SYSTICK) = 0;
            i32PeriodicTimer += SYSTICKMS;
            i32ARPTimer += SYSTICKMS;
        }

        //
        // Check for an RX Packet and read it.
        //
        if(HWREGBITW(&g_ui32Flags, FLAG_RXPKT))
        {
            //
            // Clear the RX Packet event flag.
            //
            HWREGBITW(&g_ui32Flags, FLAG_RXPKT) = 0;

            // Get the packet and set uip_len for uIP stack usage.
            //
            uip_len = (unsigned short)PacketReceive(EMAC0_BASE, uip_buf,
                                                    sizeof(g_pui8UIPBuffer));

            //
            // Process incoming IP packets here.
            //
            if(BUF->type == htons(UIP_ETHTYPE_IP))
            {
                uip_arp_ipin();
                uip_input();

                //
                // If the above function invocation resulted in data that
                // should be sent out on the network, the global variable
                // uip_len is set to a value > 0.
                //
                if(uip_len > 0)
                {
                    uip_arp_out();
                    PacketTransmit(EMAC0_BASE, uip_buf, uip_len);
                    uip_len = 0;
                }
            }

            //
            // Process incoming ARP packets here.
            //
            else if(BUF->type == htons(UIP_ETHTYPE_ARP))
            {
                uip_arp_arpin();

                //
                // If the above function invocation resulted in data that
                // should be sent out on the network, the global variable
                // uip_len is set to a value > 0.
                //
                if(uip_len > 0)
                {
                    PacketTransmit(EMAC0_BASE, uip_buf, uip_len);
                    uip_len = 0;
                }
            }
        }

        //
        // Process TCP/IP Periodic Timer here.
        //
        if(i32PeriodicTimer > UIP_PERIODIC_TIMER_MS)
        {
            i32PeriodicTimer = 0;
            for(ui32Temp = 0; ui32Temp < UIP_CONNS; ui32Temp++)
            {
                uip_periodic(ui32Temp);

                //
                // If the above function invocation resulted in data that
                // should be sent out on the network, the global variable
                // uip_len is set to a value > 0.
                //
                if(uip_len > 0)
                {
                    uip_arp_out();
                    PacketTransmit(EMAC0_BASE, uip_buf, uip_len);
                    uip_len = 0;
                }
            }

#if UIP_UDP
            for(ui32Temp = 0; ui32Temp < UIP_UDP_CONNS; ui32Temp++)
            {
                uip_udp_periodic(ui32Temp);

                //
                // If the above function invocation resulted in data that
                // should be sent out on the network, the global variable
                // uip_len is set to a value > 0.
                //
                if(uip_len > 0)
                {
                    uip_arp_out();
                    PacketTransmit(EMAC0_BASE, uip_buf, uip_len);
                    uip_len = 0;
                }
            }
#endif
        }

        //
        // Process ARP Timer here.
        //
        if(i32ARPTimer > UIP_ARP_TIMER_MS)
        {
            i32ARPTimer = 0;
            uip_arp_timer();
        }
    }
}

#ifdef UIP_ARCH_IPCHKSUM
//
// Return the IP checksum for the packet in uip_buf.  This is a dummy since
// the hardware calculates this for us.
//
u16_t
uip_ipchksum(void)
{
    //
    // Dummy function - the hardware calculates and inserts all required
    // checksums for us.
    //
    return(0xffff);
}

u16_t
uip_chksum(u16_t *data, u16_t len)
{
    return(0xffff);
}

u16_t
uip_icmp6chksum(void)
{
    return(0xffff);
}

u16_t
uip_tcpchksum(void)
{
    return(0xffff);
}
#endif
