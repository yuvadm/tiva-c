//*****************************************************************************
//
// wavfile.h - This file supports reading audio data from a .wav file and
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
//*****************************************************************************

#ifndef WAVEFILE_H_
#define WAVEFILE_H_

//*****************************************************************************
//
// The wav file header information.
//
//*****************************************************************************
typedef struct
{
    //
    // Sample rate in bytes per second.
    //
    uint32_t ui32SampleRate;

    //
    // The average byte rate for the wav file.
    //
    uint32_t ui32AvgByteRate;

    //
    // The size of the wav data in the file.
    //
    uint32_t ui32DataSize;

    //
    // The number of bits per sample.
    //
    uint16_t ui16BitsPerSample;

    //
    // The wav file format.
    //
    uint16_t ui16Format;

    //
    // The number of audio channels.
    //
    uint16_t ui16NumChannels;
}
tWavHeader;

//*****************************************************************************
//
// The structure used to hold the wav file state.
//
//*****************************************************************************
typedef struct
{
    //
    // The wav files header information
    //
    tWavHeader sWavHeader;

    //
    // The file information for the current file.
    //
    FIL i16File;

    //
    // Current state flags, a combination of the WAV_FLAG_* values.
    //
    uint32_t ui32Flags;
} tWavFile;

void WavGetFormat(tWavFile *psWavData, tWavHeader *psWaveHeader);
int WavOpen(const char *pcFileName, tWavFile *psWavData);
void WavClose(tWavFile *psWavData);
uint16_t WavRead(tWavFile *psWavData, unsigned char *pucBuffer,
                        uint32_t ui32Size);

#endif
