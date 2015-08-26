#include "util.h"

struct sockaddr_in get_ip_address() {
  struct ifaddrs * addrs, * tmp;
  struct sockaddr_in addr;
  getifaddrs(&addrs);
  tmp = addrs;

  while (tmp) {
    if (tmp->ifa_addr && (tmp->ifa_addr->sa_family == AF_INET)
        && (tmp->ifa_flags & IFF_UP) && (tmp->ifa_flags & IFF_RUNNING)
        && (strcmp(tmp->ifa_name, "lo") != 0)) {
      memcpy(&addr, tmp->ifa_addr, sizeof(struct sockaddr_in));
      break;
    }
    tmp = tmp->ifa_next;
  }
  freeifaddrs(addrs);
  return addr;
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

unsigned short in_cksum(unsigned short *addr, int len)
{
	int				nleft = len;
	int				sum = 0;
	unsigned short	*w = addr;
	unsigned short	answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

		/* 4mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(unsigned char *)(&answer) = *(unsigned char *)w ;
		sum += answer;
	}

		/* 4add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}
