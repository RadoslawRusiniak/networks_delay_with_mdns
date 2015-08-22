#include "util.h"

struct sockaddr_in get_ip(evutil_socket_t sock) {
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(sock, (struct sockaddr *)&sin, &len) == -1) {
    syserr("getsockname");
  }
  return sin;
}

void append_in_dns_name_format(char * dns, char * host) {
  //dns name format: 3wwwm5imuw3edu2pl0 
  strcat(host, ".");

  int last_appended = 0, i, len = strlen(host);
  for (i = 0; i < len; ++i) {
    if (host[i] == '.') {
      *dns++ = i - last_appended;
      for (; last_appended < i; ++last_appended) {
        *dns++ = host[last_appended];
      }
      ++last_appended;
    }
  }
  *dns++ = '\0';
}

char * read_name_from_packet(char * read_pointer, int * count) {
  char * name;
  int p = 0;
  *count = 1;
  name = (char *) malloc(256);
  name[0] = '\0';

  //read names in 3www5mimuw3edu2pl0 format
  while (*read_pointer != 0) {
    name[p++] = *read_pointer;
    read_pointer = read_pointer + 1;
    *count = *count + 1;
  }

  name[p] = '\0';

  //convert 3www5mimuw3edu2pl0 to www.mimuw.edu.pl
  int i, j;
  for (i = 0; i < (int) strlen((const char*) name); ++i) {
    p = name[i];
    for (j = 0; j < (int) p; ++j) {
      name[i] = name[i + 1];
      i = i + 1;
    }
    name[i] = '.';
  }
  name[i - 1] = '\0'; //remove the last dot
  return name;
}
