//*****************************************************************************
//
// acquire.c - Data acquisition module for data logger application.
//
// Copyright (c) 2011-2014 Texas Instruments Incorporated.  All rights reserved.
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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "driverlib/debug.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/hibernate.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_gpio.h"
#include "utils/ustdlib.h"
#include "drivers/slidemenuwidget.h"
#include "drivers/stripchartwidget.h"
#include "stripchartmanager.h"
#include "clocksetwidget.h"
#include "qs-logger.h"
#include "usbstick.h"
#include "usbserial.h"
#include "flashstore.h"
#include "menus.h"
#include "acquire.h"
#include "sensorlib/hw_mpu9150.h"
#include "sensorlib/hw_ak8975.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/ak8975.h"
#include "sensorlib/mpu9150.h"
#include "sensorlib/comp_dcm.h"
#include "driverlib/pin_map.h"

//*****************************************************************************
//
// This is the data acquisition module.  It performs acquisition of data from
// selected channels, starting and stopping data logging, storing acquired
// data, and running the strip chart display.
//
//*****************************************************************************

//*****************************************************************************
//
// Define MPU9150 I2C Address.
//
//*****************************************************************************
#define MPU9150_I2C_ADDRESS  0x69

//*****************************************************************************
//
// The following defines which ADC channel control should be used for each
// kind of data item.  Basically it maps how the ADC channels are connected
// on the board.  This is a hardware configuration.
//
// The Accelerometer, Gyro, and Compass info is brought across I2C3 as a 
// single data chunk an then unpacked.
//
//*****************************************************************************
#define CHAN_USER0              ADC_CTL_CH0
#define CHAN_USER1              ADC_CTL_CH1
#define CHAN_USER2              ADC_CTL_CH2
#define CHAN_USER3              ADC_CTL_CH3
#define CHAN_EXTTEMP            ADC_CTL_CH20
#define CHAN_CURRENT            ADC_CTL_CH23
#define CHAN_INTTEMP            ADC_CTL_TS

//*****************************************************************************
//
// The following are indexes for values returned from the ADC's. These index's
// map directly to g_pui32ADCSeq[]
//
//*****************************************************************************
#define ADCSEQ_USER0            0
#define ADCSEQ_USER1            1
#define ADCSEQ_USER2            2
#define ADCSEQ_USER3            3
#define ADCSEQ_EXTTEMP          4
#define ADCSEQ_INTTEMP          5
#define ADCSEQ_CURRENT          6

//*****************************************************************************
//
// The following maps the order that items are acquired and stored by the
// ADC sequencers.  Note that 16 samples are specified, using 2 of the
// 8 sample sequencers.  The current is sampled multiple times deliberately
// because that value tends to bounce around.  It is sampled multiple
// times and will be averaged.
//
//*****************************************************************************
uint32_t g_pui32ADCSeq[] =
{
    CHAN_USER0, CHAN_USER1, CHAN_USER2, CHAN_USER3, CHAN_EXTTEMP, CHAN_INTTEMP, 
    CHAN_CURRENT, CHAN_CURRENT,CHAN_CURRENT, CHAN_CURRENT, CHAN_CURRENT, 
    CHAN_CURRENT, CHAN_CURRENT,
};
#define NUM_ADC_CHANNELS        (sizeof(g_pui32ADCSeq) /                      \
                                 sizeof(g_pui32ADCSeq[0]))
#define NUM_CURRENT_SAMPLES     7

//*****************************************************************************
//
// Global storage for most recent Sensor Data
//
//*****************************************************************************
//
// Index values for sensor data
//
#define X 0
#define Y 1
#define Z 2
//
// Floating point data from SensorLib code
//
float g_pfAccel[3]; 
float g_pfGyro[3];
float g_pfMag[3];
//
// 16bit values post translation from floating point
//
int16_t g_i16Accel[3];
int16_t g_i16Gyro[3];
int16_t g_i16Mag[3];

//*****************************************************************************
//
// Global instance structure for the I2C master driver.
//
//*****************************************************************************
tI2CMInstance g_sI2CInst;

//*****************************************************************************
//
// Global instance structure for the MPU9150 sensor driver.
//
//*****************************************************************************
tMPU9150 g_sMPU9150Inst;

//*****************************************************************************
//
// Global flags to alert main that MPU9150 I2C transaction is complete
//
//*****************************************************************************
volatile uint_fast8_t g_vui8I2CDoneFlag;

//*****************************************************************************
//
// Global flags to alert main that MPU9150 I2C transaction error has occurred.
//
//*****************************************************************************
volatile uint_fast8_t g_vui8ErrorFlag;

//*****************************************************************************
//
// Global flags to alert main that MPU9150 data is ready to be retrieved.
//
//*****************************************************************************
volatile uint_fast8_t g_vui8DataFlag;

//*****************************************************************************
//
// A buffer to hold one set of ADC data that is acquired per sample time.
//
//*****************************************************************************
static uint32_t g_pui32ADCData[NUM_ADC_CHANNELS];

//*****************************************************************************
//
// The following variables hold the current time stamp, the next match time
// for sampling, and the period of time between samples.  All are stored in
// a 32.15 second.subsecond format.
//
//*****************************************************************************
static volatile uint32_t g_pui32TimeStamp[2];
static volatile uint32_t g_pui32NextMatch[2];
static uint32_t g_pui32MatchPeriod[2];

//*****************************************************************************
//
// The number of data items that are selected for acquisition.
//
//*****************************************************************************
static uint32_t g_ui32NumItems;

//*****************************************************************************
//
// A counter for the ADC interrupt handler.  It is used to track when new
// ADC data is acquired.
//
//*****************************************************************************
static volatile uint32_t g_ui32ADCCount;
static uint32_t g_ui32LastADCCount = 0;

//*****************************************************************************
//
// A counter for the RTC interrupt handler.
//
//*****************************************************************************
static volatile uint32_t g_pui32RTCInts;

//*****************************************************************************
//
// A flag to indicate that a keep alive packet is needed (when logging to host
// PC).
//
//*****************************************************************************
static volatile bool g_bNeedKeepAlive = false;

//*****************************************************************************
//
// Storage for a single record of acquired data.  This needs to be large
// enough to hold the time stamp and item mask (defined in the structure
// above) and as many possible data items that can be collected.  Force the
// buffer to be a multiple of 32-bits.
//
//*****************************************************************************
#define RECORD_SIZE             (sizeof(tLogRecord) + (NUM_LOG_ITEMS * 2))
static union
{
    uint32_t g_pui32RecordBuf[(RECORD_SIZE + 3) / sizeof(uint32_t)];
    tLogRecord sRecord;
}
g_sRecordBuf;

//*****************************************************************************
//
// Holds a pointer to the current configuration state, that is determined by
// the user's menu selections.
//
//*****************************************************************************
static tConfigState *g_psConfigState;

//*****************************************************************************
//
// MPU9150 Sensor callback function.  Called at the end of MPU9150 sensor
// driver transactions. This is called from I2C interrupt context. Therefore,
// we just set a flag and let main do the bulk of the computations and display.
//
//*****************************************************************************
void
MPU9150AppCallback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    //
    // If the transaction succeeded set the data flag to indicate to
    // application that this transaction is complete and data may be ready.
    //
    if(ui8Status == I2CM_STATUS_SUCCESS)
    {
        g_vui8I2CDoneFlag = 1;
    }

    //
    // Store the most recent status in case it was an error condition
    //
    g_vui8ErrorFlag = ui8Status;
}

//*****************************************************************************
//
// Function to wait for the MPU9150 transactions to complete. Use this to spin
// wait on the I2C bus.
//
//*****************************************************************************
void
MPU9150AppI2CWait(char *pcFilename, uint_fast32_t ui32Line)
{
    //
    // Put the processor to sleep while we wait for the I2C driver to
    // indicate that the transaction is complete.
    //
    while((g_vui8I2CDoneFlag == 0) && (g_vui8ErrorFlag == 0))
    {
        //
        // Do Nothing
        //
    }

    //
    // If an error occurred call the error handler immediately.
    //
    if(g_vui8ErrorFlag)
    {
        //MPU9150AppErrorHandler(pcFilename, ui32Line);
        //UARTprintf("ERROR in MPU9150AppErrorHandler\n");
    }

    //
    // clear the data flag for next use.
    //
    g_vui8I2CDoneFlag = 0;
}

//*****************************************************************************
//
// Called by the NVIC as a result of GPIO port B interrupt event. For this
// application GPIO port B pin 2 is the interrupt line for the MPU9150
//
//*****************************************************************************
void
IntGPIOb(void)
{
    unsigned long ulStatus;

    ulStatus = GPIOIntStatus(GPIO_PORTB_BASE, true);

    //
    // Clear all the pin interrupts that are set
    //
    GPIOIntClear(GPIO_PORTB_BASE, ulStatus);

    if(ulStatus & GPIO_PIN_2)
    {
        //
        // MPU9150 Data is ready for retrieval and processing.
        //
        MPU9150DataRead(&g_sMPU9150Inst, MPU9150AppCallback, &g_sMPU9150Inst);
    }
}

//*****************************************************************************
//
// Called by the NVIC as a result of I2C3 Interrupt. I2C3 is the I2C connection
// to the MPU9150.
//
//*****************************************************************************
void
MPU9150I2CIntHandler(void)
{
    //
    // Pass through to the I2CM interrupt handler provided by sensor library.
    // This is required to be at application level so that I2CMIntHandler can
    // receive the instance structure pointer as an argument.
    //
    I2CMIntHandler(&g_sI2CInst);
}

//*****************************************************************************
//
// This function is called when in VIEW mode.  The acquired data is written
// as text strings which will appear on the eval board display.
//
//*****************************************************************************
static void
UpdateViewerData(const tLogRecord *psRecord)
{
    static char pcViewerBuf[24];
    uint32_t ui32Idx, pui32RTC;
    struct tm sTime;

    //
    // Loop through the analog channels and update the text display strings.
    //
    for(ui32Idx = LOG_ITEM_USER0; ui32Idx <= LOG_ITEM_USER3; ui32Idx++)
    {
        usnprintf(pcViewerBuf, sizeof(pcViewerBuf), " CH%u: %u.%03u V ",
                  ui32Idx - LOG_ITEM_USER0, psRecord->pi16Items[ui32Idx] / 1000,
                  psRecord->pi16Items[ui32Idx] % 1000);
        MenuUpdateText(ui32Idx, pcViewerBuf);
    }

    //
    // Loop through the accel channels and update the text display strings.
    //
    for(ui32Idx = LOG_ITEM_ACCELX; ui32Idx <= LOG_ITEM_ACCELZ; ui32Idx++)
    {
        int16_t i16Accel = psRecord->pi16Items[ui32Idx];
        i16Accel *= (i16Accel < 0) ? -1 : 1;
        usnprintf(pcViewerBuf, sizeof(pcViewerBuf), " %c: %c%d.%02u g ",
                  (ui32Idx - LOG_ITEM_ACCELX) + 'X',
                  psRecord->pi16Items[ui32Idx] < 0 ? '-' : '+',
                  i16Accel / 100, i16Accel % 100);
        MenuUpdateText(ui32Idx, pcViewerBuf);
    }

    //
    // Update the display string for internal temperature.
    //
    usnprintf(pcViewerBuf, sizeof(pcViewerBuf), " INT: %d.%01u C ",
              psRecord->pi16Items[LOG_ITEM_INTTEMP] / 10,
              psRecord->pi16Items[LOG_ITEM_INTTEMP] % 10);
    MenuUpdateText(LOG_ITEM_INTTEMP, pcViewerBuf);

    //
    // Update the display string for external temperature.
    //
    usnprintf(pcViewerBuf, sizeof(pcViewerBuf), " EXT: %d.%01u C ",
              psRecord->pi16Items[LOG_ITEM_EXTTEMP] / 10,
              psRecord->pi16Items[LOG_ITEM_EXTTEMP] % 10);
    MenuUpdateText(LOG_ITEM_EXTTEMP, pcViewerBuf);

    //
    // Update the display string for processor current.
    //
    usnprintf(pcViewerBuf, sizeof(pcViewerBuf), " %u.%01u mA ",
              psRecord->pi16Items[LOG_ITEM_CURRENT] / 10,
              psRecord->pi16Items[LOG_ITEM_CURRENT] % 10);
    MenuUpdateText(LOG_ITEM_CURRENT, pcViewerBuf);

    //
    // Update the display strings for time and data.
    //
    pui32RTC = HibernateRTCGet();
    ulocaltime(pui32RTC, &sTime);
    usnprintf(pcViewerBuf, sizeof(pcViewerBuf), "%4u/%02u/%02u",
              sTime.tm_year+1900, sTime.tm_mon + 1, sTime.tm_mday);
    MenuUpdateText(TEXT_ITEM_DATE, pcViewerBuf);
    usnprintf(pcViewerBuf, sizeof(pcViewerBuf), "%02u:%02u:%02u",
              sTime.tm_hour, sTime.tm_min, sTime.tm_sec);
    MenuUpdateText(TEXT_ITEM_TIME, pcViewerBuf);
    
    //
    // Loop through the gyro channels and update the text display strings.
    //
    for(ui32Idx = LOG_ITEM_GYROX; ui32Idx <= LOG_ITEM_GYROZ; ui32Idx++)
    {
        int16_t i16Gyro = psRecord->pi16Items[ui32Idx];
        i16Gyro *= (i16Gyro < 0) ? -1 : 1;
        usnprintf(pcViewerBuf, sizeof(pcViewerBuf), " %c: %c%d.%02u  rad/s",
                  (ui32Idx - LOG_ITEM_GYROX) + 'X',
                  psRecord->pi16Items[ui32Idx] < 0 ? '-' : '+',
                  i16Gyro / 100, i16Gyro % 100);
        MenuUpdateText(ui32Idx, pcViewerBuf);
    }
    
    //
    // Loop through the mag channels and update the text display strings.
    //
    for(ui32Idx = LOG_ITEM_COMPASSX; ui32Idx <= LOG_ITEM_COMPASSZ; ui32Idx++)
    {
        int16_t i16Compass = psRecord->pi16Items[ui32Idx];
        i16Compass *= (i16Compass < 0) ? -1 : 1;
        usnprintf(pcViewerBuf, sizeof(pcViewerBuf), " %c: %c%d uT ",
                  (ui32Idx - LOG_ITEM_COMPASSX) + 'X',
                  psRecord->pi16Items[ui32Idx] < 0 ? '-' : '+',
                  i16Compass);
        MenuUpdateText(ui32Idx, pcViewerBuf);
    }
}


//*****************************************************************************
//
// This function is called from the AcquireRun() function and should be in
// context of the main thread.  It pulls data items from the ADC data buffer,
// and the I2C 9 axis data, converts units as needed, and stores the results 
// in a log record that is pointed at by the function parameter.
//
//*****************************************************************************
static void
ProcessDataItems(tLogRecord *psRecord)
{
    int32_t i32TempC;
    uint32_t ui32SelectedMask, ui32Millivolts, ui32Current;
    uint_fast8_t ui8Idx, ui8ItemIdx;

    //
    // Initialize locals.
    //
    ui8ItemIdx = 0;
    ui32SelectedMask = g_psConfigState->ui16SelectedMask;
    ui32Current = 0;
    
    //
    // Save the time stamp that was saved when the ADC data was acquired.
    // Also save into the record the bit mask of the selected data items.
    //
    psRecord->ui32Seconds = g_pui32TimeStamp[0];
    psRecord->ui16Subseconds = (uint16_t)g_pui32TimeStamp[1];
    psRecord->ui16ItemMask = (uint16_t)ui32SelectedMask;

    //
    // Process the user analog input channels.  These will be converted and
    // stored as millivolts.
    //
    for(ui8Idx = LOG_ITEM_USER0; ui8Idx <= LOG_ITEM_USER3; ui8Idx++)
    {
        //
        // Check to see if this item should be logged
        //
        if((1 << ui8Idx) & ui32SelectedMask)
        {
            ui32Millivolts = (g_pui32ADCData[ui8Idx] * 4100) / 819;
            psRecord->pi16Items[ui8ItemIdx++] = (int16_t)ui32Millivolts;
        }
    }

    //
    // Process the accelerometer data, because the data all comes across
    // I2C in one chunk if any accelerometer is desired just go ahead and 
    // process them all to save time. 
    //
    // floating point data provided in m/s^2.
    // divide by 9.81 to convert m/s^2 to g's
    // multiply by 100 to provide 2 decimal points of precision.
    //
    if( ((1 << LOG_ITEM_ACCELX) & ui32SelectedMask) | 
        ((1 << LOG_ITEM_ACCELY) & ui32SelectedMask) |
        ((1 << LOG_ITEM_ACCELZ) & ui32SelectedMask) )
    {
        // Get floating point version of the Accel Data in m/s^2.
        MPU9150DataAccelGetFloat(&g_sMPU9150Inst, &g_pfAccel[X], &g_pfAccel[Y],
                                     &g_pfAccel[Z]);

        g_i16Accel[0]= (int16_t) ((g_pfAccel[0] /9.81f)*100.f);
        g_i16Accel[1]= (int16_t) ((g_pfAccel[1] /9.81f)*100.f);
        g_i16Accel[2]= (int16_t) ((g_pfAccel[2] /9.81f)*100.f);
        // Log selected data to the record        
        if((1 << LOG_ITEM_ACCELX) & ui32SelectedMask){
            psRecord->pi16Items[ui8ItemIdx++] = g_i16Accel[X];}
        if((1 << LOG_ITEM_ACCELY) & ui32SelectedMask){
            psRecord->pi16Items[ui8ItemIdx++] = g_i16Accel[Y];}
        if((1 << LOG_ITEM_ACCELZ) & ui32SelectedMask){
            psRecord->pi16Items[ui8ItemIdx++] = g_i16Accel[Z];}
    }

    //
    // Process the external temperature. The temperature is stored in units
    // of 1/10 C.
    //
    if((1 << LOG_ITEM_EXTTEMP) & ui32SelectedMask)
    {
        i32TempC = (1866300 - ((200000 * g_pui32ADCData[ADCSEQ_EXTTEMP]) /
                                273)) / 1169;
        psRecord->pi16Items[ui8ItemIdx++] = (int16_t)i32TempC;
    }

    //
    // Process the internal temperature. The temperature is stored in units
    // of 1/10 C.
    //
    if((1 << LOG_ITEM_INTTEMP) & ui32SelectedMask)
    {
        i32TempC = 1475 - ((2250 * g_pui32ADCData[ADCSEQ_INTTEMP]) / 4095);
        psRecord->pi16Items[ui8ItemIdx++] = (int16_t)i32TempC;
    }

    //
    // Process the current. The current is stored in units of 100 uA,
    // (or 1/10000 A).  Multiple current samples were taken in order
    // to average and smooth the data.
    //
    if((1 << LOG_ITEM_CURRENT) & ui32SelectedMask)
    {
        //
        // Average all the current samples that are available in the ADC
        // buffer.
        //
        for(ui8Idx = ADCSEQ_CURRENT;
            ui8Idx < (ADCSEQ_CURRENT + NUM_CURRENT_SAMPLES); ui8Idx++)
        {
            ui32Current += g_pui32ADCData[ui8Idx];
        }
        ui32Current /= NUM_CURRENT_SAMPLES;

        //
        // Convert the averaged current into units
        //
        ui32Current = (ui32Current * 200) / 273;
        psRecord->pi16Items[ui8ItemIdx++] = (int16_t)ui32Current;
    }

    //
    // Process the Gyro data, because the data all comes across
    // I2C in one chunk if any gyro is desired just go ahead and 
    // process them all to save time. 
    //
    // floating data provided in rad/sec
    // multiply by 100 to provide 2 decimal points of precision.
    //
    if( ((1 << LOG_ITEM_GYROX) & ui32SelectedMask) | 
        ((1 << LOG_ITEM_GYROY) & ui32SelectedMask) |
        ((1 << LOG_ITEM_GYROZ) & ui32SelectedMask) )
    {
        // Get floating point version of angular velocities in rad/sec
        MPU9150DataGyroGetFloat(&g_sMPU9150Inst, &g_pfGyro[X], &g_pfGyro[Y],
                                     &g_pfGyro[Z]);

        g_i16Gyro[X]= (int16_t) (g_pfGyro[X] *100.f);
        g_i16Gyro[Y]= (int16_t) (g_pfGyro[Y] *100.f);
        g_i16Gyro[Z]= (int16_t) (g_pfGyro[Z] *100.f);

        // Log selected data to the record        
        if((1 << LOG_ITEM_GYROX) & ui32SelectedMask){
            psRecord->pi16Items[ui8ItemIdx++] = g_i16Gyro[X];}
        if((1 << LOG_ITEM_GYROY) & ui32SelectedMask){
            psRecord->pi16Items[ui8ItemIdx++] = g_i16Gyro[Y];}
        if((1 << LOG_ITEM_GYROZ) & ui32SelectedMask){
            psRecord->pi16Items[ui8ItemIdx++] = g_i16Gyro[Z];}
    }

    //
    // Process the Compass data, because the data all comes across
    // I2C in one chunk if any Compass is desired just go ahead and 
    // process them all to save time. 
    //
    // floating point data provided in tesla 
    // multiply by 1,000,000 to human friendly micro tesla unit
    if( ((1 << LOG_ITEM_COMPASSX) & ui32SelectedMask) | 
        ((1 << LOG_ITEM_COMPASSY) & ui32SelectedMask) |
        ((1 << LOG_ITEM_COMPASSZ) & ui32SelectedMask) )
    {
        // Get floating point version of magnetic fields strength in tesla
        MPU9150DataMagnetoGetFloat(&g_sMPU9150Inst, &g_pfMag[X], &g_pfMag[Y],
                                     &g_pfMag[Z]);

        //convert to micro-tesla
        g_i16Mag[X]= (int16_t) (g_pfMag[X] *1000000.f);
        g_i16Mag[Y]= (int16_t) (g_pfMag[Y] *1000000.f);
        g_i16Mag[Z]= (int16_t) (g_pfMag[Z] *1000000.f);
        
        // Log selected data to the record
        if((1 << LOG_ITEM_COMPASSX) & ui32SelectedMask){
            psRecord->pi16Items[ui8ItemIdx++] = g_i16Mag[X];}
        if((1 << LOG_ITEM_COMPASSY) & ui32SelectedMask){
            psRecord->pi16Items[ui8ItemIdx++] = g_i16Mag[Y];}
        if((1 << LOG_ITEM_COMPASSZ) & ui32SelectedMask){
            psRecord->pi16Items[ui8ItemIdx++] = g_i16Mag[Z];}
    }


    
}

//*****************************************************************************
//
// This is the handler for the ADC interrupt.  Even though more than one
// sequencer is used, they are configured so that this one runs last.
// Therefore when this ADC sequencer interrupt occurs, we know all of the ADC
// data has been acquired.
//
//*****************************************************************************
void
ADC0SS0Handler(void)
{
    //
    // Clear the interrupts for all ADC sequencers that are used.
    //
    MAP_ADCIntClear(ADC0_BASE, 0);
    MAP_ADCIntClear(ADC1_BASE, 0);

    //
    // Retrieve the data from all ADC sequencers
    //
    MAP_ADCSequenceDataGet(ADC0_BASE, 0, &g_pui32ADCData[0]);
    MAP_ADCSequenceDataGet(ADC1_BASE, 0, &g_pui32ADCData[8]);

    //
    // Set the time stamp, assume it is what was set for the last match
    // value.  This will be close to the actual time that the samples were
    // acquired, within a few microseconds.
    //
    g_pui32TimeStamp[0] = g_pui32NextMatch[0];
    g_pui32TimeStamp[1] = g_pui32NextMatch[1];

    //
    // Increment the ADC interrupt count
    //
    g_ui32ADCCount++;
}

//*****************************************************************************
//
// This is the handler for the RTC interrupt from the hibernate peripheral.
// It occurs on RTC match.  This handler will initiate an ADC acquisition,
// which will run all of the ADC sequencers.  Then it computes the next
// match value and sets it in the RTC.
//
//*****************************************************************************
void
RTCHandler(void)
{
    uint32_t ui32Status, ui32Seconds;

    //
    // Increment RTC interrupt counter
    //
    g_pui32RTCInts++;

    //
    // Clear the RTC interrupts (this can be slow for hib module)
    //
    ui32Status = HibernateIntStatus(1);
    HibernateIntClear(ui32Status);

    //
    // Read and save the current value of the seconds counter.
    //
    ui32Seconds = HibernateRTCGet();

    //
    // If we are sleep logging, then there will be no remembered value for
    // the next match value, which is also used as the time stamp when
    // data is collected.  In this case we will just use the current
    // RTC seconds.  This is safe because if sleep-logging is used, it
    // is only with periods of whole seconds, 1 second or longer.
    //
    if(g_psConfigState->ui32SleepLogging)
    {
        g_pui32NextMatch[0] = ui32Seconds;
        g_pui32NextMatch[1] = 0;
    }

    //
    // If we are logging data to PC and using a period greater than one
    // second, then use special handling.  For PC logging, if no data is
    // collected, then we must send a keep-alive packet once per second.
    //
    if((g_psConfigState->ui8Storage == CONFIG_STORAGE_HOSTPC) &&
       (g_pui32MatchPeriod[0] > 1))
    {
        //
        // If the current seconds count is less than the match value, that
        // means we got the interrupt due to one-second keep alive for the
        // host PC.
        //
        if(ui32Seconds < g_pui32NextMatch[0])
        {
            //
            // Set the next match for one second ahead (next keep-alive)
            //
            HibernateRTCMatchSet(0, ui32Seconds + 1);

            //
            // Set flag to indicate that a keep alive packet is needed
            //
            g_bNeedKeepAlive = true;

            //
            // Nothing else to do except wait for next keep alive or match
            //
            return;
        }

        //
        // Else, this is a real match so proceed to below to do a normal
        // acquisition.
        //
    }

    //
    // Kick off the next ADC acquisition.  When these are done they will
    // cause an ADC interrupt.
    //
    MAP_ADCProcessorTrigger(ADC1_BASE, 0);
    MAP_ADCProcessorTrigger(ADC0_BASE, 0);

    //
    // Set the next RTC match.  Add the match period to the previous match
    // value.  We are making an assumption here that there is enough time from
    // when the match interrupt occurred, to this point in the code, that we
    // are still setting the match time in the future.  If the period is too
    // long, then we could miss a match and never get another RTC interrupt.
    //
    g_pui32NextMatch[0] += g_pui32MatchPeriod[0];
    g_pui32NextMatch[1] += g_pui32MatchPeriod[1];
    if(g_pui32NextMatch[1] > 32767)
    {
        //
        // Handle subseconds rollover
        //
        g_pui32NextMatch[1] &= 32767;
        g_pui32NextMatch[0]++;
    }

    //
    // If logging to host PC at greater than 1 second period, then set the
    // next RTC wakeup for 1 second from now.  This will cause a keep alive
    // packet to be sent to the PC
    //
    if((g_psConfigState->ui8Storage == CONFIG_STORAGE_HOSTPC) &&
       (g_pui32MatchPeriod[0] > 1))
    {
        HibernateRTCMatchSet(0, ui32Seconds + 1);
    }
    else
    {
        //
        // Otherwise this is a normal match and the next match should also be a
        // normal match, so set the next wakeup to the calculated match time.
        //
        HibernateRTCMatchSet(0, g_pui32NextMatch[0]);
        HibernateRTCSSMatchSet(0, g_pui32NextMatch[1]);
    }

    //
    // Toggle the LED on the board so the user can see that the acquisition
    // is running.
    //
    MAP_GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_2,
                     ~MAP_GPIOPinRead(GPIO_PORTG_BASE, GPIO_PIN_2));

    //
    // Now exit the int handler.  The ADC will trigger an interrupt when
    // it is finished, and the RTC is set up for the next match.
    //
}

//*****************************************************************************
//
// This function is called from the application main loop to keep the
// acquisition running.  It checks to see if there is any new ADC data, and
// if so it processes the new ADC data.
//
// The function returns non-zero if data was acquired, 0 if no data was
// acquired.
//
//*****************************************************************************
int32_t
AcquireRun(void)
{
    tLogRecord *psRecord = &g_sRecordBuf.sRecord;

    //
    // Make sure we are properly configured to run
    //
    if(!g_psConfigState)
    {
        return(0);
    }

    //
    // Check to see if new ADC data is available
    //
    if((g_ui32ADCCount != g_ui32LastADCCount) & (g_vui8I2CDoneFlag ==1) )
    {

        g_ui32LastADCCount = g_ui32ADCCount;

        //
        // Process the ADC data and store it in the record buffer.
        //
        ProcessDataItems(psRecord);

        //
        // Reset data ready flag for I2C accel/gyro/compass data-
        //
        g_vui8I2CDoneFlag=0;

        //
        // Add the newly processed data to the strip chart.  Do not add to
        // strip start if sleep-logging.
        //
        if((g_psConfigState->ui8Storage != CONFIG_STORAGE_VIEWER) &&
           !g_psConfigState->ui32SleepLogging)
        {
            StripChartMgrAddItems(psRecord->pi16Items);
        }

        //
        // If USB stick is used, write the record to the USB stick
        //
        if(g_psConfigState->ui8Storage == CONFIG_STORAGE_USB)
        {
            USBStickWriteRecord(psRecord);
        }

        //
        // If host PC is used, write data to USB serial port
        //
        if(g_psConfigState->ui8Storage == CONFIG_STORAGE_HOSTPC)
        {
            USBSerialWriteRecord(psRecord);
        }

        //
        // If flash storage is used, write data to the flash
        //
        if(g_psConfigState->ui8Storage == CONFIG_STORAGE_FLASH)
        {
            FlashStoreWriteRecord(psRecord);

            //
            // If we are sleep logging, then save the storage address for
            // use in the next cycle.
            //
            if(g_psConfigState->ui32SleepLogging)
            {
                g_psConfigState->ui32FlashStore = FlashStoreGetAddr();
            }
        }
        else if(g_psConfigState->ui8Storage == CONFIG_STORAGE_VIEWER)
        {
            //
            // If in viewer mode, then update the viewer text strings.
            //
            UpdateViewerData(psRecord);
        }

        //
        // Return indication to caller that data was processed.
        //
        return(1);
    }
    else if((g_psConfigState->ui8Storage == CONFIG_STORAGE_HOSTPC) &&
            (g_bNeedKeepAlive == true))
    {
        //
        // Else there is no new data to process, but check to see if we are
        // logging to PC and a keep alive packet is needed.
        //
        // Clear keep-alive needed flag
        //
        g_bNeedKeepAlive = false;

        //
        // Make a keep alive packet by creating a record with timestamp of 0.
        //
        psRecord->ui32Seconds = 0;
        psRecord->ui16Subseconds = 0;
        psRecord->ui16ItemMask = 0;

        //
        // Transmit the dummy record to host PC.
        //
        USBSerialWriteRecord(psRecord);
    }

    //
    // Return indication that data was not processed.
    //
    return(0);
}

//*****************************************************************************
//
// This function is called to start an acquisition running.  It determines
// which channels are to be logged, enables the ADC/I2C sequencers, and computes
// the first RTC match value.  This will start the acquisition running.
//
//*****************************************************************************
void
AcquireStart(tConfigState *psConfig)
{
    uint32_t ui32Idx, pui32RTC[2], ui32SelectedMask;

    //
    // Check the parameters
    //
    ASSERT(psConfig);
    if(!psConfig)
    {
        return;
    }

    //
    // Update the config state pointer, save the selected item mask
    //
    g_psConfigState = psConfig;
    ui32SelectedMask = psConfig->ui16SelectedMask;

    //
    // Get the logging period from the logger configuration.  Split the
    // period into seconds and subseconds pieces and save for later use in
    // generating RTC match values.
    //
    g_pui32MatchPeriod[0] = psConfig->ui32Period >> 8;
    g_pui32MatchPeriod[1] = (psConfig->ui32Period & 0xFF) << 8;

    //
    // Determine how many channels are to be logged
    //
    ui32Idx = ui32SelectedMask;
    g_ui32NumItems = 0;
    while(ui32Idx)
    {
        if(ui32Idx & 1)
        {
            g_ui32NumItems++;
        }
        ui32Idx >>= 1;
    }

    //
    // Initialize the strip chart manager for a new run.  Don't bother with
    // the strip chart if we are using viewer mode, or sleep-logging.
    //
    if((psConfig->ui8Storage != CONFIG_STORAGE_VIEWER) &&
       !psConfig->ui32SleepLogging)
    {
        StripChartMgrInit();
        StripChartMgrConfigure(ui32SelectedMask);
    }

    //
    // Configure USB for memory stick if USB storage is chosen
    //
    if(psConfig->ui8Storage == CONFIG_STORAGE_USB)
    {
        USBStickOpenLogFile(0);
    }
    else if(psConfig->ui8Storage == CONFIG_STORAGE_FLASH)
    {

        //
        // Flash storage is to be used, prepare the flash storage module.
        // If already sleep-logging, then pass in the saved flash address
        // so it does not need to be searched.
        //
        if(psConfig->ui32SleepLogging)
        {
            FlashStoreOpenLogFile(psConfig->ui32FlashStore);
        }
        else
        {
            //
            // Otherwise not sleep logging, so just initialize the flash store,
            // this will cause it to search for the starting storage address.
            //
            FlashStoreOpenLogFile(0);
        }
    }

    //
    // Enable the ADC sequencers
    //
    MAP_ADCSequenceEnable(ADC0_BASE, 0);
    MAP_ADCSequenceEnable(ADC1_BASE, 0);

    //
    // Flush the ADC sequencers to be sure there is no lingering data.
    //
    MAP_ADCSequenceDataGet(ADC0_BASE, 0, g_pui32ADCData);
    MAP_ADCSequenceDataGet(ADC1_BASE, 0, g_pui32ADCData);

    //
    // Enable ADC interrupts
    //
    MAP_ADCIntClear(ADC0_BASE, 0);
    MAP_ADCIntClear(ADC1_BASE, 0);
    MAP_ADCIntEnable(ADC0_BASE, 0);
    MAP_IntEnable(INT_ADC0SS0);

    //
    // Enable data ready interrupt from accel/gyro/compass
    //
    MAP_IntEnable(INT_GPIOB);



    //
    // If we are not already sleep-logging, then initialize the RTC match.
    // If we are sleep logging then this does not need to be set up.
    //
    if(!psConfig->ui32SleepLogging)
    {
        //
        // Get the current RTC value
        //
        do
        {
            pui32RTC[0] = HibernateRTCGet();
            pui32RTC[1] = HibernateRTCSSGet();
        }
        while(pui32RTC[0] != HibernateRTCGet());

        //
        // Set an initial next match value.  Start with the subseconds always
        // 0 so the first match value will always be an even multiple of the
        // subsecond match.  Add 2 seconds to the current RTC just to be clear
        // of an imminent rollover.  This means that the first match will occur
        // between 1 and 2 seconds from now.
        //
        g_pui32NextMatch[0] = pui32RTC[0] + 2;
        g_pui32NextMatch[1] = 0;

        //
        // Now set the match value
        //
        HibernateRTCMatchSet(0, g_pui32NextMatch[0]);
        HibernateRTCSSMatchSet(0, g_pui32NextMatch[1]);
    }

    //
    // If we are configured to sleep, but not sleeping yet, then enter sleep
    // logging mode if allowed.
    //
    if(psConfig->bSleep && !psConfig->ui32SleepLogging)
    {
        //
        // Allow sleep logging if storing to flash at a period of 1 second
        // or greater.
        //
        if((psConfig->ui8Storage == CONFIG_STORAGE_FLASH) &&
           (psConfig->ui32Period >= 0x100))
        {
            psConfig->ui32SleepLogging = 1;
        }
    }

    //
    // Enable the RTC interrupts from the hibernate module
    //
    HibernateIntClear(HibernateIntStatus(0));
    HibernateIntEnable(HIBERNATE_INT_RTC_MATCH_0 | HIBERNATE_INT_PIN_WAKE);
    MAP_IntEnable(INT_HIBERNATE);

    //
    // Logging data should now start running
    //
}

//*****************************************************************************
//
// This function is called to stop an acquisition running.  It disables the
// ADC sequencers and the RTC match interrupt.
//
//*****************************************************************************
void
AcquireStop(void)
{
    //
    // Disable RTC interrupts
    //
    MAP_IntDisable(INT_HIBERNATE);
    
    //
    // Disable 9axis IRQ signal
    //
    MAP_IntDisable(INT_GPIOB);

    //
    // Disable ADC interrupts
    //
    MAP_IntDisable(INT_ADC0SS0);
    MAP_IntDisable(INT_ADC1SS0);

    //
    // Disable ADC sequencers
    //
    MAP_ADCSequenceDisable(ADC0_BASE, 0);
    MAP_ADCSequenceDisable(ADC1_BASE, 0);

    //
    // If USB stick is being used, then close the file so it will flush
    // the buffers to the USB stick.
    //
    if(g_psConfigState->ui8Storage == CONFIG_STORAGE_USB)
    {
        USBStickCloseFile();
    }

    //
    // Disable the configuration pointer, which acts as a flag to indicate
    // if we are properly configured for data acquisition.
    //
    g_psConfigState = 0;
}

//*****************************************************************************
//
// This function initializes the ADC hardware in preparation for data
// acquisition.
//
//*****************************************************************************
void
AcquireInit(void)
{
    uint32_t ui32Chan, ui32Base, ui32Seq, ui32ChCtl;


    //
    // Enable the ADC peripherals and the associated GPIO port
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);

    //
    // Enable I2C & associated GPIO ports
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C3);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //
    // Configure I2C Pins
    //
    MAP_GPIOPinConfigure(GPIO_PD0_I2C3SCL);
    MAP_GPIOPinConfigure(GPIO_PD1_I2C3SDA);
    MAP_GPIOPinTypeI2CSCL(GPIO_PORTD_BASE, GPIO_PIN_0);
    MAP_GPIOPinTypeI2C(GPIO_PORTD_BASE, GPIO_PIN_1);
    MAP_GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_2);
    MAP_GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_FALLING_EDGE);
    MAP_IntEnable(INT_GPIOB);
    
    //
    // Enabled LED GPIO
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_2);

    //
    // Configure the pins to be used as analog inputs.
    //
    MAP_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 |
                   GPIO_PIN_7 | GPIO_PIN_3);
    MAP_GPIOPinTypeADC(GPIO_PORTP_BASE, GPIO_PIN_0);

    //
    // Select the external reference for greatest accuracy.
    //
    MAP_ADCReferenceSet(ADC0_BASE, ADC_REF_EXT_3V);
    MAP_ADCReferenceSet(ADC1_BASE, ADC_REF_EXT_3V);

    //
    // Apply workaround for erratum 6.1, in order to use the
    // external reference.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    HWREG(GPIO_PORTB_BASE + GPIO_O_AMSEL) |= GPIO_PIN_6;

    //
    // Initialize both ADC peripherals using sequencer 0 and processor trigger.
    //
    MAP_ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    MAP_ADCSequenceConfigure(ADC1_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);

    //
    // Enter loop to configure all of the ADC sequencer steps needed to
    // acquire the data for the data logger.  Multiple ADC and sequencers
    // will be used in order to acquire all the channels.
    //
    for(ui32Chan = 0; ui32Chan < NUM_ADC_CHANNELS; ui32Chan++)
    {
        //
        // If this is the first ADC then set the base for ADC0
        //
        if(ui32Chan < 8)
        {
            ui32Base = ADC0_BASE;
            ui32Seq = 0;
        }
        else if(ui32Chan < 16)
        {
            //
            // Second ADC, set the base for ADC1
            //
            ui32Base = ADC1_BASE;
            ui32Seq = 0;
        }

        //
        // Get the channel control for each channel.  Test to see if it is the
        // last channel for the sequencer, and if so then also set the
        // interrupt and "end" flags.
        //
        ui32ChCtl = g_pui32ADCSeq[ui32Chan];
        if((ui32Chan == 7) || (ui32Chan == 15) ||
           (ui32Chan == (NUM_ADC_CHANNELS - 1)))
        {
            ui32ChCtl |= ADC_CTL_IE | ADC_CTL_END;
        }

        //
        // Configure the sequence step
        //
        MAP_ADCSequenceStepConfigure(ui32Base, ui32Seq, ui32Chan % 8,
                                      ui32ChCtl);
    }

    //
    // Initialize I2C3 Peripheral
    //
    I2CMInit(&g_sI2CInst, I2C3_BASE, INT_I2C3, 0xff, 0xff,
             MAP_SysCtlClockGet());

    //
    // Initialize the MPU9150 Driver.
    //
    MPU9150Init(&g_sMPU9150Inst, &g_sI2CInst, MPU9150_I2C_ADDRESS,
                MPU9150AppCallback, &g_sMPU9150Inst);

    //
    // Wait for transaction to complete
    //
    MPU9150AppI2CWait(__FILE__, __LINE__);

    //
    // Write application specifice sensor configuration such as filter settings
    // and sensor range settings.
    //
    g_sMPU9150Inst.pui8Data[0] = MPU9150_CONFIG_DLPF_CFG_94_98;
    g_sMPU9150Inst.pui8Data[1] = MPU9150_GYRO_CONFIG_FS_SEL_250;
    g_sMPU9150Inst.pui8Data[2] = (MPU9150_ACCEL_CONFIG_ACCEL_HPF_5HZ |
                                  MPU9150_ACCEL_CONFIG_AFS_SEL_2G);
    MPU9150Write(&g_sMPU9150Inst, MPU9150_O_CONFIG, g_sMPU9150Inst.pui8Data, 3,
                 MPU9150AppCallback, &g_sMPU9150Inst);
    
    //
    // Wait for transaction to complete
    //
    MPU9150AppI2CWait(__FILE__, __LINE__);

    //
    // Configure the data ready interrupt pin output of the MPU9150.
    //
    g_sMPU9150Inst.pui8Data[0] = MPU9150_INT_PIN_CFG_INT_LEVEL |
                                    MPU9150_INT_PIN_CFG_INT_RD_CLEAR |
                                    MPU9150_INT_PIN_CFG_LATCH_INT_EN;
    g_sMPU9150Inst.pui8Data[1] = MPU9150_INT_ENABLE_DATA_RDY_EN;
    MPU9150Write(&g_sMPU9150Inst, MPU9150_O_INT_PIN_CFG,
                 g_sMPU9150Inst.pui8Data, 2, MPU9150AppCallback,
                 &g_sMPU9150Inst);

    //
    // Wait for transaction to complete
    //
    MPU9150AppI2CWait(__FILE__, __LINE__);

    //
    // Initialize the DCM system. 50 hz sample rate.
    // accel weight = .2, gyro weight = .8, mag weight = .2
    //
    //CompDCMInit(&g_sCompDCMInst, 1.0f / 50.0f, 0.2f, 0.6f, 0.2f);

    //
    // Erase the configuration in case there was a prior configuration.
    //
    g_psConfigState = 0;
}
