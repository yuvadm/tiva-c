//*****************************************************************************
//
// led_task.c - A simple flashing LED task.
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

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "drivers/rgb.h"
#include "drivers/buttons.h"
#include "utils/uartstdio.h"
#include "led_task.h"
#include "priorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

//*****************************************************************************
//
// The stack size for the LED toggle task.
//
//*****************************************************************************
#define LEDTASKSTACKSIZE        128         // Stack size in words

//*****************************************************************************
//
// The item size and queue size for the LED message queue.
//
//*****************************************************************************
#define LED_ITEM_SIZE           sizeof(uint8_t)
#define LED_QUEUE_SIZE          5

//*****************************************************************************
//
// Default LED toggle delay value. LED toggling frequency is twice this number.
//
//*****************************************************************************
#define LED_TOGGLE_DELAY        250

//*****************************************************************************
//
// The queue that holds messages sent to the LED task.
//
//*****************************************************************************
xQueueHandle g_pLEDQueue;

//
// [G, R, B] range is 0 to 0xFFFF per color.
//
static uint32_t g_pui32Colors[3] = { 0x0000, 0x0000, 0x0000 };
static uint8_t g_ui8ColorsIndx;

extern xSemaphoreHandle g_pUARTSemaphore;

//*****************************************************************************
//
// This task toggles the user selected LED at a user selected frequency. User
// can make the selections by pressing the left and right buttons.
//
//*****************************************************************************
static void
LEDTask(void *pvParameters)
{
    portTickType ui32WakeTime;
    uint32_t ui32LEDToggleDelay;
    uint8_t i8Message;

    //
    // Initialize the LED Toggle Delay to default value.
    //
    ui32LEDToggleDelay = LED_TOGGLE_DELAY;

    //
    // Get the current tick count.
    //
    ui32WakeTime = xTaskGetTickCount();

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Read the next message, if available on queue.
        //
        if(xQueueReceive(g_pLEDQueue, &i8Message, 0) == pdPASS)
        {
            //
            // If left button, update to next LED.
            //
            if(i8Message == LEFT_BUTTON)
            {
                //
                // Update the LED buffer to turn off the currently working.
                //
                g_pui32Colors[g_ui8ColorsIndx] = 0x0000;

                //
                // Update the index to next LED
                g_ui8ColorsIndx++;
                if(g_ui8ColorsIndx > 2)
                {
                    g_ui8ColorsIndx = 0;
                }

                //
                // Update the LED buffer to turn on the newly selected LED.
                //
                g_pui32Colors[g_ui8ColorsIndx] = 0x8000;

                //
                // Configure the new LED settings.
                //
                RGBColorSet(g_pui32Colors);

                //
                // Guard UART from concurrent access. Print the currently
                // blinking LED.
                //
                xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
                UARTprintf("Led %d is blinking. [R, G, B]\n", g_ui8ColorsIndx);
                xSemaphoreGive(g_pUARTSemaphore);
            }

            //
            // If right button, update delay time between toggles of led.
            //
            if(i8Message == RIGHT_BUTTON)
            {
                ui32LEDToggleDelay *= 2;
                if(ui32LEDToggleDelay > 1000)
                {
                    ui32LEDToggleDelay = LED_TOGGLE_DELAY / 2;
                }

                //
                // Guard UART from concurrent access. Print the currently
                // blinking frequency.
                //
                xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
                UARTprintf("Led blinking frequency is %d ms.\n",
                           (ui32LEDToggleDelay * 2));
                xSemaphoreGive(g_pUARTSemaphore);
            }
        }

        //
        // Turn on the LED.
        //
        RGBEnable();

        //
        // Wait for the required amount of time.
        //
        vTaskDelayUntil(&ui32WakeTime, ui32LEDToggleDelay / portTICK_RATE_MS);

        //
        // Turn off the LED.
        //
        RGBDisable();

        //
        // Wait for the required amount of time.
        //
        vTaskDelayUntil(&ui32WakeTime, ui32LEDToggleDelay / portTICK_RATE_MS);
    }
}

//*****************************************************************************
//
// Initializes the LED task.
//
//*****************************************************************************
uint32_t
LEDTaskInit(void)
{
    //
    // Initialize the GPIOs and Timers that drive the three LEDs.
    //
    RGBInit(1);
    RGBIntensitySet(0.3f);

    //
    // Turn on the Green LED
    //
    g_ui8ColorsIndx = 0;
    g_pui32Colors[g_ui8ColorsIndx] = 0x8000;
    RGBColorSet(g_pui32Colors);

    //
    // Print the current loggling LED and frequency.
    //
    UARTprintf("\nLed %d is blinking. [R, G, B]\n", g_ui8ColorsIndx);
    UARTprintf("Led blinking frequency is %d ms.\n", (LED_TOGGLE_DELAY * 2));

    //
    // Create a queue for sending messages to the LED task.
    //
    g_pLEDQueue = xQueueCreate(LED_QUEUE_SIZE, LED_ITEM_SIZE);

    //
    // Create the LED task.
    //
    if(xTaskCreate(LEDTask, (signed portCHAR *)"LED", LEDTASKSTACKSIZE, NULL,
                   tskIDLE_PRIORITY + PRIORITY_LED_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    //
    // Success.
    //
    return(0);
}
