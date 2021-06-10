/*
 * translation_utility.h
 *
 *  Created on: May 2, 2019
 *      Author: odysseus
 */


#ifndef TRANSLATION_UTILITY_H_
#define TRANSLATION_UTILITY_H_

#include "offsets.h"
#include <string.h>

#define TABLE_SIZE 256
#define RESOLVE_SUCCESS 1
#define RESOLVE_FAIL 0

typedef struct MAC_address { unsigned char val[MAC_ADDRESS_LEN]; } MAC_address;
typedef struct IP_address { unsigned char val[IP_ADDRESS_LEN]; } IP_address;

extern MAC_address macTable[TABLE_SIZE];

void init_trans_table();

int find_free_ip();

void add_ip_mac_entry(unsigned char ipLast8bit, MAC_address* mac);

void remove_ip_mac_entry(unsigned char ipLast8bit);

int mac_address_comp(MAC_address* a, MAC_address* b);

int resolve_ip_to_mac(IP_address* ip, MAC_address* mac);

int resolve_mac_to_ip(IP_address* ip, MAC_address* mac);

#endif /* TRANSLATION_UTILITY_H_ */
