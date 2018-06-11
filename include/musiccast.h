/*
 * musiccast.h
 *
 *  Created on: Feb 17, 2015
 *      Author: jnevens
 */

#ifndef INCLUDE_MUSICCAST_H_
#define INCLUDE_MUSICCAST_H_

#include <stdbool.h>

typedef struct musiccast_conn_s musiccast_conn_t;

musiccast_conn_t * musiccast_init(const char *hostname);
void musiccast_destroy(musiccast_conn_t *mcc);

#endif /* INCLUDE_MUSICCAST_H_ */
