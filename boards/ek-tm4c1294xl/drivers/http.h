//*****************************************************************************
//
// http.h - Prototypes for the HTTP protocol layer.
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
#ifndef HTTP_H_
#define HTTP_H_

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
// HTTP request types used by HTTPMessageTypeSet().
//
//*****************************************************************************
#define HTTP_MESSAGE_CONNECT    0x0
#define HTTP_MESSAGE_GET        0x1
#define HTTP_MESSAGE_POST       0x2
#define HTTP_MESSAGE_PUT        0x3
#define HTTP_MESSAGE_DELETE     0x4
#define HTTP_MESSAGE_HEAD       0x5
#define HTTP_MESSAGE_TRACE      0x6
#define HTTP_MESSAGE_OPTIONS    0x7
#define HTTP_MESSAGE_PATCH      0x8

//*****************************************************************************
//
// Exported function prototypes.
//
//*****************************************************************************
extern void HTTPMessageTypeSet(char *pcDest, uint8_t ui8Type, char *pcResource);
extern void HTTPMessageHeaderAdd(char *pcDest, char *pcHeaderName,
                                 char *pcHeaderValue);
extern void HTTPMessageBodyAdd(char *pcDest, char *pcBodyData);

extern uint32_t HTTPResponseParse(char *pcData, char *pcResponseText,
                                  uint32_t *pui32NumHeaders);

extern void HTTPResponseHeaderExtract(char *pcData, uint32_t ui32HeaderIdx,
                                      char *pcHeaderName, char *pcHeaderValue);

extern void HTTPResponseBodyExtract(char *pcData, char *pcDest);

#ifdef __cplusplus
}
#endif

#endif
