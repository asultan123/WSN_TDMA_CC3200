/*
 * application.h
 *
 *  Created on: May 2, 2019
 *      Author: odysseus
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "ip.h"
#include "rom_map.h"
#include "utils.h"

#define APP_MAX_SYS_REQ_RETRIES 10
#define APP_DEFAULT_PERIOD_IN_MS 10000
#define APP_DEFAULT_TIMESLOT_IN_MS 1000
#define APP_EPOCH_UNTIL_CLEAN 2*APP_DEFAULT_PERIOD_IN_MS

void APP_test_systime();
void APP_mac_layer_test_sender();
void APP_mac_layer_test_reciever();
void APP_ip_layer_test_sender();
void APP_ip_layer_test_reciever();
void APP_ip_layer_test_reciever_dhcp();
void APP_node();
void APP_sink();

#endif /* APPLICATION_H_ */
