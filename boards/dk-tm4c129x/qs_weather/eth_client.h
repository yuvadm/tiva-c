//*****************************************************************************
//
// eth_client.h - Prototypes for the driver for the eth_client
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
#ifndef ETH_CLIENT_H_
#define ETH_CLIENT_H_

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Events passed back to the application.
//
//*****************************************************************************
#define ETH_EVENT_RECEIVE       0x00000001
#define ETH_EVENT_CONNECT       0x00000002
#define ETH_EVENT_DISCONNECT    0x00000003
#define ETH_EVENT_CLOSE         0x00000004
#define ETH_EVENT_INVALID_REQ   0x00000005

//*****************************************************************************
//
// The weather access methods.
//
//*****************************************************************************
typedef enum
{
    //
    // openweathermap.org.
    //
    iWSrcOpenWeatherMap
}
tWeatherSource;

//*****************************************************************************
//
// Generic weather reporting structure.
//
//*****************************************************************************
typedef struct
{
    //
    // The brief weather description, this is caller provided.
    //
    const char *pcDescription;

    //
    // The current temperature(units determined by caller).
    //
    int32_t i32Temp;

    //
    // The daily high temperature(units determined by caller).
    //
    int32_t i32TempHigh;

    //
    // The daily low temperature(units determined by caller).
    //
    int32_t i32TempLow;

    //
    // The current humidity(units determined by caller).
    //
    int32_t i32Humidity;

    //
    // The current atmospheric pressure(units determined by caller).
    //
    int32_t i32Pressure;

    //
    // The last time these values were update (GMT Unix time).
    //
    uint32_t ui32Time;

    //
    // The sunrise time (GMT Unix time).
    //
    uint32_t ui32SunRise;

    //
    // The sunset time (GMT Unix time).
    //
    uint32_t ui32SunSet;

    //
    // Icon image.
    //
    const uint8_t *pui8Image;
}
tWeatherReport;

//*****************************************************************************
//
// The type definition for event functions.
//
//*****************************************************************************
typedef void (* tEventFunction)(uint32_t ui32Event, void* pvData,
                                uint32_t ui32Param);

//*****************************************************************************
//
// Exported Ethernet function prototypes.
//
//*****************************************************************************
extern void EthClientInit(tEventFunction pfnEvent);
extern void EthClientProxySet(const char *pcProxyName);
extern void EthClientTick(uint32_t ui32TickMS);
extern uint32_t EthClientAddrGet(void);
extern void EthClientMACAddrGet(uint8_t *pui8Addr);
extern uint32_t EthClientServerAddrGet(void);
extern void EthClientReset(void);
void EthClientTCPDisconnect(void);

//*****************************************************************************
//
// Exported weather related prototypes.
//
//*****************************************************************************
extern void WeatherSourceSet(tWeatherSource eWeatherSource);
extern int32_t WeatherCurrent(tWeatherSource eWeatherSource,
                              const char *pcQuery,
                              tWeatherReport *psWeather,
                              tEventFunction pfnEvent);
extern int32_t WeatherForecast(tWeatherSource eWeatherSource,
                               const char *pcQuery,
                               tWeatherReport *psWeather,
                               tEventFunction pfnEvent);

#ifdef __cplusplus
}
#endif

#endif
