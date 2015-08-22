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

//#define DNS_NAME_OFFSET_MARK  (3 << 14)

void append_in_dns_name_format(char * dns, char * host);

char * read_name_from_packet(char * read_pointer, int * count);

#endif	/* UTIL_H */

