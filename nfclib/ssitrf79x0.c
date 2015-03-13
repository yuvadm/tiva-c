//*****************************************************************************
//
// ssitrf79x0.c - SSI Driver for the TI TRF79x0 on the dk-lm3s9b96 board.
//
// Copyright (c) 2010-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the Tiva Firmware Development Package.
//
//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "ssitrf79x0.h"
#include "trf79x0.h"
#include "trf79x0_hw.h"

//*****************************************************************************
//
// Raw SPI through SSI access API for the TRF79x0.  Most user code will not and
// should not call these functions but instead use the provided higher level
// functions in trf79x0.c, directmode.c and iso14443a.c.
//
//*****************************************************************************
//*****************************************************************************
//
// Global that holds the clock speed of the MicroController in Hz.
//
//*****************************************************************************
extern uint32_t g_ui32SysClk;

//*****************************************************************************
//
// The rate of the SSI clock and derived values.
//
//*****************************************************************************
#define SSI_CLKS_PER_MS         (SSI_CLK_RATE / 1000)
#define STATUS_READS_PER_MS     (SSI_CLKS_PER_MS / 16)
#define SSI_NO_DATA              0

//*****************************************************************************
//
// Internal helper function that sends a buffer of data to the TRF79x0, used
// by all the functions that need to send bytes.
//
//*****************************************************************************
void
SSITRF79x0GenericWrite(unsigned char const *pucBuffer, unsigned int uiLength)
{
    uint32_t ulDummyData;

    while(uiLength > 0)
    {
        //
        // Write address/command/data and clear SSI register of dummy data.
        //
        MAP_SSIDataPut(TRF79X0_SSI_BASE, (unsigned long)*pucBuffer);
        //
        // Wait until the SSI Module is completed sending uiLength bytes to the SSI module.
        //
        while(SSIBusy(TRF79X0_SSI_BASE) == true);
        MAP_SSIDataGet(TRF79X0_SSI_BASE, &ulDummyData);

        //
        // Post increment counters.
        //
        pucBuffer++;
        uiLength--;
    }


}

//*****************************************************************************
//
// Internal helper function used by all the functions that need to send bytes.
//
//*****************************************************************************
void
SSITRF79x0DummyWrite(unsigned char const *pucBuffer, unsigned int uiLength)
{
    uint32_t ulDummyData;

    while(uiLength > 0)
    {
        //
        // Write address/command/data and clear SSI register of dummy data.
        //
        SSIDataPut(TRF79X0_SSI_BASE, (unsigned long)*pucBuffer);
        SSIDataGet(TRF79X0_SSI_BASE, &ulDummyData);

        //
        // Post increment counters.
        //
        pucBuffer++;
        uiLength--;
    }
}

//*****************************************************************************
//
// Internal helper function that receives a buffer of data from the TRF79x0,
// used by all the functions that need to read bytes.
//
//*****************************************************************************
static void
SSITRF79x0GenericRead(unsigned char *pucBuffer, unsigned int uiLength)
{
    uint32_t ulData;

    while(uiLength > 0)
    {
        //
        // Write dummy data for SSI clock and read data from SSI register.
        //
        MAP_SSIDataPut(TRF79X0_SSI_BASE, (unsigned long)SSI_NO_DATA);
        //
        // Wait until the SSI Module is completed sending uiLength bytes to the SSI module.
        //
        while(SSIBusy(TRF79X0_SSI_BASE) == true);
        MAP_SSIDataGet(TRF79X0_SSI_BASE, &ulData);
//        SSIDataGet(TRF79X0_SSI_BASE, &ulData);

        //
        // Read data into buffers and post increment counters.
        //
        *pucBuffer++ = (unsigned char)ulData;

        uiLength--;
    }

}

//*****************************************************************************
//
// Asserts the chip select for the TRF79x0.
//
//*****************************************************************************
void
SSITRF79x0ChipSelectAssert(void)
{
    //
    // Disable the interrupt associated with the TRF79x0.
    //
    TRF79x0InterruptDisable();

    //
    // Assert the chip select for TRF79x0.
    //
    MAP_GPIOPinWrite(TRF79X0_CS_BASE, TRF79X0_CS_PIN, 0);
}

//*****************************************************************************
//
// Deasserts the chip select for the TRF79x0
//
//*****************************************************************************
void
SSITRF79x0ChipSelectDeAssert(void)
{
    //
    // Deassert the chip select for the TRF79x0.
    //
    MAP_GPIOPinWrite(TRF79X0_CS_BASE, TRF79X0_CS_PIN, TRF79X0_CS_PIN);

    //
    // Enable interrupt associated with the TRF79x0.
    //
    TRF79x0InterruptEnable();
}

//*****************************************************************************
//
// Initializes the SSI port and determines if the TRF79x0 is available.
//
// This function must be called prior to any other function offered by the
// TRF79x0.  It configures the SSI port to run in Motorola/Freescale
// mode.
//
// \return None.
//
//*****************************************************************************
void
SSITRF79x0Init(void)
{
    //
    // Enable the peripherals used to drive the TRF79x0 on SSI.
    //
    MAP_SysCtlPeripheralEnable(TRF79X0_SSI_PERIPH);

    //
    // Enable the GPIO peripherals associated with the SSI.
    //
    MAP_SysCtlPeripheralEnable(TRF79X0_CLK_PERIPH);
    MAP_SysCtlPeripheralEnable(TRF79X0_RX_PERIPH);
    MAP_SysCtlPeripheralEnable(TRF79X0_TX_PERIPH);
    MAP_SysCtlPeripheralEnable(TRF79X0_CS_PERIPH);

    //
    // Configure the appropriate pins to be SSI instead of GPIO.  The CS
    // is configured as GPIO to support TRF79x0 SPI requirements for R/W
    // access.
    //
    MAP_GPIOPinConfigure(TRF79X0_CLK_CONFIG);
    MAP_GPIOPinConfigure(TRF79X0_RX_CONFIG);
    MAP_GPIOPinConfigure(TRF79X0_TX_CONFIG);
    MAP_GPIOPinTypeSSI(TRF79X0_CLK_BASE, TRF79X0_CLK_PIN);
    MAP_GPIOPinTypeSSI(TRF79X0_RX_BASE, TRF79X0_RX_PIN);
    MAP_GPIOPinTypeSSI(TRF79X0_TX_BASE, TRF79X0_TX_PIN);
    MAP_GPIOPinTypeGPIOOutput(TRF79X0_CS_BASE, TRF79X0_CS_PIN);

    MAP_GPIOPadConfigSet(TRF79X0_CLK_BASE, TRF79X0_CLK_PIN,
                         GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);
    MAP_GPIOPadConfigSet(TRF79X0_RX_BASE, TRF79X0_RX_PIN,
                         GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);
    MAP_GPIOPadConfigSet(TRF79X0_TX_BASE, TRF79X0_TX_PIN,
                         GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);

    //
    // Deassert the SSI chip selects TRF79x0.
    //
    MAP_GPIOPinWrite(TRF79X0_CS_BASE, TRF79X0_CS_PIN, TRF79X0_CS_PIN);

    //
    // Configure the SSI port for 2MHz operation.
    //
    MAP_SSIConfigSetExpClk(TRF79X0_SSI_BASE, g_ui32SysClk,
                           SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, SSI_CLK_RATE,
                           8);

    if(RF_DAUGHTER_TRF7970)
    {
        //
        // Switch from SPH=0 to SPH=1.  Required for TRF7970.
        //
        HWREG(TRF79X0_SSI_BASE + SSI_O_CR0) |= SSI_CR0_SPH;
    }

    //
    // Enable the SSI controller.
    //
    MAP_SSIEnable(TRF79X0_SSI_BASE);
}

//*****************************************************************************
//
// Writes a single value to TRF79x0 for address provided.
//
// \param ucAddress is the register address to write and must be between 0
// and 0x1f, inclusive.
// \param ucData is the data byte to write.
//
// This function asserts the TRF79x0 chip select, sends a write command,
// the single data value and then deasserts the chip select.
//
// \return None.
//
//*****************************************************************************
void
SSITRF79x0WriteRegister(unsigned char ucAddress, unsigned char ucData)
{
    unsigned char pucCommand[2];

    //
    // Assert the chip select for TRF79x0.
    //
    SSITRF79x0ChipSelectAssert();

    //
    // Isolate register address.
    //
    ucAddress = ucAddress & TRF79X0_ADDRESS_MASK;

    //
    // Add TRF79x0 write single command.
    //
    ucAddress |= TRF79X0_CONTROL_REG_WRITE | TRF79X0_REG_MODE_SINGLE;

    //
    // Put the address and data into the buffer.
    //
    pucCommand[0] = ucAddress;
    pucCommand[1] = ucData;

    //
    // Start the write.
    //
    SSITRF79x0GenericWrite(pucCommand, sizeof(pucCommand));

    //
    // Deassert the chip select for the TRF79x0.
    //
    SSITRF79x0ChipSelectDeAssert();
}

//*****************************************************************************
//
// Starts a continuous write operation to the given address.
//
// \param ucAddress is the register address to start this write command and
// must be between 0 and 0x1f, inclusive.
//
// This function asserts the TRF79x0 chip select and sends a write continuous
// command.  The chip select stays asserted when the function returns and must
// be released with SSITRF79x0WriteContinuousStop().
//
// Typical usage for a write to multiple registers at once is: one call to
// SSITRF79x0WriteContinuousStart(), one or more calls to
// SSITRF79x0WriteContinuousData() and one call to
// SSITRF79x0WriteContinuousStop().
//
// \sa SSITRF79x0WriteContinuousData()
//
// \return None.
//
//*****************************************************************************
void
SSITRF79x0WriteContinuousStart(unsigned char ucAddress)
{
    //
    // Assert the chip select for TRF79x0.
    //
    SSITRF79x0ChipSelectAssert();

    //
    // Isolate register address.
    //
    ucAddress = ucAddress & TRF79X0_ADDRESS_MASK;

    //
    // Add TRF79x0 write continuous command.
    //
    ucAddress |= TRF79X0_CONTROL_REG_WRITE | TRF79X0_REG_MODE_CONTINUOUS;

    SSITRF79x0GenericWrite(&ucAddress, 1);

    //
    // Keep chip select asserted for follow-up calls to
    // SSITRF79x0WriteContinuousData().  Calling code must ensure to finish
    // with SSITRF79x0WriteContinuousStop().
    //
}

//*****************************************************************************
//
// Starts a direct continous write operation
//
// This function asserts the chip select for the TRF79x0.
//
// \return None
//
//*****************************************************************************
void
SSITRF79x0WriteDirectContinuousStart(void)
{
    //
    // Assert the chip select for TRF79x0.
    //
    SSITRF79x0ChipSelectAssert();
}

//*****************************************************************************
//
// Sends data in continuous write mode.
//
// \param pucBuffer is a pointer to the data buffer to write.
// \param uiLength is the length of the data to write in bytes.
//
// This function sends data from the buffer to the TRF79x0.  The write must
// have been previously set up with SSITRF79x0WriteContinuousStart().
//
// \return None.
//
//*****************************************************************************
void
SSITRF79x0WriteContinuousData(unsigned char const *pucBuffer,
                              unsigned int uiLength)
{
    SSITRF79x0GenericWrite(pucBuffer, uiLength);
}

//*****************************************************************************
//
// Stops a continuous write operation.
//
// This function deasserts the TRF79x0 chip select.
//
// \return None.
//
//*****************************************************************************
void
SSITRF79x0WriteContinuousStop(void)
{
    //
    // Deassert the chip select for the TRF79x0.
    //
    SSITRF79x0ChipSelectDeAssert();
}

//*****************************************************************************
//
// Reads a single value from TRF79x0 at the address provided.
//
// \param ucAddress is the register address to read and must be between 0
// and 0x1f, inclusive.
//
// This function asserts the TRF79x0 chip select, sends a read command,
// reads a single byte and then deasserts the chip select.
//
// \return This function returns the value that was stored in the given
// register.
//
//*****************************************************************************
unsigned char
SSITRF79x0ReadRegister(unsigned char ucAddress)
{
    unsigned char ucData = 0;

    //
    // Assert the chip select for TRF79x0.
    //
    SSITRF79x0ChipSelectAssert();

    //
    // Isolate register address.
    //
    ucAddress = ucAddress & TRF79X0_ADDRESS_MASK;

    //
    // Add TRF79x0 read single command.
    //
    ucAddress |= TRF79X0_CONTROL_REG_READ | TRF79X0_REG_MODE_SINGLE;

    SSITRF79x0GenericWrite(&ucAddress, 1);

    if(RF_DAUGHTER_TRF7960)
    {
    	//
    	// Switch from SPH=0 to SPH=1.
    	//
    	HWREG(TRF79X0_SSI_BASE + SSI_O_CR0) |= SSI_CR0_SPH;
    }

    //
    // Get the data.
    //
    SSITRF79x0GenericRead(&ucData, 1);

    if(RF_DAUGHTER_TRF7960)
    {
        //
        // Switch from SPH=1 to SPH=0.
        //
        HWREG(TRF79X0_SSI_BASE + SSI_O_CR0) &= ~SSI_CR0_SPH;
    }

    //
    // Deassert the chip select for the TRF79x0.
    //
    SSITRF79x0ChipSelectDeAssert();

    return(ucData);
}

//*****************************************************************************
//
// Starts a continuous read operation from the given address.
//
// \param ucAddress is the register address to start this read command and must
// be between 0 and 0x1f, inclusive.
//
// This function asserts the TRF79x0 chip select and sends a read continuous
// command.  The chip select stays asserted when the function returns and must
// be released with SSITRF79x0ReadContinuousStop().
//
// Typical usage for a read from multiple registers at once is: one call to
// SSITRF79x0ReadContinuousStart(), one or more calls to
// SSITRF79x0ReadContinuousData() and one call to
// SSITRF79x0ReadContinuousStop().
//
// \sa SSITRF79x0ReadContinuousData()
//
// \return None.
//
//*****************************************************************************
void
SSITRF79x0ReadContinuousStart(unsigned char ucAddress)
{
    //
    // Assert the chip select for TRF79x0.
    //
    SSITRF79x0ChipSelectAssert();

    //
    // Isolate register address.
    //
    ucAddress = ucAddress & TRF79X0_ADDRESS_MASK;

    //
    // Add TRF79x0 read continuous command.
    //
    ucAddress |= TRF79X0_CONTROL_REG_READ | TRF79X0_REG_MODE_CONTINUOUS;

    SSITRF79x0GenericWrite(&ucAddress, 1);

    if(RF_DAUGHTER_TRF7960)
    {
        //
        // Switch from SPH=0 to SPH=1.
        //
        HWREG(TRF79X0_SSI_BASE + SSI_O_CR0) |= SSI_CR0_SPH;
    }
}

//*****************************************************************************
//
// Receives data in continuous read mode.
//
// \param pucBuffer is a pointer to the data buffer to receive data.
// \param uiLength is the length of the data to read in bytes.
//
// This function reads data from the the TRF79x0 into the buffer.  The read
// must have been previously set up with SSITRF79x0ReadContinuousStart().
//
// \return None.
//
//*****************************************************************************
void
SSITRF79x0ReadContinuousData(unsigned char *pucBuffer, unsigned int uiLength)
{
    SSITRF79x0GenericRead(pucBuffer, uiLength);
}

//*****************************************************************************
//
// Stop a continuous read operation.
//
// This function deasserts the TRF79x0 chip select.
//
// \return None.
//
//*****************************************************************************
void
SSITRF79x0ReadContinuousStop(void)
{
    if(RF_DAUGHTER_TRF7960)
    {
        //
        // Switch from SPH=1 to SPH=0.
        //
        HWREG(TRF79X0_SSI_BASE + SSI_O_CR0) &= ~SSI_CR0_SPH;
    }

    //
    // Deassert the chip select for the TRF79x0.
    //
    SSITRF79x0ChipSelectDeAssert();
}

//*****************************************************************************
//
// Reads IRQ status value from TRF79x0.
//
// This function reads the TRF79x0 IRQ status register 0x0c and returns its
// contents.  This will make the TRF79x0 release its interrupt request.
//
// \note You should use this function instead of a direct read from register
// 0x0c if you want to retrieve the IRQ status since this function applies
// a special workaround as indicated in SLOA140.
//
// \return Returns the IRQ status
//
//*****************************************************************************
unsigned char
SSITRF79x0ReadIRQStatus(void)
{
    unsigned char pucData[2];

    //
    // Workaround as per SLOA140: When reading the IRQ status register, do a
    // continuous read with an additional register to ensure at least one
    // additional SPI clock after reading the IRQ status.  Ignore the second
    // read result.
    //

    SSITRF79x0ReadContinuousStart(TRF79X0_IRQ_STATUS_REG);
    SSITRF79x0ReadContinuousData(pucData, sizeof(pucData));
    SSITRF79x0ReadContinuousStop();

    return(pucData[0]);
}

//*****************************************************************************
//
// Executes a direct command on the TRF79x0.
//
// \param ucCommand is the command to be executed and must be a valid command
// code between 0 and 0x1f.  Definitions for command codes are given in
// trf79x0.h.
//
// \note This function applies a special workaround as indicated in SLOA140.
//
// \return Returns void.
//
//*****************************************************************************
void
SSITRF79x0WriteDirectCommand(unsigned char ucCommand)
{
    unsigned char pucCommand[2];

    //
    // Assert the chip select for TRF79x0.
    //
    SSITRF79x0ChipSelectAssert();

    //
    // Add TRF79x0 direct command.
    //
    ucCommand = ucCommand | TRF79X0_CONTROL_CMD;

    //
    // Workaround as per SLOA140: When sending a command, add a dummy cycle.
    //
    pucCommand[0] = ucCommand;
    pucCommand[1] = SSI_NO_DATA;

    if(ucCommand == TRF79X0_RESET_FIFO_CMD)
    {
    	SSITRF79x0GenericWrite(pucCommand, sizeof(pucCommand));
    }
    else
    {
    	SSITRF79x0GenericWrite(pucCommand, 1);
    }

    //
    // Deassert the chip select for the TRF79x0.
    //
    SSITRF79x0ChipSelectDeAssert();
}

//*****************************************************************************
//
// Write Direct Command Tailored for 7970 chip. Ported for redundancy
//
// \param ucCommand is the command to be executed and must be a valid command
// code between 0 and 0x1f.  Definitions for command codes are given in
// trf79x0.h. A dummy command is sent after the direct command to handle
// issues with the last command somtimes not processing.
//
//*****************************************************************************
void
SSITRF79x0WriteDirectCommandWithDummy(unsigned char ucCommand)
{
    unsigned char pucCommand[2];

    //
    // Assert the chip select for TRF7970.
    //
    SSITRF79x0ChipSelectAssert();

    //
    // Add TRF7970 direct command.
    //
    ucCommand = ucCommand | TRF79X0_CONTROL_CMD;

    //
    // Workaround as per SLOA140: When sending a command, add a dummy cycle.
    //
    pucCommand[0] = ucCommand;
    pucCommand[1] = SSI_NO_DATA;

    SSITRF79x0GenericWrite(pucCommand, sizeof(pucCommand));

    //
    // Deassert the chip select for the TRF7970.
    //
    SSITRF79x0ChipSelectDeAssert();
}

//*****************************************************************************
//
// Executes a Reset direct command on the TRF79x0.
//
// \param ucCommand is the command to be executed and must be a valid command
// code between 0 and 0x1f.  Definitions for command codes are given in
// trf79x0.h.
//
// \note This function applies a special workaround as indicated in SLOA140.
//
// \return Returns void.
//
//*****************************************************************************
void
SSITRF79x0WriteResetFifoDirectCommand(unsigned char ucCommand)
{
    unsigned char pucCommand[1];

    //
    // Assert the chip select for TRF79x0.
    //
    SSITRF79x0ChipSelectAssert();

    //
    // Add TRF79x0 direct command.
    //
    ucCommand = ucCommand | TRF79X0_CONTROL_CMD;

    //
    // Workaround as per SLOA140: When sending a command, add a dummy cycle.
    //
    pucCommand[0] = ucCommand;

    SSITRF79x0GenericWrite(pucCommand, sizeof(pucCommand));

    //
    // Deassert the chip select for the TRF79x0.
    //
    SSITRF79x0ChipSelectDeAssert();
}

//*****************************************************************************
//
// Executes: writes a packet to the TRF79x0
//
// \param pui8Buffer
// \param ui8CRCBit
// \param ui8TotalLength
// \param ui8PayloadLength
// \param bHeaderEnable
//
// \note
//
// \return Returns void.
//
//*****************************************************************************
void SSITRF79x0WritePacket(uint8_t *pui8Buffer, uint8_t ui8CRCBit, \
    uint8_t ui8TotalLength, uint8_t ui8PayloadLength, bool bHeaderEnable)
{
    uint8_t ui8LengthLowerNibble = (ui8TotalLength & 0x0F) << 4;
    uint8_t ui8LengthHigherNibble = (ui8TotalLength & 0xF0) >> 4;
    uint8_t pui8HeaderData[2];

    //
    // Assert the chip select for TRF79x0.
    //
    SSITRF79x0ChipSelectAssert();

    if(bHeaderEnable == true)
    {
    // RESET FIFO
    //while (!(IFG2 & UCB0TXIFG));        // USCI_B0 TX buffer ready?
    pui8HeaderData[0] = 0x8F;                   // Previous data to TX, RX
    SSITRF79x0GenericWrite(pui8HeaderData,1);
    //while(UCB0STAT & UCBUSY);

    // CRC COMMAND
    //while (!(IFG2 & UCB0TXIFG));        // USCI_B0 TX buffer ready?
    pui8HeaderData[0] = 0x90 | (ui8CRCBit & 0x01);    // Previous data to TX, RX
    SSITRF79x0GenericWrite(pui8HeaderData,1);
    //while(UCB0STAT & UCBUSY);

    // WRITE TO LENGTH REG
    //while (!(IFG2 & UCB0TXIFG));        // USCI_B0 TX buffer ready?
    pui8HeaderData[0] = 0x3D;
    SSITRF79x0GenericWrite(pui8HeaderData,1);
    //while(UCB0STAT & UCBUSY);

    // LENGTH HIGH Nibble
    //while (!(IFG2 & UCB0TXIFG));        // USCI_B0 TX buffer ready?
    pui8HeaderData[0] = ui8LengthHigherNibble;   // Previous data to TX, RX
    SSITRF79x0GenericWrite(pui8HeaderData,1);
    //while(UCB0STAT & UCBUSY);

    // LENGTH LOW Nibble
    //while (!(IFG2 & UCB0TXIFG));        // USCI_B0 TX buffer ready?
    pui8HeaderData[0] = ui8LengthLowerNibble;    // Previous data to TX, RX
    SSITRF79x0GenericWrite(pui8HeaderData,1);
    //while(UCB0STAT & UCBUSY);
    }
    else
    {
        //while (!(IFG2 & UCB0TXIFG));        // USCI_B0 TX buffer ready?
        pui8HeaderData[0] = 0x3F;
        SSITRF79x0GenericWrite(pui8HeaderData,1);
        //while(UCB0STAT & UCBUSY);
    }


    SSITRF79x0GenericWrite(pui8Buffer,ui8PayloadLength);
    //while(ui8PayloadLength > 0)
    //{
    //    while (!(IFG2 & UCB0TXIFG));        // USCI_B0 TX buffer ready?
    //    UCB0TXBUF = *pui8Buffer;                  // Previous data to TX, RX
    //    while(UCB0STAT & UCBUSY);
    //    pui8Buffer++;
    //    ui8PayloadLength--;
    //}

    //
    // Deassert the chip select for the TRF79x0.
    //
    SSITRF79x0ChipSelectDeAssert();
}
