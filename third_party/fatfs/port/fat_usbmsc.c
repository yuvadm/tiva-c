/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
/* TivaWare USB MSC module                                               */
/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/

/*
 * This file was modified from a sample available from the FatFs
 * web site. It was modified to work with the TivaWare USB Library.
 */
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "usblib/usblib.h"
#include "usblib/usbmsc.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhmsc.h"
#include "fatfs/src/diskio.h"
#include "fatfs/src/ff.h"

extern tUSBHMSCInstance *g_psMSCInstance;

static volatile
DSTATUS USBStat = STA_NOINIT;    /* Disk status */

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS
disk_initialize(
    BYTE bValue)                /* Physical drive number (0) */
{
    /* Set the not initialized flag again. If all goes well and the disk is */
    /* present, this will be cleared at the end of the function.            */
    USBStat |= STA_NOINIT;

    /* Find out if drive is ready yet. */
    if (USBHMSCDriveReady(g_psMSCInstance)) return(FR_NOT_READY);

    /* Clear the not init flag. */
    USBStat &= ~STA_NOINIT;

    return 0;
}



/*-----------------------------------------------------------------------*/
/* Returns the current status of a drive                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
    BYTE drv)                   /* Physical drive number (0) */
{
    if (drv) return STA_NOINIT;        /* Supports only single drive */
    return USBStat;
}



/*-----------------------------------------------------------------------*/
/* This function reads sector(s) from the disk drive                     */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
    BYTE drv,               /* Physical drive number (0) */
    BYTE* buff,             /* Pointer to the data buffer to store read data */
    DWORD sector,           /* Physical drive nmuber (0) */
    BYTE count)             /* Sector count (1..255) */
{
    if(USBStat & STA_NOINIT)
    {
        return(RES_NOTRDY);
    }

    /* READ BLOCK */
    if (USBHMSCBlockRead(g_psMSCInstance, sector, buff, count) == 0)
        return RES_OK;

    return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* This function writes sector(s) to the disk drive                     */
/*-----------------------------------------------------------------------*/

#if _READONLY == 0
DRESULT disk_write (
    BYTE ucDrive,           /* Physical drive number (0) */
    const BYTE* buff,       /* Pointer to the data to be written */
    DWORD sector,           /* Start sector number (LBA) */
    BYTE count)             /* Sector count (1..255) */
{
    if (ucDrive || !count) return RES_PARERR;
    if (USBStat & STA_NOINIT) return RES_NOTRDY;
    if (USBStat & STA_PROTECT) return RES_WRPRT;

    /* WRITE BLOCK */
    if(USBHMSCBlockWrite(g_psMSCInstance, sector, (unsigned char *)buff,
                         count) == 0)
        return RES_OK;

    return RES_ERROR;
}
#endif /* _READONLY */

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
    BYTE drv,               /* Physical drive number (0) */
    BYTE ctrl,              /* Control code */
    void *buff)             /* Buffer to send/receive control data */
{
    if(USBStat & STA_NOINIT)
    {
        return(RES_NOTRDY);
    }

    switch(ctrl)
    {
        case CTRL_SYNC:
            return(RES_OK);

        default:
            return(RES_PARERR);
    }
}

/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module           */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support a real time clock.          */

DWORD get_fattime (void)
{
    return    ((2007UL-1980) << 25) // Year = 2007
            | (6UL << 21)           // Month = June
            | (5UL << 16)           // Day = 5
            | (11U << 11)           // Hour = 11
            | (38U << 5)            // Min = 38
            | (0U >> 1)             // Sec = 0
            ;
}
