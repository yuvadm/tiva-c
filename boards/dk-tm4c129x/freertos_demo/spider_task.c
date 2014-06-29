//*****************************************************************************
//
// spider_task.c - Tasks to animate a set of spiders on the LCD, one task per
//                 spider.
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
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "drivers/touch.h"
#include "display_task.h"
#include "idle_task.h"
#include "images.h"
#include "priorities.h"
#include "random.h"
#include "freertos_demo.h"
#include "spider_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
//*****************************************************************************
//
// The stack size for the spider control task.
//
//*****************************************************************************
# define STACKSIZE_CONTROLTASK  128

//*****************************************************************************
//
// The stack sizes for the spider tasks.
//
//*****************************************************************************
# define STACKSIZE_SPIDERTASK   128

//*****************************************************************************
//
// The following define the screen area in which the spiders are allowed to
// roam
//
//*****************************************************************************
#define AREA_X                  8
#define AREA_Y                  24
#define AREA_WIDTH              303
#define AREA_HEIGHT             (231 - 24 - 20)

//*****************************************************************************
//
// The size of the spider images.
//
//*****************************************************************************
#define SPIDER_WIDTH            24
#define SPIDER_HEIGHT           24

//*****************************************************************************
//
// The following define the extents of the centroid of the spiders.
//
//*****************************************************************************
#define SPIDER_MIN_X            (AREA_X + (SPIDER_WIDTH / 2))
#define SPIDER_MAX_X            (AREA_X + AREA_WIDTH - (SPIDER_WIDTH / 2))
#define SPIDER_MIN_Y            (AREA_Y + (SPIDER_HEIGHT / 2))
#define SPIDER_MAX_Y            (AREA_Y + AREA_HEIGHT - (SPIDER_HEIGHT / 2))

//*****************************************************************************
//
// The item size, queue size, and memory size for the spider control message
// queue.
//
//*****************************************************************************
#define CONTROL_ITEM_SIZE       sizeof(unsigned long)
#define CONTROL_QUEUE_SIZE      10

//*****************************************************************************
//
// The queue that holds messages sent to the spider control task.
//
//*****************************************************************************
static xQueueHandle g_pControlQueue;

//*****************************************************************************
//
// The amount the spider moves horizontally for each direction of movement.
//
// For this and all subsequent arrays that are indexed by direction of
// movement, the indices are as follows:
//
//     0 => right
//     1 => right and down
//     2 => down
//     3 => left and down
//     4 => left
//     5 => left and up
//     6 => up
//     7 => right and up
//
//*****************************************************************************
static const int32_t g_pi32SpiderStepX[8] =
{
    1, 1, 0, -1, -1, -1, 0, 1
};

//*****************************************************************************
//
// The amount the spider moves vertically for each direction of movement.
//
//*****************************************************************************
static const int32_t g_pi32SpiderStepY[8] =
{
    0, 1, 1, 1, 0, -1, -1, -1
};

//*****************************************************************************
//
// The animation images for the spider, two per direction of movement.  In
// other words, entries 0 and 1 correspond to direction 0 (right), entries 2
// and 3 correspond to direction 1 (right and down), etc.
//
//*****************************************************************************
static const uint8_t *g_ppui8SpiderImage[16] =
{
    g_pui8SpiderR1Image,
    g_pui8SpiderR2Image,
    g_pui8SpiderDR1Image,
    g_pui8SpiderDR2Image,
    g_pui8SpiderD1Image,
    g_pui8SpiderD2Image,
    g_pui8SpiderDL1Image,
    g_pui8SpiderDL2Image,
    g_pui8SpiderL1Image,
    g_pui8SpiderL2Image,
    g_pui8SpiderUL1Image,
    g_pui8SpiderUL2Image,
    g_pui8SpiderU1Image,
    g_pui8SpiderU2Image,
    g_pui8SpiderUR1Image,
    g_pui8SpiderUR2Image
};

//*****************************************************************************
//
// The number of ticks to delay a spider task based on the direction of
// movement.  This array has only two entries; the first corresponding to
// horizontal and vertical movement, and the second corresponding to diagonal
// movement.  By having the second entry be 1.4 times the first, the spiders
// are updated slower when moving along the diagonal to compensate for the
// fact that each step is further (since it is moving one step in both the
// horizontal and vertical).
//
//*****************************************************************************
uint32_t g_pui32SpiderDelay[2];

//*****************************************************************************
//
// The horizontal position of the spiders.
//
//*****************************************************************************
static int32_t g_pi32SpiderX[MAX_SPIDERS];

//*****************************************************************************
//
// The vertical position of the spiders.
//
//*****************************************************************************
static int32_t g_pi32SpiderY[MAX_SPIDERS];

//*****************************************************************************
//
// A bitmap that indicates which spiders are alive (which corresponds to a
// running task for that spider).
//
//*****************************************************************************
static uint32_t g_ui32SpiderAlive;

//*****************************************************************************
//
// A bitmap that indicates which spiders have been killed (by touching them).
//
//*****************************************************************************
static uint32_t g_ui32SpiderDead;
xTaskHandle g_sSpiderTask; // FIXME
//*****************************************************************************
//
// Determines if a given point collides with one of the spiders.  The spider
// specified is ignored when doing collision detection in order to prevent a
// false collision with itself (when checking to see if it is safe to move the
// spider).
//
//*****************************************************************************
static int32_t
SpiderCollide(int32_t i32Spider, int32_t i32X, int32_t i32Y)
{
    int32_t i32Idx, i32DX, i32DY;

    //
    // Loop through all the spiders.
    //
    for(i32Idx = 0; i32Idx < MAX_SPIDERS; i32Idx++)
    {
        //
        // Skip this spider if it is not alive or is the spider that should be
        // ignored.
        //
        if((HWREGBITW(&g_ui32SpiderAlive, i32Idx) == 0) ||
           (i32Idx == i32Spider))
        {
            continue;
        }

        //
        // Compute the horizontal and vertical difference between this spider's
        // position and the point in question.
        //
        i32DX = ((g_pi32SpiderX[i32Idx] > i32X) ?
                 (g_pi32SpiderX[i32Idx] - i32X) :
                 (i32X - g_pi32SpiderX[i32Idx]));
        i32DY = ((g_pi32SpiderY[i32Idx] > i32Y) ?
                 (g_pi32SpiderY[i32Idx] - i32Y) :
                 (i32Y - g_pi32SpiderY[i32Idx]));

        //
        // Return this spider index if the point in question collides with it.
        //
        if((i32DX < SPIDER_WIDTH) && (i32DY < SPIDER_HEIGHT))
        {
            return(i32Idx);
        }
    }

    //
    // No collision was detected.
    //
    return(-1);
}

//*****************************************************************************
//
// This task manages the scurrying about of a spider.
//
//*****************************************************************************
static void
SpiderTask(void *pvParameters)
{
    uint32_t ui32Dir, ui32Image, ui32Temp;
    int32_t i32X, i32Y, i32Spider;

    //
    // Get the spider number from the parameter.
    //
    i32Spider = (long)pvParameters;

    //
    // Add the current tick count to the random entropy pool.
    //
    RandomAddEntropy(xTaskGetTickCount());

    //
    // Reseed the random number generator.
    //
    RandomSeed();

    //
    // Indicate that this spider is alive.
    //
    HWREGBITW(&g_ui32SpiderAlive, i32Spider) = 1;

    //
    // Indicate that this spider is not dead yet.
    //
    HWREGBITW(&g_ui32SpiderDead, i32Spider) = 0;

    //
    // Get a local copy of the spider's starting position.
    //
    i32X = g_pi32SpiderX[i32Spider];
    i32Y = g_pi32SpiderY[i32Spider];

    //
    // Choose a random starting direction for the spider.
    //
    ui32Dir = RandomNumber() >> 29;

    //
    // Start by displaying the first of the two spider animation images.
    //
    ui32Image = 0;

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // See if this spider has been killed.
        //
        if(HWREGBITW(&g_ui32SpiderDead, i32Spider) == 1)
        {
            //
            // Wait for 2 seconds.
            //
            vTaskDelay((1000 / portTICK_RATE_MS) * 2);

            //
            // Clear the spider from the display.
            //
            DisplayImage(i32X - (SPIDER_WIDTH / 2), i32Y - (SPIDER_HEIGHT / 2),
                         g_pui8SpiderBlankImage);

            //
            // Indicate that this spider is not alive.
            //
            HWREGBITW(&g_ui32SpiderAlive, i32Spider) = 0;

            //
            // Delete the current task.  This should never return.
            //
            vTaskDelete(NULL);

            //
            // In case it does return, loop forever.
            //
            while(1)
            {
            }
        }

        //
        // Enter a critical section while the next move for the spider is
        // determined.  Having more than one spider trying to move at a time
        // (via preemption) would make the collision detection check fail.
        //
        taskENTER_CRITICAL();

        //
        // Move the spider.
        //
        i32X += g_pi32SpiderStepX[ui32Dir];
        i32Y += g_pi32SpiderStepY[ui32Dir];

        //
        // See if the spider has cross the boundary of its area, if it has
        // collided with another spider, or if random chance says that the
        // spider should turn despite not having collided with anything.
        //
        if((i32X < SPIDER_MIN_X) || (i32X > SPIDER_MAX_X) ||
           (i32Y < SPIDER_MIN_Y) || (i32Y > SPIDER_MAX_Y) ||
           (SpiderCollide(i32Spider, i32X, i32Y) != -1) ||
           (RandomNumber() < 0x08000000))
        {
            //
            // Undo the previous movement of the spider.
            //
            i32X -= g_pi32SpiderStepX[ui32Dir];
            i32Y -= g_pi32SpiderStepY[ui32Dir];

            //
            // Get a random number to determine the turn to be made.
            //
            ui32Temp = RandomNumber();

            //
            // Determine how to turn the spider based on the random number.
            // Half the time the spider turns to the left and half the time it
            // turns to the right.  Of each half, it turns a quarter of a turn
            // 12.5% of the time and an eighth of a turn 87.5% of the time.
            //
            if(ui32Temp < 0x10000000)
            {
                ui32Dir = (ui32Dir + 2) & 7;
            }
            else if(ui32Temp < 0x80000000)
            {
                ui32Dir = (ui32Dir + 1) & 7;
            }
            else if(ui32Temp < 0xf0000000)
            {
                ui32Dir = (ui32Dir - 1) & 7;
            }
            else
            {
                ui32Dir = (ui32Dir - 2) & 7;
            }
        }

        //
        // Update the position of the spider.
        //
        g_pi32SpiderX[i32Spider] = i32X;
        g_pi32SpiderY[i32Spider] = i32Y;

        //
        // Exit the critical section now that the spider has been moved.
        //
        taskEXIT_CRITICAL();

        //
        // Have the display task draw the spider at the new position.  Since
        // there is a one pixel empty border around all the images, and the
        // position of the spider is incremented by only one pixel, this also
        // erases any traces of the spider in its previous position.
        //
        DisplayImage(i32X - (SPIDER_WIDTH / 2), i32Y - (SPIDER_HEIGHT / 2),
                     g_ppui8SpiderImage[(ui32Dir * 2) + ui32Image]);

        //
        // Toggle the spider animation index.
        //
        ui32Image ^= 1;

        //
        // Delay this task for an amount of time based on the direction the
        // spider is moving.
        //
        vTaskDelay(g_pui32SpiderDelay[ui32Dir & 1]);

        //
        // Add the new tick count to the random entropy pool.
        //
        RandomAddEntropy(xTaskGetTickCount());

        //
        // Reseed the random number generator.
        //
        RandomSeed();
    }
}

//*****************************************************************************
//
// Creates a spider task.
//
//*****************************************************************************
static uint32_t
CreateSpider(int32_t i32X, int32_t i32Y)
{
    uint32_t ui32Spider;

    //
    // Search to see if there is a spider task available.
    //
    for(ui32Spider = 0; ui32Spider < MAX_SPIDERS; ui32Spider++)
    {
        if(HWREGBITW(&g_ui32SpiderAlive, ui32Spider) == 0)
        {
            break;
        }
    }

    //
    // Return a failure if no spider tasks are available (in other words, the
    // maximum number of spiders are already alive).
    //
    if(ui32Spider == MAX_SPIDERS)
    {
        return(1);
    }

    //
    // Adjust the starting horizontal position to make sure it is inside the
    // allowable area for the spiders.
    //
    if(i32X < SPIDER_MIN_X)
    {
        i32X = SPIDER_MIN_X;
    }
    else if(i32X > SPIDER_MAX_X)
    {
        i32X = SPIDER_MAX_X;
    }

    //
    // Adjust the starting vertical position to make sure it is inside the
    // allowable area for the spiders.
    //
    if(i32Y < SPIDER_MIN_Y)
    {
        i32Y = SPIDER_MIN_Y;
    }
    else if(i32Y > SPIDER_MAX_Y)
    {
        i32Y = SPIDER_MAX_Y;
    }

    //
    // Save the starting position for this spider.
    //
    g_pi32SpiderX[ui32Spider] = i32X;
    g_pi32SpiderY[ui32Spider] = i32Y;

    //
    // Create a task to animate this spider.
    //
    if(xTaskCreate(SpiderTask, (signed portCHAR *)"Spider",
                   STACKSIZE_SPIDERTASK, (void *)ui32Spider,
                   tskIDLE_PRIORITY + PRIORITY_SPIDER_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    //
    // Success.
    //
    return(0);
}

//*****************************************************************************
//
// The callback function for messages from the touch screen driver.
//
//*****************************************************************************
static int32_t
ControlTouchCallback(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    portBASE_TYPE bTaskWaken;

    //
    // Ignore all messages other than pointer down messages.
    //
    if(ui32Message != WIDGET_MSG_PTR_DOWN)
    {
        return(0);
    }

    //
    // Pack the position into a message to send to the spider control task.
    //
    ui32Message = ((i32X & 65535) << 16) | (i32Y & 65535);

    //
    // Send the position message to the spider control task.
    //
    xQueueSendFromISR(g_pControlQueue, &ui32Message, &bTaskWaken);

    //
    // Perform a task yield if necessary.
    //
#if defined(__Check_Later)
    taskYIELD_FROM_ISR(bTaskWaken);
#endif

    //
    // This message has been handled.
    //
    return(0);
}

//*****************************************************************************
//
// Determines if a given touch screen point collides with one of the spiders.
//
//*****************************************************************************
static int32_t
SpiderTouchCollide(int32_t i32X, int32_t i32Y)
{
    int32_t i32Idx, i32DX, i32DY, i32Best, i32Dist;

    //
    // Until a collision is found, there is no best spider choice.
    //
    i32Best = -1;
    i32Dist = 1000000;

    //
    // Loop through all the spiders.
    //
    for(i32Idx = 0; i32Idx < MAX_SPIDERS; i32Idx++)
    {
        //
        // Skip this spider if it is not alive.
        //
        if((HWREGBITW(&g_ui32SpiderAlive, i32Idx) == 0) ||
           (HWREGBITW(&g_ui32SpiderDead, i32Idx) == 1))
        {
            continue;
        }

        //
        // Compute the horizontal and vertical difference between this spider's
        // position and the point in question.
        //
        i32DX = ((g_pi32SpiderX[i32Idx] > i32X) ?
                 (g_pi32SpiderX[i32Idx] - i32X) :
                 (i32X - g_pi32SpiderX[i32Idx]));
        i32DY = ((g_pi32SpiderY[i32Idx] > i32Y) ?
                 (g_pi32SpiderY[i32Idx] - i32Y) :
                 (i32Y - g_pi32SpiderY[i32Idx]));

        //
        // See if the point in question collides with this spider.
        //
        if((i32DX < (SPIDER_WIDTH + 4)) && (i32DY < (SPIDER_HEIGHT + 4)))
        {
            //
            // Compute distance (squared) between this point and the spider.
            //
            i32DX = (i32DX * i32DX) + (i32DY * i32DY);

            //
            // See if this spider is closer to the point in question than any
            // other spider encountered.
            //
            if(i32DX < i32Dist)
            {
                //
                // Save this spider as the new best choice.
                //
                i32Best = i32Idx;
                i32Dist = i32DX;
            }
        }
    }

    //
    // Return the best choice, if one was found.
    //
    if(i32Best != -1)
    {
        return(i32Best);
    }

    //
    // Loop through all the spiders.  This time, the spiders that are dead but
    // not cleared from the screen are not ignored.
    //
    for(i32Idx = 0; i32Idx < MAX_SPIDERS; i32Idx++)
    {
        //
        // Skip this spider if it is not alive.
        //
        if(HWREGBITW(&g_ui32SpiderAlive, i32Idx) == 0)
        {
            continue;
        }

        //
        // Compute the horizontal and vertical difference between this spider's
        // position and the point in question.
        //
        i32DX = ((g_pi32SpiderX[i32Idx] > i32X) ?
                 (g_pi32SpiderX[i32Idx] - i32X) :
                 (i32X - g_pi32SpiderX[i32Idx]));
        i32DY = ((g_pi32SpiderY[i32Idx] > i32Y) ?
                 (g_pi32SpiderY[i32Idx] - i32Y) :
                 (i32Y - g_pi32SpiderY[i32Idx]));

        //
        // See if the point in question collides with this spider.
        //
        if((i32DX < (SPIDER_WIDTH + 4)) && (i32DY < (SPIDER_HEIGHT + 4)))
        {
            //
            // Compute distance (squared) between this point and the spider.
            //
            i32DX = (i32DX * i32DX) + (i32DY * i32DY);

            //
            // See if this spider is closer to the point in question than any
            // other spider encountered.
            //
            if(i32DX < i32Dist)
            {
                //
                // Save this spider as the new best choice.
                //
                i32Best = i32Idx;
                i32Dist = i32DX;
            }
        }
    }

    //
    // Return the best choice, if one was found.
    //
    return(i32Best);
}

//*****************************************************************************
//
// This task provides overall control of the spiders, spawning and killing them
// in response to presses on the touch screen.
//
//*****************************************************************************
static void
ControlTask(void *pvParameters)
{
    uint32_t ui32Message;
    int32_t i32X, i32Y, i32Spider;

    //
    // Initialize the touch screen driver and register a callback function.
    //
    TouchScreenInit(g_ui32SysClock);
    TouchScreenCallbackSet(ControlTouchCallback);

    //
    // Lower the priority of the touch screen interrupt handler.  This is
    // required so that the interrupt handler can safely call the interrupt-
    // safe FreeRTOS functions (specifically to send messages to the queue).
    //
    IntPrioritySet(INT_ADC0SS3, 0xc0);

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Read the next message from the queue.
        //
        if(xQueueReceive(g_pControlQueue, &ui32Message, portMAX_DELAY) ==
           pdPASS)
        {
            //
            // Extract the position of the screen touch from the message.
            //
            i32X = ui32Message >> 16;
            i32Y = ui32Message & 65535;

            //
            // Ignore this screen touch if it is not inside the spider area.
            //
            if((i32X >= AREA_X) && (i32X < (AREA_X + AREA_WIDTH)) &&
               (i32Y >= AREA_Y) && (i32Y < (AREA_Y + AREA_HEIGHT)))
            {
                //
                // See if this position collides with any of the spiders.
                //
                i32Spider = SpiderTouchCollide(i32X, i32Y);
                if(i32Spider == -1)
                {
                    //
                    // There is no collision, so create a new spider (if
                    // possible) at this position.
                    //
                    CreateSpider(i32X, i32Y);
                }
                else
                {
                    //
                    // There is a collision, so kill this spider.
                    //
                    HWREGBITW(&g_ui32SpiderDead, i32Spider) = 1;
                }
            }
        }
    }
}

//*****************************************************************************
//
// Sets the speed of the spiders by specifying the number of milliseconds
// between updates to the spider's position.
//
//*****************************************************************************
void
SpiderSpeedSet(uint32_t ui32Speed)
{
    //
    // Convert the update rate from milliseconds to ticks.  The second entry
    // of the array is 1.4 times the first so that updates when moving along
    // the diagonal, which are longer steps, are done less frequently by a
    // proportional amount.
    //
    g_pui32SpiderDelay[0] = (ui32Speed * (1000 / portTICK_RATE_MS)) / 1000;
    g_pui32SpiderDelay[1] = (ui32Speed * 14 * (1000 / portTICK_RATE_MS)) /
                            10000;
}

//*****************************************************************************
//
// Initializes the spider tasks.
//
//*****************************************************************************
uint32_t
SpiderTaskInit(void)
{
    uint32_t ui32Idx;

    //
    // Set the initial speed of the spiders.
    //
    SpiderSpeedSet(10);

    //
    // Create a queue for sending messages to the spider control task.
    //
    g_pControlQueue = xQueueCreate(CONTROL_QUEUE_SIZE, CONTROL_QUEUE_SIZE);
    if(g_pControlQueue == NULL)
    {
        return(1);
    }

    //
    // Create the spider control task.
    //
    if(xTaskCreate(ControlTask, (signed portCHAR *)"ControlTask",
                   STACKSIZE_CONTROLTASK, NULL,
                   tskIDLE_PRIORITY + PRIORITY_CONTROL_TASK, &g_sSpiderTask) != pdTRUE)
    {// FIXME
        return(1);
    }

    //
    // Create eight spiders initially.
    //
    for(ui32Idx = 0; ui32Idx < 8; ui32Idx++)
    {
        //
        // Create a spider that is centered vertically and equally spaced
        // horizontally across the display.
        //
        if(CreateSpider((ui32Idx * (AREA_WIDTH / 8)) + (AREA_WIDTH / 16),
                        (AREA_HEIGHT / 2) + AREA_Y) == 1)
        {
            return(1);
        }

        //
        // Provide an early indication that this spider is alive.  The task is
        // not running yet (since this function is called before the scheduler
        // has been started) so this variable is not set by the task (yet).
        // Manually setting it allows the remaining initial spiders to be
        // created properly.
        //
        HWREGBITW(&g_ui32SpiderAlive, ui32Idx) = 1;
    }

    //
    // Success.
    //
    return(0);
}
