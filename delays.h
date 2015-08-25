#ifndef DELAYS_H
#define	DELAYS_H

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>

#include "err.h"
#include "socket_event_manager.h"

#define ICMP_DATA 0x32116209

struct delay {
  int avg;
  int min;
  int max;
};

struct peer {
  char hostname[256];
  struct in_addr ip_addr;
  struct delay icmp_delay;
  struct delay udp_delay;
  struct delay tcp_delay;
};

struct peer peers[100];

void handle_incoming_address(char * hostname, struct in_addr addr);

#endif	/* DELAYS_H */

