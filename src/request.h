/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2015-2026 Jasper Nevens
 */

#ifndef SRC_REQUEST_H_
#define SRC_REQUEST_H_

bool musiccast_request(musiccast_conn_t *mcc, const char *url, char **response);
bool musiccast_request_host(const char *host, const char *url, char **response);

#endif /* SRC_REQUEST_H_ */
