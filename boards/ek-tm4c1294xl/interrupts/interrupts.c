//*****************************************************************************
//
// interrupts.c - Interrupt preemption and tail-chaining example.
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
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "drivers/pinout.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Interrupts (interrupts)</h1>
//!
//! This example application demonstrates the interrupt preemption and
//! tail-chaining capabilities of Cortex-M4 microprocessor and NVIC.  Nested
//! interrupts are synthesized when the interrupts have the same priority,
//! increasing priorities, and decreasing priorities.  With increasing
//! priorities, preemption will occur; in the other two cases tail-chaining
//! will occur.  The currently pending interrupts and the currently executing
//! interrupt will be displayed on the UART; GPIO pins B3, L1 and L0 (the
//! GPIO on jumper J27 on the left edge of the board) will be asserted upon
//! interrupt handler entry and de-asserted before interrupt handler exit so
//! that the off-to-on time can be observed with a scope or logic analyzer to
//! see the speed of tail-chaining (for the two cases where tail-chaining is
//! occurring).
//
//*****************************************************************************


//****************************************************************************
//
// Defines for Interrupt Priority.
//
//****************************************************************************
#define EQUAL_PRIORITY          0
#define DECREASING_PRIORITY     1
#define INCREASING_PRIORITY     2

//****************************************************************************
//
// System clock rate in Hz.
//
//****************************************************************************
uint32_t g_ui32SysClock;

//****************************************************************************
//
// Interrupt Mode.
//
//****************************************************************************
uint32_t g_ui32IntMode;

//*****************************************************************************
//
// The count of interrupts received.  This is incremented as each interrupt
// handler runs, and its value saved into interrupt handler specific values to
// determine the order in which the interrupt handlers were executed.
//
//*****************************************************************************
volatile uint32_t g_ui32Index;

//*****************************************************************************
//
// The value of g_ui32Index when the INT_GPIOA interrupt was processed.
//
//*****************************************************************************
volatile uint32_t g_ui32GPIOa;

//*****************************************************************************
//
// The value of g_ui32Index when the INT_GPIOB interrupt was processed.
//
//*****************************************************************************
volatile uint32_t g_ui32GPIOb;

//*****************************************************************************
//
// The value of g_ui32Index when the INT_GPIOC interrupt was processed.
//
//*****************************************************************************
volatile uint32_t g_ui32GPIOc;

//*****************************************************************************
//
// GPIOs that are used for this example.
//
//*****************************************************************************
#define GPIO_A_BASE     GPIO_PORTB_BASE
#define GPIO_A_PIN      GPIO_PIN_3
#define GPIO_B_BASE     GPIO_PORTL_BASE
#define GPIO_B_PIN      GPIO_PIN_1
#define GPIO_C_BASE     GPIO_PORTL_BASE
#define GPIO_C_PIN      GPIO_PIN_0

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
// Delay for the specified number of seconds.  Depending upon the current
// SysTick value, the delay will be between N-1 and N seconds (i.e. N-1 full
// seconds are guaranteed, along with the remainder of the current second).
//
//*****************************************************************************
void
Delay(uint32_t ui32Seconds)
{
    uint8_t ui8Loop;

    //
    // Loop while there are more seconds to wait.
    //
    while(ui32Seconds--)
    {
        for(ui8Loop = 0; ui8Loop < 100; ui8Loop++)
        {
            //
            // Wait until the SysTick value is less than 1000.
            //
            while(ROM_SysTickValueGet() > 1000)
            {
            }

            //
            // Wait until the SysTick value is greater than 1000.
            //
            while(ROM_SysTickValueGet() < 1000)
            {
            }
        }
    }
}

//*****************************************************************************
//
// Display the interrupt state on the UART.  The currently active and pending
// interrupts are displayed.
//
//*****************************************************************************
void
DisplayIntStatus(void)
{
    uint32_t ui32Temp;
    char pcBuffer[6];

    //
    // Put the status header text on the UART.
    //
    UARTprintf("\033[2J\033[H");
    UARTprintf("Interrupts example\n\n");
    switch (g_ui32IntMode)
    {
        case 0:
            UARTprintf("Equal Priority\n\n");
            break;

        case 1:
            UARTprintf("Decreasing Priority\n\n");
            break;

        case 2:
            UARTprintf("Increasing Priority\n\n");
            break;
        default:
            break;
    }

    UARTprintf("Active: ");

    //
    // Display the currently active interrupts.
    //
    ui32Temp = HWREG(NVIC_ACTIVE0);
    pcBuffer[0] = ' ';
    pcBuffer[1] = (ui32Temp & 1) ? '1' : ' ';
    pcBuffer[2] = (ui32Temp & 2) ? '2' : ' ';
    pcBuffer[3] = (ui32Temp & 4) ? '3' : ' ';
    pcBuffer[4] = ' ';
    pcBuffer[5] = '\0';
    UARTprintf(pcBuffer);

    //
    // Display the currently pending interrupts.
    //
    ui32Temp = HWREG(NVIC_PEND0);
    pcBuffer[1] = (ui32Temp & 1) ? '1' : ' ';
    pcBuffer[2] = (ui32Temp & 2) ? '2' : ' ';
    pcBuffer[3] = (ui32Temp & 4) ? '3' : ' ';
    UARTprintf("Pending: ");
    UARTprintf(pcBuffer);
}

//*****************************************************************************
//
// This is the handler for INT_GPIOA.  It simply saves the interrupt sequence
// number.
//
//*****************************************************************************
void
IntGPIOa(void)
{
    //
    // Set GPIO high to indicate entry to this interrupt handler.
    //
    ROM_GPIOPinWrite(GPIO_A_BASE, GPIO_A_PIN, GPIO_A_PIN);

    //
    // Put the current interrupt state on the UART.
    //
    DisplayIntStatus();

    //
    // Wait two seconds.
    //
    Delay(2);

    //
    // Save and increment the interrupt sequence number.
    //
    g_ui32GPIOa = g_ui32Index++;

    //
    // Set GPIO low to indicate exit from this interrupt handler.
    //
    ROM_GPIOPinWrite(GPIO_A_BASE, GPIO_A_PIN, 0);
}

//*****************************************************************************
//
// This is the handler for INT_GPIOB.  It triggers INT_GPIOA and saves the
// interrupt sequence number.
//
//*****************************************************************************
void
IntGPIOb(void)
{
    //
    // Set GPIO high to indicate entry to this interrupt handler.
    //
    ROM_GPIOPinWrite(GPIO_B_BASE, GPIO_B_PIN, GPIO_B_PIN);

    //
    // Put the current interrupt state on the UART.
    //
    DisplayIntStatus();

    //
    // Trigger the INT_GPIOA interrupt.
    //
    HWREG(NVIC_SW_TRIG) = INT_GPIOA - 16;

    //
    // Put the current interrupt state on the UART.
    //
    DisplayIntStatus();

    //
    // Wait two seconds.
    //
    Delay(2);

    //
    // Save and increment the interrupt sequence number.
    //
    g_ui32GPIOb = g_ui32Index++;

    //
    // Set GPIO low to indicate exit from this interrupt handler.
    //
    ROM_GPIOPinWrite(GPIO_B_BASE, GPIO_B_PIN, 0);
}

//*****************************************************************************
//
// This is the handler for INT_GPIOC.  It triggers INT_GPIOB and saves the
// interrupt sequence number.
//
//*****************************************************************************
void
IntGPIOc(void)
{
    //
    // Set GPIO high to indicate entry to this interrupt handler.
    //
    ROM_GPIOPinWrite(GPIO_C_BASE, GPIO_C_PIN, GPIO_C_PIN);

    //
    // Put the current interrupt state on the UART.
    //
    DisplayIntStatus();

    //
    // Trigger the INT_GPIOB interrupt.
    //
    HWREG(NVIC_SW_TRIG) = INT_GPIOB - 16;

    //
    // Put the current interrupt state on the UART.
    //
    DisplayIntStatus();

    //
    // Wait two seconds.
    //
    Delay(2);

    //
    // Save and increment the interrupt sequence number.
    //
    g_ui32GPIOc = g_ui32Index++;

    //
    // Set GPIO low to indicate exit from this interrupt handler.
    //
    ROM_GPIOPinWrite(GPIO_C_BASE, GPIO_C_PIN, 0);
}

//*****************************************************************************
//
// Configure the UART and its pins. This must be called before UARTprintf().
//
//*****************************************************************************
void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, g_ui32SysClock);
}

//*****************************************************************************
//
// This is the main example program.  It checks to see that the interrupts are
// processed in the correct order when they have identical priorities,
// increasing priorities, and decreasing priorities.  This exercises interrupt
// preemption and tail chaining.
//
//*****************************************************************************
int
main(void)
{
    uint_fast8_t ui8Error;

    //
    // Run from the PLL at 120 MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet(false, false);

    //
    // Configure the UART.
    //
    ConfigureUART();

    //
    // Configure the B3, L1 and L0 to be outputs to indicate entry/exit of one
    // of the interrupt handlers.
    //
    GPIOPinTypeGPIOOutput(GPIO_A_BASE, GPIO_A_PIN);
    GPIOPinTypeGPIOOutput(GPIO_B_BASE, GPIO_B_PIN);
    GPIOPinTypeGPIOOutput(GPIO_C_BASE, GPIO_C_PIN);
    GPIOPinWrite(GPIO_A_BASE, GPIO_A_PIN, 0);
    GPIOPinWrite(GPIO_B_BASE, GPIO_B_PIN, 0);
    GPIOPinWrite(GPIO_C_BASE, GPIO_C_PIN, 0);

    //
    // Set up and enable the SysTick timer.  It will be used as a reference
    // for delay loops in the interrupt handlers.  The SysTick timer period
    // will be set up for 100 times per second.
    //
    ROM_SysTickPeriodSet(g_ui32SysClock / 100);
    ROM_SysTickEnable();

    //
    // Reset the error indicator.
    //
    ui8Error = 0;

    //
    // Enable interrupts to the processor.
    //
    ROM_IntMasterEnable();

    //
    // Enable the interrupts.
    //
    ROM_IntEnable(INT_GPIOA);
    ROM_IntEnable(INT_GPIOB);
    ROM_IntEnable(INT_GPIOC);

    //
    // Indicate that the equal interrupt priority test is beginning.
    //
    g_ui32IntMode = EQUAL_PRIORITY;

    //
    // Set the interrupt priorities so they are all equal.
    //
    ROM_IntPrioritySet(INT_GPIOA, 0x00);
    ROM_IntPrioritySet(INT_GPIOB, 0x00);
    ROM_IntPrioritySet(INT_GPIOC, 0x00);

    //
    // Reset the interrupt flags.
    //
    g_ui32GPIOa = 0;
    g_ui32GPIOb = 0;
    g_ui32GPIOc = 0;
    g_ui32Index = 1;

    //
    // Trigger the interrupt for GPIO C.
    //
    HWREG(NVIC_SW_TRIG) = INT_GPIOC - 16;

    //
    // Put the current interrupt state on the LCD.
    //
    DisplayIntStatus();

    //
    // Verify that the interrupts were processed in the correct order.
    //
    if((g_ui32GPIOa != 3) || (g_ui32GPIOb != 2) || (g_ui32GPIOc != 1))
    {
        ui8Error |= 1;
    }

    //
    // Wait two seconds.
    //
    Delay(2);

    //
    // Indicate that the decreasing interrupt priority test is beginning.
    //
    g_ui32IntMode = DECREASING_PRIORITY;

    //
    // Set the interrupt priorities so that they are decreasing (i.e. C > B >
    // A).
    //
    ROM_IntPrioritySet(INT_GPIOA, 0x80);
    ROM_IntPrioritySet(INT_GPIOB, 0x40);
    ROM_IntPrioritySet(INT_GPIOC, 0x00);

    //
    // Reset the interrupt flags.
    //
    g_ui32GPIOa = 0;
    g_ui32GPIOb = 0;
    g_ui32GPIOc = 0;
    g_ui32Index = 1;

    //
    // Trigger the interrupt for GPIO C.
    //
    HWREG(NVIC_SW_TRIG) = INT_GPIOC - 16;

    //
    // Put the current interrupt state on the UART.
    //
    DisplayIntStatus();

    //
    // Verify that the interrupts were processed in the correct order.
    //
    if((g_ui32GPIOa != 3) || (g_ui32GPIOb != 2) || (g_ui32GPIOc != 1))
    {
        ui8Error |= 2;
    }

    //
    // Wait two seconds.
    //
    Delay(2);

    //
    // Indicate that the increasing interrupt priority test is beginning.
    //
    g_ui32IntMode = INCREASING_PRIORITY;

    //
    // Set the interrupt priorities so that they are increasing (i.e. C < B <
    // A).
    //
    ROM_IntPrioritySet(INT_GPIOA, 0x00);
    ROM_IntPrioritySet(INT_GPIOB, 0x40);
    ROM_IntPrioritySet(INT_GPIOC, 0x80);

    //
    // Reset the interrupt flags.
    //
    g_ui32GPIOa = 0;
    g_ui32GPIOb = 0;
    g_ui32GPIOc = 0;
    g_ui32Index = 1;

    //
    // Trigger the interrupt for GPIO C.
    //
    HWREG(NVIC_SW_TRIG) = INT_GPIOC - 16;

    //
    // Put the current interrupt state on the UART.
    //
    DisplayIntStatus();

    //
    // Verify that the interrupts were processed in the correct order.
    //
    if((g_ui32GPIOa != 1) || (g_ui32GPIOb != 2) || (g_ui32GPIOc != 3))
    {
        ui8Error |= 4;
    }

    //
    // Wait two seconds.
    //
    Delay(2);

    //
    // Disable the interrupts.
    //
    ROM_IntDisable(INT_GPIOA);
    ROM_IntDisable(INT_GPIOB);
    ROM_IntDisable(INT_GPIOC);

    //
    // Disable interrupts to the processor.
    //
    ROM_IntMasterDisable();

    //
    // Print out the test results.
    //
    UARTprintf("\033[2J\033[H");
    UARTprintf("Interrupts example\n\n");
    if(ui8Error)
    {
        if(ui8Error & 1)
        {
            UARTprintf("Equal Priority Fail!\n");
        }
        if(ui8Error & 2)
        {
            UARTprintf("Decreasing Priority Fail!\n");
        }
        if(ui8Error & 4)
        {
            UARTprintf("Increasing Priority Fail!\n");
        }
    }
    else
    {
        UARTprintf("Success!");
    }

    //
    // Loop forever.
    //
    while(1)
    {
    }
}
