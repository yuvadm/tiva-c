//*****************************************************************************
//
// http.c - HTTP request creation functions.
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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_types.h"
#include "utils/ustdlib.h"
#include "http.h"

//*****************************************************************************
//
// Buffer used to store temporary strings for constructing/parsing requests.
//
//*****************************************************************************
static char g_pcTempData[256];

//*****************************************************************************
//
// Declarations for request type strings.
//
//*****************************************************************************
static char g_pcHttpConnect[] = "CONNECT ";
static char g_pcHttpGet[] = "GET ";
static char g_pcHttpPost[] = "POST ";
static char g_pcHttpPut[] = "PUT ";
static char g_pcHttpDelete[] = "DELETE ";
static char g_pcHttpHead[] = "HEAD ";
static char g_pcHttpTrace[] = "TRACE ";
static char g_pcHttpOptions[] = "OPTIONS ";
static char g_pcHttpPatch[] = "PATCH ";

//*****************************************************************************
//
// HTTP suffixes used by HTTPMessageTypeSet().  Default is HTTP 1.1.
//
//*****************************************************************************
#ifdef USE_HTTP_1_0
static const char g_pcSuffixHttp10[] = " HTTP/1.0\r\n\r\n";
#else
static const char g_pcSuffixHttp11[] = " HTTP/1.1\r\n\r\n";
#endif

//*****************************************************************************
//
//! Extract the portion of a string up to a specified character.
//!
//! \param cWhichOne specifies the character to compare against.
//! \param pcSource is a pointer to the source/input string.
//! \param pcOutput is a pointer to the destination/output string.
//! \param pui32Size is a pointer to a variable that will receive the size of
//!  the destination/output string.
//!
//! \return None.
//
//*****************************************************************************
static void
BufferFillToCharacter(char cWhichOne, char *pcSource, char *pcOutput,
                      uint32_t *pui32Size)
{
    //
    // Search the string until the character is found.
    //
    *pui32Size = 0;
    while(*pcSource)
    {
        if(*pcSource == cWhichOne)
        {
            pcOutput[*pui32Size] = 0;
            pcSource++;
            return;
        }
        else
        {
            pcOutput[*pui32Size] = *pcSource;
            pcSource++;
            *pui32Size += 1;
        }
    }

    return;
}

//*****************************************************************************
//
//! Extract the portion of a string up to end-of-line (EOL).
//!
//! \param pcSource is a pointer to the source/input string.
//! \param pcOutput is a pointer to the destination/output string.
//! \param pui32Size is a pointer to a variable that will receive the size of
//!  the destination/output string.
//!
//! \return None.
//
//*****************************************************************************
static void
BufferFillToEOL(char *pcSource, char *pcOutput, uint32_t *pui32Size)
{
    //
    // Search the string until EOL is found.
    //
    *pui32Size = 0;
    while(*pcSource)
    {
        if((*pcSource == '\r') && (*(pcSource+ 1) == '\n'))
        {
            pcOutput[*pui32Size] = 0;
            pcSource += 2;
            return;
        }
        else
        {
            pcOutput[*pui32Size] = *pcSource;
            pcSource++;
            *pui32Size += 1;
        }
    }

    return;
}

static void
InsertRequest(char *pcDest, char *pcRequest)
{
    uint32_t i;
    uint32_t ui32ReqSize;
    uint32_t ui32DstSize;

    ui32ReqSize = strlen(pcRequest);
    ui32DstSize = strlen(pcDest);

    pcDest[ui32DstSize + ui32ReqSize] = 0;
    for(i = ui32DstSize; i-- > 0; )
    {
        pcDest[ui32ReqSize + i] = pcDest[i];
    }

    for(i = 0; i < ui32ReqSize; i++)
    {
        pcDest[i] = pcRequest[i];
    }
}

//*****************************************************************************
//
//! Set the HTTP message type.
//!
//! \param pcDest is a pointer to the destination/output string.
//! \param ui8Type is the HTTP request type.  Macros such as HTTP_MESSAGE_GET
//! are defined in http.h.
//! \param pcResource is a pointer to a string containing the resource portion
//! of the HTTP message.  The resource goes in between the type  (ex: GET) and
//! HTTP suffix on the first line of a HTTP request.  An example would be
//! index.html.
//!
//! This function should be called to start off a new HTTP request.
//!
//! \return None.
//
//*****************************************************************************
void
HTTPMessageTypeSet(char *pcDest, uint8_t ui8Type, char *pcResource)
{
    //
    // Check to see if the resource and destination pointers are the same.  If
    // yes, insert the request type at the beginning of the resource string.
    //
    if(pcDest == pcResource)
    {
        //
        // Add the request type to the buffer.
        //
        switch(ui8Type)
        {
            case HTTP_MESSAGE_CONNECT:
            {
                InsertRequest(pcDest, g_pcHttpConnect);
                break;
            }
            case HTTP_MESSAGE_GET:
            {
                InsertRequest(pcDest, g_pcHttpGet);
                break;
            }
            case HTTP_MESSAGE_POST:
            {
                InsertRequest(pcDest, g_pcHttpPost);
                break;
            }
            case HTTP_MESSAGE_PUT:
            {
                InsertRequest(pcDest, g_pcHttpPut);
                break;
            }
            case HTTP_MESSAGE_DELETE:
            {
                InsertRequest(pcDest, g_pcHttpDelete);
                break;
            }
            case HTTP_MESSAGE_HEAD:
            {
                InsertRequest(pcDest, g_pcHttpHead);
                break;
            }
            case HTTP_MESSAGE_TRACE:
            {
                InsertRequest(pcDest, g_pcHttpTrace);
                break;
            }
            case HTTP_MESSAGE_OPTIONS:
            {
                InsertRequest(pcDest, g_pcHttpOptions);
                break;
            }
            case HTTP_MESSAGE_PATCH:
            {
                InsertRequest(pcDest, g_pcHttpPatch);
                break;
            }
        }
    }
    else
    {
        //
        // Add the request type to the buffer.
        //
        switch(ui8Type)
        {
            case HTTP_MESSAGE_CONNECT:
            {
                usprintf(pcDest, g_pcHttpConnect);
                break;
            }
            case HTTP_MESSAGE_GET:
            {
                usprintf(pcDest, g_pcHttpGet);
                break;
            }
            case HTTP_MESSAGE_POST:
            {
                usprintf(pcDest, g_pcHttpPost);
                break;
            }
            case HTTP_MESSAGE_PUT:
            {
                usprintf(pcDest, g_pcHttpPut);
                break;
            }
            case HTTP_MESSAGE_DELETE:
            {
                usprintf(pcDest, g_pcHttpDelete);
                break;
            }
            case HTTP_MESSAGE_HEAD:
            {
                usprintf(pcDest, g_pcHttpHead);
                break;
            }
            case HTTP_MESSAGE_TRACE:
            {
                usprintf(pcDest, g_pcHttpTrace);
                break;
            }
            case HTTP_MESSAGE_OPTIONS:
            {
                usprintf(pcDest, g_pcHttpOptions);
                break;
            }
            case HTTP_MESSAGE_PATCH:
            {
                usprintf(pcDest, g_pcHttpPatch);
                break;
            }
        }

        //
        // Add the resource to the buffer.
        //
        strcat(pcDest, pcResource);
    }

    //
    // Finish the first "line" by adding the HTTP suffix.
    //
#ifdef USE_HTTP_1_0
    strcat(pcDest, g_pcSuffixHttp10);
#else
    strcat(pcDest, g_pcSuffixHttp11);
#endif
}

//*****************************************************************************
//
//! Add a header to a HTTP request.
//!
//! \param pcDest is a pointer to the destination/output string.
//! \param pcHeaderName is a pointer to a string containing the header name.
//! \param pcHeaderValue is a pointer to a string containing the header data.
//!
//! Note that this function must be called after HTTPMessageTypeSet() as it
//! simply appends a header to an existing string/buffer.
//!
//! \return None.
//
//*****************************************************************************
void
HTTPMessageHeaderAdd(char *pcDest, char *pcHeaderName, char *pcHeaderValue)
{
    //
    // Add the header name to the buffer.
    //
    strcat(pcDest, pcHeaderName);

    //
    // Add ":" and space.
    //
    strcat(pcDest, ": ");

    //
    // Add header value.
    //
    strcat(pcDest, pcHeaderValue);

    //
    // Add \r and \n.
    //
    strcat(pcDest, "\r\n");
}

//*****************************************************************************
//
//! Add body data to to a HTTP request.
//!
//! \param pcDest is a pointer to the destination/output string.
//! \param pcBodyData is a pointer to a string containing the body data.  This
//! can be anything from HTML to encoded data (such as JSON).
//!
//! Note that this function must be called after HTTPMessageTypeSet() and
//! HTTPMessageHeaderAdd() as it simply appends the body data to an existing
//! string/buffer.
//!
//! \return None.
//
//*****************************************************************************
void
HTTPMessageBodyAdd(char *pcDest, char *pcBodyData)
{
    //
    // First, insert blank line between header section and body.
    //
    strcat(pcDest, "\r\n");

    //
    // Add body content.
    //
    strcat(pcDest, pcBodyData);

    //
    // Add blank line.
    //
    strcat(pcDest, "\r\n\r\n");
}

//*****************************************************************************
//
//! Parse a HTTP response.
//!
//! \param pcData is a pointer to the source string/buffer.
//! \param pcResponseText is a pointer to a string that will receive the
//! response text from the first line of the HTTP response.
//! \param pui32NumHeaders is a pointer to a variable that will receive the
//! number of headers detected in pcData.
//!
//! Note that this function must be called after HTTPMessageTypeSet() as it
//! simply appends a header to an existing string/buffer.
//!
//! \return Returns the HTTP response code.  If parsing error occurs, returns 0.
//
//*****************************************************************************
uint32_t
HTTPResponseParse(char *pcData, char *pcResponseText, uint32_t *pui32NumHeaders)
{
    uint32_t ui32Response = 0;
    uint32_t ui32Size;

    //
    // Look for "HTTP/n.n" piece.
    //
    BufferFillToCharacter(' ', pcData, g_pcTempData, &ui32Size);
    pcData += (ui32Size + 1);

    //
    // Fail if not a HTTP response.
    //
    if(ustrncmp(g_pcTempData, "HTTP/", 5))
    {
        *pcResponseText = 0;
        *pui32NumHeaders = 0;
        return 0;
    }

    //
    // Get the return code.
    //
    BufferFillToCharacter(' ', pcData, g_pcTempData, &ui32Size);
    pcData += (ui32Size + 1);

    //
    // Convert return code to unsigned long.
    //
    ui32Response = ustrtoul(g_pcTempData, 0, 10);

    //
    // Get the response text.
    //
    BufferFillToEOL(pcData, pcResponseText, &ui32Size);
    pcData += (ui32Size + 2);

    //
    // Parse the remainder of the packet.
    //
    *pui32NumHeaders = 0;
    while(*pcData)
    {
        //
        // Search line by line and count headers.  Search until there is a blank
        // line, which means end of headers.
        //
        BufferFillToEOL(pcData, g_pcTempData, &ui32Size);
        pcData += (ui32Size + 2);

        //
        // Blank line.  For the purposes of this function, we're done.
        //
        if(ui32Size == 0)
        {
            break;
        }
        else
        {
            //
            // Increment header counter.
            //
            *pui32NumHeaders += 1;
        }
    }

    return ui32Response;
}

//*****************************************************************************
//
//! Extract a specified header from a HTTP response string/buffer.
//!
//! \param pcData is a pointer to the source string/buffer.
//! \param ui32HeaderIdx specifies the index of the header to extract.
//! \param pcHeaderName is a pointer to a string that will receive the name of
//! the header specified by ui32HeaderIdx.
//! \param pcHeaderValue is a pointer to a string that will receive the value of
//! the header specified by ui32HeaderIdx.
//!
//! Note that this function should be used in conjunction with
//! HTTPResponseParse() since it notifies the application of the number of
//! headers in a string/buffer.
//!
//! \return None.
//
//*****************************************************************************
void
HTTPResponseHeaderExtract(char *pcData, uint32_t ui32HeaderIdx,
                          char *pcHeaderName, char *pcHeaderValue)
{
    uint32_t ui32Size;
    uint32_t ui32HeaderNumber = 0;

    //
    // Read first line and discard.
    //
    BufferFillToEOL(pcData, g_pcTempData, &ui32Size);
    pcData += (ui32Size + 2);

    //
    // Find and return the requested header.
    //
    while(*pcData)
    {
        BufferFillToEOL(pcData, g_pcTempData, &ui32Size);
        pcData += (ui32Size + 2);

        //
        // Blank line, end of header section.
        //
        if(ui32Size == 0)
        {
            break;
        }
        else
        {
            if(ui32HeaderNumber == ui32HeaderIdx)
            {
                BufferFillToCharacter(':', g_pcTempData, pcHeaderName,
                                      &ui32Size);
                BufferFillToCharacter(0, &g_pcTempData[ui32Size + 2],
                                      pcHeaderValue, &ui32Size);
                pcHeaderValue[ui32Size] = 0;

                break;
            }
            else
            {
                //
                // Increment header counter.
                //
                ui32HeaderNumber++;
            }
        }
    }
}

//*****************************************************************************
//
//! Extract the body from a HTTP response string/buffer.
//!
//! \param pcData is a pointer to the source string/buffer.
//! \param pcDest is a pointer to a string that will receive the body data.
//!
//! \return None.
//
//*****************************************************************************
void
HTTPResponseBodyExtract(char *pcData, char *pcDest)
{
    bool bBodyFound = false;
    uint32_t ui32Size;

    //
    // Find and return the body.
    //
    while(*pcData)
    {
        //
        // Read lines until a blank line is found.  This is the start of the
        // body.
        //
        BufferFillToEOL(pcData, g_pcTempData, &ui32Size);
        pcData += (ui32Size + 2);

        //
        // Blank line, end of header section.
        //
        if(ui32Size == 0)
        {
            bBodyFound = true;
        }

        //
        // If the body has been found, start filling the buffer.
        //
        if(bBodyFound)
        {

            pcDest[0] = 0;
            strcat(pcDest, pcData);
            break;
        }
    }
}
