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

#define MAX_CLIENTS 16
#define BUF_SIZE 16

#define MAX_LINE    16384
#define MCAST_IP    "224.0.0.251"
#define MCAST_PORT  5353

void read_mcast_data(evutil_socket_t fd, short events, void *arg);

struct read_mcast_state {
  char buffer[MAX_LINE];
  struct event * read_mcast_event;
};

struct connection_description {
  struct sockaddr_in address;
  evutil_socket_t sock;
  struct event * ev;
};

struct connection_description clients[MAX_CLIENTS];

void init_clients(void) {
  memset(clients, 0, sizeof (clients));
}

struct connection_description *get_client_slot(void) {
  int i;
  for (i = 0; i < MAX_CLIENTS; i++)
      if (!clients[i].ev)
          return &clients[i];
  return NULL;
}

void timer_cb(evutil_socket_t sock, short ev, void *arg) {
  fprintf(stdout, "Tick\n");
}

struct event * add_timer_event(struct event_base *base) {
  
  struct timeval time;
  time.tv_sec = 2;
  time.tv_usec = 0;
  
  struct event *timer_event =
          event_new(base, 0, EV_PERSIST, timer_cb, NULL);
  if (!timer_event) syserr("Creating timer event.");
  if (evtimer_add(timer_event, &time)) syserr("Adding timer event to base.");
  return timer_event;
}

struct read_mcast_state * alloc_read_mcast_state(struct event_base *base, evutil_socket_t fd)
{
  struct read_mcast_state *state = malloc(sizeof(struct read_mcast_state));
  if (!state) {
    syserr("Error allocating memory for multicast read structure");
  }
  state->read_mcast_event = event_new(base, fd, EV_READ|EV_PERSIST, read_mcast_data, state);
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

struct read_mcast_state * create_mcast_socket_and_state(struct event_base *base)
{
  evutil_socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1 ||
          evutil_make_listen_socket_reuseable(sock) ||
          evutil_make_socket_nonblocking(sock)) {
      syserr("Error preparing mcast read socket.");
  }
  
  int loop = 1;
  if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
    syserr("setsockopt IP_MULTICAST_LOOP");
  }
  
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
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

void client_socket_cb(evutil_socket_t sock, short ev, void *arg) {
  struct connection_description *cl = (struct connection_description *) arg;
  char buf[BUF_SIZE + 1];

  int r = read(sock, buf, BUF_SIZE);
  if (r <= 0) {
      if (r < 0) {
          fprintf(stderr, "Error (%s) while reading data from %s:%d. Closing connection.\n",
                  strerror(errno), inet_ntoa(cl->address.sin_addr), ntohs(cl->address.sin_port));
      } else {
          fprintf(stderr, "Connection from %s:%d closed.\n",
                  inet_ntoa(cl->address.sin_addr), ntohs(cl->address.sin_port));
      }
      if (event_del(cl->ev) == -1) syserr("Can't delete the event.");
      event_free(cl->ev);
      if (close(sock) == -1) syserr("Error closing socket.");
      cl->ev = NULL;
      return;
  }
  buf[r] = 0;
  printf("[%s:%d] %s\n", inet_ntoa(cl->address.sin_addr), ntohs(cl->address.sin_port), buf);
}

void listener_socket_cb(evutil_socket_t sock, short ev, void *arg) {
  struct event_base *base = (struct event_base *) arg;

  struct sockaddr_in sin;
  socklen_t addr_size = sizeof (struct sockaddr_in);
  evutil_socket_t connection_socket = accept(sock, (struct sockaddr *) &sin, &addr_size);

  if (connection_socket == -1) syserr("Error accepting connection.");

  struct connection_description *cl = get_client_slot();
  if (!cl) {
      close(connection_socket);
      fprintf(stderr, "Ignoring connection attempt from %s:%d due to lack of space.\n",
              inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
      return;
  }

  memcpy(&(cl->address), &sin, sizeof (struct sockaddr_in));
  cl->sock = connection_socket;

  struct event *an_event =
          event_new(base, connection_socket, EV_READ | EV_PERSIST, client_socket_cb, (void *) cl);
  if (!an_event) syserr("Error creating event.");
  cl->ev = an_event;
  if (event_add(an_event, NULL) == -1) syserr("Error adding an event to a base.");
}

int main(int argc, char *argv[]) {
  struct event_base *base;

  init_clients();

  base = event_base_new();
  if (!base) syserr("Error creating base.");

  evutil_socket_t listener_socket;
  listener_socket = socket(PF_INET, SOCK_STREAM, 0);
  if (listener_socket == -1 ||
          evutil_make_listen_socket_reuseable(listener_socket) ||
          evutil_make_socket_nonblocking(listener_socket)) {
      syserr("Error preparing socket.");
  }

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(4242);
  if (bind(listener_socket, (struct sockaddr *) &sin, sizeof (sin)) == -1) {
      syserr("bind");
  }

  if (listen(listener_socket, 5) == -1) syserr("listen");

  struct event *listener_socket_event =
          event_new(base, listener_socket, EV_READ | EV_PERSIST, listener_socket_cb, (void *) base);
  if (!listener_socket_event) syserr("Error creating event for a listener socket.");

  if (event_add(listener_socket_event, NULL) == -1) syserr("Error adding listener_socket event.");

  struct event *timer_event = add_timer_event(base);
  struct read_mcast_state *read_mcast_state = create_mcast_socket_and_state(base);

  printf("Entering dispatch loop.\n");
  if (event_base_dispatch(base) == -1) syserr("Error running dispatch loop.");
  printf("Dispatch loop finished.\n");

  free_read_mcast_state(read_mcast_state);
  event_free(timer_event);
  event_free(listener_socket_event);
  event_base_free(base);

  return 0;
}
