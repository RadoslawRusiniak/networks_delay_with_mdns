/* 
 * File:   mdns.h
 * Author: radek8
 *
 * Created on 11 sierpień 2015, 21:17
 */

#ifndef MDNS_H
#define	MDNS_H

#include <event2/event.h>
#include <event2/util.h>

#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "err.h"

#define BUFSIZE       1024

void send_mcast_data(evutil_socket_t sock, short ev, void *arg);

void read_mcast_data(evutil_socket_t sock, short events, void *arg);


#endif	/* MDNS_H */

