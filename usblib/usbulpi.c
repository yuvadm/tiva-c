//*****************************************************************************
//
// usbulpi.c - ULPI access functions.
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
// This is part of revision 2.1.0.12573 of the Tiva USB Library.
//
//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "driverlib/usb.h"
#include "usbulpi.h"

//*****************************************************************************
//
// Hardware ULPI registers.
//
//*****************************************************************************
#define ULPI_FCTL               0x04
#define ULPI_FCTL_SET           0x05
#define ULPI_FCTL_CLEAR         0x06
#define ULPI_ICTL               0x07
#define ULPI_ICTL_SET           0x08
#define ULPI_ICTL_CLEAR         0x09
#define ULPI_OTGCTL             0x0A
#define ULPI_OTGCTL_SET         0x0B
#define ULPI_OTGCTL_CLEAR       0x0C

//*****************************************************************************
//
// The following are defines for the bit fields in the ULPI_FCTL register.
//
//*****************************************************************************
#define ULPI_FCTL_XCVR_M        0x03
#define ULPI_FCTL_XCVR_HS       0x00
#define ULPI_FCTL_XCVR_FS       0x01
#define ULPI_FCTL_XCVR_LS       0x02
#define ULPI_FCTL_XCVR_FSLS     0x03
#define ULPI_FCTL_TERMSEL       0x04
#define ULPI_FCTL_OPMODE_M      0x18
#define ULPI_FCTL_OPMODE_NORM   0x00
#define ULPI_FCTL_OPMODE_NODRV  0x08
#define ULPI_FCTL_OPMODE_NONRZI 0x10
#define ULPI_FCTL_OPMODE_DISAUTO \
                                0x18
#define ULPI_FCTL_OPMODE_RESET  0x20
#define ULPI_FCTL_OPMODE_SUSPEND \
                                0x40

//*****************************************************************************
//
// The following are defines for the bit fields in the ULPI_ICTL register.
//
//*****************************************************************************
#define ULPI_ICTL_SER6PIN       0x01
#define ULPI_ICTL_SER3PIN       0x02
#define ULPI_ICTL_AUTORESUME    0x10
#define ULPI_ICTL_INDINV        0x20
#define ULPI_ICTL_INDPASSTHRU   0x40
#define ULPI_ICTL_PROTDIS       0x80

//*****************************************************************************
//
// The following are defines for the bit fields in the ULPI_OTGCTL register.
//
//*****************************************************************************
#define ULPI_OTGCTL_ID_EN       0x01
#define ULPI_OTGCTL_DPPD_EN     0x02
#define ULPI_OTGCTL_DMPD_EN     0x04
#define ULPI_OTGCTL_DISCHRG_VBUS \
                                0x08
#define ULPI_OTGCTL_CHRG_VBUS   0x10
#define ULPI_OTGCTL_VBUSINT_EN  0x20
#define ULPI_OTGCTL_VBUSEXT_EN  0x40
#define ULPI_OTGCTL_VBUSEXT_IND 0x80

//*****************************************************************************
//
//! Sets the configuration of an external USB Phy.
//!
//! \param ui32Base specifies the USB module base address.
//! \param ui32Config specifies the configuration options for the external Phy.
//!
//! This function sets the configuration options for an externally connected
//! USB Phy that is connected using the ULPI interface.  The \e ui32Config
//! parameter holds all of the configuration options defined by the
//! \b UPLI_CFG_ values.  The values are grouped as follows:
//!
//! Connection speed, using one of the following:
//! - \b UPLI_CFG_HS enables high speed operation.
//! - \b UPLI_CFG_FS enables full speed operation.
//! - \b UPLI_CFG_HS enables low speed operation.
//!
//! Any of the following can be included:
//! - \b UPLI_CFG_AUTORESUME enable automatic transmission of resume signaling
//!   from the Phy.
//! - \b UPLI_CFG_INVVBUSIND inverts the external VBUS indicator if it is
//!   selected.
//! - \b UPLI_CFG_PASSTHRUIND passes the external VBUS indicator through
//!   without using the Phy's VBUS comparator.
//! - \b ULPI_CFG_EXTVBUSDRV enables an external VBUS drive source.
//! - \b ULPI_CFG_EXTVBUSIND enables an external signal for VBUS valid.
//!
//! \return None.
//
//*****************************************************************************
void
ULPIConfigSet(uint32_t ui32Base, uint32_t ui32Config)
{
    uint8_t ui8Val;

    ui8Val = USBULPIRegRead(ui32Base, ULPI_FCTL);
    ui8Val &= ~(ULPI_FCTL_XCVR_M);
    ui8Val = ui8Val | (uint8_t)ui32Config;

    USBULPIRegWrite(ui32Base, ULPI_FCTL, ui8Val);

    ui8Val = USBULPIRegRead(ui32Base, ULPI_ICTL);
    ui8Val &= ~(ULPI_ICTL_AUTORESUME | ULPI_ICTL_INDINV |
                ULPI_ICTL_INDPASSTHRU);
    ui8Val = ui8Val | (uint8_t)((ui32Config >> 8) & 0xff);

    USBULPIRegWrite(ui32Base, ULPI_ICTL, ui8Val);

    ui8Val = USBULPIRegRead(ui32Base, ULPI_OTGCTL);
    ui8Val &= ~(ULPI_OTGCTL_VBUSINT_EN | ULPI_OTGCTL_VBUSEXT_EN |
                ULPI_OTGCTL_VBUSEXT_IND);
    ui8Val = ui8Val | (uint8_t)((ui32Config >> 16) & 0xff);

    USBULPIRegWrite(ui32Base, ULPI_OTGCTL, ui8Val);
}

//*****************************************************************************
//
//! Enables or disables power to the external USB Phy.
//!
//! \param ui32Base specifies the USB module base address.
//! \param bEnable specifies if the Phy is fully powered or in suspend mode.
//!
//! This function sets the current power configuration for the external ULPI
//! connected Phy.  When \e bEnable is \b true the Phy is fully powered and
//! when \b false the USB Phy is in suspend mode.
//!
//! \return None.
//
//*****************************************************************************
void
ULPIPowerTransceiver(uint32_t ui32Base, bool bEnable)
{
    if(bEnable)
    {
        USBULPIRegWrite(ui32Base, ULPI_FCTL_CLEAR,
                        ULPI_FCTL_OPMODE_SUSPEND);
    }
    else
    {
        USBULPIRegWrite(ui32Base, ULPI_FCTL_SET,
                        ULPI_FCTL_OPMODE_SUSPEND);
    }
}
