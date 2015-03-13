//*****************************************************************************
//
// trf79x0.h - Header file for the TI TRF79X0 driver
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

#ifndef __TRF79X0_H__
#define __TRF79X0_H__

#include "types.h"

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
// timer in the TRF7960 and must enable the no-response interrupt.
//
//*****************************************************************************
#define TRF7960_RX_TIMEOUT      10

//*****************************************************************************
//
// An enum defining the various daughter boards that can be attached to the
// development board.
//
//*****************************************************************************
typedef enum
{
    RF_DAUGHTER_NONE = 0,
    RF_DAUGHTER_TRF7960ATB = 1,
    RF_DAUGHTER_TRF7970ATB = 2,
    RF_DAUGHTER_TRF7970ABP = 3,
    RF_DAUGHTER_UNKNOWN = 0xFFFF
}
tRFDaughterBoard;

extern tRFDaughterBoard g_eRFDaughterType;
#define NFC_NONE            0
#define NFC_CARD_EMU_TAG4A  1
#define NFC_CARD_EMU_TAG4B  2

extern unsigned char g_ucNfcWorkMode;

//*****************************************************************************
//
// IRQ Status Register (0x0C) for NFC and Card Emulation Operation
//
//*****************************************************************************
#define RF_FIELD_CHANGE     0x04
#define SDD_COMPLETED       0x08

//*****************************************************************************
//
// TRF79X0 Register Definitions.
//
//*****************************************************************************
#define TRF79X0_CHIP_STATUS_CTRL_REG        0x00
#define TRF79X0_ISO_CONTROL_REG             0x01
#define TRF79X0_ISO14443B_OPTIONS_REG       0x02
#define TRF79X0_ISO14443A_OPTIONS_REG       0x03
#define TRF79X0_TX_TIMER_EPC_HIGH           0x04
#define TRF79X0_TX_TIMER_EPC_LOW            0x05
#define TRF79X0_TX_PULSE_LENGTH_CTRL_REG    0x06
#define TRF79X0_RX_NO_RESPONSE_WAIT_REG     0x07
#define TRF79X0_RX_WAIT_TIME_REG            0x08
#define TRF79X0_MODULATOR_CONTROL_REG       0x09
#define TRF79X0_RX_SPECIAL_SETTINGS_REG     0x0A
#define TRF79X0_REGULATOR_CONTROL_REG       0x0B
#define TRF79X0_IRQ_STATUS_REG              0x0C
#define TRF79X0_IRQ_MASK_REG                0x0D
#define TRF79X0_COLLISION_POSITION_REG      0x0E
#define TRF79X0_RSSI_LEVEL_REG              0x0F
#define TRF79X0_RAM_START_ADDRESS_REG       0x10
#define TRF797X0_SPECIAL_FUNC_1_REG         0x10
#define TRF797X0_SPECIAL_FUNC_2_REG         0x11
#define TRF79X0_FIFO_IRQ_LEVEL_REG          0x14
#define TRF79X0_NFC_LO_FIELD_LEVEL_REG      0x16
#define TRF79X0_NFC_ID_REG                  0x17
#define TRF79X0_NFC_TARGET_LEVEL_REG        0x18
#define TRF79X0_NFC_TARGET_PROTOCOL_REG     0x19
#define TRF79X0_TEST_SETTING1_REG           0x1A
#define TRF79X0_TEST_SETTING2_REG           0x1B
#define TRF79X0_FIFO_STATUS_REG             0x1C
#define TRF79X0_TX_LENGTH_BYTE1_REG         0x1D
#define TRF79X0_TX_LENGTH_BYTE2_REG         0x1E
#define TRF79X0_FIFO_REG                    0x1F

//*****************************************************************************
//
// TRF79X0 TRF79X0_CHIP_STATUS_CTRL_REG Register Definitions.
//
//*****************************************************************************
#define TRF79X0_STATUS_CTRL_DIRECT          0x40
#define TRF79X0_STATUS_CTRL_RF_ON           0x20
#define TRF79X0_STATUS_CTRL_RF_PWR_HALF     0x10
#define TRF79X0_STATUS_CTRL_RF_PWR_FULL     0x00
#define TRF79X0_STATUS_CTRL_5V_OPERATION    0x01

//*****************************************************************************
//
// TRF79X0 TRF79X0_ISO_CONTROL Register Definitions.
//
//*****************************************************************************
#define TRF79X0_ISO_CONTROL_RX_CRC_N                  0x80
#define TRF79X0_ISO_CONTROL_DIR_MODE                  0x40
#define TRF79X0_ISO_NFC_TARGET                        0x00
#define TRF79X0_ISO_NFC_INITIATOR                     0x10
#define TRF79X0_NFC_PASSIVE_MODE                      0x00
#define TRF79X0_NFC_ACTIVE_MODE                       0x08
#define TRF79X0_NFC_NORMAL_MODE                       0x00
#define TRF79X0_NFC_CARD_EMULATION_MODE               0x40
#define TRF79X0_ISO_CONTROL_14443A_106K               0x08
#define TRF79X0_ISO_CONTROL_14443A_106K               0x08
#define TRF79X0_ISO_CONTROL_14443B_106K               0x0C
#define TRF79X0_ISO_CONTROL_15693_LOW_1SUB_1OUT4      0x00
#define TRF79X0_ISO_CONTROL_15693_HIGH_1SUB_1OUT4     0x02
#define TRF79X0_ISO_CONTROL_15693_HIGH_1SUB_1OUT256   0x03

//*****************************************************************************
//
// TRF79X0 TRF79X0_MODULATOR_CONTROL_REG Register Definitions.
//
//*****************************************************************************
#define TRF79X0_MOD_CTRL_SYS_CLK_13_56MHZ   0x30
#define TRF79X0_MOD_CTRL_SYS_CLK_6_78MHZ    0x20
#define TRF79X0_MOD_CTRL_SYS_CLK_3_3MHZ     0x10
#define TRF79X0_MOD_CTRL_SYS_CLK_DISABLE    0x00
#define TRF79X0_MOD_CTRL_MOD_ASK_30         0x07
#define TRF79X0_MOD_CTRL_MOD_ASK_22         0x06
#define TRF79X0_MOD_CTRL_MOD_ASK_16         0x05
#define TRF79X0_MOD_CTRL_MOD_ASK_13         0x04
#define TRF79X0_MOD_CTRL_MOD_ASK_8_5        0x03
#define TRF79X0_MOD_CTRL_MOD_ASK_7          0x02
#define TRF79X0_MOD_CTRL_MOD_OOK_100        0x01
#define TRF79X0_MOD_CTRL_MOD_ASK_10         0x00

//*****************************************************************************
//
// TRF79X0 TRF79X0_RX_SPECIAL_SETTINGS_REG Register Definitions.
//
//*****************************************************************************
#define TRF79X0_RX_SP_SET_M848              0x20
#define TRF79X0_RX_SP_SET_C424              0x40

//*****************************************************************************
//
// TRF79X0 TRF79X0_REGULATOR_CONTROL_REG Register Definitions.
//
//*****************************************************************************
#define TRF79X0_REGULATOR_CTRL_AUTO_REG     0x80
#define TRF79X0_REGULATOR_CTRL_VRS_2_7V     0x00
#define TRF79X0_REGULATOR_CTRL_VRS_2_8V     0x01
#define TRF79X0_REGULATOR_CTRL_VRS_2_9V     0x02
#define TRF79X0_REGULATOR_CTRL_VRS_3_0V     0x03
#define TRF79X0_REGULATOR_CTRL_VRS_3_1V     0x04
#define TRF79X0_REGULATOR_CTRL_VRS_3_2V     0x05
#define TRF79X0_REGULATOR_CTRL_VRS_3_3V     0x06
#define TRF79X0_REGULATOR_CTRL_VRS_3_4V     0x07

//*****************************************************************************
//
// TRF79x0 Command Definitions.
//
//*****************************************************************************
#define TRF79X0_IDLE_CMD                                    0x00
#define TRF79X0_SOFT_INIT_CMD                               0x03
#define TRF79X0_INITIAL_RF_COLLISION_AVOID_CMD              0x04
#define TRF79X0_PERFORM_RES_RF_COLLISION_AVOID_CMD          0x05
#define TRF79X0_PERFORM_RES_RF_COLLISION_AVOID_N0_CMD       0x06
#define TRF79X0_RESET_FIFO_CMD                              0x0F
#define TRF79X0_TRANSMIT_NO_CRC_CMD                         0x10
#define TRF79X0_TRANSMIT_CRC_CMD                            0x11
#define TRF79X0_DELAY_TRANSMIT_NO_CRC_CMD                   0x12
#define TRF79X0_DELAY_TRANSMIT_CRC_CMD                      0x13
#define TRF79X0_TRANSMIT_NEXT_SLOT_CMD                      0x14
#define TRF79X0_CLOSE_SLOT_SEQUENCE_CMD                     0x15
#define TRF79X0_STOP_DECODERS_CMD                           0x16
#define TRF79X0_RUN_DECODERS_CMD                            0x17
#define TRF79X0_TEST_INTERNAL_RF_CMD                        0x18
#define TRF79X0_TEST_EXTERNAL_RF_CMD                        0x19
#define TRF79X0_RX_ADJUST_GAIN_CMD                          0x1A

//*****************************************************************************
//
// TRF79x0 Command/Address mode definitions.
//
//*****************************************************************************
#define TRF79X0_ADDRESS_MASK                0x1F
#define TRF79X0_CONTROL_CMD                 0x80
#define TRF79X0_CONTROL_REG_READ            0x40
#define TRF79X0_CONTROL_REG_WRITE           0x00
#define TRF79X0_REG_MODE_SINGLE             0x00
#define TRF79X0_REG_MODE_CONTINUOUS         0x20

//*****************************************************************************
//
// TRF7960/7970 Modulator control register mode default values to
// determine RF Daughter Board.
//
//*****************************************************************************
#define TRF7960_DEFAULT_ID       0x11
#define TRF7970_DEFAULT_ID       0x91

//*****************************************************************************
//
// The following defines are used with the TRF79x0Transceive() function with
// the uiFlags parameter.
//
//*****************************************************************************

//
// Transmit without CRC, receive without CRC check.
//
#define TRF79X0_TRANSCEIVE_NO_CRC   0
//
// Transmit without CRC, receive with CRC check.
//
#define TRF79X0_TRANSCEIVE_RX_CRC   1
//
// Transmit with CRC, receive without CRC check.
//
#define TRF79X0_TRANSCEIVE_TX_CRC   2
//
// Transmit with CRC, receive with CRC check.
//
#define TRF79X0_TRANSCEIVE_CRC      (TRF79X0_TRANSCEIVE_TX_CRC | \
                                     TRF79X0_TRANSCEIVE_RX_CRC)

//*****************************************************************************
//
// These defines specify abstract IRQ causes to wait for. Since the IRQ
// state register does not lend itself to easy cumulative storage (for
// example just because bit 0x80 was set at least once does not mean that the
// transmission is complete) these are defined to have an abstract way to
// express certain conditions that one would want to wait for.
//
//*****************************************************************************

//
// Wait for any IRQ to occur.
//
#define TRF79X0_WAIT_ANY        0x00000001

//
// Wait for an IRQ that signifies the end of transmission to occur.
//
#define TRF79X0_WAIT_TXEND      0x00000002

//
// Wait for an IRQ that signifies the end of reception to occur, this
// will either be 0x40 with no other flags set, or 0x01 for RX timeout.
//
#define TRF79X0_WAIT_RXEND      0x00000004

//*****************************************************************************
//
// These enumerations are used as part of the state machine layout for NFC P2P
//
//*****************************************************************************

//
// States for the TRF79x0 State Machine
//
typedef enum
{
    BOARD_INIT = 0,
    P2P_INITATIOR_MODE,
    P2P_PASSIVE_TARGET_MODE,
    P2P_ACTIVE_TARGET_MODE,
    CARD_EMULATION_TYPE_A,
    CARD_EMULATION_TYPE_B
} tTRF79x0TRFMode;

//
// Frequency Settings for TRF79x0
//
typedef enum
{
    FREQ_STAND_BY= 0,           // Used for Board Initialization
    FREQ_106_KBPS,
    FREQ_212_KBPS,
    FREQ_424_KBPS
} tTRF79x0Frequency;

//
// CRC Settings for TRF79x0
//
typedef enum
{
    CRC_BIT_DISABLE = 0,
    CRC_BIT_ENABLE
} tTRF79x0CRC;

//
// IRQ Flag deffinitions. Defined in datasheet, provided for ease of use
//
typedef enum
{
    IRQ_STATUS_IDLE = 0x00,
    IRQ_STATUS_COLLISION_ERROR = 0x01,
    IRQ_STATUS_COLLISION_AVOID_FINISHED = 0x02,
    IRQ_STATUS_RF_FIELD_CHANGE = 0x04,
    IRQ_STATUS_SDD_COMPLETE = 0x08,
    IRQ_STATUS_PROTOCOL_ERROR = 0x10,
    IRQ_STATUS_FIFO_HIGH_OR_LOW  = 0x20,
    IRQ_STATUS_RX_COMPLETE = 0x40,
    IRQ_STATUS_TX_COMPLETE = 0x80,
    IRQ_STATUS_TIME_OUT = 0xFF
} tTRF79x0IRQFlag;

//*****************************************************************************
//
// Exported function prototypes.
//
//*****************************************************************************
extern void TRF79x0Init(void);
extern void TRF79x0SetMode(tTRF79x0TRFMode eMode, tTRF79x0Frequency eFrequency);
extern void TRF79x0Interrupt(void);
extern void TRF79x0InterruptInit(void);
extern void TRF79x0InterruptEnable(void);
extern void TRF79x0InterruptDisable(void);
extern void TRF79x0DisableTransmitter(void);
extern void TRF797x0ResetDecoders(void);
extern uint8_t* TRF79x0GetNFCBuffer(void);
extern void TRF79x0DirectCommand(uint8_t ucCommand);
extern void TRF79x0ResetFifoCommand(void);
extern tStatus TRF79x0Init2(tTRF79x0TRFMode eMode,
                                tTRF79x0Frequency eFrequency);
tStatus TRF79x0WriteFIFO(uint8_t *pui8Buffer, tTRF79x0CRC eCRCBit,
                            uint8_t ui8Length);
tTRF79x0IRQFlag TRF79x0IRQHandler(uint16_t ui16TimeOut);
extern void TRF79x0WriteRegister(unsigned char ucAddress,
                                 unsigned char ucData);
extern void TRF79x0WriteRegisterContinuous(unsigned char ucAddress,
                                           unsigned char *pucData,
                                           unsigned int uiLength);
extern unsigned char TRF79x0ReadIRQStatus(void);
extern unsigned char TRF79x0ReadRegister(unsigned char ucAddress);
extern void TRF79x0ReadRegisterContinuous(unsigned char ucAddress,
                                          unsigned char *pucData,
                                          unsigned int uiLength);
extern void TRF79x0FIFOWrite(unsigned char const *pucData,
                             unsigned int uiLength);
extern void TRF79x0Receive(unsigned char *pucData, unsigned int *puiLength);
extern void TRF79x0Transmit(unsigned char const *pucData,
                            unsigned int uiLength, unsigned int uiBits);
extern void TRF79x0Transceive(unsigned char const *pucTXBuf,
                              unsigned int uiTXLen, unsigned int uiTXBits,
                              unsigned char *pucRXBuf, unsigned int *puiRXLen,
                              unsigned int *puiRXBits, unsigned int uiFlags);
extern void TRF79x0IRQClearAll(void);
extern void TRF79x0IRQClearCauses(unsigned int uiCauses);
extern int TRF79x0IRQWait(unsigned long ulCondition);
extern int TRF79x0IRQWaitTimeout(unsigned long ulCondition,
                                 unsigned long ulTimeout);
extern int TRF79x0GetCollisionPosition(void);
extern int TRF79x0IsCollision(void);
extern void TRF79x0InitialSettings(void);
extern void TRF79x0ReceiveAgain(unsigned char *pucRXBuf,
                                    unsigned int *puiRXLen);
extern void TRF79x0ReceiveEnd(void);
extern void TRF79x0TransceiveISO15693(unsigned char const *pucTXBuf,
                                unsigned int uiTXLen,
                                unsigned int uiTXBits, unsigned char *pucRXBuf,
                                unsigned int *puiRXLen, unsigned int *puiRXBits,
                                unsigned int uiFlags);
extern int SendResponse(int Something, int DataLength, char *DataPtr);
extern int SendResponse_w_o_CRC(int Something, int DataLength, char *DataPtr);
#endif
