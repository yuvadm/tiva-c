//******************************************************************************
//
// wavfile.c - This file supports reading audio data from a .wav file and
// reading the file format.
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
// This is part of revision 2.1.0.12573 of the Tiva Utility Library.
//
//******************************************************************************

#include <stdint.h>
#include "inc/hw_types.h"
#include "third_party/fatfs/src/ff.h"
#include "third_party/fatfs/src/diskio.h"
#include "wavfile.h"

//******************************************************************************
//
// The flag values for the ui32Flags member of the tWavFile structure.
//
//******************************************************************************
#define WAV_FLAG_FILEOPEN       0x00000001

//******************************************************************************
//
// Basic wav file RIFF header information used to open and read a wav file.
//
//******************************************************************************
#define RIFF_CHUNK_ID_RIFF      0x46464952
#define RIFF_CHUNK_ID_FMT       0x20746d66
#define RIFF_CHUNK_ID_DATA      0x61746164
#define RIFF_TAG_WAVE           0x45564157
#define RIFF_FORMAT_UNKNOWN     0x0000
#define RIFF_FORMAT_PCM         0x0001
#define RIFF_FORMAT_MSADPCM     0x0002
#define RIFF_FORMAT_IMAADPCM    0x0011

//******************************************************************************
//
// This function returns the format of a wav file that has been opened with
// the WavOpen() function.
//
// \param psWavData is the structure that was passed to the WavOpen() function.
// \param psWavHeader is the structure to fill with the format of the wav file.
//
// This function is used to get the audio format of a file that was opened
// with the WavOpen() function.  The \e psWavData parameter should be the
// same structure that was passed to the WavOpen() function.  The
// \e psWavHeader function will be filled with the format of the open file if
// the \e psWavData is a valid open file.  If this function is called with
// an invalid \e psWavData then the results will be undetermined.
//
// \return None.
//
//******************************************************************************
void
WavGetFormat(tWavFile *psWavData, tWavHeader *psWavHeader)
{
    //
    // Only return data if the file is open.
    //
    psWavHeader->ui32DataSize = psWavData->sWavHeader.ui32DataSize;
    psWavHeader->ui16NumChannels = psWavData->sWavHeader.ui16NumChannels;
    psWavHeader->ui32SampleRate = psWavData->sWavHeader.ui32SampleRate;
    psWavHeader->ui32AvgByteRate = psWavData->sWavHeader.ui32AvgByteRate;
    psWavHeader->ui16BitsPerSample = psWavData->sWavHeader.ui16BitsPerSample;
}

//******************************************************************************
//
// This function is called to open and determine if a file is a valid .wav
// file.
//
// \param pcFileName is the null terminated string for the file to open.
// \param psWavData is the structure used to hold the file state information.
//
// This function is used to open a file and determine if it is a valid .wav
// file.  The \e pcFileName will be opened and read to look for a valid .wav
// file header and prepared for calling the WavRead() or WavGetFormat()
// functions.  When an application is done with the .wav file it should call
// the WavClose() function to free up the file.  The function will return
// zero if the function successfully opened a .wav file and a non-zero value
// indicates that the file was a valid .wav file or the file could not be
// opened.
//
// \return A value of zero indicates that the file was successfully opened and
// any other value indicates that the file was not opened.
//
//******************************************************************************
int
WavOpen(const char *pcFileName, tWavFile *psWavData)
{
    unsigned char pucBuffer[16];
    uint32_t *pui32Buffer;
    uint16_t *pui16Buffer;
    uint32_t ui32ChunkSize;
    uint32_t ui32Count;

    //
    // Create some local pointers using in parsing values.
    //
    pui32Buffer = (uint32_t *)pucBuffer;
    pui16Buffer = (uint16_t *)pucBuffer;

    //
    // Open the file as read only.
    //
    if(f_open(&psWavData->i16File, pcFileName, FA_READ) != FR_OK)
    {
        return(-1);
    }

    //
    // File is open.
    //
    psWavData->ui32Flags = WAV_FLAG_FILEOPEN;

    //
    // Read the first 12 bytes.
    //
    if(f_read(&psWavData->i16File, pucBuffer, 12, (UINT *)&ui32Count) != FR_OK)
    {
        return(-1);
    }

    //
    // Look for RIFF tag.
    //
    if((pui32Buffer[0] != RIFF_CHUNK_ID_RIFF) || 
       (pui32Buffer[2] != RIFF_TAG_WAVE))
    {
        return(-1);
    }

    //
    // Read the next chunk header.
    //
    if(f_read(&psWavData->i16File, pucBuffer, 8, (UINT *)&ui32Count) != FR_OK)
    {
        return(-1);
    }

    //
    // Now look for the RIFF ID format tag.
    //
    if(pui32Buffer[0] != RIFF_CHUNK_ID_FMT)
    {
        return(-1);
    }

    //
    // Read the format chunk size and insure that it is 16.
    //
    ui32ChunkSize = pui32Buffer[1];

    if(ui32ChunkSize > 16)
    {
        return(-1);
    }

    //
    // Read the next chunk header.
    //
    if(f_read(&psWavData->i16File, pucBuffer, ui32ChunkSize, 
              (UINT *)&ui32Count) != FR_OK)
    {
        return(-1);
    }

    //
    // Save the audio format data so that it can be returned later if
    // requested.
    //
    psWavData->sWavHeader.ui16Format = pui16Buffer[0];
    psWavData->sWavHeader.ui16NumChannels =  pui16Buffer[1];
    psWavData->sWavHeader.ui32SampleRate = pui32Buffer[1];
    psWavData->sWavHeader.ui32AvgByteRate = pui32Buffer[2];
    psWavData->sWavHeader.ui16BitsPerSample = pui16Buffer[7];

    //
    // Only mono and stereo supported.
    //
    if(psWavData->sWavHeader.ui16NumChannels > 2)
    {
        return(-1);
    }

    //
    // Read the next chunk header.
    //
    if(f_read(&psWavData->i16File, pucBuffer, 8, (UINT *)&ui32Count) != FR_OK)
    {
        return(-1);
    }

    //
    // Now make sure that the file has a data chunk.
    //
    if(pui32Buffer[0] != RIFF_CHUNK_ID_DATA)
    {
        return(-1);
    }

    //
    // Save the size of the data.
    //
    psWavData->sWavHeader.ui32DataSize = pui32Buffer[1];

    return(0);
}

//******************************************************************************
//
// This is used to close a .wav file that was opened with WavOpen().
//
// \param psWavData is the file structure that was passed into the WavOpen()
// function.
//
// This function should be called when a function has completed using a .wav
// file that was opened with the WavOpen() function.  This will free up any
// file system data that is held while the file is open.
//
// \return None.
//
//******************************************************************************
void
WavClose(tWavFile *psWavData)
{
    if(psWavData->ui32Flags & WAV_FLAG_FILEOPEN)
    {
        //
        // Close out the file.
        //
        f_close(&psWavData->i16File);

        //
        // Mark file as no longer open.
        //
        psWavData->ui32Flags &= ~WAV_FLAG_FILEOPEN;
    }
}

//******************************************************************************
//
// This function is used to read audio data from a file that was opened with
// the WavOpen() function.
//
// \param psWavData is the file structure that was passed into the WavOpen()
// function.
// \param pucBuffer is the buffer to read data into.
// \param ui32Size is the amount of data to read in bytes.
//
//
// This function handles reading data from a .wav file that was opened with
// the WavOpen() function.   The function will return the actual number of
// of bytes read from the file.
//
// \return This function returns the number of bytes read from the file.
//
//******************************************************************************
uint16_t
WavRead(tWavFile *psWavData, unsigned char *pucBuffer, uint32_t ui32Size)
{
    uint32_t ui32Count;

    //
    // Read in another buffer from the file.
    //
    if(f_read(&psWavData->i16File, pucBuffer, ui32Size, 
              (UINT *)&ui32Count) != FR_OK)
    {
        return(0);
    }

    return(ui32Count);
}
