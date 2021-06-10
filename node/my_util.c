/*
 * my_util.c
 *
 *  Created on: May 4, 2019
 *      Author: USER
 */


#include "my_util.h"

int UTIL_within_tolerance(int val, int tolerance)
{
    return (abs(val) <= tolerance) == 1;
}

void UTIL_convert_unsigned_long_long_to_8bytes(unsigned long long n, char* bytes)
{
    bytes[0] = (n >> 56) & 0xFF;
    bytes[1] = (n >> 48) & 0xFF;
    bytes[2] = (n >> 40) & 0xFF;
    bytes[3] = (n >> 32) & 0xFF;
    bytes[4] = (n >> 24) & 0xFF;
    bytes[5] = (n >> 16) & 0xFF;
    bytes[6] = (n >> 8) & 0xFF;
    bytes[7] = n & 0xFF;
}

void UTIL_convert_int_to_4bytes(int n, char* bytes)
{
    bytes[0] = (n >> 24) & 0xFF;
    bytes[1] = (n >> 16) & 0xFF;
    bytes[2] = (n >> 8) & 0xFF;
    bytes[3] = n & 0xFF;
}

int UTIL_convert_4Bytes_to_Int(char* data)
{
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

}

unsigned long long UTIL_convert_8Bytes_to_unsigned_long_long(char* data)
{
    unsigned long long res;
    unsigned long long d[8];

    d[0] = data[0];
    d[1] = data[1];
    d[2] = data[2];
    d[3] = data[3];
    d[4] = data[4];
    d[5] = data[5];
    d[6] = data[6];
    d[7] = data[7];

    res = d[0]<<56 | d[1]<<48 | d[2]<<40 | d[3]<<32;
    res = res | d[4]<<24 | d[5]<<16 | d[6]<<8 | d[7];
    return res;
}

void UTIL_UART_print_mac(MAC_address _mac)
{
    UART_PRINT("%02x:%02x:%02x:%02x:%02x:%02x", \
               _mac.val[0],_mac.val[1],_mac.val[2],_mac.val[3],_mac.val[4],_mac.val[5]);
}

void UTIL_UART_print_ip(IP_address _ip)
{
    UART_PRINT("%d:%d:%d:%d", \
               _ip.val[0],_ip.val[1],_ip.val[2],_ip.val[3]);
}


void UTIL_delay(int timeInMs){
    unsigned long long last = systime;
    while(systime-last<timeInMs);
}

int UTIL_ip_comp(IP_address* a, IP_address* b)
{
    if ((a->val[0] == b->val[0]) && \
             (a->val[1] == b->val[1]) && \
             (a->val[2] == b->val[2]) && \
             (a->val[3] == b->val[3]))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void UTIL_clear_payload(char* _payload, int payloadSize)
{
    int i;
    for(i = 0; i<payloadSize; i++)
    {
        _payload[i] = 0;
    }
}
