/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2015-2026 Jasper Nevens
 * discover.c — locate Yamaha MusicCast devices via SSDP (UPnP)
 */
#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <json-c/json.h>

#include <musiccast.h>
#include <request.h>

#define MUSICCAST_SSDP_MCAST_ADDR	"239.255.255.250"
#define MUSICCAST_SSDP_PORT		1900
#define MUSICCAST_DISCOVER_DEFAULT_TIMEOUT_MS	3000
#define MUSICCAST_DISCOVER_MAX_CANDIDATES		64
#define MUSICCAST_SSDP_RECV_BUF			8192

static bool timeval_after(const struct timeval *a, const struct timeval *b)
{
	if (a->tv_sec != b->tv_sec)
		return a->tv_sec > b->tv_sec;
	return a->tv_usec > b->tv_usec;
}

static void timeval_sub(const struct timeval *end, const struct timeval *now,
			struct timeval *out)
{
	out->tv_sec = end->tv_sec - now->tv_sec;
	out->tv_usec = end->tv_usec - now->tv_usec;
	if (out->tv_usec < 0) {
		out->tv_sec--;
		out->tv_usec += 1000000;
	}
}

static const char musiccast_ssdp_msearch[] =
	"M-SEARCH * HTTP/1.1\r\n"
	"HOST: 239.255.255.250:1900\r\n"
	"MAN: \"ssdp:discover\"\r\n"
	"MX: 2\r\n"
	"ST: urn:schemas-upnp-org:device:MediaRenderer:1\r\n"
	"\r\n";

static int strcasecmp_header(const char *line, const char *name)
{
	size_t n = strlen(name);

	if (strncasecmp(line, name, n) != 0)
		return 0;
	if (line[n] != ':' && line[n] != ' ')
		return 0;
	return 1;
}

static const char *header_value(const char *line)
{
	const char *p = strchr(line, ':');

	if (!p)
		return NULL;
	p++;
	while (*p == ' ' || *p == '\t')
		p++;
	return p;
}

static bool parse_location_host(const char *location, char *host, size_t hostlen)
{
	const char *p = location;

	if (strncmp(p, "http://", 7) == 0)
		p += 7;
	else if (strncmp(p, "https://", 8) == 0)
		p += 8;

	if (*p == '\0')
		return false;

	size_t i = 0;
	while (p[i] && p[i] != ':' && p[i] != '/' && i + 1 < hostlen) {
		host[i] = p[i];
		i++;
	}
	host[i] = '\0';
	return host[0] != '\0';
}

static bool candidate_add(char candidates[][INET_ADDRSTRLEN], size_t *count,
			  const char *host)
{
	struct in_addr addr;

	if (inet_pton(AF_INET, host, &addr) != 1)
		return false;

	for (size_t i = 0; i < *count; i++) {
		if (strcmp(candidates[i], host) == 0)
			return true;
	}

	if (*count >= MUSICCAST_DISCOVER_MAX_CANDIDATES)
		return false;

	strncpy(candidates[*count], host, INET_ADDRSTRLEN);
	candidates[*count][INET_ADDRSTRLEN - 1] = '\0';
	(*count)++;
	return true;
}

static void ssdp_collect_candidates(int sock, unsigned int timeout_ms,
				    char candidates[][INET_ADDRSTRLEN],
				    size_t *count)
{
	struct timeval end;
	struct timeval now;

	gettimeofday(&end, NULL);
	end.tv_usec += (timeout_ms % 1000) * 1000;
	end.tv_sec += timeout_ms / 1000 + end.tv_usec / 1000000;
	end.tv_usec %= 1000000;

	for (int i = 0; i < 3; i++) {
		struct sockaddr_in mcast = { 0 };
		mcast.sin_family = AF_INET;
		mcast.sin_port = htons(MUSICCAST_SSDP_PORT);
		inet_pton(AF_INET, MUSICCAST_SSDP_MCAST_ADDR, &mcast.sin_addr);
		sendto(sock, musiccast_ssdp_msearch, sizeof(musiccast_ssdp_msearch) - 1, 0,
		       (struct sockaddr *)&mcast, sizeof(mcast));
	}

	while (1) {
		fd_set fds;
		struct timeval tv;
		char buf[MUSICCAST_SSDP_RECV_BUF];
		struct sockaddr_in peer;
		socklen_t peerlen = sizeof(peer);
		ssize_t n;

		gettimeofday(&now, NULL);
		if (timeval_after(&now, &end))
			break;

		timeval_sub(&end, &now, &tv);
		FD_ZERO(&fds);
		FD_SET(sock, &fds);
		if (select(sock + 1, &fds, NULL, NULL, &tv) <= 0)
			break;

		n = recvfrom(sock, buf, sizeof(buf) - 1, 0,
			     (struct sockaddr *)&peer, &peerlen);
		if (n <= 0)
			continue;

		char peer_ip[INET_ADDRSTRLEN];

		buf[n] = '\0';
		inet_ntop(AF_INET, &peer.sin_addr, peer_ip, sizeof(peer_ip));
		candidate_add(candidates, count, peer_ip);

		char *line = buf;
		while (line && *line) {
			char *next = strchr(line, '\n');
			if (next)
				*next++ = '\0';
			if (line[strlen(line) - 1] == '\r')
				line[strlen(line) - 1] = '\0';

			if (strcasecmp_header(line, "LOCATION")) {
				const char *location = header_value(line);
				char host[INET_ADDRSTRLEN];

				if (location && parse_location_host(location, host, sizeof(host)))
					candidate_add(candidates, count, host);
			}
			line = next;
		}
	}
}

static char *json_strdup_string_field(json_object *jobj, const char *key)
{
	json_object *field = json_object_object_get(jobj, key);

	if (!field || !json_object_is_type(field, json_type_string))
		return NULL;
	return strdup(json_object_get_string(field));
}

static char *address_reverse_hostname(const char *address)
{
	struct sockaddr_in sa = { .sin_family = AF_INET };
	char host[NI_MAXHOST];

	if (inet_pton(AF_INET, address, &sa.sin_addr) != 1)
		return NULL;
	if (getnameinfo((struct sockaddr *)&sa, sizeof(sa), host, sizeof(host),
			NULL, 0, 0) != 0)
		return NULL;
	if (strcmp(host, address) == 0)
		return NULL;
	return strdup(host);
}

static bool musiccast_probe_host(const char *host, char **model_name, char **device_id,
				 char **hostname)
{
	char *response = NULL;
	json_object *jobj;
	json_object *jmodel;

	*model_name = NULL;
	*device_id = NULL;
	*hostname = NULL;

	if (!musiccast_request_host(host,
				    "/YamahaExtendedControl/v1/system/getDeviceInfo",
				    &response))
		return false;

	jobj = json_tokener_parse(response);
	free(response);
	if (!jobj)
		return false;

	jmodel = json_object_object_get(jobj, "model_name");
	if (!jmodel || !json_object_is_type(jmodel, json_type_string)) {
		json_object_put(jobj);
		return false;
	}

	*model_name = strdup(json_object_get_string(jmodel));
	*device_id = json_strdup_string_field(jobj, "device_id");
	json_object_put(jobj);

	response = NULL;
	if (musiccast_request_host(host,
				   "/YamahaExtendedControl/v1/system/getNetworkStatus",
				   &response)) {
		jobj = json_tokener_parse(response);
		free(response);
		if (jobj) {
			*hostname = json_strdup_string_field(jobj, "network_name");
			json_object_put(jobj);
		}
	}

	if (!*hostname)
		*hostname = address_reverse_hostname(host);

	return *model_name != NULL;
}

musiccast_discover_list_t *musiccast_discover(unsigned int timeout_ms)
{
	musiccast_discover_list_t *list;
	char candidates[MUSICCAST_DISCOVER_MAX_CANDIDATES][INET_ADDRSTRLEN];
	size_t candidate_count = 0;
	int sock;
	struct sockaddr_in bind_addr = { 0 };

	if (timeout_ms == 0)
		timeout_ms = MUSICCAST_DISCOVER_DEFAULT_TIMEOUT_MS;

	list = calloc(1, sizeof(*list));
	if (!list)
		return NULL;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
		goto fail;

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
		goto fail_sock;

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(0);
	if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0)
		goto fail_sock;

	ssdp_collect_candidates(sock, timeout_ms, candidates, &candidate_count);
	close(sock);

	for (size_t i = 0; i < candidate_count; i++) {
		char *model_name = NULL;
		char *device_id = NULL;
		char *hostname = NULL;
		musiccast_discovered_device_t *dev;

		if (!musiccast_probe_host(candidates[i], &model_name, &device_id, &hostname))
			continue;

		list->devices = realloc(list->devices,
					(list->count + 1) * sizeof(*list->devices));
		if (!list->devices) {
			free(model_name);
			free(device_id);
			free(hostname);
			goto fail;
		}

		dev = &list->devices[list->count++];
		dev->address = strdup(candidates[i]);
		dev->hostname = hostname;
		dev->model_name = model_name;
		dev->device_id = device_id;
		if (!dev->address) {
			free(model_name);
			free(device_id);
			free(hostname);
			goto fail;
		}
	}

	return list;

fail_sock:
	if (sock >= 0)
		close(sock);
fail:
	musiccast_discover_list_free(list);
	return NULL;
}

void musiccast_discover_list_free(musiccast_discover_list_t *list)
{
	if (!list)
		return;

	for (size_t i = 0; i < list->count; i++) {
		free(list->devices[i].address);
		free(list->devices[i].hostname);
		free(list->devices[i].model_name);
		free(list->devices[i].device_id);
	}
	free(list->devices);
	free(list);
}
