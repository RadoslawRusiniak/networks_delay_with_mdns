#include "util.h"

struct sockaddr_in get_ip(evutil_socket_t sock) {
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(sock, (struct sockaddr *)&sin, &len) == -1) {
    syserr("getsockname");
  }
  return sin;
}

char * get_hostname() {
  struct addrinfo hints, *info, *p;
  int gai_result;

  char hostname[1024];
  hostname[1023] = '\0';
  gethostname(hostname, 1023);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; /*either IPV4 or IPV6*/
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_CANONNAME;

  if ((gai_result = getaddrinfo(hostname, "http", &hints, &info)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_result));
      exit(1);
  }

  char *ret_name = malloc(256);
  for (p = info; p != NULL; p = p->ai_next) {
    memcpy(ret_name, p->ai_canonname, strlen(p->ai_canonname));
  }
  freeaddrinfo(info);
  
  return ret_name;
}
