//*****************************************************************************
//
// images.c - The image sets for the various weather conditions.
//
// Copyright (c) 2014 Texas Instruments Incorporated.  All rights reserved.
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

//*****************************************************************************
//
// Sun Image
//
//*****************************************************************************
const uint8_t g_pui8SunImage;

//*****************************************************************************
//
// Cloudy Image
//
//*****************************************************************************
const uint8_t g_pui8CloudyImage;

//*****************************************************************************
//
// Rain Image
//
//*****************************************************************************
const uint8_t g_pui8RainImage;

//*****************************************************************************
//
// Thunderstorm Image
//
//*****************************************************************************
const uint8_t g_pui8ThuderStormImage;

//*****************************************************************************
//
// Fog/Mist Image
//
//*****************************************************************************
const uint8_t g_pui8FogImage;

//*****************************************************************************
//
// Snow Image
//
//*****************************************************************************
const uint8_t g_pui8SnowImage;

//*****************************************************************************
//
// Moon Image
//
//*****************************************************************************
const uint8_t g_pui8MoonImage;

//*****************************************************************************
//
// Right Arrow Image
//
//*****************************************************************************
const uint8_t g_pui8RightImage;

//*****************************************************************************
//
// Left Arrow Image
//
//*****************************************************************************
const uint8_t g_pui8LeftImage;

//*****************************************************************************
//
// Up Tab Image
//
//*****************************************************************************
const uint8_t g_pui8UpTabImage;

//*****************************************************************************
//
// Down Tab Image
//
//*****************************************************************************
const uint8_t g_pui8DownTabImage;

//*****************************************************************************
//
// Texas Instrument Logo Image
//
//*****************************************************************************
uint8_t g_pui8TIImagePalette[16 * 3];
uint8_t g_pui8TIImage;
