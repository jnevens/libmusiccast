#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>

#include <musiccast.h>

const char *argp_program_version = MUSICCAST_VERSION;

static char doc[] =
	"mcctl detect [options]\n"
	"mcctl -a HOST COMMAND [options]\n"
	"mcctl HOST COMMAND [options]\n"
	"\n"
	"Commands: device-info, features, zones, network-status, func-status, location-info,\n"
	"          status, sound-program-list, power [on|standby|toggle],\n"
	"          volume [LEVEL|up|down] (LEVEL within min-max from getFeatures;\n"
	"          use --step with up/down, default 1), mute [on|off|toggle],\n"
	"          preset list | preset play NUM | preset store NUM | preset clear NUM,\n"
	"          help (use -z for zone on zone commands, default main)";

static char args_doc[] = "detect | help | COMMAND";

static struct argp_option options[] = {
	{ "address", 'a', "HOST", 0, "MusicCast device address (IP or hostname)" },
	{ "verbose", 'v', 0, 0, "Produce verbose output" },
	{ "quiet", 'q', 0, 0, "Don't produce any output" },
	{ "silent", 's', 0, OPTION_ALIAS },
	{0, 0, 0, 0, "Request system information (flags or COMMAND)"},
	{ "device-info", -1, 0, 0, "Show system device info" },
	{ "features", -2, 0, 0, "Show system features" },
	{ "network-status", -3, 0, 0, "Show system network status" },
	{ "func-status", -4, 0, 0, "Show system func status" },
	{ "location-info", -5, 0, 0, "Show system location info" },
	{ "status", -6, 0, 0, "Show zone status" },
	{ "sound-program-list", -7, 0, 0, "Show system sound program list" },
	{ "step", -11, "step", 0, "Step for volume up/down (default = 1)" },
	{ "zone", 'z', "zone", 0, "Select zone" },
	{ "timeout", 't', "SECONDS", 0, "Discovery wait time (with detect)" },
	{ 0 }
};

struct arguments_s {
	char *hostname;
	bool silent;
	bool verbose;
	bool detect;
	bool help_cmd;
	unsigned int detect_timeout_sec;
	bool device_info;
	bool features;
	bool network_status;
	bool func_status;
	bool location_info;
	bool status;
	bool sound_program_list;
	bool zones_cmd;
	bool power_cmd;
	bool volume_cmd;
	bool mute_cmd;
	const char *volume_value;
	const char *volume_step;
	const char *power_state;
	const char *mute_state;
	const char *zone;
	bool preset_cmd;
	int preset_action;
	int preset_num;
};

enum {
	PRESET_ACTION_NONE = 0,
	PRESET_ACTION_LIST,
	PRESET_ACTION_PLAY,
	PRESET_ACTION_STORE,
	PRESET_ACTION_CLEAR,
};

static struct arguments_s arguments = {
	.detect_timeout_sec = 3,
};

static bool arguments_has_action(const struct arguments_s *args)
{
	return args->help_cmd || args->device_info || args->features
		|| args->network_status || args->func_status || args->location_info
		|| args->status || args->sound_program_list || args->zones_cmd
		|| args->power_cmd || args->volume_cmd || args->mute_cmd
		|| args->preset_cmd;
}

static bool parse_command(const char *arg, struct arguments_s *args)
{
	if (strcmp(arg, "detect") == 0) {
		args->detect = 1;
		return true;
	}
	if (strcmp(arg, "help") == 0) {
		args->help_cmd = 1;
		return true;
	}
	if (strcmp(arg, "device-info") == 0) {
		args->device_info = 1;
		return true;
	}
	if (strcmp(arg, "features") == 0) {
		args->features = 1;
		return true;
	}
	if (strcmp(arg, "network-status") == 0) {
		args->network_status = 1;
		return true;
	}
	if (strcmp(arg, "func-status") == 0) {
		args->func_status = 1;
		return true;
	}
	if (strcmp(arg, "location-info") == 0) {
		args->location_info = 1;
		return true;
	}
	if (strcmp(arg, "status") == 0) {
		args->status = 1;
		return true;
	}
	if (strcmp(arg, "sound-program-list") == 0) {
		args->sound_program_list = 1;
		return true;
	}
	if (strcmp(arg, "zones") == 0) {
		args->zones_cmd = 1;
		return true;
	}
	if (strcmp(arg, "power") == 0) {
		args->power_cmd = 1;
		return true;
	}
	if (strcmp(arg, "volume") == 0) {
		args->volume_cmd = 1;
		return true;
	}
	if (strcmp(arg, "mute") == 0) {
		args->mute_cmd = 1;
		return true;
	}
	if (strcmp(arg, "preset") == 0) {
		args->preset_cmd = 1;
		return true;
	}
	return false;
}

static bool parse_preset_action(const char *arg, int *action)
{
	if (strcmp(arg, "list") == 0) {
		*action = PRESET_ACTION_LIST;
		return true;
	}
	if (strcmp(arg, "play") == 0 || strcmp(arg, "recall") == 0) {
		*action = PRESET_ACTION_PLAY;
		return true;
	}
	if (strcmp(arg, "store") == 0) {
		*action = PRESET_ACTION_STORE;
		return true;
	}
	if (strcmp(arg, "clear") == 0) {
		*action = PRESET_ACTION_CLEAR;
		return true;
	}
	return false;
}

static bool parse_preset_num(const char *arg, int *num)
{
	char *end;
	long n;

	n = strtol(arg, &end, 10);
	if (end == arg || *end != '\0' || n < 1)
		return false;

	*num = (int)n;
	return true;
}

static const char *command_zone(const struct arguments_s *args)
{
	return args->zone ? args->zone : "main";
}

static bool parse_volume_set_value(const char *value, musiccast_zone_volume_action_e *action,
				   int *level_or_step)
{
	char *end;
	long n;

	if (strcmp(value, "up") == 0) {
		*action = MC_ZONE_VOLUME_SET_UP;
		return true;
	}
	if (strcmp(value, "down") == 0) {
		*action = MC_ZONE_VOLUME_SET_DOWN;
		return true;
	}

	n = strtol(value, &end, 10);
	if (end == value || *end != '\0')
		return false;

	*action = MC_ZONE_VOLUME_SET_DIRECT;
	*level_or_step = (int)n;
	return true;
}

static bool parse_power_set_value(const char *state, musiccast_power_state_e *out)
{
	if (strcmp(state, "on") == 0) {
		*out = MC_POWER_STATE_ON;
		return true;
	}
	if (strcmp(state, "standby") == 0) {
		*out = MC_POWER_STATE_STANDBY;
		return true;
	}
	if (strcmp(state, "toggle") == 0) {
		*out = MC_POWER_STATE_TOGGLE;
		return true;
	}
	return false;
}

static bool parse_mute_set_value(const char *state, bool *toggle, bool *muted)
{
	if (strcmp(state, "on") == 0) {
		*toggle = false;
		*muted = true;
		return true;
	}
	if (strcmp(state, "off") == 0) {
		*toggle = false;
		*muted = false;
		return true;
	}
	if (strcmp(state, "toggle") == 0) {
		*toggle = true;
		return true;
	}
	return false;
}

static size_t detect_field_len(const char *s)
{
	return s ? strlen(s) : 0;
}

static void detect_update_width(size_t *width, const char *value)
{
	size_t len = detect_field_len(value);

	if (len > *width)
		*width = len;
}

static void detect_print_results(const musiccast_discover_list_t *found, bool verbose)
{
	size_t w_addr = strlen("ADDRESS");
	size_t w_host = strlen("NAME");
	size_t w_model = strlen("MODEL");
	size_t w_id = strlen("DEVICE_ID");

	for (size_t i = 0; i < found->count; i++) {
		const musiccast_discovered_device_t *d = &found->devices[i];

		detect_update_width(&w_addr, d->address);
		detect_update_width(&w_host, d->hostname);
		detect_update_width(&w_model, d->model_name);
		detect_update_width(&w_id, d->device_id);
	}

	if (w_addr < 15)
		w_addr = 15;

	printf("%-*s  %-*s  %-*s",
	       (int)w_addr, "ADDRESS",
	       (int)w_host, "NAME",
	       (int)w_model, "MODEL");
	if (verbose)
		printf("  %-*s", (int)w_id, "DEVICE_ID");
	printf("\n");

	for (size_t i = 0; i < found->count; i++) {
		const musiccast_discovered_device_t *d = &found->devices[i];
		const char *addr = d->address ? d->address : "";
		const char *host = d->hostname ? d->hostname : "";
		const char *model = d->model_name ? d->model_name : "";
		const char *id = d->device_id ? d->device_id : "";

		printf("%-*s  %-*s  %-*s",
		       (int)w_addr, addr,
		       (int)w_host, host,
		       (int)w_model, model);
		if (verbose)
			printf("  %-*s", (int)w_id, id);
		printf("\n");
	}
}

static void zones_print_list(const musiccast_zone_list_t *list)
{
	size_t w_zone = strlen("ZONE");

	for (size_t i = 0; i < list->count; i++) {
		const char *id = list->zones[i].id ? list->zones[i].id : "";

		detect_update_width(&w_zone, id);
	}

	printf("%-*s\n", (int)w_zone, "ZONE");

	for (size_t i = 0; i < list->count; i++) {
		const char *id = list->zones[i].id ? list->zones[i].id : "";

		printf("%-*s\n", (int)w_zone, id);
	}
}

static void preset_print_list(const musiccast_netusb_preset_list_t *list)
{
	size_t w_num = strlen("NUM");
	size_t w_input = strlen("INPUT");
	size_t w_text = strlen("TEXT");

	for (size_t i = 0; i < list->count; i++) {
		const musiccast_netusb_preset_t *p = &list->presets[i];
		char num_buf[16];
		const char *input;
		const char *text;

		snprintf(num_buf, sizeof(num_buf), "%d", p->num);
		detect_update_width(&w_num, num_buf);
		if (p->empty) {
			input = "";
			text = "";
		} else {
			input = p->input ? p->input : "";
			text = p->text ? p->text : "";
		}
		detect_update_width(&w_input, input);
		detect_update_width(&w_text, text);
	}

	printf("%-*s  %-*s  %s\n", (int)w_num, "NUM", (int)w_input, "INPUT", "TEXT");

	for (size_t i = 0; i < list->count; i++) {
		const musiccast_netusb_preset_t *p = &list->presets[i];
		char num_buf[16];
		const char *input;
		const char *text;

		snprintf(num_buf, sizeof(num_buf), "%d", p->num);
		if (p->empty) {
			input = "";
			text = "(empty)";
		} else {
			input = p->input ? p->input : "";
			text = p->text ? p->text : "";
		}
		printf("%-*s  %-*s  %s\n", (int)w_num, num_buf, (int)w_input, input, text);
	}
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments_s *args = state->input;

	switch (key) {
	case 'a':
		args->hostname = arg;
		break;
	case 'q':
	case 's':
		args->silent = 1;
		break;
	case 'v':
		args->verbose = 1;
		break;
	case 'z':
		args->zone = arg;
		break;
	case 't':
		args->detect_timeout_sec = (unsigned int)atoi(arg);
		if (args->detect_timeout_sec == 0)
			args->detect_timeout_sec = 3;
		break;
	case -1:
		args->device_info = 1;
		break;
	case -2:
		args->features = 1;
		break;
	case -3:
		args->network_status = 1;
		break;
	case -4:
		args->func_status = 1;
		break;
	case -5:
		args->location_info = 1;
		break;
	case -6:
		args->status = 1;
		break;
	case -7:
		args->sound_program_list = 1;
		break;
	case -11:
		args->volume_step = arg;
		break;
	case ARGP_KEY_ARG:
		if (parse_command(arg, args))
			break;
		if (args->power_cmd && !args->power_state) {
			args->power_state = arg;
			break;
		}
		if (args->volume_cmd && !args->volume_value) {
			args->volume_value = arg;
			break;
		}
		if (args->mute_cmd && !args->mute_state) {
			args->mute_state = arg;
			break;
		}
		if (args->preset_cmd) {
			if (args->preset_action == PRESET_ACTION_NONE) {
				if (!parse_preset_action(arg, &args->preset_action))
					argp_usage(state);
				break;
			}
			if (args->preset_action != PRESET_ACTION_LIST && args->preset_num == 0) {
				if (!parse_preset_num(arg, &args->preset_num))
					argp_usage(state);
				break;
			}
			argp_usage(state);
		}
		if (!args->hostname) {
			args->hostname = arg;
			break;
		}
		argp_usage(state);
		break;
	case ARGP_KEY_END:
		if (args->detect || args->help_cmd)
			break;
		if (!args->hostname)
			argp_usage(state);
		if (!arguments_has_action(args))
			argp_usage(state);
		if (args->preset_cmd) {
			if (args->preset_action == PRESET_ACTION_NONE)
				argp_usage(state);
			if (args->preset_action != PRESET_ACTION_LIST && args->preset_num == 0)
				argp_usage(state);
		}
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

static int run_device_commands(musiccast_conn_t *mcc, struct arguments_s *args)
{
	if (args->device_info) {
		char *device_info = musiccast_system_device_info(mcc);

		if (!device_info) {
			fprintf(stderr, "Failed to retrieve system device info!\n");
			return 1;
		}
		printf("%s", device_info);
		free(device_info);
		return 0;
	}
	if (args->features) {
		char *system_features = musiccast_system_features(mcc);

		if (!system_features) {
			fprintf(stderr, "Failed to retrieve system features!\n");
			return 1;
		}
		printf("%s", system_features);
		free(system_features);
		return 0;
	}
	if (args->network_status) {
		char *response = musiccast_system_network_status(mcc);

		if (!response) {
			fprintf(stderr, "Failed to retrieve system network status!\n");
			return 1;
		}
		printf("%s", response);
		free(response);
		return 0;
	}
	if (args->func_status) {
		char *response = musiccast_system_func_status(mcc);

		if (!response) {
			fprintf(stderr, "Failed to retrieve system func status!\n");
			return 1;
		}
		printf("%s", response);
		free(response);
		return 0;
	}
	if (args->location_info) {
		char *response = musiccast_system_location_info(mcc);

		if (!response) {
			fprintf(stderr, "Failed to retrieve system location info!\n");
			return 1;
		}
		printf("%s", response);
		free(response);
		return 0;
	}
	if (args->status) {
		const char *zone = command_zone(args);
		char *zone_status;

		zone_status = musiccast_zone_status(mcc, zone);
		if (!zone_status) {
			fprintf(stderr, "Failed to retrieve zone (%s) status!\n", zone);
			return 1;
		}
		printf("%s", zone_status);
		free(zone_status);
		return 0;
	}
	if (args->sound_program_list) {
		char *response = musiccast_system_sound_program_list(mcc);

		if (!response) {
			fprintf(stderr, "Failed to retrieve system sound program list!\n");
			return 1;
		}
		printf("%s", response);
		free(response);
		return 0;
	}
	if (args->zones_cmd) {
		musiccast_zone_list_t *list = musiccast_zone_list(mcc);

		if (!list) {
			fprintf(stderr, "Failed to retrieve zone list!\n");
			return 1;
		}
		if (!args->silent)
			zones_print_list(list);
		musiccast_zone_list_free(list);
		return 0;
	}
	if (args->power_cmd) {
		const char *zone = command_zone(args);

		if (!args->power_state) {
			char *power = musiccast_zone_power_get(mcc, zone);

			if (!power) {
				fprintf(stderr, "Failed to retrieve zone (%s) power!\n", zone);
				return 1;
			}
			printf("%s\n", power);
			free(power);
			return 0;
		}

		musiccast_power_state_e state;

		if (!parse_power_set_value(args->power_state, &state)) {
			fprintf(stderr, "Invalid power state (use on, standby, or toggle)\n");
			return 1;
		}
		if (!musiccast_zone_power_set(mcc, zone, state)) {
			fprintf(stderr, "Failed to set zone (%s) power!\n", zone);
			return 1;
		}
		return 0;
	}
	if (args->volume_cmd) {
		const char *zone = command_zone(args);

		if (!args->volume_value) {
			musiccast_zone_volume_info_t info;

			if (!musiccast_zone_volume_query(mcc, zone, &info)) {
				fprintf(stderr, "Failed to retrieve zone (%s) volume!\n", zone);
				return 1;
			}
			if (info.range_valid) {
				printf("%d (%d-%d)\n", info.volume, info.min, info.max);
				if (args->verbose && info.max_volume > 0)
					printf("max_volume setup: %d  step: %d\n",
					       info.max_volume, info.step);
			} else {
				printf("%d\n", info.volume);
			}
			return 0;
		}

		musiccast_zone_volume_action_e action;
		int level_or_step = args->volume_step ? atoi(args->volume_step) : 1;
		musiccast_zone_volume_info_t info;

		if (!parse_volume_set_value(args->volume_value, &action, &level_or_step)) {
			fprintf(stderr, "Invalid volume (use LEVEL, up, or down)\n");
			return 1;
		}
		if (action == MC_ZONE_VOLUME_SET_DIRECT
		    && musiccast_zone_volume_query(mcc, zone, &info)
		    && info.range_valid
		    && (level_or_step < info.min || level_or_step > info.max)) {
			fprintf(stderr, "Volume %d out of range (%d-%d)\n",
				level_or_step, info.min, info.max);
			return 1;
		}
		if (!musiccast_zone_volume_set(mcc, zone, action, level_or_step)) {
			fprintf(stderr, "Failed to set zone (%s) volume!\n", zone);
			return 1;
		}
		return 0;
	}
	if (args->mute_cmd) {
		const char *zone = command_zone(args);
		bool do_toggle = false;
		bool muted = false;

		if (!args->mute_state) {
			if (!musiccast_zone_mute_get(mcc, zone, &muted)) {
				fprintf(stderr, "Failed to retrieve zone (%s) mute!\n", zone);
				return 1;
			}
			printf("%s\n", muted ? "on" : "off");
			return 0;
		}

		if (!parse_mute_set_value(args->mute_state, &do_toggle, &muted)) {
			fprintf(stderr, "Invalid mute state (use on, off, or toggle)\n");
			return 1;
		}
		if (do_toggle) {
			if (!musiccast_zone_mute_get(mcc, zone, &muted)) {
				fprintf(stderr, "Failed to retrieve zone (%s) mute!\n", zone);
				return 1;
			}
			muted = !muted;
		}
		if (!musiccast_zone_mute(mcc, zone, muted)) {
			fprintf(stderr, "Failed to set zone (%s) mute!\n", zone);
			return 1;
		}
		return 0;
	}
	if (args->preset_cmd) {
		const char *zone = command_zone(args);

		if (args->preset_action == PRESET_ACTION_LIST) {
			musiccast_netusb_preset_list_t *list = musiccast_netusb_preset_list(mcc);

			if (!list) {
				fprintf(stderr, "Failed to retrieve Net/USB preset list!\n");
				return 1;
			}
			if (!args->silent)
				preset_print_list(list);
			musiccast_netusb_preset_list_free(list);
			return 0;
		}
		if (args->preset_action == PRESET_ACTION_PLAY) {
			if (!musiccast_netusb_preset_recall(mcc, zone, args->preset_num)) {
				fprintf(stderr, "Failed to recall preset %d on zone (%s)!\n",
					args->preset_num, zone);
				return 1;
			}
			return 0;
		}
		if (args->preset_action == PRESET_ACTION_STORE) {
			if (!musiccast_netusb_preset_store(mcc, args->preset_num)) {
				fprintf(stderr, "Failed to store current content to preset %d!\n",
					args->preset_num);
				return 1;
			}
			return 0;
		}
		if (args->preset_action == PRESET_ACTION_CLEAR) {
			if (!musiccast_netusb_preset_clear(mcc, args->preset_num)) {
				fprintf(stderr, "Failed to clear preset %d!\n", args->preset_num);
				return 1;
			}
			return 0;
		}
	}
	return 1;
}

int main(int argc, char *argv[])
{
	int rv;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	if (arguments.help_cmd) {
		argp_help(&argp, stdout, ARGP_HELP_STD_HELP, argv[0]);
		return 0;
	}

	if (arguments.detect) {
		musiccast_discover_list_t *found;
		unsigned int timeout_ms = arguments.detect_timeout_sec * 1000;

		found = musiccast_discover(timeout_ms);
		if (!found) {
			fprintf(stderr, "Discovery failed.\n");
			return 1;
		}
		if (!arguments.silent)
			detect_print_results(found, arguments.verbose);
		musiccast_discover_list_free(found);
		return 0;
	}

	musiccast_conn_t *mcc = musiccast_connect(arguments.hostname);

	if (mcc == NULL) {
		fprintf(stderr, "Failed to initialize libMusicCast!\n");
		return 1;
	}

	rv = run_device_commands(mcc, &arguments);
	musiccast_destroy(mcc);
	return rv;
}
