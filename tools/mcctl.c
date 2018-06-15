#include <stdio.h>
#include <stdlib.h>
#include <argp.h>

#include <musiccast.h>

/* Program documentation. */
static char doc[] = "HOSTNAME";

/* A description of the arguments we accept. */
static char args_doc[] = "HOSTNAME";

/* The options we understand. */
static struct argp_option options[] = {
		{ "verbose", 'v', 0, 0, "Produce verbose output" },
		{ "quiet", 'q', 0, 0, "Don't produce any output" },
		{ "silent", 's', 0, OPTION_ALIAS },
//		{ "output", 'o', "FILE", 0, "Output to FILE instead of standard output" },
		{0,0,0,0,"Request system information"},
		{ "device-info", -1, 0, 0, "Show system device info" },
		{ "features", -2, 0, 0, "Show system features" },
		{ "network-status", -3, 0, 0, "Show system network status" },
		{ "func-status", -4, 0, 0, "Show system func status" },
		{ "location-info", -5, 0, 0, "Show system location info" },
		{ "status", -6, 0, 0, "Show system status" },
		{ "sound-program-list", -7, 0, 0, "Show system sound program list" },
		{ 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments_s {
	char *hostname;
	bool silent;
	bool verbose;
	bool device_info;
	bool features;
	bool network_status;
	bool func_status;
	bool location_info;
	bool status;
	bool sound_program_list;
};

struct arguments_s arguments = {
		.silent = 0,
		.verbose = 0,
		.hostname = NULL,
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	/* Get the input argument from argp_parse, which we
	 know is a pointer to our arguments structure. */
	struct arguments_s *arguments = state->input;

	switch (key) {
	case 'q':
	case 's':
		arguments->silent = 1;
		break;
	case 'v':
		arguments->verbose = 1;
		break;
	case -1:
		arguments->device_info = 1;
		break;
	case -2:
		arguments->features = 1;
		break;
	case -3:
		arguments->network_status = 1;
		break;
	case -4:
		arguments->func_status = 1;
		break;
	case -5:
		arguments->location_info = 1;
		break;
	case -6:
		arguments->status = 1;
		break;
	case -7:
		arguments->sound_program_list = 1;
		break;
//	case 'o':
//		arguments->output_file = arg;
//		break;
	case ARGP_KEY_ARG:
		if (state->arg_num >= 1)
			/* Too many arguments. */
			argp_usage(state);
		if (state->arg_num == 0)
			arguments->hostname = arg;
		break;
	case ARGP_KEY_END:
		if (state->arg_num < 1)
			/* Not enough arguments. */
			argp_usage(state);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char *argv[])
{
	/* Parse our arguments; every option seen by parse_opt will
	 be reflected in arguments. */
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	musiccast_conn_t *mcc = musiccast_init(arguments.hostname);
	if (mcc == NULL)
	{
		fprintf(stderr, "Failed to initialize libMusicCast!\n");
		return -1;
	}

	if (arguments.device_info) {
		if (!musiccast_system_device_info(mcc))
			fprintf(stderr, "Failed to retrieve system device info!\n");
	} else if (arguments.features) {
		if (!musiccast_system_features(mcc))
			fprintf(stderr, "Failed to retrieve system features!\n");
	} else if (arguments.network_status) {
		if (!musiccast_system_network_status(mcc))
			fprintf(stderr, "Failed to retrieve system network status!\n");
	} else if (arguments.func_status) {
		if (!musiccast_system_func_status(mcc))
			fprintf(stderr, "Failed to retrieve system func status!\n");
	} else if (arguments.location_info) {
		if (!musiccast_system_location_info(mcc))
			fprintf(stderr, "Failed to retrieve system location info!\n");
	} else if (arguments.status) {
		if (!musiccast_system_status(mcc))
			fprintf(stderr, "Failed to retrieve system status!\n");
	} else if (arguments.sound_program_list) {
		if (!musiccast_system_sound_program_list(mcc))
			fprintf(stderr, "Failed to retrieve system sound program list!\n");
	}

	musiccast_destroy(mcc);
	return 0;
}
