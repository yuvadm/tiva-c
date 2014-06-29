//*****************************************************************************
//
// can.c - Simple CAN example.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C123G Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/fpu.h"
#include "driverlib/can.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "grlib/grlib.h"
#include "drivers/cfal96x64x16.h"
#include "utils/uartstdio.h"
#include "driverlib/interrupt.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>CAN Example (can)</h1>
//!
//! This example application utilizes CAN to send characters back and forth
//! between two boards. It uses the UART to read / write the characters to
//! the UART terminal. It also uses the graphical display on the board to show
//! the last character transmited / received. Error handling is also included.
//!
//! CAN HARDWARE SETUP:
//!
//! To use this example you will need to hook up two DK-TM4C123G boards
//! together in a CAN network. This involves hooking the CANH screw terminals
//! together and the CANL terminals together. In addition 120ohm termination
//! resistors will need to be added to the edges of the network between CANH
//! and CANL.  In the two board setup this means hooking a 120 ohm resistor
//! between CANH and CANL on both boards.
//!
//! See diagram below for visual. '---' represents wire.
//!
//! \verbatim
//!       CANH--+--------------------------+--CANH
//!             |                          |
//!            .-.                        .-.
//!            | |120ohm                  | |120ohm
//!            | |                        | |
//!            '-'                        '-'
//!             |                          |
//!       CANL--+--------------------------+--CANL
//! \endverbatim
//!
//! SOFTWARE SETUP:
//!
//! Once the hardware connections are setup connect both boards to the computer
//! via the In-Circuit Debug Interface USB port next to the graphical display.
//! Attach a UART terminal to each board configured 115,200 baud, 8-n-1 mode.
//!
//! Anything you type into one terminal will show up in the other terminal and
//! vice versa. The last character sent / received will also be displayed on
//! the graphical display on the board.
//
//*****************************************************************************

//*****************************************************************************
//
// A counter that keeps track of the number of times the TX & RX interrupt has
// occurred, which should match the number of messages that were transmitted /
// received.
//
//*****************************************************************************
volatile uint32_t g_ui32RXMsgCount = 0;
volatile uint32_t g_ui32TXMsgCount = 0;

//*****************************************************************************
//
// A flag for the interrupt handler to indicate that a message was received.
//
//*****************************************************************************
volatile bool g_bRXFlag = 0;

//*****************************************************************************
//
// A global to keep track of the error flags that have been thrown so they may
// be processed. This is necessary because reading the error register clears
// the flags, so it is necessary to save them somewhere for processing.
//
//*****************************************************************************
volatile uint32_t g_ui32ErrFlag = 0;

//*****************************************************************************
//
// CAN message Objects for data being sent / received
//
//*****************************************************************************
tCANMsgObject g_sCAN0RxMessage;
tCANMsgObject g_sCAN0TxMessage;

//*****************************************************************************
//
// Message Identifiers and Objects
// RXID is set to 0 so all messages are received
//
//*****************************************************************************
#define CAN0RXID                0
#define RXOBJECT                1
#define CAN0TXID                2
#define TXOBJECT                2

//*****************************************************************************
//
// Variables to hold character being sent / reveived
//
//*****************************************************************************
uint8_t g_ui8TXMsgData;
uint8_t g_ui8RXMsgData;

//*****************************************************************************
//
// Global context for text printed on graphics display
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// Screen line deffinitions, offsets from top in pixels
//
//*****************************************************************************
#define SCREENLINE1             10
#define SCREENLINE2             20
#define SCREENLINE3             30
#define SCREENLINE4             40
#define SCREENLINE5             50

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
// CAN 0 Interrupt Handler. It checks for the cause of the interrupt, and
// maintains a count of all messages that have been transmitted / received
//
//*****************************************************************************
void
CAN0IntHandler(void)
{
    uint32_t ui32Status;

    //
    // Read the CAN interrupt status to find the cause of the interrupt
    //
    // CAN_INT_STS_CAUSE register values
    // 0x0000        = No Interrupt Pending
    // 0x0001-0x0020 = Number of message object that caused the interrupt
    // 0x8000        = Status interrupt
    // all other numbers are reserved and have no meaning in this system
    //
    ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    //
    // If this was a status interrupt acknowledge it by reading the CAN
    // controller status register.
    //
    if(ui32Status == CAN_INT_INTID_STATUS)
    {
        //
        // Read the controller status.  This will return a field of status
        // error bits that can indicate various errors. Refer to the
        // API documentation for details about the error status bits.
        // The act of reading this status will clear the interrupt.
        //
        ui32Status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);

        //
        // Add ERROR flags to list of current errors. To be handled
        // later, because it would take too much time here in the
        // interrupt.
        //
        g_ui32ErrFlag |= ui32Status;
    }

    //
    // Check if the cause is message object RXOBJECT, which we are using
    // for receiving messages.
    //
    else if(ui32Status == RXOBJECT)
    {
        //
        // Getting to this point means that the RX interrupt occurred on
        // message object RXOBJECT, and the message reception is complete.
        // Clear the message object interrupt.
        //
        CANIntClear(CAN0_BASE, RXOBJECT);

        //
        // Increment a counter to keep track of how many messages have been
        // received.  In a real application this could be used to set flags to
        // indicate when a message is received.
        //
        g_ui32RXMsgCount++;

        //
        // Set flag to indicate received message is pending.
        //
        g_bRXFlag = true;

        //
        // Since a message was received, clear any error flags.
        // This is done because before the message is received it triggers
        // a Status Interrupt for RX complete. by clearing the flag here we
        // prevent unnecessary error handling from happeneing
        //
        g_ui32ErrFlag = 0;
    }

    //
    // Check if the cause is message object TXOBJECT, which we are using
    // for transmitting messages.
    //
    else if(ui32Status == TXOBJECT)
    {
        //
        // Getting to this point means that the TX interrupt occurred on
        // message object TXOBJECT, and the message reception is complete.
        // Clear the message object interrupt.
        //
        CANIntClear(CAN0_BASE, TXOBJECT);

        //
        // Increment a counter to keep track of how many messages have been
        // transmitted. In a real application this could be used to set
        // flags to indicate when a message is transmitted.
        //
        g_ui32TXMsgCount++;

        //
        // Since a message was transmitted, clear any error flags.
        // This is done because before the message is transmitted it triggers
        // a Status Interrupt for TX complete. by clearing the flag here we
        // prevent unnecessary error handling from happeneing
        //
        g_ui32ErrFlag = 0;
    }

    //
    // Otherwise, something unexpected caused the interrupt.  This should
    // never happen.
    //
    else
    {
        //
        // Spurious interrupt handling can go here.
        //
    }
}

//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************
void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, 16000000);
}

//*****************************************************************************
//
// Setup CAN0 to both send and receive at 500KHz.
// Interrupts on
// Use PE4 / PE5
//
//*****************************************************************************
void
InitCAN0(void)
{
    //
    // For this example CAN0 is used with RX and TX pins on port E4 and E5.
    // GPIO port E needs to be enabled so these pins can be used.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    //
    // Configure the GPIO pin muxing to select CAN0 functions for these pins.
    // This step selects which alternate function is available for these pins.
    //
    GPIOPinConfigure(GPIO_PE4_CAN0RX);
    GPIOPinConfigure(GPIO_PE5_CAN0TX);

    //
    // Enable the alternate function on the GPIO pins.  The above step selects
    // which alternate function is available.  This step actually enables the
    // alternate function instead of GPIO for these pins.
    //
    GPIOPinTypeCAN(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    //
    // The GPIO port and pins have been set up for CAN.  The CAN peripheral
    // must be enabled.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);

    //
    // Initialize the CAN controller
    //
    CANInit(CAN0_BASE);

    //
    // Set up the bit rate for the CAN bus.  This function sets up the CAN
    // bus timing for a nominal configuration.  You can achieve more control
    // over the CAN bus timing by using the function CANBitTimingSet() instead
    // of this one, if needed.
    // In this example, the CAN bus is set to 500 kHz.
    //
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);

    //
    // Enable interrupts on the CAN peripheral.  This example uses static
    // allocation of interrupt handlers which means the name of the handler
    // is in the vector table of startup code.
    //
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);

    //
    // Enable the CAN interrupt on the processor (NVIC).
    //
    IntEnable(INT_CAN0);

    //
    // Enable the CAN for operation.
    //
    CANEnable(CAN0_BASE);

    //
    // Initialize a message object to be used for receiving CAN messages with
    // any CAN ID.  In order to receive any CAN ID, the ID and mask must both
    // be set to 0, and the ID filter enabled.
    //
    g_sCAN0RxMessage.ui32MsgID = CAN0RXID;
    g_sCAN0RxMessage.ui32MsgIDMask = 0;
    g_sCAN0RxMessage.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    g_sCAN0RxMessage.ui32MsgLen = sizeof(g_ui8RXMsgData);

    //
    // Now load the message object into the CAN peripheral.  Once loaded the
    // CAN will receive any message on the bus, and an interrupt will occur.
    // Use message object RXOBJECT for receiving messages (this is not the
    //same as the CAN ID which can be any value in this example).
    //
    CANMessageSet(CAN0_BASE, RXOBJECT, &g_sCAN0RxMessage, MSG_OBJ_TYPE_RX);

    //
    // Initialize the message object that will be used for sending CAN
    // messages.  The message will be 1 bytes that will contain the character
    // received from the other controller. Initially it will be set to 0.
    //
    g_ui8TXMsgData = 0;
    g_sCAN0TxMessage.ui32MsgID = CAN0TXID;
    g_sCAN0TxMessage.ui32MsgIDMask = 0;
    g_sCAN0TxMessage.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    g_sCAN0TxMessage.ui32MsgLen = sizeof(g_ui8TXMsgData);
    g_sCAN0TxMessage.pui8MsgData = (uint8_t *)&g_ui8TXMsgData;
}

//*****************************************************************************
//
// Set up the OLED Graphical Display
//
//*****************************************************************************
void
InitGraphics(void)
{
    tRectangle sRect;

    //
    // Initialize the display driver.
    //
    CFAL96x64x16Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sCFAL96x64x16);

    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMax = 9;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_psFontFixed6x8);
    GrStringDrawCentered(&g_sContext, "CAN Example", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 4, 0);

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&g_sContext);
}

//*****************************************************************************
//
// Can ERROR handling. When a message is received if there is an erro it is
// saved to g_ui32ErrFlag, the Error Flag Set. Below the flags are checked
// and cleared. It is left up to the user to add handling fuctionality if so
// desiered.
//
// For more information on the error flags please see the CAN section of the
// microcontroller datasheet.
//
// NOTE: you may experience errors during setup when only one board is powered
// on. This is caused by one board sending signals and there not being another
// board there to acknoledge it. Dont worry about these errors, they can be
// disregarded.
//
//*****************************************************************************
void
CANErrorHandler(void)
{
    //
    // CAN controller has entered a Bus Off state.
    //
    if(g_ui32ErrFlag & CAN_STATUS_BUS_OFF)
    {
        //
        // Handle Error Condition here
        //
        UARTprintf("    ERROR: CAN_STATUS_BUS_OFF \n");

        //
        // Clear CAN_STATUS_BUS_OFF Flag
        //
        g_ui32ErrFlag &= ~(CAN_STATUS_BUS_OFF);

    }

    //
    // CAN controller error level has reached warning level.
    //
    if(g_ui32ErrFlag & CAN_STATUS_EWARN)
    {
        //
        // Handle Error Condition here
        //
        //UARTprintf("    ERROR: CAN_STATUS_EWARN \n");

        //
        // Clear CAN_STATUS_EWARN Flag
        //
        g_ui32ErrFlag &= ~(CAN_STATUS_EWARN);
    }

    //
    // CAN controller error level has reached error passive level.
    //
    if(g_ui32ErrFlag & CAN_STATUS_EPASS)
    {
        //
        // Handle Error Condition here
        //

        //
        // Clear CAN_STATUS_EPASS Flag
        //
        g_ui32ErrFlag &= ~(CAN_STATUS_EPASS);
    }

    //
    // A message was received successfully since the last read of this status.
    //
    if(g_ui32ErrFlag & CAN_STATUS_RXOK)
    {
        //
        // Handle Error Condition here
        //

        //
        // Clear CAN_STATUS_RXOK Flag
        //
        g_ui32ErrFlag &= ~(CAN_STATUS_RXOK);
    }

    //
    // A message was transmitted successfully since the last read of this
    // status.
    //
    if(g_ui32ErrFlag & CAN_STATUS_TXOK)
    {
        //
        // Handle Error Condition here
        //

        //
        // Clear CAN_STATUS_TXOK Flag
        //
        g_ui32ErrFlag &= ~(CAN_STATUS_TXOK);
    }

    //
    // This is the mask for the last error code field.
    //
    if(g_ui32ErrFlag & CAN_STATUS_LEC_MSK)
    {
        //
        // Handle Error Condition here
        //

        //
        // Clear CAN_STATUS_LEC_MSK Flag
        //
        g_ui32ErrFlag &= ~(CAN_STATUS_LEC_MSK);
    }

    //
    // A bit stuffing error has occurred.
    //
    if(g_ui32ErrFlag & CAN_STATUS_LEC_STUFF)
    {
        //
        // Handle Error Condition here
        //

        //
        // Clear CAN_STATUS_LEC_STUFF Flag
        //
        g_ui32ErrFlag &= ~(CAN_STATUS_LEC_STUFF);
    }

    //
    // A formatting error has occurred.
    //
    if(g_ui32ErrFlag & CAN_STATUS_LEC_FORM)
    {
        //
        // Handle Error Condition here
        //

        //
        // Clear CAN_STATUS_LEC_FORM Flag
        //
        g_ui32ErrFlag &= ~(CAN_STATUS_LEC_FORM);
    }

    //
    // An acknowledge error has occurred.
    //
    if(g_ui32ErrFlag & CAN_STATUS_LEC_ACK)
    {
        //
        // Handle Error Condition here
        //

        //
        // Clear CAN_STATUS_LEC_ACK Flag
        //
        g_ui32ErrFlag &= ~(CAN_STATUS_LEC_ACK);
    }

    //
    // The bus remained a bit level of 1 for longer than is allowed.
    //
    if(g_ui32ErrFlag & CAN_STATUS_LEC_BIT1)
    {
        //
        // Handle Error Condition here
        //

        //
        // Clear CAN_STATUS_LEC_BIT1 Flag
        //
        g_ui32ErrFlag &= ~(CAN_STATUS_LEC_BIT1);
    }

    //
    // The bus remained a bit level of 0 for longer than is allowed.
    //
    if(g_ui32ErrFlag & CAN_STATUS_LEC_BIT0)
    {
        //
        // Handle Error Condition here
        //

        //
        // Clear CAN_STATUS_LEC_BIT0 Flag
        //
        g_ui32ErrFlag &= ~(CAN_STATUS_LEC_BIT0);
    }

    //
    // A CRC error has occurred.
    //
    if(g_ui32ErrFlag & CAN_STATUS_LEC_CRC)
    {
        //
        // Handle Error Condition here
        //

        //
        // Clear CAN_STATUS_LEC_CRC Flag
        //
        g_ui32ErrFlag &= ~(CAN_STATUS_LEC_CRC);
    }

    //
    // This is the mask for the CAN Last Error Code (LEC).
    //
    if(g_ui32ErrFlag & CAN_STATUS_LEC_MASK)
    {
        //
        // Handle Error Condition here
        //

        //
        // Clear CAN_STATUS_LEC_MASK Flag
        //
        g_ui32ErrFlag &= ~(CAN_STATUS_LEC_MASK);
    }

    //
    // If there are any bits still set in g_ui32ErrFlag then something unhandled
    // has happened. Print the value of g_ui32ErrFlag.
    //
    if(g_ui32ErrFlag !=0)
    {
        UARTprintf("    Unhandled ERROR: %x \n",g_ui32ErrFlag);
    }
}

//*****************************************************************************
//
// Set up the system, initialize the UART, Graphics, and CAN. Then poll the
// UART for data. If there is any data send it, if there is any thing received
// print it out to the UART. If there are errors call the error handling
// function.
//
//*****************************************************************************
int
main(void)
{
    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    //
    // Initialize the UART
    //
    ConfigureUART();

    //
    // Initialize the graphical display
    //
    InitGraphics();

    //
    // Initialize CAN0
    //
    InitCAN0();

    //
    // Print welcome message
    //
    UARTprintf("\nCAN Example App\n");
    UARTprintf("Type something to see it show up on the other terminal: \n\n");

    //
    // Poll UART for data, transmit across CAN when something is entered
    //
    while(1)
    {
        //
        // If the flag is set, that means that the RX interrupt occurred and
        // there is a message ready to be read from the CAN
        //
        if(g_bRXFlag)
        {
            //
            // Reuse the same message object that was used earlier to configure
            // the CAN for receiving messages.  A buffer for storing the
            // received data must also be provided, so set the buffer pointer
            // within the message object.
            //
            g_sCAN0RxMessage.pui8MsgData = (uint8_t *) &g_ui8RXMsgData;

            //
            // Read the message from the CAN.  Message object RXOBJECT is used
            // (which is not the same thing as CAN ID).  The interrupt clearing
            // flag is not set because this interrupt was already cleared in
            // the interrupt handler.
            //
            CANMessageGet(CAN0_BASE, RXOBJECT, &g_sCAN0RxMessage, 0);

            //
            // Clear the pending message flag so that the interrupt handler can
            // set it again when the next message arrives.
            //
            g_bRXFlag = 0;

            //
            // Check to see if there is an indication that some messages were
            // lost.
            //
            if(g_sCAN0RxMessage.ui32Flags & MSG_OBJ_DATA_LOST)
            {
                UARTprintf("\nCAN message loss detected\n");
            }

            //
            // Print the received character to the UART terminal
            //
            UARTprintf("%c", g_ui8RXMsgData);

            //
            // Print the received character to the display, 
            // clear line with spaces
            //
            GrStringDrawCentered(&g_sContext, "RX Data", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2,
                                 SCREENLINE2, 0);
            GrStringDrawCentered(&g_sContext, (const char *) &g_ui8RXMsgData,
                                 1, GrContextDpyWidthGet(&g_sContext) / 2,
                                 SCREENLINE3, true);
            GrFlush(&g_sContext);
        }
        else
        {
            //
            // Error Handling
            //
            if(g_ui32ErrFlag != 0)
            {
                CANErrorHandler();
            }

            //
            // See if there is something new to transmit
            //
            while(ROM_UARTCharsAvail(UART0_BASE))
            {
                //
                // Read the next character from the UART terminal
                //
                g_ui8TXMsgData = ROM_UARTCharGetNonBlocking(UART0_BASE);

                //
                // Write the character to the display
                // clear line with spaces
                //
                GrStringDrawCentered(&g_sContext, "TX Data", -1,
                                     GrContextDpyWidthGet(&g_sContext) / 2,
                                     SCREENLINE4, true);
                GrStringDrawCentered(&g_sContext,
                                     (const char *)&g_ui8TXMsgData, 1,
                                     GrContextDpyWidthGet(&g_sContext) / 2,
                                     SCREENLINE5, true);
                GrFlush(&g_sContext);

                //
                // Send the CAN message using object number TXOBJECT (not the
                // same thing as CAN ID, which is also TXOBJECT in this
                // example).  This function will cause the message to be
                // transmitted right away.
                //
                CANMessageSet(CAN0_BASE, TXOBJECT, &g_sCAN0TxMessage,
                              MSG_OBJ_TYPE_TX);
            }
        }
    }
}
