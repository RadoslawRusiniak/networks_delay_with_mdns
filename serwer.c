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

//#define BSIZE       256

#define MAX_LINE    16384
#define MCAST_IP    "224.0.0.251"
#define MCAST_PORT  5353

void read_mcast_data(evutil_socket_t fd, short events, void *arg);

struct read_mcast_state {
  char buffer[MAX_LINE];
  struct event * read_mcast_event;
};

void timer_cb(evutil_socket_t sock, short ev, void *arg) {
  fprintf(stdout, "Tick\n");  
//  char buffer[BSIZE];
//  size_t length;
//  strncpy(buffer, "Send data\n", BSIZE);
//  length = strnlen(buffer, BSIZE);
////  fprintf(stdout, "%d\n", (int)length);  
////  if (write(sock, buffer, length) != length)
////    syserr("write");
//  
////    (void) printf("sending to socket: %s\n", argv[i]);
//    int sflags = 0;
////    int rcva_len = (socklen_t) sizeof(my_address);
//    int snd_len = sendto(sock, buffer, length, sflags,
//        //(struct sockaddr *) &my_address, rcva_len);
//        0, 0);
//    if (snd_len != (ssize_t) length) {
//      syserr("partial / failed write");
//    }
}

struct event * add_timer_event(struct event_base *base, evutil_socket_t sock) {
  
  struct timeval time;
  time.tv_sec = 2;
  time.tv_usec = 0;
  
  struct event *timer_event =
          event_new(base, sock, EV_PERSIST, timer_cb, NULL);
  if (!timer_event) syserr("Creating timer event.");
  if (evtimer_add(timer_event, &time)) syserr("Adding timer event to base.");
  return timer_event;
}

struct read_mcast_state * alloc_read_mcast_state(struct event_base *base, evutil_socket_t sock)
{
  struct read_mcast_state *state = malloc(sizeof(struct read_mcast_state));
  if (!state) {
    syserr("Error allocating memory for multicast read structure");
  }
  state->read_mcast_event = event_new(base, sock, EV_READ|EV_PERSIST, read_mcast_data, state);
  if (!state->read_mcast_event) {
    free(state);
    syserr("Error creating multicast read event");
  }
  if (event_add(state->read_mcast_event, NULL) == EXIT_FAILURE) {
    free(state);
    syserr("Error adding reading event to a base.");
  }
  return state;
}

void free_read_mcast_state(struct read_mcast_state *state)
{
  if (event_del(state->read_mcast_event) == -1) syserr("Can't delete the read_mcast_event.");
  event_free(state->read_mcast_event);
  free(state);
}

struct read_mcast_state * create_mcast_socket_and_state(struct event_base *base, evutil_socket_t sock)
{
//  evutil_socket_t sock = *fd;
//  sock = socket(AF_INET, SOCK_DGRAM, 0);
//  if (sock == -1 ||
//          evutil_make_listen_socket_reuseable(sock) ||
//          evutil_make_socket_nonblocking(sock)) {
//      syserr("Error preparing mcast read socket.");
//  }
  
  int loop = 1;
  if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
    syserr("setsockopt IP_MULTICAST_LOOP");
  }
  
  u_char ttl = 255;
  if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) == -1) {
    syserr("setsockopt IP_MULTICAST_TTL");
  }
  
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = inet_addr(MCAST_IP); //TODO maybe 0 could be here
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
  
  
  struct read_mcast_state *state = alloc_read_mcast_state(base, sock);
  return state;
}

void read_mcast_data(evutil_socket_t fd, short events, void *arg)
{
  struct read_mcast_state *state = arg;
  char buf[1024];
  struct sockaddr_in src_addr;
  socklen_t len;
  ssize_t result;
  result = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&src_addr, &len);
  fprintf(stderr, "recv %zd from %s:%d\n", result, inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));
  if (result == 0) {
    free_read_mcast_state(state);
  } else if (result < 0) {
    if (errno == EAGAIN) // XXXX use evutil macro
    {
      return;
    }
    fprintf(stderr, "Error while receiving data from multicast, freeing reading state...");
    free_read_mcast_state(state);
    fprintf(stderr, "Reading state freeing done.");
    fatal("Receiving data from multicast.");
  }
}

int main(int argc, char *argv[]) {
  struct event_base *base;

  base = event_base_new();
  if (!base) syserr("Error creating base.");

  evutil_socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1 ||
          evutil_make_listen_socket_reuseable(sock) ||
          evutil_make_socket_nonblocking(sock)) {
      syserr("Error preparing mcast read socket.");
  }
  assert(sock);
  struct read_mcast_state *read_mcast_state = create_mcast_socket_and_state(base, sock);
  struct event *timer_event = add_timer_event(base, sock);

  printf("Entering dispatch loop.\n");
  if (event_base_dispatch(base) == -1) syserr("Error running dispatch loop.");
  printf("Dispatch loop finished.\n");

  free_read_mcast_state(read_mcast_state);
  event_free(timer_event);
//  event_free(listener_socket_event);
  event_base_free(base);

  return 0;
}
