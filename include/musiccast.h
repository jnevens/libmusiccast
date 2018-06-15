/*
 * musiccast.h
 *
 *  Created on: Feb 17, 2015
 *      Author: jnevens
 */

#ifndef INCLUDE_MUSICCAST_H_
#define INCLUDE_MUSICCAST_H_

#include <stdbool.h>

typedef struct musiccast_conn_s musiccast_conn_t;

musiccast_conn_t * musiccast_init(const char *hostname);
void musiccast_destroy(musiccast_conn_t *mcc);


/* retrieve system info */
bool musiccast_system_device_info(musiccast_conn_t *mc);
bool musiccast_system_features(musiccast_conn_t *mc);
bool musiccast_system_network_status(musiccast_conn_t *mc);
bool musiccast_system_func_status(musiccast_conn_t *mc);
bool musiccast_system_location_info(musiccast_conn_t *mc);
bool musiccast_system_status(musiccast_conn_t *mc);
bool musiccast_system_sound_program_list(musiccast_conn_t *mc);

#endif /* INCLUDE_MUSICCAST_H_ */
