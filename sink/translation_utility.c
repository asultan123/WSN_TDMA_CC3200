/*
 * translation_utility.c
 *
 *  Created on: May 3, 2019
 *      Author: USER
 */

#include "translation_utility.h"

IP_address ipTable[TABLE_SIZE];
MAC_address macTable[TABLE_SIZE];

void init_trans_table()
{
    int i,j;
    for(i = 0; i<TABLE_SIZE; i++)
    {
        for(j = 0; j<MAC_ADDRESS_LEN; j++)
        {
            macTable[i].val[j] = 0;
        }
    }

    MAC_address broadcastMac;


    for(j = 0; j<MAC_ADDRESS_LEN; j++)
    {
        broadcastMac.val[j] = 0xff;
    }

    add_ip_mac_entry(255, &broadcastMac);

}

int mac_address_comp(MAC_address* a, MAC_address* b)
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

int find_free_ip()
{
    MAC_address nullMac = {{0x00,0x00,0x00,0x00}};
    int i = 2;
    while(i<TABLE_SIZE-1)
    {
        if(mac_address_comp(&nullMac, &macTable[i]))
        {
            return i;
        }
        i++;
    }
    return -1;
}

void add_ip_mac_entry(unsigned char ipLast8Bit, MAC_address* mac)
{
    int j;
    for(j = 0; j<MAC_ADDRESS_LEN; j++)
    {
        macTable[ipLast8Bit].val[j] = mac->val[j];
    }
}

void remove_ip_mac_entry(unsigned char ipLast8bit)
{
    int j;
    for(j = 0; j<MAC_ADDRESS_LEN; j++)
    {
        macTable[ipLast8bit].val[j] = 0x00;
    }
}

int resolve_ip_to_mac(IP_address* ip, MAC_address* mac)
{
    MAC_address nullMac = {{0,0,0,0}};
    if(mac_address_comp(&nullMac, &macTable[ip->val[3]]) == 0)
    {
        mac->val[0] = macTable[ip->val[3]].val[0];
        mac->val[1] = macTable[ip->val[3]].val[1];
        mac->val[2] = macTable[ip->val[3]].val[2];
        mac->val[3] = macTable[ip->val[3]].val[3];
        mac->val[4] = macTable[ip->val[3]].val[4];
        mac->val[5] = macTable[ip->val[3]].val[5];
        return RESOLVE_SUCCESS;
    }
    else
    {
        return RESOLVE_FAIL;
    }
}

int resolve_mac_to_ip(IP_address* ip, MAC_address* mac)
{
    unsigned char i;
    for(i = 1; i<TABLE_SIZE-1; i++)
    {
        if(mac_address_comp(mac, &macTable[i]))
        {
            ip->val[0] = 0xff;
            ip->val[1] = 0xff;
            ip->val[2] = 0xff;
            ip->val[3] = i;
            return RESOLVE_SUCCESS;
        }
    }
    return RESOLVE_FAIL;
}
