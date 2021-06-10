//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     -   Transceiver mode
// Application Overview -   This is a sample application demonstrating the
//                          use of raw sockets on a CC3200 device.Based on the
//                          user input, the application either transmits data
//                          on the channel requested or collects Rx statistics
//                          on the channel.
//
// Application Details  -
// docs\examples\CC32xx_Transceiver_Mode.pdf
// or
// http://processors.wiki.ti.com/index.php/CC32xx_Transceiver_Mode
//
//*****************************************************************************


//*****************************************************************************
//
//! \addtogroup transceiver
//! @{
//
//*****************************************************************************

// Standard includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

// Simplelink includes
#include "simplelink.h"

//Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "interrupt.h"
#include "utils.h"
#include "uart.h"
#include "hw_memmap.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"

//Common interface includes
#include "common.h"
#ifndef NOTERM
#include "uart_if.h"
#endif

#include "pinmux.h"
#include <stdio.h>

// Driverlib includes
#include "hw_apps_rcm.h"
#include "hw_common_reg.h"
#include "hw_memmap.h"
#include "timer.h"
#include "utils.h"

// Common interface includes
#include "timer_if.h"
#include "gpio_if.h"

#include "globals.h"
#include "application.h"

#define APPLICATION_NAME        "TRANSCEIVER_MODE"
#define APPLICATION_VERSION     "1.1.1"

#define PREAMBLE            1        /* Preamble value 0- short, 1- long */
#define CPU_CYCLES_1MSEC (80*1000)
#define MSG_BROADCAST 1
#define MSG_ME 2
#define MSG_NONE 3
#define MSG_NOT_ME 4
#define BUF_LEN 1500

#define MAC_INFO_OFFSET             8
#define MAC_ADDRESS_LEN             6
#define IP_ADDRESS_LEN              4
#define CRC_LEN                     2
#define MAC_ADDRESS_DEST_OFFSET     MAC_INFO_OFFSET
#define MAC_ADDRESS_SRC_OFFSET      MAC_ADDRESS_DEST_OFFSET   + MAC_ADDRESS_LEN
#define IP_HEADER_OFFSET            MAC_ADDRESS_SRC_OFFSET    + MAC_ADDRESS_LEN
#define CRC_OFFSET                  IP_HEADER_OFFSET          + IP_HEADER_LEN
//#define IP_ADDRESS_SRC_OFFSET       CRC_OFFSET                + CRC_LEN
#define IP_ADDRESS_DEST_OFFSET      IP_ADDRESS_SRC_OFFSET      + IP_ADDRESS_LEN
#define HEADER_OFFSET               IP_ADDRESS_DEST_OFFSET    + IP_ADDRESS_LEN
#define DATA_LEN                    BUF_LEN-(HEADER_OFFSET)
#define IP_HEADER_LEN               10
#define IP_OFFSET_SENDER            2 * MAC_ADDRESS_LEN + IP_HEADER_LEN + CRC_LEN


#define dying_delay 10000
#define Channel 3
#define POWER 15
#define tran_rate 2
#define clean_period 10000
// Application specific status/error codes
typedef enum{
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    TX_CONTINUOUS_FAILED = -0x7D0,
    RX_STATISTICS_FAILED = TX_CONTINUOUS_FAILED - 1,
    DEVICE_NOT_IN_STATION_MODE = RX_STATISTICS_FAILED - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;


typedef struct
{
    int choice;
    int channel;
    int packets;
    SlRateIndex_e rate;
    int Txpower;
}UserIn;

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
volatile unsigned long  g_ulStatus = 0;//SimpleLink Status
unsigned long  g_ulGatewayIP = 0; //Network Gateway IP address
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
_u8 macAddressVal[SL_MAC_ADDR_LEN];
_u8 macAddressLen = SL_MAC_ADDR_LEN;
char broadcast[MAC_ADDRESS_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff};
char broadcast_IP[IP_ADDRESS_LEN] = {0xff,0xff,0xff,0xff};
char req_IP[IP_ADDRESS_LEN]={0x01,0x11,0x22,0x33};
char srcBuff[MAC_ADDRESS_LEN*2+1];
const int key = 23;
char myIp[4]={};
//char RawData_Ping[] = {
//       /*---- wlan header start -----*/
//       0x88,                                /* version , type sub type */
//       0x02,                                /* Frame control flag */
//       0x2C, 0x00,
//       0x00, 0x23, 0x75, 0x55,0x55, 0x55,   /* destination */
//       0x00, 0x22, 0x75, 0x55,0x55, 0x55,   /* bssid */
//       0x08, 0x00, 0x28, 0x19,0x02, 0x85,   /* source */
//       0x80, 0x42, 0x00, 0x00,
//       0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00, /* LLC */
//       /*---- ip header start -----*/
//       0x45, 0x00, 0x00, 0x54, 0x96, 0xA1, 0x00, 0x00, 0x40, 0x01,
//       0x57, 0xFA,                          /* checksum */
//       0xc0, 0xa8, 0x01, 0x64,              /* src ip */
//       0xc0, 0xa8, 0x01, 0x02,              /* dest ip  */
//       /* payload - ping/icmp */
//       0x08, 0x00, 0xA5, 0x51,
//       0x5E, 0x18, 0x00, 0x00, 0x41, 0x08, 0xBB, 0x8D, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00};

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************
/*static UserIn UserInput();*/
/*static int Tx_continuous(int iChannel,SlRateIndex_e rate,int iNumberOfPackets,\
                                   int iTxPowerLevel,long dIntervalMiliSec);
static int RxStatisticsCollect();*/
static void DisplayBanner(char * AppName);
static void BoardInit(void);

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************

//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************

static volatile unsigned long g_ulSysTickValue;
static volatile unsigned long g_ulBase;
static volatile unsigned long g_ulRefBase;
static volatile unsigned long g_ulRefTimerInts = 0;
static volatile unsigned long g_ulIntClearVector;
unsigned long long systime = 0;

//*****************************************************************************
//
//! The interrupt handler for the first timer interrupt.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************


int g_i = 0;
void
TimerBaseIntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    Timer_IF_InterruptClear(g_ulBase);

    systime ++;

//    if(systime%1000 == 0)
//    {
//        //GPIO_IF_LedToggle(MCU_GREEN_LED_GPIO);
//    }
}

//*****************************************************************************
//
//! The interrupt handler for the second timer interrupt.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
void
TimerRefIntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    Timer_IF_InterruptClear(g_ulRefBase);

    g_ulRefTimerInts ++;
    GPIO_IF_LedToggle(MCU_RED_LED_GPIO);
}
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'slWlanConnectAsyncResponse_t' - Applications
            // can use it if required
            //
            //  slWlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
            //

            // Copy new connection SSID and BSSID to global parameters
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT("[WLAN EVENT] STA Connected to the AP: %s , "
                      "BSSID: %x:%x:%x:%x:%x:%x\n\r",
                      g_ucConnectionSSID,g_ucConnectionBSSID[0],
                      g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                      g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                      g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION
            if(SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                UART_PRINT("[WLAN EVENT]Device disconnected from the AP: %s,"
                           " BSSID: %x:%x:%x:%x:%x:%x on application's"
                           " request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else
            {
                UART_PRINT("[WLAN ERROR]Device disconnected from the AP AP: %s,"
                           " BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        default:
        {
            UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;

            //Gateway IP address
            g_ulGatewayIP = pEventData->gateway;

            UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
                     "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));
        }
        break;

        default:
        {
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles HTTP server events
//!
//! \param[in]  pServerEvent - Contains the relevant event information
//! \param[in]    pServerResponse - Should be filled by the user with the
//!                                      relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse)
{
    // Unused in this application
}

//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]     pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}

//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    //
    // This application doesn't work w/ socket - Events are not expected
    //

}

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************

//*****************************************************************************
//
//! \brief This function initializes the application variables
//!
//! \param    None
//!
//! \return None
//!
//*****************************************************************************
static void InitializeAppVariables()
{
    g_ulStatus = 0;
    g_ulGatewayIP = 0;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
}

//*****************************************************************************
//! \brief This function puts the device in its default state. It:
//!           - Set the mode to STATION
//!           - Configures connection policy to Auto and AutoSmartConfig
//!           - Deletes all the stored profiles
//!           - Enables DHCP
//!           - Disables Scan policy
//!           - Sets Tx power to maximum
//!           - Sets power policy to normal
//!           - Unregister mDNS services
//!           - Remove all filters
//!
//! \param   none
//! \return  On success, zero is returned. On error, negative is returned
//*****************************************************************************
static long ConfigureSimpleLinkToDefaultState()
{
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    unsigned char ucVal = 1;
    unsigned char ucConfigOpt = 0;
    unsigned char ucConfigLen = 0;
    unsigned char ucPower = 0;

    long lRetVal = -1;
    long lMode = -1;

    lMode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lMode);

    // If the device is not in station-mode, try configuring it in station-mode
    if (ROLE_STA != lMode)
    {
        if (ROLE_AP == lMode)
        {
            // If the device is in AP mode, we need to wait for this event
            // before doing anything
            while(!IS_IP_ACQUIRED(g_ulStatus))
            {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
#endif
            }
        }

        // Switch to STA role and restart
        lRetVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(0xFF);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        // Check if the device is in station again
        if (ROLE_STA != lRetVal)
        {
            // We don't want to proceed if the device is not coming up in STA-mode
            return DEVICE_NOT_IN_STATION_MODE;
        }
    }

    // Get the device's version-information
    ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
    ucConfigLen = sizeof(ver);
    lRetVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ucConfigOpt,
                                &ucConfigLen, (unsigned char *)(&ver));
    ASSERT_ON_ERROR(lRetVal);

    UART_PRINT("Host Driver Version: %s\n\r",SL_DRIVER_VERSION);
    UART_PRINT("Build Version %d.%d.%d.%d.31.%d.%d.%d.%d.%d.%d.%d.%d\n\r",
    ver.NwpVersion[0],ver.NwpVersion[1],ver.NwpVersion[2],ver.NwpVersion[3],
    ver.ChipFwAndPhyVersion.FwVersion[0],ver.ChipFwAndPhyVersion.FwVersion[1],
    ver.ChipFwAndPhyVersion.FwVersion[2],ver.ChipFwAndPhyVersion.FwVersion[3],
    ver.ChipFwAndPhyVersion.PhyVersion[0],ver.ChipFwAndPhyVersion.PhyVersion[1],
    ver.ChipFwAndPhyVersion.PhyVersion[2],ver.ChipFwAndPhyVersion.PhyVersion[3]);

    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION,
                                SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal)
    {
        // Wait
        while(IS_CONNECTED(g_ulStatus))
        {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
#endif
        }
    }

    // Enable DHCP client
    lRetVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&ucVal);
    ASSERT_ON_ERROR(lRetVal);

    // Disable scan
    ucConfigOpt = SL_SCAN_POLICY(0);
    lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , ucConfigOpt, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);


    ucPower = 0;
    lRetVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (unsigned char *)&ucPower);
    ASSERT_ON_ERROR(lRetVal);
    lRetVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(lRetVal);

    memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    lRetVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(lRetVal);

    InitializeAppVariables();

    return lRetVal;
}

static void DisplayBanner(char * AppName)
{
    UART_PRINT("\n\n\n\r");
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\t\t\t CC3200 %s Application       \n\r", AppName);
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\n\n\n\r");
}

static void BoardInit(void)
{
#ifndef USE_TIRTOS

#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif

    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}
static int verify_data(char* data,  char* check){
    int sum = 0;
    int i = 0;
    int ch = (int)( (unsigned)check[1] << 8 ) + (int) check[0];
    for (i = 0; data[i] != 0; i++){
        sum = sum + data[i];
    }

    if (sum % key == ch){return 1;} else {return 0;}
}

static int send1pac_with_sd(int iSoc, int channel, int pwr, int rate, char* dest, char* IP_header, char* dest_ip, char* data, int dataSize ){
    int i=0;
    int idx = 0;
    int lRetVal;
    assert(dataSize+HEADER_OFFSET <= BUF_LEN);
    char s_data[BUF_LEN];
    short check=0;

    for(i = 0; i<BUF_LEN; i++)
    {
        s_data[i] = 0;
    }

    for(i = 0; i<dataSize; i++)
    {
        check = check + data[i];
    }
    for(i = 0; i<MAC_ADDRESS_LEN; i++)
    {
        s_data[i] = dest[i];
    }

    idx = 0;
    for(i = MAC_ADDRESS_LEN; i<MAC_ADDRESS_LEN*2; i++)
    {
        s_data[i] = macAddressVal[idx++];
    }

    idx = 0;
    for (i=MAC_ADDRESS_LEN*2; i<MAC_ADDRESS_LEN*2+IP_HEADER_LEN; i++)
    {
        s_data[i] = IP_header[idx++];
    }

    check = check % key;

    s_data[MAC_ADDRESS_LEN*2+IP_HEADER_LEN]   = (unsigned)check &  0xff;
    s_data[MAC_ADDRESS_LEN*2+IP_HEADER_LEN+1] = (unsigned)check >> 8  ;

    idx = 0;
    for(i = IP_OFFSET_SENDER; i<IP_OFFSET_SENDER+IP_ADDRESS_LEN; i++)
    {
        s_data[i] = myIp[idx++];
    }

    idx = 0;
    for(i = IP_OFFSET_SENDER+IP_ADDRESS_LEN; i<IP_OFFSET_SENDER+2*IP_ADDRESS_LEN; i++)
    {
        s_data[i] = dest_ip[idx++];
    }

    idx = 0;
    for (i=IP_OFFSET_SENDER+2*IP_ADDRESS_LEN; i < IP_OFFSET_SENDER+IP_ADDRESS_LEN*2 + dataSize; i++)//adjust after adding IPs
    {
        s_data[i]=data[idx++];
    }

    lRetVal = sl_Send(iSoc,s_data,BUF_LEN,\
                       SL_RAW_RF_TX_PARAMS(channel,  rate, pwr, PREAMBLE));
    if(lRetVal < 0)
    {
        ASSERT_ON_ERROR(lRetVal);
        return -1;
    }

    return lRetVal;
}


int isBroadcastMac(char* dest){
    return (dest[0]==broadcast[0] & dest[1]==broadcast[1] & dest[2]==broadcast[2] \
            & dest[3]==broadcast[3] & dest[4]==broadcast[4] & dest[5]==broadcast[5]) > 0;
}

int isBroadcastIP(char* dest){
    return (dest[0]==broadcast_IP[0] & dest[1]==broadcast_IP[1] & dest[2]==broadcast_IP[2] & dest[3]==broadcast_IP[3] ) > 0;
}

int isMyMac(char* dest){

    return (dest[0]==macAddressVal[0] & dest[1]==macAddressVal[1] & dest[2]==macAddressVal[2] \
            & dest[3]==macAddressVal[3] & dest[4]==macAddressVal[4] & dest[5]==macAddressVal[5] ) > 0;
}

static void parse(char* acBuffer,char* dest, char* src, char* dest_ip, char* src_ip, char* ip_head, char* rdata, char* check){
        int i;
        int idx = 0;
        for(i = MAC_ADDRESS_DEST_OFFSET; i< MAC_ADDRESS_DEST_OFFSET + MAC_ADDRESS_LEN; i++)
        {
            dest[idx++] = acBuffer[i];
    //        dest_t[idx++] = acBuffer[i];
        }

        idx = 0;
        for(i = MAC_ADDRESS_SRC_OFFSET; i< MAC_ADDRESS_SRC_OFFSET + MAC_ADDRESS_LEN; i++)
        {
            src[idx++] = acBuffer[i];
    //        src_t[idx++] = acBuffer[i];
        }

        idx = 0;
         for(i = IP_HEADER_OFFSET ; i<IP_HEADER_OFFSET+IP_HEADER_LEN; i++)
         {
             ip_head[idx++] = acBuffer[i];
         }

         idx=0;
         for (i=CRC_OFFSET; i<CRC_OFFSET+CRC_LEN; i++){
             check[idx++] = acBuffer[i];
         }
        idx = 0;
         for(i = IP_ADDRESS_SRC_OFFSET; i<IP_ADDRESS_SRC_OFFSET+IP_ADDRESS_LEN; i++)
         {
             src_ip[idx++] = acBuffer[i];
         }
        idx = 0;
         for(i = IP_ADDRESS_DEST_OFFSET; i<IP_ADDRESS_DEST_OFFSET+IP_ADDRESS_LEN; i++)
         {
             dest_ip[idx++] = acBuffer[i];
         }

        idx = 0;
        for(i = HEADER_OFFSET; i<BUF_LEN; i++)
        {
            rdata[idx++] = acBuffer[i];
    //        data_t[idx++] = acBuffer[i];
        }
}

static int sd_setup(_u8 iChannel, SlSuseconds_t timeOut)
{
    int iSoc = sl_Socket(SL_AF_RF,SL_SOCK_RAW,iChannel);
    long lRetVal = -1;
    ASSERT_ON_ERROR(iSoc);
    struct SlTimeval_t timeval;

    timeval.tv_sec  =  0;             // Seconds
    timeval.tv_usec = timeOut;         // Microseconds.

    // Enable receive timeout
    lRetVal = sl_SetSockOpt(iSoc,SL_SOL_SOCKET,SL_SO_RCVTIMEO, &timeval, \
                                  sizeof(timeval));
    ASSERT_ON_ERROR(lRetVal);

    return iSoc;
}

static int recv1pac_with_sd(int iSoc, char data[DATA_LEN], char srcmac[MAC_ADDRESS_LEN], char src_ip[IP_ADDRESS_LEN], char ip_head[IP_HEADER_LEN], char check[CRC_LEN])
{
    char acBuffer[BUF_LEN];
    long lRetVal = -1;
    char dest[MAC_ADDRESS_LEN]={};
    char dest_ip[IP_ADDRESS_LEN]={};

    lRetVal = sl_Recv(iSoc,acBuffer,BUF_LEN,0);

    if(lRetVal < 0 && lRetVal != SL_EAGAIN)
    {
        //error
        ASSERT_ON_ERROR(sl_Close(iSoc));
        ASSERT_ON_ERROR(lRetVal);
        LOOP_FOREVER();
    }
    else
    {
      //  static void parse(char* acBuffer,char* dest, char* src, char* dest_ip, char* src_ip, char* ip_head, char* rdata, char* check){

        parse(acBuffer,dest, srcmac, dest_ip, src_ip, ip_head, data, check);

        if(isMyMac(dest))/// & isMyIP(dest_ip))
        {
            return MSG_ME;
        }
        else if(isBroadcastMac(dest) & isBroadcastIP(dest_ip) & isBroadcastIP(src_ip))
        {
            return MSG_BROADCAST;
        }
        else
        {
            return MSG_NONE;
        }
    }

}

static int sd_listen(int iSoc, char* srcmac, char* src_ip, char* ip_head, char* dataBuffer, char* check, int timeout){
    int now = systime;
    int last= systime;
    int ret = recv1pac_with_sd(iSoc, dataBuffer, srcmac ,src_ip, ip_head, check);
    while((ret != MSG_ME && ret != MSG_BROADCAST) && (now-last < timeout))
    {
        now = systime;
        ret = recv1pac_with_sd(iSoc, dataBuffer, srcmac, src_ip, ip_head, check);
    }
    return ret;

}

static void assignIP(char* assigned_ip, int last8Bits){
    int i = 0;
    for (i = 0; i < IP_ADDRESS_LEN-1; i++)
    {
        assigned_ip[i]=0x0a;
    }
    assigned_ip [3] = last8Bits;

}

void init_board(){

    long lRetVal = -1;
    unsigned char policyVal;
    BoardInit();
    PinMuxConfig();
    InitTerm();
    InitializeAppVariables();
    DisplayBanner(APPLICATION_NAME);
    lRetVal = ConfigureSimpleLinkToDefaultState();

    if(lRetVal < 0)
    {
        if (DEVICE_NOT_IN_STATION_MODE == lRetVal)
          UART_PRINT("Failed to configure the device in its default state \n\r");
          LOOP_FOREVER();
    }

    UART_PRINT("Device is configured in default state \n\r");

    CLR_STATUS_BIT_ALL(g_ulStatus);
    lRetVal = sl_Start(0, 0, 0);
    if (lRetVal < 0 || ROLE_STA != lRetVal)
    {
        UART_PRINT("Failed to start the device \n\r");
        LOOP_FOREVER();
    }

    UART_PRINT("Device started as STATION \n\r");
    lRetVal = sl_WlanPolicySet(  SL_POLICY_CONNECTION,
                  SL_CONNECTION_POLICY(0,0,0,0,0),
                  &policyVal,
                  1 /*PolicyValLen*/);
    if (lRetVal < 0)
    {
        UART_PRINT("Failed to set policy \n\r");
        LOOP_FOREVER();
    }

    sl_NetCfgGet(SL_MAC_ADDRESS_GET,NULL,&macAddressLen,(_u8 *)macAddressVal);


}

char* get_mac_as_ascii(char* srcmac){
    int i;
    for (i = 0; i < 6; i++){
        sprintf(&srcBuff[i*2],"%02x", srcmac[i]);
    }
    srcBuff[12] = '\0';
    return srcBuff;
}

static void set_array(char* arr, int len)
{
    int idx = 0;
    for(idx = 0; idx<len; idx++)
    {
        arr[idx] = 0;
    }
}

static void set_array_eq(char* arr1, char* arr2, int len)
{
    int idx = 0;
    for(idx = 0; idx<len; idx++)
    {
        arr1[idx] = arr2[idx];
    }
}

static int is_array_eq(char* arr1, char* arr2, int len)
{
    int idx = 0;
    for(idx = 0; idx<len; idx++)
    {
        if (arr1[idx] != arr2[idx])
        {
            return 0;
        }
        else if (idx == (len-1))
        {
            return 1;
        }
    }
    return 0;
}

static void num2bytes (unsigned long long time, char* arr, _u8 len)
{
int idx = 0;

for(idx = 0; idx < len; idx++)
{
 arr[idx] = (char)(time >> (8*idx));
}

}

char intBuffer[10];
char* get_u8_as_string(int num)
{
    sprintf(intBuffer, "%d", num);
    return intBuffer;
}
int succ=0;

//int max_cap;
//static int find_node(char mac[MAC_ADDRESS_LEN]){
//    int idx = 0;
//    for(idx=0; idx<max_cap; idx++){
//        if(is_array_eq(latched_nodes[idx].MAC,mac,MAC_ADDRESS_LEN)){
//            return idx;
//        }
//    }
//    return -1;
//}

void main()
{

    init_board();
    // INIT SYSTIME TIMER
    g_ulBase = TIMERA0_BASE;
    Timer_IF_Init(PRCM_TIMERA0, g_ulBase, TIMER_CFG_PERIODIC, TIMER_A, 0);
    Timer_IF_IntSetup(g_ulBase, TIMER_A, TimerBaseIntHandler);
    Timer_IF_Start(g_ulBase, TIMER_A, 1);

    APP_sink();

}
