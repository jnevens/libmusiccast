/*
 * request.h
 *
 *  Created on: Jun 12, 2018
 *      Author: jnevens
 */

#ifndef SRC_REQUEST_H_
#define SRC_REQUEST_H_

bool musiccast_request(musiccast_conn_t *mcc, const char *url, char **response);

#endif /* SRC_REQUEST_H_ */
