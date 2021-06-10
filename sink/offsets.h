/*
 * offsets.h
 *
 *  Created on: May 2, 2019
 *      Author: odysseus
 */

#ifndef OFFSETS_H_
#define OFFSETS_H_

#define MAC_ADDRESS_LEN             6
#define MAC_SEND_PAYLOAD_SIZE       1200
#define MAC_RECV_PAYLOAD_SIZE       1500
#define MAC_HEADER_SIZE             2 * MAC_ADDRESS_LEN;
#define MAC_INFO_OFFSET             8
#define MAC_ADDRESS_DEST_OFFSET     MAC_INFO_OFFSET
#define MAC_ADDRESS_SRC_OFFSET      MAC_ADDRESS_DEST_OFFSET   + MAC_ADDRESS_LEN
#define MAC_HEADER_OFFSET           MAC_ADDRESS_SRC_OFFSET   + MAC_ADDRESS_LEN

#define IP_ADDRESS_LEN              4
#define IP_SEND_PAYLOAD_SIZE        900
#define IP_RECV_PAYLOAD_SIZE        1000
#define IP_ADDRESS_TOTAL_LEN_OFFSET 2
#define IP_ADDRESS_PROTOCOL_OFFSET  9
#define IP_ADDRESS_CRC_OFFSET       10
#define IP_ADDRESS_SRC_OFFSET       12
#define IP_ADDRESS_DST_OFFSET       IP_ADDRESS_SRC_OFFSET + IP_ADDRESS_LEN
#define IP_ADDRESS_HEADER_OFFSET    IP_ADDRESS_DST_OFFSET + IP_ADDRESS_LEN
#define IP_HEADER_SIZE              20

#endif /* OFFSETS_H_ */
