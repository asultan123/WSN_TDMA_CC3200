/*
 * mac.h
 *
 *  Created on: May 2, 2019
 *      Author: odysseus
 */


#ifndef MAC_H_
#define MAC_H_

#include "translation_utility.h"
#include "offsets.h"
#include "netcfg.h"
#include "socket.h"
#include "common.h"
#include "globals.h"
#include <assert.h>
#include "uart_if.h"
#include "my_util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PREAMBLE 1
#define MAC_MAX_RETRIES 10
#define MAC_CHANNEL 1
#define MAC_PWR 10
#define MAC_RATE 2
#define MAC_DEFAULT_TIMEOUT 1
#define MAC_ACK_DEFAULT_WAIT_TIME 50
#define MAC_ACK_NOT_RECIEVED 0
#define MAC_ACK_RECIEVED 1
#define MAC_MSG_BROADCAST 1
#define MAC_MSG_ME 2
#define MAC_MSG_NONE 3
#define MAC_SEND_SUCCESS 1
#define MAC_SEND_FAIL 0
#define MAC_INIT_SUCCESS 1
#define MAC_INIT_FAIL 0
#define MAC_IGNORE_BROADCAST 0
#define MAC_ACCEPT_BROADCAST 1

extern char sendPayload[MAC_SEND_PAYLOAD_SIZE];
extern char recvPayload[MAC_RECV_PAYLOAD_SIZE];

extern MAC_address my_mac;
extern MAC_address broadcast_mac;

/* Description:
 *
 * Opens raw socket and sets options
 * Sets MAC_sd to opened socket
 * Clears IP to MAC translation table
 * Initiates my_mac
 *
 * Return:
 *
 * MAC_INIT_SUCCESS on successful socket open
 * MAC_INIT_FAIL on fail
 * */

int MAC_init();

int MAC_is_broadcast_mac(MAC_address* _mac);

int MAC_is_my_mac(MAC_address* _mac);

int MAC_sd_open();

void MAC_sd_close();

int MAC_mac_address_equal(MAC_address* a, MAC_address* b);

void MAC_parse_header(MAC_address* src, MAC_address* dest, char** dataStart);

void MAC_add_header_to_send_payload(MAC_address* destMac);

void MAC_add_data_to_send_payload(char* data, int dataSize);

//void MAC_clear_payload(char* _payload, int payloadSize);

int MAC_recv_payload_using_socket(MAC_address* dest, char** dataStart);

int MAC_send_payload_using_socket();

int MAC_recv_ack(MAC_address* expectedDstMac);

int MAC_send_ack(MAC_address* dstMac);

int MAC_is_ack(char* data);

/* Description:
 *
 * Creates MAC layer header
 * Sends packet using socket descriptor
 * Waits for ACK, if not received will try again for max retries
 *
 * Return:
 *
 * MAC_SEND_SUCCESS
 * MAC_SEND_FAIL
 * */

int MAC_send_data(MAC_address* destMac, char* data, int dataSize);

/* Description:
 *
 * Waits for data for duration equal to timeout
 * If packet recieved, parses header and checks destination
 * If destination is this node or broadcast will send back an ack
 * Resolves mac to ip
 *
 * If ignoreBroadCast is 1 then a broadcast message will be treated as NOT_ME
 *
 * Return:
 *
 * MAC_MSG_BROADCAST
 * MAC_MSG_ME
 * MAC_MSG_NONE
 * MAC_MSG_NOT_ME
*/

int MAC_recv_data(MAC_address* srcMAC, char** data, int timeout, int ignoreBroadcast);

#endif /* MAC_H_ */
