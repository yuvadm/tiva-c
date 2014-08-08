/*****************************************************************************
*
* exosite_meta.c - Exosite meta information handler.
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

#include "exosite_meta.h"
#include "drivers/exosite_hal_lwip.h"
#include <string.h>
//#include <inc/common.h>

// external functions
// externs
// local functions
// exported functions
// local defines
// globals

/*****************************************************************************
*
*  exosite_meta_init
*
*  \param  int reset : 1 - reset meta data
*
*  \return None
*
*  \brief  Does whatever we need to do to initialize the NV meta structure
*
*****************************************************************************/
void exosite_meta_init(int reset)
{
  char strBuf[META_MARK_SIZE];

  exoHAL_EnableMeta();  //turn on the necessary hardware / peripherals

  //check our meta mark - if it isn't there, we wipe the meta structure
  exosite_meta_read((unsigned char *)strBuf, META_MARK_SIZE, META_MARK);
  if (strncmp(strBuf, EXOMARK, META_MARK_SIZE) || reset)
    exosite_meta_defaults();

  return;
}


/*****************************************************************************
*
*  exosite_meta_defaults
*
*  \param  None
*
*  \return None
*
*  \brief  Writes default meta values to NV memory.  Erases existing meta
*          information!
*
*****************************************************************************/
void exosite_meta_defaults(void)
{
  const unsigned char meta_server_ip[6] = {173,255,209,28,0,80};

  exoHAL_EraseMeta(); //erase the information currently in meta
  exosite_meta_write((unsigned char *)meta_server_ip, 6, META_SERVER);     //store server IP
  exosite_meta_write((unsigned char *)EXOMARK, META_MARK_SIZE, META_MARK); //store exosite mark

  return;
}


/*****************************************************************************
*
*  exosite_meta_write
*
*  \param  write_buffer - string buffer containing info to write to meta;
*          srcBytes - size of string in bytes; element - item from
*          MetaElements enum.
*
*  \return None
*
*  \brief  Writes specific meta information to meta memory
*
*****************************************************************************/
void exosite_meta_write(unsigned char * write_buffer, unsigned short srcBytes, unsigned char element)
{
  exosite_meta * meta_info = 0;

  //TODO - do not write if the data already there is identical...

  switch (element) {
    case META_CIK:
      if (srcBytes > META_CIK_SIZE) return;
      exoHAL_WriteMetaItem(write_buffer, srcBytes, (int)meta_info->cik); //store CIK
      break;
    case META_SERVER:
      if (srcBytes > META_SERVER_SIZE) return;
      exoHAL_WriteMetaItem(write_buffer, srcBytes, (int)meta_info->server); //store server IP
      break;
    case META_MARK:
      if (srcBytes > META_MARK_SIZE) return;
      exoHAL_WriteMetaItem(write_buffer, srcBytes, (int)meta_info->mark); //store exosite mark
      break;
    case META_UUID:
      if (srcBytes > META_UUID_SIZE) return;
      exoHAL_WriteMetaItem(write_buffer, srcBytes, (int)meta_info->uuid); //store UUID
      break;
    case META_MFR:
      if (srcBytes > META_MFR_SIZE) return;
      exoHAL_WriteMetaItem(write_buffer, srcBytes, (int)meta_info->mfr); //store manufacturing info
      break;
    case META_NONE:
    default:
      break;
  }

  return;
}


/*****************************************************************************
*
*  exosite_meta_read
*
*  \param  read_buffer - string buffer to receive element data; destBytes -
*          size of buffer in bytes; element - item from MetaElements enum.
*
*  \return None
*
*  \brief  Writes specific meta information to meta memory
*
*****************************************************************************/
void exosite_meta_read(unsigned char * read_buffer, unsigned short destBytes, unsigned char element)
{
  exosite_meta * meta_info = 0;

  switch (element) {
    case META_CIK:
      if (destBytes < META_CIK_SIZE) return;
      exoHAL_ReadMetaItem(read_buffer, META_CIK_SIZE, (int)meta_info->cik); //read CIK
      break;
    case META_SERVER:
      if (destBytes < META_SERVER_SIZE) return;
      exoHAL_ReadMetaItem(read_buffer, META_SERVER_SIZE, (int)meta_info->server); //read server IP
      break;
    case META_MARK:
      if (destBytes < META_MARK_SIZE) return;
      exoHAL_ReadMetaItem(read_buffer, META_MARK_SIZE, (int)meta_info->mark); //read exosite mark
      break;
    case META_UUID:
      if (destBytes < META_UUID_SIZE) return;
      exoHAL_ReadMetaItem(read_buffer, META_UUID_SIZE, (int)meta_info->uuid); //read provisioning UUID
      break;
    case META_MFR:
      if (destBytes < META_MFR_SIZE) return;
      exoHAL_ReadMetaItem(read_buffer, META_MFR_SIZE, (int)meta_info->mfr); //read manufacturing info
      break;
    case META_NONE:
    default:
      break;
  }

  return;
}


