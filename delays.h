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
#include "util.h"
#include "socket_event_manager.h"

#define BSIZE 1000
#define ICMP_HEADER_LEN 8
#define ICMP_ID 0x13
#define ICMP_DATA 32116209

struct delay {
  double avg_delay;
  int nr_of_measurements;
  int sum_of_delays;
  long long last_measurements[10];
  int next_index;
  long long time_in_usec_before_send;
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

void send_ping_requests(evutil_socket_t sock, short events, void * arg);

void receive_ping_reply(evutil_socket_t sock, short events, void * arg);


#endif	/* DELAYS_H */

