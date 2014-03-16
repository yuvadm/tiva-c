//*****************************************************************************
//
// mpu_fault.c - MPU example.
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
// This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.h"
#include "driverlib/mpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "drivers/pinout.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>MPU (mpu_fault)</h1>
//!
//! This example application demonstrates the use of the MPU to protect a
//! region of memory from access, and to generate a memory management fault
//! when there is an access violation.
//!
//! UART0, connected to the ICDI virtual COM port and running at 115,200,
//! 8-N-1, is used to display messages from this application.
//!
//*****************************************************************************


//*****************************************************************************
//
// Variables to hold the state of the fault status when the fault occurs and
// the faulting address.
//
//*****************************************************************************
static volatile uint32_t g_ui32MMAR;
static volatile uint32_t g_ui32FaultStatus;

//*****************************************************************************
//
// A counter to track the number of times the fault handler has been entered.
//
//*****************************************************************************
static volatile uint32_t g_ui32MPUFaultCount;

//*****************************************************************************
//
// A location for storing data read from various addresses.  Volatile forces
// the compiler to use it and not optimize the access away.
//
//*****************************************************************************
static volatile uint32_t g_ui32Value;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// A simple function to perform a write using a 16-bit instruction, allowing
// for easy fix-up in the mpu fault handler.
//
//*****************************************************************************
#if defined(gcc) || defined(sourcerygxx)
void __attribute__((naked))
Write(uint32_t ui32Addr, uint32_t ui32Data)
{
    __asm("    str     r1, [r0]\n"
          "    bx      lr");
}

uint32_t __attribute__((naked))
Read(uint32_t ui32Offset)
{
    __asm("    ldr     r0, [r0]\n"
          "    bx      lr");
}
#endif

#if defined(rvmdk)
__asm void
Write(uint32_t ui32Addr, uint32_t ui32Data)
{
    str  r1, [r0];
    bx   lr;
}

__asm uint32_t
Read(uint32_t ui32Offset)
{
    ldr  r0, [r0];
    bx   lr;
}
#endif

#if defined(ewarm)
void
Write(uint32_t ui32Addr, uint32_t ui32Data)
{
    __asm("str  r1, [r0]\n"
          "bx   lr");
}

uint32_t
Read(uint32_t ui32Offset)
{
    __asm("ldr  r0, [r0]\n"
          "bx   lr\n");
#pragma diag_suppress=Pe940
}
#pragma diag_default=Pe940
#endif

#if defined(ccs)
__asm("    .sect \".text\"\n"
      "    .clink\n"
      "    .thumbfunc Write\n"
      "    .thumb\n"
      "    .global Write\n"
      "Write:\n"
      "    str     r1, [r0]\n"
      "    bx      lr\n");

__asm("    .sect \".text\"\n"
      "    .clink\n"
      "    .thumbfunc Read\n"
      "    .thumb\n"
      "    .global Read\n"
      "Read:\n"
      "    ldr     r0, [r0]\n"
      "    bx      lr\n");
void Write(uint32_t ui32Addr, uint32_t ui32Data);
uint32_t Read(uint32_t ui32Offset);
#endif

//*****************************************************************************
//
// The exception handler for memory management faults, which are caused by MPU
// access violations.  This handler will verify the cause of the fault and
// clear the NVIC fault status register.
//
//*****************************************************************************
void
MPUFaultHandler(void)
{
    //
    // Preserve the value of the MMAR (the address causing the fault).
    // Preserve the fault status register value, then clear it.
    //
    g_ui32MMAR = HWREG(NVIC_MM_ADDR);
    g_ui32FaultStatus = HWREG(NVIC_FAULT_STAT);
    HWREG(NVIC_FAULT_STAT) = g_ui32FaultStatus;

    //
    // Increment a counter to indicate the fault occurred.
    //
    g_ui32MPUFaultCount++;

    //
    // How the MPU fault is handled is application dependent...
    // In this sample code, we are skipping the faulted instruction and
    // continuing through the application. Since we are forcing the Read
    // and Write function to use 16bits instructions only, it is safe to
    // add 2 to the faulted instruction address to get the next
    // instruction address.
    //
    #if defined(rvmdk)
    {
        uint32_t puiFaultPC;

        puiFaultPC = __current_sp() + 0x18;
        *(uint32_t*)puiFaultPC = *(uint32_t*)puiFaultPC + 2;
    }
    #else
    __asm("    mov     r0, sp\n"
          "    ldr     r1, [r0, #0x18]\n"
          "    adds    r1, #2\n"
          "    str     r1, [r0, #0x18]");
    #endif
}

//*****************************************************************************
//
// This example demonstrates how to configure MPU regions for different levels
// of memory protection.  The following memory map is set up:
//
// 0000.0000 - 0000.7000 - rgn 0: executable read-only, flash
// 0000.7000 - 0000.8000 - rgn 0: no access, flash (disabled sub-region 7)
// 2000.0000 - 2000.8000 - rgn 1: read-write, RAM
// 2000.8000 - 2000.A000 - rgn 2: read-only, RAM (disabled sub-rgn 4 of rgn 1)
// 2000.A000 - 2000.FFFF - rgn 1: read-write, RAM
// 4000.0000 - 4001.0000 - rgn 3: read-write, peripherals
// 4001.0000 - 4002.0000 - rgn 3: no access (disabled sub-region 1)
// 4002.0000 - 4006.0000 - rgn 3: read-write, peripherals
// 4006.0000 - 4008.0000 - rgn 3: no access (disabled sub-region 6, 7)
// 4400.0000 - 4403.0000 - rgn 4: no access (disabled sub-region 0, 1, 2)
// 4403.0000 - 4404.0000 - rgn 4: read-write, peripherals (sub-region 3)
// 4404.0000 - 4405.0000 - rgn 4: no access (disabled sub-region 4)
// 4405.0000 - 4406.0000 - rgn 4: read-write, peripherals (sub-region 5)
// 4406.0000 - 4408.0000 - rgn 4: no access (disabled sub-region 6, 7)
// E000.E000 - E000.F000 - rgn 5: read-write, NVIC
// 2003.8000 - 2003.FFFF - rgn 6: read-write, upper 32K RAM
// 0100.0000 - 0100.FFFF - rgn 7: executable read-only, ROM
//
// The example code will attempt to perform the following operations and check
// the faulting behavior:
//
// - write to flash                         (should fault)
// - read from the disabled area of flash   (should fault)
// - read from the read-only area of RAM    (should not fault)
// - write to the read-only section of RAM  (should fault)
//
//*****************************************************************************
int
main(void)
{
    bool bFail = 0;
    uint32_t ui32SysClock;

    //
    // Run from the PLL at 120 MHz.
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN |
                                           SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet(false, false);

    //
    // Initialize the UART.
    //
    UARTStdioConfig(0, 115200, ui32SysClock);

    //
    // Clear the terminal and print banner.
    //
    UARTprintf("\033[2J\033[H");
    UARTprintf("MPU Fault example ...\n\n");

    //
    // Configure an executable, read-only MPU region for flash.  It is a 32 KB
    // region with the last 4 KB disabled to result in a 28 KB executable
    // region.  This region is needed so that the program can execute from
    // flash.
    //
    ROM_MPURegionSet(0, FLASH_BASE,
                     MPU_RGN_SIZE_32K | MPU_RGN_PERM_EXEC |
                     MPU_RGN_PERM_PRV_RO_USR_RO | MPU_SUB_RGN_DISABLE_7 |
                     MPU_RGN_ENABLE);

    //
    // Configure a read-write MPU region for RAM.  It is a 64 KB region.  There
    // is a 8 KB sub-region in the middle that is disabled in order to open up
    // a hole in which different permissions can be applied.
    //
    ROM_MPURegionSet(1, SRAM_BASE,
                     MPU_RGN_SIZE_64K | MPU_RGN_PERM_NOEXEC |
                     MPU_RGN_PERM_PRV_RW_USR_RW | MPU_SUB_RGN_DISABLE_4 |
                     MPU_RGN_ENABLE);
    //
    // Configure a read-only MPU region for the 8 KB of RAM that is disabled in
    // the previous region.  This region is used for demonstrating read-only
    // permissions.
    //
    ROM_MPURegionSet(2, SRAM_BASE + 0x8000,
                     MPU_RGN_SIZE_8K | MPU_RGN_PERM_NOEXEC |
                     MPU_RGN_PERM_PRV_RO_USR_RO | MPU_RGN_ENABLE);

    //
    // Configure a read-write MPU region for peripherals.  The region is 512 KB
    // total size, with several sub-regions disabled to prevent access to areas
    // where there are no peripherals.  This region is needed because the
    // program needs access to some peripherals.
    //
    ROM_MPURegionSet(3, 0x40000000,
                     MPU_RGN_SIZE_512K | MPU_RGN_PERM_NOEXEC |
                     MPU_RGN_PERM_PRV_RW_USR_RW | MPU_SUB_RGN_DISABLE_1 |
                     MPU_SUB_RGN_DISABLE_6 | MPU_SUB_RGN_DISABLE_7 |
                     MPU_RGN_ENABLE);

    //
    // Configure a read-write MPU region for peripherals.  The region is 512 KB
    // total size, with several sub-regions disabled to prevent access to areas
    // where there are no peripherals.  This region is needed because the
    // program needs access to some peripherals.
    //
    ROM_MPURegionSet(4, 0x44000000,
                     MPU_RGN_SIZE_512K | MPU_RGN_PERM_NOEXEC |
                     MPU_RGN_PERM_PRV_RW_USR_RW | MPU_SUB_RGN_DISABLE_0 |
                     MPU_SUB_RGN_DISABLE_1 | MPU_SUB_RGN_DISABLE_2 |
                     MPU_SUB_RGN_DISABLE_4 | MPU_SUB_RGN_DISABLE_6 |
                     MPU_SUB_RGN_DISABLE_7 |
                     MPU_RGN_ENABLE);

    //
    // Configure a read-write MPU region for access to the NVIC.  The region is
    // 4 KB in size.  This region is needed because NVIC registers are needed
    // in order to control the MPU.
    //
    ROM_MPURegionSet(5, NVIC_BASE,
                     MPU_RGN_SIZE_4K | MPU_RGN_PERM_NOEXEC |
                     MPU_RGN_PERM_PRV_RW_USR_RW | MPU_RGN_ENABLE);

    //
    // Configure a read-write MPU region for the top 32KB of RAM.
    // This region is used as stack for Sourcery CodeBench.
    //
    ROM_MPURegionSet(6, SRAM_BASE + (ROM_SysCtlSRAMSizeGet() - (32 * 1024)),
                     MPU_RGN_SIZE_32K | MPU_RGN_PERM_NOEXEC |
                     MPU_RGN_PERM_PRV_RW_USR_RW | MPU_RGN_ENABLE);

    //
    // Configure an executable, read-only MPU region for ROM.  It is a 64 KB
    // region.  This region is needed so that ROM library calls work.
    //
    ROM_MPURegionSet(7, 0x01000000,
                     MPU_RGN_SIZE_64K | MPU_RGN_PERM_EXEC |
                     MPU_RGN_PERM_PRV_RO_USR_RO | MPU_RGN_ENABLE);

    //
    // Need to clear the NVIC fault status register to make sure there is no
    // status hanging around from a previous program.
    //
    g_ui32FaultStatus = HWREG(NVIC_FAULT_STAT);
    HWREG(NVIC_FAULT_STAT) = g_ui32FaultStatus;

    //
    // Enable the MPU fault.
    //
    ROM_IntEnable(FAULT_MPU);

    //
    // Enable the MPU.  This will begin to enforce the memory protection
    // regions.  The MPU is configured so that when in the hard fault or NMI
    // exceptions, a default map will be used.  Neither of these should occur
    // in this example program.
    //
    ROM_MPUEnable(MPU_CONFIG_HARDFLT_NMI);

    //
    // Attempt to write to the flash.  This should cause a protection fault due
    // to the fact that this region is read-only.
    //
    UARTprintf("Check flash write\n");
    g_ui32MPUFaultCount = 0;
    Write(0x100, 0x12345678);

    //
    // Verify that the fault occurred, at the expected address.
    //
    if((g_ui32MPUFaultCount == 1) && (g_ui32FaultStatus == 0x82) &&
       (g_ui32MMAR == 0x100))
    {
        UARTprintf("OK\n");
    }
    else
    {
        bFail = 1;
        UARTprintf("NOK\n");
    }

    //
    // Attempt to read from the disabled section of flash, the upper 4 KB of
    // the 32 KB region.
    //
    UARTprintf("Check flash read\n");
    g_ui32MPUFaultCount = 0;
    g_ui32Value = Read(0x7820);

    //
    // Verify that the fault occurred, at the expected address.
    //
    if((g_ui32MPUFaultCount == 1) && (g_ui32FaultStatus == 0x82) &&
       (g_ui32MMAR == 0x7820))
    {
        UARTprintf("OK\n");
    }
    else
    {
        bFail = 1;
        UARTprintf("NOK\n");
    }

    //
    // Attempt to read from the read-only area of RAM, the middle 8 KB of the
    // 64 KB region.
    //
    UARTprintf("Check RAM read\n");
    g_ui32MPUFaultCount = 0;
    g_ui32Value = Read(0x20008440);

    //
    // Verify that the RAM read did not cause a fault.
    //
    if(g_ui32MPUFaultCount == 0)
    {
        UARTprintf("OK\n");
    }
    else
    {
        bFail = 1;
        UARTprintf("NOK\n");
    }

    //
    // Attempt to write to the read-only area of RAM, the middle 8 KB of the
    // 64 KB region.
    //
    UARTprintf("Check RAM write\n");
    g_ui32MPUFaultCount = 0;
    Write(0x20008460, 0xabcdef00);

    //
    // Verify that the RAM write caused a fault.
    //
    if((g_ui32MPUFaultCount == 1) && (g_ui32FaultStatus == 0x82) &&
       (g_ui32MMAR == 0x20008460))
    {
        UARTprintf("OK\n");
    }
    else
    {
        bFail = 1;
        UARTprintf("NOK\n");
    }

    //
    // Display the results of the example program.
    //
    if(bFail)
    {
        UARTprintf("Failure\n");
    }
    else
    {
        UARTprintf("Success!\n");
    }

    //
    // Disable the MPU, so there are no lingering side effects if another
    // program is run.
    //
    ROM_MPUDisable();

    //
    // Loop forever.
    //
    while(1)
    {
    }
}
