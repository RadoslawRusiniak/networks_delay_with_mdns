#include "socket_event_manager.h"

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
  
  struct sockaddr_in local_address;
  local_address.sin_family = AF_INET;
  local_address.sin_addr.s_addr = inet_addr(MCAST_IP);
  local_address.sin_port = htons(MCAST_PORT);
  if (bind(sock, (struct sockaddr *)&local_address, sizeof local_address) < 0) {
    syserr("bind");
  }
  
  struct sockaddr_in mcast_address;
  mcast_address.sin_family = AF_INET;
  mcast_address.sin_port = htons(MCAST_PORT);
  if (inet_aton(MCAST_IP, &mcast_address.sin_addr) == 0) {
    syserr("inet_aton");
  }
  if (connect(sock, (struct sockaddr *)&mcast_address, sizeof mcast_address) < 0) {
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
  
  struct sockaddr_in mcast_address;
  mcast_address.sin_family = AF_INET;
  mcast_address.sin_addr.s_addr = inet_addr(MCAST_IP);
  mcast_address.sin_port = htons(MCAST_PORT);
  if (bind(sock, (struct sockaddr*)&mcast_address, sizeof(mcast_address)) < 0) {
    syserr("bind");
  }
  
  struct ip_mreqn mreqn;
  mreqn.imr_multiaddr = mcast_address.sin_addr;
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
