//*****************************************************************************
//
// sleep_modes.c - Sleep modes example.
//
// Copyright (c) 2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "drivers/buttons.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Sleep Modes(sleep_modes)</h1>
//!
//! This example demonstrates the different power modes available on the Tiva
//! C Series devices. The user button (USR-SW1) is used to cycle through the
//! different power modes.
//! The SRAM, Flash, and LDO are all configured to a lower power setting for
//! the different modes.
//!
//! A timer is configured to toggle an LED in an ISR in both Run and Sleep
//! mode.
//! In Deep-Sleep the PWM is used to toggle the same LED in hardware. The three
//! remaining LEDs are used to indicate the current power mode.
//!
//!         LED key in addition to the toggling LED:
//!             3 LEDs on - Run Mode
//!             2 LEDs on - Sleep Mode
//!             1 LED on - Deep-Sleep Mode
//!
//! UART0, connected to the Virtual Serial Port and running at 115,200, 8-N-1,
//! is used to display messages from this application.
//
//*****************************************************************************

//****************************************************************************
//
// Status LED defines.
//
//****************************************************************************

//
// PF4
//
#define RUN_GPIO_SYSCTL     SYSCTL_PERIPH_GPIOF
#define RUN_GPIO_BASE       GPIO_PORTF_BASE
#define RUN_GPIO_PIN        GPIO_PIN_4

//
// PN0
//
#define SLEEP_GPIO_SYSCTL   SYSCTL_PERIPH_GPION
#define SLEEP_GPIO_BASE     GPIO_PORTN_BASE
#define SLEEP_GPIO_PIN      GPIO_PIN_0

//
// PN1
//
#define DSLEEP_GPIO_SYSCTL  SYSCTL_PERIPH_GPION
#define DSLEEP_GPIO_BASE    GPIO_PORTN_BASE
#define DSLEEP_GPIO_PIN     GPIO_PIN_1

//
// PF0
//
#define TOGGLE_GPIO_SYSCTL  SYSCTL_PERIPH_GPIOF
#define TOGGLE_GPIO_BASE    GPIO_PORTF_BASE
#define TOGGLE_GPIO_PIN     GPIO_PIN_0

//****************************************************************************
//
// System clock rate in Hz.
//
//****************************************************************************
uint32_t g_ui32SysClock;

//****************************************************************************
//
// Global to track power state:
// 0 - Run Mode
// 1 - Sleep Mode
// 2 - Deep-Sleep Mode
//
//****************************************************************************
volatile uint32_t g_ui32SleepMode = 0;

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

    //
    // Enable UART0.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

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
// The function for setting the state of the LEDs for indicating the current
// power mode.
//
//*****************************************************************************
void
PowerLEDsSet(void)
{
    switch (g_ui32SleepMode)
    {
        case 0:

            //
            // Turn on LED(s) to indicate we are in Run Mode.
            //
            ROM_GPIOPinWrite(RUN_GPIO_BASE, RUN_GPIO_PIN, RUN_GPIO_PIN);
            ROM_GPIOPinWrite(SLEEP_GPIO_BASE, SLEEP_GPIO_PIN, SLEEP_GPIO_PIN);
            ROM_GPIOPinWrite(DSLEEP_GPIO_BASE, DSLEEP_GPIO_PIN,
                    DSLEEP_GPIO_PIN);

            break;

        case 1:

            //
            // Turn on LED(s) to indicate we are in Sleep Mode.
            //
            ROM_GPIOPinWrite(RUN_GPIO_BASE, RUN_GPIO_PIN, ~RUN_GPIO_PIN);
            ROM_GPIOPinWrite(SLEEP_GPIO_BASE, SLEEP_GPIO_PIN, SLEEP_GPIO_PIN);
            ROM_GPIOPinWrite(DSLEEP_GPIO_BASE, DSLEEP_GPIO_PIN,
                    DSLEEP_GPIO_PIN);

            break;

        case 2:

            //
            // Turn on LED(s) to indicate we are in Deep-Sleep Mode.
            //
            ROM_GPIOPinWrite(RUN_GPIO_BASE, RUN_GPIO_PIN, ~RUN_GPIO_PIN);
            ROM_GPIOPinWrite(SLEEP_GPIO_BASE, SLEEP_GPIO_PIN, ~SLEEP_GPIO_PIN);
            ROM_GPIOPinWrite(DSLEEP_GPIO_BASE, DSLEEP_GPIO_PIN,
                    DSLEEP_GPIO_PIN);

            break;

        default:
            break;
    }
}

//*****************************************************************************
//
// The interrupt handler for the button interrupt.
//
//*****************************************************************************
void
ButtonIntHandler(void)
{
    //
    // Delay here on button push for simple debouncing.
    //
    SysCtlDelay(g_ui32SysClock / 10);

    //
    // Clear the timer interrupt.
    //
    ROM_GPIOIntClear(GPIO_PORTJ_AHB_BASE, GPIO_INT_PIN_0);

    //
    // Increment Mode.
    //
    g_ui32SleepMode = (g_ui32SleepMode + 1) % 3;

    switch (g_ui32SleepMode)
    {
        //
        // Enter Run Mode.
        //
        case 0:

            //
            // Disable the PWM.
            //
            ROM_PWMGenDisable(PWM0_BASE, PWM_GEN_0);

            //
            // Configure Toggle LED as a GPIO output.
            //
            ROM_GPIOPinTypeGPIOOutput(TOGGLE_GPIO_BASE, TOGGLE_GPIO_PIN);

            //
            // Enable the timer.
            //
            ROM_TimerEnable(TIMER0_BASE, TIMER_A);

            //
            // Print mode over the UART.
            //
            UARTprintf("\033[100D");
            UARTprintf("\033[K");
            UARTprintf("Run\t\tMOSC with PLL\tTimer");
            SysCtlDelay(10000);
            break;

            //
            // Enter Sleep Mode.
            //
        case 1:

            //
            // Print mode over the UART.
            // Delay to let the UART finish before going to Sleep.
            //
            UARTprintf("\033[100D");
            UARTprintf("\033[K");
            UARTprintf("Sleep\t\tPIOSC\t\tTimer");
            SysCtlDelay(10000);

            //
            // Switch clock to PIOSC and power down the MOSC before going into 
            // Sleep.
            //
            g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_OSC_INT |
                                                      SYSCTL_USE_OSC |
                                                      SYSCTL_MAIN_OSC_DIS), 
                                                      16000000);

            break;

            //
            // Enter Deep-Sleep Mode.
            //
        case 2:

            //
            // Switch back to the MOSC + PLL.
            //
            g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                                      SYSCTL_OSC_MAIN |
                                                      SYSCTL_USE_PLL |
                                                      SYSCTL_CFG_VCO_320), 
                                                      16000000);

            //
            // Disable the timer.
            //
            ROM_TimerDisable(TIMER0_BASE, TIMER_A);

            //
            // Configure the toggle pin as a PWM pin.
            //
            ROM_GPIOPinConfigure(GPIO_PF0_M0PWM0);
            ROM_GPIOPinTypePWM(TOGGLE_GPIO_BASE, TOGGLE_GPIO_PIN);

            //
            // Enable the PWM.
            //
            ROM_PWMGenEnable(PWM0_BASE, PWM_GEN_0);

            //
            // Print mode over the UART.
            // Delay to let the UART finish before going to Sleep.
            //
            UARTprintf("\033[100D");
            UARTprintf("\033[K");
            UARTprintf("Deep-Sleep\tLFIOSC\t\tPWM");
            SysCtlDelay(10000);
            break;

        default:
            break;
    }

    //
    // Set LEDs to show what mode we are in.
    //
    PowerLEDsSet();
}


//*****************************************************************************
//
// The interrupt handler for the timer interrupt.
//
//*****************************************************************************
void
Timer0IntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    //
    // Toggle the LED.
    //
    ROM_GPIOPinWrite(TOGGLE_GPIO_BASE, TOGGLE_GPIO_PIN,
            (ROM_GPIOPinRead(TOGGLE_GPIO_BASE, TOGGLE_GPIO_PIN) ^
             TOGGLE_GPIO_PIN));
}

//*****************************************************************************
//
// This example application demonstrates the use of the different sleep modes
// and different power configuration options.
//
//*****************************************************************************
int
main(void)
{
    //
    // Set the clocking to run from the MOSC with the PLL at 16MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                              SYSCTL_OSC_MAIN |
                                              SYSCTL_USE_PLL |
                                              SYSCTL_CFG_VCO_320), 16000000);


    //
    // Set the clocking for Deep-Sleep.
    // Power down the PIOSC & MOSC to save power and run from the
    // internal 30kHz osc.
    //
    ROM_SysCtlDeepSleepClockConfigSet(1, (SYSCTL_DSLP_OSC_INT30 |
                SYSCTL_DSLP_PIOSC_PD | SYSCTL_DSLP_MOSC_PD));

    //
    // Initialize the UART and write the banner.
    // Indicate we are currently in Run Mode.
    //
    ConfigureUART();
    UARTprintf("\033[2J\033[H");
    UARTprintf("Sleep Modes example\n\n");
    UARTprintf("Mode:\t\tClock Source:\tLED Toggle Source:");
    UARTprintf("\nRun\t\tMOSC with PLL\tTimer");

    //
    // Initialize the buttons driver.
    //
    ButtonsInit();

    //
    // Enable the interrupt for the button.
    //
    ROM_GPIOIntEnable(GPIO_PORTJ_AHB_BASE, GPIO_INT_PIN_0);

    //
    // Enable the GPIO ports that are used for the on-board LEDs.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //
    // Set pad config.
    //
    MAP_GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0,
                          GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    //
    // Set direction.
    //
    ROM_GPIODirModeSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_DIR_MODE_IN);

    //
    // Enable the interrupt for the button.
    //
    ROM_GPIOIntEnable(GPIO_PORTJ_BASE, GPIO_INT_PIN_0);

    //
    // Enable interrupt to NVIC.
    //
    ROM_IntEnable(INT_GPIOJ);

    //
    // Enable the GPIO ports that are used for the on-board LEDs.
    //
    ROM_SysCtlPeripheralEnable(RUN_GPIO_SYSCTL);
    ROM_SysCtlPeripheralEnable(SLEEP_GPIO_SYSCTL);
    ROM_SysCtlPeripheralEnable(DSLEEP_GPIO_SYSCTL);
    ROM_SysCtlPeripheralEnable(TOGGLE_GPIO_SYSCTL);

    //
    // Enable the GPIO pins for the LED.
    //
    ROM_GPIOPinTypeGPIOOutput(RUN_GPIO_BASE, RUN_GPIO_PIN);
    ROM_GPIOPinTypeGPIOOutput(SLEEP_GPIO_BASE, SLEEP_GPIO_PIN);
    ROM_GPIOPinTypeGPIOOutput(DSLEEP_GPIO_BASE, DSLEEP_GPIO_PIN);
    ROM_GPIOPinTypeGPIOOutput(TOGGLE_GPIO_BASE, TOGGLE_GPIO_PIN);

    //
    // Enable the peripherals used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    //
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();

    //
    // Configure the 32-bit periodic timer.
    //
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock);

    //
    // Setup the interrupts for the timer timeout.
    //
    ROM_IntEnable(INT_TIMER0A);
    ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    //
    // Configure the PWM0 to count down without synchronization.
    // This will be used in Deep-Sleep.
    //
    ROM_PWMGenConfigure(PWM0_BASE, PWM_GEN_0,
                        PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    //
    // Enable the PWM0 output signal.
    //
    ROM_PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);

    //
    // Set up the period to match the timer.
    //
    ROM_PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, 65000);

    //
    // Configure the PWM for a 50% duty cycle.
    //
    ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 65000 >> 1);

    //
    // Enable the timer.
    //
    ROM_TimerEnable(TIMER0_BASE, TIMER_A);

    //
    // Enable the Timer in Sleep Mode.
    //
    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);

    //
    // Enable the PWM in Deep-Sleep Mode.
    //
    ROM_SysCtlPeripheralDeepSleepEnable(SYSCTL_PERIPH_PWM0);

    //
    // Enable the Button Port in Sleep & Deep-Sleep Mode.
    //
    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOJ);
    ROM_SysCtlPeripheralDeepSleepEnable(SYSCTL_PERIPH_GPIOJ);

    //
    // Enable the LED Ports in Sleep & Deep-Sleep Mode.
    //
    ROM_SysCtlPeripheralSleepEnable(SLEEP_GPIO_SYSCTL);
    ROM_SysCtlPeripheralDeepSleepEnable(DSLEEP_GPIO_SYSCTL);
    ROM_SysCtlPeripheralSleepEnable(TOGGLE_GPIO_SYSCTL);
    ROM_SysCtlPeripheralDeepSleepEnable(TOGGLE_GPIO_SYSCTL);

    //
    // Enable Auto Clock Gating Control.
    //
    ROM_SysCtlPeripheralClockGating(true);

    //
    // Set LDO to 1.15V in Sleep.
    // Set LDO to 1.10V in Deep-Sleep.
    //
    SysCtlLDOSleepSet(SYSCTL_LDO_1_15V);
    SysCtlLDODeepSleepSet(SYSCTL_LDO_1_10V);

    //
    // Set SRAM to Standby when in Sleep Mode.
    // Set Flash & SRAM to Low Power in Deep-Sleep Mode.
    //
    SysCtlSleepPowerSet(SYSCTL_SRAM_STANDBY);
    SysCtlDeepSleepPowerSet(SYSCTL_FLASH_LOW_POWER | SYSCTL_SRAM_LOW_POWER);

    //
    // Call to set initial LED power state.
    //
    PowerLEDsSet();

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Handle going into the different sleep modes outside of
        // interrupt context.
        //
        if (g_ui32SleepMode == 1)
        {
            //
            // Go into Sleep Mode.
            //
            ROM_SysCtlSleep();
        }
        else if (g_ui32SleepMode == 2)
        {
            //
            // Go into Deep-Sleep Mode.
            //
            ROM_SysCtlDeepSleep();
        }
    }
}

