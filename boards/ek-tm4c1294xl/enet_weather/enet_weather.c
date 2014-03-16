//*****************************************************************************
//
// enet_weather.c - Sample Weather application using lwIP.
//
// Copyright (c) 2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_gpio.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/emac.h"
#include "drivers/pinout.h"
#include "utils/cmdline.h"
#include "utils/locator.h"
#include "utils/flash_pb.h"
#include "utils/lwiplib.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "eth_client.h"
#include "enet_weather.h"
#include "commands.h"
#include "json.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Ethernet Weather Application (enet_weather)</h1>
//!
//! This example application demonstrates the operation of the Tiva C series
//! evaluation kit as a weather reporting application.
//!
//! The application supports updating weather information from Open Weather Map
//! weather provider(http://openweathermap.org/).  The application uses the
//! lwIP stack to obtain an address through DNS, resolve the address of the
//! Open Weather Map site and then build and handle all of the requests
//! necessary to access the weather information.  The application can also use
//! a web proxy, allows for a custom city to be added to the list of cities and
//! toggles temperature units from Celsius to Fahrenheit.  The application
//! scrolls through the city list at a fixed interval when there is a valid
//! IP address and displays this information over the UART.
//!
//! The application will print the city name, status, humidity, current temp
//! and the high/low. In addition the application will display what city it
//! is currently updating. Once the app has scrolled through the cities a
//! a defined amount of times, it will attempt to update the information.
//!
//! UART0, connected to the Virtual Serial Port and running at 115,200, 8-N-1,
//! is used to display messages from this application.
//!
//!/ For additional details about Open Weather Map refer to their web page at:
//! http://openweathermap.org/
//!
//! For additional details on lwIP, refer to the lwIP web page at:
//! http://savannah.nongnu.org/projects/lwip/
//
//*****************************************************************************

//*****************************************************************************
//
// Connection states for weather application.
//
//*****************************************************************************
volatile enum
{
    STATE_NOT_CONNECTED,
        STATE_NEW_CONNECTION,
        STATE_CONNECTED_IDLE,
        STATE_WAIT_DATA,
        STATE_UPDATE_CITY,
        STATE_WAIT_NICE,
}
g_iState = STATE_NOT_CONNECTED;

//*****************************************************************************
//
// Input buffer for the command line interpreter.
//
//*****************************************************************************
char g_cInput[APP_INPUT_BUF_SIZE];

//*****************************************************************************
//
// The city being displayed.
//
//*****************************************************************************
uint32_t g_ui32CityActive;

//*****************************************************************************
//
// The city being updated.
//
//*****************************************************************************
uint32_t g_ui32CityUpdating;

//*****************************************************************************
//
// Global for IP address.
//
//*****************************************************************************
uint32_t g_ui32IPaddr;

//*****************************************************************************
//
// Global to track whether we are processing commands or not.
//
//*****************************************************************************
uint32_t g_ui32ProcessingCmds;

//*****************************************************************************
//
// The delay count to reduce traffic to the weather server.
//
//*****************************************************************************
volatile uint32_t g_ui32Delay;

//*****************************************************************************
//
// Global to track number of times the app has cycled through the list of
// cities.
//
//*****************************************************************************
volatile uint32_t g_ui32Cycles;

//*****************************************************************************
//
// The delay count to update the UART.
//
//*****************************************************************************
uint32_t g_ui32UARTDelay;

//*****************************************************************************
//
// System Clock rate in Hertz.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

//*****************************************************************************
//
// Varriable to track when to display the scrolling list of cities.
//
//*****************************************************************************
uint32_t g_ui32ShowCities;

//*****************************************************************************
//
// Varriable to track when to update the UART.
//
//*****************************************************************************
uint32_t g_UpdateUART = 1;

//*****************************************************************************
//
// Storage for the filename list box widget string table.
//
//*****************************************************************************
#define NUM_CITIES              30

//*****************************************************************************
//
// The defaults for the flash and application settings.
//
//*****************************************************************************
static const sParameters g_sDefaultParams =
{
    0,
    {"Custom City Name"},
    {"your.proxy.com"},
    false,
    false,
    false,
    false
};

//
// The current live configuration settings for the application.
//
sParameters g_sConfig;

//*****************************************************************************
//
// The state of each city panel.
//
//*****************************************************************************
struct
{
    //
    // The last update time for this city.
    //
    uint32_t ui32LastUpdate;

    //
    // The current weather report data for this city.
    //
    tWeatherReport sReport;

    //
    // Indicates if the city needs updating.
    //
    bool bNeedsUpdate;

    //
    // The name of the current city.
    //
    const char *pcName;
}
g_psCityInfo[NUM_CITIES];

//
// The list of city names.
//
static const char *g_ppcCityNames[NUM_CITIES - 1] =
{
    "Austin, TX",
    "Beijing, China",
    "Berlin, Germany",
    "Boston, MA",
    "Buenos Aires, Argentina",
    "Chicago, IL",
    "Dallas, TX",
    "Frankfurt, Germany",
    "Hong Kong, HK",
    "Jerusalem, Israel",
    "Johannesburg, ZA",
    "London, England",
    "Mexico City, Mexico",
    "Moscow, Russia",
    "New Delhi, India",
    "New York, NY",
    "Paris, France",
    "Rome, Italy",
    "San Jose, CA",
    "Sao Paulo, Brazil",
    "Seoul, S. Korea",
    "Shanghai, China",
    "Shenzhen, China",
    "Singapore City, Singapore",
    "Sydney, Australia",
    "Taipei, Taiwan",
    "Tokyo, Japan",
    "Toronto, Canada",
    "Vancouver, Canada"
};

//*****************************************************************************
//
// Constant strings for status messages.
//
//*****************************************************************************
const char g_pcNotFound[] = "City Not Found";
const char g_pcServerBusy[] = "Server Busy";
const char g_pcWaitData[] = "Waiting for Data";

//*****************************************************************************
//
// Interrupt priority definitions.  The top 3 bits of these values are
// significant with lower values indicating higher priority interrupts.
//
//*****************************************************************************
#define SYSTICK_INT_PRIORITY    0x80
#define ETHERNET_INT_PRIORITY   0xC0

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
// Variables for the various temperature parametrics.
//
//*****************************************************************************
char g_pcTempHighLow[40] = "--/--C";
char g_pcTemp[40] = "--C";
char g_pcHumidity[40] = "Humidity: --%";
char g_pcStatus[40];
char g_pcCity[40];

//*****************************************************************************
//
// MAC address.
//
//*****************************************************************************
char g_pcMACAddr[40];

//*****************************************************************************
//
// IP address.
//
//*****************************************************************************
char g_pcIPAddr[20];

//*****************************************************************************
//
// Celsius to Fahrenheit conversion.
//
//*****************************************************************************
static int32_t
TempCtoF(int32_t i32Temp)
{
    //
    // Only convert if measurements are not in Celsius.
    //
    if(g_sConfig.bCelsius == false)
    {
        i32Temp = ((i32Temp * 9) / 5) + 32;
    }

    return(i32Temp);
}

//*****************************************************************************
//
// Reset a cities information so that it can be properly updated.
//
//*****************************************************************************
void
ResetCity(uint32_t ui32Idx)
{
    g_psCityInfo[ui32Idx].sReport.i32Pressure = INVALID_INT;
    g_psCityInfo[ui32Idx].sReport.i32Humidity = INVALID_INT;
    g_psCityInfo[ui32Idx].sReport.i32Temp = INVALID_INT;
    g_psCityInfo[ui32Idx].sReport.i32TempHigh = INVALID_INT;
    g_psCityInfo[ui32Idx].sReport.i32TempLow = INVALID_INT;
    g_psCityInfo[ui32Idx].sReport.pcDescription = 0;
    g_psCityInfo[ui32Idx].sReport.pui8Image = 0;
    g_psCityInfo[ui32Idx].sReport.ui32SunRise = 0;
    g_psCityInfo[ui32Idx].sReport.ui32SunSet = 0;
    g_psCityInfo[ui32Idx].sReport.ui32Time = 0;
    g_psCityInfo[ui32Idx].ui32LastUpdate = 0;

    //
    // Make sure to copy the name into the string for a custom city.
    //
    if(ui32Idx == NUM_CITIES - 1)
    {
        //
        // Custom city is in another list.
        //
        g_psCityInfo[ui32Idx].pcName = g_sConfig.pcCustomCity;

        if(g_ui32CityActive == ui32Idx)
        {
            //
            // Update the custom city name.
            //
            ustrncpy(g_pcCity, g_psCityInfo[ui32Idx].pcName, sizeof(g_pcCity));
        }

        //
        // The custom city only needs an update if enabled.
        //
        if(g_sConfig.bCustomEnabled)
        {
            g_psCityInfo[ui32Idx].bNeedsUpdate = true;
        }
    }
    else
    {
        g_psCityInfo[ui32Idx].pcName = g_ppcCityNames[ui32Idx];
        g_psCityInfo[ui32Idx].bNeedsUpdate = true;
    }
}

//*****************************************************************************
//
// Update the IP address string.
//
//*****************************************************************************
void
UpdateIPAddress(char *pcAddr, uint32_t ipAddr)
{
    uint8_t *pui8Temp = (uint8_t *)&ipAddr;

    if(ipAddr == 0)
    {
        ustrncpy(pcAddr, "IP: ---.---.---.---", sizeof(g_pcIPAddr));
    }
    else
    {
        usprintf(pcAddr,"IP: %d.%d.%d.%d", pui8Temp[0], pui8Temp[1],
                pui8Temp[2], pui8Temp[3]);
    }
}

//*****************************************************************************
//
// Handles the proxy select button presses.
//
//*****************************************************************************
void
ProxyEnable(void)
{
    //
    // If a city was waiting to be updated then reset its data.
    //
    if(g_iState != STATE_CONNECTED_IDLE)
    {
        ResetCity(g_ui32CityUpdating);
    }

    //
    // Reset the state to not connected.
    //
    g_iState = STATE_NOT_CONNECTED;

    //
    // Toggle the proxy setting.
    //
    if(!g_sConfig.bProxyEnabled)
    {
        //
        // Reset the IP address on the screen and disable the proxy which
        // resets the network interface and starts DHCP again.
        //
        UpdateIPAddress(g_pcIPAddr, 0);
        EthClientProxySet(0);

    }
    else
    {
        //
        // Enable the proxy which resets the network interface and starts DHCP
        // again.
        //
        EthClientProxySet(g_sConfig.pcProxy);
    }

    //
    // A change was made so update the settings in flash.
    //
    g_sConfig.bSave = true;
}

//*****************************************************************************
//
// Update the information for the current city.
//
//*****************************************************************************
void
UpdateCity(uint32_t ui32Idx, bool bDraw)
{
    char cUnits;
    bool bIntDisabled;

    //
    // Need to disable interrupts since this can be called from interrupt
    // handlers for both System tick and Ethernet controller and from the
    // main routine.
    //
    bIntDisabled = IntMasterDisable();

    if(g_sConfig.bCelsius)
    {
        cUnits = 'C';
    }
    else
    {
        cUnits = 'F';
    }

    //
    // Update the city.
    //
    ustrncpy(g_pcCity, g_psCityInfo[ui32Idx].pcName, sizeof(g_pcCity));

    //
    // Check if the humidity value is valid.
    //
    if(g_psCityInfo[ui32Idx].sReport.i32Humidity == INVALID_INT)
    {
        usprintf(g_pcHumidity, "Humidity: --");
    }
    else
    {
        usprintf(g_pcHumidity, "Humidity: %d",
                g_psCityInfo[ui32Idx].sReport.i32Humidity);
    }

    //
    // Copy the updated description.
    //
    if(g_psCityInfo[ui32Idx].sReport.pcDescription)
    {
        ustrncpy(g_pcStatus, g_psCityInfo[ui32Idx].sReport.pcDescription,
                sizeof(g_pcStatus));
    }
    else if((g_ui32CityUpdating == g_ui32CityActive) &&
            (g_iState != STATE_NOT_CONNECTED))
    {
        //
        // Waiting on data for this city.
        //
        ustrncpy(g_pcStatus, g_pcWaitData, sizeof(g_pcStatus));
    }
    else
    {
        //
        // No current status.
        //
        ustrncpy(g_pcStatus, "--", sizeof(g_pcStatus));
    }


    //
    // Check if the temperature value is valid.
    //
    if(g_psCityInfo[ui32Idx].sReport.i32Temp == INVALID_INT)
    {
        usprintf(g_pcTemp, "--%c", cUnits);
        usprintf(g_pcTempHighLow, "--/--%c",
                TempCtoF(g_psCityInfo[ui32Idx].sReport.i32TempHigh),
                TempCtoF(g_psCityInfo[ui32Idx].sReport.i32TempLow), cUnits);
    }
    else
    {
        usprintf(g_pcTemp, "%d%c",
                TempCtoF(g_psCityInfo[ui32Idx].sReport.i32Temp), cUnits);
        usprintf(g_pcTempHighLow, "%d/%d%c",
                TempCtoF(g_psCityInfo[ui32Idx].sReport.i32TempHigh),
                TempCtoF(g_psCityInfo[ui32Idx].sReport.i32TempLow), cUnits);
    }

    //
    // Update the information if requested.
    //
    if(bDraw)
    {
        UARTprintf((const char *)&g_pcCity);
        UARTprintf("\n\tStatus: ");
        UARTprintf((const char *)&g_pcStatus);
        UARTprintf("\n\t");
        UARTprintf((const char *)&g_pcHumidity);
        UARTprintf("%%\n\tTemperature: ");
        UARTprintf((const char *)&g_pcTemp);
        UARTprintf("\n\tHigh/Low: ");
        UARTprintf((const char *)&g_pcTempHighLow);
    }

    if(bIntDisabled == false)
    {
        IntMasterEnable();
    }
}

//*****************************************************************************
//
// Update the MAC address string.
//
//*****************************************************************************
void
UpdateMACAddr(void)
{
    uint8_t pui8MACAddr[6];

    EthClientMACAddrGet(pui8MACAddr);

    usprintf(g_pcMACAddr, "MAC: %02x:%02x:%02x:%02x:%02x:%02x",
            pui8MACAddr[0], pui8MACAddr[1], pui8MACAddr[2], pui8MACAddr[3],
            pui8MACAddr[4], pui8MACAddr[5]);
}

//*****************************************************************************
//
// Print the IP address string.
//
//*****************************************************************************
void
PrintIPAddress(char *pcAddr, uint32_t ipaddr)
{
    uint8_t *pui8Temp = (uint8_t *)&ipaddr;

    //
    // Convert the IP Address into a string.
    //
    UARTprintf("%d.%d.%d.%d\n", pui8Temp[0], pui8Temp[1], pui8Temp[2],
            pui8Temp[3]);
}

//*****************************************************************************
//
// Prompts the user for a command, and blocks while waiting for the user's
// input. This function will return after the execution of a single command.
//
//*****************************************************************************
void
CheckForUserCommands(void)
{
    int iStatus;

    //
    // Peek to see if a full command is ready for processing
    //
    if(UARTPeek('\r') == -1)
    {
        //
        // If not, return so other functions get a chance to run.
        //
        return;
    }

    //
    // If we do have commands, process them immediately in the order they were
    // received.
    //
    while(UARTPeek('\r') != -1)
    {
        //
        // Indicate we are processing a command.
        //
        g_ui32ProcessingCmds = 1;

        //
        // Get a user command back
        //
        UARTgets(g_cInput, APP_INPUT_BUF_SIZE);

        //
        // Process the received command
        //
        iStatus = CmdLineProcess(g_cInput);

        //
        // Handle the case of bad command.
        //
        if(iStatus == CMDLINE_BAD_CMD)
        {
            UARTprintf("Bad command!\n");
        }

        //
        // Handle the case of too many arguments.
        //
        else if(iStatus == CMDLINE_TOO_MANY_ARGS)
        {
            UARTprintf("Too many arguments for command processor!\n");
        }
    }

    //
    // Print a prompt
    //
    UARTprintf("\n> ");

}

//*****************************************************************************
//
// Updates the UART information.
//
//*****************************************************************************
void
UpdateUART(uint32_t ui32City)
{
    volatile uint32_t ui32IPaddr;
    uint32_t    ui32Idx;

    //
    // See if the user has hit 'ENTER'
    //
    if(UARTPeek('\r') != -1 && g_ui32ShowCities)
    {
        //
        // Get a user command back
        //
        UARTgets(g_cInput, APP_INPUT_BUF_SIZE);

        //
        // Update the state and redraw the UART.
        //
        g_UpdateUART = 1;
        g_ui32ShowCities = 0;
        g_ui32ProcessingCmds = 0;
        g_ui32CityActive = 0;
        g_ui32Cycles = 0;
        g_ui32UARTDelay = 0;
    }

    //
    // Get the IP address.
    //
    ui32IPaddr = EthClientAddrGet();

    //
    // If the IP changed, reassign.
    //
    if (g_ui32IPaddr != ui32IPaddr)
    {
        //
        // Reassign the global IP to the current.
        //
        g_ui32IPaddr = ui32IPaddr;

        //
        // Update the UART.
        //
        g_UpdateUART = 1;
    }

    //
    // Check if we should scroll the cities.
    //
    if (g_ui32UARTDelay == 0 && g_ui32ShowCities)
    {
        //
        // Clear the terminal.  Print the banner and IP place holder.
        //
        UARTprintf("\033[2J\033[H");
        UARTprintf("Ethernet Weather Example\n\n");
        UARTprintf("IP: ");

        //
        // Print the IP address.
        //
        PrintIPAddress(g_pcIPAddr, ui32IPaddr);
        UARTprintf("\n");
        UARTprintf("Hit 'ENTER' to exit\n\n");

        //
        // Print the current city information.
        //
        UpdateCity(ui32City, true);

        //
        // 'Updating' banner.
        //
        UARTprintf("\n__________________________\n\nUpdating:");

        if (g_ui32Cycles >= UPDATE_CYCLES)
        {
            //
            // Reset the cities so that they update.
            //
            for(ui32Idx = 0; ui32Idx < NUM_CITIES; ui32Idx++)
            {
                ResetCity(ui32Idx);
            }

            //
            // Indicate that we have reset all of the cities.
            //
            g_ui32Cycles = 0;
        }

        //
        // Reset the delay.
        //
        g_ui32UARTDelay = CYCLE_DELAY;

        if (ui32IPaddr != 0 && ui32IPaddr != 0xffffffff)
        {
            //
            // Increment the display city.
            //
            g_ui32CityActive++;

            //
            // Check if the current city index is at the end.
            //
            if ((g_ui32CityActive >= NUM_CITIES - 1 &&
                        g_sDefaultParams.bCustomEnabled == false) ||
                    (g_ui32CityActive >= NUM_CITIES &&
                     g_sDefaultParams.bCustomEnabled == true))
            {
                g_ui32CityActive = 0;
                g_ui32Cycles++;
            }
        }

        g_UpdateUART = 0;
    }
    else if (g_ui32UARTDelay == 0 && (ui32IPaddr == 0 ||
                ui32IPaddr == 0xffffffff))
    {
        //
        // Reset the delay.
        //
        g_ui32UARTDelay = CYCLE_DELAY;
    }

    else if (g_ui32UARTDelay > 0)
    {
        g_ui32UARTDelay--;
    }

    //
    // Check if we need to update the UART with the standard text.
    //
    if (g_UpdateUART && !g_ui32ShowCities && !g_ui32ProcessingCmds)
    {
        //
        // Clear the terminal.  Print the banner and IP place holder.
        //
        UARTprintf("\033[2J\033[H");
        UARTprintf("Ethernet Weather Example\n\n");
        UARTprintf("IP: ");

        //
        // Print the IP address.
        //
        PrintIPAddress(g_pcIPAddr, ui32IPaddr);
        UARTprintf("\n");
        UARTprintf("Type 'help' for help.\n\n>");

        //
        // Dont need to update anymore.
        //
        g_UpdateUART = 0;
    }

    //
    // If we are not scrolling. Check for commands.
    //
    if (!g_ui32ShowCities)
    {
        //
        // Check for user commands.
        //
        CheckForUserCommands();
    }
}

//*****************************************************************************
//
// The weather event handler.
//
//*****************************************************************************
void
WeatherEvent(uint32_t ui32Event, void* pvData, uint32_t ui32Param)
{

    if(ui32Event == ETH_EVENT_RECEIVE)
    {
        //
        // Let the main loop update the city.
        //
        g_iState = STATE_UPDATE_CITY;

        g_psCityInfo[g_ui32CityUpdating].ui32LastUpdate =
            g_psCityInfo[g_ui32CityUpdating].sReport.ui32Time;
    }
    else if(ui32Event == ETH_EVENT_INVALID_REQ)
    {
        g_psCityInfo[g_ui32CityUpdating].sReport.pcDescription = g_pcNotFound;
        g_iState = STATE_UPDATE_CITY;
    }
    else if(ui32Event == ETH_EVENT_CLOSE)
    {
        if(g_iState == STATE_WAIT_DATA)
        {
            g_psCityInfo[g_ui32CityUpdating].sReport.pcDescription =
                g_pcServerBusy;

            g_iState = STATE_UPDATE_CITY;
        }
    }

    //
    // If the update indicated no time, then just set the time to a value
    // to indicate that at least the update occurred.
    //
    if(g_psCityInfo[g_ui32CityUpdating].ui32LastUpdate == 0)
    {
        //
        // Failed to update for some reason.
        //
        g_psCityInfo[g_ui32CityUpdating].ui32LastUpdate = 1;
    }
}

//*****************************************************************************
//
// The interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Call the lwIP timer handler.
    //
    EthClientTick(SYSTEM_TICK_MS);

    //
    // Handle the delay between accesses to the weather server.
    //
    if(g_ui32Delay != 0)
    {
        g_ui32Delay--;
    }
}

//*****************************************************************************
//
// Network events handler.
//
//*****************************************************************************
void
EnetEvents(uint32_t ui32Event, void *pvData, uint32_t ui32Param)
{
    if(ui32Event == ETH_EVENT_CONNECT)
    {
        g_iState = STATE_NEW_CONNECTION;

        //
        // Update the string version of the address.
        //
        UpdateIPAddress(g_pcIPAddr, EthClientAddrGet());
    }
    else if(ui32Event == ETH_EVENT_DISCONNECT)
    {
        //
        // If a city was waiting to be updated then reset its data.
        //
        if(g_iState != STATE_CONNECTED_IDLE)
        {
            ResetCity(g_ui32CityUpdating);
        }

        g_iState = STATE_NOT_CONNECTED;

        //
        // Update the address to 0.0.0.0.
        //
        UpdateIPAddress(g_pcIPAddr, 0);
    }
}

//*****************************************************************************
//
// This example demonstrates the use of the Ethernet Controller.
//
//*****************************************************************************
int
main(void)
{
    enum
    {
        eRequestIdle,
        eRequestUpdate,
        eRequestForecast,
        eRequestCurrent,
        eRequestComplete,
    }
    iRequest;
    int32_t i32City;
    sParameters *pParams;
    iRequest = eRequestIdle;

    //
    // Make sure the main oscillator is enabled because this is required by
    // the PHY.  The system must have a 25MHz crystal attached to the OSC
    // pins.  The SYSCTL_MOSC_HIGHFREQ parameter is used when the crystal
    // frequency is 10MHz or higher.
    //
    SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);

    //
    // Run from the PLL at 120 MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                SYSCTL_OSC_MAIN |
                SYSCTL_USE_PLL |
                SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet(true, false);

    //
    // Initialize the UART.
    //
    UARTStdioConfig(0, 115200, g_ui32SysClock);

    //
    // Configure SysTick for a periodic interrupt at 10ms.
    //
    SysTickPeriodSet((g_ui32SysClock / 1000) * SYSTEM_TICK_MS);
    SysTickEnable();
    SysTickIntEnable();

    //
    // Initialized the flash program block and read the current settings.
    //
    FlashPBInit(FLASH_PB_START, FLASH_PB_END, 256);
    pParams = (sParameters *)FlashPBGet();

    //
    // Use the defaults if no settings were found.
    //
    if(pParams == 0)
    {
        g_sConfig = g_sDefaultParams;
    }
    else
    {
        g_sConfig = *pParams;
    }

    //
    // Initialize all of the cities.
    //
    for(i32City = 0; i32City < NUM_CITIES; i32City++)
    {
        ResetCity(i32City);
    }

    //
    // Set the IP address to 0.0.0.0.
    //
    UpdateIPAddress(g_pcIPAddr, 0);

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Set the interrupt priorities.  We set the SysTick interrupt to a higher
    // priority than the Ethernet interrupt to ensure that the file system
    // tick is processed if SysTick occurs while the Ethernet handler is being
    // processed.  This is very likely since all the TCP/IP and HTTP work is
    // done in the context of the Ethernet interrupt.
    //
    IntPriorityGroupingSet(4);
    IntPrioritySet(INT_EMAC0, ETHERNET_INT_PRIORITY);
    IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);

    if(g_sConfig.bProxyEnabled)
    {
        EthClientProxySet(g_sConfig.pcProxy);
        EthClientInit(EnetEvents);
    }
    else
    {
        EthClientProxySet(0);
        EthClientInit(EnetEvents);
    }

    UpdateMACAddr();

    //
    // Initialize the city index and enable swipe detection.
    //
    g_ui32CityActive = 0;
    g_ui32CityUpdating = 0;

    //
    // Get the IP address.
    //
    g_ui32IPaddr = EthClientAddrGet();

    //
    // Update the current city information.
    //
    UpdateUART(g_ui32CityActive);

    //
    // Loop forever.  All the work is done in interrupt handlers.
    //
    while(1)
    {
        if(g_iState == STATE_NEW_CONNECTION)
        {
            iRequest = eRequestIdle;

            g_iState = STATE_CONNECTED_IDLE;
        }
        else if(g_iState == STATE_CONNECTED_IDLE)
        {
            if(iRequest == eRequestIdle)
            {
                //
                // If this city needs updating then start an update.
                //
                if((g_psCityInfo[g_ui32CityUpdating].bNeedsUpdate) &&
                        ((g_ui32CityUpdating < NUM_CITIES - 1) ||
                         (g_sConfig.bCustomEnabled == true)))
                {
                    iRequest = eRequestUpdate;
                }

                if(iRequest != eRequestUpdate)
                {
                    //
                    // If the custom city is enabled and it needs updating,
                    // then update it first.
                    //
                    if((g_psCityInfo[NUM_CITIES - 1].bNeedsUpdate) &&
                            (g_sConfig.bCustomEnabled == true))
                    {
                        g_ui32CityUpdating = NUM_CITIES - 1;
                    }
                    else
                    {
                        //
                        // Move on to the next city to see if it needs to be
                        // updated on the next pass.
                        //
                        g_ui32CityUpdating++;
                    }

                    if(g_ui32CityUpdating >= NUM_CITIES)
                    {
                        g_ui32CityUpdating = 0;
                    }
                }
            }
            else if(iRequest == eRequestUpdate)
            {
                //
                // Print the current city being updated.
                //
                if (g_ui32ShowCities)
                {
                    UARTprintf("\n\t%s", 
                               g_psCityInfo[g_ui32CityUpdating].pcName);
                }

                g_iState = STATE_WAIT_DATA;

                //
                // Timeout at 10 seconds.
                //
                g_ui32Delay = 1000;

                WeatherForecast(iWSrcOpenWeatherMap,
                        g_psCityInfo[g_ui32CityUpdating].pcName,
                        &g_psCityInfo[g_ui32CityUpdating].sReport,
                        WeatherEvent);

                iRequest = eRequestForecast;
            }
            else if(iRequest == eRequestForecast)
            {
                g_iState = STATE_WAIT_DATA;

                //
                // Timeout at 10 seconds.
                //
                g_ui32Delay = 1000;

                WeatherCurrent(iWSrcOpenWeatherMap,
                        g_psCityInfo[g_ui32CityUpdating].pcName,
                        &g_psCityInfo[g_ui32CityUpdating].sReport,
                        WeatherEvent);

                iRequest = eRequestCurrent;
            }
            else if(iRequest == eRequestCurrent)
            {
                //
                // Return to the idle request state.
                //
                iRequest = eRequestIdle;

                //
                // Done updating this city.
                //
                g_psCityInfo[g_ui32CityUpdating].bNeedsUpdate = false;
            }
        }
        else if(g_iState == STATE_UPDATE_CITY)
        {
            if(iRequest == eRequestCurrent)
            {
                //
                // If the city is the current active city and the the
                // application is on the main screen then update the screen
                // and not just the values.
                //
                if(g_ui32CityUpdating == g_ui32CityActive)
                {
                    //
                    // Update the current city.
                    //
                    UpdateCity(g_ui32CityUpdating, false);
                }

                //
                // Done updating this city.
                //
                g_psCityInfo[g_ui32CityUpdating].bNeedsUpdate = false;
            }

            //
            // Go to the wait nice state.
            //
            g_iState = STATE_WAIT_NICE;

            //
            // 10ms * 10 is a 1 second delay between updates.
            //
            g_ui32Delay = SYSTEM_TICK_MS * 10;
        }
        else if(g_iState == STATE_WAIT_NICE)
        {
            //
            // Wait for the "nice" delay to not hit the server too often.
            //
            if(g_ui32Delay == 0)
            {
                g_iState = STATE_CONNECTED_IDLE;
            }
        }
        else if(g_iState == STATE_WAIT_DATA)
        {
            //
            // If no data has been received by the timeout then close the
            // connection.
            //
            if(g_ui32Delay == 0)
            {
                //
                // Close out the current connection.
                //
                EthClientTCPDisconnect();
            }
        }

        //
        // Update the UART.
        //
        UpdateUART(g_ui32CityActive);
    }
}

