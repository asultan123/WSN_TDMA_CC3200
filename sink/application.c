/*
 * application.c
 *
 *  Created on: May 4, 2019
 *      Author: USER
 */


#include "application.h"

void APP_test_systime()
{
    unsigned long long last = systime;
    MAP_UtilsDelay(80*1000*100);
    assert(systime>last);
    UART_PRINT("\r\nSYSTIME CHECK SUCCESS\r\n");
}

void APP_mac_layer_test_sender()
{
    APP_test_systime();
    assert(MAC_init()==MAC_INIT_SUCCESS);
    MAC_address target = {{0xf4,0xb8,0x5e,0x01,0x84,0xc1}};

    char data = 0;

    while(1)
    {
        UART_PRINT("\r\nSENDING DATA: %d TO BROADCAST MAC\r\n" , data);
        int res = MAC_send_data(&broadcast_mac, &data, 1);
        if(res == MAC_SEND_SUCCESS)
        {
            UART_PRINT("\r\nDATA SENT SUCCESSFULLY\r\n");
        }
        else
        {
            UART_PRINT("\r\nDATA NOT SENT\r\n");
        }

        UART_PRINT("\r\nSENDING DATA: %d TO TARGET MAC: " , data);
        UTIL_UART_print_mac(target);
        UART_PRINT("\r\n");

        res = MAC_send_data(&target, &data, 1);
        if(res == MAC_SEND_SUCCESS)
        {
            UART_PRINT("\r\nDATA SENT SUCCESSFULLY\r\n");
        }
        else
        {
            UART_PRINT("\r\nDATA NOT SENT\r\n");
        }

        data++;
        UTIL_delay(100);
    }
}

void APP_mac_layer_test_reciever()
{
    APP_test_systime();
    assert(MAC_init()==MAC_INIT_SUCCESS);
    MAC_address srcMac;
    char* data;
    int attempt = 1;

    while(1)
    {
        UART_PRINT("\r\nRECIEVING DATA ATTEMPT %d\r\n",attempt++);
        int res = MAC_recv_data(&srcMac, &data, 100, MAC_ACCEPT_BROADCAST);

        if(res != MAC_MSG_NONE)
        {
            UART_PRINT("\r\nDATA RECIEVED: %d FROM SRC MAC:", data[0]);
            UTIL_UART_print_mac(srcMac);
            UART_PRINT("\r\n");
        }
        else
        {
            UART_PRINT("\r\nDATA NOT RECIEVED\r\n");
        }
        UTIL_delay(1);
    }

}

void APP_ip_layer_test_sender()
{
    char sendData[7] = {'H','E','L','L','O',0,0};
    char* recvData;

    APP_test_systime();
    assert(IP_init(GLOBAL_ROLE_NODE)==IP_INIT_SUCCESS);
    assert(IP_request_ip() == IP_REQUEST_SUCCESS);

    char dataUpdate = 1;
    IP_address srcFromRecentlySentData;

    while(1)
    {
        if(IP_send_data(&gateway, sendData, 7) == IP_SEND_SUCCESS)
        {
            UART_PRINT("\r\nSENT DATA %s%d SUCCESSFULLY TO SINK\r\n",sendData,sendData[6]);
            sendData[6] = dataUpdate++;
            if(IP_recv_data(&srcFromRecentlySentData, &recvData, 100))
            {
                if(UTIL_ip_comp(&srcFromRecentlySentData, &gateway))
                {
                    UART_PRINT("\r\nMESSAGE RECIEVED FROM SINK %s%d\r\n",recvData,recvData[4]);
                }
            }
        }
        else
        {
            UART_PRINT("\r\nSENT DATA %s%d FAILED \r\n",sendData,sendData[6]);
        }
        UTIL_delay(100);
    }
}
void APP_ip_layer_test_reciever_dhcp()
{

    MAC_address srcMac;
    char* dataFromMac;
    int recvRes;

    assert(IP_init(GLOBAL_ROLE_SINK)==IP_INIT_SUCCESS);
    #if GLOBAL_VERBOSE == 1
    UART_PRINT("\r\nSTARTED DHCP\r\n");
    #endif
    while(1)
    {
        recvRes = MAC_recv_data(&srcMac, &dataFromMac, 100, MAC_ACCEPT_BROADCAST);

        if(recvRes == MAC_MSG_BROADCAST)
        {
            if(IP_is_request_msg(dataFromMac))
            {
                #if GLOBAL_VERBOSE == 1
                UART_PRINT("\r\nIP REQUEST DETECTED... ISSUING\r\n");
                #endif

                if(IP_issue_ip(&srcMac) == IP_ISSUE_SUCCESS)
                {
                    #if GLOBAL_VERBOSE == 1
                    UART_PRINT("\r\nISSUE SUCCESS\r\n");
                    #endif
                }
                else
                {
                    #if GLOBAL_VERBOSE == 1
                    UART_PRINT("\r\nISSUE FAIL\r\n");
                    #endif
                }
            }
        }
        else
        {

        }
    }
}

void APP_ip_layer_test_reciever()
{
    APP_test_systime();
    assert(IP_init(GLOBAL_ROLE_SINK)==IP_INIT_SUCCESS);

    UART_PRINT("\r\nSTARTED SINK\r\n");

    IP_address recvIp;
    char * data;
    char respData[5] = {'Y','E','S',0,0};
    while(1)
    {
        int res = IP_recv_data(&recvIp, &data, 100);
        if(res == IP_MSG_APPLICATION){
            UART_PRINT("\r\nAPPLICATION MESSAGE RECIEVED: %s%d", data,data[6]);
            UART_PRINT(" FROM SRC IP: ");
            UTIL_UART_print_ip(recvIp);
            UART_PRINT("\r\n");
            respData[4] = data[6];
            UART_PRINT("\r\nRESPONDING WITH %s%d\r\n", respData,respData[4]);
            IP_send_data(&recvIp, respData, 5);
        }
        else if(res == IP_MSG_NO_MSG)
        {
            //UART_PRINT("\r\nAN NO MSG HAS BEEN RECIEVED\r\n");
        }
        else if(res == IP_ISSUE_SUCCESS)
        {
            UART_PRINT("\r\nAN IP REQ WAS MADE AND HAS BEEN SERVICED SUCCESSFULLY\r\n");
        }
        else if(res == IP_ISSUE_FAIL)
        {
            UART_PRINT("\r\nAN IP REQ WAS MADE AND NO IP HAS BEEN ISSUED\r\n");
        }
        else if(res == IP_MSG_INVALID)
        {
            UART_PRINT("\r\nINVALID MESSAGE RECIEVED\r\n");
        }
        else
        {
            UART_PRINT("\r\nUNKNOWN RESPONSE FROM IP RECV DATA\r\n");
        }
        UTIL_delay(1);
    }
}

void APP_node()
{
    APP_test_systime();

    assert(IP_init(GLOBAL_ROLE_NODE)==IP_INIT_SUCCESS);
    assert(IP_request_ip() == IP_REQUEST_SUCCESS);

    IP_address srcIp;

    char appBuff[2] = {'S',0};
    char* respData;
    int sysPeriod;
    int retries = 0;
    unsigned long long targetTimeStamp;
    unsigned long long last;
    unsigned long long offset;
    int slotNumber;
    int slotCount;

    do
    {
        if(IP_send_data(&gateway, appBuff, 2) == IP_SEND_SUCCESS)
        {
            if(IP_recv_data(&srcIp, &respData, 100) == IP_MSG_APPLICATION)
            {
                sysPeriod = UTIL_convert_4Bytes_to_Int(respData);
                systime = UTIL_convert_8Bytes_to_unsigned_long_long(respData+4);
                break;
            }
        }

        retries++;

    }while(retries < APP_MAX_SYS_REQ_RETRIES);

    assert(retries<APP_MAX_SYS_REQ_RETRIES);

    slotCount = (sysPeriod / APP_DEFAULT_TIMESLOT_IN_MS);

    slotNumber = my_ip.val[3]%slotCount;

    offset = slotNumber * APP_DEFAULT_TIMESLOT_IN_MS;

    //wait until start of next period

    while(!UTIL_within_tolerance(systime%sysPeriod, 5))
    {
        //SLEEP HERE
    }

    while(1)
    {
        targetTimeStamp = offset + systime;
        last = systime;

        //wait until offset

        while(systime - last < targetTimeStamp)
        {
            //SLEEP HERE
        }

        appBuff[0] = 'D';
        appBuff[1] = 123; //WRITE TEMPRETURE DATA HERE

        if(IP_send_data(&gateway, appBuff, 2) == IP_SEND_SUCCESS)
        {
            UART_PRINT("\r\nSENT DATA TO SINK\r\n");
        }

        while(!UTIL_within_tolerance(systime%sysPeriod, 5))
        {
            //SLEEP HERE
        }

    }

}
void APP_sink()
{
    IP_address recvIp;
    char* recvData;
    char sendData[12];

    APP_test_systime();

    assert(IP_init(GLOBAL_ROLE_SINK)==IP_INIT_SUCCESS);

    UART_PRINT("\r\nSTARTED SINK\r\n");

    unsigned long long lastClean = systime;

    while(1)
    {

        if(systime - lastClean > APP_EPOCH_UNTIL_CLEAN)
        {
            UART_PRINT("\r\nDISCARDING ALL IPS WITH TIMESTAMP OLDER THAN %llu\r\n", lastClean);
            IP_CLEAN_UP(lastClean);
            lastClean = systime;
        }

        int res = IP_recv_data(&recvIp, &recvData, 100);

        if(res == IP_MSG_APPLICATION){

            UART_PRINT("\r\nAPPLICATION MESSAGE RECIEVED: %s", recvData[0]);
            UART_PRINT(" FROM SRC IP: ");
            UTIL_UART_print_ip(recvIp);
            UART_PRINT("\r\n");

            if(recvData[0] == 'S')
            {
                UART_PRINT("\r\nSYSTEM PARAMETERS REQUESTED\r\n");

                UTIL_convert_int_to_4bytes(APP_DEFAULT_PERIOD_IN_MS, sendData);
                UTIL_convert_unsigned_long_long_to_8bytes(systime, sendData+4);
                if(IP_send_data(&recvIp, sendData, 12) == IP_SEND_SUCCESS)
                {
                    UART_PRINT("\r\nPARAMETERS SENT, SYSTIME: %llu , SYSPERIOD: %d\r\n", systime, APP_DEFAULT_PERIOD_IN_MS);
                }
            }
            else if(recvData[0] == 'D')
            {
                UART_PRINT("\r\nDATA SENT... LOGGING TEMP: %d AT TIME: %llu\r\n", recvData[1], systime);
            }
            else
            {
                UART_PRINT("\r\nINVALID APP REQUEST RECIEVED... DISCARDING\r\n");
            }
        }
        else if(res == IP_MSG_NO_MSG)
        {
            //UART_PRINT("\r\nAN NO MSG HAS BEEN RECIEVED\r\n");
        }
        else if(res == IP_ISSUE_SUCCESS)
        {
            UART_PRINT("\r\nAN IP REQ WAS MADE AND HAS BEEN SERVICED SUCCESSFULLY\r\n");
        }
        else if(res == IP_ISSUE_FAIL)
        {
            UART_PRINT("\r\nAN IP REQ WAS MADE AND NO IP HAS BEEN ISSUED\r\n");
        }
        else if(res == IP_MSG_INVALID)
        {
            UART_PRINT("\r\nINVALID MESSAGE RECIEVED\r\n");
        }
        else
        {
            UART_PRINT("\r\nUNKNOWN RESPONSE FROM IP RECV DATA\r\n");
        }

        UTIL_delay(1);
    }

}
