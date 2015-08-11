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
#include "mdns.h"

#define MCAST_IP    "224.0.0.251"
#define MCAST_PORT  5353

evutil_socket_t create_write_mcast_socket(struct event_base *base) {
  evutil_socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1 ||
          evutil_make_listen_socket_reuseable(sock) ||
          evutil_make_socket_nonblocking(sock)) {
      syserr("Error preparing mcast write socket.");
  }
  
  int ttl = 255;
  if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&ttl, sizeof(ttl)) < 0) {
    syserr("setsockopt multicast ttl");
  }
  int loop = 1;
  if (setsockopt(sock, SOL_IP, IP_MULTICAST_LOOP, (void*)&loop, sizeof(loop)) < 0) {
    syserr("setsockopt loop");
  }

  struct sockaddr_in remote_address;
  remote_address.sin_family = AF_INET;
  remote_address.sin_port = htons(MCAST_PORT);
  if (inet_aton(MCAST_IP, &remote_address.sin_addr) == 0)
    syserr("inet_aton");
  if (connect(sock, (struct sockaddr *)&remote_address, sizeof remote_address) < 0) {
    syserr("connect");
  }
  
  return sock;
}

struct event * create_write_mcast_event(struct event_base *base, 
        evutil_socket_t sock, int queries_interval) {
  
  struct timeval time;
  time.tv_sec = queries_interval;
  time.tv_usec = 0;
  
  struct event *timer_event =
          event_new(base, sock, EV_PERSIST, send_mcast_data, NULL);
  if (!timer_event) {
    syserr("Creating timer event.");
  }
  if (evtimer_add(timer_event, &time)) {
    syserr("Adding timer event to base.");
  }
  return timer_event;
}

evutil_socket_t create_read_mcast_socket(struct event_base *base)
{
  evutil_socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1 ||
          evutil_make_listen_socket_reuseable(sock) ||
          evutil_make_socket_nonblocking(sock)) {
      syserr("Error preparing mcast read socket.");
  }
  
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = inet_addr(MCAST_IP);
  sin.sin_port = htons(MCAST_PORT);
  if (bind(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
    syserr("bind");
  }
  
  struct in_addr mcast_addr;
  mcast_addr.s_addr = inet_addr(MCAST_IP);
  
  struct ip_mreqn mreqn;
  mreqn.imr_multiaddr = mcast_addr;
  mreqn.imr_address.s_addr = htonl(INADDR_ANY);
  mreqn.imr_ifindex = 0;
  if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreqn, sizeof(mreqn)) < 0) {
    syserr("setsockopt IP_ADD_MEMBERSHIP %s", strerror(errno));
  }

  return sock;
}

struct event * create_read_mcast_event(struct event_base *base, evutil_socket_t sock) {
    
  struct event *event = event_new(base, sock, EV_READ|EV_PERSIST, read_mcast_data, NULL);
  if (!event) {
    syserr("Error creating multicast read event");
  }
  if (event_add(event, NULL) == EXIT_FAILURE) {
    syserr("Error adding reading event to a base.");
  }

  return event;
}

int main(int argc, char *argv[]) {
  parse_arguments(argc, argv);
  print_arguments();
  
  struct event_base *base;

  base = event_base_new();
  if (!base) syserr("Error creating base.");

  evutil_socket_t read_sock = create_read_mcast_socket(base);
  assert(read_sock);
  struct event * read_mcast_event = create_read_mcast_event(base, read_sock);
  
  evutil_socket_t write_sock = create_write_mcast_socket(base);
  assert(write_sock);
  struct event * write_mcast_event = create_write_mcast_event(base, write_sock, SEND_QUERIES_INTERVAL);
  
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(write_sock, (struct sockaddr *)&sin, &len) == -1) {
    syserr("getsockname");
  }
  fprintf(stderr, "My ip: %s, port number %d.\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
  
  printf("Entering dispatch loop.\n");
  if (event_base_dispatch(base) == -1) syserr("Error running dispatch loop.");
  printf("Dispatch loop finished.\n");

  event_free(read_mcast_event);
  event_free(write_mcast_event);
  event_base_free(base);

  return 0;
}
