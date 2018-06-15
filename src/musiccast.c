/*
 * musiccast.c
 *
 *  Created on: Jun 11, 2018
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <musiccast_priv.h>
#include <request.h>

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
		exit(1);
	}

	// printf("hostname %s resolved: %s\n", hostname, inet_ntoa(mcc->ip));

	return mcc;
}

void musiccast_destroy(musiccast_conn_t *mcc)
{
	free(mcc);
}

bool musiccast_system_device_info(musiccast_conn_t *mc)
{
	bool rv;
	char *response = NULL;
	rv = musiccast_request(mc, "/YamahaExtendedControl/v1/system/getDeviceInfo", &response);
	printf("%s\n", response);
	free(response);
	return rv;
}

bool musiccast_system_features(musiccast_conn_t *mc)
{
	bool rv;
	char *response = NULL;
	rv = musiccast_request(mc, "/YamahaExtendedControl/v1/system/getFeatures", &response);
	printf("%s\n", response);
	free(response);
	return rv;
}

bool musiccast_system_network_status(musiccast_conn_t *mc)
{
	bool rv;
	char *response = NULL;
	rv = musiccast_request(mc, "/YamahaExtendedControl/v1/system/getNetworkStatus", &response);
	printf("%s\n", response);
	free(response);
	return rv;
}

bool musiccast_system_func_status(musiccast_conn_t *mc)
{
	bool rv;
	char *response = NULL;
	rv = musiccast_request(mc, "/YamahaExtendedControl/v1/system/getFuncStatus", &response);
	printf("%s\n", response);
	free(response);
	return rv;
}

bool musiccast_system_location_info(musiccast_conn_t *mc)
{
	bool rv;
	char *response = NULL;
	rv = musiccast_request(mc, "/YamahaExtendedControl/v1/system/getLocationInfo", &response);
	printf("%s\n", response);
	free(response);
	return rv;
}

bool musiccast_system_status(musiccast_conn_t *mc)
{
	bool rv;
	char *response = NULL;
	rv = musiccast_request(mc, "/YamahaExtendedControl/v1/system/getStatus", &response);
	printf("%s\n", response);
	free(response);
	return rv;
}

bool musiccast_system_sound_program_list(musiccast_conn_t *mc)
{
	bool rv;
	char *response = NULL;
	rv = musiccast_request(mc, "/YamahaExtendedControl/v1/system/getSoundProgramList", &response);
	printf("%s\n", response);
	free(response);
	return rv;
}
