//*****************************************************************************
//
// bl_config.h - The configurable parameters of the boot loader.
//
// Copyright (c) 2012-2014 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 2.1.0.12573 of the DK-TM4C123G Firmware Package.
//
//*****************************************************************************

#ifndef __BL_CONFIG_H__
#define __BL_CONFIG_H__

//*****************************************************************************
//
// The following defines are used to configure the operation of the boot
// loader.  For each define, its interactions with other defines are described.
// First is the dependencies (in other words, the defines that must also be
// defined if it is defined), next are the exclusives (in other words, the
// defines that can not be defined if it is defined), and finally are the
// requirements (in other words, the defines that must be defined if it is
// defined).
//
// The following defines must be defined in order for the boot loader to
// operate:
//
//     One of CAN_ENABLE_UPDATE, ENET_ENABLE_UPDATE, I2C_ENABLE_UPDATE,
//            SSI_ENABLE_UPDATE, UART_ENABLE_UPDATE, or USB_ENABLE_UPDATE
//     APP_START_ADDRESS
//     VTABLE_START_ADDRESS
//     FLASH_PAGE_SIZE
//     STACK_SIZE
//
//*****************************************************************************

//*****************************************************************************
//
// The frequency of the crystal used to clock the microcontroller.
//
// This defines the crystal frequency used by the microcontroller running the
// boot loader.  If this is unknown at the time of production, then use the
// UART_AUTOBAUD feature to properly configure the UART.
//
// Depends on: None
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define CRYSTAL_FREQ            16000000

//*****************************************************************************
//
// The starting address of the application.  This must be a multiple of 1024
// bytes (making it aligned to a page boundary).  A vector table is expected at
// this location, and the perceived validity of the vector table (stack located
// in SRAM, reset vector located in flash) is used as an indication of the
// validity of the application image.
//
// The flash image of the boot loader must not be larger than this value.
//
// Depends on: None
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define APP_START_ADDRESS       0x2800

//*****************************************************************************
//
// The address at which the application locates its exception vector table.
// This must be a multiple of 1024 bytes (making it aligned to a page
// boundary).  Typically, an application will start with its vector table and
// this value should be set to APP_START_ADDRESS.  This option is provided to
// cater for applications which run from external memory which may not be
// accessible by the NVIC (the vector table offset register is only 30 bits
// long).
//
// Depends on: None
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define VTABLE_START_ADDRESS    0x2800

//*****************************************************************************
//
// The size of a single, erasable page in the flash.  This must be a power
// of 2.
//
// Depends on: None
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define FLASH_PAGE_SIZE         0x00000400

//*****************************************************************************
//
// The amount of space at the end of flash to reserved.  This must be a
// multiple of 1024 bytes (making it aligned to a page boundary).  This
// reserved space is not erased when the application is updated, providing
// non-volatile storage that can be used for parameters.
//
// Depends on: None
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define FLASH_RSVD_SPACE        0x00000800

//*****************************************************************************
//
// The number of words of stack space to reserve for the boot loader.
//
// Depends on: None
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define STACK_SIZE              128

//*****************************************************************************
//
// The number of words in the data buffer used for receiving packets.  This
// value must be at least 3.  If using auto-baud on the UART, this must be at
// least 20.  The maximum usable value is 65 (larger values will result in
// unused space in the buffer).
//
// Depends on: None
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define BUFFER_SIZE             20

//*****************************************************************************
//
// Enables updates to the boot loader.  Updating the boot loader is an unsafe
// operation since it is not fully fault tolerant (losing power to the device
// part way though could result in the boot loader no longer being present in
// flash).
//
// Depends on: None
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define ENABLE_BL_UPDATE

//*****************************************************************************
//
// Enables runtime and download CRC32 checking of the main firmware image.
// If this is defined, the boot loader will scan the main firmware image for
// an image information header (stored immediately above the vector table and
// marked by the words 0xFF01FF02 and 0xFF03FF04).  If the header is found and
// the CRC32 value it contains matches that calculated for the image, the
// firmware is run.  If the CRC32 does not match or the image information
// is not found, the boot loader retains control and waits for a new download.
// To aid debugging, if this option is used without ENFORCE_CRC being set, the
// image will also be booted if the header is present but the length field is
// set to 0xFFFFFFFF, typically indicating that the firmware file has not been
// run through the post-processing tool which inserts the length and CRC values.
//
// Note that firmware images intended for use with CRC checking must have been
// built with an 8 word image header appended to the top of the vector table
// and the binary must have been processed by a tool such as tools/binpack.exe
// to ensure that the required length (3rd word) and CRC32 (4th word) fields
// are populated in the header.
//
// Depends on: ENFORCE_CRC
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define CHECK_CRC

//*****************************************************************************
//
// This definition may be used alongside CHECK_CRC to remove the debug behavior
// which will allow an image with an uninitialized header to be run.  With
// ENFORCE_CRC defined firmware images will only be booted if they contain a
// valid image information header and if the embedded CRC32 in that header
// matches the calculated value.
//
// Depends on: None
// Exclusive of: None
// Requires: CHECK_CRC
//
//*****************************************************************************
//#define ENFORCE_CRC

//*****************************************************************************
//
// This definition will cause the the boot loader to erase the entire flash on
// updates to the boot loader or to erase the entire application area when the
// application is updated.  This erases any unused sections in the flash before
// the firmware is updated.
//
// Depends on: None
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define FLASH_CODE_PROTECTION

//*****************************************************************************
//
// Enables the call to decrypt the downloaded data before writing it into
// flash.  The decryption routine is empty in the reference boot loader source,
// which simply provides a placeholder for adding an actual decryption
// algorithm.  Although this option is retained for backwards compatibility, it
// is recommended that a decryption function be specified using the newer hook
// function mechanism and BL_DECRYPT_FN_HOOK instead.
//
// Depends on: None
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define ENABLE_DECRYPTION

//*****************************************************************************
//
// Enables support for the MOSCFAIL handler in the NMI interrupt.
//
// Depends on: None
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define ENABLE_MOSCFAIL_HANDLER

//*****************************************************************************
//
// Enables the pin-based forced update check.  When enabled, the boot loader
// will go into update mode instead of calling the application if a pin is read
// at a particular polarity, forcing an update operation.  In either case, the
// application is still able to return control to the boot loader in order to
// start an update.  For applications which need to perform more complex
// checking than is possible using a single GPIO, a hook function may be
// provided using BL_CHECK_UPDATE_FN_HOOK instead.
//
// Depends on: None
// Exclusive of: None
// Requires: FORCED_UPDATE_PERIPH, FORCED_UPDATE_PORT, FORCED_UPDATE_PIN,
//           FORCED_UPDATE_POLARITY
//
//*****************************************************************************
//#define ENABLE_UPDATE_CHECK

//*****************************************************************************
//
// The GPIO module to enable in order to check for a forced update.  This will
// be one of the SYSCTL_RCGC2_GPIOx values, where "x" is replaced with the port
// name (such as B).  The value of "x" should match the value of "x" for
// FORCED_UPDATE_PORT.
//
// Depends on: ENABLE_UPDATE_CHECK
// Exclusive of: None
// Requries: None
//
//*****************************************************************************
//#define FORCED_UPDATE_PERIPH    SYSCTL_RCGC2_GPIOB

//*****************************************************************************
//
// The GPIO port to check for a forced update.  This will be one of the
// GPIO_PORTx_BASE values, where "x" is replaced with the port name (such as
// B).  The value of "x" should match the value of "x" for
// FORCED_UPDATE_PERIPH.
//
// Depends on: ENABLE_UPDATE_CHECK
// Exclusive of: None
// Requries: None
//
//*****************************************************************************
//#define FORCED_UPDATE_PORT      GPIO_PORTB_BASE

//*****************************************************************************
//
// The pin to check for a forced update.  This is a value between 0 and 7.
//
// Depends on: ENABLE_UPDATE_CHECK
// Exclusive of: None
// Requries: None
//
//*****************************************************************************
//#define FORCED_UPDATE_PIN       4

//*****************************************************************************
//
// The polarity of the GPIO pin that results in a forced update.  This value
// should be 0 if the pin should be low and 1 if the pin should be high.
//
// Depends on: ENABLE_UPDATE_CHECK
// Exclusive of: None
// Requries: None
//
//*****************************************************************************
//#define FORCED_UPDATE_POLARITY  0

//*****************************************************************************
//
// This enables a weak pull-up or pull-down for the GPIO pin used in a forced
// update.  Only one of FORCED_UPDATE_WPU or FORCED_UPDATE_WPD should be
// defined, or neither if a weak pull-up or pull-down is not required.
//
// Depends on: ENABLE_UPDATE_CHECK
// Exclusive of: None
// Requries: None
//
//*****************************************************************************
//#define FORCED_UPDATE_WPU
//#define FORCED_UPDATE_WPD

//*****************************************************************************
//
// This enables the use of the GPIO_LOCK mechanism for configuration of
// protected GPIO pins (for example JTAG pins).  If this value is not defined,
// the locking mechanism will not be used.  The only legal values for this
// feature are GPIO_LOCK_KEY for Fury devices and GPIO_LOCK_KEY_DD for all
// other devices except Sandstorm devices, which do not support this feature.
//
// Depends on: ENABLE_UPDATE_CHECK
// Exclusive of: None
// Requries: None
//
//*****************************************************************************
//#define FORCED_UPDATE_KEY       GPIO_LOCK_KEY
//#define FORCED_UPDATE_KEY       GPIO_LOCK_KEY_DD

//*****************************************************************************
//
// Selects the UART as the port for communicating with the boot loader.
//
// Depends on: None
// Exclusive of: CAN_ENABLE_UPDATE, ENET_ENABLE_UPDATE, I2C_ENABLE_UPDATE,
//               SSI_ENABLE_UPDATE, USB_ENABLE_UPDATE
// Requires: UART_AUTOBAUD or UART_FIXED_BAUDRATE, BUFFER_SIZE
//
//*****************************************************************************
//#define UART_ENABLE_UPDATE

//*****************************************************************************
//
// Enables automatic baud rate detection.  This can be used if the crystal
// frequency is unknown, or if operation at different baud rates is desired.
//
// Depends on: UART_ENABLE_UPDATE
// Exclusive of: UART_FIXED_BAUDRATE
// Requires: None
//
//*****************************************************************************
//#define UART_AUTOBAUD

//*****************************************************************************
//
// Selects the baud rate to be used for the UART.
//
// Depends on: UART_ENABLE_UPDATE, CRYSTAL_FREQ
// Exclusive of: UART_AUTOBAUD
// Requires: None
//
//*****************************************************************************
//#define UART_FIXED_BAUDRATE     115200

//*****************************************************************************
//
// Selects the SSI port as the port for communicating with the boot loader.
//
// Depends on: None
// Exclusive of: CAN_ENABLE_UPDATE, ENET_ENABLE_UPDATE, I2C_ENABLE_UPDATE,
//               UART_ENABLE_UPDATE, USB_ENABLE_UPDATE
// Requires: BUFFER_SIZE
//
//*****************************************************************************
//#define SSI_ENABLE_UPDATE

//*****************************************************************************
//
// Selects the I2C port as the port for communicating with the boot loader.
//
// Depends on: None
// Exclusive of: CAN_ENABLE_UPDATE, ENET_ENABLE_UPDATE, SSI_ENABLE_UPDATE,
//               UART_ENABLE_UPDATE, USB_ENABLE_UPDATE
// Requires: I2C_SLAVE_ADDR, BUFFER_SIZE
//
//*****************************************************************************
//#define I2C_ENABLE_UPDATE

//*****************************************************************************
//
// Specifies the I2C address of the boot loader.
//
// Depends on: I2C_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define I2C_SLAVE_ADDR          0x42

//*****************************************************************************
//
// Selects Ethernet update via the BOOTP/TFTP protocol.
//
// Depends on: None
// Exclusive of: CAN_ENABLE_UPDATE, I2C_ENABLE_UPDATE, SSI_ENABLE_UPDATE,
//               UART_ENABLE_UPDATE, USB_ENABLE_UPDATE
// Requires: CRYSTAL_FREQ
//
//*****************************************************************************
//#define ENET_ENABLE_UPDATE

//*****************************************************************************
//
// Enables the use of the Ethernet status LED outputs to indicate traffic and
// connection status.
//
// Depends on: ENET_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define ENET_ENABLE_LEDS

//*****************************************************************************
//
// Specifies the hard coded MAC address for the Ethernet interface.  There are
// six individual values (ENET_MAC_ADDR0 through ENET_MAC_ADDR5) that provide
// the six bytes of the MAC address, where ENET_MAC_ADDR0 though ENET_MAC_ADDR2
// provide the organizationally unique identifier (OUI) and ENET_MAC_ADDR3
// through ENET_MAC_ADDR5 provide the extension identifier.  If these values
// are not provided, the MAC address will be extracted from the user registers.
//
// Depends on: ENET_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define ENET_MAC_ADDR0          0x00
//#define ENET_MAC_ADDR1          0x00
//#define ENET_MAC_ADDR2          0x00
//#define ENET_MAC_ADDR3          0x00
//#define ENET_MAC_ADDR4          0x00
//#define ENET_MAC_ADDR5          0x00

//*****************************************************************************
//
// Specifies the name of the BOOTP server from which to request information.
// The use of this specifier allows a board-specific BOOTP server to co-exist
// on a network with the DHCP server that may be part of the network
// infrastructure.  The BOOTP server provided by Texas Instruments requires
// that this be set to "stellaris".
//
// Depends on: ENET_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define ENET_BOOTP_SERVER       "stellaris"

//*****************************************************************************
//
// Selects USB update via Device Firmware Update class.
//
// Depends on: None
// Exclusive of: CAN_ENABLE_UPDATE, ENET_ENABLE_UPDATE, I2C_ENABLE_UPDATE,
//               SSI_ENABLE_UPDATE, UART_ENABLE_UPDATE,
// Requires: CRYSTAL_FREQ, USB_VENDOR_ID, USB_PRODUCT_ID, USB_DEVICE_ID,
//           USB_MAX_POWER
//
//*****************************************************************************
#define USB_ENABLE_UPDATE

//*****************************************************************************
//
// The USB vendor ID published by the DFU device.  This value is the TI
// Tiva vendor ID.  Change this to the vendor ID you have been assigned by the
// USB-IF.
//
// Depends on: USB_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_VENDOR_ID           0x1cbe

//*****************************************************************************
//
// The USB device ID published by the DFU device.  If you are using your own
// vendor ID, chose a device ID that is different from the ID you use in
// non-update operation.  If you have sublicensed TI's vendor ID, you must
// use an assigned product ID here.
//
// Depends on: USB_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_PRODUCT_ID          0x00FF

//*****************************************************************************
//
// Selects the BCD USB device release number published in the device
// descriptor.
//
// Depends on: USB_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_DEVICE_ID           0x0001

//*****************************************************************************
//
// Sets the maximum power consumption that the DFU device will report to the
// USB host in the configuration descriptor.  Units are milliamps.
//
// Depends on: USB_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_MAX_POWER           150

//*****************************************************************************
//
// Specifies whether the DFU device reports to the host that it is self-powered
// (defined as 0) or bus-powered (defined as 1).
//
// Depends on: USB_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_BUS_POWERED         1

//*****************************************************************************
//
// Specifies whether the target board uses a multiplexer to select between USB
// host and device modes.
//
// Depends on: USB_ENABLE_UPDATE
// Exclusive of: None
// Requires: USB_MUX_PERIPH, USB_MUX_PORT, USB_MUX_PIN, USB_MUX_DEVICE
//
//*****************************************************************************
//#define USB_HAS_MUX

//*****************************************************************************
//
// Specifies the GPIO peripheral containing the pin which is used to select
// between USB host and device modes.  The value is of the form
// SYSCTL_RCGC2_GPIOx, where GPIOx represents the required GPIO port.
//
// Depends on: USB_ENABLE_UPDATE, USB_HAS_MUX
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define USB_MUX_PERIPH          SYSCTL_RCGC2_GPIOH

//*****************************************************************************
//
// Specifies the GPIO port containing the pin which is used to select between
// USB host and device modes.  The value is of the form GPIO_PORTx_BASE, where
// PORTx represents the required GPIO port.
//
// Depends on: USB_ENABLE_UPDATE, USB_HAS_MUX
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define USB_MUX_PORT            GPIO_PORTH_BASE

//*****************************************************************************
//
// Specifies the GPIO pin number used to select between USB host and device
// modes.  Valid values are 0 through 7.
//
// Depends on: USB_ENABLE_UPDATE, USB_HAS_MUX
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define USB_MUX_PIN             2

//*****************************************************************************
//
// Specifies the state of the GPIO pin required to select USB device-mode
// operation.  Valid values are 0 (low) or 1 (high).
//
// Depends on: USB_ENABLE_UPDATE, USB_HAS_MUX
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define USB_MUX_DEVICE          1

//*****************************************************************************
//
// Specifies whether the target board requires configuration of the pin used
// for VBUS.  This applies to Blizzard class and later devices.
//
// Depends on: USB_ENABLE_UPDATE
// Exclusive of: None
// Requires: USB_VBUS_PERIPH, USB_VBUS_PORT, USB_VBUS_PIN
//
//*****************************************************************************
#define USB_VBUS_CONFIG

//*****************************************************************************
//
// Specifies the GPIO peripheral containing the pin which is used for VBUS.
// The value is of the form SYSCTL_RCGCGPIO_Rx, where the Rx represents
// the required GPIO port.  This applies to Blizzard class and later 
// devices.
//
// Depends on: USB_ENABLE_UPDATE, USB_VBUS_CONFIG
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_VBUS_PERIPH          SYSCTL_RCGCGPIO_R1

//*****************************************************************************
//
// Specifies the GPIO port containing the pin which is used for VBUS.  The value
// is of the form GPIO_PORTx_BASE, where PORTx represents the required GPIO
// port.
//
// Depends on: USB_ENABLE_UPDATE, USB_VBUS_CONFIG
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_VBUS_PORT            GPIO_PORTB_BASE

//*****************************************************************************
//
// Specifies the GPIO pin number used for VBUS.  Valid values are 0 through 7.
//
// Depends on: USB_ENABLE_UPDATE, USB_VBUS_CONFIG
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_VBUS_PIN             1

//*****************************************************************************
//
// Specifies whether the target board requires configuration of the pin used
// for ID.  This applies to Blizzard class and later devices.
//
// Depends on: USB_ENABLE_UPDATE
// Exclusive of: None
// Requires: USB_ID_PERIPH, USB_ID_PORT, USB_ID_PIN
//
//*****************************************************************************
#define USB_ID_CONFIG

//*****************************************************************************
//
// Specifies the GPIO peripheral containing the pin which is used for ID.
// The value is of the form SYSCTL_RCGCGPIO_Rx, where the Rx represents
// the required GPIO port.  This applies to Blizzard class and later 
// devices.
//
// Depends on: USB_ENABLE_UPDATE, USB_ID_CONFIG
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_ID_PERIPH          SYSCTL_RCGCGPIO_R1

//*****************************************************************************
//
// Specifies the GPIO port containing the pin which is used for ID.  The value
// is of the form GPIO_PORTx_BASE, where PORTx represents the required GPIO
// port.
//
// Depends on: USB_ENABLE_UPDATE, USB_ID_CONFIG
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_ID_PORT            GPIO_PORTB_BASE

//*****************************************************************************
//
// Specifies the GPIO pin number used for ID.  Valid values are 0 through 7.
//
// Depends on: USB_ENABLE_UPDATE, USB_ID_CONFIG
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_ID_PIN             0

//*****************************************************************************
//
// Specifies whether the target board requires configuration of the pin used
// for DP.  This applies to Blizzard class and later devices.
//
// Depends on: USB_ENABLE_UPDATE
// Exclusive of: None
// Requires: USB_DP_PERIPH, USB_DP_PORT, USB_DP_PIN
//
//*****************************************************************************
#define USB_DP_CONFIG

//*****************************************************************************
//
// Specifies the GPIO peripheral containing the pin which is used for DP.
// The value is of the form SYSCTL_RCGCGPIO_Rx, where the Rx represents
// the required GPIO port.  This applies to Blizzard class and later 
// devices.
//
// Depends on: USB_ENABLE_UPDATE, USB_DP_CONFIG
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_DP_PERIPH          SYSCTL_RCGCGPIO_R10

//*****************************************************************************
//
// Specifies the GPIO port containing the pin which is used for DP.  The value
// is of the form GPIO_PORTx_BASE, where PORTx represents the required GPIO
// port.
//
// Depends on: USB_ENABLE_UPDATE, USB_DP_CONFIG
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_DP_PORT            GPIO_PORTL_BASE

//*****************************************************************************
//
// Specifies the GPIO pin number used for DP.  Valid values are 0 through 7.
//
// Depends on: USB_ENABLE_UPDATE, USB_DP_CONFIG
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_DP_PIN             6

//*****************************************************************************
//
// Specifies whether the target board requires configuration of the pin used
// for DM.  This applies to Blizzard class and later devices.
//
// Depends on: USB_ENABLE_UPDATE
// Exclusive of: None
// Requires: USB_DM_PERIPH, USB_DM_PORT, USB_DM_PIN
//
//*****************************************************************************
#define USB_DM_CONFIG

//*****************************************************************************
//
// Specifies the GPIO peripheral containing the pin which is used for DM.
// The value is of the form SYSCTL_RCGCGPIO_Rx, where the Rx represents
// the required GPIO port.  This applies to Blizzard class and later 
// devices.
//
// Depends on: USB_ENABLE_UPDATE, USB_DM_CONFIG
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_DM_PERIPH          SYSCTL_RCGCGPIO_R10

//*****************************************************************************
//
// Specifies the GPIO port containing the pin which is used for DM.  The value
// is of the form GPIO_PORTx_BASE, where PORTx represents the required GPIO
// port.
//
// Depends on: USB_ENABLE_UPDATE, USB_DM_CONFIG
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_DM_PORT            GPIO_PORTL_BASE

//*****************************************************************************
//
// Specifies the GPIO pin number used for DM.  Valid values are 0 through 7.
//
// Depends on: USB_ENABLE_UPDATE, USB_DM_CONFIG
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
#define USB_DM_PIN             7

//*****************************************************************************
//
// Selects an update via the CAN port.
//
// Depends on: None
// Exclusive of: ENET_ENABLE_UPDATE, I2C_ENABLE_UPDATE, SSI_ENABLE_UPDATE,
//               UART_ENABLE_UPDATE, USB_ENABLE_UPDATE
// Requires: CAN_RX_PERIPH, CAN_RX_PORT, CAN_RX_PIN, CAN_TX_PERIPH,
//           CAN_TX_PORT, CAN_TX_PIN, CAN_BIT_RATE, CRYSTAL_FREQ.
//
//*****************************************************************************
//#define CAN_ENABLE_UPDATE

//*****************************************************************************
//
// Enables the UART to CAN bridging for use when the CAN port is selected for
// communicating with the boot loader.
//
// Depends on: CAN_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define CAN_UART_BRIDGE

//*****************************************************************************
//
// The GPIO module to enable in order to configure the CAN0 Rx pin.  This will
// be one of the SYSCTL_RCGC2_GPIOx values, where "x" is replaced with the port
// name (such as B).  The value of "x" should match the value of "x" for
// CAN_RX_PORT.
//
// Depends on: CAN_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define CAN_RX_PERIPH           SYSCTL_RCGC2_GPIOA

//*****************************************************************************
//
// The GPIO port used to configure the CAN0 Rx pin.  This will be one of the
// GPIO_PORTx_BASE values, where "x" is replaced with the port name (such as
// B).  The value of "x" should match the value of "x" for CAN_RX_PERIPH.
//
// Depends on: CAN_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define CAN_RX_PORT             GPIO_PORTA_BASE

//*****************************************************************************
//
// The GPIO pin that is shared with the CAN0 Rx pin.  This is a value between 0
// and 7.
//
// Depends on: CAN_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define CAN_RX_PIN              4

//*****************************************************************************
//
// The GPIO module to enable in order to configure the CAN0 Tx pin.  This will
// be one of the SYSCTL_RCGC2_GPIOx values, where "x" is replaced with the port
// name (such as B).  The value of "x" should match the value of "x" for
// CAN_TX_PORT.
//
// Depends on: CAN_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define CAN_TX_PERIPH           SYSCTL_RCGC2_GPIOA

//*****************************************************************************
//
// The GPIO port used to configure the CAN0 Tx pin.  This will be one of the
// GPIO_PORTx_BASE values, where "x" is replaced with the port name (such as
// B).  The value of "x" should match the value of "x" for CAN_TX_PERIPH.
//
// Depends on: CAN_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define CAN_TX_PORT             GPIO_PORTA_BASE

//*****************************************************************************
//
// The GPIO pin that is shared with the CAN0 Tx pin.  This is a value between 0
// and 7.
//
// Depends on: CAN_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define CAN_TX_PIN              5

//*****************************************************************************
//
// The bit rate used on the CAN bus.  This must be one of 20000, 50000, 125000,
// 250000, 500000, or 1000000.  The CAN bit rate must be less than or equal to
// the crystal frequency divided by 8 (CRYSTAL_FREQ / 8).
//
// Depends on: CAN_ENABLE_UPDATE
// Exclusive of: None
// Requires: None
//
//*****************************************************************************
//#define CAN_BIT_RATE            1000000

//*****************************************************************************
//
// Performs application-specific low level hardware initialization on system
// reset.  If hooked, this function will be called immediately after the boot
// loader code relocation completes.  An application may perform any required
// low-level hardware initialization during this function.  Note that the
// system clock has not been set when this function is called.  Initialization
// that assumes the system clock is set may be performed in the BL_INIT_FN_HOOK
// function instead.
//
// void MyHwInitFunc(void);
//
//*****************************************************************************
//#define BL_HW_INIT_FN_HOOK      MyHwInitFunc

//*****************************************************************************
//
// Performs application-specific initialization on system reset.  If hooked,
// this function will be called during boot loader initialization to perform
// any board- or application-specific initialization which is required.  The
// function is called following a reset immediately after the selected boot
// loader peripheral has been configured and the system clock has been set.
//
// void MyInitFunc(void);
//
//*****************************************************************************
//#define BL_INIT_FN_HOOK         MyInitFunc

//*****************************************************************************
//
// Performs application-specific reinitialization on boot loader entry via SVC.
// If hooked, this function will be called during boot loader reinitialization
// to perform any board- or application-specific initialization which is
// required.  The function is called following boot loader entry from an
// application, after any system clock rate adjustments have been made.
//
// void MyReinitFunc(void);
//
//*****************************************************************************
//#define BL_REINIT_FN_HOOK       MyReinitFunc

//*****************************************************************************
//
// Informs an application that a download is starting.  If hooked, this
// function will be called when a firmware download is about to begin.  The
// function is called after the first data packet of the download is received
// but before it has been written to flash.
//
// void MyStartFunc(void);
//
//*****************************************************************************
//#define BL_START_FN_HOOK        MyStartFunc

//*****************************************************************************
//
// Informs an application of download progress.  If hooked, this function will
// be called periodically during a firmware download to provide progress
// information.  The function is called after each data packet is received from
// the host.  Parameters provide the number of bytes of data received and, in
// cases other than Ethernet update, the expected total number of bytes in the
// download (the TFTP protocol used by the Ethernet boot loader does not send
// the final image size before the download starts so in this case the ulTotal
// parameter is set to 0 to indicate that the size is unknown).
//
// void MyProgressFunc(unsigned long ulCompleted, unsigned long ulTotal);
//
//*****************************************************************************
//#define BL_PROGRESS_FN_HOOK     MyProgressFunc

//*****************************************************************************
//
// Informs an application that a download has completed.  If hooked, this
// function will be called when a firmware download has just completed.  The
// function is called after the final data packet of the download has been
// written to flash.
//
// void MyEndFunc(void);
//
//*****************************************************************************
//#define BL_END_FN_HOOK          MyEndFunc

//*****************************************************************************
//
// Allows an application to perform in-place data decryption during download.
// If hooked, this function will be called to perform in-place decryption of
// each data packet received during a firmware download.
//
// void MyDecryptionFunc(unsigned char *pucBuffer, unsigned long ulSize);
//
// This value takes precedence over ENABLE_DECRYPTION.  If both are defined,
// the hook function defined using BL_DECRYPT_FN_HOOK is called rather than the
// previously-defined DecryptData() stub function.
//
//*****************************************************************************
//#define BL_DECRYPT_FN_HOOK      MyDecryptionFunc

//*****************************************************************************
//
// Allows an application to force a new firmware download.  If hooked, this
// function will be called during boot loader initialization to determine
// whether a firmware update should be performed regardless of whether a valid
// main code image is already present.  If the function returns 0, the existing
// main code image is booted (if present), otherwise the boot loader will wait
// for a new firmware image to be downloaded.
//
// unsigned long MyCheckUpdateFunc(void);
//
// This value takes precedence over ENABLE_UPDATE_CHECK} if both are defined.
// If you wish to perform a GPIO check in addition to any other update check
// processing required, the GPIO code must be included within the hook function
// itself.
//
//*****************************************************************************
//#define BL_CHECK_UPDATE_FN_HOOK MyCheckUpdateFunc

//*****************************************************************************
//
// Allows an application to replace the flash block erase function.  If hooked,
// this function will be called whenever a block of flash is to be erased.  The
// function must erase the block and wait until the operation has completed.
// The size of the block which will be erased is defined by FLASH_BLOCK_SIZE.
//
// void MyFlashEraseFunc(unsigned long ulBlockAddr);
//
//*****************************************************************************
//#define BL_FLASH_ERASE_FN_HOOK  MyFlashEraseFunc

//*****************************************************************************
//
// Allows an application to replace the flash programming function.  If hooked,
// this function will be called to program the flash with firmware image data
// received during download operations.  The function must program the supplied
// data and wait until the operation has completed.
//
// void MyFlashProgramFunc(unsigned long ulDstAddr,
//                         unsigned char *pucSrcData,
//                         unsigned long ulLength);
//
//*****************************************************************************
//#define BL_FLASH_PROGRAM_FN_HOOK MyFlashProgramFunc

//*****************************************************************************
//
// Allows an application to replace the flash error clear function.  If hooked,
// this function must clear any flash error indicators and prepare to detect
// access violations that may occur in a future erase or program operations.
//
// void MyFlashClearErrorFunc(void);
//
//*****************************************************************************
//#define BL_FLASH_CL_ERR_FN_HOOK MyFlashClearErrorFunc

//*****************************************************************************
//
// Reports whether or not a flash access violation error has occurred.  If
// hooked, this function will be called after flash erase or program
// operations.  The return code indicates whether or not an access violation
// error occurred since the last call to the function defined by
// BL_FLASH_CL_ERR_FN_HOOK, with 0 indicating no errors and non-zero indicating
// an error.
//
// unsigned long MyFlashErrorFunc(void);
//
//*****************************************************************************
//#define BL_FLASH_ERROR_FN_HOOK  MyFlashErrorFunc

//*****************************************************************************
//
// Reports the total size of the device flash.  If hooked, this function will
// be called to determine the size of the supported flash device.  The return
// code is the number of bytes of flash in the device.  Note that this does not
// take into account any reserved space defined via the FLASH_RSVD_SPACE value.
//
// unsigned long MyFlashSizeFunc(void);
//
//*****************************************************************************
//#define BL_FLASH_SIZE_FN_HOOK   MyFlashSizeFunc

//*****************************************************************************
//
// Reports the address of the first byte after the end of the device flash.  If
// hooked, this function will be called to determine the address of the end of
// valid flash.  Note that this does not take into account any reserved space
// defined via the FLASH_RSVD_SPACE value.
//
// unsigned long MyFlashEndFunc(void);
//
//*****************************************************************************
//#define BL_FLASH_END_FN_HOOK    MyFlashEndFunc

//*****************************************************************************
//
// Checks whether the start address and size of an image are valid.  If hooked,
// this function will be called when a new firmware download is about to start.
// Parameters provided are the requested start address for the new download
// and, when using protocols which transmit the image length in advance, the
// size of the image that is to be downloaded.  The return code will be
// non-zero to indicate that the start address is valid and the image will fit
// in the available space, or 0 if either the address is invalid or the image
// is too large for the device.
//
// unsigned long MyFlashAddrCheckFunc(unsigned long ulAddr,
//                                    unsigned long ulSize);
//
//*****************************************************************************
//#define BL_FLASH_AD_CHECK_FN_HOOK MyFlashAddrCheckFunc

#endif // __BL_CONFIG_H__
