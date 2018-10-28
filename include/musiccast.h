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

typedef enum {
	MC_POWER_STATE_ON,
	MC_POWER_STATE_STANDBY,
	MC_POWER_STATE_TOGGLE
} musiccast_power_state_e;

typedef enum {
	MC_ZONE_VOLUME_SET_UP,
	MC_ZONE_VOLUME_SET_DOWN,
	MC_ZONE_VOLUME_SET_DIRECT,
} musiccast_zone_volume_action_e;

musiccast_conn_t * musiccast_init(const char *hostname);
void musiccast_destroy(musiccast_conn_t *mcc);

/* retrieve system info */
bool musiccast_system_device_info(musiccast_conn_t *mc);
bool musiccast_system_features(musiccast_conn_t *mc);
bool musiccast_system_network_status(musiccast_conn_t *mc);
bool musiccast_system_func_status(musiccast_conn_t *mc);
bool musiccast_system_location_info(musiccast_conn_t *mc);
bool musiccast_system_sound_program_list(musiccast_conn_t *mc);

/* zone */
bool musiccast_zone_power_set(musiccast_conn_t *mc, const char *zone, musiccast_power_state_e state);
bool musiccast_zone_status(musiccast_conn_t *mc, const char *zone);
bool musiccast_zone_volume_set(musiccast_conn_t *mc, const char *zone, musiccast_zone_volume_action_e action, int volume_step);
bool musiccast_zone_mute(musiccast_conn_t *mc, const char *zone, bool mute);

#endif /* INCLUDE_MUSICCAST_H_ */
