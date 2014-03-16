//*****************************************************************************
//
// ringbuf.h - Defines and Macros for the ring buffer utilities.
//
// Copyright (c) 2008-2014 Texas Instruments Incorporated.  All rights reserved.
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

#ifndef __RINGBUF_H__
#define __RINGBUF_H__

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
// The structure used for encapsulating all the items associated with a
// ring buffer.
//
//*****************************************************************************
typedef struct
{
    //
    // The ring buffer size.
    //
    uint32_t ui32Size;

    //
    // The ring buffer write index.
    //
    volatile uint32_t ui32WriteIndex;

    //
    // The ring buffer read index.
    //
    volatile uint32_t ui32ReadIndex;

    //
    // The ring buffer.
    //
    uint8_t *pui8Buf;

}
tRingBufObject;

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern bool RingBufFull(tRingBufObject *psRingBuf);
extern bool RingBufEmpty(tRingBufObject *psRingBuf);
extern void RingBufFlush(tRingBufObject *psRingBuf);
extern uint32_t RingBufUsed(tRingBufObject *psRingBuf);
extern uint32_t RingBufFree(tRingBufObject *psRingBuf);
extern uint32_t RingBufContigUsed(tRingBufObject *psRingBuf);
extern uint32_t RingBufContigFree(tRingBufObject *psRingBuf);
extern uint32_t RingBufSize(tRingBufObject *psRingBuf);
extern uint8_t RingBufReadOne(tRingBufObject *psRingBuf);
extern void RingBufRead(tRingBufObject *psRingBuf, uint8_t *pui8Data,
                        uint32_t ui32Length);
extern void RingBufWriteOne(tRingBufObject *psRingBuf, uint8_t ui8Data);
extern void RingBufWrite(tRingBufObject *psRingBuf, uint8_t *pui8Data,
                         uint32_t ui32Length);
extern void RingBufAdvanceWrite(tRingBufObject *psRingBuf,
                                uint32_t ui32NumBytes);
extern void RingBufAdvanceRead(tRingBufObject *psRingBuf,
                                uint32_t ui32NumBytes);
extern void RingBufInit(tRingBufObject *psRingBuf, uint8_t *pui8Buf,
                        uint32_t ui32Size);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __RINGBUF_H__
