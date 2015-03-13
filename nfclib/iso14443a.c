//*****************************************************************************
//
// iso14443a.c - ISO 14443A implementation.
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

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "trf79x0.h"
#include "iso14443a.h"

//*****************************************************************************
//
// Global anti-collision state for use by ISO14443ASelectFirst() and
// ISO14443ASelectNext().
//
//*****************************************************************************
static struct ISO14443AAnticolState g_sAnticolState;

//*****************************************************************************
//
// ISO14443-A Anti-collision implementation, iterative depth-first tree search
// with optional backtracking.
//
// Usage:
// In ISO 14443 A there are two types of resting states for cards: IDLE and
// HALT.  A card enters IDLE state after powering up and performing all the
// necessary internal initialization.  The specification states that the card
// must be in IDLE state and ready to accept commands 5ms after being put into
// an unmodulated (e.g. no commands sent) field of the necessary strength.
//
// This pause is guaranteed by ISO14443APowerOn().
//
// During the selection and anti-collision phase cards will be in intermediary
// states (READY and READY*) but then always return to the original state
// (IDLE and HALT).
//
// After a card has been selected by any of the ISO14443ASelect* functions
// of this module it can be sent to the HALT state with ISO14443AHalt(),
// and must be sent to HALT (or deactivated in another way) before calling
// another ISO14443ASelect* function.
//
// Cards in the IDLE state react to both WUPA and REQA commands, cards in
// HALT state react only to WUPA commands.  Cards in HALT state can not
// return to IDLE state except through completely powering off the card
// and powering it up again, but the specification makes no claims as to how
// long the field must be off in order for the card to power off and this time
// will vary between card types.
//
// The ISO14443ASelectFirst/Next functions take one parameter (\e ucCmd) that
// must be \b ISO14443A_REQA or \b ISO14443A_WUPA to specify which wake up
// method to use.  ISO14443ASelect will always use WUPA.
//
// This leads to two main usage protocols:
// <h3>A: Detect only new cards</h3> <ul>
//  <li> Keep field enabled at all times
//  <li> Use ISO14443ASelectFirst() with ISO14443A_REQA to find new cards that
//     entered the field.  Note: Ensure a pause of 5ms before each call to
//     ISO14443ASelectFirst(), e.g. with ISO14443APowerOn().
//  <li> If a card was found by SelectFirst, operate on that card and
//     deactivate it with ISO14443AHalt().  Note: all successful calls to any
//     ISO14443ASelect* function should always be paired with a call to
//     ISO14443AHalt() before the next call to any ISO14443ASelect* function.
//  <li> Repeatedly call ISO14443ASelectFirst() with \b ISO14443A_REQA in a
//      loop.  It will only find new cards and not relist the cards that were
//      already handled and halted
//  </ul>
//  Pseudo C: <pre>
//  while(1) {
//      ISO14443APowerOn();
//      if(ISO14443ASelectFirst(ISO14443A_REQA, ...)) {
//
//          <i>Do something with the card</i>
//
//          ISO14443AHalt();
//      }
//
//      <i>Do NOT power off the field</i>
//  }
// </pre>
//
// <h3>B: List all cards in the field</h3> <ul>
//  <li> Optionally disable the field or do other things, but enable the
//      field at least for 5ms (e.g. with ISO14443APowerOn())
//  <li> Call ISO14443ASelectFirst() with \b ISO14443A_WUPA to find the first
//      card, handle it, call ISO14443AHalt().  If at least one card was
//      found, use ISO14443ASelectNext() with \b ISO14443A_WUPA in a loop to
//      find more cards, handle them and call ISO14443AHalt() on them.
//  <li> You may disable the field and restart the procedure at any time with
//      ISO14443APowerOn() and ISO14443ASelectFirst().  It will always list
//      all cards in the field, not only new cards.
// </ul>
//  Pseudo C: <pre>
//  while(1) {
//      ISO14443APowerOn();
//      if(ISO14443ASelectFirst(ISO14443A_WUPA, ...)) {
//          do {
//
//              <i>Do something with the card</i>
//
//              ISO14443AHalt();
//          } while(ISO14443ASelectNext(ISO14443A_WUPA, ...));
//      }
//
//      <i>You may power off the field here</i>
//  }
// </pre>
//
// In both cases ISO14443ASelect() can be used at any time (after halting a
// previously selected card) to select a card by known UID.
//
//*****************************************************************************

//*****************************************************************************
//
// This structure stores the UID that we're currently working on.
//
//*****************************************************************************
struct ISO14443AAnticolState
{
    //
    // This field stores the raw responses that the anti-collision is actually
    // performed over, e.g. 3 times 5 bytes.  Same goes for \e ucCollisions.
    // Before returning the UID to the calling code this must be cleaned,
    // that is remove cascade tag and BCC.
    //
    unsigned char ucUID[15];
    //
    // Stores the collision positions discovered so far.  It is a bit field
    // with the same indices as \e ucUID.
    //
    unsigned char ucCollisions[15];
    //
    // Stores the number of bits that we've successfully received or
    // disambiguated.  Note: Real count for the \e ucUID field of this
    // structure, not NVB format.  For example 8 means 1 byte and 0 bits, 40
    // means full cascade level 1, 41 means full cascade level 1 plus 1 bit in
    // cascade level 2.
    //
    unsigned int iBitPos;
};

//*****************************************************************************
//
// Set up registers for ISO 14443 A 106Kbit/s operation.  This function must
// be called after initializing the TRF79x0 (for example with TRF79x0Init()
// or TRF79x0DirectCommand() with argument \b TRF79X0_SOFT_INIT_CMD) and before
// calling any of the other ISO14443A functions.
//
//*****************************************************************************
void
ISO14443ASetupRegisters(void)
{
    //
    // Set the ISO format to ISO1443A 106Kbps.
    //
    TRF79x0WriteRegister(TRF79X0_ISO_CONTROL_REG,
                         TRF79X0_ISO_CONTROL_14443A_106K);

    //
    // Set the TX pulse to 106ns (0x20 * 73.7ns).
    //
    TRF79x0WriteRegister(TRF79X0_TX_PULSE_LENGTH_CTRL_REG, 0x20);

    //
    // Set the RX No response wait time to 529us (0xe * 37.76us).
    //
    TRF79x0WriteRegister(TRF79X0_RX_NO_RESPONSE_WAIT_REG, 0x0e);

    //
    // Set the RX wait time to 66us (7 * 9.44us).
    //
    TRF79x0WriteRegister(TRF79X0_RX_WAIT_TIME_REG, 0x07);

    //
    // Set the SYSCLK to 6.78MHz and the Modulation Depth to OOK.
    //
    TRF79x0WriteRegister(TRF79X0_MODULATOR_CONTROL_REG,
                         (TRF79X0_MOD_CTRL_SYS_CLK_6_78MHZ |
                          TRF79X0_MOD_CTRL_MOD_OOK_100));

    //
    // Configure the Special Settings Register.
    //
    TRF79x0WriteRegister(TRF79X0_RX_SPECIAL_SETTINGS_REG,
       (TRF79x0ReadRegister(TRF79X0_RX_SPECIAL_SETTINGS_REG) & 0x0f) |
        TRF79X0_RX_SP_SET_M848);

    //
    // Configure the Test Settings Register.
    //
    TRF79x0WriteRegister(TRF79X0_TEST_SETTING1_REG, 0x20);

    //
    // Set the regulator voltage to be automatic.
    //
    TRF79x0WriteRegister(TRF79X0_REGULATOR_CONTROL_REG,
                         TRF79X0_REGULATOR_CTRL_AUTO_REG);
}

//*****************************************************************************
//
// Power on the field and wait for a time that is long enough to guarantee
// that all cards in the field will be initialized.
//
//*****************************************************************************
void
ISO14443APowerOn(void)
{
    unsigned char ucReg;

    //
    // Enable RF field and receiver.
    //
    ucReg = TRF79x0ReadRegister(TRF79X0_CHIP_STATUS_CTRL_REG);
    TRF79x0WriteRegister(TRF79X0_CHIP_STATUS_CTRL_REG,
                         ucReg | TRF79X0_STATUS_CTRL_RF_ON);

    //
    // Wait 5ms (as per ISO 14443-3 clause 5).
    //
    SysCtlDelay(((SysCtlClockGet() / 3) * 5) / 1000);
}

//*****************************************************************************
//
// Power off the field and wait for some time.
//
//*****************************************************************************
void
ISO14443APowerOff(void)
{
    unsigned char ucReg;

    //
    // Disable RF field and receiver.
    //
    ucReg = TRF79x0ReadRegister(TRF79X0_CHIP_STATUS_CTRL_REG);

    TRF79x0WriteRegister(TRF79X0_CHIP_STATUS_CTRL_REG,
                         ucReg & ~TRF79X0_STATUS_CTRL_RF_ON);

    //
    // Wait 5ms.
    //
    SysCtlDelay(((SysCtlClockGet() / 3) * 5) / 1000);
}

//*****************************************************************************
//
// Transmit a HLTA command that should HALT the currently selected card.  You
// should always call this function after a successful call to either
// ISO14443ASelect(), ISO14443ASelectFirst() or ISO14443ASelectNext() and
// before any other call to any of those functions.
//
//*****************************************************************************
void
ISO14443AHalt(void)
{
    //
    // HLTA command.
    //
    const unsigned char pucHLTA[2] = {0x50, 0x00};

    TRF79x0Transceive(pucHLTA, sizeof(pucHLTA), 0, NULL, NULL, NULL,
                      TRF79X0_TRANSCEIVE_CRC);
}

//*****************************************************************************
//
// Transceive ISO 14443-A REQA type command.
//
// \param ucCmd is the command, either \b ISO14443A_REQA or \b ISO14443A_WUPA
// \param piATQA is a pointer to an integer to store the received ATQA and
//  will be set to -1 if a collision occurred.
//
// \return true if at least one card responded that is capable of bit-frame
//  anti-collision (e.g. no collision and one of the lower 5 bits of response
//  set, or collision within the first 5 bits, or collision not in the first
//  5 bits but at least one of the first 5 bits is a 1-bit) and false
// otherwise.
//
// \note User code usually does not need to call this function since it is
// implicitly called in ISO14443ASelect(), ISO14443ASelectFirst() or
// ISO14443ASelectNext().
//
//*****************************************************************************
int
ISO14443AREQA(unsigned char ucCmd, int *piATQA)
{
    unsigned char pucResponse[2];
    unsigned int uiRxSize;
    int iColPos;

    uiRxSize = sizeof(pucResponse);

    //
    // Transmit WUPA/REQA, receive ATQA.
    //
    TRF79x0Transceive(&ucCmd, 0, 7, pucResponse, &uiRxSize, 0,
                      TRF79X0_TRANSCEIVE_NO_CRC);

    if(uiRxSize == 2)
    {
        //
        // Valid ATQA received, return it as an integer.  Was transmitted
        // LSByte first.
        //
        if(piATQA != NULL)
        {
            *piATQA = pucResponse[0] | (pucResponse[1] << 8);
        }

        //
        // Return true if one of the lower 5 bits was set.
        //
        return((pucResponse[0] & 0x1F) != 0);
    }
    else
    {
        //
        // No valid ATQA received.
        //
        if(piATQA != NULL)
        {
            *piATQA = -1;
        }

        if(uiRxSize == 0)
        {
            //
            // No response at all -> no card with bit-frame anti-collision.
            //
            return(0);
        }
        else
        {
            //
            // Probably some collision.
            //
            iColPos = TRF79x0GetCollisionPosition();

            if(iColPos > 5)
            {
                //
                // Collision not within the first 5 bits, return true if one of
                // the lower 5 bits was set.
                //
                return((pucResponse[0] & 0x1F) != 0);
            }
            else if(iColPos > 0 && iColPos <= 5)
            {
                //
                // Collision within the first 5 bits, so at least one of them
                // was 1.
                //
                return(1);
            }
            else
            {
                //
                // No collision, but only 1 byte sent?  That card's not right.
                //
                return(0);
            }
        }
    }
}

//*****************************************************************************
//
// Find one card through the anti-collision procedure with given \e psState.
//
// \param psState is the anti-collision state to start from.  If this state
// already specifies a full UID then it will be selected, otherwise
// anti-collision will be tried to complete that starting state, with no
// backtracking.
// \param pucUID is an output buffer to write the selected UID and may be
// \b NULL in which case the UID will not be returned.
// \param puiUIDSize inputs the available space in bytes in \e pucUID and
// returns with the actual length that has been stored there.
// \param pucSAK is an output parameter that stores the received SAK value.
// May be \b NULL in which case the SAK will not be returned
//
// This is a depth first search in a binary tree over the UID space.  On each
// attempt we can learn up to 4 bytes of the UID of the card(s) currently in
// the field.  If the UIDs of two cards differ we will learn that too and get
// the collision position: the position of the bit where the UID of at least
// two cards differs.  We will mark this position in the appropriate field in
// the structure ISO14443AnticolState and then branch first in the direction of
// 0 and increase iBitPos to include this bit.
//
// \return This function returns 1 if a card was selected and 0 otherwise.
//
//*****************************************************************************
static int
ISO14443ADoAnticol(struct ISO14443AAnticolState *psState, unsigned char *pucUID,
                   unsigned int *puiUIDSize, unsigned char *pucSAK)
{
    int iCascadeLevel, iPos;
    unsigned char pucCmd[7], pucResponse[5];
    unsigned int uiRxSize;
    int iIdx, iMaskPosition, iCollPosition, iValidBits, iMaxLength, iNVB;

    iCascadeLevel = 1;

    while(iCascadeLevel < 4)
    {
        //
        // Already known bits for this cascade level, e.g. not including
        // the possible 5 bytes * 8 bits/byte for the lower levels.
        //
        iValidBits = psState->iBitPos - (iCascadeLevel - 1) * 5 * 8;

        //
        // Clamp to a full cascade level.
        //
        if(iValidBits > 40)
        {
            iValidBits = 40;
        }

        //
        // NVB format: bytes.
        //
        iNVB = (iValidBits / 8) << 4;

        //
        // NVB format: bits.
        //
        iNVB |= (iValidBits % 8);

        //
        // Also count the command byte and the NVB byte itself.
        //
        iNVB += 0x20;

        //
        // Prepare command for this level: ANTICOLLISION if less than a full 5
        // bytes for the current cascade level, SELECT otherwise.
        //
        switch (iCascadeLevel)
        {
            case 1:
            {
                pucCmd[0] = 0x93;
                break;
            }
            case 2:
            {
                pucCmd[0] = 0x95;
                break;
            }
            case 3:
            {
                pucCmd[0] = 0x97;
                break;
            }
            default:
            {
                break;
            }
        }

        pucCmd[1] = iNVB;

        //
        // Copy over known bytes (number of bits for this level divided by 8,
        // rounded up).
        //
        memcpy(pucCmd + 2, psState->ucUID + (iCascadeLevel - 1) * 5,
               (iValidBits + 7) / 8);

        //
        // Enforce a small delay of ~600us before each anti-collision frame.
        //
        SysCtlDelay(((SysCtlClockGet() / 3) * 6) / 10000);

        //
        // Maximal expected response length.
        //
        uiRxSize = 5;

        if(iNVB != 0x70)
        {
            //
            // Anti-collision command.
            //
            TRF79x0Transceive(pucCmd, pucCmd[1] >> 4, pucCmd[1] & 0xf,
                              pucResponse, &uiRxSize, NULL,
                              TRF79X0_TRANSCEIVE_NO_CRC);

            if(uiRxSize == 0)
            {
                return(0);
            }

            iCollPosition = TRF79x0GetCollisionPosition();

            if(iCollPosition < 0)
            {
                //
                // No collision occurred, add full response data to known bits.
                //
                iCollPosition = 40;
            }
            else
            {
                //
                // Collision occurred, only add the part that was received
                // correctly.
                //
                // TF7960 Collision position register is in NVB format,
                // convert to straight bit position.  This will be the number
                // of bits that were the same in all responding cards.
                //
                iCollPosition -= 0x20;
                iCollPosition = ((iCollPosition >> 4) * 8) + (iCollPosition & 0xf);
            }

            //
            // Bounds check the results and return 0 if it was invalid.
            //
            if(iCollPosition < 0 || iCollPosition > 40)
            {
                return(0);
            }

            //
            // Mask out the invalid bits in the last byte of the response, if
            // any.
            //
            // Graphic:
            // UID bytes:  | first  || second || third  || fourth || fifth  |
            //             | iValidBits |
            //             |       iCollPosition       |
            // In this graphic the first byte is fully valid.  The second byte
            // was sent partially invalid, but should have been masked on a
            // previous run.  The third byte is received partially invalid and
            // needs to be masked.  Response will only contain the second and
            // third byte (although both are received properly byte-aligned).
            //
            //
            // This many bits in response are valid or at least compatible
            // with the UID.
            //
            iMaskPosition = iCollPosition - (iValidBits / 8) * 8;

            if(iMaskPosition % 8)
            {
                //
                // Need to construct a mask for iMaskPosition%8 bits and
                // apply it at iMaskPosition/8.
                //
                pucResponse[iMaskPosition / 8] &= ~((~0) << (iMaskPosition % 8));
            }

            //
            // Merge in up to iMaskPosition/8 (rounded up) byte into response
            // at index iBitPos/8 (rounded down).
            //
            for(iIdx = 0; iIdx < (iMaskPosition + 7) / 8; iIdx++)
            {
                psState->ucUID[(psState->iBitPos / 8) + iIdx] |=
                    pucResponse[iIdx];
            }

            psState->iBitPos += iCollPosition - iValidBits;

            //
            // Only within this cascade level:
            //
            if(psState->iBitPos % 40 != 0)
            {
                //
                // Mark backtracking point.
                //
                psState->ucCollisions[psState->iBitPos / 8] |=
                    1 << (psState->iBitPos % 8);

                //
                // Walk into the 0 direction.
                //
                psState->iBitPos += 1;
            }
        }
        else
        {
            //
            // Select command.
            //
            TRF79x0Transceive(pucCmd, pucCmd[1] >> 4, pucCmd[1] & 0xf,
                              pucResponse, &uiRxSize, NULL,
                              TRF79X0_TRANSCEIVE_CRC);

            if(uiRxSize == 1)
            {
                //
                // SAK received.
                //
                if(pucResponse[0] & 0x04)
                {
                    //
                    // UID not complete, increase cascade level.
                    //
                    iCascadeLevel++;

                    if(iCascadeLevel > 3)
                    {
                        break;
                    }
                }
                else
                {
                    //
                    // UID complete, return.
                    //
                    break;
                }
            }
            else
            {
                //
                // Some error, card not selected.
                //
                memset(psState->ucUID, 0, sizeof(psState->ucUID));
                psState->iBitPos = 0;
                break;
            }
        }
    }

    //
    // Some error, not fully selected.
    //
    if(((psState->iBitPos % 40) != 0) || (psState->iBitPos == 0))
    {
        return(0);
    }

    //
    // Fully selected a card.  pucResponse[0] should be from the last
    // transaction, of a SELECT command, and therefore contain the SAK
    //
    if(pucSAK != NULL)
    {
        *pucSAK = pucResponse[0];
    }

    //
    // If requested, return the UID, without cascade tag and BCC.
    //
    if(pucUID != NULL && puiUIDSize != NULL)
    {
        iMaxLength = *puiUIDSize;
        iPos = 0;
        *puiUIDSize = 0;

        //
        // From the 5 bytes in each cascade level the 3 middle bytes need to be
        // copied for each level except for the last, where the first 4 bytes
        // need to be copied.
        //
        for(iPos = 0; iPos < psState->iBitPos / 8; iPos += 5)
        {
            if(iPos + 5 < psState->iBitPos / 8)
            {
                //
                // Not the last cascade level.
                //
                if(*puiUIDSize + 3 > iMaxLength)
                {
                    //
                    // Not enough space
                    //
                    *puiUIDSize = 0;
                    break;
                }

                //
                // Copy 3 bytes (e.g. don't copy cascade tag and BCC).
                //
                memcpy(pucUID + *puiUIDSize, psState->ucUID + iPos + 1, 3);

                *puiUIDSize += 3;
            }
            else
            {
                //
                // Last cascade level.
                //
                if(*puiUIDSize + 4 > iMaxLength)
                {
                    //
                    // Not enough space.
                    //
                    *puiUIDSize = 0;

                    break;
                }

                //
                // Copy 4 bytes (e.g. don't copy BCC).
                //
                memcpy(pucUID + *puiUIDSize, psState->ucUID + iPos, 4);

                *puiUIDSize += 4;
            }
        }
    }
    return(1);
}

//*****************************************************************************
//
// Selects the first (or only) card and returns its UID, UID length and
// SAK bytes.
//
// \param ucCmd must be ISO14443A_REQA or ISO14443A_WUPA.
// \param pucUID will store UID of the card that was selected.  May be NULL
// in which case the UID will not be returned.
// \param puiUIDSize must be initialized with the length of the buffer in
// \e pucUID and will return the number of bytes actually stored.
// \param pucSAK will store the SAK byte of the card that was selected and may
// be NULL in which case the SAK byte will not be returned.
//
// The function call initializes and updates a static internal state that
// marks the position in the anti-collision procedure.  ISO14443ASelectNext()
// can be used to continue with the anti-collision from that starting point.
//
// \note You should call ISO14443AHalt() if this function returned true and
// you are done operating on the card.
//
// \return Function returns 1 if a card was selected, 0 otherwise.
//
//*****************************************************************************
int
ISO14443ASelectFirst(unsigned char ucCmd, unsigned char *pucUID,
                     unsigned int *puiUIDSize, unsigned char *pucSAK)
{
    //
    // Initialize/clear static state.
    //
    memset(&g_sAnticolState, 0, sizeof(g_sAnticolState));

    //
    // Wake up all or only new tags.
    //
    if(ISO14443AREQA(ucCmd, NULL) == 0)
    {
        //
        // No tag with support for bit frame anti-collision found.
        //
        return(0);
    }

    return(ISO14443ADoAnticol(&g_sAnticolState, pucUID, puiUIDSize, pucSAK));
}

//*****************************************************************************
//
// Selects the next card and returns its UID, UID length and SAK bytes.
//
// \param ucCmd must be ISO14443A_REQA or ISO14443A_WUPA.
// \param UID will store UID of the card that was selected and may be NULL
// in which case the UID will not be returned.
// \param puiUIDSize must be initialized with the length of the buffer in
// \e UID and will return the number of bytes actually stored.
// \param pucSAK will store the SAK byte of the card that was selected and may
// be NULL in which case the SAK byte will not be returned.
//
// Uses the state that was initialized by ISO14443SelectFirst() and tries
// to find more cards in the field.
//
// \note You should call ISO14443AHalt() if this function returned true and
// you are done operating on the card.
//
// \return This function returns 1 if a card was selected and 0 otherwise.
//
//*****************************************************************************
int
ISO14443ASelectNext(unsigned char ucCmd, unsigned char *pucUID,
                    unsigned int *puiUIDSize, unsigned char *pucSAK)
{
    //
    // Backtrack through static state: starting at iBitPos and going reverse,
    // find the first bit that's set in collisions, walk into the 1 direction,
    // clear the collision indicator and set iBitPos to that position.
    //
    while(--g_sAnticolState.iBitPos > 0)
    {
        //
        // Clear UID bit at this position to clean the state.
        //
        g_sAnticolState.ucUID[g_sAnticolState.iBitPos / 8] &=
            ~(1 << (g_sAnticolState.iBitPos % 8));

        if(g_sAnticolState.ucCollisions[g_sAnticolState.iBitPos / 8] &
           (1 << (g_sAnticolState.iBitPos % 8)))
        {
            //
            // This is our new starting point, set UID bit to walk into the
            // 1 direction.
            //
            g_sAnticolState.ucUID[g_sAnticolState.iBitPos / 8] |=
                1 << (g_sAnticolState.iBitPos % 8);

            //
            // Remove backtracking marker.
            //
            g_sAnticolState.ucCollisions[g_sAnticolState.iBitPos / 8] &=
                ~(1 << (g_sAnticolState.iBitPos % 8));

            //
            // Increment bit position to account for the bit that we just
            // added, then break loop to perform anti-collision with the new
            // partial UID.
            //
            g_sAnticolState.iBitPos++;

            break;
        }

        //
        // Not a backtracking point, go further back.
        //
    }

    //
    // No further backtracking points -> no other cards.
    //
    if(g_sAnticolState.iBitPos <= 0)
    {
        return(0);
    }

    //
    // Wake up all or only new tags.
    //
    if(!ISO14443AREQA(ucCmd, NULL))
    {
        //
        // No tag with support for bit frame anti-collision found.
        //
        return(0);
    }

    return(ISO14443ADoAnticol(&g_sAnticolState, pucUID, puiUIDSize, pucSAK));
}

//*****************************************************************************
//
// Selects a card with given UID and return its SAK byte.
//
// \param pucUID must point to the UID of the card that should be selected and
// may not be NULL.
// \param uiUIDSize must be the length in bytes of the UID stored in \e pucUID.
// \param pucSAK will store the SAK byte of the card that was selected.  May be
// \b NULL in which case the SAK byte will not be returned.
//
// \note You should call ISO14443AHalt() if this function returned true and
// you are done operating on the card.
//
// \return This function will return 1 if a card was selected and 0 otherwise.
//
//*****************************************************************************
int
ISO14443ASelect(unsigned char const *pucUID, unsigned int uiUIDSize,
                unsigned char *pucSAK)
{
    int iIdx, iPos;
    struct ISO14443AAnticolState sState;

    //
    // Check if the given UID size is supported.
    //
    if((uiUIDSize != 4) && (uiUIDSize != 7) && (uiUIDSize != 10))
    {
        return(0);
    }

    //
    // Prepare a state for the given UID.
    //
    sState.iBitPos = 0;

    for(iPos = 0; iPos < uiUIDSize;)
    {
        //
        // Check if this is the final cascade level.
        //
        if(iPos + 4 < uiUIDSize)
        {
            //
            // If this was not the final cascade level then add a cascade tag.
            //
            sState.ucUID[sState.iBitPos / 8] = 0x88;

            //
            // Copy three bytes of UID.
            //
            memcpy(sState.ucUID + (sState.iBitPos / 8) + 1, pucUID + iPos, 3);

            //
            // Increment position in UID.
            //
            iPos += 3;
        }
        else
        {
            //
            // For the final cascade level just copy four bytes of UID.
            //
            memcpy(sState.ucUID + (sState.iBitPos / 8), pucUID + iPos, 4);

            //
            // Increment position in UID.
            //
            iPos += 4;
        }

        //
        // Calculate BCC.
        //
        sState.ucUID[sState.iBitPos / 8 + 4] = 0;

        for(iIdx = 0; iIdx < 4; iIdx++)
        {
            sState.ucUID[sState.iBitPos / 8 + 4] ^=
                sState.ucUID[sState.iBitPos / 8 + iIdx];
        }

        //
        // Increment position in state.
        //
        sState.iBitPos += 40;
    }

    //
    // Always wake up all cards.
    //
    if(!ISO14443AREQA(ISO14443A_WUPA, NULL))
    {
        //
        // No tag with support for bit frame anti-collision found.
        //
        return(0);
    }

    return(ISO14443ADoAnticol(&sState, NULL, NULL, pucSAK));
}

//*****************************************************************************
//
// Helper functions for ISO 14443-A frames to be sent or received in Direct
// Mode.
//
//*****************************************************************************

//*****************************************************************************
//
// Calculates odd parity for one byte.
//
//*****************************************************************************
static unsigned char
ParityByte(unsigned char ucByte)
{
    ucByte ^= ucByte >> 1;
    ucByte ^= ucByte >> 1;
    ucByte ^= ucByte >> 1;
    ucByte ^= ucByte >> 1;
    ucByte ^= ucByte >> 1;
    ucByte ^= ucByte >> 1;
    ucByte ^= ucByte >> 1;
    return((ucByte & 1) ^ 1);
}

//*****************************************************************************
//
// Checks that data has correct (odd) parity
//
// \param pusData is the data buffer to check and must store 16 bits per one
// logical byte: the lower 8 bits are the data byte, the LSBit in the upper
// byte is the parity.
// \param lSize is the number of logical bytes/16 bit words in \e pusData.
//
// \return This function returns 1 if the parity was correct and 0 otherwise.
//
//*****************************************************************************
int
ISO14443ACheckParity(const unsigned short * const pusData, const long lSize)
{
    int iFailed, iIdx;

    iFailed = 0;

    for(iIdx = 0; iIdx < lSize; iIdx++)
    {
        iFailed |= (pusData[iIdx] >> 8) ^ ParityByte(pusData[iIdx] & 0xff);
    }

    return(!iFailed);
}

//*****************************************************************************
//
// Sets data to correct (odd) parity
//
// \param pusData is the data buffer to update and must store 16 bits per one
// logical byte: the lower 8 bits are the data byte, the LSBit in the upper
// byte is the parity.
// \param lSize is the number of logical bytes/16 bit words in \e data
//
//*****************************************************************************
void
ISO14443ACalculateParity(unsigned short * const pusData, const long lSize)
{
    int iIdx;

    for(iIdx = 0; iIdx < lSize; iIdx++)
    {
        pusData[iIdx] = (pusData[iIdx] & 0xff) |
                        (ParityByte(pusData[iIdx] & 0xff) << 8);
    }
}

//*****************************************************************************
//
// Calculate CRC-A and return it.
//
//*****************************************************************************
static unsigned short
CalculateCRC(const unsigned short * const pusData, const long lSize)
{
    unsigned short usCrc;
    int iIdx, iBit;
    unsigned char ucByte, ucBit;

    usCrc = 0x6363;

    for(iIdx = 0; iIdx < lSize; iIdx++)
    {
        ucByte = pusData[iIdx] & 0xff;

        for(iBit = 0; iBit < 8; iBit++)
        {
            ucBit = (usCrc ^ ucByte) & 1;

            ucByte >>= 1;
            usCrc >>= 1;

            if(ucBit)
            {
                usCrc ^= 0x8408;
            }
        }
    }
    return(usCrc);
}

//*****************************************************************************
//
// Check that data has correct CRC in last two bytes.
//
// \param pusData is the data buffer to check and must store 16 bits per one
// logical byte: the lower 8 bits are the data byte, the LSBit in the upper
// byte is the parity.
// \param lSize is the number of logical bytes/16 bit words in \e pusData.
// Must be at least 2, since the CRC consists of two bytes.
//
// \return This function returns 1 if the CRC was correct and 0 otherwise.
//
//*****************************************************************************
int
ISO14443ACheckCRC(const unsigned short * const pusData, const long lSize)
{
    unsigned short usCrc;

    if(lSize < 2)
    {
        return(0);
    }

    usCrc = CalculateCRC(pusData, lSize - 2);

    if(((usCrc & 0xff) == (pusData[lSize - 2] & 0xff)) &&
       (((usCrc >> 8) & 0xff) == (pusData[lSize - 1] & 0xff)))
    {
        return(1);
    }
    return(0);
}

//*****************************************************************************
//
// Appends correct CRC to the data
//
// \param pusData is the data buffer to update and must store 16 bits per one
// logical byte: the lower 8 bits are the data byte, the LSBit in the upper
// byte is the parity.
// \param lSize is the number of logical bytes/16 bit words in \e pusData.  The
// buffer in \e pusData must have room for an additional two logical bytes.
//
// \return This function returns the new length to correctly append the CRC.
//
//*****************************************************************************
long
ISO14443ACalculateCRC(unsigned short * const pusData, const long lSize)
{
    unsigned short usCrc;

    usCrc = CalculateCRC(pusData, lSize);

    pusData[lSize] = usCrc & 0xff;
    pusData[lSize + 1] = (usCrc >> 8) & 0xff;

    ISO14443ACalculateParity(pusData + lSize, 2);

    return(lSize + 2);
}
