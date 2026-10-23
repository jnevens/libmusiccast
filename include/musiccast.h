/*
 * musiccast.h
 *
 *  Created on: Feb 17, 2015
 *      Author: jnevens
 */

#ifndef INCLUDE_MUSICCAST_H_
#define INCLUDE_MUSICCAST_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct musiccast_conn_s musiccast_conn_t;

typedef struct musiccast_discovered_device_s {
	char *address;
	char *hostname;
	char *model_name;
	char *device_id;
} musiccast_discovered_device_t;

typedef struct musiccast_discover_list_s {
	musiccast_discovered_device_t *devices;
	size_t count;
} musiccast_discover_list_t;

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

/* LAN discovery (SSDP); timeout_ms 0 uses a default of 3 seconds */
musiccast_discover_list_t *musiccast_discover(unsigned int timeout_ms);
void musiccast_discover_list_free(musiccast_discover_list_t *list);

/* retrieve system info */
char *musiccast_system_device_info(musiccast_conn_t *mc);
char *musiccast_system_features(musiccast_conn_t *mc);
char *musiccast_system_network_status(musiccast_conn_t *mc);
char *musiccast_system_func_status(musiccast_conn_t *mc);
char *musiccast_system_location_info(musiccast_conn_t *mc);
char *musiccast_system_sound_program_list(musiccast_conn_t *mc);

/* zone */
bool musiccast_zone_power_set(musiccast_conn_t *mc, const char *zone, musiccast_power_state_e state);
char *musiccast_zone_status(musiccast_conn_t *mc, const char *zone);
bool musiccast_zone_volume_set(musiccast_conn_t *mc, const char *zone, musiccast_zone_volume_action_e action, int volume_step);
bool musiccast_zone_mute(musiccast_conn_t *mc, const char *zone, bool mute);

#endif /* INCLUDE_MUSICCAST_H_ */
