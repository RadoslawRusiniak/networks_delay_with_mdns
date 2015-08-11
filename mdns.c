
#include "mdns.h"

void send_mcast_data(evutil_socket_t sock, short ev, void *arg) {
  fprintf(stderr, "Sending multicast data\n");
  char buffer[BUFSIZE];
  size_t length;
  strncpy(buffer, "timer_cb data", BUFSIZE);
  length = strnlen(buffer, BUFSIZE);
  if (write(sock, buffer, length) != length) {
    syserr("write");
  }
  fprintf(stderr, "Data sent\n");
}

void read_mcast_data(evutil_socket_t sock, short events, void *arg)
{
  char buf[1024];
  
//    struct evbuffer *input = bufferevent_get_input(bev);
  struct sockaddr_in src_addr;
  socklen_t len;
  ssize_t result;
  result = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)&src_addr, &len);
  fprintf(stderr, "recv %zd from %s:%d\n", result, inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));
  if(result < 0) {
    fatal("Receiving data from multicast.");
  } else if (result == 0) {
    fprintf(stderr, "Connection closed.\n");
    if(close(sock) == -1) syserr("Error closing socket.");
  }
}