/*-----------------------------------------------------------------------*/
/* Dual-disk wrapper allowing operating of two different drives          */
/* underneath the FatFs layer without modification of the existing       */
/* single-unit drivers for those drives.                                 */
/*                                                                       */
/* This file was modified from a sample driver available from the FatFs  */
/* web site.                                                             */
/*-----------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "fatfs/src/diskio.h"
#include "fatfs/src/ff.h"

/*-----------------------------------------------------------------------*/
/* Configuration                                                         */
/*-----------------------------------------------------------------------*/
/* This wrapper allows two independent, low level, single drive FatFs    */
/* drivers to be used simultaneously to provide a FatFs implementation   */
/* with two physical drives.  Configuration is set using two #define     */
/* labels, one each from the following two groups, to indicate which     */
/* drivers to use for each of the logical drives supported.              */
/*                                                                       */
/* DISK0_EK_LM4F232H5QD - Logical disk 0 is an EK-LM4F232H5QD SD Card.   */
/* DISK0_USB_MSC        - Logical disk 0 is a USB Mass Storage Class     */
/*                        device.                                        */
/*                                                                       */
/* DISK1_EK_LM4F232H5QD - Logical disk 1 is an EK-LM4F232H5QD SD Card.   */
/* DISK1_USB_MSC        - Logical disk 1 is a USB Mass Storage Class     */
/*                        device.                                        */
/*                                                                       */
/* The same driver cannot be used to support both logical drives.        */
/*                                                                       */
/* Note that the USB MSC driver does not support a timer function so we  */
/* need to undef DRIVEn_TIMERPROC whenever this driver is configured.    */
/*-----------------------------------------------------------------------*/
#if defined(DISK0_EK_LM4F232H5QD)
#define DRIVE0_DRIVER "mmc-ek-lm4f232.c"
#define DRIVE0_TIMERPROC disk0_timerproc
#elif defined(DISK0_DK_TM4C123G)
#define DRIVE0_DRIVER "mmc-dk-tm4c123g.c"
#define DRIVE0_TIMERPROC disk0_timerproc
#elif defined(DISK0_DK_TM4C129XNCZAD)
#define DRIVE0_DRIVER "mmc-dk-tm4c129x.c"
#define DRIVE0_TIMERPROC disk0_timerproc
#elif defined(DISK0_USB_MSC)
#define DRIVE0_DRIVER "fat_usbmsc.c"
#undef DRIVE0_TIMERPROC
#else
#error "Unrecognized/undefined driver for DISK0!"
#endif

#if defined(DISK1_EK_LM4F232H5QD)
#define DRIVE1_DRIVER "mmc-ek-lm4f232.c"
#define DRIVE1_TIMERPROC disk1_timerproc
#elif defined(DISK1_DK_TM4C123G)
#define DRIVE1_DRIVER "mmc-dk-tm4c123g.c"
#define DRIVE1_TIMERPROC disk1_timerproc
#elif defined(DISK1_DK_TM4C129XNCZAD)
#define DRIVE1_DRIVER "mmc-dk-tm4c129x.c"
#define DRIVE1_TIMERPROC disk1_timerproc
#elif defined(DISK1_USB_MSC)
#define DRIVE1_DRIVER "fat_usbmsc.c"
#undef DRIVE0_TIMERPROC
#else
#error "Unrecognized/undefined driver for DISK1!"
#endif

/*-----------------------------------------------------------------------*/
/* The first low level driver                                            */
/*-----------------------------------------------------------------------*/
#define disk_initialize disk0_initialize
#define disk_status disk0_status
#define disk_read disk0_read
#define disk_write disk0_write
#define disk_ioctl disk0_ioctl
#define disk_timerproc disk0_timerproc
#define get_fattime disk0_get_fattime

#include DRIVE0_DRIVER

#undef disk_initialize
#undef disk_status
#undef disk_read
#undef disk_write
#undef disk_ioctl
#undef disk_timerproc
#undef get_fattime

/*-----------------------------------------------------------------------*/
/* The second low level driver                                           */
/*-----------------------------------------------------------------------*/
#define disk_initialize disk1_initialize
#define disk_status disk1_status
#define disk_read disk1_read
#define disk_write disk1_write
#define disk_ioctl disk1_ioctl
#define disk_timerproc disk1_timerproc
#define get_fattime disk1_get_fattime

#include DRIVE1_DRIVER

#undef disk_initialize
#undef disk_status
#undef disk_read
#undef disk_write
#undef disk_ioctl
#undef disk_timerproc
#undef get_fattime

/*-----------------------------------------------------------------------*/
/* Timer tick function.                                                  */
/*-----------------------------------------------------------------------*/
/* When using an SD card driver, this function must be called every 10mS */
/* by the application code. Note that this is not part of the device     */
/* driver interface that is called directly by FatFs.                    */
/*                                                                       */
void disk_timerproc (void)
{
#ifdef DRIVE0_TIMERPROC
    disk0_timerproc();
#endif

#ifdef DRIVE1_TIMERPROC
    disk1_timerproc();
#endif
}

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/
DSTATUS
disk_initialize(
    BYTE bValue)                /* Physical drive number */
{
    /* Depending upon the physical drive, call the relevant low level */
    /* driver. Note that we call each with physical drive number 0 since the */
    /* low level drivers typically expect this (they only support a single */
    /* drive). */
    if(bValue == 0)
    {
        return(disk0_initialize(0));
    }
    else
    {
        return(disk1_initialize(0));
    }
}

/*-----------------------------------------------------------------------*/
/* Returns the current status of a drive                                 */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
    BYTE drv)                   /* Physical drive number */
{
    /* Depending upon the physical drive, call the relevant low level */
    /* driver. Note that we call each with physical drive number 0 since the */
    /* low level drivers typically expect this (they only support a single */
    /* drive). */
    if(drv == 0)
    {
        return(disk0_status(0));
    }
    else
    {
        return(disk1_status(0));
    }

}

/*-----------------------------------------------------------------------*/
/* This function reads sector(s) from the disk drive                     */
/*-----------------------------------------------------------------------*/
DRESULT disk_read (
    BYTE drv,               /* Physical drive number */
    BYTE* buff,             /* Pointer to the data buffer to store read data */
    DWORD sector,           /* Sector number */
    BYTE count)             /* Sector count (1..255) */
{
    /* Depending upon the physical drive, call the relevant low level */
    /* driver. Note that we call each with physical drive number 0 since the */
    /* low level drivers typically expect this (they only support a single */
    /* drive). */
    if(drv == 0)
    {
        return(disk0_read(0, buff, sector, count));
    }
    else
    {
        return(disk1_read(0, buff, sector, count));
    }
}



/*-----------------------------------------------------------------------*/
/* This function writes sector(s) to the disk drive                     */
/*-----------------------------------------------------------------------*/
#if _READONLY == 0
DRESULT disk_write (
    BYTE ucDrive,           /* Physical drive number (0) */
    const BYTE* buff,       /* Pointer to the data to be written */
    DWORD sector,           /* Sector number */
    BYTE count)             /* Sector count (1..255) */
{
    /* Depending upon the physical drive, call the relevant low level */
    /* driver. Note that we call each with physical drive number 0 since the */
    /* low level drivers typically expect this (they only support a single */
    /* drive). */
    if(ucDrive == 0)
    {
        return(disk0_write(0, buff, sector, count));
    }
    else
    {
        return(disk1_write(0, buff, sector, count));
    }
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
    /* Depending upon the physical drive, call the relevant low level */
    /* driver. Note that we call each with physical drive number 0 since the */
    /* low level drivers typically expect this (they only support a single */
    /* drive). */
    if(drv == 0)
    {
        return(disk0_ioctl(0, ctrl, buff));
    }
    else
    {
        return(disk1_ioctl(0, ctrl, buff));
    }
}

/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module           */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support a real time clock.          */
/*                                                         */
/* Note that each driver implements the same function but, */
/* since the function doesn't take any parameter, we can't */
/* decide automatically which driver's get_fattime() to    */
/* invoke. Instead, we control this with a #define         */
/* which defaults to calling the first driver.             */
/*                                                         */
DWORD get_fattime (void)
{
#ifndef DRIVE1_TIME_MASTER
    return(disk0_get_fattime());
#else
    return(disk1_get_fattime());
#endif
}
