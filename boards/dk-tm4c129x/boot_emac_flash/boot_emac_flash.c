//*****************************************************************************
//
// boot_emac_flash.c - Ethernet flash-based boot loader description.
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

//
// This file is just a stub to hold the following doc comments
//

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Ethernet flash-based Boot Loader (boot_emac_flash)</h1>
//!
//! The boot loader is a small piece of code that can be programmed at the
//! beginning of flash to act as an application loader as well as an update
//! mechanism for an application running on a Tiva microcontroller,
//! utilizing either UART0, USB, or Ethernet.  The capabilities of the
//! boot loader are configured via the bl_config.h include file.  For this
//! example, the boot loader uses Ethernet to load an application.
//!
//! The configuration is set to boot applications which are linked to run from
//! address 0x4000 in flash.  This is minimal address since the flash erase
//! size is 16K bytes.
//! 
//! Please note that LMFlash programmer version number needs to be at
//! least 1588 or later. Older LMFlash programmer doesn't work with this 
//! Ethernet boot loader.
//!
//
//*****************************************************************************


