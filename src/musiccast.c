/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2015-2026 Jasper Nevens
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
static bool musiccast_response_ok(const char *response);
bool musiccast_zone_status_parse(musiccast_conn_t *mc, int zone, const char *zone_state);

struct in_addr hostname_to_ip(const char * hostname)
{
	struct hostent *he;
	struct in_addr **addr_list;
	struct in_addr ip = { .s_addr = 0x0 };
	int i;

	if ((he = gethostbyname(hostname)) == NULL) {
		fprintf(stderr, "gethostbyname: %s\n", strerror(errno));
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

musiccast_conn_t *musiccast_connect(const char *hostname)
{
	musiccast_conn_t *mcc = calloc(1, sizeof(musiccast_conn_t));

	if (!mcc)
		return NULL;

	mcc->ip = hostname_to_ip(hostname);
	if (mcc->ip.s_addr == 0) {
		fprintf(stderr, "Failed to resolve hostname!\n");
		free(mcc);
		return NULL;
	}

	return mcc;
}

musiccast_conn_t * musiccast_init(const char *hostname)
{
	musiccast_conn_t *mcc = musiccast_connect(hostname);

	if (!mcc)
		return NULL;

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
	for (size_t i = 0; i < mcc->device.zone_num; i++) {
		const char *zone_name = mcc->device.zone[i].id;
		char *zone_state = musiccast_zone_status(mcc, zone_name);
		if (!zone_state) {
			fprintf(stderr, "Failed to request zone status (%s)\n", zone_name);
			musiccast_destroy(mcc);
			return NULL;
		}

		if (!musiccast_zone_status_parse(mcc, (int)i, zone_state)) {
			fprintf(stderr, "Failed to parse zone status (%s)\n", zone_name);
			musiccast_destroy(mcc);
			return NULL;
		}
		free(zone_state);
	}

	return mcc;
}

const char *musiccast_version(void)
{
	return MUSICCAST_VERSION;
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
	for (size_t i = 0; i < mcc->device.zone_num; i++) {
		json_object *jobj_zone = json_object_array_get_idx(jobj_zones, (int)i);
		json_object *jobj_zone_id = json_object_object_get(jobj_zone, "id");
		mcc->device.zone[i].id = strdup(json_object_get_string(jobj_zone_id));
		json_object *jobj_zone_range_steps = json_object_object_get(jobj_zone, "range_step");
		size_t range_steps_count = (size_t)json_object_array_length(jobj_zone_range_steps);
		for (size_t j = 0; j < range_steps_count; j++) {
			json_object *jobj_range_step = json_object_array_get_idx(jobj_zone_range_steps, (int)j);
			json_object *jobj_range_step_id = json_object_object_get(jobj_range_step, "id");
			mcc->device.zone[i].range_steps[j].id = strdup(json_object_get_string(jobj_range_step_id));
			json_object *jobj_range_step_min = json_object_object_get(jobj_range_step, "min");
			mcc->device.zone[i].range_steps[j].min = json_object_get_double(jobj_range_step_min);
			json_object *jobj_range_step_max = json_object_object_get(jobj_range_step, "max");
			mcc->device.zone[i].range_steps[j].max = json_object_get_double(jobj_range_step_max);
			json_object *jobj_range_step_step = json_object_object_get(jobj_range_step, "step");
			mcc->device.zone[i].range_steps[j].step = json_object_get_double(jobj_range_step_step);
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

musiccast_zone_list_t *musiccast_zone_list(musiccast_conn_t *mc)
{
	char *response = NULL;
	json_object *jobj;
	json_object *jobj_zones;
	musiccast_zone_list_t *list;

	if (!mc)
		return NULL;

	if (!musiccast_request(mc, "/YamahaExtendedControl/v1/system/getFeatures", &response))
		return NULL;
	if (!response || !musiccast_response_ok(response)) {
		free(response);
		return NULL;
	}

	jobj = json_tokener_parse(response);
	free(response);
	if (!jobj)
		return NULL;

	jobj_zones = json_object_object_get(jobj, "zone");
	if (!jobj_zones || !json_object_is_type(jobj_zones, json_type_array)) {
		json_object_put(jobj);
		return NULL;
	}

	list = calloc(1, sizeof(*list));
	if (!list) {
		json_object_put(jobj);
		return NULL;
	}

	list->count = (size_t)json_object_array_length(jobj_zones);
	if (list->count > 0) {
		list->zones = calloc(list->count, sizeof(*list->zones));
		if (!list->zones) {
			free(list);
			json_object_put(jobj);
			return NULL;
		}
	}

	for (size_t i = 0; i < list->count; i++) {
		json_object *jobj_zone = json_object_array_get_idx(jobj_zones, (int)i);
		json_object *jobj_id = json_object_object_get(jobj_zone, "id");
		const char *id;

		if (!jobj_id || !json_object_is_type(jobj_id, json_type_string))
			continue;

		id = json_object_get_string(jobj_id);
		list->zones[i].id = strdup(id);
	}

	json_object_put(jobj);
	return list;
}

void musiccast_zone_list_free(musiccast_zone_list_t *list)
{
	if (!list)
		return;

	for (size_t i = 0; i < list->count; i++)
		free(list->zones[i].id);
	free(list->zones);
	free(list);
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
	free(response);
	free(path);
	return rv;
}

char *musiccast_zone_power_get(musiccast_conn_t *mc, const char *zone)
{
	char *status;
	char *power = NULL;
	json_object *jobj;
	json_object *jobj_power;

	status = musiccast_zone_status(mc, zone);
	if (!status)
		return NULL;

	jobj = json_tokener_parse(status);
	free(status);
	if (!jobj)
		return NULL;

	jobj_power = json_object_object_get(jobj, "power");
	if (jobj_power && json_object_is_type(jobj_power, json_type_string))
		power = strdup(json_object_get_string(jobj_power));

	json_object_put(jobj);
	return power;
}

static bool musiccast_features_volume_range(musiccast_conn_t *mc, const char *zone,
					    int *min, int *max, int *step)
{
	char *features;
	json_object *jobj;
	json_object *jobj_zones;
	bool found = false;

	features = musiccast_system_features(mc);
	if (!features)
		return false;

	jobj = json_tokener_parse(features);
	free(features);
	if (!jobj)
		return false;

	jobj_zones = json_object_object_get(jobj, "zone");
	if (!jobj_zones || !json_object_is_type(jobj_zones, json_type_array)) {
		json_object_put(jobj);
		return false;
	}

	for (size_t i = 0; i < (size_t)json_object_array_length(jobj_zones); i++) {
		json_object *jobj_zone = json_object_array_get_idx(jobj_zones, (int)i);
		json_object *jobj_zone_id = json_object_object_get(jobj_zone, "id");
		json_object *jobj_range_steps;
		const char *zone_id;

		if (!jobj_zone_id || !json_object_is_type(jobj_zone_id, json_type_string))
			continue;

		zone_id = json_object_get_string(jobj_zone_id);
		if (strcmp(zone_id, zone) != 0)
			continue;

		jobj_range_steps = json_object_object_get(jobj_zone, "range_step");
		if (!jobj_range_steps || !json_object_is_type(jobj_range_steps, json_type_array))
			break;

		for (size_t j = 0; j < (size_t)json_object_array_length(jobj_range_steps); j++) {
			json_object *jobj_step = json_object_array_get_idx(jobj_range_steps, (int)j);
			json_object *jobj_step_id = json_object_object_get(jobj_step, "id");

			if (!jobj_step_id || !json_object_is_type(jobj_step_id, json_type_string))
				continue;
			if (strcmp(json_object_get_string(jobj_step_id), "volume") != 0)
				continue;

			*min = (int)json_object_get_double(json_object_object_get(jobj_step, "min"));
			*max = (int)json_object_get_double(json_object_object_get(jobj_step, "max"));
			*step = (int)json_object_get_double(json_object_object_get(jobj_step, "step"));
			found = true;
			break;
		}
		break;
	}

	json_object_put(jobj);
	return found;
}

bool musiccast_zone_volume_query(musiccast_conn_t *mc, const char *zone,
				 musiccast_zone_volume_info_t *info)
{
	char *status;
	json_object *jobj;
	json_object *jobj_volume;
	json_object *jobj_max_volume;
	int min = 0, max = 0, step = 0;

	if (!info)
		return false;

	memset(info, 0, sizeof(*info));

	status = musiccast_zone_status(mc, zone);
	if (!status)
		return false;

	jobj = json_tokener_parse(status);
	free(status);
	if (!jobj)
		return false;

	jobj_volume = json_object_object_get(jobj, "volume");
	if (!jobj_volume || !json_object_is_type(jobj_volume, json_type_int)) {
		json_object_put(jobj);
		return false;
	}

	info->volume = json_object_get_int(jobj_volume);
	jobj_max_volume = json_object_object_get(jobj, "max_volume");
	if (jobj_max_volume && json_object_is_type(jobj_max_volume, json_type_int))
		info->max_volume = json_object_get_int(jobj_max_volume);

	json_object_put(jobj);

	if (musiccast_features_volume_range(mc, zone, &min, &max, &step)) {
		info->min = min;
		info->max = max;
		info->step = step;
		info->range_valid = true;
	}

	return true;
}

bool musiccast_zone_volume_get(musiccast_conn_t *mc, const char *zone, int *volume)
{
	musiccast_zone_volume_info_t info;

	if (!volume)
		return false;
	if (!musiccast_zone_volume_query(mc, zone, &info))
		return false;

	*volume = info.volume;
	return true;
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
		if (asprintf(&path, "/YamahaExtendedControl/v1/%s/setVolume?volume=%d", zone, volume_step) < 0)
			return false;
	} else if (action == MC_ZONE_VOLUME_SET_UP) {
		if (asprintf(&path, "/YamahaExtendedControl/v1/%s/setVolume?volume=up&step=%d", zone, volume_step) < 0)
			return false;
	} else if (action == MC_ZONE_VOLUME_SET_DOWN) {
		if (asprintf(&path, "/YamahaExtendedControl/v1/%s/setVolume?volume=down&step=%d", zone, volume_step) < 0)
			return false;
	} else {
		return false;
	}

	rv = musiccast_request(mc, path, &response);
	free(response);
	free(path);
	return rv;
}

bool musiccast_zone_mute_get(musiccast_conn_t *mc, const char *zone, bool *muted)
{
	char *status;
	json_object *jobj;
	json_object *jobj_mute;

	if (!mc || !zone || !muted)
		return false;

	status = musiccast_zone_status(mc, zone);
	if (!status)
		return false;

	jobj = json_tokener_parse(status);
	free(status);
	if (!jobj)
		return false;

	jobj_mute = json_object_object_get(jobj, "mute");
	if (!jobj_mute || !json_object_is_type(jobj_mute, json_type_boolean)) {
		json_object_put(jobj);
		return false;
	}

	*muted = json_object_get_boolean(jobj_mute);
	json_object_put(jobj);
	return true;
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

	if (asprintf(&path, "/YamahaExtendedControl/v1/%s/setMute?enable=%s", zone, action) < 0)
		return false;

	rv = musiccast_request(mc, path, &response);
	free(response);
	free(path);
	return rv;
}

char *musiccast_zone_input_get(musiccast_conn_t *mc, const char *zone)
{
	char *status;
	char *input = NULL;
	json_object *jobj;
	json_object *jobj_input;

	status = musiccast_zone_status(mc, zone);
	if (!status)
		return NULL;

	jobj = json_tokener_parse(status);
	free(status);
	if (!jobj)
		return NULL;

	jobj_input = json_object_object_get(jobj, "input");
	if (jobj_input && json_object_is_type(jobj_input, json_type_string))
		input = strdup(json_object_get_string(jobj_input));

	json_object_put(jobj);
	return input;
}

static const char *zone_input_list_item_id(json_object *item)
{
	json_object *jobj_id;

	if (!item)
		return NULL;
	if (json_object_is_type(item, json_type_string))
		return json_object_get_string(item);
	if (!json_object_is_type(item, json_type_object))
		return NULL;

	jobj_id = json_object_object_get(item, "id");
	if (!jobj_id || !json_object_is_type(jobj_id, json_type_string))
		return NULL;
	return json_object_get_string(jobj_id);
}

musiccast_zone_input_list_t *musiccast_zone_input_list(musiccast_conn_t *mc, const char *zone)
{
	char *features;
	json_object *jobj;
	json_object *jobj_zones;
	json_object *jobj_inputs = NULL;
	musiccast_zone_input_list_t *list;
	bool found = false;
	size_t n = 0;

	if (!mc || !zone)
		return NULL;

	features = musiccast_system_features(mc);
	if (!features)
		return NULL;

	jobj = json_tokener_parse(features);
	free(features);
	if (!jobj)
		return NULL;

	jobj_zones = json_object_object_get(jobj, "zone");
	if (!jobj_zones || !json_object_is_type(jobj_zones, json_type_array)) {
		json_object_put(jobj);
		return NULL;
	}

	for (size_t i = 0; i < (size_t)json_object_array_length(jobj_zones); i++) {
		json_object *jobj_zone = json_object_array_get_idx(jobj_zones, (int)i);
		json_object *jobj_zone_id = json_object_object_get(jobj_zone, "id");
		const char *zone_id;

		if (!jobj_zone_id || !json_object_is_type(jobj_zone_id, json_type_string))
			continue;

		zone_id = json_object_get_string(jobj_zone_id);
		if (strcmp(zone_id, zone) != 0)
			continue;

		found = true;
		jobj_inputs = json_object_object_get(jobj_zone, "input_list");
		break;
	}

	if (!found || !jobj_inputs || !json_object_is_type(jobj_inputs, json_type_array)) {
		json_object_put(jobj);
		return NULL;
	}

	list = calloc(1, sizeof(*list));
	if (!list) {
		json_object_put(jobj);
		return NULL;
	}

	list->count = (size_t)json_object_array_length(jobj_inputs);
	if (list->count > 0) {
		list->ids = calloc(list->count, sizeof(*list->ids));
		if (!list->ids) {
			free(list);
			json_object_put(jobj);
			return NULL;
		}
	}

	for (size_t i = 0; i < list->count; i++) {
		const char *id = zone_input_list_item_id(json_object_array_get_idx(jobj_inputs, (int)i));

		if (!id || id[0] == '\0')
			continue;

		list->ids[n] = strdup(id);
		if (!list->ids[n]) {
			list->count = n;
			musiccast_zone_input_list_free(list);
			json_object_put(jobj);
			return NULL;
		}
		n++;
	}
	list->count = n;

	json_object_put(jobj);
	return list;
}

void musiccast_zone_input_list_free(musiccast_zone_input_list_t *list)
{
	if (!list)
		return;

	for (size_t i = 0; i < list->count; i++)
		free(list->ids[i]);
	free(list->ids);
	free(list);
}

/* YXC input ids are tokens such as net_radio, hdmi1, tuner. */
static bool zone_input_id_valid(const char *input)
{
	if (!input || input[0] == '\0')
		return false;

	for (const char *p = input; *p != '\0'; p++) {
		if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')
		    || (*p >= '0' && *p <= '9') || *p == '_')
			continue;
		return false;
	}
	return true;
}

bool musiccast_zone_input_set(musiccast_conn_t *mc, const char *zone, const char *input)
{
	bool rv;
	char *response = NULL;
	char *path = NULL;

	if (!mc || !zone || !zone_input_id_valid(input))
		return false;

	if (asprintf(&path, "/YamahaExtendedControl/v1/%s/setInput?input=%s", zone, input) <= 0)
		return false;

	rv = musiccast_request(mc, path, &response);
	if (rv && response)
		rv = musiccast_response_ok(response);
	free(response);
	free(path);
	return rv;
}

static bool musiccast_response_ok(const char *response)
{
	json_object *jobj;
	json_object *jobj_code;
	bool ok = false;

	if (!response)
		return false;

	jobj = json_tokener_parse(response);
	if (!jobj)
		return false;

	jobj_code = json_object_object_get(jobj, "response_code");
	if (jobj_code && json_object_is_type(jobj_code, json_type_int))
		ok = json_object_get_int(jobj_code) == 0;

	json_object_put(jobj);
	return ok;
}

musiccast_netusb_preset_list_t *musiccast_netusb_preset_list(musiccast_conn_t *mc)
{
	char *response = NULL;
	json_object *jobj;
	json_object *jobj_presets;
	musiccast_netusb_preset_list_t *list;

	if (!mc)
		return NULL;

	if (!musiccast_request(mc, "/YamahaExtendedControl/v1/netusb/getPresetInfo", &response))
		return NULL;
	if (!response || !musiccast_response_ok(response)) {
		free(response);
		return NULL;
	}

	jobj = json_tokener_parse(response);
	free(response);
	if (!jobj)
		return NULL;

	jobj_presets = json_object_object_get(jobj, "preset_info");
	if (!jobj_presets || !json_object_is_type(jobj_presets, json_type_array)) {
		json_object_put(jobj);
		return NULL;
	}

	list = calloc(1, sizeof(*list));
	if (!list) {
		json_object_put(jobj);
		return NULL;
	}

	list->count = (size_t)json_object_array_length(jobj_presets);
	if (list->count == 0) {
		json_object_put(jobj);
		return list;
	}

	list->presets = calloc(list->count, sizeof(*list->presets));
	if (!list->presets) {
		free(list);
		json_object_put(jobj);
		return NULL;
	}

	for (size_t i = 0; i < list->count; i++) {
		json_object *jobj_entry = json_object_array_get_idx(jobj_presets, (int)i);
		json_object *jobj_input = json_object_object_get(jobj_entry, "input");
		json_object *jobj_text = json_object_object_get(jobj_entry, "text");
		const char *input = NULL;
		const char *text = "";

		list->presets[i].num = (int)i + 1;
		if (jobj_input && json_object_is_type(jobj_input, json_type_string))
			input = json_object_get_string(jobj_input);
		if (jobj_text && json_object_is_type(jobj_text, json_type_string))
			text = json_object_get_string(jobj_text);

		if (!input || strcmp(input, "unknown") == 0 || text[0] == '\0') {
			list->presets[i].empty = true;
			list->presets[i].input = NULL;
			list->presets[i].text = NULL;
		} else {
			list->presets[i].empty = false;
			list->presets[i].input = strdup(input);
			list->presets[i].text = strdup(text);
		}
	}

	json_object_put(jobj);
	return list;
}

void musiccast_netusb_preset_list_free(musiccast_netusb_preset_list_t *list)
{
	if (!list)
		return;

	for (size_t i = 0; i < list->count; i++) {
		free(list->presets[i].input);
		free(list->presets[i].text);
	}
	free(list->presets);
	free(list);
}

bool musiccast_netusb_preset_recall(musiccast_conn_t *mc, const char *zone, int num)
{
	bool rv;
	char *response = NULL;
	char *path = NULL;

	if (!mc || !zone || num < 1)
		return false;

	if (asprintf(&path,
		     "/YamahaExtendedControl/v1/netusb/recallPreset?zone=%s&num=%d",
		     zone, num) <= 0)
		return false;

	rv = musiccast_request(mc, path, &response);
	if (rv && response)
		rv = musiccast_response_ok(response);
	free(response);
	free(path);
	return rv;
}

bool musiccast_netusb_preset_store(musiccast_conn_t *mc, int num)
{
	bool rv;
	char *response = NULL;
	char *path = NULL;

	if (!mc || num < 1)
		return false;

	if (asprintf(&path, "/YamahaExtendedControl/v1/netusb/storePreset?num=%d", num) <= 0)
		return false;

	rv = musiccast_request(mc, path, &response);
	if (rv && response)
		rv = musiccast_response_ok(response);
	free(response);
	free(path);
	return rv;
}

bool musiccast_netusb_preset_clear(musiccast_conn_t *mc, int num)
{
	bool rv;
	char *response = NULL;
	char *path = NULL;

	if (!mc || num < 1)
		return false;

	if (asprintf(&path, "/YamahaExtendedControl/v1/netusb/clearPreset?num=%d", num) <= 0)
		return false;

	rv = musiccast_request(mc, path, &response);
	if (rv && response)
		rv = musiccast_response_ok(response);
	free(response);
	free(path);
	return rv;
}

char *musiccast_netusb_play_info(musiccast_conn_t *mc)
{
	char *response = NULL;

	if (!mc)
		return NULL;
	if (!musiccast_request(mc, "/YamahaExtendedControl/v1/netusb/getPlayInfo", &response))
		return NULL;
	if (!response || !musiccast_response_ok(response)) {
		free(response);
		return NULL;
	}
	return response;
}

char *musiccast_tuner_play_info(musiccast_conn_t *mc)
{
	char *response = NULL;

	if (!mc)
		return NULL;
	if (!musiccast_request(mc, "/YamahaExtendedControl/v1/tuner/getPlayInfo", &response))
		return NULL;
	if (!response || !musiccast_response_ok(response)) {
		free(response);
		return NULL;
	}
	return response;
}

char *musiccast_cd_play_info(musiccast_conn_t *mc)
{
	char *response = NULL;

	if (!mc)
		return NULL;
	if (!musiccast_request(mc, "/YamahaExtendedControl/v1/cd/getPlayInfo", &response))
		return NULL;
	if (!response || !musiccast_response_ok(response)) {
		free(response);
		return NULL;
	}
	return response;
}

static char *json_strdup_string_field(json_object *parent, const char *key)
{
	json_object *jobj;
	const char *s;

	if (!parent)
		return NULL;
	jobj = json_object_object_get(parent, key);
	if (!jobj || !json_object_is_type(jobj, json_type_string))
		return NULL;
	s = json_object_get_string(jobj);
	if (!s || s[0] == '\0')
		return NULL;
	return strdup(s);
}

static int json_get_int_field(json_object *parent, const char *key, int fallback)
{
	json_object *jobj;

	if (!parent)
		return fallback;
	jobj = json_object_object_get(parent, key);
	if (!jobj || !json_object_is_type(jobj, json_type_int))
		return fallback;
	return json_object_get_int(jobj);
}

static void now_playing_apply_netusb(musiccast_now_playing_t *info, json_object *jobj)
{
	info->playback = json_strdup_string_field(jobj, "playback");
	info->artist = json_strdup_string_field(jobj, "artist");
	info->album = json_strdup_string_field(jobj, "album");
	info->track = json_strdup_string_field(jobj, "track");
	info->play_time = json_get_int_field(jobj, "play_time", -60000);
	info->total_time = json_get_int_field(jobj, "total_time", 0);
	info->play_info = true;
}

static void now_playing_apply_cd(musiccast_now_playing_t *info, json_object *jobj)
{
	info->playback = json_strdup_string_field(jobj, "playback");
	info->artist = json_strdup_string_field(jobj, "artist");
	info->album = json_strdup_string_field(jobj, "album");
	info->track = json_strdup_string_field(jobj, "track");
	info->play_time = json_get_int_field(jobj, "play_time", -60000);
	info->total_time = json_get_int_field(jobj, "total_time", 0);
	info->play_info = true;
}

static void now_playing_apply_tuner(musiccast_now_playing_t *info, json_object *jobj)
{
	json_object *jobj_rds;
	json_object *jobj_dab;
	json_object *jobj_fm;
	json_object *jobj_band;
	const char *band = NULL;

	jobj_band = json_object_object_get(jobj, "band");
	if (jobj_band && json_object_is_type(jobj_band, json_type_string))
		band = json_object_get_string(jobj_band);

	info->playback = json_strdup_string_field(jobj, "playback");
	if (!info->playback)
		info->playback = strdup("play");

	jobj_rds = json_object_object_get(jobj, "rds");
	if (jobj_rds) {
		info->track = json_strdup_string_field(jobj_rds, "program_service");
		if (!info->track)
			info->track = json_strdup_string_field(jobj_rds, "radio_text_a");
		info->detail = json_strdup_string_field(jobj_rds, "program_type");
	}

	jobj_dab = json_object_object_get(jobj, "dab");
	if (jobj_dab) {
		if (!info->track) {
			info->track = json_strdup_string_field(jobj_dab, "service_label");
			if (!info->track)
				info->track = json_strdup_string_field(jobj_dab, "ch_label");
		}
		if (!info->detail)
			info->detail = json_strdup_string_field(jobj_dab, "ensemble_label");
	}

	jobj_fm = json_object_object_get(jobj, "fm");
	if (jobj_fm && band && strcmp(band, "fm") == 0 && !info->track) {
		int freq = json_get_int_field(jobj_fm, "freq", 0);
		char buf[64];

		if (freq > 0) {
			snprintf(buf, sizeof(buf), "%.1f MHz", freq / 1000.0);
			info->track = strdup(buf);
		}
	}

	if (band && band[0] != '\0') {
		char buf[64];

		snprintf(buf, sizeof(buf), "band=%s", band);
		if (info->detail) {
			char *combined;

			if (asprintf(&combined, "%s  %s", buf, info->detail) > 0) {
				free(info->detail);
				info->detail = combined;
			}
		} else {
			info->detail = strdup(buf);
		}
	}

	info->play_info = true;
}

static int now_playing_source_kind(const char *input)
{
	if (!input || input[0] == '\0')
		return 0;
	if (strcmp(input, "cd") == 0)
		return 3;
	if (strcmp(input, "tuner") == 0 || strcmp(input, "dab") == 0
	    || strcmp(input, "fm") == 0 || strcmp(input, "am") == 0)
		return 2;
	if (strncmp(input, "hdmi", 4) == 0 || strncmp(input, "av", 2) == 0
	    || strcmp(input, "aux") == 0 || strncmp(input, "audio", 5) == 0
	    || strcmp(input, "phono") == 0 || strcmp(input, "tv") == 0)
		return 0;
	return 1;
}

static const char *playback_query_value(musiccast_playback_action_e action)
{
	switch (action) {
	case MC_PLAYBACK_PLAY:
		return "play";
	case MC_PLAYBACK_PAUSE:
		return "pause";
	case MC_PLAYBACK_STOP:
		return "stop";
	case MC_PLAYBACK_TOGGLE:
		return "play_pause";
	case MC_PLAYBACK_PREVIOUS:
		return "previous";
	case MC_PLAYBACK_NEXT:
		return "next";
	default:
		return NULL;
	}
}

static bool playback_set_module(musiccast_conn_t *mc, const char *module, const char *playback)
{
	bool rv;
	char *response = NULL;
	char *path = NULL;

	if (!mc || !module || !playback)
		return false;

	if (asprintf(&path, "/YamahaExtendedControl/v1/%s/setPlayback?playback=%s",
		     module, playback) <= 0)
		return false;

	rv = musiccast_request(mc, path, &response);
	if (rv && response)
		rv = musiccast_response_ok(response);
	free(response);
	free(path);
	return rv;
}

bool musiccast_netusb_playback_set(musiccast_conn_t *mc, musiccast_playback_action_e action)
{
	return playback_set_module(mc, "netusb", playback_query_value(action));
}

/* CD setPlayback has no play_pause; flip using the current playback state. */
static const char *cd_toggle_playback(musiccast_conn_t *mc)
{
	char *raw;
	json_object *jobj;
	char *state;
	const char *target = "play";

	raw = musiccast_cd_play_info(mc);
	if (!raw)
		return "play";

	jobj = json_tokener_parse(raw);
	free(raw);
	if (!jobj)
		return "play";

	state = json_strdup_string_field(jobj, "playback");
	json_object_put(jobj);
	if (state && strcmp(state, "pause") != 0 && strcmp(state, "stop") != 0)
		target = "pause";
	free(state);
	return target;
}

bool musiccast_cd_playback_set(musiccast_conn_t *mc, musiccast_playback_action_e action)
{
	const char *playback;

	if (action == MC_PLAYBACK_TOGGLE)
		playback = cd_toggle_playback(mc);
	else
		playback = playback_query_value(action);

	return playback_set_module(mc, "cd", playback);
}

bool musiccast_tuner_preset_switch(musiccast_conn_t *mc, bool next)
{
	bool rv;
	char *response = NULL;
	char *path = NULL;
	const char *dir = next ? "next" : "previous";

	if (!mc)
		return false;

	if (asprintf(&path, "/YamahaExtendedControl/v1/tuner/switchPreset?dir=%s", dir) <= 0)
		return false;

	rv = musiccast_request(mc, path, &response);
	if (rv && response)
		rv = musiccast_response_ok(response);
	free(response);
	free(path);
	return rv;
}

bool musiccast_playback_set(musiccast_conn_t *mc, const char *zone,
			    musiccast_playback_action_e action)
{
	char *input;
	int kind;
	bool rv = false;

	if (!mc || !zone || !playback_query_value(action))
		return false;

	input = musiccast_zone_input_get(mc, zone);
	if (!input)
		return false;

	kind = now_playing_source_kind(input);
	if (kind == 1)
		rv = musiccast_netusb_playback_set(mc, action);
	else if (kind == 3)
		rv = musiccast_cd_playback_set(mc, action);
	else if (kind == 2
		 && (action == MC_PLAYBACK_NEXT || action == MC_PLAYBACK_PREVIOUS))
		rv = musiccast_tuner_preset_switch(mc, action == MC_PLAYBACK_NEXT);

	free(input);
	return rv;
}

void musiccast_now_playing_free(musiccast_now_playing_t *info)
{
	if (!info)
		return;
	free(info->input);
	free(info->power);
	free(info->playback);
	free(info->artist);
	free(info->album);
	free(info->track);
	free(info->detail);
	memset(info, 0, sizeof(*info));
	info->play_time = -60000;
}

bool musiccast_now_playing(musiccast_conn_t *mc, const char *zone,
			   musiccast_now_playing_t *info)
{
	char *status = NULL;
	char *play_raw = NULL;
	json_object *jobj_status = NULL;
	json_object *jobj_play = NULL;
	const char *power;
	int kind;
	bool ok = false;

	if (!mc || !zone || !info)
		return false;

	musiccast_now_playing_free(info);
	info->play_time = -60000;

	status = musiccast_zone_status(mc, zone);
	if (!status)
		return false;

	jobj_status = json_tokener_parse(status);
	free(status);
	if (!jobj_status)
		return false;

	info->power = json_strdup_string_field(jobj_status, "power");
	info->input = json_strdup_string_field(jobj_status, "input");
	if (!info->input)
		info->input = strdup("unknown");

	power = info->power;
	if (power && strcmp(power, "standby") == 0) {
		ok = true;
		goto out;
	}

	kind = now_playing_source_kind(info->input);
	if (kind == 1)
		play_raw = musiccast_netusb_play_info(mc);
	else if (kind == 2)
		play_raw = musiccast_tuner_play_info(mc);
	else if (kind == 3)
		play_raw = musiccast_cd_play_info(mc);

	if (play_raw) {
		jobj_play = json_tokener_parse(play_raw);
		free(play_raw);
		if (jobj_play) {
			if (kind == 1)
				now_playing_apply_netusb(info, jobj_play);
			else if (kind == 2)
				now_playing_apply_tuner(info, jobj_play);
			else if (kind == 3)
				now_playing_apply_cd(info, jobj_play);
		}
	}

	ok = true;
out:
	json_object_put(jobj_play);
	json_object_put(jobj_status);
	return ok;
}
