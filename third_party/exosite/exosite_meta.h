/*****************************************************************************
*
* exosite_meta.h - Meta informatio header
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

#ifndef EXOSITE_META_H
#define EXOSITE_META_H

// defines
#define META_SIZE                 256
#define META_CIK_SIZE             40
#define META_SERVER_SIZE          6
#define META_PAD0_SIZE            2
#define META_MARK_SIZE            8
#define META_UUID_SIZE            12
#define META_PAD1_SIZE            4
#define META_RSVD_SIZE            48          // TODO - flash block size is 128 - either make MFR these 48 RSVD bytes or instrument flash routine to use next block for MFR
#define META_MFR_SIZE             128
typedef struct {
    char cik[META_CIK_SIZE];                   // our client interface key
    char server[META_SERVER_SIZE];             // ip address of m2.exosite.com (not using DNS at this stage)
    char pad0[META_PAD0_SIZE];                 // pad 'server' to 8 bytes
    char mark[META_MARK_SIZE];                 // watermark
    char uuid[META_UUID_SIZE];                 // UUID in ascii
    char pad1[META_PAD1_SIZE];                 // pad 'uuid' to 16 bytes
    char rsvd[META_RSVD_SIZE];                 // reserved space - pad to ensure mfr is at end of RDK_META_SIZE
    char mfr[META_MFR_SIZE];                   // manufacturer data structure
} exosite_meta;

#define EXOMARK "exosite!"

typedef enum
{
    META_CIK,
    META_SERVER,
    META_MARK,
    META_UUID,
    META_MFR,
    META_NONE
} MetaElements;

// functions for export
void exosite_meta_defaults(void);
void exosite_meta_init(int reset);
void exosite_meta_write(unsigned char * write_buffer, unsigned short srcBytes, unsigned char element);
void exosite_meta_read(unsigned char * read_buffer, unsigned short destBytes, unsigned char element);

#endif

