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
  
  fprintf(stderr, "Instance data:\n");
  struct sockaddr_in sin = get_ip(write_MDNS_sock);
  fprintf(stderr, "\tMy ip: %s, port number %d\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
  char hostname[256];
  gethostname(hostname, 256);
  fprintf(stderr, "\tMy hostname: %s\n", hostname);
  
  fprintf(stderr, "Entering dispatch loop.\n");
  if (event_base_dispatch(base) == -1) syserr("Error running dispatch loop.");
  fprintf(stderr, "Dispatch loop finished.\n");

  event_free(read_mcast_event);
  event_free(write_mcast_event);
  event_base_free(base);

  return 0;
}
