#include "util.h"

struct sockaddr_in get_ip(evutil_socket_t sock) {
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(sock, (struct sockaddr *)&sin, &len) == -1) {
    syserr("getsockname");
  }
  return sin;
}

void get_hostname(char * ret_name) {
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

  if (info != NULL) {
    memcpy(ret_name, info->ai_canonname, strlen(info->ai_canonname));
  }
  ret_name[strlen(info->ai_canonname)] = '\0';
  freeaddrinfo(info);
}
