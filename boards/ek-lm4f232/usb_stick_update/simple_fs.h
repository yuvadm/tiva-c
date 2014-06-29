//*****************************************************************************
//
// simple_fs.h - Header for simple FAT file system support
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
// This is part of revision 2.1.0.12573 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#ifndef __SIMPLE_FS_H__
#define __SIMPLE_FS_H__

//*****************************************************************************
//
// \addtogroup simple_fs_api
// @{
//
//*****************************************************************************

//*****************************************************************************
//
// Macro to read a single sector from a file that was opened with
// SimpleFsOpen()
//
// This convenience macro maps to the function SimpleFsGetNextFileSector()
// called with a parameter of 0.  It should be used to read successive
// sectors from a file after the file has been opened with SimpleFsOpen().
//
// When a sector is read, it is loaded into the sector buffer that was passed
// when SimpleFsInit() was called.
//
// A non-zero value will be returned to the caller as int32_t as successive
// sectors are successfully read into the sector buffer.  At the end of the
// file, or if there is any error, then a value of 0 is returned.
//
// Note that the a whole sector is always loaded, even if the end of
// a file does not fill the last sector.  It is the responsibility of the
// caller to track the file size and to deal with a partially full last
// sector.
//
// \return Non-zero if a sector was read into the sector buffer, or
// 0 if there are no more sectors or if any error occurred.
//
//*****************************************************************************
#define SimpleFsReadFileSector() SimpleFsGetNextFileSector(0)

//*****************************************************************************
//
// Read a single sector from the application-specific storage device into
// the sector buffer.
//
// \param ui32Sector is the absolute sector number to read from the storage
// device
// \param pui8SectorBuf is a pointer to a 512 byte buffer where the sector
// data should be written
//
// This function is used by the simple file system functions to read a sector
// of data from a storage device.  It must be implemented as part of the
// application specific code.  For example, it could be used to read sectors
// from a USB mass storage device, or from an SD card, or any device that can
// be used to store a FAT file system.  Note that the sector size is always
// assumed to be 512 bytes.
//
// \return Zero value if a sector of data was successfully read from the
// device and stored in the sector buffer, non-zero if not successful.
//
//*****************************************************************************
//
// This function to be supplied by the client
//
extern uint32_t SimpleFsReadMediaSector(uint_fast32_t ui32Sector,
                                        uint8_t *pui8SectorBuf);

//*****************************************************************************
//
// Close the Doxygen group.
// @}
//
//*****************************************************************************

//*****************************************************************************
//
// Functions to help with accessing the FAT file system on a storage device
//
//****************************************************************************
extern uint32_t SimpleFsInit(uint8_t *pui8SectorBuf);
extern uint32_t SimpleFsOpen(char *pcName83);
extern uint32_t SimpleFsGetNextFileSector(uint_fast32_t ui32StartCluster);

#endif // __SIMPLE_FS_H__
