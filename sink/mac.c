/*
 * mac.c
 *
 *  Created on: May 3, 2019
 *      Author: USER
 */


#include "mac.h"

char sendPayload[MAC_SEND_PAYLOAD_SIZE];
char recvPayload[MAC_RECV_PAYLOAD_SIZE];

MAC_address my_mac;
MAC_address broadcast_mac;

int MAC_sd = -1;



int MAC_init(){

    _u8 local_macAddressLen = SL_MAC_ADDR_LEN;
    _u8 local_my_mac[MAC_ADDRESS_LEN];
    int i;

    if(MAC_sd_open()<0)
    {
        return MAC_INIT_FAIL;
    }

#if GLOBAL_VERBOSE == 1
    UART_PRINT("\r\nMAC LAYER HAS OPENED A SOCKET\r\n");
#endif

    init_trans_table();

#if GLOBAL_VERBOSE == 1
    UART_PRINT("\r\nTRANSLATION TABLE HAS BEEN RESET\r\n");
#endif

    sl_NetCfgGet(SL_MAC_ADDRESS_GET,NULL,&local_macAddressLen,local_my_mac);

    for(i = 0; i<MAC_ADDRESS_LEN; i++)
    {
        my_mac.val[i] = local_my_mac[i];
        broadcast_mac.val[i] = 0xff;
    }

#if GLOBAL_VERBOSE == 1
    UART_PRINT("\r\nSET BROADCAST MAC AND ACQUIRED MY MAC ADDRESS\r\n");
    UTIL_UART_print_mac(my_mac);
    UART_PRINT("\r\n");
#endif

    return MAC_INIT_SUCCESS;
}

int MAC_sd_open()
{
    _u8 iChannel = MAC_CHANNEL;
    SlSuseconds_t timeOut = MAC_DEFAULT_TIMEOUT;
    MAC_sd = sl_Socket(SL_AF_RF,SL_SOCK_RAW,iChannel);
    long lRetVal = -1;

    assert(MAC_sd > 0);

    struct SlTimeval_t timeval;

    timeval.tv_sec  =  0;             // Seconds
    timeval.tv_usec = timeOut;         // Microseconds.

    // Enable receive timeout
    lRetVal = sl_SetSockOpt(MAC_sd,SL_SOL_SOCKET,SL_SO_RCVTIMEO, &timeval, \
                                  sizeof(timeval));

    ASSERT_ON_ERROR(lRetVal);

    return MAC_sd;
}

int MAC_is_ack(char* data)
{
    char ack[4] = {'a','c','k',0};
    return strcmp(data, ack) == 0;
}

int MAC_is_my_mac(MAC_address* _mac)
{
    return MAC_mac_address_equal(_mac, &my_mac);
}

int MAC_is_broadcast_mac(MAC_address* _mac)
{
    return MAC_mac_address_equal(_mac, &broadcast_mac);
}

int MAC_mac_address_equal(MAC_address* a, MAC_address* b)
{
    if ((a->val[0] == b->val[0]) && \
             (a->val[1] == b->val[1]) && \
             (a->val[2] == b->val[2]) && \
             (a->val[3] == b->val[3]) && \
             (a->val[4] == b->val[4]) && \
             (a->val[5] == b->val[5]))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int MAC_send_ack(MAC_address* destMac)
{
    char ack[3] = {'a','c','k'};

    UTIL_clear_payload(sendPayload, MAC_SEND_PAYLOAD_SIZE);
    MAC_add_header_to_send_payload(destMac);
    MAC_add_data_to_send_payload(ack, 3);

    int retries = 0;
    while(MAC_send_payload_using_socket()<0 && retries < MAC_MAX_RETRIES)
    {
        UTIL_delay(1);
        retries++;
    }

    return retries < MAC_MAX_RETRIES;
}

int MAC_recv_ack(MAC_address* expectedSrcMac)
{
    MAC_address srcMacFromPayload;
    char* dataFromPayload;

    unsigned long long last = systime;

    while(systime - last < MAC_ACK_DEFAULT_WAIT_TIME)
    {

        int res = MAC_recv_payload_using_socket(&srcMacFromPayload, &dataFromPayload);

        if(res == MAC_MSG_ME)
        {
            if(MAC_is_ack(dataFromPayload))
            {
                if(MAC_mac_address_equal(&broadcast_mac, expectedSrcMac))
                {
                    return MAC_ACK_RECIEVED;
                }
                else if(MAC_mac_address_equal(&srcMacFromPayload, expectedSrcMac))
                {
                    return MAC_ACK_RECIEVED;
                }
            }
        }
        else
        {
            UTIL_delay(5);
        }
    }
    return MAC_ACK_NOT_RECIEVED;
}

void MAC_parse_header(MAC_address *dst, MAC_address *src, char **dataStart)
{
    int i, idx;

    idx = 0;
    for(i = MAC_ADDRESS_DEST_OFFSET; i<MAC_ADDRESS_SRC_OFFSET; i++)
    {
        dst->val[idx++] = recvPayload[i];
    }

    idx = 0;
    for(i = MAC_ADDRESS_SRC_OFFSET; i<MAC_HEADER_OFFSET; i++)
    {
        src->val[idx++] = recvPayload[i];
    }

    *dataStart = recvPayload + MAC_HEADER_OFFSET;
}

//void UTIL_clear_payload(char* _payload, int payloadSize)
//{
//    int i;
//    for(i = 0; i<payloadSize; i++)
//    {
//        _payload[i] = 0;
//    }
//}

void MAC_add_header_to_send_payload(MAC_address* destMac)
{
    int i, idx;

    idx = 0;
    for(i = 0; i<MAC_ADDRESS_LEN; i++)
    {
        sendPayload[i] = (*destMac).val[idx++];
    }

    idx = 0;
    for(i = MAC_ADDRESS_LEN; i <  2 * MAC_ADDRESS_LEN; i++)
    {
        sendPayload[i] = my_mac.val[idx++];
    }

}

void MAC_add_data_to_send_payload(char* data, int dataSize)
{
    int i, idx;

    idx = 0;
    for(i = 2 * MAC_ADDRESS_LEN; i < (2 * MAC_ADDRESS_LEN) + dataSize ; i++)
    {
        sendPayload[i] = data[idx++];
    }
}

int MAC_recv_payload_using_socket(MAC_address* srcMacFromPayload, char** dataFromPayload)
{
    long lRetVal = -1;
    MAC_address dstMacFromPayload;

    UTIL_clear_payload(recvPayload, MAC_RECV_PAYLOAD_SIZE);

    lRetVal = sl_Recv(MAC_sd,recvPayload,MAC_RECV_PAYLOAD_SIZE,0);

    if(lRetVal < 0 && lRetVal != SL_EAGAIN)
    {
        //error
        ASSERT_ON_ERROR(sl_Close(MAC_sd));
        ASSERT_ON_ERROR(lRetVal);
        LOOP_FOREVER();
    }
    else
    {

        MAC_parse_header(&dstMacFromPayload, srcMacFromPayload, dataFromPayload);

        if(MAC_is_my_mac(&dstMacFromPayload))/// & isMyIP(dest_ip))
        {
            return MAC_MSG_ME;
        }
        else if(MAC_is_broadcast_mac(&dstMacFromPayload))
        {
            return MAC_MSG_BROADCAST;
        }
        else
        {
            return MAC_MSG_NONE;
        }
    }
}

int MAC_send_payload_using_socket(){

    int lRetVal = -1;

    lRetVal = sl_Send(MAC_sd,sendPayload,MAC_SEND_PAYLOAD_SIZE,\
                       SL_RAW_RF_TX_PARAMS(MAC_CHANNEL, MAC_RATE, MAC_PWR, PREAMBLE));
    if(lRetVal < 0)
    {
        ASSERT_ON_ERROR(lRetVal);
        return -1;
    }

    return lRetVal;
}

int MAC_send_data(MAC_address* destMac, char* data, int dataSize)
{

    assert(MAC_HEADER_OFFSET+dataSize < MAC_SEND_PAYLOAD_SIZE);

    UTIL_clear_payload(sendPayload, MAC_SEND_PAYLOAD_SIZE);
    MAC_add_header_to_send_payload(destMac);
    MAC_add_data_to_send_payload(data, dataSize);

    int retries = 0;
    do{
        MAC_send_payload_using_socket();
        retries++;
    }while(retries<MAC_MAX_RETRIES && (MAC_recv_ack(destMac) == MAC_ACK_NOT_RECIEVED));

    if(retries>=MAC_MAX_RETRIES)
    {
        return MAC_SEND_FAIL;
    }
    else
    {
        return MAC_SEND_SUCCESS;
    }

}

int MAC_recv_data(MAC_address* srcMac, char** data, int timeout, int ignoreBroadcast)
{
    unsigned long long last = systime;
    while(systime - last < timeout)
    {
        int res = MAC_recv_payload_using_socket(srcMac, data);
        if(res == MAC_MSG_ME || (res == MAC_MSG_BROADCAST && ignoreBroadcast == MAC_ACCEPT_BROADCAST))
        {
            MAC_send_ack(srcMac);
            return res;
        }
    }
    return MAC_MSG_NONE;
}


