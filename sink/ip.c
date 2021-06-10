/*
 * ip.c
 *
 *  Created on: May 4, 2019
 *      Author: USER
 */

#include "ip.h"


char IP_sendPayload[IP_SEND_PAYLOAD_SIZE];
char IP_recvPayload[IP_RECV_PAYLOAD_SIZE];

IP_address my_ip;
IP_address gateway;
int role;

unsigned long long IP_last_talked_to[IP_MAX_COUNT];

int IP_init(int _role)
{
    role = _role;

    int i;
    if(MAC_init() == MAC_INIT_FAIL)
    {
        return IP_INIT_FAIL;
    }
    for(i = 0; i<IP_MAX_COUNT; i++)
    {
        IP_last_talked_to[i] = 0;
    }
    if(role == GLOBAL_ROLE_SINK)
    {
        my_ip.val[0] = 0xff;
        my_ip.val[1] = 0xff;
        my_ip.val[2] = 0xff;
        my_ip.val[3] = 0x01;
        gateway.val[0] = 0xff;
        gateway.val[1] = 0xff;
        gateway.val[2] = 0xff;
        gateway.val[3] = 0x01;
    }
    return IP_INIT_SUCCESS;
}

int IP_is_request_msg(char* data)
{
    char requestString[8] = {'i','s','_','s','i','n','k', 0};
    return strcmp(data, requestString) == 0;
}

int IP_issue_ip(MAC_address* dstMac)
{
    int last8bitOfFreeIp = find_free_ip();

    if(last8bitOfFreeIp == -1)
    {
        return IP_ISSUE_FAIL;
    }

    IP_address assignedIp ={{0xff,0xff,0xff,(unsigned char)last8bitOfFreeIp}};
    UTIL_clear_payload(IP_sendPayload, IP_SEND_PAYLOAD_SIZE);
    IP_add_header_to_send_payload(&assignedIp);
    int retries = 0;
    while(retries < IP_MAX_ISSUE_RETRIES && MAC_send_data(dstMac, IP_sendPayload, IP_SEND_PAYLOAD_SIZE) == MAC_SEND_FAIL)
    {
        retries++;
        UTIL_delay(1);
    }
    if(retries < IP_MAX_REQUEST_IP_TIMEOUT)
    {
        add_ip_mac_entry(assignedIp.val[3], dstMac);
        IP_last_talked_to[assignedIp.val[3]] = systime;

        return IP_ISSUE_SUCCESS;
    }
    else
    {
        return IP_ISSUE_FAIL;
    }
}

int IP_request_ip()
{

    MAC_address gateway_mac;
    char requestString[8] = {'i','s','_','s','i','n','k', 0};
    char* respData;

#if GLOBAL_VERBOSE == 1
    UART_PRINT("\r\nREQUESTING IP FROM CLOSEST NEARBY SINK\r\n");
#endif
    int retries = 0;

    while(retries < IP_MAX_REQUEST_RETRIES)
    {
        if(MAC_send_data(&broadcast_mac, requestString, 8) == MAC_SEND_SUCCESS)
        {
            if(MAC_recv_data(&gateway_mac, &respData, IP_MAX_REQUEST_IP_TIMEOUT, MAC_IGNORE_BROADCAST) == MAC_MSG_ME)
            {
                #if GLOBAL_VERBOSE == 1
                    UART_PRINT("\r\nACQUIRED IP FROM CLOSEST NEARBY SINK\r\n");
                #endif
                break;
            }
        }

        UTIL_delay(1);
        retries++;
    };

    if(retries < IP_MAX_REQUEST_RETRIES)
    {
        char* dataStart;
        IP_parse_mac_response(respData, &gateway,  &my_ip, &dataStart);

        UART_PRINT("\r\nACQUIRED IP: ");
        UTIL_UART_print_ip(my_ip);
        UART_PRINT("\r\n");

        add_ip_mac_entry(gateway.val[3], &gateway_mac);
        return IP_REQUEST_SUCCESS;
    }
    else
    {
        return IP_REQUEST_FAIL;
    }
}

void IP_add_header_to_send_payload(IP_address* destIp)
{
    int i, idx;

    idx = 0;
    for(i = IP_ADDRESS_SRC_OFFSET; i<IP_ADDRESS_DST_OFFSET; i++)
    {
        IP_sendPayload[i] = my_ip.val[idx++];
    }

    idx = 0;
    for(i = IP_ADDRESS_DST_OFFSET; i<IP_ADDRESS_HEADER_OFFSET; i++)
    {
        IP_sendPayload[i] = destIp->val[idx++];
    }
}

void IP_add_data_to_send_payload(char* data, int dataSize)
{
    assert(dataSize + IP_HEADER_SIZE < IP_SEND_PAYLOAD_SIZE);
    int i, idx;
    idx = 0;
    for(i = IP_ADDRESS_HEADER_OFFSET; i<IP_ADDRESS_HEADER_OFFSET + dataSize; i++)
    {
        IP_sendPayload[i] = data[idx++];
    }
}

//void IP_parse_recv_payload(IP_address* srcIp, IP_address* destIp, char** dataStart)
//{
//    int i, idx;
//
//    idx = 0;
//    for(i = IP_ADDRESS_SRC_OFFSET; i < IP_ADDRESS_DST_OFFSET; i++)
//    {
//        srcIp->val[idx++] = IP_recvPayload[i];
//    }
//
//    idx = 0;
//    for(i = IP_ADDRESS_DST_OFFSET; i < IP_ADDRESS_HEADER_OFFSET; i++)
//    {
//        destIp->val[idx++] = IP_recvPayload[i];
//    }
//
//
//}

void IP_parse_mac_response(char* respData, IP_address* srcIp, IP_address* destIp, char** dataStart)
{
    int i, idx;

    idx = 0;
    for(i = IP_ADDRESS_SRC_OFFSET; i < IP_ADDRESS_DST_OFFSET; i++)
    {
        srcIp->val[idx++] = respData[i];
    }

    idx = 0;
    for(i = IP_ADDRESS_DST_OFFSET; i < IP_ADDRESS_HEADER_OFFSET; i++)
    {
        destIp->val[idx++] = respData[i];
    }

    *dataStart = respData + IP_ADDRESS_HEADER_OFFSET;
}

int IP_send_data(IP_address* dstIp, char* data, int dataSize)
{
    UTIL_clear_payload(IP_sendPayload, IP_SEND_PAYLOAD_SIZE);
    IP_add_header_to_send_payload(dstIp);
    IP_add_data_to_send_payload(data, dataSize);
    MAC_address resolvedMac;
    if(resolve_ip_to_mac(dstIp,&resolvedMac) == RESOLVE_SUCCESS)
    {
        if(MAC_send_data(&resolvedMac, IP_sendPayload, IP_SEND_PAYLOAD_SIZE) == MAC_SEND_SUCCESS)
        {
            return IP_SEND_SUCCESS;
        }
        else
        {
            return IP_SEND_FAIL;
        }
    }

#if GLOBAL_VERBOSE == 1
    UART_PRINT("\r\nCOULD NOT FIND ENTRY IN TRANSLATION TABLE FOR REQUESTED IP\r\n");
#endif

    return IP_SEND_FAIL;
}

int IP_recv_data(IP_address* srcIp, char** data, int timeout)
{
    MAC_address srcMac;
    IP_address dstIp;
    IP_address resolvedIp;
    MAC_address resolvedMac;
    char* dataFromMac;
    int recvRes;
    int resolveRes;
    int resolveResMac;

    if(role == GLOBAL_ROLE_SINK)
    {
        recvRes = MAC_recv_data(&srcMac, &dataFromMac, timeout, MAC_ACCEPT_BROADCAST);
        resolveRes = resolve_mac_to_ip(&resolvedIp, &srcMac);

        if(recvRes == MAC_MSG_BROADCAST)
        {
            if(IP_is_request_msg(dataFromMac))
            {
                #if GLOBAL_VERBOSE == 1
                UART_PRINT("\r\nIP REQUEST DETECTED... ISSUING\r\n");
                #endif

                return IP_issue_ip(&srcMac);
            }
        }
        else if(recvRes == MAC_MSG_ME )
        {
            if(resolveRes == RESOLVE_SUCCESS) //was sender a previous active / is sender currently active
            {
                IP_parse_mac_response(dataFromMac,srcIp,&dstIp, data);
                resolveResMac = resolve_ip_to_mac(srcIp, &resolvedMac);
                if(resolveResMac == RESOLVE_SUCCESS && MAC_mac_address_equal(&resolvedMac, &srcMac) && UTIL_ip_comp(&dstIp, &my_ip))
                {
                    #if GLOBAL_VERBOSE == 1
                    UART_PRINT("\r\nAPP MSG DETECTING... FORWARDING TO APP LAYER AND UPDATING LAST TALKED TO\r\n");
                    #endif

                    IP_last_talked_to[srcIp->val[3]] = systime;
                    return IP_MSG_APPLICATION;
                }
                else
                {
                    #if GLOBAL_VERBOSE == 1
                    UART_PRINT("\r\nINVALID MESSAGE RECIEVED, SENDER KNOWS SINK BUT SINK HAS MISMATCH MAC ADDRESS FOR SENDER IN TRANS TABLE... IGNORING\r\n");
                    #endif
                    return IP_MSG_INVALID;
                }
            }
        }
        else
        {
            #if GLOBAL_VERBOSE == 1
            //UART_PRINT("\r\nNO MESSAGE RECIEVED\r\n");
            #endif

            return IP_MSG_NO_MSG;
        }

    }
    else if(role == GLOBAL_ROLE_NODE)
    {
        recvRes = MAC_recv_data(&srcMac, &dataFromMac, timeout, MAC_IGNORE_BROADCAST);
        resolveRes = resolve_mac_to_ip(&resolvedIp, &srcMac);

        if(recvRes == MAC_MSG_ME)
        {
            if(resolveRes == RESOLVE_SUCCESS)
            {
                if(UTIL_ip_comp(&gateway, &resolvedIp))
                {
                    IP_parse_mac_response(dataFromMac,srcIp,&dstIp, data);

                    if(UTIL_ip_comp(&dstIp, &my_ip))
                    {
                        if(UTIL_ip_comp(srcIp, &resolvedIp))
                        {
                            return IP_MSG_APPLICATION;
                        }
                        else
                        {
                            #if GLOBAL_VERBOSE == 1
                            UART_PRINT("\r\nINVALID MESSAGE RECIEVED, SENDER SRC IP IS WRONG (NOT GATEWAY), POSSIBLE CORRUPTION IN EITEHR GATEWAY IP OR TRANS TABLE... IGNORING\r\n");
                            #endif
                            return IP_MSG_NO_MSG;
                        }
                    }
                    else
                    {
                        #if GLOBAL_VERBOSE == 1
                        UART_PRINT("\r\nINVALID MESSAGE RECIEVED, SENDER DEST MAC IS CORRECT BUT SENDER DEST IP IS WRONG, POSSIBLE CORRUPTION IN SENDER TRANS TABLE... IGNORING\r\n");
                        #endif
                        return IP_MSG_NO_MSG;
                    }
                }
                else
                {
                    #if GLOBAL_VERBOSE == 1
                    UART_PRINT("\r\nINVALID MESSAGE RECIEVED, SENDER NOT GATEWAY... IGNORING\r\n");
                    #endif
                    return IP_MSG_NO_MSG;
                }
            }
            else
            {
                #if GLOBAL_VERBOSE == 1
                UART_PRINT("\r\nINVALID MESSAGE RECIEVED, SENDER KNOWS NODE BUT NODE HAS NOT ENTRY FOR SENDER IN TRANS TABLE... IGNORING\r\n");
                #endif
                return IP_MSG_NO_MSG;
            }
        }
        else
        {

            #if GLOBAL_VERBOSE == 1
            UART_PRINT("\r\nNO MESSAGE RECIEVED\r\n");
            #endif

            return IP_MSG_NO_MSG;
        }
    }
    else
    {

        #if GLOBAL_VERBOSE == 1
        UART_PRINT("\r\nINVALID ROLE SPECIFIED, ABORTING...\r\n");
        #endif

        assert(0>1);
        return IP_MSG_INVALID;
    }
    return IP_MSG_INVALID;
}

void IP_CLEAN_UP(unsigned long long cutOffTimeStamp)
{
    int i;
    IP_address ipToRemove = {{0xff,0xff,0xff,0}};
    for(i = 1; i<TABLE_SIZE-1; i++)
    {
        if(IP_last_talked_to[i] < cutOffTimeStamp && IP_last_talked_to[i] !=0 )
        {
            ipToRemove.val[3] = i;
            UART_PRINT("\r\n");
            UTIL_UART_print_ip(ipToRemove);
            UART_PRINT("\r\n");
            IP_last_talked_to[i] = 0;
            remove_ip_mac_entry((unsigned char)i);
        }
    }
}

//
//#if GLOBAL_VERBOSE == 1
//#endif
//
//                if(UTIL_ip_comp(&dstIp, &my_ip) && UTIL_ip_comp(srcIp, &resolvedIp))
//                {
//                    #if GLOBAL_VERBOSE == 1
//                    UART_PRINT("\r\nAPP MSG DETECTING... FORWARDING TO APP LAYER AND UPDATING LAST TALKED TO\r\n");
//                    #endif
//
//                    IP_last_talked_to[srcIp->val[3]] = systime;
//                    return IP_MSG_APPLICATION;
//                }
