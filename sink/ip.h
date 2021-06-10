/*
 * ip.h
 *
 *  Created on: May 2, 2019
 *      Author: odysseus
 */


#ifndef IP_H_
#define IP_H_

#include "translation_utility.h"
#include "offsets.h"
#include "my_util.h"
#include "mac.h"
#include "globals.h"

#define IP_MAX_COUNT 254
#define IP_MAX_REQUEST_RETRIES 10
#define IP_MAX_ISSUE_RETRIES IP_MAX_REQUEST_RETRIES
#define IP_MAX_REQUEST_IP_TIMEOUT 50
#define IP_REQUEST_SUCCESS 1
#define IP_REQUEST_FAIL 0
#define IP_ISSUE_SUCCESS 1
#define IP_ISSUE_FAIL 0
#define IP_INIT_SUCCESS 1
#define IP_INIT_FAIL 0
#define IP_SEND_SUCCESS 1
#define IP_SEND_FAIL 0
#define IP_MSG_IP_REQUEST 1
#define IP_MSG_APPLICATION 2
#define IP_MSG_INVALID 3
#define IP_MSG_NO_MSG 4

extern char IP_sendPayload[IP_SEND_PAYLOAD_SIZE];
extern char IP_recvPayload[IP_RECV_PAYLOAD_SIZE];

extern IP_address my_ip;
extern IP_address gateway;
extern int role;

unsigned long long IP_last_talked_to[IP_MAX_COUNT];

int IP_init(int role);

int IP_is_request_msg(char* data);

int IP_issue_ip(MAC_address* dstMac);

int IP_request_ip();

void IP_add_header_to_send_payload(IP_address* destIp);

void IP_add_data_to_send_payload(char* data, int dataSize);

//void IP_parse_recv_payload(IP_address* srcIp, IP_address* destIp, char** dataPointer);

void IP_parse_mac_response(char* respData, IP_address* srcIp, IP_address* destIp, char** dataStart);

int IP_send_data(IP_address* dstIp, char* data, int dataSize);

int IP_recv_data(IP_address* srcIp, char** data, int timeout);

void IP_CLEAN_UP(unsigned long long cutOffTimeStampe);


#endif /* IP_H_ */
