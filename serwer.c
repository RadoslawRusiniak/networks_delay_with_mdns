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

struct connection_description {
  struct sockaddr_in address;
  evutil_socket_t sock;
  struct event *ev;
};

struct connection_description clients[MAX_CLIENTS];

void init_clients(void)
{
  memset(clients, 0, sizeof(clients));
}

struct connection_description *get_client_slot(void)
{
  int i;
  for(i = 0; i < MAX_CLIENTS; i++)
    if(!clients[i].ev)
      return &clients[i];
  return NULL;
}

void client_socket_cb(evutil_socket_t sock, short ev, void *arg)
{
  struct connection_description *cl = (struct connection_description *)arg;
  char buf[BUF_SIZE+1];

  int r = read(sock, buf, BUF_SIZE);
  if(r <= 0) {
    if(r < 0) {
      fprintf(stderr, "Error (%s) while reading data from %s:%d. Closing connection.\n",
	      strerror(errno), inet_ntoa(cl->address.sin_addr), ntohs(cl->address.sin_port));
    } else {
      fprintf(stderr, "Connection from %s:%d closed.\n",
	      inet_ntoa(cl->address.sin_addr), ntohs(cl->address.sin_port));
    }
    if(event_del(cl->ev) == -1) syserr("Can't delete the event.");
    event_free(cl->ev);
    if(close(sock) == -1) syserr("Error closing socket.");
    cl->ev = NULL;
    return;
  }
  buf[r] = 0;
  printf("[%s:%d] %s\n", inet_ntoa(cl->address.sin_addr), ntohs(cl->address.sin_port), buf);
}

void listener_socket_cb(evutil_socket_t sock, short ev, void *arg)
{
  struct event_base *base = (struct event_base *)arg;

  struct sockaddr_in sin;
  socklen_t addr_size = sizeof(struct sockaddr_in);
  evutil_socket_t connection_socket = accept(sock, (struct sockaddr *)&sin, &addr_size);

  if(connection_socket == -1) syserr("Error accepting connection.");

  struct connection_description *cl = get_client_slot();
  if(!cl) {
    close(connection_socket);
    fprintf(stderr, "Ignoring connection attempt from %s:%d due to lack of space.\n",
	    inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
    return;
  }

  memcpy(&(cl->address), &sin, sizeof(struct sockaddr_in));
  cl->sock = connection_socket;

  struct event *an_event =
    event_new(base, connection_socket, EV_READ|EV_PERSIST, client_socket_cb, (void *)cl);
  if(!an_event) syserr("Error creating event.");
  cl->ev = an_event;
  if(event_add(an_event, NULL) == -1) syserr("Error adding an event to a base.");
}

int main(int argc, char *argv[])
{
  struct event_base *base;

  init_clients();

  base = event_base_new();
  if(!base) syserr("Error creating base.");

  evutil_socket_t listener_socket;
  listener_socket = socket(PF_INET, SOCK_STREAM, 0);
  if(listener_socket == -1 ||
     evutil_make_listen_socket_reuseable(listener_socket) ||
     evutil_make_socket_nonblocking(listener_socket)) {
    syserr("Error preparing socket.");
  }

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(4242);
  if(bind(listener_socket, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
    syserr("bind");
  }

  if(listen(listener_socket, 5) == -1) syserr("listen");

  struct event *listener_socket_event = 
    event_new(base, listener_socket, EV_READ|EV_PERSIST, listener_socket_cb, (void *)base);
  if(!listener_socket_event) syserr("Error creating event for a listener socket.");

  if(event_add(listener_socket_event, NULL) == -1) syserr("Error adding listener_socket event.");

  printf("Entering dispatch loop.\n");
  if(event_base_dispatch(base) == -1) syserr("Error running dispatch loop.");
  printf("Dispatch loop finished.\n");

  event_free(listener_socket_event);
  event_base_free(base);

  return 0;
}
