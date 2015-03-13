//*****************************************************************************
//
// trf79x0.c - Driver for the TI TRF79x0 on the dk-lm3s9b96 board.
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
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/timer.h"
#include "utils/uartstdio.h"
#include "ssitrf79x0.h"
#include "trf79x0_hw.h"
#include "trf79x0.h"
#include "nfc.h"
#include "nfclib/debug.h"

//extern unsigned char g_ucNfcWorkMode = NFC_NONE;

//*****************************************************************************
//
// Global Defines
//
//*****************************************************************************
#define NFC_FIFO_SIZE       255
// Fifo size depends on the maximum payload size defined in LLCP.h
uint8_t g_fifo_buffer[NFC_FIFO_SIZE];
uint8_t g_fifo_bytes_received = 0;
volatile uint8_t g_irq_flag = 0x00;
volatile uint8_t g_time_out_flag = 0x00;

tTRF79x0TRFMode g_selected_mode = BOARD_INIT;
tTRF79x0Frequency g_selected_frequency = FREQ_STAND_BY;

// Used for debugging
#define     OUTPUT_FIFO_ENABLE      0

#define     TRF7970A_5V_OPERATION           0x01

//*****************************************************************************
//
// A global variable indicating which RF daughter board, if any, is currently
// connected to the development board.
//
//*****************************************************************************
tRFDaughterBoard g_eRFDaughterType = RF_DAUGHTER_NONE;

//*****************************************************************************
//
// API for the TRF79x0.  Provides register read/write access, command
// execution, abstracted access to IRQ results and comprehensive transceiver
// functionality for higher-layer frame transmission and reception.
//
// Most user code will only need TRF79x0Init() from this module to set up
// and initialize the TRF79x0 and will then use the functions defined by
// some higher layer protocol, such as from iso14443a.c.
//
//*****************************************************************************
//*****************************************************************************
//
// The number of counts to pass to SysCtlDelay() to get approximately 1ms
// delay.
//
//*****************************************************************************
static unsigned long g_ulDelayms;

//*****************************************************************************
//
// Global that holds the clock speed of the MicroController in Hz.
//
//*****************************************************************************
extern uint32_t g_ui32SysClk;

//*****************************************************************************
//
// This structure holds information about encountered IRQs.  The collision
// position can be queried by TRF79x0GetCollisionPosition().
// TRF79x0IRQWait() and TRF79x0IRQWaitTimeout() can be used to wait for
// an interrupt cause to be asserted.  TRF79x0IRQClearAll() and
// TRF79x0IRQClearCauses() can be used to clear indicated causes from this
// structure, since TRF79x0IRQWait()/TRF79x0IRQWaitTimeout() do not do
// that.
//
//*****************************************************************************
static volatile struct
{
    //
    // This stores the contents of the IRQ status register at the most recent
    // IRQ.  However, the contents of this field are not reliable since IRQs
    // may occur shortly after one another and a loop that simply queries state
    // might miss all but the last of these.
    //
    unsigned char ucState;

    //
    // Indicates whether a collision was detected since the last call to
    // TRF79x0GetCollisionPosition().
    //
    unsigned char ucCollisionDetected;

    //
    // Stores the last collision position as returned in registers
    // 0xd and 0xe.
    //
    unsigned int uiCollisionPosition;

    //
    // Bitfield tracking the occurrence of abstract interrupt causes.  The
    // values of enum TRF79x0WaitCondition are used as indices into the
    // bitfield, e.g. for a TRF79X0_WAIT_TXEND interrupt the bit at
    // <tt>(1<<TRF79X0_WAIT_TXEND)</tt> is set.
    //
    unsigned int uiIrqCauses;
}
g_sIRQState;

//*****************************************************************************
//
// Definitions for different interrupt status bits.
//
//*****************************************************************************
#define TX_FIFO_ALMOST_EMPTY  0xA0
#define TX_COMPLETE           0x80
#define RX_FIFO_ALMOST_FULL   0x60
#define RX_COMPLETE           0x40
#define COLLISION_DETECTED    0x02

//*****************************************************************************
//
// Timeout to apply while waiting for reception, this is expressed in
// milliseconds.
//
// For a more accurate timeout indication you can program the no-response
// timer in the TRF79x0 and must enable the no-response interrupt.
//
//*****************************************************************************
#define TRF79X0_RX_TIMEOUT      10

//*****************************************************************************
//
// This structure holds information about the transmission state for use by
// the FIFO refill algorithm in the IRQ handler.  It is set up by
// TRF79x0FIFOWrite().
//
//*****************************************************************************
static volatile struct
{
    //
    // Pointer to the next byte to be transmitted
    //
    unsigned char const *pucBuffer;

    //
    // Number of bytes left that need to be transmitted
    //
    unsigned int uiBytesRemaining;
} g_sTXState;

//*****************************************************************************
//
// This structure holds information about the reception state for use by the
// FIFO read algorithm in the IRQ handler.  It is set up by TRF79x0Receive().
//
//*****************************************************************************
static volatile struct
{
    //
    // Pointer to write the next received byte to.
    //
    unsigned char *pucBuffer;

    //
    // Pointer to the received length counter.  This is the counter that is
    // passed in to TRF79x0Receive().  The integer that this pointer points
    // to contains the number of bytes that were received (and stored in
    // pucBuffer).
    //
    unsigned int *puiLength;

    //
    // Length of the buffer that pucBuffer pointed to at the start of
    // reception.  No more bytes are received when *puiLength equals this
    // value.
    //
    unsigned int uiMaxLength;
} g_sRXState;

//*****************************************************************************
//
// Initializes the TRF79x0 and its communication interface.
//
// This function must be called prior to any other function offered by the
// TRF79x0.  This function initializes the GPIO and pin settings, sets up the
// communication interface by calling SSITRF79x0Init() and sets up the
// interrupt handler by calling TRF79x0InterruptInit().
//
// \return None.
//
//*****************************************************************************
void
TRF79x0Init(void)
{
    //
    // Set up GPIO resources for bit-banging output access to EN and MOD
    // and input for IRQ.
    //
    SysCtlPeripheralEnable(TRF79X0_EN_PERIPH);
    SysCtlPeripheralEnable(TRF79X0_IRQ_PERIPH);
    if(g_eRFDaughterType != RF_DAUGHTER_TRF7970ABP)
    {
        SysCtlPeripheralEnable(TRF79X0_MOD_PERIPH);
        SysCtlPeripheralEnable(TRF79X0_EN2_PERIPH);
        SysCtlPeripheralEnable(TRF79X0_ASKOK_PERIPH);
    }

    //
    // Set the IRQ pin as an input.
    //
    GPIOPinTypeGPIOInput(TRF79X0_IRQ_BASE, TRF79X0_IRQ_PIN);

    //
    // Set the EN, EN2, MOD, and ASKOK pins as outputs.
    //
    GPIOPinTypeGPIOOutput(TRF79X0_EN_BASE, TRF79X0_EN_PIN);
    if(g_eRFDaughterType != RF_DAUGHTER_TRF7970ABP)
    {
        GPIOPinTypeGPIOOutput(TRF79X0_EN2_BASE, TRF79X0_EN2_PIN);
        GPIOPinTypeGPIOOutput(TRF79X0_MOD_BASE, TRF79X0_MOD_PIN);
        GPIOPinTypeGPIOOutput(TRF79X0_ASKOK_BASE, TRF79X0_ASKOK_PIN);
    }

    //
    // Set the MOD and ASKOK pins to start with a low value.
    //
    if(g_eRFDaughterType != RF_DAUGHTER_TRF7970ABP)
    {
        GPIOPinWrite(TRF79X0_MOD_BASE, TRF79X0_MOD_PIN, 0);
        GPIOPinWrite(TRF79X0_ASKOK_BASE, TRF79X0_ASKOK_PIN, 0);
    }

    //
    // Set up the SSI communication interface.
    //
    SSITRF79x0Init();

    //
    // Calculate the number of units for a 1ms delay using SysCtlDelay().
    //
    // NOTE: the ifdef is necessary because of an API change
    //
#ifdef TARGET_IS_TM4C123_RA1
    //
    // Blizzard Silicon (and before)
    //
    g_ulDelayms=(SysCtlClockGet()/3000);
#else
    //
    // Snowflake Silicon (and after)
    //
    g_ulDelayms = (g_ui32SysClk / 3000);
#endif

    //
    // Force a toggle on the EN and EN2 pins.
    //
    GPIOPinWrite(TRF79X0_EN_BASE, TRF79X0_EN_PIN, 0);
    GPIOPinWrite(TRF79X0_EN_BASE, TRF79X0_EN_PIN,
                 TRF79X0_EN_PIN);

//  //
//  // Delay 2ms between ENABLE.
//  //
//  SysCtlDelay(g_ulDelayms * 2);
//
//    GPIOPinWrite(TRF79X0_EN2_BASE, TRF79X0_EN2_PIN, 0);
//    GPIOPinWrite(TRF79X0_EN2_BASE, TRF79X0_EN2_PIN,
//                 TRF79X0_EN2_PIN);

    //
    // Delay 2ms before initializing the TRF79x0.
    //
    SysCtlDelay(g_ulDelayms * 2);

    //
    // Initialize the TRF7970 with a software initialization command, idle
    // command, and set the modulator control register to
    //
    if(RF_DAUGHTER_TRF7970)
    {
        TRF79x0DirectCommand(TRF79X0_SOFT_INIT_CMD);
        TRF79x0DirectCommand(TRF79X0_IDLE_CMD);
    }

    //
    // Get RF Daughter Board ID TRF7960/TRF7970 ATB
    //
    TRF79x0ReadRegister(TRF79X0_MODULATOR_CONTROL_REG);

    TRF79x0WriteRegister(TRF79X0_MODULATOR_CONTROL_REG, 0x01);

    //
    // Set up the interrupt handler and enable the RX timeout IRQ.
    //
    TRF79x0InterruptInit();
    TRF79x0WriteRegister(TRF79X0_IRQ_MASK_REG,
                         TRF79x0ReadRegister(TRF79X0_IRQ_MASK_REG) | 1);

    //
    // Delay 4ms before leaving the initialization function.
    //
    SysCtlDelay(g_ulDelayms * 4);
}

//*****************************************************************************
//
// Set the Operating mode for the TRF79x0
//
// Set bits in ISO_CONTROL_REG based on mode given
// Supported modes:
//      NFC_P2P_PASSIVE_TARGET_MODE
//      NFC_P2P_INITIATOR_MODE
//
// \return None.
//
//*****************************************************************************
void
TRF79x0SetMode(tTRF79x0TRFMode eMode, tTRF79x0Frequency eFrequency)
{
    g_selected_mode = eMode;
    g_selected_frequency = eFrequency;

    if(g_selected_mode == P2P_PASSIVE_TARGET_MODE)
    {
        //
        // Register 01h. ISO Control Register
        //
        if (eFrequency == FREQ_106_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x21);
        } else if (eFrequency == FREQ_212_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x22);
        } else if (eFrequency == FREQ_424_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x23);
        }
    }
    else if(g_selected_mode == P2P_INITATIOR_MODE)
    {
        if (eFrequency == FREQ_106_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x31);
        } else if (eFrequency == FREQ_212_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x32);
        } else if (eFrequency == FREQ_424_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x33);
        }
    }

}

//*****************************************************************************
//
// Prepare the TRF79x0 interrupt handler.
//
// Sets up the GPIO for a level triggered interrupt on the TRF79x0 IRQ line
// and calls TRF79x0InterruptEnable().  Processor interrupts need to be
// enabled (IntMasterEnable() from DriverLib) for the interrupt handler to
// to be actually called.
//
// \return None.
//
//*****************************************************************************
void
TRF79x0InterruptInit(void)
{
    //
    // Set GPIO Interrupt to level triggered active high.
    //
    GPIOIntTypeSet(TRF79X0_IRQ_BASE, TRF79X0_IRQ_PIN, GPIO_RISING_EDGE);

    //
    // Clear out any pending interrupt.
    //
    GPIOIntClear(TRF79X0_IRQ_BASE, TRF79X0_IRQ_PIN);

    //
    // Set GPIO Interrupt Enable.
    //
    TRF79x0InterruptEnable();

    //
    // Enable the GPIO interrupt.
    //
    IntEnable(TRF79X0_IRQ_INT);
}

//*****************************************************************************
//
// IRQ pin Interrupt Handler. This function is triggered by the IRQ pin going
// high. The g_irq_flag flag is set as a result.
//
//*****************************************************************************
void TRF79x0IRQPinInterruptHandler(void)
{
    uint32_t ui32IRQGPIOBankIntStatus;

    //
    // Get the masked interrupt status.
    //
    ui32IRQGPIOBankIntStatus=GPIOIntStatus(TRF79X0_IRQ_BASE,true);


    //
    // check if IRQ pin is high
    //
    if(ui32IRQGPIOBankIntStatus & TRF79X0_IRQ_PIN)
    {
        //
        // Clear the asserted interrupts.
        //
        GPIOIntClear(TRF79X0_IRQ_BASE, TRF79X0_IRQ_PIN);

        //
        // Set flag appropriately.
        //
        g_irq_flag = 0x01;
    }

}

//*****************************************************************************
//
// Internal helper function to transmit up to uiMaxLength bytes from g_sTXState
// to the FIFO.
//
//*****************************************************************************
static void
FIFOTransmitSomeBytes(unsigned int uiMaxLength)
{
    unsigned int uiLength;

    if(g_sTXState.uiBytesRemaining > 0)
    {
        //
        // There are some bytes in g_sTXState that still need to
        // be sent.
        //
        uiLength = g_sTXState.uiBytesRemaining;

        if(uiLength > uiMaxLength)
        {
            //
            // Clamp number of bytes to be sent to parameter uiMaxLength,
            // which is 12 for the initial call with an empty FIFO and
            // 9 for subsequent calls from the IRQ.
            //
            uiLength = uiMaxLength;
        }

        //
        // Send the data in a continuous write to the FIFO "register".
        //
        if(RF_DAUGHTER_TRF7960)
        {
            SSITRF79x0WriteContinuousStart(TRF79X0_FIFO_REG);
            SSITRF79x0WriteContinuousData(g_sTXState.pucBuffer, uiLength);
            SSITRF79x0WriteContinuousStop();
        }

        if(RF_DAUGHTER_TRF7970)
        {
            SSITRF79x0WriteContinuousData(g_sTXState.pucBuffer, uiLength);
            SSITRF79x0WriteContinuousStop();
        }

        //
        // Update g_sTXState to reflect what we just sent.
        //
        g_sTXState.pucBuffer += uiLength;
        g_sTXState.uiBytesRemaining -= uiLength;
    }
}




//*****************************************************************************
//
// Clears all IRQ causes from g_sIRQState.
//
// You will need to call either this function or TRF79x0IRQClearCauses()
// before a call to TRF79x0IRQWait() or TRF79x0IRQWaitTimeout() in order to
// clear sticky causes from the interrupt state.  If a cause has been indicated
// before and is not cleared from the state then the wait functions will
// return immediately.
//
// \return None.
//
//*****************************************************************************
void
TRF79x0IRQClearAll(void)
{
    //
    // Clear the interrupt causes flags.
    //
    g_sIRQState.uiIrqCauses = 0;
}

//*****************************************************************************
//
// Clears all given IRQ causes from g_sIRQState.
//
// \param causes is a bitfield of clauses to clear.  This is a logical or of
// one or more terms of the form <tt>(1<<<i>x</i>)</tt> where <i>x</i>
// is a value from enumeration TRF79x0WaitCondition.
//
// You will need to call either this function or TRF79x0IRQClearAll()
// before a call to TRF79x0IRQWait() or TRF79x0IRQWaitTimeout().
//
// \return None.
//
//*****************************************************************************
void
TRF79x0IRQClearCauses(unsigned int uiCauses)
{
    //
    // Clear the requested interrupt causes.
    //
    g_sIRQState.uiIrqCauses &= ~uiCauses;
}

//*****************************************************************************
//
// Returns the last indicated collision position and clears the collision
// position indicator.
//
// \return This function returns the collision position as returned by the
// TRF79x0 in registers 0xd and 0xe, or -1 if no collision was indicated since
// the last call to this function.
//
//*****************************************************************************
int
TRF79x0GetCollisionPosition(void)
{
    //
    // If there were no collisions detected then just return.
    //
    if(!g_sIRQState.ucCollisionDetected)
    {
        return(-1);
    }

    //
    // Clear the collisions detected flag and return the number of collisions
    // detected.
    //
    g_sIRQState.ucCollisionDetected = 0;

    return(g_sIRQState.uiCollisionPosition);
}

//*****************************************************************************
//
// Enables the TRF79x0 IRQ handler.
//
// The interrupt handler needs and the processor interrupt to be enabled
// (IntMasterEnable() from DriverLib) in order for transmission and
// reception to work.
//
// \return None.
//
//*****************************************************************************
void
TRF79x0InterruptEnable(void)
{
    //
    // Enable interrupts on the pin assigned to the IRQ signal.
    //
    GPIOIntEnable(TRF79X0_IRQ_BASE, TRF79X0_IRQ_PIN);
}

//*****************************************************************************
//
// Disables the TRF79x0 IRQ handler.
//
// \return None.
//
//*****************************************************************************
void
TRF79x0InterruptDisable(void)
{
    //
    // Disable interrupts on the pin assigned to the IRQ signal.
    //
    GPIOIntDisable(TRF79X0_IRQ_BASE, TRF79X0_IRQ_PIN);
}

//*****************************************************************************
//
// TRF79x0DisableTransmitter - Disable the TRF79x0 Transmitter and Reset Fifo
//
//*****************************************************************************
void TRF79x0DisableTransmitter(void)
{
    //
    // Register 00h. Chip Status Control
    //
    TRF79x0WriteRegister(TRF79X0_CHIP_STATUS_CTRL_REG,0x00 | TRF7970A_5V_OPERATION);

    //
    // Reset FIFO CMD + Dummy byte
    //
    TRF79x0ResetFifoCommand();
}

//*****************************************************************************
//
// stop, then start the decoders
//
//*****************************************************************************
void TRF797x0ResetDecoders(void)
{
    TRF79x0DirectCommand(TRF79X0_STOP_DECODERS_CMD);
    TRF79x0DirectCommand(TRF79X0_RUN_DECODERS_CMD);

}

//*****************************************************************************
//
//
//
//*****************************************************************************
uint8_t* TRF79x0GetNFCBuffer(void)
{
    return g_fifo_buffer;
}

//*****************************************************************************
//
// Waits for an abstract IRQ cause.
//
// \param eCondition is the IRQ cause to wait for.
//
// Waits until the IRQ handler indicates that the given abstract IRQ cause
// has been met.
//
// \return Returns 1.
//
//*****************************************************************************
int
TRF79x0IRQWait(unsigned long ulCondition)
{
    //
    // Wait with no timeout.
    //
    return(TRF79x0IRQWaitTimeout(ulCondition, 0));
}

//*****************************************************************************
//
// Waits for an abstract IRQ cause or timeout.
//
// \param ulCondition is the IRQ cause to wait for.
// \param ulTimeout is the number of milliseconds to wait before a timeout
// occurs.
//
// Waits until the IRQ handler indicates that the given abstract IRQ cause
// has been met or the timeout occurs.  If ulTimeout is 0 then this function
// will wait forever.
//
// \return This function returns 1 if the condition was reached or 0 if the
// function aborted due to the timeout being met.
//
//*****************************************************************************
int
TRF79x0IRQWaitTimeout(unsigned long ulCondition, unsigned long ulTimeout)
{
    unsigned long ulTime;

    //
    // If timeout was not set or not reached, return true.
    //
    if(ulTimeout == 0)
    {
        return(1);
    }

    ulTime = 0;

    while((g_sIRQState.uiIrqCauses & ulCondition) == 0)
    {
        if(ulTimeout == ulTime)
        {
            //
            // Abort if timeout is set and reached.
            //
            break;
        }

        //
        // Delay 1ms and check again.
        //
        SysCtlDelay(g_ulDelayms);

        //
        // Increment the loop count.
        //
        ulTime++;
    }

    //
    // If timeout was set and reached: return false.
    //
    if(ulTimeout == ulTime)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//*****************************************************************************
//
// Issues a direct command on the TRF79x0.
//
// \param ucCommand is the command to be executed.  Must be a valid command
// code between 0 and 0x1f.  Definitions for command codes are given in
// trf79x0.h.
//
// \return None.
//
//*****************************************************************************
void
TRF79x0DirectCommand(unsigned char ucCommand)
{
    SSITRF79x0WriteDirectCommand(ucCommand);
}

//*****************************************************************************
//
// Issues a direct Reset FIFO command on the TRF79x0.
//
// \param ucCommand is the command to be executed.  Must be a valid command
// code between 0 and 0x1f.  Definitions for command codes are given in
// trf79x0.h.
//
// \return None.
//
//*****************************************************************************
void
TRF79x0ResetFifoCommand(void)
{
    SSITRF79x0WriteResetFifoDirectCommand(TRF79X0_RESET_FIFO_CMD);
}

//*****************************************************************************
//
//! Writes a single value to the TRF79x0 for address provided.
//!
//! \param ucAddress is the register address to write to.  Must be between 0
//! and 0x1f, inclusive.
//! \param ucData is the data byte to be written.
//!
//! \return None.
//
//*****************************************************************************
void
TRF79x0WriteRegister(unsigned char ucAddress, unsigned char ucData)
{
    SSITRF79x0WriteRegister(ucAddress, ucData);
}


//*****************************************************************************
//
// Initialize the mode and frequecy  for the TRF79x0.
// Useful for hot switching modes
//
// \param eMode is the mode the TRF79x0 is operating in.
//    Implemented:                  Future Implementation:
//    BOARD_INIT                    P2P_ACTIVE_TARGET_MODE
//    P2P_INITATIOR_MODE            CARD_EMULATION_TYPE_A
//    P2P_PASSIVE_TARGET_MODE       CARD_EMULATION_TYPE_B
//
// \param eFrequency is the frequency to set the board to.
//  Valid values are:
//    FREQ_STAND_BY
//    FREQ_106_KBPS
//    FREQ_212_KBPS
//    FREQ_424_KBPS
//
//*****************************************************************************
tStatus TRF79x0Init2(tTRF79x0TRFMode eMode, tTRF79x0Frequency eFrequency)
{
    uint8_t ui8RxVal;
    uint8_t ui8RxValCont[2];

    g_selected_mode = eMode;
    g_selected_frequency = eFrequency;

    if (eMode == BOARD_INIT) {

        do {
            //
            // Soft Init Command
            //
            TRF79x0DirectCommand(TRF79X0_SOFT_INIT_CMD);

            //
            // Idle Command
            //
            TRF79x0DirectCommand(TRF79X0_IDLE_CMD);

            //
            // Delay 1ms
            // NOTE: Sysctl delay takes 3 clock ticks to complete,
            //       thus 1ms = (clock/1000)/3 or clock/3000
            //
            SysCtlDelay(g_ulDelayms * 1 );

            //
            // Register 09h. Modulator Control
            //
            ui8RxVal=TRF79x0ReadRegister(TRF79X0_MODULATOR_CONTROL_REG);

        } while (ui8RxVal != 0x91);

        //
        // Register 09h. Modulator Control
        //
        // SYS_CLK (in this case 13.56 MHz) out optional, based on system req.
        TRF79x0WriteRegister(TRF79X0_MODULATOR_CONTROL_REG, 0x00);

        //
        // Register 0Bh. Regulator Control
        //
        TRF79x0WriteRegister(TRF79X0_REGULATOR_CONTROL_REG, 0x87);

        //
        // Reset FIFO CMD + Dummy byte
        //
        TRF79x0ResetFifoCommand();

        //
        // Register 00h. Chip Status Control
        //
        // +5 V operation
        TRF79x0WriteRegister(TRF79X0_CHIP_STATUS_CTRL_REG, 0x00 | TRF7970A_5V_OPERATION);

        //
        // Register 0Dh. Interrupt Mask Register
        //
//      TRF79x0WriteRegister(TRF79X0_IRQ_MASK_REG, 0x3F);//NO Response IRQEnable
        TRF79x0WriteRegister(TRF79X0_IRQ_MASK_REG, 0x3E);

        //
        // Register 14h. FIFO IRQ Level
        //
        // RX High = 96 bytes , TX Low = 32 bytes
        TRF79x0WriteRegister(TRF79X0_FIFO_IRQ_LEVEL_REG, 0x0F);
    } else if (eMode == P2P_INITATIOR_MODE) {
        // TODO - Understand why the SOFT Init at start up, does
        //      not allow to send packets to reader
        //
        // Soft Init Command
        //
        TRF79x0DirectCommand(TRF79X0_SOFT_INIT_CMD);

        //
        // Idle Command
        //
        TRF79x0DirectCommand(TRF79X0_IDLE_CMD);

        // Register 00h. Chip Status Control
        // RF output active, +5 V operation
        TRF79x0WriteRegister(TRF79X0_CHIP_STATUS_CTRL_REG, 0x02 | TRF7970A_5V_OPERATION);

        // Check if there an external RF Field
        TRF79x0DirectCommand(TRF79X0_TEST_EXTERNAL_RF_CMD);

        //
        // Delay 50uS
        //
        SysCtlDelay((g_ulDelayms/1000) * 50);

        ui8RxVal=TRF79x0ReadRegister(TRF79X0_RSSI_LEVEL_REG);

        // If the External RF Field is 0x00, we continue else we return fail
        if ((ui8RxVal & 0x3F) != 0x00) {
            //UARTprintf("Initiator Mode field disabled. RSSI: 0x%x \n",
            //ui8RxVal);

            // Register 00h. Chip Status Control
            // RF output de-activated, +5 V operation
            TRF79x0WriteRegister(TRF79X0_CHIP_STATUS_CTRL_REG, 0x00 | TRF7970A_5V_OPERATION);
            return STATUS_FAIL;
        }

        //
        // Register 09h. Modulator Control
        //
        // SYS_CLK (in this case 13.56 MHz) out optional, based on system req.
        TRF79x0WriteRegister(TRF79X0_MODULATOR_CONTROL_REG, 0x00);

        //
        // Register 0Bh. Regulator Control
        //
        TRF79x0WriteRegister(TRF79X0_REGULATOR_CONTROL_REG, 0x01);

        //
        // Register 14h. FIFO IRQ Level
        //
        // RX High = 96 bytes , TX Low = 32 bytes
        TRF79x0WriteRegister(TRF79X0_FIFO_IRQ_LEVEL_REG, 0x0F);

        //
        // Register 01h. Chip Status Control
        //
        if (eFrequency == FREQ_106_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x31);
        } else if (eFrequency == FREQ_212_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x1A);
        } else if (eFrequency == FREQ_424_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x1B);
        }

        //
        // Register 0Ah. RX Special Settings
        //
        // Turn off transmitter, +5 V operation
        TRF79x0WriteRegister(TRF79X0_RX_SPECIAL_SETTINGS_REG, 0x2F);

        //
        // Register 16h. NFC Low Detection Level
        //
        TRF79x0WriteRegister(TRF79X0_NFC_LO_FIELD_LEVEL_REG, 0x83);

        //
        // Register 18h. NFC Target level
        //
//        TRF79x0WriteRegister(TRF79X0_NFC_TARGET_LEVEL_REG, 0x07);

        //
        // Register 00h. Chip Status Control
        //
        // Turn off transmitter, +5 V operation
        TRF79x0WriteRegister(TRF79X0_CHIP_STATUS_CTRL_REG, 0x20 |TRF7970A_5V_OPERATION);

        //
        // Guard Time Delay (GT_F) - 20 mS - Incremented to 30 mS due to the GS3.
        //
        SysCtlDelay(g_ulDelayms * 30);

    } else if (eMode == P2P_PASSIVE_TARGET_MODE || eMode == P2P_ACTIVE_TARGET_MODE) {
        //
        // Soft Init Command
        //
        TRF79x0DirectCommand(TRF79X0_SOFT_INIT_CMD);

        //
        // Idle Command
        //
        TRF79x0DirectCommand(TRF79X0_IDLE_CMD);

        //
        // Disable Decoder Command
        //
        TRF79x0DirectCommand(TRF79X0_STOP_DECODERS_CMD);

        //
        // Register 01h. ISO Control Register
        //
        if (eFrequency == FREQ_106_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x21);
        } else if (eFrequency == FREQ_212_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x22);
        } else if (eFrequency == FREQ_424_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x23);
        }

        //
        // Register 09h. Modulator Control
        //
        // SYS_CLK Disabled, based on system req.
        TRF79x0WriteRegister(TRF79X0_MODULATOR_CONTROL_REG, 0x00);

        //
        // Register 0Ah. RX Special Settings
        //
//        TRF79x0WriteRegister(TRF79X0_RX_SPECIAL_SETTINGS_REG, 0x30);

        //
        // Register 0Bh. Regulator Control
        //
        TRF79x0WriteRegister(TRF79X0_REGULATOR_CONTROL_REG, 0x01);

        //
        // Register 14h. FIFO IRQ Level
        //
        // RX High = 96 bytes , TX Low = 32 bytes
        TRF79x0WriteRegister(TRF79X0_FIFO_IRQ_LEVEL_REG, 0x0F);

        //
        // Register 16h. NFC Low Detection Level
        //
        TRF79x0WriteRegister(TRF79X0_NFC_LO_FIELD_LEVEL_REG, 0x83);

        //
        // Register 18h. NFC Target level
        //
        TRF79x0WriteRegister(TRF79X0_NFC_TARGET_LEVEL_REG, 0x07);

        //
        // Register 00h. Chip Status Control
        //
        // RF output active, +5 V operation
        TRF79x0WriteRegister(TRF79X0_CHIP_STATUS_CTRL_REG, 0x20 | TRF7970A_5V_OPERATION);

        //
        // Read IRQ Register & Collision Register to clear data.
        //
        TRF79x0ReadRegisterContinuous(TRF79X0_IRQ_STATUS_REG, ui8RxValCont, 2);

        //
        // Enable Decoder Command
        //
        TRF79x0DirectCommand(TRF79X0_RUN_DECODERS_CMD);
    }

    return STATUS_SUCCESS;
}

//*****************************************************************************
//
// Write Fifo - used for NFC
//
//*****************************************************************************
tStatus TRF79x0WriteFIFO(uint8_t *pui8Buffer, tTRF79x0CRC eCRCBit,
                            uint8_t ui8Length)
{
    tStatus eStatus;
    tTRF79x0IRQFlag irq_flag = IRQ_STATUS_IDLE;
    uint8_t remaining_bytes = 0;
    uint8_t ui8FifoStatusLength = 0;
    uint8_t ui8PayloadLength = 0;
    uint8_t pui8IRQBuffer[2];

    if (ui8Length > 127) {
        ui8PayloadLength = 127;
    } else {
        ui8PayloadLength = ui8Length;
    }

    remaining_bytes = ui8Length - ui8PayloadLength;

    if(g_selected_mode == P2P_ACTIVE_TARGET_MODE)
    {
        //
        // Register 01h. ISO Control Register
        //
        if (g_selected_frequency == FREQ_106_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x31);
        } else if (g_selected_frequency == FREQ_212_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x32);
        } else if (g_selected_frequency == FREQ_424_KBPS) {
            TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x33);
        }
    }

    if (IRQ_IS_SET())
    {
        //
        // Read IRQ Register
        //
        TRF79x0ReadRegisterContinuous(TRF79X0_IRQ_STATUS_REG, pui8IRQBuffer, 2);
    }

    SSITRF79x0WritePacket(pui8Buffer, eCRCBit, ui8Length, ui8PayloadLength, \
                            true);

    while (irq_flag != IRQ_STATUS_TX_COMPLETE) {
        // Workaround for Type A commands - check the IRQ within 10 mS to
        // refill FIFO
        if(g_selected_mode == CARD_EMULATION_TYPE_A)
            irq_flag = TRF79x0IRQHandler(10);
        else
        {
            // No workaround needed, implement a longer timeout, allowing for
            // FIFO IRQ to handle the FIFO levels
            irq_flag = TRF79x0IRQHandler(100);
        }

        if (irq_flag == IRQ_STATUS_PROTOCOL_ERROR) {
            eStatus = STATUS_FAIL;
            break;
        } else if (irq_flag == IRQ_STATUS_TX_COMPLETE) {
            if(g_selected_mode == P2P_ACTIVE_TARGET_MODE)
            {
                //
                // Delay 1uS
                //
                SysCtlDelay((g_ulDelayms/1000) * 1);

                //
                // Register 01h. ISO Control Register
                //
                if(g_selected_frequency == FREQ_106_KBPS)
                    TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x21);
                else if(g_selected_frequency == FREQ_212_KBPS)
                    TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x22);
                else if(g_selected_frequency == FREQ_424_KBPS)
                    TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, 0x23);
            }
            eStatus = STATUS_SUCCESS;
        } else if ((irq_flag == IRQ_STATUS_FIFO_HIGH_OR_LOW
                || irq_flag == IRQ_STATUS_TIME_OUT) && remaining_bytes > 0) {
            // Modify the pointer to point to the next address of data for
            // payload larger than 127 bytes
            pui8Buffer = pui8Buffer + ui8PayloadLength;

            ui8FifoStatusLength=TRF79x0ReadRegister(TRF79X0_FIFO_STATUS_REG);

            // Check if there are more remaining bytes than available spots on
            // the TRF7970
            if (remaining_bytes > (127 - ui8FifoStatusLength)) {
                // If there are more bytes than available then payload length
                //is the (127 - ui8FifoStatusLength)
                ui8PayloadLength = (127 - ui8FifoStatusLength);
            } else {
                ui8PayloadLength = remaining_bytes;
            }

            remaining_bytes = remaining_bytes - ui8PayloadLength;

            SSITRF79x0WritePacket(pui8Buffer, eCRCBit, ui8Length, \
                                    ui8PayloadLength, false);
        }
    }

    return eStatus;
}

//*****************************************************************************
//
// IRQ Handler
//
// NOTE: currently TimerSet, TimerDisable, and TimerInteruptHandler must be
// implemented by the user.
//
//*****************************************************************************
extern void TimerSet(uint16_t timeout_ms, uint8_t * timeout_flag);

tTRF79x0IRQFlag
TRF79x0IRQHandler(uint16_t ui16TimeOut)
{
    tTRF79x0IRQFlag eIRQStatus = IRQ_STATUS_IDLE;
    uint8_t pui8IRQBuffer[2];
    uint8_t pui8TargetProtocol[2];
    uint8_t ui8FifoStatusLength;
    uint8_t ui8FifoIndex = 0;
    uint8_t ui8PacketLength = 0;

    //volatile uint8_t x;

    if (IRQ_IS_SET())
    {
        g_irq_flag = 0x01;
    }
    else
    {
        g_irq_flag = 0x00;
        //
        // Initialize a ui16TimeOut timeout
        //
        TimerSet(ui16TimeOut, (uint8_t*) &g_time_out_flag);

    }

    //
    // Check if the IRQ flag has been set
    //
    while (g_irq_flag == 0x00 && g_time_out_flag == 0x00) {
        ;
        //
        // Enable Low Power Mode 0
        //
        //__bis_SR_register(LPM0_bits);
    }

    //
    // Disable Timer
    //
    TimerDisable(TIMER0_BASE, TIMER_A);

    if (g_time_out_flag == 0x01) {
        //MCU_rssiDisplay(0);
        eIRQStatus = IRQ_STATUS_TIME_OUT;
    } else {

        TRF79x0ReadRegisterContinuous(TRF79X0_NFC_TARGET_PROTOCOL_REG, \
                                        pui8TargetProtocol, 2);

        //
        // Read IRQ Register
        //
        TRF79x0ReadRegisterContinuous(TRF79X0_IRQ_STATUS_REG, pui8IRQBuffer, 2);

        if (pui8IRQBuffer[0] & IRQ_STATUS_FIFO_HIGH_OR_LOW) {
            if (pui8IRQBuffer[0] & IRQ_STATUS_RX_COMPLETE) {
                g_fifo_bytes_received = 0;
                //
                // Read the FIFO status and FIFO into g_nfc_buffer
                //
                ui8FifoStatusLength=TRF79x0ReadRegister(TRF79X0_FIFO_STATUS_REG);

                ui8FifoIndex = 0;

                while ((ui8FifoStatusLength > 0) &&
                        (g_fifo_bytes_received < NFC_FIFO_SIZE))
                {

                    //
                    // Update the received bytes
                    //
                    g_fifo_bytes_received += ui8FifoStatusLength;
                    #ifdef DEBUG
                    //DebugPrintf("%d\n",g_fifo_bytes_received);
                    #endif

                    //
                    // Read the FIFO Data
                    //
                    TRF79x0ReadRegisterContinuous(TRF79X0_FIFO_REG,
                        &g_fifo_buffer[ui8FifoIndex], ui8FifoStatusLength);

                    ui8PacketLength = g_fifo_buffer[0];

                    //
                    // Update ui8FifoIndex
                    //
                    ui8FifoIndex = ui8FifoIndex + ui8FifoStatusLength;

                    if (!IRQ_IS_SET())
                    {
                        g_irq_flag = 0;
                    }

                    //
                    // Type F - P2P Workaround
                    //
                    if((g_selected_mode == P2P_PASSIVE_TARGET_MODE) ||
                        (g_selected_mode == P2P_INITATIOR_MODE))
                    {
                        //
                        // Check if we have received all the bytes defined in
                        // the first packet.
                        //
                        if(g_fifo_buffer[0] == g_fifo_bytes_received)
                        {
                            eIRQStatus = IRQ_STATUS_RX_COMPLETE;
                            break;
                        }
                        //
                        // If we have not read all the bytes, then every 1 mS
                        // go read out the FIFO status register to ensure we do
                        // not get an overflow flag.
                        //
                        else
                        {
                            //
                            // Initialize a 1 mS timeout
                            //
                            ui16TimeOut = 0x01;
                            TimerSet(ui16TimeOut, (uint8_t*) &g_time_out_flag);

                            while(g_irq_flag == 0x00 && g_time_out_flag == 0x00)
                            {
                                //
                                // Enable Low Power Mode 0
                                //
                               // __bis_SR_register(LPM0_bits);
                            }

                            //
                            // Disable Timer
                            //
                            TimerDisable(TIMER0_BASE, TIMER_A);
                        }

                    }
                    else
                    {
                        while ((g_irq_flag == 0) && (
                                    (uint8_t) g_fifo_bytes_received !=
                                                            ui8PacketLength))
                        {
                            //
                            // Enable Low Power Mode 0
                            //
                            //__bis_SR_register(LPM0_bits);
                        }
                    }

                    TRF79x0ReadRegisterContinuous(TRF79X0_IRQ_STATUS_REG,
                                                    pui8IRQBuffer, 2);

                    //
                    // Read the FIFO status and FIFO into g_nfc_buffer
                    //
                    ui8FifoStatusLength =
                                TRF79x0ReadRegister(TRF79X0_FIFO_STATUS_REG);
                    //
                    // Mask off the lower 7 bits.
                    //
                    ui8FifoStatusLength &= 0x7F;
                }

                //TRF79x0ResetFifoCommand();

                eIRQStatus = IRQ_STATUS_RX_COMPLETE;
            }
            else if (pui8IRQBuffer[0] & IRQ_STATUS_TX_COMPLETE)
            {
                eIRQStatus = IRQ_STATUS_FIFO_HIGH_OR_LOW;
            }
        }
        else if (pui8IRQBuffer[0] == IRQ_STATUS_RX_COMPLETE)
        {

            //
            // Read the FIFO status and FIFO into g_nfc_buffer
            //
            ui8FifoStatusLength=TRF79x0ReadRegister(TRF79X0_FIFO_STATUS_REG);

            if (ui8FifoStatusLength != 0) {
                //
                // Read the FIFO Data
                //
                TRF79x0ReadRegisterContinuous(TRF79X0_FIFO_REG, g_fifo_buffer,
                        ui8FifoStatusLength);

                g_fifo_bytes_received = ui8FifoStatusLength;
            } else {
                TRF79x0Init2(g_selected_mode, g_selected_frequency);
                return IRQ_STATUS_IDLE;
            }

            // Check if the selected_mode corresponds to the command read in
            // the command
            if ((pui8TargetProtocol[0] == 0xC9
                    && g_selected_mode == CARD_EMULATION_TYPE_A)
                    || (pui8TargetProtocol[0] == 0xC5
                            && g_selected_mode == CARD_EMULATION_TYPE_B)
                    || (pui8TargetProtocol[0] == 0xD2
                            && g_selected_mode == P2P_PASSIVE_TARGET_MODE
                            && g_selected_frequency == FREQ_212_KBPS)
                    || (pui8TargetProtocol[0] == 0xD3
                            && g_selected_mode == P2P_PASSIVE_TARGET_MODE
                            && g_selected_frequency == FREQ_424_KBPS)
                    || (pui8TargetProtocol[0] == 0xD2
                            && g_selected_mode == P2P_ACTIVE_TARGET_MODE
                            && g_selected_frequency == FREQ_212_KBPS)
                    || (pui8TargetProtocol[0] == 0xD3
                            && g_selected_mode == P2P_ACTIVE_TARGET_MODE
                            && g_selected_frequency == FREQ_424_KBPS)
                    || (g_selected_mode == P2P_INITATIOR_MODE))
            {
                eIRQStatus = IRQ_STATUS_RX_COMPLETE;
                if(g_selected_mode == P2P_INITATIOR_MODE ||
                   g_selected_mode == P2P_PASSIVE_TARGET_MODE)
                    //
                    // 500 microsecond // TR0
                    //
                    SysCtlDelay(g_ulDelayms / 2);
            }
            else
                TRF79x0Init2(g_selected_mode, g_selected_frequency);

        } else if (pui8IRQBuffer[0] & IRQ_STATUS_COLLISION_AVOID_FINISHED) {
            eIRQStatus = IRQ_STATUS_COLLISION_AVOID_FINISHED;
        } else if (pui8IRQBuffer[0] & IRQ_STATUS_RX_COMPLETE) {
            // Handle the case for P2P Initiator Mode where IRQ is triggered
            // with value 0xC0 - TODO
            if(pui8IRQBuffer[0] & IRQ_STATUS_TX_COMPLETE)
            {

            }
            else if(pui8IRQBuffer[0] & IRQ_STATUS_PROTOCOL_ERROR)
            {
                TRF79x0Init2(g_selected_mode, g_selected_frequency);
            }
            else
            {
                //
                // Read the FIFO status and FIFO into g_nfc_buffer
                //
                ui8FifoStatusLength =
                                TRF79x0ReadRegister(TRF79X0_FIFO_STATUS_REG);

                TRF79x0ResetFifoCommand();
            }
        }
        else if (pui8IRQBuffer[0] & IRQ_STATUS_PROTOCOL_ERROR
                || pui8IRQBuffer[0] & IRQ_STATUS_COLLISION_ERROR)
        {
            eIRQStatus = IRQ_STATUS_PROTOCOL_ERROR;
            TRF79x0Init2(g_selected_mode, g_selected_frequency);
        }
        else if (pui8IRQBuffer[0] & IRQ_STATUS_TX_COMPLETE)
        {

            // Reset FIFO CMD + Dummy byte
            TRF79x0ResetFifoCommand();

            eIRQStatus = IRQ_STATUS_TX_COMPLETE;
        }
        else if (pui8IRQBuffer[0] & IRQ_STATUS_RF_FIELD_CHANGE)
        {

            eIRQStatus = IRQ_STATUS_RF_FIELD_CHANGE;
        }

    }

    //
    // Reset Global Flags
    //
    g_irq_flag = 0x00;
    g_time_out_flag = 0x00;

    return eIRQStatus;
}

//*****************************************************************************
//
// Writes a sequence of values to the TRF79x0 starting at the address
// provided.
//
// \param ucAddress is the register address to start the write at.  Must be
// between 0 and 0x1f, inclusive.
// \param pucData is a pointer to the data buffer to be written.
// \param uiLength is the length of the buffer and number of bytes to write.
//
// \return None.
//
//*****************************************************************************
void
TRF79x0WriteRegisterContinuous(unsigned char ucAddress, unsigned char *pucData,
                               unsigned int uiLength)
{
    SSITRF79x0WriteContinuousStart(ucAddress);
    SSITRF79x0WriteContinuousData(pucData, uiLength);
    SSITRF79x0WriteContinuousStop();
}

//*****************************************************************************
//
// Reads IRQ status value from TRF79x0.
//
// This function reads the TRF79x0 IRQ status register 0x0c and returns its
// contents.  This will make the TRF79x0 release its interrupt request.
//
// \return Returns the IRQ status
//
//*****************************************************************************
unsigned char
TRF79x0ReadIRQStatus(void)
{
    return(SSITRF79x0ReadIRQStatus());
}

//*****************************************************************************
//
// Reads a single value from TRF79x0 at the address provided.
//
// \param ucAddress is the register address to read from.  Must be between 0
// and 0x1f, inclusive.
//
// \return Returns the value that was stored in the given register.
//
//*****************************************************************************
unsigned char
TRF79x0ReadRegister(unsigned char ucAddress)
{
    return(SSITRF79x0ReadRegister(ucAddress));
}

//*****************************************************************************
//
// Reads a sequence of values from the TRF79x0 starting at the address
// provided.
//
// \param ucAddress is the register address to start the read at.  Must be
// between 0 and 0x1f, inclusive.
// \param pucData is a pointer to the data buffer to store the read bytes into.
// \param uiLength is the length of the buffer and number of bytes to read.
//
// \return None.
//
//*****************************************************************************
void
TRF79x0ReadRegisterContinuous(unsigned char ucAddress, unsigned char *pucData,
                              unsigned int uiLength)
{
    SSITRF79x0ReadContinuousStart(ucAddress);
    SSITRF79x0ReadContinuousData(pucData, uiLength);
    SSITRF79x0ReadContinuousStop();
}

//*****************************************************************************
//
// Writes a sequence of values to the FIFO of the TRF79x0.
//
// \param pucData is a pointer to the data buffer to be written.
// \param length is the length of the buffer and number of bytes to write.
//
// This function sets up g_sTXState for the write operation to the FIFO and
// sends the first chunk of up to 12 bytes.  If more bytes need to be written
// this will be handled by the IRQ handler, which therefore must be enabled.
//
// \return None.
//
//*****************************************************************************
void
TRF79x0FIFOWrite(unsigned char const *pucData, unsigned int uiLength)
{
    //
    // Set up TX state to send the buffer.
    //
    g_sTXState.pucBuffer = pucData;
    g_sTXState.uiBytesRemaining = uiLength;

    //
    // This will start transmission and write the first couple byte (12 at
    // most) to the FIFO.  If more bytes are to be written then the IRQ handler
    // will pick up and send the remainder.
    //
    FIFOTransmitSomeBytes(12);
    return;
}

//*****************************************************************************
//
// Writes to the FIFO, starting a transmission by the RF front end.
//
// \param pucData is a pointer to the data buffer to be written.
// \param uiLength is the number of bytes to send.
// \param uiBits is the additional number of bits to send.
//
// This function sets up the TX length byte registers 0x1D and 0x1E with
// the given bytes and bits and then calls TRF79x0FIFOWrite() to initiate the
// write to the FIFO.
// If the RF front end has been enabled for transmission with
// TRF79x0DirectCommand() with parameter \b TRF79X0_TRANSMIT_NO_CRC_CMD or
// \b TRF79X0_TRANSMIT_CRC_CMD this function call will start the radio
// transmission.
//
// \return None.
//
//*****************************************************************************
void
TRF79x0Transmit(unsigned char const *pucData, unsigned int uiLength,
                unsigned int uiBits)
{
    unsigned char pucLengthRegs[2];

    //
    // Prepare the length to be written into the FIFO for registers 0x1D and
    // 0x1E.
    //
    pucLengthRegs[0] = (uiLength >> 4) & 0xff;
    pucLengthRegs[1] = (uiLength & 0xf) << 4;

    if(uiBits > 0)
    {
        //
        // Last byte is incomplete.
        //
        pucLengthRegs[1] |= ((uiBits & 0x7) << 1) | 1;

        //
        // This is an additional byte, so increase the number of bytes for the
        // purpose of SPI transmission below by 1.
        //
        uiLength++;
    }

    //
    // The data from pucLengthRegs is written to registers 0x1D and 0x1E
    // in continuous mode.  In principle the continuous mode could simply
    // be kept active in order to write to the FIFO (starts at 0x1F).  However
    // there is a necessary workaround when only one byte needs to be
    // transmitted (see SLOA140).  Also stopping the continuous write here and
    // separately enabling it in TRF79x0WriteFIFO makes for more logical
    // function separation.
    //
    if(RF_DAUGHTER_TRF7960)
    {
        SSITRF79x0WriteContinuousStart(TRF79X0_TX_LENGTH_BYTE1_REG);
        SSITRF79x0WriteContinuousData(pucLengthRegs, sizeof(pucLengthRegs));
        SSITRF79x0WriteContinuousStop();
    }

    if(RF_DAUGHTER_TRF7970)
    {
        SSITRF79x0WriteContinuousData(pucLengthRegs, sizeof(pucLengthRegs));
    }

    TRF79x0FIFOWrite(pucData, uiLength);
}

//*****************************************************************************
//
// Sets up reception from the FIFO
//
// \param pucData is a pointer to the data buffer to receive the data.
// \param puiLength is a pointer to the length of the \e pucData buffer in
// bytes.
//
// This function sets up g_sRXState for the read operation from the FIFO.  The
// actual reading will be handled by the IRQ handler, which therefore must
// be enabled.  When the function returns the \e puiLength parameter will
// contain the number of bytes that were actually received.  These values are
// updated asynchronously by the IRQ handler.
//
// \return None.
//
//*****************************************************************************
void
TRF79x0Receive(unsigned char *pucData, unsigned int *puiLength)
{
    unsigned int uiMaxLength;

    uiMaxLength = *puiLength;

    //
    // Already received: 0 bytes.
    //
    *puiLength = 0;

    //
    // The uiMaxLength member is the ultimate deciding factor on whether the
    // IRQ receiver is enabled.  So set it to 0 first and only set it to its
    // final value when the other members are set.
    //
    g_sRXState.uiMaxLength = 0;

    g_sRXState.pucBuffer = pucData;
    g_sRXState.puiLength = puiLength;
    g_sRXState.uiMaxLength = uiMaxLength;
}

//*****************************************************************************
//
// Sets up reception from the FIFO with wait time out feature
//
// \param pucData is a pointer to the data buffer to receive the data.
// \param puiLength is a pointer to the length of the \e pucData buffer in
// bytes.
//
// This function sets up g_sRXState for the read operation from the FIFO. The
// actual reading will be handled by the IRQ handler, which therefore must
// be enabled.  When the function returns the \e puiLength parameter will
// contain the number of bytes that were actually received.  These values are
// updated asynchronously by the IRQ handler.
//
// \return None.
//
//*****************************************************************************
void
TRF79x0ReceiveAgain(unsigned char *pucRXBuf, unsigned int *puiRXLen)
{
    if((pucRXBuf != 0) && (puiRXLen != 0) && (*puiRXLen > 0))
        TRF79x0Receive(pucRXBuf, puiRXLen);

    TRF79x0IRQWaitTimeout(TRF79X0_WAIT_RXEND, TRF79X0_RX_TIMEOUT);

    //
    // Abort receive job, e.g. if timeout reached.
    //
    g_sRXState.uiMaxLength = 0;
}

//*****************************************************************************
//
//
//
//*****************************************************************************
void
TRF79x0ReceiveEnd(void)
{
    TRF79x0IRQClearCauses(TRF79X0_WAIT_RXEND);

    //
    // Abort receive job, e.g. if timeout reached.
    //
    g_sRXState.uiMaxLength = 0;

    TRF79x0ResetFifoCommand();
}

//*****************************************************************************
//
// Coordinated transmission and reception function.
//
// \param pucTXBuf is a pointer to the data buffer.
// \param uiTXLen is the number of full bytes to send.
// \param uiTXBits is the number of additional bits to send
// \param pucRXBuf is a pointer to a data buffer to receive data.  If this is
// \b 0 then no reception will take place.
// \param puiRXLen is pointer that inputs the length of \e pucRXBuf and outputs
// the number of bytes that were actually received.
// \param puiRXBits is unused.
// \param uiFlags is a bitfield of uiFlags to modify the transceiver operation.
// Should contain at least \b TRF79X0_TRANSCEIVE_NO_CRC,
// \b TRF79X0_TRANSCEIVE_RX_CRC, \b TRF79X0_TRANSCEIVE_TX_CRC or
// \b TRF79X0_TRANSCEIVE_CRC.  These values indicate whether a CRC should be
// added when transmitting (\b TRF79X0_TRANSCEIVE_TX_CRC or
// \b TRF79X0_TRANSCEIVE_CRC) and whether it should be checked when receiving
// (\b TRF79X0_TRANSCEIVE_RX_CRC or \b TRF79X0_TRANSCEIVE_CRC).
//
// This function calls, in order:
//
// - TRF79x0WriteRegister() to set up reception with/without CRC (in
//   register 0x1),
// - TRF79x0DirectCommand() with \b TRF79X0_RESET_FIFO_CMD to clear the FIFO,
// - TRF79x0DirectCommand() with \b TRF79X0_TRANSMIT_CRC_CMD or
//   \b TRF79X0_TRANSMIT_NO_CRC_CMD to prepare transmission with/without CRC,
// - TRF79x0IRQClearAll() to clear the IRQ state,
// - TRF79x0GetCollisionPosition() to clear the stored collision position,
// - TRF79x0Receive() to set up reception (if enabled),
// - TRF79x0Transmit() to set up transmission,
// - TRF79x0IRQWaitTimeout() with \b TRF79X0_WAIT_TXEND to wait for the
//   end of transmission and
// - TRF79x0IRQWaitTimeout() with \b TRF79X0_WAIT_RXEND to wait for the
//   end of reception (if enabled).
//
// The uiFlags and puiRXBits parameters offer for future, source-compatible
// extensions such as integrated collision handling (which would result in
// incomplete byte reception).
//
// \return None.
//
//*****************************************************************************
void
TRF79x0Transceive(unsigned char const *pucTXBuf, unsigned int uiTXLen,
                  unsigned int uiTXBits, unsigned char *pucRXBuf,
                  unsigned int *puiRXLen, unsigned int *puiRXBits,
                  unsigned int uiFlags)
{
    int iRXEnabled;
    unsigned char ucISOState;
    unsigned char ucBuf[30];

    ucISOState = TRF79x0ReadRegister(TRF79X0_ISO_CONTROL_REG);

    if(uiFlags & TRF79X0_TRANSCEIVE_RX_CRC)
    {
        //
        // Receive with CRC.
        //
        TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG,
                             ucISOState & ~TRF79X0_ISO_CONTROL_RX_CRC_N);
    }
    else
    {
        //
        // Receive without CRC.
        //
        TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG,
                             ucISOState | TRF79X0_ISO_CONTROL_RX_CRC_N);
    }

    if(RF_DAUGHTER_TRF7960)
    {
        TRF79x0DirectCommand(TRF79X0_RESET_FIFO_CMD);

        if(uiFlags & TRF79X0_TRANSCEIVE_TX_CRC)
        {
            //
            // Transmit with CRC.
            //
            TRF79x0DirectCommand(TRF79X0_TRANSMIT_CRC_CMD);
        }
        else
        {
            //
            // Transmit without CRC.
            //
            TRF79x0DirectCommand(TRF79X0_TRANSMIT_NO_CRC_CMD);
        }

        //
        // Disable any possible old receive job.
        //
        g_sRXState.uiMaxLength = 0;

        //
        // Clear all IRQ causes.
        //
        TRF79x0IRQClearAll();

        //
        // Clear stored collision position.
        //
        TRF79x0GetCollisionPosition();

        //
        // If receive is enabled, set up receive job.
        //
        iRXEnabled = 0;

        if((pucRXBuf != 0) && (puiRXLen != 0) && (*puiRXLen > 0))
        {
            TRF79x0Receive(pucRXBuf, puiRXLen);
            iRXEnabled = 1;
        }

        //
        // Writing the FIFO starts the transmission.  This function will return
        // after writing up to 12 bytes with the remaining bytes to be written
        // by the interrupt handler.
        //
        TRF79x0Transmit(pucTXBuf, uiTXLen, uiTXBits);

        //
        // Wait for the interrupt handler to signal the end of transmission
        // with no further FIFO loading.  This IRQ should always happen, so
        // no timeout necessary.  However, for robustness reasons: Use the RX
        // timeout.
        //
        TRF79x0IRQWaitTimeout(TRF79X0_WAIT_TXEND, TRF79X0_RX_TIMEOUT);

        //
        // If receive is enabled, wait for receive end.
        //
        if(iRXEnabled)
        {
            TRF79x0IRQWaitTimeout(TRF79X0_WAIT_RXEND, TRF79X0_RX_TIMEOUT);

            //
            // Abort receive job, e.g. if timeout reached.
            //
            g_sRXState.uiMaxLength = 0;
        }
    }

    if(RF_DAUGHTER_TRF7970)
    {
        //
        // Prepare SELECT command
        //
        ucBuf[0] = TRF79X0_CONTROL_CMD | TRF79X0_RESET_FIFO_CMD;

        if(uiFlags & TRF79X0_TRANSCEIVE_TX_CRC)
        {
            //
            // Transmit with CRC.
            //
            ucBuf[1] = TRF79X0_CONTROL_CMD | TRF79X0_TRANSMIT_CRC_CMD;
        }
        else
        {
            //
            // Transmit without CRC.
            //
            ucBuf[1] = TRF79X0_CONTROL_CMD | TRF79X0_TRANSMIT_NO_CRC_CMD;
        }

        //
        // Disable any possible old receive job.
        //
        g_sRXState.uiMaxLength = 0;

        //
        // Clear all IRQ causes.
        //
        TRF79x0IRQClearAll();

        //
        // Clear stored collision position.
        //
        TRF79x0GetCollisionPosition();

        //
        // If receive is enabled, set up receive job.
        //
        iRXEnabled = 0;

        if((pucRXBuf != 0) && (puiRXLen != 0) && (*puiRXLen > 0))
        {
            TRF79x0Receive(pucRXBuf, puiRXLen);
            iRXEnabled = 1;
        }

        //
        // Writing the FIFO starts the transmission.  This function will return
        // after writing up to 12 bytes with the remaining bytes to be written
        // by the interrupt handler.
        //

        //
        // Look into what is ucBuf being used for.
        //
        ucBuf[2] = 0x3D;

        //
        // Send the data in a continuous write to the FIFO "register".
        //
        SSITRF79x0WriteDirectContinuousStart();
        SSITRF79x0WriteContinuousData(ucBuf, 3);
        TRF79x0Transmit(pucTXBuf, uiTXLen, uiTXBits);

        //
        // Wait for the interrupt handler to signal the end of transmission
        // with no further FIFO loading.  This IRQ should always happen, so
        // no timeout necessary.  However, for robustness reasons: Use the RX
        // timeout.
        //
        TRF79x0IRQWaitTimeout(TRF79X0_WAIT_TXEND, TRF79X0_RX_TIMEOUT);

        //
        // If receive is enabled, wait for receive end.
        //
        if(iRXEnabled)
        {
            TRF79x0IRQWaitTimeout(TRF79X0_WAIT_RXEND, TRF79X0_RX_TIMEOUT);

            //
            // Abort receive job, e.g. if timeout reached.
            //
            g_sRXState.uiMaxLength = 0;
        }
    }
}
