//*****************************************************************************
//
// softuart_echo.c - Example for reading data from and writing data to the
//                   SoftUART.
//
// Copyright (c) 2012-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "grlib/grlib.h"
#include "drivers/cfal96x64x16.h"
#include "utils/softuart.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>SoftUART Echo (softuart_echo)</h1>
//!
//! This example application utilizes the SoftUART to echo text.  The SoftUART
//! is configured to use the same pins as the first UART (connected to the FTDI
//! virtual serial port on the evaluation board), at 115,200 baud, 8-n-1 mode.
//! All characters received on the SoftUART are transmitted back to the
//! SoftUART.
//
//*****************************************************************************

//*****************************************************************************
//
// The instance data for the SoftUART module.
//
//*****************************************************************************
static tSoftUART g_sUART;

//*****************************************************************************
//
// The buffer to be used for the SoftUART transmit buffer.
//
//*****************************************************************************
static uint8_t g_pui8TxBuffer[64];

//*****************************************************************************
//
// The buffer to be used for the SoftUART receive buffer.
//
//*****************************************************************************
static uint16_t g_pui16RxBuffer[64];

//*****************************************************************************
//
// The number of processor clocks in the time period of a single bit on the
// SoftUART interface.
//
//*****************************************************************************
static uint32_t g_ui32BitTime;

//*****************************************************************************
//
// A flag that is set in the SoftUART "interrupt" handler when there are
// characters in the receive buffer that need to be read.
//
//*****************************************************************************
static volatile uint32_t g_ui32Flag = 0;

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
// The interrupt handler for the SoftUART transmit timer interrupt.
//
//*****************************************************************************
void
Timer0AIntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    //
    // Call the SoftUART transmit timer tick function.
    //
    SoftUARTTxTimerTick(&g_sUART);
}

//*****************************************************************************
//
// The interrupt handler for the SoftUART receive timer interrupt.
//
//*****************************************************************************
void
Timer0BIntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMB_TIMEOUT);

    //
    // Call the SoftUART receive timer tick function, and see if the timer
    // should be disabled.
    //
    if(SoftUARTRxTick(&g_sUART, false) == SOFTUART_RXTIMER_END)
    {
        //
        // Disable the timer interrupt since the SoftUART doesn't need it any
        // longer.
        //
        ROM_TimerDisable(TIMER0_BASE, TIMER_B);
    }
}

//*****************************************************************************
//
// The interrupt handler for the SoftUART GPIO edge interrupt.
//
//*****************************************************************************
void
GPIOAIntHandler(void)
{
    //
    // Configure the SoftUART receive timer so that it will sample at the
    // mid-bit time of this character.
    //
    ROM_TimerDisable(TIMER0_BASE, TIMER_B);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_B, g_ui32BitTime);
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
    ROM_TimerEnable(TIMER0_BASE, TIMER_B);

    //
    // Call the SoftUART receive timer tick function.
    //
    SoftUARTRxTick(&g_sUART, true);
}

//*****************************************************************************
//
// The SoftUART "interrupt" handler.
//
//*****************************************************************************
void
SoftUARTIntHandler(void)
{
    uint32_t ui32Status;

    //
    // Get the interrrupt status.
    //
    ui32Status = SoftUARTIntStatus(&g_sUART, true);

    //
    // Clear the asserted interrupts.
    //
    SoftUARTIntClear(&g_sUART, ui32Status);

    //
    // Set the flag indicating that there are characters to be read from the
    // receive buffer.  This is done instead of reading the characters here in
    // order to minimize the amount of time spent in the "interrupt" handler
    // (which is important at higher baud rates).
    //
    g_ui32Flag = 1;
}

//*****************************************************************************
//
// Send a string to the UART.
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
        SoftUARTCharPut(&g_sUART, *pui8Buffer++);
    }
}

//*****************************************************************************
//
// This example demonstrates how to send a string of data to the UART.
//
//*****************************************************************************
int
main(void)
{
    tRectangle sRect;
    tContext sContext;

    //
    // Set the clocking to run at 50 MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);
    
    //
    // Initialize the display driver.
    //
    CFAL96x64x16Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sCFAL96x64x16);

    //
    // Fill the top part of the screen with blue to create the banner.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.i16YMax = 9;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    //
    // Change foreground for white text.
    //
    GrContextForegroundSet(&sContext, ClrWhite);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&sContext, g_psFontFixed6x8);
    GrStringDrawCentered(&sContext, "softuart-echo", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 4, 0);
    
    //
    // Initialize the display and write some instructions.
    //
    GrStringDrawCentered(&sContext, "Connect a", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 20, false);
    GrStringDrawCentered(&sContext, "terminal", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 30, false);
    GrStringDrawCentered(&sContext, "to UART0.", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 40, false);
    GrStringDrawCentered(&sContext, "115000,N,8,1", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 50, false);

    //
    // Enable the peripherals used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    //
    // Compute the bit time for 115,200 baud.
    //
    g_ui32BitTime = (ROM_SysCtlClockGet() / 115200) - 1;

    //
    // Configure the SoftUART for 8-N-1 operation.
    //
    SoftUARTInit(&g_sUART);
    SoftUARTRxGPIOSet(&g_sUART, GPIO_PORTA_BASE, GPIO_PIN_0);
    SoftUARTTxGPIOSet(&g_sUART, GPIO_PORTA_BASE, GPIO_PIN_1);
    SoftUARTRxBufferSet(&g_sUART, g_pui16RxBuffer,
                        sizeof(g_pui16RxBuffer) / sizeof(g_pui16RxBuffer[0]));
    SoftUARTTxBufferSet(&g_sUART, g_pui8TxBuffer, sizeof(g_pui8TxBuffer));
    SoftUARTCallbackSet(&g_sUART, SoftUARTIntHandler);
    SoftUARTConfigSet(&g_sUART,
                      (SOFTUART_CONFIG_WLEN_8 | SOFTUART_CONFIG_STOP_ONE |
                       SOFTUART_CONFIG_PAR_NONE));

    //
    // Configure the timer for the SoftUART transmitter.
    //
    ROM_TimerConfigure(TIMER0_BASE,
                       (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC |
                        TIMER_CFG_B_PERIODIC));
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32BitTime);
    ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT | TIMER_TIMB_TIMEOUT);
    ROM_TimerEnable(TIMER0_BASE, TIMER_A);

    //
    // Set the priorities of the interrupts associated with the SoftUART.  The
    // receiver is higher priority than the transmitter, and the receiver edge
    // interrupt is higher priority than the receiver timer interrupt.
    //
    ROM_IntPrioritySet(INT_GPIOA, 0x00);
    ROM_IntPrioritySet(INT_TIMER0B, 0x40);
    ROM_IntPrioritySet(INT_TIMER0A, 0x80);

    //
    // Enable the interrupts associated with the SoftUART.
    //
    ROM_IntEnable(INT_TIMER0A);
    ROM_IntEnable(INT_TIMER0B);
    ROM_IntEnable(INT_GPIOA);

    //
    // Prompt for text to be entered.
    //
    UARTSend((uint8_t *)"\033[2JEnter text: ", 16);

    //
    // Enable the SoftUART interrupt.
    //
    SoftUARTIntEnable(&g_sUART, SOFTUART_INT_RX | SOFTUART_INT_RT);

    //
    // Loop forever echoing data through the UART.
    //
    while(1)
    {
        //
        // Wait until there are characters available in the receive buffer.
        //
        while(g_ui32Flag == 0)
        {
        }
        g_ui32Flag = 0;

        //
        // Loop while there are characters in the receive buffer.
        //
        while(SoftUARTCharsAvail(&g_sUART))
        {
            //
            // Read the next character from the UART and write it back to the
            // UART.
            //
            SoftUARTCharPutNonBlocking(&g_sUART,
                                       SoftUARTCharGetNonBlocking(&g_sUART));
        }
    }
}
