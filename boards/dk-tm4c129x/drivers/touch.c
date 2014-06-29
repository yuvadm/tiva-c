//*****************************************************************************
//
// touch.c - Touch screen driver for the DK-TM4C129X.
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
#include "inc/hw_adc.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "drivers/touch.h"

//*****************************************************************************
//
//! \addtogroup touch_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// This driver operates in four different screen orientations.  They are:
//
// * Portrait - The screen is taller than it is wide, and the flex connector is
//              on the left of the display.  This is selected by defining
//              PORTRAIT.
//
// * Landscape - The screen is wider than it is tall, and the flex connector is
//               on the bottom of the display.  This is selected by defining
//               LANDSCAPE.
//
// * Portrait flip - The screen is taller than it is wide, and the flex
//                   connector is on the right of the display.  This is
//                   selected by defining PORTRAIT_FLIP.
//
// * Landscape flip - The screen is wider than it is tall, and the flex
//                    connector is on the top of the display.  This is
//                    selected by defining LANDSCAPE_FLIP.
//
// These can also be imagined in terms of screen rotation; if portrait mode is
// 0 degrees of screen rotation, landscape is 90 degrees of counter-clockwise
// rotation, portrait flip is 180 degrees of rotation, and landscape flip is
// 270 degress of counter-clockwise rotation.
//
// If no screen orientation is selected, landscape mode will be used.
//
//*****************************************************************************
#if ! defined(PORTRAIT) && ! defined(PORTRAIT_FLIP) &&  \
    ! defined(LANDSCAPE) && ! defined(LANDSCAPE_FLIP)
#define LANDSCAPE_FLIP
#endif

//*****************************************************************************
//
// The GPIO pins/ADC channels to which the touch screen is connected.
//
//*****************************************************************************
#define TS_XP_BASE              GPIO_PORTE_BASE
#define TS_XP_PIN               GPIO_PIN_7
#define TS_XP_ADC               ADC_CTL_CH21
#define TS_XN_BASE              GPIO_PORTT_BASE
#define TS_XN_PIN               GPIO_PIN_2
#define TS_YP_BASE              GPIO_PORTP_BASE
#define TS_YP_PIN               GPIO_PIN_7
#define TS_YP_ADC               ADC_CTL_CH22
#define TS_YN_BASE              GPIO_PORTT_BASE
#define TS_YN_PIN               GPIO_PIN_3

//*****************************************************************************
//
// Touchscreen calibration parameters.  Screen orientation is a build time
// selection.
//
//*****************************************************************************
const int32_t g_pi32TouchParameters[7] =
{
#ifdef PORTRAIT
    3840,                       // M0
    318720,                     // M1
    -297763200,                 // M2
    328576,                     // M3
    -8896,                      // M4
    -164591232,                 // M5
    3100080,                    // M6
#endif
#ifdef LANDSCAPE
    328192,                     // M0
    -4352,                      // M1
    -178717056,                 // M2
    1488,                       // M3
    -314592,                    // M4
    1012670064,                 // M5
    3055164,                     // M6
#endif
#ifdef PORTRAIT_FLIP
    1728,                       // M0
    -321696,                    // M1
    1034304336,                 // M2
    -325440,                    // M3
    1600,                       // M4
    1161009600,                 // M5
    3098070,                    // M6
#endif
#ifdef LANDSCAPE_FLIP
    -326400,                    // M0
    -1024,                      // M1
    1155718720,                 // M2
    3768,                       // M3
    312024,                     // M4
    -299081088,                 // M5
    3013754,                    // M6
#endif
};

//*****************************************************************************
//
// The lowest ADC reading assumed to represent a press on the screen.  Readings
// below this indicate no press is taking place.
//
//*****************************************************************************
#define TOUCH_MIN               150

//*****************************************************************************
//
// The current state of the touch screen driver's state machine.  This is used
// to cycle the touch screen interface through the powering sequence required
// to read the two axes of the surface.
//
//*****************************************************************************
static uint32_t g_ui32TSState;
#define TS_STATE_INIT           0
#define TS_STATE_SKIP_X         1
#define TS_STATE_READ_X         2
#define TS_STATE_SKIP_Y         3
#define TS_STATE_READ_Y         4

//*****************************************************************************
//
// The most recent raw ADC reading for the X position on the screen.  This
// value is not affected by the selected screen orientation.
//
//*****************************************************************************
volatile int16_t g_i16TouchX;

//*****************************************************************************
//
// The most recent raw ADC reading for the Y position on the screen.  This
// value is not affected by the selected screen orientation.
//
//*****************************************************************************
volatile int16_t g_i16TouchY;

//*****************************************************************************
//
// The minimum raw reading that should be considered valid press.
//
//*****************************************************************************
int16_t g_i16TouchMin = TOUCH_MIN;

//*****************************************************************************
//
// A pointer to the function to receive messages from the touch screen driver
// when events occur on the touch screen (debounced presses, movement while
// pressed, and debounced releases).
//
//*****************************************************************************
static int32_t (*g_pfnTSHandler)(uint32_t ui32Message, int32_t i32X,
                                 int32_t i32Y);

//*****************************************************************************
//
// The current state of the touch screen debouncer.  When zero, the pen is up.
// When three, the pen is down.  When one or two, the pen is transitioning from
// one state to the other.
//
//*****************************************************************************
static uint8_t g_ui8State = 0;

//*****************************************************************************
//
// The queue of debounced pen positions.  This is used to slightly delay the
// returned pen positions, so that the pen positions that occur while the pen
// is being raised are not send to the application.
//
//*****************************************************************************
static int16_t g_pi16Samples[8];

//*****************************************************************************
//
// The count of pen positions in g_pi16Samples.  When negative, the buffer is
// being pre-filled as a result of a detected pen down event.
//
//*****************************************************************************
static int8_t g_i8Index = 0;

//*****************************************************************************
//
//! Debounces presses of the touch screen.
//!
//! This function is called when a new X/Y sample pair has been captured in
//! order to perform debouncing of the touch screen.
//!
//! \return None.
//
//*****************************************************************************
static void
TouchScreenDebouncer(void)
{
    int32_t i32X, i32Y, i32Temp;

    //
    // Convert the ADC readings into pixel values on the screen.
    //
    i32X = g_i16TouchX;
    i32Y = g_i16TouchY;
    i32Temp = (((i32X * g_pi32TouchParameters[0]) +
                (i32Y * g_pi32TouchParameters[1]) + g_pi32TouchParameters[2]) /
               g_pi32TouchParameters[6]);
    i32Y = (((i32X * g_pi32TouchParameters[3]) +
             (i32Y * g_pi32TouchParameters[4]) + g_pi32TouchParameters[5]) /
            g_pi32TouchParameters[6]);
    i32X = i32Temp;

    //
    // See if the touch screen is being touched.
    //
    if((g_i16TouchX < g_i16TouchMin) || (g_i16TouchY < g_i16TouchMin))
    {
        //
        // If there are no valid values yet then ignore this state.
        //
        if((g_ui8State & 0x80) == 0)
        {
            g_ui8State = 0;
        }

        //
        // See if the pen is not up right now.
        //
        if(g_ui8State != 0x00)
        {
            //
            // Decrement the state count.
            //
            g_ui8State--;

            //
            // See if the pen has been detected as up three times in a row.
            //
            if(g_ui8State == 0x80)
            {
                //
                // Indicate that the pen is up.
                //
                g_ui8State = 0x00;

                //
                // See if there is a touch screen event handler.
                //
                if(g_pfnTSHandler)
                {
                    //
                    // If we got caught pre-filling the values, just return the
                    // first valid value as a press and release.  If this is
                    // not done there is a perceived miss of a press event.
                    //
                    if(g_i8Index < 0)
                    {
                        g_pfnTSHandler(WIDGET_MSG_PTR_DOWN, g_pi16Samples[0],
                                       g_pi16Samples[1]);
                        g_i8Index = 0;
                    }

                    //
                    // Send the pen up message to the touch screen event
                    // handler.
                    //
                    g_pfnTSHandler(WIDGET_MSG_PTR_UP, g_pi16Samples[g_i8Index],
                                   g_pi16Samples[g_i8Index + 1]);
                }
            }
        }
    }
    else
    {
        //
        // If the state was counting down above then fall back to the idle
        // state and start waiting for new values.
        //
        if((g_ui8State & 0x80) && (g_ui8State != 0x83))
        {
            //
            // Restart the release count down.
            //
            g_ui8State = 0x83;
        }

        //
        // See if the pen is not down right now.
        //
        if(g_ui8State != 0x83)
        {
            //
            // Increment the state count.
            //
            g_ui8State++;

            //
            // See if the pen has been detected as down three times in a row.
            //
            if(g_ui8State == 0x03)
            {
                //
                // Indicate that the pen is down.
                //
                g_ui8State = 0x83;

                //
                // Set the index to -8, so that the next 3 samples are stored
                // into the sample buffer before sending anything back to the
                // touch screen event handler.
                //
                g_i8Index = -8;

                //
                // Store this sample into the sample buffer.
                //
                g_pi16Samples[0] = i32X;
                g_pi16Samples[1] = i32Y;
            }
        }
        else
        {
            //
            // See if the sample buffer pre-fill has completed.
            //
            if(g_i8Index == -2)
            {
                //
                // See if there is a touch screen event handler.
                //
                if(g_pfnTSHandler)
                {
                    //
                    // Send the pen down message to the touch screen event
                    // handler.
                    //
                    g_pfnTSHandler(WIDGET_MSG_PTR_DOWN, g_pi16Samples[0],
                                   g_pi16Samples[1]);
                }

                //
                // Store this sample into the sample buffer.
                //
                g_pi16Samples[0] = i32X;
                g_pi16Samples[1] = i32Y;

                //
                // Set the index to the next sample to send.
                //
                g_i8Index = 2;
            }

            //
            // Otherwise, see if the sample buffer pre-fill is in progress.
            //
            else if(g_i8Index < 0)
            {
                //
                // Store this sample into the sample buffer.
                //
                g_pi16Samples[g_i8Index + 10] = i32X;
                g_pi16Samples[g_i8Index + 11] = i32Y;

                //
                // Increment the index.
                //
                g_i8Index += 2;
            }

            //
            // Otherwise, the sample buffer is full.
            //
            else
            {
                //
                // See if there is a touch screen event handler.
                //
                if(g_pfnTSHandler)
                {
                    //
                    // Send the pen move message to the touch screen event
                    // handler.
                    //
                    g_pfnTSHandler(WIDGET_MSG_PTR_MOVE,
                                   g_pi16Samples[g_i8Index],
                                   g_pi16Samples[g_i8Index + 1]);
                }

                //
                // Store this sample into the sample buffer.
                //
                g_pi16Samples[g_i8Index] = i32X;
                g_pi16Samples[g_i8Index + 1] = i32Y;

                //
                // Increment the index.
                //
                g_i8Index = (g_i8Index + 2) & 7;
            }
        }
    }
}

//*****************************************************************************
//
//! Handles the ADC interrupt for the touch screen.
//!
//! This function is called when the ADC sequence that samples the touch screen
//! has completed its acquisition.  The touch screen state machine is advanced
//! and the acquired ADC sample is processed appropriately.
//!
//! It is the responsibility of the application using the touch screen driver
//! to ensure that this function is installed in the interrupt vector table for
//! the ADC0 samples sequencer 3 interrupt.
//!
//! \return None.
//
//*****************************************************************************
void
TouchScreenIntHandler(void)
{
    //
    // Clear the ADC sample sequence interrupt.
    //
    HWREG(ADC0_BASE + ADC_O_ISC) = ADC_ISC_IN3;

    //
    // Determine what to do based on the current state of the state machine.
    //
    switch(g_ui32TSState)
    {
        //
        // The new sample is an X axis sample that should be discarded.
        //
        case TS_STATE_SKIP_X:
        {
            //
            // Read and throw away the ADC sample.
            //
            HWREG(ADC0_BASE + ADC_O_SSFIFO3);

            //
            // Set the analog mode select for the YP pin.
            //
            HWREG(TS_YP_BASE + GPIO_O_AMSEL) |= TS_YP_PIN;

            //
            // Configure the Y axis touch layer pins as inputs.
            //
            HWREG(TS_YP_BASE + GPIO_O_DIR) &= ~TS_YP_PIN;
            HWREG(TS_YN_BASE + GPIO_O_DIR) &= ~TS_YN_PIN;

            //
            // The next sample will be a valid X axis sample.
            //
            g_ui32TSState = TS_STATE_READ_X;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The new sample is an X axis sample that should be processed.
        //
        case TS_STATE_READ_X:
        {
            //
            // Read the raw ADC sample.
            //
            g_i16TouchX = HWREG(ADC0_BASE + ADC_O_SSFIFO3);

            //
            // Clear the analog mode select for the YP pin.
            //
            HWREG(TS_YP_BASE + GPIO_O_AMSEL) &= ~TS_YP_PIN;

            //
            // Configure the X and Y axis touch layers as outputs.
            //
            HWREG(TS_XP_BASE + GPIO_O_DIR) |= TS_XP_PIN;
            HWREG(TS_XN_BASE + GPIO_O_DIR) |= TS_XN_PIN;
            HWREG(TS_YP_BASE + GPIO_O_DIR) |= TS_YP_PIN;
            HWREG(TS_YN_BASE + GPIO_O_DIR) |= TS_YN_PIN;

            //
            // Drive the positive side of the Y axis touch layer with VDD and
            // the negative side with GND.  Also, drive both sides of the X
            // axis layer with GND to discharge any residual voltage (so that
            // a no-touch condition can be properly detected).
            //
            HWREG(TS_XP_BASE + GPIO_O_DATA + (TS_XP_PIN << 2)) = 0;
            HWREG(TS_XN_BASE + GPIO_O_DATA + (TS_XN_PIN << 2)) = 0;
            HWREG(TS_YP_BASE + GPIO_O_DATA + (TS_YP_PIN << 2)) = TS_YP_PIN;
            HWREG(TS_YN_BASE + GPIO_O_DATA + (TS_YN_PIN << 2)) = 0;

            //
            // Configure the sample sequence to capture the X axis value.
            //
            HWREG(ADC0_BASE + ADC_O_SSMUX3) = TS_XP_ADC;

            //
            // The next sample will be an invalid Y axis sample.
            //
            g_ui32TSState = TS_STATE_SKIP_Y;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The new sample is a Y axis sample that should be discarded.
        //
        case TS_STATE_SKIP_Y:
        {
            //
            // Read and throw away the ADC sample.
            //
            HWREG(ADC0_BASE + ADC_O_SSFIFO3);

            //
            // Set the analog mode select for the XP pin.
            //
            HWREG(TS_XP_BASE + GPIO_O_AMSEL) |= TS_XP_PIN;

            //
            // Configure the X axis touch layer pins as inputs.
            //
            HWREG(TS_XP_BASE + GPIO_O_DIR) &= ~TS_XP_PIN;
            HWREG(TS_XN_BASE + GPIO_O_DIR) &= ~TS_XN_PIN;

            //
            // The next sample will be a valid Y axis sample.
            //
            g_ui32TSState = TS_STATE_READ_Y;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The new sample is a Y axis sample that should be processed.
        //
        case TS_STATE_READ_Y:
        {
            //
            // Read the raw ADC sample.
            //
            g_i16TouchY = HWREG(ADC0_BASE + ADC_O_SSFIFO3);

            //
            // The next configuration is the same as the initial configuration.
            // Therefore, fall through into the initialization state to avoid
            // duplicating the code.
            //
        }

        //
        // The state machine is in its initial state
        //
        case TS_STATE_INIT:
        {
            //
            // Clear the analog mode select for the XP pin.
            //
            HWREG(TS_XP_BASE + GPIO_O_AMSEL) &= ~TS_XP_PIN;

            //
            // Configure the X and Y axis touch layers as outputs.
            //
            HWREG(TS_XP_BASE + GPIO_O_DIR) |= TS_XP_PIN;
            HWREG(TS_XN_BASE + GPIO_O_DIR) |= TS_XN_PIN;
            HWREG(TS_YP_BASE + GPIO_O_DIR) |= TS_YP_PIN;
            HWREG(TS_YN_BASE + GPIO_O_DIR) |= TS_YN_PIN;

            //
            // Drive one side of the X axis touch layer with VDD and the other
            // with GND.  Also, drive both sides of the Y axis layer with GND
            // to discharge any residual voltage (so that a no-touch condition
            // can be properly detected).
            //
            HWREG(TS_XP_BASE + GPIO_O_DATA + (TS_XP_PIN << 2)) = TS_XP_PIN;
            HWREG(TS_XN_BASE + GPIO_O_DATA + (TS_XN_PIN << 2)) = 0;
            HWREG(TS_YP_BASE + GPIO_O_DATA + (TS_YP_PIN << 2)) = 0;
            HWREG(TS_YN_BASE + GPIO_O_DATA + (TS_YN_PIN << 2)) = 0;

            //
            // Configure the sample sequence to capture the Y axis value.
            //
            HWREG(ADC0_BASE + ADC_O_SSMUX3) = TS_YP_ADC;

            //
            // If this is the valid Y sample state, then there is a new X/Y
            // sample pair.  In that case, run the touch screen debouncer.
            //
            if(g_ui32TSState == TS_STATE_READ_Y)
            {
                TouchScreenDebouncer();
            }

            //
            // The next sample will be an invalid X axis sample.
            //
            g_ui32TSState = TS_STATE_SKIP_X;

            //
            // This state has been handled.
            //
            break;
        }
    }
}

//*****************************************************************************
//
//! Initializes the touch screen driver.
//!
//! \param ui32SysClock is the frequency of the system clock.
//!
//! This function initializes the touch screen driver, beginning the process of
//! reading from the touch screen.  This driver uses the following hardware
//! resources:
//!
//! - ADC 0 sample sequence 3
//! - Timer 5 subtimer B
//!
//! \return None.
//
//*****************************************************************************
void
TouchScreenInit(uint32_t ui32SysClock)
{
    //
    // Set the initial state of the touch screen driver's state machine.
    //
    g_ui32TSState = TS_STATE_INIT;

    //
    // There is no touch screen handler initially.
    //
    g_pfnTSHandler = 0;

    //
    // Enable the peripherals used by the touch screen interface.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER5);

    //
    // Configure the ADC sample sequence used to read the touch screen reading.
    //
    ROM_ADCHardwareOversampleConfigure(ADC0_BASE, 4);
    ROM_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_TIMER, 0);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 3, 0,
                                 TS_YP_ADC | ADC_CTL_END | ADC_CTL_IE);
    ROM_ADCSequenceEnable(ADC0_BASE, 3);

    //
    // Enable the ADC sample sequence interrupt.
    //
    ROM_ADCIntEnable(ADC0_BASE, 3);
    ROM_IntEnable(INT_ADC0SS3);

    //
    // Configure the timer to trigger the sampling of the touch screen
    // every 2.5 milliseconds.
    //
    if((HWREG(TIMER5_BASE + TIMER_O_CTL) & TIMER_CTL_TAEN) == 0)
    {
        ROM_TimerConfigure(TIMER5_BASE, (TIMER_CFG_SPLIT_PAIR |
                                         TIMER_CFG_A_PWM |
                                         TIMER_CFG_B_PERIODIC));
    }
    ROM_TimerPrescaleSet(TIMER5_BASE, TIMER_B, 255);
    ROM_TimerLoadSet(TIMER5_BASE, TIMER_B, ((ui32SysClock / 256) / 400) - 1);
    TimerControlTrigger(TIMER5_BASE, TIMER_B, true);

    //
    // Enable the timer.  At this point, the touch screen state machine will
    // sample and run every 2.5 ms.
    //
    ROM_TimerEnable(TIMER5_BASE, TIMER_B);
}

//*****************************************************************************
//
//! Sets the callback function for touch screen events.
//!
//! \param pfnCallback is a pointer to the function to be called when touch
//! screen events occur.
//!
//! This function sets the address of the function to be called when touch
//! screen events occur.  The events that are recognized are the screen being
//! touched (``pen down''), the touch position moving while the screen is
//! touched (``pen move''), and the screen no longer being touched (``pen
//! up'').
//!
//! \return None.
//
//*****************************************************************************
void
TouchScreenCallbackSet(int32_t (*pfnCallback)(uint32_t ui32Message,
                                              int32_t i32X, int32_t i32Y))
{
    //
    // Save the pointer to the callback function.
    //
    g_pfnTSHandler = pfnCallback;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
