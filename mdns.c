
#include "mdns.h"

void append_in_dns_name_format(unsigned char* dns, unsigned char* host)
{
  //dns name format: 3wwwm5imuw3edu2pl0 
  strcat((char*)host, ".");
  
  int last_appended = 0 , i, len = strlen((char*)host);
  for (i = 0; i < len; ++i) {
    if (host[i] == '.') {
      *dns++ = i - last_appended;
      for( ; last_appended < i; ++last_appended) {
        *dns++ = host[last_appended];
      }
      ++last_appended;
    }
  }
  *dns++='\0';
}

void prepare_dns_question_header(struct DNS_HEADER *dns)
{
  memset(dns, 0, sizeof(struct DNS_HEADER));
  dns->q_count = htons(1);
}

void send_mcast_data(evutil_socket_t sock, short events, void *arg)
{
  fprintf(stderr, "Sending PTR question via multicast\n");
  char buf[BUFSIZE];
  
  struct DNS_HEADER *dns = (struct DNS_HEADER *)&buf;
  prepare_dns_question_header(dns);
  
  unsigned char * qname = (unsigned char *)&buf[sizeof(struct DNS_HEADER)];
  unsigned char service[64] = DNS_DISCOVERY_RR;
  append_in_dns_name_format(qname, service);
  
  struct QUESTION *qinfo = (struct QUESTION *)
      &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)];
  qinfo->qtype = htons(T_PTR); //type of the query
  qinfo->qclass = htons(1); //its internet
  
  size_t length = sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION);
  if (write(sock, buf, length) != length) {
    syserr("write");
  }
  fprintf(stderr, "PTR question via multicast sent\n");
}

void read_mcast_data(evutil_socket_t sock, short events, void *arg)
{
  char buf[BUFSIZE];
  struct sockaddr_in src_addr;
  socklen_t len;
  ssize_t result;
  result = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)&src_addr, &len);
  fprintf(stderr, "\nReceived %zd bytes from %s:%d\n", 
          result, inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));
  
  if(result < 0) {
    fatal("Receiving data from multicast.");
  } else if (result == 0) {
    fprintf(stderr, "Connection closed.\n");
    if(close(sock) == -1) syserr("Error closing socket.");
  }
  
  struct DNS_HEADER * dns = NULL;
  dns = (struct DNS_HEADER *) &buf;

  printf("The response contains : ");
  printf("\n %d Questions.", ntohs(dns->q_count));
  printf("\n %d Answers.", ntohs(dns->ans_count));
  printf("\n %d Authoritative Servers.", ntohs(dns->auth_count));
  printf("\n %d Additional records.\n", ntohs(dns->add_count));
}