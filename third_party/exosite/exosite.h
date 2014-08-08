/*****************************************************************************
*
* exosite.h - Exosite library interface header
* Copyright (c) 2013, Exosite LLC
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are met:
* 
*     * Redistributions of source code must retain the above copyright notice,
*       this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright 
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Exosite LLC nor the names of its contributors may
*       be used to endorse or promote products derived from this software 
*       without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/

#ifndef EXOSITE_H
#define EXOSITE_H

#include <stdint.h>

// defines
enum UUIDInterfaceTypes
{
    IF_WIFI,
    IF_ENET,
    IF_FILE,
    IF_HDD,
    IF_I2C,
    IF_GPRS,
    IF_NONE
};

enum ExositeStatusCodes
{
    EXO_STATUS_OK,
    EXO_STATUS_INIT,
    EXO_STATUS_BAD_UUID,
    EXO_STATUS_BAD_VENDOR,
    EXO_STATUS_BAD_MODEL,
    EXO_STATUS_BAD_INIT,
    EXO_STATUS_BAD_TCP,
    EXO_STATUS_BAD_SN,
    EXO_STATUS_CONFLICT,
    EXO_STATUS_BAD_CIK,
    EXO_STATUS_NOAUTH,
    EXO_STATUS_END
};

#define EXOSITE_VENDOR_MAXLENGTH                 20
#define EXOSITE_MODEL_MAXLENGTH                  20
#define EXOSITE_SN_MAXLENGTH                     EXOSITE_HAL_SN_MAXLENGTH
#define EXOSITE_DEMO_UPDATE_INTERVAL            4000// ms
#define CIK_LENGTH                              40

// functions for export
int Exosite_Write(char * pbuf, unsigned int bufsize);
int Exosite_Read(char * palias, char * pbuf, unsigned int buflen);
int Exosite_Init(const char *vendor, const char *model, const unsigned char if_nbr, int reset);
int Exosite_Activate(void);
void Exosite_SetCIK(char * pCIK);
int Exosite_GetCIK(char * pCIK);
int Exosite_StatusCode(void);
int Exosite_GetResponse(void);
#endif

