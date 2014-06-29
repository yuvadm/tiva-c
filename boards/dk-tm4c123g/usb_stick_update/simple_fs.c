//*****************************************************************************
//
// simple_fs.c - Functions for simple FAT file system support
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
// This is part of revision 2.1.0.12573 of the DK-TM4C123G Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "simple_fs.h"

//*****************************************************************************
//
// \addtogroup simple_fs_api
// @{
//
// This file system API should be used as follows:
// - Initialize it by calling SimpleFsInit().  You must supply a pointer to a
// 512 byte buffer that will be used for storing device sector data.
// - "Open" a file by calling SimpleFsOpen() and passing the 8.3-style filename
// as an 11-character string.
// - Read successive sectors from the file by using the convenience macro
// SimpleFsReadFileSector().
//
// This API does not use any file handles so there is no way to open more than
// one file at a time.  There is also no random access into the file, each
// sector must be read in sequence.
//
// The client of this API supplies a 512-byte buffer for storage of data read
// from the device.  But this file also maintains an additional, internal
// 512-byte buffer used for caching FAT sectors.  This minimizes the amount
// of device reads required to fetch cluster chain entries from the FAT.
//
// The application code (the client) must also provide a function used for
// reading sectors from the storage device, whatever it may be.  This allows
// the code in this file to be independent of the type of device used for
// storing the file system.  The name of the function is
// SimpleFsReadMediaSector().
//
//*****************************************************************************

//*****************************************************************************
//
// Setup a macro for handling packed data structures.
//
//*****************************************************************************
#if defined(ccs) ||                                                           \
    defined(codered) ||                                                       \
    defined(gcc) ||                                                           \
    defined(rvmdk) ||                                                         \
    defined(__ARMCC_VERSION) ||                                               \
    defined(sourcerygxx)
#define PACKED                  __attribute__((packed))
#elif defined(ewarm)
#define PACKED
#else
#error "Unrecognized COMPILER!"
#endif

//*****************************************************************************
//
// Instruct the IAR compiler to pack the following structures.
//
//*****************************************************************************
#ifdef ewarm
#pragma pack(1)
#endif

//*****************************************************************************
//
// Structures for mapping FAT file system
//
//*****************************************************************************

//*****************************************************************************
//
// The FAT16 boot sector extension
//
//*****************************************************************************
typedef struct
{
    uint8_t ui8DriveNumber;
    uint8_t ui8Reserved;
    uint8_t ui8ExtSig;
    uint32_t ui32Serial;
    char pcVolumeLabel[11];
    char pcFsType[8];
    uint8_t ui8BootCode[448];
    uint16_t ui16Sig;
}
PACKED tBootExt16;

//*****************************************************************************
//
// The FAT32 boot sector extension
//
//*****************************************************************************
typedef struct
{
    uint32_t ui32SectorsPerFAT;
    uint16_t ui16Flags;
    uint16_t ui16Version;
    uint32_t ui32RootCluster;
    uint16_t ui16InfoSector;
    uint16_t ui16BootCopy;
    uint8_t ui8Reserved[12];
    uint8_t ui8DriveNumber;
    uint8_t ui8Reserved1;
    uint8_t ui8ExtSig;
    uint32_t ui32Serial;
    char pcVolumeLabel[11];
    char pcFsType[8];
    uint8_t ui8BootCode[420];
    uint16_t ui16Sig;
}
PACKED tBootExt32;

//*****************************************************************************
//
// The FAT16/32 boot sector main section
//
//*****************************************************************************
typedef struct
{
    uint8_t ui8Jump[3];
    uint8_t i8OEMName[8];
    uint16_t ui16BytesPerSector;
    uint8_t ui8SectorsPerCluster;
    uint16_t ui16ReservedSectors;
    uint8_t ui8NumFATs;
    uint16_t ui16NumRootEntries;
    uint16_t ui16TotalSectorsSmall;
    uint8_t ui8MediaDescriptor;
    uint16_t ui16SectorsPerFAT;
    uint16_t ui16SectorsPerTrack;
    uint16_t ui16NumberHeads;
    uint32_t ui32HiddenSectors;
    uint32_t ui32TotalSectorsBig;
    union
    {
        tBootExt16 sExt16;
        tBootExt32 sExt32;
    }
    PACKED ext;
}
PACKED tBootSector;

//*****************************************************************************
//
// The partition table
//
//*****************************************************************************
typedef struct
{
    uint8_t ui8Status;
    uint8_t ui8CHSFirst[3];
    uint8_t ui8Type;
    uint8_t ui8CHSLast[3];
    uint32_t ui32FirstSector;
    uint32_t ui32NumBlocks;
}
PACKED tPartitionTable;

//*****************************************************************************
//
// The master boot record (MBR)
//
//*****************************************************************************
typedef struct
{
    uint8_t ui8CodeArea[440];
    uint8_t ui8DiskSignature[4];
    uint8_t ui8Nulls[2];
    tPartitionTable sPartTable[4];
    uint16_t ui16Sig;
}
PACKED tMasterBootRecord;

//*****************************************************************************
//
// The structure for a single directory entry
//
//*****************************************************************************
typedef struct
{
    char    pcFileName[11];
    uint8_t ui8Attr;
    uint8_t ui8Reserved;
    uint8_t ui8CreateTime[5];
    uint8_t ui8LastDate[2];
    uint16_t ui16ClusterHi;
    uint8_t ui8LastModified[4];
    uint16_t ui16Cluster;
    uint32_t ui32FileSize;
}
PACKED tDirEntry;

//*****************************************************************************
//
// Tell the IAR compiler that the remaining structures do not need to be
// packed.
//
//*****************************************************************************
#ifdef ewarm
#pragma pack()
#endif

//*****************************************************************************
//
// This structure holds information about the layout of the file system
//
//*****************************************************************************
typedef struct
{
    uint32_t ui32FirstSector;
    uint32_t ui32NumBlocks;
    uint16_t ui16SectorsPerCluster;
    uint16_t ui16MaxRootEntries;
    uint32_t ui32SectorsPerFAT;
    uint32_t ui32FirstFATSector;
    uint32_t ui32LastFATSector;
    uint32_t ui32FirstDataSector;
    uint32_t ui32Type;
    uint32_t ui32StartRootDir;
}
tPartitionInfo;

static tPartitionInfo sPartInfo;

//*****************************************************************************
//
// A pointer to the client provided sector buffer.
//
//*****************************************************************************
static uint8_t *g_pui8SectorBuf;

//*****************************************************************************
//
// Initializes the simple file system
//
// \param pui8SectorBuf is a pointer to a caller supplied 512-byte buffer
// that will be used for holding sectors that are loaded from the media
// storage device.
//
// Reads the MBR, partition table, and boot record to find the logical
// structure of the file system.  This function stores the file system
// structural data internally so that the remaining functions of the API
// can read the file system.
//
// To read data from the storage device, the function SimpleFsReadMediaSector()
// will be called.  This function is not implemented here but must be
// implemented by the user of this simple file system.
//
// This file system support is extremely simple-minded.  It will only
// find the first partition of a FAT16 or FAT32 formatted mass storage
// device.  Only very minimal error checking is performed in order to save
// code space.
//
// \return Zero if successful, non-zero if there was an error.
//
//*****************************************************************************
uint32_t
SimpleFsInit(uint8_t *pui8SectorBuf)
{
    tMasterBootRecord *pMBR;
    tPartitionTable *pPart;
    tBootSector *pBoot;

    //
    // Save the sector buffer pointer.  The input parameter is assumed
    // to be good.
    //
    g_pui8SectorBuf = pui8SectorBuf;

    //
    // Get the MBR
    //
    if(SimpleFsReadMediaSector(0, pui8SectorBuf))
    {
        return(1);
    }

    //
    // Verify MBR signature - bare minimum validation of MBR.
    //
    pMBR = (tMasterBootRecord *)pui8SectorBuf;
    if(pMBR->ui16Sig != 0xAA55)
    {
        return(1);
    }

    //
    // See if this is a MBR or a boot sector.
    //
    pBoot = (tBootSector *)pui8SectorBuf;
    if((strncmp(pBoot->ext.sExt16.pcFsType, "FAT", 3) != 0) &&
       (strncmp(pBoot->ext.sExt32.pcFsType, "FAT32", 5) != 0))
    {
        //
        // Get the first partition table
        //
        pPart = &(pMBR->sPartTable[0]);

        //
        // Could optionally check partition type here ...
        //

        //
        // Get the partition location and size
        //
        sPartInfo.ui32FirstSector = pPart->ui32FirstSector;
        sPartInfo.ui32NumBlocks = pPart->ui32NumBlocks;

        //
        // Read the boot sector from the partition
        //
        if(SimpleFsReadMediaSector(sPartInfo.ui32FirstSector, pui8SectorBuf))
        {
            return(1);
        }
    }
    else
    {
        //
        // Extract the number of sectors from the boot sector.
        //
        sPartInfo.ui32FirstSector = 0;
        if(pBoot->ui16TotalSectorsSmall == 0)
        {
            sPartInfo.ui32NumBlocks = pBoot->ui32TotalSectorsBig;
        }
        else
        {
            sPartInfo.ui32NumBlocks = pBoot->ui16TotalSectorsSmall;
        }
    }

    //
    // Get pointer to the boot sector
    //
    if(pBoot->ext.sExt16.ui16Sig != 0xAA55)
    {
        return(1);
    }

    //
    // Verify the sector size is 512.  We can't deal with anything else
    //
    if(pBoot->ui16BytesPerSector != 512)
    {
        return(1);
    }

    //
    // Extract some info from the boot record
    //
    sPartInfo.ui16SectorsPerCluster = pBoot->ui8SectorsPerCluster;
    sPartInfo.ui16MaxRootEntries = pBoot->ui16NumRootEntries;

    //
    // Decide if we are dealing with FAT16 or FAT32.
    // If number of root entries is 0, that suggests FAT32
    //
    if(sPartInfo.ui16MaxRootEntries == 0)
    {
        //
        // Confirm FAT 32 signature in the expected place
        //
        if(!strncmp(pBoot->ext.sExt32.pcFsType, "FAT32   ", 8))
        {
            sPartInfo.ui32Type = 32;
        }
        else
        {
            return(1);
        }
    }
    //
    // Root entries is non-zero, suggests FAT16
    //
    else
    {
        //
        // Confirm FAT16 signature
        //
        if(!strncmp(pBoot->ext.sExt16.pcFsType, "FAT16   ", 8))
        {
            sPartInfo.ui32Type = 16;
        }
        else
        {
            return(1);
        }
    }

    //
    // Find the beginning of the FAT, in absolute sectors
    //
    sPartInfo.ui32FirstFATSector = sPartInfo.ui32FirstSector +
                                 pBoot->ui16ReservedSectors;

    //
    // Find the end of the FAT in absolute sectors.  FAT16 and 32
    // are handled differently.
    //
    sPartInfo.ui32SectorsPerFAT = (sPartInfo.ui32Type == 16) ?
                                pBoot->ui16SectorsPerFAT :
                                pBoot->ext.sExt32.ui32SectorsPerFAT;
    sPartInfo.ui32LastFATSector = sPartInfo.ui32FirstFATSector +
                                sPartInfo.ui32SectorsPerFAT - 1;

    //
    // Find the start of the root directory and the data area.
    // For FAT16, the root will be stored as an absolute sector number
    // For FAT32, the root will be stored as the starting cluster of the root
    // The data area start is the absolute first sector of the data area.
    //
    if(sPartInfo.ui32Type == 16)
    {
        sPartInfo.ui32StartRootDir = sPartInfo.ui32FirstFATSector +
                                   (sPartInfo.ui32SectorsPerFAT *
                                    pBoot->ui8NumFATs);
        sPartInfo.ui32FirstDataSector = sPartInfo.ui32StartRootDir +
                                      (sPartInfo.ui16MaxRootEntries / 16);
    }
    else
    {
        sPartInfo.ui32StartRootDir = pBoot->ext.sExt32.ui32RootCluster;
        sPartInfo.ui32FirstDataSector = sPartInfo.ui32FirstFATSector +
                            (sPartInfo.ui32SectorsPerFAT * pBoot->ui8NumFATs);
    }

    //
    // At this point the file system has been initialized, so return
    // success to the caller.
    //
    return(0);
}

//*****************************************************************************
//
// Find the next cluster in a FAT chain
//
// \param ui32ThisCluster is the current cluster in the chain
//
// Reads the File Allocation Table (FAT) of the file system to find the
// next cluster in a chain of clusters.  The current cluster is passed in
// and the next cluster in the chain will be returned.
//
// This function reads sectors from the storage device as needed in order
// to parse the FAT tables.  Error handling is minimal since there is not
// much that can be done if an error is encountered.  If any error is
// encountered, or if this is the last cluster in the chain, then 0 is
// returned.  This signals the caller to stop traversing the chain (either
// due to error or end of chain).
//
// The function maintains a cache of a single sector from the FAT.  It only
// reads in a new FAT sector if the requested cluster is not in the
// currently cached sector.
//
// \return Next cluster number if successful, 0 if this is the last cluster
// or any error is found.
//
//*****************************************************************************
static uint32_t
SimpleFsGetNextCluster(uint_fast32_t ui32ThisCluster)
{
    static uint8_t ui8FATCache[512];
    static uint_fast32_t ui32CachedFATSector = (uint32_t)-1;
    uint_fast32_t ui32ClustersPerFATSector;
    uint_fast32_t ui32ClusterIdx;
    uint_fast32_t ui32FATSector;
    uint_fast32_t ui32NextCluster;
    uint_fast32_t ui32MaxCluster;

    //
    // Compute the maximum possible reasonable cluster number
    //
    ui32MaxCluster = sPartInfo.ui32NumBlocks / sPartInfo.ui16SectorsPerCluster;

    //
    // Make sure cluster input number is reasonable.  If not then return
    // 0 indicating error.
    //
    if((ui32ThisCluster < 2) || (ui32ThisCluster > ui32MaxCluster))
    {
        return(0);
    }

    //
    // Compute the index of the requested cluster within the sector.
    // Also compute the sector number within the FAT that contains the
    // entry for the requested cluster.
    //
    ui32ClustersPerFATSector = (sPartInfo.ui32Type == 16) ? 256 : 128;
    ui32ClusterIdx = ui32ThisCluster % ui32ClustersPerFATSector;
    ui32FATSector = ui32ThisCluster / ui32ClustersPerFATSector;

    //
    // Check to see if the FAT sector we need is already cached
    //
    if(ui32FATSector != ui32CachedFATSector)
    {
        //
        // FAT sector we need is not cached, so read it in
        //
        if(SimpleFsReadMediaSector(sPartInfo.ui32FirstFATSector + ui32FATSector,
                                 ui8FATCache) != 0)
        {
            //
            // There was an error so mark cache as unavailable and return
            // an error.
            //
            ui32CachedFATSector = (uint32_t)-1;
            return(0);
        }

        //
        // Remember which FAT sector was just loaded into the cache.
        //
        ui32CachedFATSector = ui32FATSector;
    }

    //
    // Now look up the next cluster value from the cached sector, using this
    // requested cluster as an index. It needs to be indexed as 16 or 32
    // bit values depending on whether it is FAT16 or 32
    // If the cluster value means last cluster, then return 0
    //
    if(sPartInfo.ui32Type == 16)
    {
        ui32NextCluster = ((uint16_t *)ui8FATCache)[ui32ClusterIdx];
        if(ui32NextCluster >= 0xFFF8)
        {
            return(0);
        }
    }
    else
    {
        ui32NextCluster = ((uint32_t *)ui8FATCache)[ui32ClusterIdx];
        if(ui32NextCluster >= 0x0FFFFFF8)
        {
            return(0);
        }
    }

    //
    // Check new cluster value to make sure it is reasonable.  If not then
    // return 0 to indicate an error.
    //
    if((ui32NextCluster >= 2) && (ui32NextCluster <= ui32MaxCluster))
    {
        return(ui32NextCluster);
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
// Read a single sector from a file into the sector buffer
//
// \param ui32StartCluster is the first cluster of the file, used to
// initialize the file read.  Use 0 for successive sectors.
//
// Reads sectors in sequence from a file and stores the data in the sector
// buffer that was passed in the initial call to SimpleFsInit().  The function
// is initialized with the file to read by passing the starting cluster of
// the file.  The function will initialize some static data and return.  It
// does not read any file data when passed a starting cluster (and
// returns 0 - this is normal).
//
// Once the function has been initialized with the file's starting cluster,
// then successive calls should be made, passing a value of 0 for the
// cluster number.  This tells the function to read the next sector from the
// file and store it in the sector buffer.  The function remembers the last
// sector that was read, and each time it is called with a cluster value of
// 0, it will read the next sector.  The function will traverse the FAT
// chain as needed to read all the sectors.  When a sector has been
// successfully read from a file, the function will return non-zero.  When
// there are no more sectors to read, or any error is encountered, the
// function will return 0.
//
// Note that the function always reads a whole sector, even if the end of
// a file does not fill the last sector.  It is the responsibility of the
// caller to track the file size and to deal with a partially full last
// sector.
//
// \return Non-zero if a sector was read into the sector buffer, or
// 0 if there are no more sectors or if any error occurred.
//
//*****************************************************************************
uint32_t
SimpleFsGetNextFileSector(uint_fast32_t ui32StartCluster)
{
    static uint_fast32_t ui32WorkingCluster = 0;
    static uint_fast32_t ui32WorkingSector;
    uint_fast32_t ui32ReadSector;

    //
    // If user specified starting cluster, then init the working cluster
    // and sector values
    //
    if(ui32StartCluster)
    {
        ui32WorkingCluster = ui32StartCluster;
        ui32WorkingSector = 0;
        return(0);
    }

    //
    // Otherwise, make sure there is a valid working cluster already
    //
    else if(ui32WorkingCluster == 0)
    {
        return(0);
    }

    //
    // If the current working sector is the same as sectors per cluster,
    // then that means that the next cluster needs to be loaded.
    //
    if(ui32WorkingSector == sPartInfo.ui16SectorsPerCluster)
    {
        //
        // Get the next cluster in the chain for this file.
        //
        ui32WorkingCluster = SimpleFsGetNextCluster(ui32WorkingCluster);

        //
        // If the next cluster is valid, then reset the working sector
        //
        if(ui32WorkingCluster)
        {
            ui32WorkingSector = 0;
        }

        //
        // Next cluster is not valid, or this was the end of the chain.
        // Clear the working cluster and return an indication that no new
        // sector data was loaded.
        //
        else
        {
            ui32WorkingCluster = 0;
            return(0);
        }
    }

    //
    // Calculate the sector to read from.  It is the sector of the start
    // of the working cluster, plus the working sector (the sector within
    // the cluster), plus the offset to the start of the data area.
    // Note that the cluster needs to be reduced by 2 in order to index
    // properly into the data area.  That is a feature of FAT file system.
    //
    ui32ReadSector = (ui32WorkingCluster - 2) * sPartInfo.ui16SectorsPerCluster;
    ui32ReadSector += ui32WorkingSector;
    ui32ReadSector += sPartInfo.ui32FirstDataSector;

    //
    // Attempt to read the next sector from the cluster.  If not successful,
    // then clear the working cluster and return a non-success indication.
    //
    if(SimpleFsReadMediaSector(ui32ReadSector, g_pui8SectorBuf) != 0)
    {
        ui32WorkingCluster = 0;
        return(0);
    }
    else
    {
        //
        // Read was successful.  Increment to the next sector of the cluster
        // and return a success indication.
        //
        ui32WorkingSector++;
        return(1);
    }
}

//*****************************************************************************
//
// Find a file in the root directory of the file system and open it for
// reading.
//
// \param pcName83 is an 11-character string that represents the 8.3 file
// name of the file to open.
//
// This function traverses the root directory of the file system to find
// the file name specified by the caller.  Note that the file name must be
// an 8.3 file name that is 11 characters int32_t.  The first 8 characters are
// the base name and the last 3 characters are the extension.  If there are
// fewer characters in the base name or extension, the name should be padded
// with spaces.  For example "myfile.bn" has fewer than 11 characters, and
// should be passed with padding like this: "myfile  bn ".  Note the extra
// spaces, and that the dot ('.') is not part of the string that is passed
// to this function.
//
// If the file is found, then it initializes the file for reading, and returns
// the file length.  The file can be read by making successive calls to
// SimpleFsReadFileSector().
//
// The function only searches the root directory and ignores any
// subdirectories.  It also ignores any int32_t file name entries, looking only
// at the 8.3 file name for a match.
//
// \return The size of the file if it is found, or 0 if the file could not
// be found.
//
//*****************************************************************************
uint32_t
SimpleFsOpen(char *pcName83)
{
    tDirEntry *pDirEntry;
    uint_fast32_t ui32DirSector;
    uint_fast32_t ui32FirstCluster;

    //
    // Find starting root dir sector, only used for FAT16
    // If FAT32 then this is the first cluster of root dir
    //
    ui32DirSector = sPartInfo.ui32StartRootDir;

    //
    // For FAT32, root dir is like a file, so init a file read of the root dir
    //
    if(sPartInfo.ui32Type == 32)
    {
        SimpleFsGetNextFileSector(ui32DirSector);
    }

    //
    // Search the root directory entry for the firmware file
    //
    while(1)
    {
        //
        // Read in a directory block.
        //
        if(sPartInfo.ui32Type == 16)
        {
            //
            // For FAT16, read in a sector of the root directory
            //
            if(SimpleFsReadMediaSector(ui32DirSector, g_pui8SectorBuf))
            {
                return(0);
            }
        }
        else
        {
            //
            // For FAT32, the root directory is treated like a file.
            // The root directory sector will be loaded into the sector buf
            //
            if(SimpleFsGetNextFileSector(0) == 0)
            {
                return(0);
            }
        }

        //
        // Initialize the directory entry pointer to the first entry of
        // this sector.
        //
        pDirEntry = (tDirEntry *)g_pui8SectorBuf;

        //
        // Iterate through all the directory entries in this sector
        //
        while((uint8_t *)pDirEntry < &g_pui8SectorBuf[512])
        {
            //
            // If the 8.3 filename of this entry matches the firmware
            // file name, then we have a match, so return a pointer to
            // this entry.
            //
            if(!strncmp(pDirEntry->pcFileName, pcName83, 11))
            {
                //
                // Compute the starting cluster of the file
                //
                ui32FirstCluster = pDirEntry->ui16Cluster;
                if(sPartInfo.ui32Type == 32)
                {
                    //
                    // For FAT32, add in the upper word of the
                    // starting cluster number
                    //
                    ui32FirstCluster += pDirEntry->ui16ClusterHi << 16;
                }

                //
                // Initialize the start of the file
                //
                SimpleFsGetNextFileSector(ui32FirstCluster);
                return(pDirEntry->ui32FileSize);
            }

            //
            // Advance to the next entry in this sector.
            //
            pDirEntry++;
        }

        //
        // Need to get the next sector in the directory.  Handled
        // differently depending on if this is FAT16 or 32
        //
        if(sPartInfo.ui32Type == 16)
        {
            //
            // FAT16: advance sectors as int32_t as there are more possible
            // entries.
            //
            sPartInfo.ui16MaxRootEntries -= 512 / 32;
            if(sPartInfo.ui16MaxRootEntries)
            {
                ui32DirSector++;
            }
            else
            {
                //
                // Ran out of directory entries and didn't find the file,
                // so return a null.
                //
                return(0);
            }
        }
        else
        {
            //
            // FAT32: there is nothing to compute here.  The next root
            // dir sector will be fetched at the top of the loop
            //
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
// @}
//
//*****************************************************************************
