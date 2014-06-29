//*****************************************************************************
//
// interrupts.c - Interrupt preemption and tail-chaining example.
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
// This is part of revision 2.1.0.12573 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
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
//! interrupt will be displayed on the display; GPIO pins E1, E2 and E3 will
//! be asserted upon interrupt handler entry and de-asserted before interrupt
//! handler exit so that the off-to-on time can be observed with a scope or
//! logic analyzer to see the speed of tail-chaining (for the two cases where
//! tail-chaining is occurring).
//
//*****************************************************************************

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
// seconds are guaranteed, aint32_t with the remainder of the current second).
//
//*****************************************************************************
void
Delay(uint32_t ui32Seconds)
{
    //
    // Loop while there are more seconds to wait.
    //
    while(ui32Seconds--)
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

    //
    // Display the currently active interrupts.
    //
    ui32Temp = HWREG(NVIC_ACTIVE0);
    UARTprintf("\rActive: %c%c%c ", (ui32Temp & 1) ? '1' : ' ',
               (ui32Temp & 2) ? '2' : ' ', (ui32Temp & 4) ? '3' : ' ');

    //
    // Display the currently pending interrupts.
    //
    ui32Temp = HWREG(NVIC_PEND0);
    UARTprintf("Pending: %c%c%c", (ui32Temp & 1) ? '1' : ' ',
               (ui32Temp & 2) ? '2' : ' ', (ui32Temp & 4) ? '3' : ' ');
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
    // Set PE1 high to indicate entry to this interrupt handler.
    //
    ROM_GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_PIN_1);

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
    // Set PE1 low to indicate exit from this interrupt handler.
    //
    ROM_GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0);
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
    // Set PE2 high to indicate entry to this interrupt handler.
    //
    ROM_GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, GPIO_PIN_2);

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
    // Set PE2 low to indicate exit from this interrupt handler.
    //
    ROM_GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, 0);
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
    // Set PE3 high to indicate entry to this interrupt handler.
    //
    ROM_GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, GPIO_PIN_3);

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
    // Set PE3 low to indicate exit from this interrupt handler.
    //
    ROM_GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0);
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
// This is the main example program.  It checks to see that the interrupts are
// processed in the correct order when they have identical priorities,
// increasing priorities, and decreasing priorities.  This exercises interrupt
// preemption and tail chaining.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32Error;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Enable the peripherals used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    //
    // Initialize the UART.
    //
    ConfigureUART();

    UARTprintf("\033[2JInterrupts\n");

    //
    // Configure the PB0-PB2 to be outputs to indicate entry/exit of one
    // of the interrupt handlers.
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_1 | GPIO_PIN_2 |
                                               GPIO_PIN_3);
    ROM_GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);

    //
    // Set up and enable the SysTick timer.  It will be used as a reference
    // for delay loops in the interrupt handlers.  The SysTick timer period
    // will be set up for one second.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet());
    ROM_SysTickEnable();

    //
    // Reset the error indicator.
    //
    ui32Error = 0;

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
    UARTprintf("\nEqual Priority\n");

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
        ui32Error |= 1;
    }

    //
    // Wait two seconds.
    //
    Delay(2);

    //
    // Indicate that the decreasing interrupt priority test is beginning.
    //
    UARTprintf("\nDecreasing Priority\n");

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
        ui32Error |= 2;
    }

    //
    // Wait two seconds.
    //
    Delay(2);

    //
    // Indicate that the increasing interrupt priority test is beginning.
    //
    UARTprintf("\nIncreasing Priority\n");

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
        ui32Error |= 4;
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
    UARTprintf("\nInterrupt Priority =: %s  >: %s  <: %s\n",
               (ui32Error & 1) ? "Fail" : "Pass",
               (ui32Error & 2) ? "Fail" : "Pass",
               (ui32Error & 4) ? "Fail" : "Pass");

    //
    // Loop forever.
    //
    while(1)
    {
    }
}
