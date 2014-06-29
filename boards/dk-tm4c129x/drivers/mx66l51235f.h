//*****************************************************************************
//
// mx66l51235f.h - Prototypes for the MX66L51235F driver.
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

#ifndef __DRIVERS_MX66L51235F_H__
#define __DRIVERS_MX66L51235F_H__

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
// The memory size and block size in bytes.
//
//*****************************************************************************
#define MX66L51235F_MEMORY_SIZE 0x04000000
#define MX66L51235F_BLOCK_SIZE  0x1000

//*****************************************************************************
//
// Prototypes.
//
//*****************************************************************************
extern void MX66L51235FInit(uint32_t ui32SysClock);
extern void MX66L51235FSectorErase(uint32_t ui32Addr);
extern void MX66L51235FBlockErase32(uint32_t ui32Addr);
extern void MX66L51235FBlockErase64(uint32_t ui32Addr);
extern void MX66L51235FChipErase(void);
extern void MX66L51235FPageProgram(uint32_t ui32Addr, const uint8_t *pui8Data,
                                   uint32_t ui32Count);
extern void MX66L51235FRead(uint32_t ui32Addr, uint8_t *pui8Data,
                            uint32_t ui32Count);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __DRIVERS_MX66L51235F_H__
