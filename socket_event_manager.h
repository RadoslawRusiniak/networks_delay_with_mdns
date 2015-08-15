#ifndef SOCKET_EVENT_MANAGER_H
#define	SOCKET_EVENT_MANAGER_H

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
#include "mdns.h"

#define MCAST_IP    "224.0.0.251"
#define MCAST_PORT  5353

evutil_socket_t read_MDNS_sock;
evutil_socket_t write_MDNS_sock;

evutil_socket_t create_write_mcast_socket(struct event_base *base);

struct event * create_write_mcast_event(struct event_base *base, 
        evutil_socket_t sock, int queries_interval);

evutil_socket_t create_read_mcast_socket(struct event_base *base);

struct event * create_read_mcast_event(struct event_base *base, evutil_socket_t sock);


#endif	/* SOCKET_EVENT_MANAGER_H */

