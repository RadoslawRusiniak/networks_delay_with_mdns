#ifndef UTIL_H
#define	UTIL_H

#include <event2/event.h>
#include <event2/util.h>

#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "err.h"

struct sockaddr_in get_ip(evutil_socket_t sock);

char * get_hostname();

#endif	/* UTIL_H */

