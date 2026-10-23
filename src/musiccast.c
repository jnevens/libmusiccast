/*
 * musiccast.c
 *
 *  Created on: Jun 11, 2018
 *      Author: jnevens
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <json-c/json.h>

#include <musiccast_priv.h>
#include <request.h>


// static function declarations
static bool musiccast_system_device_info_parse(musiccast_conn_t *mcc, const char *device_info);
static bool musiccast_system_features_parse(musiccast_conn_t *mcc, const char *system_features);
bool musiccast_zone_status_parse(musiccast_conn_t *mc, int zone, const char *zone_state);

struct in_addr hostname_to_ip(const char * hostname)
{
	struct hostent *he;
	struct in_addr **addr_list;
	struct in_addr ip = { .s_addr = 0x0 };
	int i;

	if ((he = gethostbyname(hostname)) == NULL) {
		// get the host info
		fprintf(stderr, "gethostbyname: %m");
		return ip;
	}

	addr_list = (struct in_addr **) he->h_addr_list;

	for (i = 0; addr_list[i] != NULL; i++) {
		//Return the first one;
		ip = *addr_list[i];
		break;
	}

	return ip;
}

musiccast_conn_t * musiccast_init(const char *hostname)
{
	musiccast_conn_t *mcc = calloc(1, sizeof(musiccast_conn_t));

	mcc->ip = hostname_to_ip(hostname);

	if (mcc->ip.s_addr == 0) {
		fprintf(stderr, "Failed to resolve hostname!\n");
		free(mcc);
		return NULL;
	}

	// get device info
	char *device_info = musiccast_system_device_info(mcc);
	if (!device_info) {
		fprintf(stderr, "Device unreachable!\n");
		musiccast_destroy(mcc);
		return NULL;
	}

	if(!musiccast_system_device_info_parse(mcc, device_info)) {
		fprintf(stderr, "Failed to parse device info\n");
		musiccast_destroy(mcc);
		return NULL;
	}
	free(device_info);

	// get system features
	char *system_features = musiccast_system_features(mcc);
	if (!system_features) {
		fprintf(stderr, "Failed to request system features\n");
		musiccast_destroy(mcc);
		return NULL;
	}

	if(!musiccast_system_features_parse(mcc, system_features)) {
		fprintf(stderr, "Failed to parse system features\n");
		musiccast_destroy(mcc);
		return NULL;
	}
	free(system_features);

	// get zone info
	for (int i = 0; i < mcc->device.zone_num; i++) {
		const char *zone_name = mcc->device.zone[i].id;
		char *zone_state = musiccast_zone_status(mcc, zone_name);
		if (!zone_state) {
			fprintf(stderr, "Failed to request zone status (%s)\n", zone_name);
			musiccast_destroy(mcc);
			return NULL;
		}

		if (!musiccast_zone_status_parse(mcc, i, zone_state)) {
			fprintf(stderr, "Failed to parse zone status (%s)\n", zone_name);
			musiccast_destroy(mcc);
			return NULL;
		}
		free(zone_state);
	}

	return mcc;
}

void musiccast_destroy(musiccast_conn_t *mcc)
{
	for(int i = 0; i < MUSICCAST_MAX_ZONES; i++) {
		free(mcc->device.zone[i].id);
		for(int j = 0; j < MUSICCAST_MAX_ZONE_RANGE_STEPS; j++)
			free(mcc->device.zone[i].range_steps[j].id);
	}
	free(mcc->device.model_name);
	free(mcc->device.device_id);
	free(mcc->device.system_id);
	free(mcc);
}

char *musiccast_system_device_info(musiccast_conn_t *mc)
{
	char *response = NULL;
	musiccast_request(mc, "/YamahaExtendedControl/v1/system/getDeviceInfo", &response);
	return response;
}

static bool musiccast_system_device_info_parse(musiccast_conn_t *mcc, const char *device_info)
{
	json_object *jobj = json_tokener_parse(device_info);
	json_object *jobj_model_name;
	json_object *jobj_device_id;
	json_object *jobj_system_id;

	if (!jobj)
		return false;

	jobj_model_name = json_object_object_get(jobj, "model_name");
	jobj_device_id = json_object_object_get(jobj, "device_id");
	jobj_system_id = json_object_object_get(jobj, "system_id");
	if (!jobj_model_name || !json_object_is_type(jobj_model_name, json_type_string)) {
		json_object_put(jobj);
		return false;
	}

	mcc->device.model_name = strdup(json_object_get_string(jobj_model_name));
	if (jobj_device_id && json_object_is_type(jobj_device_id, json_type_string))
		mcc->device.device_id = strdup(json_object_get_string(jobj_device_id));
	if (jobj_system_id && json_object_is_type(jobj_system_id, json_type_string))
		mcc->device.system_id = strdup(json_object_get_string(jobj_system_id));
	json_object_put(jobj);

	return mcc->device.model_name != NULL;
}

char *musiccast_system_features(musiccast_conn_t *mc)
{
	char *response = NULL;
	musiccast_request(mc, "/YamahaExtendedControl/v1/system/getFeatures", &response);
	return response;
}

static bool musiccast_system_features_parse(musiccast_conn_t *mcc, const char *system_features)
{
	json_object *jobj = json_tokener_parse(system_features);
	json_object *jobj_system = json_object_object_get(jobj, "system");
	json_object *jobj_system_zone_num = json_object_object_get(jobj_system, "zone_num");
	mcc->device.zone_num = json_object_get_int(jobj_system_zone_num);
	json_object *jobj_zones = json_object_object_get(jobj, "zone");
	for (int i = 0; i < mcc->device.zone_num; i++) {
		json_object *jobj_zone = json_object_array_get_idx(jobj_zones, i);
		json_object *jobj_zone_id = json_object_object_get(jobj_zone, "id");
		mcc->device.zone[i].id = strdup(json_object_get_string(jobj_zone_id));
		printf("found zone: %s\n", mcc->device.zone[i].id);
		json_object *jobj_zone_range_steps = json_object_object_get(jobj_zone, "range_step");
		int range_steps_count = json_object_array_length(jobj_zone_range_steps);
		for (int j = 0; j < range_steps_count; j++) {
			json_object *jobj_range_step = json_object_array_get_idx(jobj_zone_range_steps, j);
			json_object *jobj_range_step_id = json_object_object_get(jobj_range_step, "id");
			mcc->device.zone[i].range_steps[j].id = strdup(json_object_get_string(jobj_range_step_id));
			json_object *jobj_range_step_min = json_object_object_get(jobj_range_step, "min");
			mcc->device.zone[i].range_steps[j].min = json_object_get_double(jobj_range_step_min);
			json_object *jobj_range_step_max = json_object_object_get(jobj_range_step, "max");
			mcc->device.zone[i].range_steps[j].max = json_object_get_double(jobj_range_step_max);
			json_object *jobj_range_step_step = json_object_object_get(jobj_range_step, "step");
			mcc->device.zone[i].range_steps[j].step = json_object_get_double(jobj_range_step_step);

			printf("range step: id:%s, min:%lf, max:%lf, step:%lf\n",
					mcc->device.zone[i].range_steps[j].id,
					mcc->device.zone[i].range_steps[j].min,
					mcc->device.zone[i].range_steps[j].max,
					mcc->device.zone[i].range_steps[j].step);
		}
	}

	json_object_put(jobj);
	return true;
}

char *musiccast_system_network_status(musiccast_conn_t *mc)
{
	char *response = NULL;
	musiccast_request(mc, "/YamahaExtendedControl/v1/system/getNetworkStatus", &response);
	return response;
}

char *musiccast_system_func_status(musiccast_conn_t *mc)
{
	char *response = NULL;
	musiccast_request(mc, "/YamahaExtendedControl/v1/system/getFuncStatus", &response);
	return response;
}

char *musiccast_system_location_info(musiccast_conn_t *mc)
{
	char *response = NULL;
	 musiccast_request(mc, "/YamahaExtendedControl/v1/system/getLocationInfo", &response);
	return response;
}

char *musiccast_system_sound_program_list(musiccast_conn_t *mc)
{
	char *response = NULL;
	musiccast_request(mc, "/YamahaExtendedControl/v1/system/getSoundProgramList", &response);
	return response;
}

bool musiccast_zone_power_set(musiccast_conn_t *mc, const char *zone, musiccast_power_state_e state)
{
	bool rv;
	char *response = NULL;
	char *path = NULL;
	char *action = NULL;

	if (!mc)
		return false;
	if (!zone)
		return false;

	switch (state) {
	case MC_POWER_STATE_ON:
		action = "on";
		break;
	case MC_POWER_STATE_STANDBY:
		action = "standby";
		break;
	case MC_POWER_STATE_TOGGLE:
		action = "toggle";
		break;
	default:
		return false;
	}

	if (asprintf(&path, "/YamahaExtendedControl/v1/%s/setPower?power=%s", zone, action) <= 0)
		return false;

	rv = musiccast_request(mc, path, &response);
	printf("%s\n", response);
	free(response);
	free(path);
	return rv;
}

char *musiccast_zone_status(musiccast_conn_t *mc, const char *zone)
{
	char *response = NULL;
	char *path = NULL;

	if (!mc)
		return NULL;
	if (!zone)
		return NULL;

	if(asprintf(&path, "/YamahaExtendedControl/v1/%s/getStatus", zone) <= 0 ) {
		return NULL;
	}

	musiccast_request(mc, path, &response);
	free(path);
	return response;
}

/*
{
  "response_code": 0,
  "power": "on",
  "sleep": 0,
  "volume": 4,
  "mute": false,
  "max_volume": 60,
  "input": "net_radio",
  "distribution_enable": true,
  "tone_control": {
    "mode": "manual",
    "bass": 5,
    "treble": 0
  },
  "balance": 0,
  "link_control": "standard",
  "link_audio_delay": "audio_sync_on",
  "link_audio_quality": "compressed",
  "disable_flags": 0
}
*/

bool musiccast_zone_status_parse(musiccast_conn_t *mc, int zone, const char *zone_status)
{
	json_object *jobj = json_tokener_parse(zone_status);
	json_object *jobj_power = json_object_object_get(jobj, "power");
	const char *zone_power = json_object_get_string(jobj_power);
	if (strcmp(zone_power, "on") == 0) {
		mc->device.zone[zone].power = MC_POWER_STATE_ON;
	} else if (strcmp(zone_power, "standby") == 0) {
		mc->device.zone[zone].power = MC_POWER_STATE_STANDBY;
	}
	json_object *jobj_volume = json_object_object_get(jobj, "volume");
	mc->device.zone[zone].volume = json_object_get_int(jobj_volume);
	json_object_put(jobj);

	return true;
}

bool musiccast_zone_volume_set(musiccast_conn_t *mc, const char *zone, musiccast_zone_volume_action_e action, int volume_step)
{
	bool rv;
	char *response = NULL;
	char *path = NULL;

	if (!mc)
		return false;
	if (!zone)
		return false;

	if (action == MC_ZONE_VOLUME_SET_DIRECT) {
		asprintf(&path, "/YamahaExtendedControl/v1/%s/setVolume?volume=%d", zone, volume_step);
	} else if (action == MC_ZONE_VOLUME_SET_UP) {
		asprintf(&path, "/YamahaExtendedControl/v1/%s/setVolume?volume=up&step=%d", zone, volume_step);
	} else if (action == MC_ZONE_VOLUME_SET_DOWN) {
		asprintf(&path, "/YamahaExtendedControl/v1/%s/setVolume?volume=down&step=%d", zone, volume_step);
	} else {
		return false;
	}

	rv = musiccast_request(mc, path, &response);
	printf("%s\n", response);
	free(response);
	free(path);
	return rv;
}

bool musiccast_zone_mute(musiccast_conn_t *mc, const char *zone, bool mute)
{
	bool rv;
	char *response = NULL;
	char *path = NULL;
	char *action = (mute) ? "true" : "false";

	if (!mc)
		return false;
	if (!zone)
		return false;

	asprintf(&path, "/YamahaExtendedControl/v1/%s/setMute?enable=%s", zone, action);

	rv = musiccast_request(mc, path, &response);
	printf("%s\n", response);
	free(response);
	free(path);
	return rv;
}
