//*****************************************************************************
//
// scribble.c - A simple scribble pad to demonstrate the touch screen driver.
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
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "utils/ringbuf.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Scribble Pad (scribble)</h1>
//!
//! The scribble pad provides a drawing area on the screen.  Touching the
//! screen will draw onto the drawing area using a selection of fundamental
//! colors (in other words, the seven colors produced by the three color
//! channels being either fully on or fully off).  Each time the screen is
//! touched to start a new drawing, the drawing area is erased and the next
//! color is selected.
//
//*****************************************************************************

//*****************************************************************************
//
// A structure used to pass touchscreen messages from the interrupt-context
// handler function to the main loop for processing.
//
//*****************************************************************************
typedef struct
{
    uint32_t ui32Msg;
    int32_t i32X;
    int32_t i32Y;
}
tScribbleMessage;

//*****************************************************************************
//
// The number of messages we can store in the message queue.
//
//*****************************************************************************
#define MSG_QUEUE_SIZE          16

//*****************************************************************************
//
// The ring buffer memory and control structure we use to implement the
// message queue.
//
//*****************************************************************************
static tScribbleMessage g_psMsgQueueBuffer[MSG_QUEUE_SIZE];
static tRingBufObject g_sMsgQueue;

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
// The colors that are used to draw on the screen.
//
//*****************************************************************************
static const uint32_t g_pui32Colors[] =
{
    ClrWhite,
    ClrYellow,
    ClrMagenta,
    ClrRed,
    ClrCyan,
    ClrLime,
    ClrBlue
};

//*****************************************************************************
//
// The index to the current color in use.
//
//*****************************************************************************
static uint32_t g_ui32ColorIdx;

//*****************************************************************************
//
// The previous pen position returned from the touch screen driver.
//
//*****************************************************************************
static int32_t g_i32X, g_i32Y;

//*****************************************************************************
//
// The drawing context used to draw to the screen.
//
//*****************************************************************************
static tContext g_sContext;

//*****************************************************************************
//
// The interrupt-context handler for touch screen events from the touch screen
// driver.  This function merely bundles up the event parameters and posts
// them to a message queue.  In the context of the main loop, they will be
// read from the queue and handled using TSMainHandler().
//
//*****************************************************************************
int32_t
TSHandler(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    tScribbleMessage sMsg;

    //
    // Build the message that we will write to the queue.
    //
    sMsg.ui32Msg = ui32Message;
    sMsg.i32X = i32X;
    sMsg.i32Y = i32Y;

    //
    // Make sure the queue isn't full. If it is, we just ignore this message.
    //
    if(!RingBufFull(&g_sMsgQueue))
    {
        RingBufWrite(&g_sMsgQueue, (uint8_t *)&sMsg, sizeof(tScribbleMessage));
    }

    //
    // Tell the touch handler that everything is fine.
    //
    return(1);
}

//*****************************************************************************
//
// The main loop handler for touch screen events from the touch screen driver.
//
//*****************************************************************************
int32_t
TSMainHandler(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    tRectangle sRect;

    //
    // See which event is being sent from the touch screen driver.
    //
    switch(ui32Message)
    {
        //
        // The pen has just been placed down.
        //
        case WIDGET_MSG_PTR_DOWN:
        {
            //
            // Erase the drawing area.
            //
            GrContextForegroundSet(&g_sContext, ClrBlack);
            sRect.i16XMin = 0;
            sRect.i16YMin = 0;
            sRect.i16XMax = 319;
            sRect.i16YMax = 239;
            GrRectFill(&g_sContext, &sRect);

            //
            // Flush any cached drawing operations.
            //
            GrFlush(&g_sContext);

            //
            // Set the drawing color to the current pen color.
            //
            GrContextForegroundSet(&g_sContext, g_pui32Colors[g_ui32ColorIdx]);

            //
            // Save the current position.
            //
            g_i32X = i32X;
            g_i32Y = i32Y;

            //
            // This event has been handled.
            //
            break;
        }

        //
        // Then pen has moved.
        //
        case WIDGET_MSG_PTR_MOVE:
        {
            //
            // Draw a line from the previous position to the current position.
            //
            GrLineDraw(&g_sContext, g_i32X, g_i32Y, i32X, i32Y);

            //
            // Flush any cached drawing operations.
            //
            GrFlush(&g_sContext);

            //
            // Save the current position.
            //
            g_i32X = i32X;
            g_i32Y = i32Y;

            //
            // This event has been handled.
            //
            break;
        }

        //
        // The pen has just been picked up.
        //
        case WIDGET_MSG_PTR_UP:
        {
            //
            // Draw a line from the previous position to the current position.
            //
            GrLineDraw(&g_sContext, g_i32X, g_i32Y, i32X, i32Y);

            //
            // Flush any cached drawing operations.
            //
            GrFlush(&g_sContext);

            //
            // Increment to the next drawing color.
            //
            g_ui32ColorIdx++;
            if(g_ui32ColorIdx ==
               (sizeof(g_pui32Colors) / sizeof(g_pui32Colors[0])))
            {
                g_ui32ColorIdx = 0;
            }

            //
            // This event has been handled.
            //
            break;
        }
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
// This function is called in the context of the main loop to process any
// touch screen messages that have been sent.  Messages are posted to a
// queue from the message handler and pulled off here.  This is required
// since it is not safe to have two different execution contexts performing
// graphics operations using the same graphics context.
//
//*****************************************************************************
void
ProcessTouchMessages(void)
{
    tScribbleMessage sMsg;

    //
    // Loop while there are more messages to process.
    //
    while(!RingBufEmpty(&g_sMsgQueue))
    {
        //
        // Get the next message.
        //
        RingBufRead(&g_sMsgQueue, (uint8_t *)&sMsg, sizeof(tScribbleMessage));

        //
        // Dispatch it to the handler.
        //
        TSMainHandler(sMsg.ui32Msg, sMsg.i32X, sMsg.i32Y);
    }
}

//*****************************************************************************
//
// Provides a scribble pad using the display on the Intelligent Display Module.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32SysClock;

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
    FrameDraw(&g_sContext, "scribble");

    //
    // Print the instructions across the top of the screen in white with a 20
    // point san-serif font.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrContextFontSet(&g_sContext, g_psFontCmss20);
    GrStringDrawCentered(&g_sContext, "Touch the screen to draw", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2,
                         ((GrContextDpyHeightGet(&g_sContext) - 32) / 2) + 14,
                         0);

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&g_sContext);

    //
    // Set the color index to zero.
    //
    g_ui32ColorIdx = 0;

    //
    // Initialize the message queue we use to pass messages from the touch
    // interrupt handler context to the main loop for processing.
    //
    RingBufInit(&g_sMsgQueue, (uint8_t *)g_psMsgQueueBuffer,
                (MSG_QUEUE_SIZE * sizeof(tScribbleMessage)));

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit(ui32SysClock);

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(TSHandler);

    //
    // Loop forever.  All the drawing is done in the touch screen event
    // handler.
    //
    while(1)
    {
        //
        // Process any new touchscreen messages.
        //
        ProcessTouchMessages();
    }
}
