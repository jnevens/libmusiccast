#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>

#include <musiccast.h>

static char doc[] =
	"mcctl detect [options]\n"
	"mcctl -a HOST COMMAND [options]\n"
	"mcctl HOST COMMAND [options]\n"
	"\n"
	"Commands: device-info, features, network-status, func-status, location-info,\n"
	"          status, sound-program-list\n"
	"          (zone controls: --power-set, --mute, --volume)";

static char args_doc[] = "detect | COMMAND";

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
	{ "power-set", -8, "state", 0, "Set zone power" },
	{ "mute", -9, "1", 0, "Mute zone (1/0)" },
	{ "volume", -10, "val", 0, "Set zone value (50/up/down) (step with --step)" },
	{ "step", -11, "step", 0, "Set volume step (default = 1)" },
	{ "zone", 'z', "zone", 0, "Select zone" },
	{ "timeout", 't', "SECONDS", 0, "Discovery wait time (with detect)" },
	{ 0 }
};

struct arguments_s {
	char *hostname;
	bool silent;
	bool verbose;
	bool detect;
	unsigned int detect_timeout_sec;
	bool device_info;
	bool features;
	bool network_status;
	bool func_status;
	bool location_info;
	bool status;
	bool sound_program_list;
	bool power_set;
	bool zone_mute;
	bool zone_mute_state;
	bool zone_volume_set;
	const char *zone_volume;
	const char *zone_volume_step;
	const char *power_state;
	const char *zone;
};

static struct arguments_s arguments = {
	.detect_timeout_sec = 3,
};

static bool arguments_has_action(const struct arguments_s *args)
{
	return args->device_info || args->features || args->network_status
		|| args->func_status || args->location_info || args->status
		|| args->sound_program_list || args->power_set || args->zone_mute
		|| args->zone_volume_set;
}

static bool parse_command(const char *arg, struct arguments_s *args)
{
	if (strcmp(arg, "detect") == 0) {
		args->detect = 1;
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
	case -8:
		args->power_set = 1;
		args->power_state = arg;
		break;
	case -9:
		args->zone_mute = 1;
		args->zone_mute_state = true;
		if (strcmp(arg, "0") == 0)
			args->zone_mute_state = false;
		break;
	case -10:
		args->zone_volume_set = 1;
		args->zone_volume = arg;
		break;
	case -11:
		args->zone_volume_step = arg;
		break;
	case ARGP_KEY_ARG:
		if (parse_command(arg, args))
			break;
		if (!args->hostname) {
			args->hostname = arg;
			break;
		}
		argp_usage(state);
		break;
	case ARGP_KEY_END:
		if (args->detect)
			break;
		if (!args->hostname)
			argp_usage(state);
		if (!arguments_has_action(args))
			argp_usage(state);
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
		char *zone_status;

		if (!args->zone) {
			fprintf(stderr, "Please select zone (--zone=main)\n");
			return 1;
		}
		zone_status = musiccast_zone_status(mcc, args->zone);
		if (!zone_status) {
			fprintf(stderr, "Failed to retrieve zone (%s) status!\n", args->zone);
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
	if (args->power_set) {
		if (strcmp(args->power_state, "on") == 0) {
			if (!musiccast_zone_power_set(mcc, args->zone, MC_POWER_STATE_ON))
				fprintf(stderr, "Failed to zone power (%s) status!\n", args->zone);
		} else if (strcmp(args->power_state, "standby") == 0) {
			if (!musiccast_zone_power_set(mcc, args->zone, MC_POWER_STATE_STANDBY))
				fprintf(stderr, "Failed to zone power (%s) status!\n", args->zone);
		} else if (strcmp(args->power_state, "toggle") == 0) {
			if (!musiccast_zone_power_set(mcc, args->zone, MC_POWER_STATE_TOGGLE))
				fprintf(stderr, "Failed to zone power (%s) status!\n", args->zone);
		} else {
			fprintf(stderr, "Invalid power state!\n");
			return 1;
		}
		return 0;
	}
	if (args->zone_mute) {
		if (!args->zone) {
			fprintf(stderr, "Please select zone (--zone=main)\n");
			return 1;
		}
		if (!musiccast_zone_mute(mcc, args->zone, args->zone_mute_state)) {
			fprintf(stderr, "Failed to mute zone!\n");
			return 1;
		}
		return 0;
	}
	if (args->zone_volume_set) {
		int step_volume = args->zone_volume_step ? atoi(args->zone_volume_step) : 1;
		musiccast_zone_volume_action_e action = MC_ZONE_VOLUME_SET_DIRECT;

		if (strcmp(args->zone_volume, "up") == 0)
			action = MC_ZONE_VOLUME_SET_UP;
		else if (strcmp(args->zone_volume, "down") == 0)
			action = MC_ZONE_VOLUME_SET_DOWN;
		else
			step_volume = atoi(args->zone_volume);

		if (!args->zone) {
			fprintf(stderr, "Please select zone (--zone=main)\n");
			return 1;
		}
		if (!musiccast_zone_volume_set(mcc, args->zone, action, step_volume)) {
			fprintf(stderr, "Failed to set volume of zone (%s)!\n", args->zone);
			return 1;
		}
		return 0;
	}
	return 1;
}

int main(int argc, char *argv[])
{
	int rv;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

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

	musiccast_conn_t *mcc = musiccast_init(arguments.hostname);

	if (mcc == NULL) {
		fprintf(stderr, "Failed to initialize libMusicCast!\n");
		return 1;
	}

	rv = run_device_commands(mcc, &arguments);
	musiccast_destroy(mcc);
	return rv;
}
