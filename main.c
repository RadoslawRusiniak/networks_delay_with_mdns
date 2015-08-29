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

#include <assert.h>

#include "err.h"
#include "args.h"
#include "socket_event_manager.h"
#include "mdns.h"
#include "util.h"
#include "delays.h"

int main(int argc, char *argv[])
{
  parse_arguments(argc, argv);
  print_arguments();
  
  struct event_base *base;
  base = event_base_new();
  if (!base) syserr("Error creating base.");
  
  write_MDNS_sock = create_write_mcast_socket(base);
  assert(write_MDNS_sock);
  struct event * write_mcast_event = create_write_mcast_event(base, write_MDNS_sock, SEND_QUERIES_INTERVAL);
  
  read_MDNS_sock = create_read_mcast_socket(base);
  assert(read_MDNS_sock);
  struct event * read_mcast_event = create_read_mcast_event(base, read_MDNS_sock);
  
  icmp_sock = create_icmp_socket(base);
  assert(icmp_sock);
  struct event * icmp_send_event = create_icmp_send_event(base, icmp_sock, DELAYS_MEASUREMENT_INTERVAL);
  struct event * icmp_recv_event = create_icmp_recv_event(base, icmp_sock);
  
  udp_sock = create_udp_socket(base);
  assert(udp_sock);
  struct event * udp_send_event = create_udp_send_query_event(base, udp_sock, DELAYS_MEASUREMENT_INTERVAL);
//  struct event * udp_recv_event = create_udp_recv_event(base, udp_sock);
  
  fprintf(stderr, "Instance data:\n");
  struct sockaddr_in addr = get_ip_address();
  char hostname[256];
  gethostname(hostname, 256);
  fprintf(stderr, " Ip: %s\n", inet_ntoa(addr.sin_addr));
  fprintf(stderr, " Hostname: %s\n", hostname);
  
  fprintf(stderr, "Entering dispatch loop.\n");
  if (event_base_dispatch(base) == -1) syserr("Error running dispatch loop.");
  fprintf(stderr, "Dispatch loop finished.\n");

  event_free(read_mcast_event);
  event_free(write_mcast_event);
  event_free(icmp_send_event);
  event_free(icmp_recv_event);
  event_free(udp_send_event);
//  event_free(udp_recv_event);
  event_base_free(base);
  close_sockets();
  
  return 0;
}
