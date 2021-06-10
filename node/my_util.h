/*
 * my_util.h
 *
 *  Created on: May 4, 2019
 *      Author: USER
 */

#ifndef MY_UTIL_H_
#define MY_UTIL_H_

#include "globals.h"
#include "translation_utility.h"
#include "common.h"
#include "uart_if.h"

void UTIL_convert_unsigned_long_long_to_8bytes(unsigned long long n, char* bytes);
void UTIL_convert_int_to_4bytes(int n, char* bytes);
int UTIL_convert_4Bytes_to_Int(char* data);
int UTIL_within_tolerance(int val, int tolerance);
unsigned long long UTIL_convert_8Bytes_to_unsigned_long_long(char* data);
void UTIL_UART_print_mac(MAC_address _mac);
void UTIL_UART_print_ip(IP_address _mac);
void UTIL_delay(int timeInMs);
void UTIL_clear_payload(char* _payload, int payloadSize);
int UTIL_ip_comp(IP_address* a, IP_address* b);

#endif /* MY_UTIL_H_ */
