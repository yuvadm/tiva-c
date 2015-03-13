//*****************************************************************************
//
// directmode.h - Direct mode communications.
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
#include "inc/hw_timer.h"
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"
#include "driverlib/interrupt.h"
#include "ssitrf79x0.h"
#include "trf79x0_hw.h"
#include "directmode.h"
#include "trf79x0.h"
#include "iso14443a.h"

#if defined(rvmdk)
#define inline __inline
#endif

//*****************************************************************************
//
// Direct mode 0 implementation for ISO 14443 A.
//
// This file implements transmission of raw ISO 14443-2 modulation type A
// formatted bit streams at ~106kbit/s on the TRF79x0 in direct mode 0.
// The functionality will generate and receive the correct SOF and EOF markers
// but everything else (parity and CRC) is the responsibility of the calling
// code.  iso14443.c has functions ISO14443ACalculateParity()/
// ISO14443ACheckParity()/ ISO14443ACalculateCRC()/ ISO14443ACheckCRC() for
// this purpose.  Since it transmits and receives raw bit streams it can also
// be used for MIFARE Classic communication which needs incorrect parity bits.
//
// The implementation uses one timer (TIMER 0) for timing, so this can not
// be used by anything else, or at least must be set up again before each
// use, with DirectModeInit().
//
// DirectModeEnable() and DirectModeDisable() keep track of state and will
// not re-enable the mode if it was already active.  DirectModeIsEnabled()
// can be used to query the state.  While direct mode is active no other
// functionality on the TRF79x0 should be accessed and its IRQ is disabled.
//
// \note DirectModeDisable() implements a workaround for an apparent bug in
// the TRF7960 which will perform a soft reset of the TRF7960.  In order to
// not leave the chip in an entirely unexpected state it will then call
// ISO14443ASetupRegisters() to prepare the chip for ISO 14443 A operation
// (which is most likely what you'll be using together with this code).  If
// you do not want ISO 14443 A operation you need to restore the necessary
// settings yourself.
//
//*****************************************************************************

//
// Keep track whether direct mode is enabled.
//
static int g_iDirectModeEnabled = 0;

//
// Receive timeout.  This is a loop count, not as reliable as SysCtlDelay(),
// but not really critical.
//
#define DIRECTMODE_RECEIVE_TIMEOUT 30000

//
// Use timer 0 for direct mode timing.
//
#define DIRECTMODE_TIMER_PORT   TIMER0_BASE
#define DIRECTMODE_TIMER_SYSCTL SYSCTL_PERIPH_TIMER0

//*****************************************************************************
//
// The macros below do exactly the same as GPIOPinWrite() and GPIOPinRead()
// from gpio.c and TimerIntStatus() and TimerIntClear() from timer.c, just
// without the function call and with compile time argument optimization.
//
//*****************************************************************************
#define GPIOPinWrite(ulPort, ucPins, ucVal) \
    (HWREG((ulPort) + (GPIO_O_DATA + ((ucPins) << 2))) = (ucVal))

#define GPIOPinRead(ulPort, ucPins) \
    (HWREG((ulPort) + (GPIO_O_DATA + ((ucPins) << 2))))

#define TimerIntStatus(ulBase, bMasked) \
    ((bMasked) ? HWREG((ulBase) + TIMER_O_MIS) : \
           HWREG((ulBase) + TIMER_O_RIS))

//*****************************************************************************
//
// Shortcut to mimic TimerIntClear() function in DriverLib without the call
// overhead.
//
//*****************************************************************************
#define TimerIntClear(ulBase, ulIntFlags)                                     \
    HWREG((ulBase) + TIMER_O_ICR) = (ulIntFlags)

//*****************************************************************************
//
// Set timer value.
// This function is missing from the StellarisWare timer API, so here it is
// as a macro
//
//*****************************************************************************
#define TimerValueSet(ulBase, ulTimer, ulValue)                               \
    HWREG((ulBase) + ((ulTimer)==TIMER_A ? TIMER_O_TAV : TIMER_O_TBV)) =      \
        (ulValue)

//*****************************************************************************
//
// This macro enables modulation/disables the field.
//
//*****************************************************************************
#define MODOn()                                                               \
    GPIOPinWrite(TRF79X0_MOD_BASE, TRF79X0_MOD_PIN,                           \
                 TRF79X0_MOD_PIN)

//*****************************************************************************
//
// This macro disables modulation/enables the field.
//
//*****************************************************************************
#define MODOff()                                                              \
    GPIOPinWrite(TRF79X0_MOD_BASE, TRF79X0_MOD_PIN, 0)

//*****************************************************************************
//
// Waits for the next one-eighth bit interval, depends on timer B being set up
// for one-eighth bit intervals.
//
//*****************************************************************************
#define WaitEighthBit()                                                       \
{                                                                             \
    while(!(TimerIntStatus(DIRECTMODE_TIMER_PORT, 0) & TIMER_TIMB_TIMEOUT))   \
    {                                                                         \
    };                                                                        \
                                                                              \
    TimerIntClear(DIRECTMODE_TIMER_PORT, TIMER_TIMB_TIMEOUT);                 \
}

//*****************************************************************************
//
// Waits for the next quarter bit interval, depends on timer A being set up
// for quarter bit intervals.
//
//*****************************************************************************
#define WaitQuarterBit()                                                      \
{                                                                             \
    while(!(TimerIntStatus(DIRECTMODE_TIMER_PORT, 0) & TIMER_TIMA_TIMEOUT))   \
    {                                                                         \
    }                                                                         \
    TimerIntClear(DIRECTMODE_TIMER_PORT, TIMER_TIMA_TIMEOUT);                 \
}

//*****************************************************************************
//
// Modulation sequences, names from ISO 14443-2.
//
// All these sequences end at 0.75 bit period and start
// somewhere before 1 bit period.  This way they can be freely combined
// for an overall rate of one sequence per bit period, and give less than
// 0.25 bit periods for computation.
//
//*****************************************************************************

//*****************************************************************************
//
// X: pulse after half-bit.
//
// - Wait 1/4 bit period for previous sequence to get to the start of this bit.
// - Wait 1/4 bit period.
// - Wait 1/4 bit period to get to 1/2 bit period.
// - Set MOD bit active.
// - Wait 1/4 bit period to get to 3/4 bit period.
// - Set MOD bit inactive.
//
//*****************************************************************************
#define SequenceX()                                                           \
    WaitQuarterBit();                                                         \
    WaitQuarterBit();                                                         \
    WaitQuarterBit();                                                         \
    MODOn();                                                                  \
    WaitQuarterBit();                                                         \
    MODOff();

//*****************************************************************************
//
// Y: This sequence just waits out a full bit period with no other toggle.
//
// - Wait 1/4 bit period for previous sequence to get to the start of this bit.
// - Wait 1/4 bit period.
// - Wait 1/4 bit period to get to 1/2 bit period.
// - Wait 1/4 bit period to get to 3/4 bit period.
//
//*****************************************************************************
#define SequenceY()                                                           \
    WaitQuarterBit();                                                         \
    WaitQuarterBit();                                                         \
    WaitQuarterBit();                                                         \
    WaitQuarterBit();

//*****************************************************************************
//
// Z: Mode pulse at start of bit period.
//
// - Wait 1/4 bit period for previous sequence to get to the start of this bit.
// - Set MOD bit active.
// - Wait 1/4 bit period.
// - Set MOD bit inactive.
// - Wait 1/4 bit period to get to 1/2 bit period.
// - Wait 1/4 bit period to get to 3/4 bit period.
//
//*****************************************************************************
#define SequenceZ()                                                           \
    WaitQuarterBit();                                                         \
    MODOn();                                                                  \
    WaitQuarterBit();                                                         \
    MODOff();                                                                 \
    WaitQuarterBit();                                                         \
    WaitQuarterBit();

//*****************************************************************************
//
// Set up timers and GPIO port for direct mode operation.
//
// This sets up GPTM 0 timer A for quarter bit periods (used in sending)
// and timer B for one-eighth bit periods (used in receiving).
//
//*****************************************************************************
void
DirectModeInit(void)
{
    //
    // Enable GPIO port A for bit-banging receive.
    //
    SysCtlPeripheralEnable(TRF79X0_RX_PERIPH);
    SysCtlPeripheralEnable(TRF79X0_EN_PERIPH);
    SysCtlPeripheralEnable(TRF79X0_MOD_PERIPH);
    SysCtlPeripheralEnable(TRF79X0_IRQ_PERIPH);

    //
    // Enable and configure timer in periodic up mode
    //
    SysCtlPeripheralEnable(DIRECTMODE_TIMER_SYSCTL);
    TimerConfigure(DIRECTMODE_TIMER_PORT,
                   TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC_UP |
                   TIMER_CFG_B_PERIODIC_UP);

    //
    // Configure timer max value for an fc/32 = 13.56MHz/32 = quarter bit
    // at ~106kHz.  This means that the timer must count up to
    // SysClk/(13.56MHz/32) = (32*SysClk)/13.56MHz.  This comes down to 117.99
    // at 50MHz.  The error at 50MHz is negligible, but at other frequencies or
    // in the general case a fractional logic might be needed.
    // Note that the argument for TimerLoadSet is actually the desired divisor
    // minus 1.  117.99 would round to 118, so the argument must be 117.
    // However since integer calculation is truncating and not rounding this is
    // directly the result of the division.  Should a different frequency be
    // used where the result of the division is not also the rounded result of
    // the division minus 1 then proper rounding logic must be added.
    //
    TimerLoadSet(DIRECTMODE_TIMER_PORT, TIMER_A,
                 ((SysCtlClockGet() * 32) / 13560000));

    //
    // Configure Timer B for fc/16 = one eighth bit at ~106kHz.  Same
    // considerations as above apply.
    //
    TimerLoadSet(DIRECTMODE_TIMER_PORT, TIMER_B,
                 ((SysCtlClockGet() * 16) / 13560000));
}

//*****************************************************************************
//
// Dual use send code for direct mode.  Can either accept an opaque bit stream
// ( (iMode && DIRECT_MODE_SEND_MASK) == DIRECT_MODE_SEND_OPAQUE ) or
// structured bytes with parity (... DIRECT_MODE_SEND_PARITY), e.q. as
// parity_data_t.  In the first case uiBytes gives the number of opaque 8
// bit units to send (e.g. sizeof(*pvBuffer) == uiBytes + (uiBits > 0 ?
// 1 : 0) ), in the second case it's the number of logical bytes (since each
// logical byte is encoded as a 16bit word the buffer size must be twice as
// big).  In both cases uiBits gives the number of least significant bits
// that should additionally be sent.
//
//*****************************************************************************
static inline void
DirectModeSend(int iMode, void const *pvBuffer, unsigned int uiBytes,
               unsigned int uiBits)
{
    //
    // We'll keep the current pointer as an 8-bit value and the current byte as
    // an 16-bit value in any case.  In parity mode we'll arrange the pointer
    // movement and uiCurrentByte assignment specially.
    //
    unsigned char const *pucCurrent;
    unsigned short usPos, usCurrentByte;
    unsigned char ucLastBit, ucCurrentBit, ucBitsRemain;

    //
    // Initialize the byte and bit position.
    //
    usPos = 0;
    ucLastBit = 0;

    iMode = iMode & DIRECT_MODE_SEND_MASK;

    //
    // Create a byte pointer to use with the rest of this function.
    //
    pucCurrent = pvBuffer;

    //
    // Set the MOD pin inactive.
    //
    MODOff();

    //
    // Start the timer.
    //
    TimerEnable(DIRECTMODE_TIMER_PORT, TIMER_A);

    //
    // SOF.
    //
    SequenceZ();

    while(usPos++ < uiBytes)
    {
        //
        // Prepare the bit counter and value for this byte for either
        // 8 bits per byte or 9 bits per 16 bit word.
        //
        if(iMode == DIRECT_MODE_SEND_OPAQUE)
        {
            ucBitsRemain = 8;
            usCurrentByte = *pucCurrent;
        }
        else
        {
            ucBitsRemain = 9;
            usCurrentByte = pucCurrent[0] | (pucCurrent[1] << 8);
        }

        //
        // Send the bits of this byte.
        //
        do
        {
            ucCurrentBit = usCurrentByte & 0x1;

            if(ucCurrentBit)
            {
                //
                // Transfer a 1 Bit.
                //
                SequenceX();
            }
            else
            {
                //
                // Transfer a 0-Bit, encoded differently depending on if this
                // was the last bit.
                //
                if(ucLastBit)
                {
                    SequenceY();
                }
                else
                {
                    SequenceZ();
                }
            }

            //
            // Shift to next bit.
            //
            usCurrentByte >>= 1;

            ucLastBit = ucCurrentBit;
        }
        while(--ucBitsRemain > 0);

        //
        // Increment the data pointer by either a byte or one 16 bit word.
        //
        pucCurrent += (iMode == DIRECT_MODE_SEND_OPAQUE) ? 1 : 2;
    }

    //
    // This is the same as above for the possibly remaining fractional byte.
    //
    if(uiBits > 0)
    {
        ucBitsRemain = uiBits;
        usCurrentByte = *pucCurrent;

        //
        // If sending parity then or in the parity.
        //
        if(iMode == DIRECT_MODE_SEND_PARITY)
        {
            usCurrentByte |= pucCurrent[-1] << 8;
        }

        do
        {
            ucCurrentBit = usCurrentByte & 0x1;

            //
            // Transfer a 1 Bit.
            //
            if(ucCurrentBit)
            {
                SequenceX();
            }
            else
            {
                //
                // Transfer a 0-Bit, encoded differently depending on if this
                // was the last bit.
                //
                if(ucLastBit)
                {
                    SequenceY();
                }
                else
                {
                    SequenceZ();
                }
            }

            //
            // Shift to next bit.
            //
            usCurrentByte >>= 1;
            ucLastBit = ucCurrentBit;
        }
        while(--ucBitsRemain > 0);
    }

    //
    // EOF is either a 0 or a Y.
    //
    if(ucLastBit)
    {
        SequenceY();
    }
    else
    {
        SequenceZ();
    }

    SequenceY();

    //
    // Disable the timer and return.
    //
    TimerDisable(DIRECTMODE_TIMER_PORT, TIMER_A);
}

//*****************************************************************************
//
// Dual-use receive code for direct mode 0.  Similar to the send code can
// either output an opaque bitstream (DIRECT_MODE_RECV_OPAQUE), or bytes with
// associated parity bits (DIRECT_MODE_RECV_PARITY).
//
//*****************************************************************************
static void
DirectModeReceive(int iMode, void *pvBuffer, unsigned int *puiBytes,
                  unsigned int *puiBits)
{
    unsigned int uiMaxBytes, uiCountBytes, uiCountBits;
    int iCurrentBitVal, iLastBitVal, iCount, iHaveSOF;
    unsigned char *pucCurrent;
    unsigned int uiCurrentByte;
    unsigned int uiBitsRemain;
    int iTimeout;

    //
    // Signal description: The input on MISO will start out low
    // and then change to the sub carrier data stream which is either
    // high, or high-low-high with a frequency of 848kHz.  Exactly one
    // half bit will be all high and one half bit will be alternating.
    //

    // Reception methodology: Use the IRQ logic as an edge detector.
    // Configure the GPIO pin for edge triggered interrupts (the interrupt
    // will not actually be enabled, so no handler will be called).  Clear
    // the interrupt before each sampling interval and check its unmasked
    // status afterwards.
    //
    // The reception may not be perfectly aligned to the bit clock, in that
    // case the edges will dominate the high signal, e.g. even if there is
    // just one edge in a sampling period the complete period will read as
    // "edges present".  Look for changes in the sampling result to decode the
    // manchester encoded stream: there will be a change in the middle of each
    // bit (and the direction of that change signifies the bit value) and there
    // might be change at the start/end of a bit.  One bit is 8 sampling
    // periods, so expected is a change every 8 periods.  If a change occurs
    // after 4 periods this is at the start/end of a bit and should be ignored
    // (and the counter kept incrementing).  When keeping in mind that the
    // subcarier edges may dominate the steady signal that means that there
    // must have been at least 7 periods since a recognized edge to recognize a
    // subcarrier-steady edge as a data edge, or 6 periods since a recognized
    // edge to recognize a steady-subcarrier edge as a data edge.
    //

    //
    // Pointer to the next storage location.
    //
    pucCurrent = pvBuffer;

    //
    // Currently sampled data unit (either 8 or 9 bits).
    //
    uiCurrentByte = 0;

    //
    // Set up edge detection.
    //
    GPIOIntTypeSet(TRF79X0_RX_BASE, TRF79X0_RX_PIN, GPIO_BOTH_EDGES);

    //
    // Make sure that data parameters are correct before using them.
    //
    if((pvBuffer == NULL) || (puiBytes == NULL) || (*puiBytes == 0))
    {
        return;
    }

    //
    // Maximal number of bytes to receive, and count of bytes and count of bits
    // received so far.
    //
    uiMaxBytes = *puiBytes;
    uiCountBytes = 0;
    uiCountBits = 0;

    //
    // iCurrentBitVal contains the sampling result for the most recently ended
    // sampling interval, while iLastBitVal is for the interval before that.
    // Edges are detected by having iCurrentBitVal != iLastBitVal.
    //
    iCurrentBitVal = 0;
    iLastBitVal = 0;

    //
    // iCount contains the number of quarter bit intervals since the last
    // recognized data edge.  It is initialized with a half bit period
    // at the start to immediately detect the data edge in the middle of
    // the SOF bit, and afterwards incremented for each sampling period and
    // reset to 0 when a data edge is detected.  When an edge is ignored count
    // will also be set to exactly a half bit period in order to guarantee
    // that the next edge will be detected as a data edge.
    //
    iCount = 4;

    iMode = iMode & DIRECT_MODE_RECV_MASK;

    //
    // Initialized the number of bits left in the data unit.
    //
    if(iMode == DIRECT_MODE_RECV_OPAQUE)
    {
        uiBitsRemain = 8;
    }
    else
    {
        uiBitsRemain = 9;
    }

    //
    // Ignore the first bit which is a start-of-frame indicator.
    //
    iHaveSOF = 0;

    //
    // The signal starts out low, so wait for the rising edge.
    //
    GPIOIntClear(TRF79X0_RX_BASE, TRF79X0_RX_PIN);
    {
        //
        // Initialize the timeout.
        //
        iTimeout = DIRECTMODE_RECEIVE_TIMEOUT;

        while(!(GPIOIntStatus(TRF79X0_RX_BASE, 0) &
                TRF79X0_RX_PIN) && (iTimeout-- > 0))
        {
        }
    }

    //
    // Set the timer to 0 and start it.
    //
    TimerValueSet(DIRECTMODE_TIMER_PORT, TIMER_B, 0);
    TimerEnable(DIRECTMODE_TIMER_PORT, TIMER_B);

    //
    // Reset edge detector.
    //
    GPIOIntClear(TRF79X0_RX_BASE, TRF79X0_RX_PIN);

    do
    {
        //
        // Wait until the end of the current sampling interval.
        //
        WaitEighthBit();

        //
        // Copy over the sampling result to be processed, reset edge detector.
        //
        iCurrentBitVal = (GPIOIntStatus(TRF79X0_RX_BASE, 0) &
                          TRF79X0_RX_PIN);

        GPIOIntClear(TRF79X0_RX_BASE, TRF79X0_RX_PIN);

        //
        // Check for a change in bit polarity.
        //
        if(iLastBitVal != iCurrentBitVal)
        {
            if(iLastBitVal)
            {
                //
                // may be overly long.
                //
                if(iCount <= 6)
                {
                    //
                    // ignore, but force iCount to sane value.
                    //
                    iCount = 4;
                }
                else
                {
                    if(iHaveSOF)
                    {
                        //
                        // This edge is a 1 bit, add it to the current data
                        // unit.
                        //
                        uiBitsRemain--;

                        uiCurrentByte |= 1 << uiCountBits;

                        uiCountBits++;
                    }
                    else
                    {
                        iHaveSOF = 1;
                    }

                    //
                    // Reset iCount.
                    //
                    iCount = 0;
                }
            }
            else
            {
                //
                // may be overly short
                //
                if(iCount <= 5)
                {
                    //
                    // ignore, but force iCount to sane value.
                    //
                    iCount = 4;
                }
                else
                {
                    if(iHaveSOF)
                    {
                        //
                        // This edge is a 0 bit, add it to the current data
                        // unit.
                        //
                        uiBitsRemain--;
                        uiCountBits++;
                    }
                    else
                    {
                        iHaveSOF = 1;
                    }

                    //
                    // Reset iCount.
                    //
                    iCount = 0;
                }
            }
        }

        //
        // Increment number of one eighth bit periods since last recognized
        // edge.
        //
        iCount++;
        iLastBitVal = iCurrentBitVal;

        if(uiBitsRemain == 0)
        {
            //
            // Store received data unit, advance pointer.
            //
            if(iMode == DIRECT_MODE_RECV_OPAQUE)
            {
                uiBitsRemain = 8;
                *pucCurrent = uiCurrentByte;
                pucCurrent += 1;
            }
            else
            {
                uiBitsRemain = 9;
                pucCurrent[0] = uiCurrentByte & 0xff;
                pucCurrent[1] = uiCurrentByte >> 8;
                pucCurrent += 2;
            }

            //
            // Clear temporary store.
            //
            uiCurrentByte = 0;
            uiCountBits = 0;

            //
            // Increment counter, abort when the receive buffer is full.
            //
            uiCountBytes++;
            if((uiCountBytes + 1) >= uiMaxBytes)
            {
                break;
            }
        }

        //
        // More than 2 bit periods (16 eighth bit periods) since the last edge
        // signify a time out, end of reception.
        //
    }
    while(iCount < 16);

    //
    // Stop timer.
    //
    TimerDisable(DIRECTMODE_TIMER_PORT, TIMER_B);

    //
    // Store length.
    //
    *puiBytes = uiCountBytes;

    if(puiBits != NULL)
    {
        if(uiCountBits > 0)
        {
            //
            // Store incomplete byte.
            //
            if(iMode == DIRECT_MODE_RECV_OPAQUE)
            {
                *pucCurrent = uiCurrentByte;
            }
            else
            {
                pucCurrent[0] = uiCurrentByte & 0xff;
                pucCurrent[1] = uiCurrentByte >> 8;
            }
        }

        //
        // Store length of incomplete byte.
        //
        *puiBits = uiCountBits;
    }
}

//*****************************************************************************
//
// Transmits and receives an ISO 14443-2 type A frame in direct mode 0.
//
// \param iMode is a flag field to specify the format of the input and output
// parameters.  Should be a combination of (either \b DIRECT_MODE_SEND_OPAQUE
// or \b DIRECT_MODE_SEND_PARITY) and (either \b DIRECT_MODE_RECV_OPAQUE or
// \b DIRECT_MODE_RECV_PARITY).  See discussion below.
// \param pvSendBuf is the data buffer to send.
// \param uiSendBytes determines the number of full data units to be sent (8
// or 9 bits each).  For a discussion of data unit sizes see below.
// \param uiSendBits determines how many bits from an additional, fractional
// data unit should be sent.  Setting this to a value other than 0 means that
// \e pvSendBuf has space for an \e uiSendBytes + 1 data units.
// \param pvRecvBuf is the data buffer for receiving.
// \param puiRecvBytes inputs the space available in \e pvRecvBuf (in logical
// data units) and outputs the number of full data units actually received
// \param puiRecvBits outputs the number of additional bits received after the
// last full data unit indicated in \e puiRecvBytes
//
// Both input and output can be in one of two formats: OPAQUE and PARITY.
//
// - \b OPAQUE specifies an opaque bit stream, where each byte in the input
//   corresponds to 8 bits sent on the radio interface and 8 bits received on
//   the radio interface correspond to 1 byte in the output.
// - \b PARITY has for each byte in the input/output an associated parity bit.
//   These are stored as a 16 bit word: the payload byte is in the lower 8 bits
//   and the parity bit is the least significant bit of the higher byte.
//
// The principal data unit size for OPAQUE is 8 bits, and the principal data
// unit size for PARITY is 9 bits (stored as a 16 bit word).  All inputs
// and outputs are in terms of data units, which means that the actual storage
// size, in bytes, for PARITY mode is twice the number of data units.
//
// In both modes additional bits can be sent or received after the last
// full data unit.  PARITY mode is best suited for ISO 14443 operation
// since it conveniently associates each byte with its parity bit, and
// allows for direct access to the payload byte of each data unit through
// simple masking, and not requiring shifts and masks over two bytes.
//
// Direct mode needs to have been enabled with DirectModeEnable() (with
// argument \e iMode = 0) before calling this function.  This function will
// disable the master processor interrupt while it is running.
//
//*****************************************************************************
void
DirectModeTransceive(int iMode, void const *pvSendBuf, unsigned int uiSendBytes,
                     unsigned int uiSendBits, void *pvRecvBuf,
                     unsigned int *puiRecvBytes, unsigned int *puiRecvBits)
{
    int iDisabled;

    //
    // Disable interrupts.
    //
    iDisabled = IntMasterDisable();

    //
    // Send and receive
    //
    DirectModeSend(iMode, pvSendBuf, uiSendBytes, uiSendBits);
    DirectModeReceive(iMode, pvRecvBuf, puiRecvBytes, puiRecvBits);

    //
    // Enable interrupts if necessary.
    //
    if(iDisabled == 0)
    {
        IntMasterEnable();
    }
}

//*****************************************************************************
//
// Starts direct mode.
//
// \param iMode is the direct mode to enable and must be 0 for now.
//
// This function sets the desired direct mode type on the TRF79x0 and then
// enables direct mode.  This also has the effect of disabling the
// TRF79x0 IRQ.  No TRF79x0 operation can be performed while direct mode is
// active (and none should be attempted).
// The function sets an internal flag and does nothing if direct mode has
// already been enabled by this function and not been disabled with
// DirectModeDisable().
//
//*****************************************************************************
void
DirectModeEnable(unsigned int iMode)
{
    unsigned char pucRegs[3];

    //
    // Check to see if direct mode is already enabled, and if so, do nothing
    //
    if(g_iDirectModeEnabled)
    {
        return;
    }

    //
    // Read chip status control and ISO registers.
    //
    TRF79x0ReadRegisterContinuous(TRF79X0_CHIP_STATUS_CTRL_REG, pucRegs, 2);

    //
    // Set direct mode type to bitstream.
    //
    if(iMode)
    {
        pucRegs[TRF79X0_ISO_CONTROL_REG] |= TRF79X0_ISO_CONTROL_DIR_MODE;

    }
    else
    {
        pucRegs[TRF79X0_ISO_CONTROL_REG] &= ~TRF79X0_ISO_CONTROL_DIR_MODE;
    }

    //
    // Enable direct mode in saved registers.
    //
    pucRegs[0] |= TRF79X0_STATUS_CTRL_DIRECT;

    //
    // Write direct mode type to TRF79x0.
    //
    TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG, pucRegs[1]);

    //
    // Clear pucRegs[2]
    //
    pucRegs[2] = 0;

    //
    // Start direct mode
    // This write will not finish (which would end direct mode) but instead
    // must be finished with TRF79x0DirectModeDisable().  Also the IRQ handler
    // has been deactivated while the chip select is asserted since it can't
    // use the SPI anyway.
    //
    SSITRF79x0WriteContinuousStart(TRF79X0_CHIP_STATUS_CTRL_REG);
    SSITRF79x0WriteContinuousData(pucRegs, 1);

    //
    // Delay 8 dummy clock cycles
    //
    SSITRF79x0DummyWrite(&pucRegs[2], 1);

    //
    // Set up GPIO configuration: Use the input (normally MISO) as a GPIO to
    // bit-bang the reception of the sub-carrier signal
    //
    GPIOPinTypeGPIOInput(TRF79X0_RX_BASE, TRF79X0_RX_PIN);

    //
    // Set flag
    //
    g_iDirectModeEnabled = 1;
}

//*****************************************************************************
//
// Stops direct mode.
//
// This stops the direct mode and releases the communication interface.
// It checks an internal flag and does nothing if direct mode has not been
// enabled with DirectModeEnable() or has been disabled with
// DirectModeDisable() before.
//
// \note There seems to be a bug in the TRF7960 which makes the chip unusable
// for some time after exiting direct mode due to the MISO line not
// working properly.  Currently the required workaround is to send a
// \b TRF79X0_SOFT_INIT_CMD command and then reinitialize the chip, with
// ISO14443ASetupRegisters().  This is done by this function, so you'll
// find the TRF79x0 configured for ISO 14443-A even if it wasn't before.
//
//*****************************************************************************
void
DirectModeDisable(void)
{
    int iDisabled;

    //
    // Check to see if direct mode is enabled, and if not, do nothing.
    //
    if(!g_iDirectModeEnabled)
    {
        return;
    }

    //
    // Kludge: We want to prevent the IRQ handler from going off
    // before we have reinitialized the interface.  The call to
    // SSITRF79x0WriteContinuousStop(), and by extension all the
    // calls to TRF79x0DirectCommand or TRF79x0Read*, will enable the
    // IRQ, so we disable the processor IRQ for the time being.
    //
    iDisabled = IntMasterDisable();

    //
    // Restore SSI pin settings.
    //
    GPIOPinTypeSSI(TRF79X0_RX_BASE, TRF79X0_RX_PIN);

    //
    // Disable direct mode.
    //
    SSITRF79x0WriteContinuousStop();

    //
    // For good measure: Discard bytes from FIFO.
    //
    TRF79x0DirectCommand(TRF79X0_RESET_FIFO_CMD);

    //
    // Clear flag.
    //
    g_iDirectModeEnabled = 0;

    //
    // Re-enable processor IRQ if necessary.
    //
    if(iDisabled == 0)
    {
        IntMasterEnable();
    }

    //
    // Enable TRF IRQ.
    //
    TRF79x0InterruptEnable();

    //
    // This code should be removed if a better solution is found since
    // the direct mode code should not directly depend on ISO 14443-A
    // and there might, hypothetically, be other protocols that the user
    // might want to use.
    //
    TRF79x0DirectCommand(TRF79X0_SOFT_INIT_CMD);
    ISO14443ASetupRegisters();
    ISO14443APowerOn();
}

//*****************************************************************************
//
// Queries whether direct mode is enabled.
//
// \return A non-zero value indicates that direct mode is enabled and a zero
// value indicates that direct mode is disabled.
//
//*****************************************************************************
int
DirectModeIsEnabled(void)
{
    return(g_iDirectModeEnabled);
}
