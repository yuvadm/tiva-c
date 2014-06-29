//*****************************************************************************
//
// usb_sound.h - USB host audio handling header definitions.
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
// This is part of revision 2.1.0.12573 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#ifndef USB_SOUND_H_
#define USB_SOUND_H_

//*****************************************************************************
//
// The following are defines for the ui32Event value that is returned with the
// tEventCallback function provided to the USBSoundInit() function.
//
//*****************************************************************************

//
// A USB audio device has been connected.
//
#define SOUND_EVENT_READY       0x00000001

//
// A USB device has been disconnected.
//
#define SOUND_EVENT_DISCONNECT  0x00000002

//
// An unknown device has been connected.
//
#define SOUND_EVENT_UNKNOWN_DEV 0x00000003

typedef void (* tUSBBufferCallback)(void *pvBuffer, uint32_t ui32Event);
typedef void (* tEventCallback)(uint32_t ui32Event, uint32_t ui32Param);

extern void USBMain(uint32_t ui32Ticks);

extern void USBSoundInit(uint32_t ui32EnableReceive,
                      tEventCallback pfnCallback);
extern void USBSoundVolumeSet(uint32_t ui32Percent);
extern uint32_t USBSoundVolumeGet(uint32_t ui32Channel);

extern uint32_t USBSoundOutputFormatGet(uint32_t ui32SampleRate,
                                             uint32_t ui32Bits,
                                             uint32_t ui32Channels);
extern uint32_t USBSoundOutputFormatSet(uint32_t ui32SampleRate,
                                             uint32_t ui32Bits,
                                             uint32_t ui32Channels);
extern uint32_t USBSoundInputFormatGet(uint32_t ui32SampleRate,
                                            uint32_t ui32BitsPerSample,
                                            uint32_t ui32Channels);
extern uint32_t USBSoundInputFormatSet(uint32_t ui32SampleRate,
                                            uint32_t ui32Bits,
                                            uint32_t ui32Channels);

extern uint32_t USBSoundBufferOut(const void *pvData,
                                       uint32_t ui32Length,
                                       tUSBBufferCallback pfnCallback);

extern uint32_t USBSoundBufferIn(const void *pvData,
                                      uint32_t ui32Length,
                                      tUSBBufferCallback pfnCallback);

#endif
