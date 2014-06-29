//*****************************************************************************
//
// display_task.c - Task to display text and images on the LCD.
//
// Copyright (c) 2009-2014 Texas Instruments Incorporated.  All rights reserved.
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

#include <stdint.h>
#include <stdbool.h>
#include "grlib/grlib.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "display_task.h"
#include "idle_task.h"
#include "priorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
//*****************************************************************************
//
// This structure defines the messages that are sent to the display task.
//
//*****************************************************************************
typedef struct
{
    uint32_t ui32Type;
    uint16_t ui16X;
    uint16_t ui16Y;
    const char *pcMessage;
}

tDisplayMessage;
xTaskHandle g_sDisplayTask; // FIXME
//*****************************************************************************
//
// The stack size for the display task.
//
//*****************************************************************************
#define STACKSIZE_DISPLAYTASK   128

//*****************************************************************************
//
// Defines to indicate the contents of the display message.
//
//*****************************************************************************
#define DISPLAY_IMAGE           1
#define DISPLAY_STRING          2
#define DISPLAY_MOVE            3
#define DISPLAY_DRAW            4

//*****************************************************************************
//
// The item size, queue size, and memory size for the display message queue.
//
//*****************************************************************************
#define DISPLAY_ITEM_SIZE       sizeof(tDisplayMessage)
#define DISPLAY_QUEUE_SIZE      10

//*****************************************************************************
//
// The queue that holds messages sent to the display task.
//
//*****************************************************************************
static xQueueHandle g_pDisplayQueue;

//*****************************************************************************
//
// The most recent position of the display pen.
//
//*****************************************************************************
static uint32_t g_ui32DisplayX, g_ui32DisplayY;

//*****************************************************************************
//
// This task receives messages from the other tasks and updates the display as
// directed.
//
//*****************************************************************************
static void
DisplayTask(void *pvParameters)
{
    tDisplayMessage sMessage;
    tContext sContext;

    //
    // Initialize the graphics library context.
    //
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);
    GrContextForegroundSet(&sContext, ClrWhite);
    GrContextBackgroundSet(&sContext, ClrBlack);
    GrContextFontSet(&sContext, g_psFontFixed6x8);

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Read the next message from the queue.
        //
        if(xQueueReceive(g_pDisplayQueue, &sMessage, portMAX_DELAY) == pdPASS)
        {
            //
            // Determine the message type.
            //
            switch(sMessage.ui32Type)
            {
                //
                // The drawing of an image has been requested.
                //
                case DISPLAY_IMAGE:
                {
                    //
                    // Draw this image on the display.
                    //
                    GrImageDraw(&sContext, (uint8_t *)sMessage.pcMessage,
                                sMessage.ui16X, sMessage.ui16Y);

                    //
                    // This message has been handled.
                    //
                    break;
                }

                //
                // The drawing of a string has been requested.
                //
                case DISPLAY_STRING:
                {
                    //
                    // Draw this string on the display.
                    //
                    GrStringDraw(&sContext, sMessage.pcMessage, -1,
                                 sMessage.ui16X, sMessage.ui16Y, 1);

                    //
                    // This message has been handled.
                    //
                    break;
                }

                //
                // A move of the pen has been requested.
                //
                case DISPLAY_MOVE:
                {
                    //
                    // Save the new pen position.
                    //
                    g_ui32DisplayX = sMessage.ui16X;
                    g_ui32DisplayY = sMessage.ui16Y;

                    //
                    // This message has been handled.
                    //
                    break;
                }

                //
                // A draw with the pen has been requested.
                //
                case DISPLAY_DRAW:
                {
                    //
                    // Draw a line from the previous pen position to the new
                    // pen position.
                    //
                    GrLineDraw(&sContext, g_ui32DisplayX, g_ui32DisplayY,
                               sMessage.ui16X, sMessage.ui16Y);

                    //
                    // Save the new pen position.
                    //
                    g_ui32DisplayX = sMessage.ui16X;
                    g_ui32DisplayY = sMessage.ui16Y;

                    //
                    // This message has been handled.
                    //
                    break;
                }
            }
        }
    }
}

//*****************************************************************************
//
// Sends a request to the display task to draw an image on the display.
//
//*****************************************************************************
void
DisplayImage(uint32_t ui16X, uint32_t ui16Y, const uint8_t *pui8Image)
{
    tDisplayMessage sMessage;

    //
    // Construct the message to be sent.
    //
    sMessage.ui32Type = DISPLAY_IMAGE;
    sMessage.ui16X = ui16X;
    sMessage.ui16Y = ui16Y;
    sMessage.pcMessage = (char *)pui8Image;

    //
    // Send the image draw request to the display task.
    //
    xQueueSend(g_pDisplayQueue, &sMessage, portMAX_DELAY);
}

//*****************************************************************************
//
// Sends a request to the display task to draw a string on the display.
//
//*****************************************************************************
void
DisplayString(uint32_t ui16X, uint32_t ui16Y, const char *pcString)
{
    tDisplayMessage sMessage;

    //
    // Construct the message to be sent.
    //
    sMessage.ui32Type = DISPLAY_STRING;
    sMessage.ui16X = ui16X;
    sMessage.ui16Y = ui16Y;
    sMessage.pcMessage = pcString;

    //
    // Send the string draw request to the display task.
    //
    xQueueSend(g_pDisplayQueue, &sMessage, portMAX_DELAY);
}

//*****************************************************************************
//
// Sends a request to the display task to move the pen.
//
//*****************************************************************************
void
DisplayMove(uint32_t ui16X, uint32_t ui16Y)
{
    tDisplayMessage sMessage;

    //
    // Construct the message to be sent.
    //
    sMessage.ui32Type = DISPLAY_MOVE;
    sMessage.ui16X = ui16X;
    sMessage.ui16Y = ui16Y;

    //
    // Send the pen move request to the display task.
    //
    xQueueSend(g_pDisplayQueue, &sMessage, portMAX_DELAY);
}

//*****************************************************************************
//
// Sends a request to the display task to draw with the pen.
//
//*****************************************************************************
void
DisplayDraw(uint32_t ui16X, uint32_t ui16Y)
{
    tDisplayMessage sMessage;

    //
    // Construct the message to be sent.
    //
    sMessage.ui32Type = DISPLAY_DRAW;
    sMessage.ui16X = ui16X;
    sMessage.ui16Y = ui16Y;

    //
    // Send the pen draw request to the display task.
    //
    xQueueSend(g_pDisplayQueue, &sMessage, portMAX_DELAY);
}

//*****************************************************************************
//
// Initializes the display task.
//
//*****************************************************************************
uint32_t
DisplayTaskInit(void)
{
    //
    // Create a queue for sending messages to the display task.
    //
    g_pDisplayQueue = xQueueCreate(DISPLAY_QUEUE_SIZE, DISPLAY_ITEM_SIZE);
    if(g_pDisplayQueue == NULL)
    {
        return(1);
    }

    //
    // Create the display task.
    //
    if(xTaskCreate(DisplayTask, (signed portCHAR *)"Display",
                   STACKSIZE_DISPLAYTASK, NULL,
                   tskIDLE_PRIORITY + PRIORITY_DISPLAY_TASK, &g_sDisplayTask) !=  pdTRUE)
    { // FIXME
        return(1);
    }

    //
    // Success.
    //
    return(0);
}
