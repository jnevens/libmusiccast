/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2015-2026 Jasper Nevens
 */

#ifndef SRC_MUSICCAST_PRIV_H_
#define SRC_MUSICCAST_PRIV_H_

#include <musiccast.h>

#define MUSICCAST_MAX_ZONES 3
#define MUSICCAST_MAX_ZONE_RANGE_STEPS 10

typedef struct {
	char *id;
	double min;
	double max;
	double step;
} mussiccast_range_step_t;

typedef struct {
	char *id;
	int volume;
	// func list
	// input list
	// equalizer_mode_list
	// link_control_list
	// link_audio_delay_list
	// link_audio_quality_list
	// cursor_list
	// menu_list
	// actual_volume_mode_list
	musiccast_power_state_e power;
	mussiccast_range_step_t range_steps[MUSICCAST_MAX_ZONE_RANGE_STEPS];
} mussiccast_zone_t;

typedef struct {
	char *model_name;
	char *device_id;
	char *system_id;
	size_t zone_num;
	mussiccast_zone_t zone[MUSICCAST_MAX_ZONES];
} mussiccast_device_t;

struct musiccast_conn_s {
	struct in_addr	ip;
	mussiccast_device_t device;
};

#endif /* SRC_MUSICCAST_PRIV_H_ */
