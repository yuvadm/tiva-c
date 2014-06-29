//*****************************************************************************
//
// usb_host_audioin.c - Main routine for the USB host audio input example.
//
// Copyright (c) 2010-2014 Texas Instruments Incorporated.  All rights reserved.
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
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_sysctl.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/udma.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/slider.h"
#include "grlib/listbox.h"
#include "grlib/pushbutton.h"
#include "utils/ustdlib.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/usb_sound.h"
#include "drivers/sound.h"
#include "drivers/touch.h"
#include "drivers/pinout.h"
#include "drivers/usb_sound.h"
#include "usblib/usblib.h"
#include "usblib/host/usbhost.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB host audio example application using a USB audio device for input
//! and PWM audio to the on board speaker for output. (usb_host_audioin)</h1>
//!
//! This example application demonstrates streaming audio from a USB audio
//! device that supports recording an audio source at 48000 16 bit stereo.
//! The application starts recording audio from the USB audio device when
//! the "Record" button is pressed and stream it to the speaker on the board.
//! Because some audio devices require more power, you may need to use an
//! external 5 volt supply to provide enough power to the USB audio device.
//!
//! The application can be recompiled to run using and external USB phy to
//! implement a high speed host using an external USB phy.  To use the external
//! phy the application must be built with \b USE_ULPI defined.  This disables
//! the internal phy and the connector on the DK-TM4C129X board and enables the
//! connections to the external ULPI phy pins on the DK-TM4C129X board.
//
//*****************************************************************************

//*****************************************************************************
//
// Interrupt priority definitions.  The top 3 bits of these values are
// significant with lower values indicating higher priority interrupts.
//
//*****************************************************************************
#define AUDIO_INT_PRIORITY      0x00
#define ADC3_INT_PRIORITY       0x80

//*****************************************************************************
//
// Forward declarations for functions called by the widgets used in the user
// interface.
//
//*****************************************************************************
static void OnRecord(tWidget *pWidget);

//*****************************************************************************
//
// Audio buffering definitions, these are optimized to deal with USB audio.
//
//*****************************************************************************
#define USB_TRANSFER_SIZE       (192)
#define USB_BUFFERS             (18)
#define USB_AUDIO_BUFFER_SIZE   (USB_TRANSFER_SIZE * USB_BUFFERS)
#define PWM_AUDIO_BUFFER_SIZE   (USB_AUDIO_BUFFER_SIZE / (2 * 3 * 2))
#define AUDIO_MIN_DIFF          (USB_TRANSFER_SIZE * ((USB_BUFFERS >> 1) - 1))
#define AUDIO_NOMINAL_DIFF      (USB_TRANSFER_SIZE * (USB_BUFFERS >> 1))
#define AUDIO_MAX_DIFF          (USB_TRANSFER_SIZE * ((USB_BUFFERS >> 1) + 1))
uint32_t g_ui32SysClock;

//*****************************************************************************
//
// The USB audio buffer and it's pointers.
//
//*****************************************************************************
static uint8_t g_pui8USBAudioBuffer[USB_AUDIO_BUFFER_SIZE];
static uint8_t *g_pui8USBWrite;
static volatile uint32_t g_ui32USBCount;

//*****************************************************************************
//
// The PWM audio buffer and it's pointers.
//
//*****************************************************************************
static int16_t g_pi16PWMAudioBuffer[PWM_AUDIO_BUFFER_SIZE];
static int32_t i32PWMAudioIdx;

//*****************************************************************************
//
// Graphics context used to show text on the display.
//
//*****************************************************************************
static tContext g_sContext;

//*****************************************************************************
//
// Variable status string for the application.
//
//*****************************************************************************
#define STATUS_SIZE             40
static char g_pcStatusText[STATUS_SIZE];

//*****************************************************************************
//
// Holds global flags for the system.
//
//*****************************************************************************
static uint32_t g_ui32Flags;

//
// Currently streaming audio to the USB device.
//
#define FLAGS_STREAMING         1

//
// New audio device present.
//
#define FLAGS_DEVICE_CONNECT    2

//
// New audio device present.
//
#define FLAGS_DEVICE_READY      3

//*****************************************************************************
//
// Widget definitions
//
//*****************************************************************************

//*****************************************************************************
//
// Defines for the basic screen area used by the application.
//
//*****************************************************************************
#define STATUS_HEIGHT           40
#define BG_MIN_X                7
#define BG_MAX_X                (320 - 8)
#define BG_MIN_Y                24
#define BG_MAX_Y                (240 - 8)
#define BUTTON_HEIGHT           (STATUS_HEIGHT - 8)

//*****************************************************************************
//
// The list box used to display directory contents.
//
//*****************************************************************************
tCanvasWidget g_sStatusPanel;

//
// Status text area.
//
Canvas(g_sStatusText, &g_sStatusPanel, 0, 0, &g_sKentec320x240x16_SSD2119,
       BG_MIN_X + 112, BG_MAX_Y - STATUS_HEIGHT + 4, 189, BUTTON_HEIGHT,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrLightGrey, ClrDarkGray, ClrBlack,
       g_psFontCmss16, "", 0, 0);

//
// Record button.
//
RectangularButton(g_sRecord, &g_sStatusPanel, &g_sStatusText, 0,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X + 4,
       BG_MAX_Y - STATUS_HEIGHT + 4, 50, BUTTON_HEIGHT,
       PB_STYLE_FILL | PB_STYLE_TEXT |
       PB_STYLE_RELEASE_NOTIFY, ClrLightGrey, ClrDarkGray, 0,
       ClrBlack, g_psFontCmss16, "Record", 0, 0, 0 ,0 , OnRecord);

//
// Background of the status area behind the buttons.
//
Canvas(g_sStatusPanel, WIDGET_ROOT, 0, &g_sRecord,
       &g_sKentec320x240x16_SSD2119, BG_MIN_X, BG_MAX_Y - STATUS_HEIGHT,
       BG_MAX_X - BG_MIN_X, STATUS_HEIGHT,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_TOP, ClrGray, ClrWhite, ClrBlack, 0,
       0, 0, 0);

//*****************************************************************************
//
// The PWM audio callback from the sound driver for the dk-tm4c129x board.
//
//*****************************************************************************
void
PWMAudioCallback(uint32_t ui32Half)
{
    int32_t i32Current, i32PeriodAdj;

    //
    // Initial sound period adjustment is 0.
    //
    i32PeriodAdj = 0;

    //
    // Calculate the number of buffers that we are from ideal.
    //
    i32Current = ((int32_t)g_pui8USBAudioBuffer - (int32_t)g_pui8USBWrite +
              (USB_AUDIO_BUFFER_SIZE / 2)) / (int32_t)(USB_TRANSFER_SIZE);

    //
    // Make sample rate adjustments based on being in the top half of the
    // ping-pong buffer.
    //
    if(ui32Half)
    {
        //
        // USB is faster than the PWM audio.
        //
        if(g_ui32USBCount > (USB_BUFFERS / 2))
        {
            //
            // PWM audio is running slow so speed it up and handle the sign
            // properly.
            //
            if(i32Current < 0)
            {
                i32PeriodAdj = i32Current;
            }
            else
            {
                i32PeriodAdj = -i32Current;
            }
        }
        else if(g_ui32USBCount < (USB_BUFFERS / 2))
        {
            //
            // PWM audio is running fast so slow it down and handle the sign
            // properly.
            //
            if(i32Current < 0)
            {
                i32PeriodAdj = -i32Current;
            }
            else
            {
                i32PeriodAdj = i32Current;
            }
        }
    }
    else
    {
        //
        // The measurements are reversed when in the bottom half.
        //
        i32Current = -i32Current;

        //
        // USB is faster than the PWM audio.
        //
        if(g_ui32USBCount > (USB_BUFFERS / 2))
        {
            //
            // PWM audio is running slow so speed it up and handle the sign
            // properly.
            //
            if(i32Current < 0)
            {
                i32PeriodAdj = i32Current;
            }
            else
            {
                i32PeriodAdj = -i32Current;
            }
        }
        else if(g_ui32USBCount < (USB_BUFFERS / 2))
        {
            //
            // PWM audio is running fast so slow it down and handle the sign
            // properly.
            //
            if(i32Current < 0)
            {
                i32PeriodAdj = -i32Current;
            }
            else
            {
                i32PeriodAdj = i32Current;
            }
        }
    }

    //
    // Was there an adjustment to make?
    //
    if(i32PeriodAdj != 0)
    {
        SoundPeriodAdjust(i32PeriodAdj);
    }

    //
    // Reset the USB audio buffer count.
    //
    g_ui32USBCount = 0;
}


//*****************************************************************************
//
// Schedules new USB Isochronous input from the USB audio device when a
// previous transfer has completed.
//
//*****************************************************************************
static void
USBAudioInCallback(void *pvBuffer, uint32_t ui32Event, uint32_t ui32Value)
{
    int32_t i32Idx;
    int16_t *pi16Buffer;

    //
    // Increase the number of USB audio buffers received.
    //
    g_ui32USBCount++;

    //
    // Create a 16-bit pointer to the data.
    //
    pi16Buffer = (int16_t *)g_pui8USBWrite;

    //
    // If a buffer has been played then schedule a new one to play.
    //
    if((ui32Event == USB_EVENT_RX_AVAILABLE) &&
       (HWREGBITW(&g_ui32Flags, FLAGS_STREAMING)))
    {
        //
        // Increment the read pointer.
        //
        g_pui8USBWrite += USB_TRANSFER_SIZE;

        //
        // Wrap the read pointer if necessary.
        //
        if(g_pui8USBWrite >= (g_pui8USBAudioBuffer + USB_AUDIO_BUFFER_SIZE))
        {
            g_pui8USBWrite = g_pui8USBAudioBuffer;
        }

        //
        // Schedule a new USB audio buffer to be transmitted to the USB
        // audio device.
        //
        USBSoundBufferIn(g_pui8USBWrite, USB_TRANSFER_SIZE, USBAudioInCallback);

        //
        // This copy throws away 2 out of 3 samples to match the basic
        // sample rate difference of 48kHz versus 16kHz.
        //
        for(i32Idx = 0; i32Idx < (ui32Value >> 1); i32Idx+=6)
        {
            //
            // Basic stereo mix to mono.
            //
            g_pi16PWMAudioBuffer[i32PWMAudioIdx] = pi16Buffer[i32Idx] +
                                       pi16Buffer[i32Idx + 1];

            i32PWMAudioIdx++;

            if(i32PWMAudioIdx >= PWM_AUDIO_BUFFER_SIZE)
            {
                i32PWMAudioIdx = 0;
            }
        }
    }
}

//*****************************************************************************
//
// This function starts up the audio streaming from the USB audio device.  The
// PWM audio is started later when enough audio has been received to start
// transferring buffers to the PWM audio interface.
//
//*****************************************************************************
static void
StartStreaming(void)
{
    int i;

    //
    // Change the text on the button to Stop.
    //
    PushButtonTextSet(&g_sRecord, "Stop");
    WidgetPaint((tWidget *)&g_sRecord);

    //
    // Zero out the PWM audio buffer.
    //
    for(i=0; i< PWM_AUDIO_BUFFER_SIZE; i++)
    {
        g_pi16PWMAudioBuffer[i] = 0;
    }

    //
    // Initialize the USB audio write pointer to the middle of the buffer.
    //
    g_pui8USBWrite = g_pui8USBAudioBuffer + (USB_AUDIO_BUFFER_SIZE / 2);

    //
    // Initialize the fill position of the PWM audio buffer to the middle of
    // its buffer.
    //
    i32PWMAudioIdx = PWM_AUDIO_BUFFER_SIZE/2;

    //
    // Initialize the USB audio count.
    //
    g_ui32USBCount = 0;

    //
    // Initialize the PWM audio and start playing.
    //
    SoundInit(g_ui32SysClock);
    SoundStart((int16_t *)g_pi16PWMAudioBuffer, PWM_AUDIO_BUFFER_SIZE, 16000,
               PWMAudioCallback);

    //
    // Request and audio buffer from the USB device.
    //
    USBSoundBufferIn(g_pui8USBWrite, USB_TRANSFER_SIZE, USBAudioInCallback);
}

//*****************************************************************************
//
// Stops audio streaming for the application.
//
//*****************************************************************************
static void
StopAudio(void)
{
    int32_t i32Idx;
    uint32_t *pui32Buffer;

    //
    // Stop playing audio.
    //
    HWREGBITW(&g_ui32Flags, FLAGS_STREAMING) = 0;

    //
    // Do 32-bit accesses to clear out the buffer.
    //
    pui32Buffer = (uint32_t *)g_pui8USBAudioBuffer;

    //
    // Zero out the buffer.
    //
    for(i32Idx = 0; i32Idx < (USB_AUDIO_BUFFER_SIZE >> 2); i32Idx++)
    {
        pui32Buffer[i32Idx] = 0;
    }

    //
    // Initialize the read and write pointers.
    //
    g_pui8USBWrite = g_pui8USBAudioBuffer + (USB_AUDIO_BUFFER_SIZE / 2);

    //
    // Change the text on the button to Stop.
    //
    PushButtonTextSet(&g_sRecord, "Record");
    WidgetPaint((tWidget *)&g_sRecord);

    SoundStop();
}

//*****************************************************************************
//
// The "Play/Stop" button widget call back function.
//
// This function is called whenever someone presses the "Play/Stop" button.
//
//*****************************************************************************
static void
OnRecord(tWidget *pWidget)
{
    //
    // Nothing to do if not ready yet.
    //
    if(HWREGBITW(&g_ui32Flags, FLAGS_DEVICE_READY))
    {
        //
        // Determine if this was a Play or Stop command.
        //
        if(HWREGBITW(&g_ui32Flags, FLAGS_STREAMING))
        {
            //
            // If already playing then this was a press to stop play back.
            //
            StopAudio();
        }
        else
        {
            //
            // Indicate that audio streaming should start.
            //
            HWREGBITW(&g_ui32Flags, FLAGS_STREAMING) = 1;
            StartStreaming();
        }
    }
}

//*****************************************************************************
//
// This function handled global level events for the USB host audio.  This
// function was passed into the USBSoundInit() function.
//
//*****************************************************************************
static void
AudioEvent(uint32_t ui32Event, uint32_t ui32Param)
{
    switch(ui32Event)
    {
        case SOUND_EVENT_READY:
        {
            //
            // Flag that a new audio device is present.
            //
            HWREGBITW(&g_ui32Flags, FLAGS_DEVICE_CONNECT) = 1;

            break;
        }
        case SOUND_EVENT_DISCONNECT:
        {
            //
            // Device is no longer present.
            //
            HWREGBITW(&g_ui32Flags, FLAGS_DEVICE_READY) = 0;
            HWREGBITW(&g_ui32Flags, FLAGS_DEVICE_CONNECT) = 0;

            //
            // Stop streaming audio.
            //
            StopAudio();

            //
            // Change the text to reflect the change.
            //
            CanvasTextSet(&g_sStatusText, "No Device");
            WidgetPaint((tWidget *)&g_sStatusText);

            break;
        }
        case SOUND_EVENT_UNKNOWN_DEV:
        {
            if(ui32Param == 1)
            {
                //
                // Unknown device connected.
                //
                CanvasTextSet(&g_sStatusText, "Unknown Device");
                WidgetPaint((tWidget *)&g_sStatusText);
            }
            else
            {
                //
                // Unknown device disconnected.
                //
                CanvasTextSet(&g_sStatusText, "No Device");
                WidgetPaint((tWidget *)&g_sStatusText);
            }

            break;
        }
        default:
        {
            break;
        }
    }
}

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
// The program main function.  It performs initialization, then handles wav
// file playback.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32Temp, ui32PLLRate;

    //
    // Set the system clock to run at 120MHz from the PLL.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                            SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                            SYSCTL_CFG_VCO_480), 120000000);

    //
    // Set the device pinout appropriately for this board.
    //
    PinoutSet();
    //
    // Save the PLL rate used by this application.
    //
    ui32PLLRate = 480000000;

    //
    // Set the interrupt priorities to give USB and timer higher priority
    // than the ADC.  While playing the touch screen should have lower priority
    // to reduce audio drop out.
    //
    ROM_IntPriorityGroupingSet(4);
    ROM_IntPrioritySet(INT_USB0, AUDIO_INT_PRIORITY);
    ROM_IntPrioritySet(INT_TIMER5A, AUDIO_INT_PRIORITY);
    ROM_IntPrioritySet(INT_ADC0SS3, ADC3_INT_PRIORITY);

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

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
    FrameDraw(&g_sContext, "usb-host-audio-in");

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(g_ui32SysClock);

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Issue the initial paint request to the widgets then immediately call
    // the widget manager to process the paint message.  This ensures that the
    // display is drawn as quickly as possible and saves the delay we would
    // otherwise experience if we processed the paint message after mounting
    // and reading the SD card.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sStatusPanel);
    WidgetPaint(WIDGET_ROOT);
    WidgetMessageQueueProcess();

    //
    // Not playing anything right now.
    //
    g_ui32Flags = 0;

    //
    // Tell the USB library the CPU clock and the PLL frequency.  This is a
    // new requirement for TM4C129 devices.
    //
    USBHCDFeatureSet(0, USBLIB_FEATURE_CPUCLK, &g_ui32SysClock);
    USBHCDFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

    //
    // Configure the USB host audio.
    //
    USBSoundInit(0, AudioEvent);

    //
    // Initialize audio streaming to stopped state.
    //
    StopAudio();
    SoundInit(g_ui32SysClock);
    SoundVolumeSet(255);

    while(1)
    {
        //
        // On connect change the device state to ready.
        //
        if(HWREGBITW(&g_ui32Flags, FLAGS_DEVICE_CONNECT))
        {
            HWREGBITW(&g_ui32Flags, FLAGS_DEVICE_CONNECT) = 0;

            //
            // Attempt to set the audio format to 48000 16 bit stereo.
            //
            if(USBSoundInputFormatSet(48000, 16, 2) == 0)
            {
                ui32Temp = 48000;
            }
            else
            {
                ui32Temp = 0;
            }

            //
            // Try to set the output format to match the input and fail if it
            // cannot be set.
            //
            if((ui32Temp != 0) &&
               (USBSoundOutputFormatSet(ui32Temp, 16, 2) != 0))
            {
                ui32Temp = 0;
            }

            //
            // If the audio device was support put the sample rate in the
            // status line.
            //
            if(ui32Temp != 0)
            {
                //
                // Print the time string in the format mm.ss/mm.ss
                //
                usprintf(g_pcStatusText, "Ready  %dHz 16 bit Stereo",
                         ui32Temp);

                //
                // USB device is ready for operation.
                //
                HWREGBITW(&g_ui32Flags, FLAGS_DEVICE_READY) = 1;
            }
            else
            {
                strcpy(g_pcStatusText, "Unsupported Audio Device");
            }

            //
            // Update the status line.
            //
            CanvasTextSet(&g_sStatusText, g_pcStatusText);
            WidgetPaint((tWidget *)&g_sStatusText);
        }

        //
        // Allow the USB non-interrupt code to run.
        //
        USBSoundMain();

        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();
    }
}
