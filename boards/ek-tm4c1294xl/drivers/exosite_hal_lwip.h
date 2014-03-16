//*****************************************************************************
//
// exosite_hal_lwip.h - Abstraction layer between Excosite and eth_client_lwip.
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
// This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#ifndef EXOSITE_HAL_LWIP_H
#define EXOSITE_HAL_LWIP_H

#include <stdint.h>
#include <stdbool.h>

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
// Proxy Config.
//
//*****************************************************************************
extern bool g_bUseProxy;
extern char g_pcProxyAddress[50];
extern uint16_t g_ui16ProxyPort;

//*****************************************************************************
//
// Exosite Config.
//
//*****************************************************************************
#define EXOSITE_ADDRESS         "m2.exosite.com"
#define EXOSITE_PORT            80

//*****************************************************************************
//
// Meta Structure.
//
//*****************************************************************************
#define EXOMETA_ADDR_OFFSET                  0

//*****************************************************************************
//
// Maximum length for the serial number.
//
//*****************************************************************************
#define EXOSITE_HAL_SN_MAXLENGTH             25

//*****************************************************************************
//
// Maximum size for the circular receive buffer.
//
//*****************************************************************************
#define RECEIVE_BUFFER_SIZE                 1024

//*****************************************************************************
//
// EEPROM status.
//
//*****************************************************************************
#define EEPROM_INITALIZED                   1
#define EEPROM_IDLE                         2
#define EEPROM_READING                      3
#define EEPROM_WRITING                      4
#define EEPROM_ERASING                      5
#define EEPROM_ERROR                        6

//*****************************************************************************
//
// Flag indexes for g_sExosite.ui32Flags
//
//*****************************************************************************
#define FLAG_ENET_INIT          0
#define FLAG_CONNECTED          1
#define FLAG_DNS_INIT           2
#define FLAG_PROXY_SET          3
#define FLAG_BUSY               4
#define FLAG_SENT               5
#define FLAG_RECEIVED           6
#define FLAG_CONNECT_WAIT       7

//*****************************************************************************
//
// Prototypes.
//
//*****************************************************************************
typedef void (* tExositeEventHandler)(uint32_t ui32Event, void* pvData1,
                                      uint16_t ui16Size1, void* pvData2,
                                      uint16_t ui16Size2);

int exoHAL_ReadUUID(unsigned char ucIfNbr, unsigned char * pucUUIDBuf);
void exoHAL_EnableMeta(void);
void exoHAL_EraseMeta(void);
void exoHAL_WriteMetaItem(unsigned char * pucBuffer, int iLength, int iOffset);
void exoHAL_ReadMetaItem(unsigned char * pucBuffer, int iLength, int iOffset);
void exoHAL_SocketClose(long ulSocket);
long exoHAL_SocketOpenTCP(unsigned char *pucServer);
long exoHAL_ServerConnect(long ulSocket);
unsigned char exoHAL_SocketSend(long lSocket, char * pcBuffer, int iLength);
unsigned char exoHAL_SocketRecv(long lSocket, char * pcBuffer, int iLength);
void exoHAL_MSDelay(unsigned short usDelay);
void exoHAL_Tick(unsigned long ulDelay);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //EXOSITE_HAL_LWIP_H

