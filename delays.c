#include "delays.h"

int nr_of_peers = 0;

void init_peer(int pos, char * host, struct in_addr addr) {
  strcpy(peers[pos].hostname, host);
  peers[pos].ip_addr = addr;
  peers[pos].icmp_delay.nr_of_measurements = 0;
  peers[pos].icmp_delay.next_index = 0;
  peers[pos].icmp_delay.sum_of_delays = 0;
}

void handle_incoming_address(char * host, struct in_addr addr) {
  int i, already_in = 0;
  for (i = 0; i < nr_of_peers; ++i) {
    if (strcmp(host, peers[i].hostname) == 0) {
      peers[i].ip_addr = addr;
      already_in = 1;
      break;
    }
  }
  if (!already_in) {
    fprintf(stderr, "Adding new peer with hostname: %s\n", host);
    init_peer(nr_of_peers, host, addr);
    nr_of_peers += 1;
  }
  fprintf(stderr, "Incoming address handled, nr_of_peers: %d\n", nr_of_peers);
}

void send_ping_request(evutil_socket_t sock, struct in_addr addr);

void send_ping_requests(evutil_socket_t sock, short events, void * arg) {
  int i;
  for (i = 0; i < nr_of_peers; ++i) {
    fprintf(stderr, "Sending ping request to %d-th peer\n", i + 1);
    send_ping_request(sock, peers[i].ip_addr);
  }
}

void send_ping_request(evutil_socket_t sock, struct in_addr addr) {
  struct sockaddr_in send_addr;
  struct icmp* icmp;
  char send_buffer[BSIZE];
  ssize_t data_len = 0 ,icmp_len = 0, len = 0;
  
  send_addr.sin_family = AF_INET;
  send_addr.sin_addr.s_addr = addr.s_addr;
  send_addr.sin_port = htons(0);

  memset(send_buffer, 0, sizeof(send_buffer));
  icmp = (struct icmp *) send_buffer;
  icmp->icmp_type = ICMP_ECHO;
  icmp->icmp_code = 0;
  icmp->icmp_id = htons(ICMP_ID);
  icmp->icmp_seq = htons(0);
  data_len = snprintf((char*) (send_buffer + ICMP_HEADER_LEN), sizeof(send_buffer)-ICMP_HEADER_LEN, 
          "%d", ICMP_DATA);
  if (data_len < 1)
    syserr("snprinf");
  icmp_len = data_len + ICMP_HEADER_LEN;
  icmp->icmp_cksum = 0;
  icmp->icmp_cksum = in_cksum((unsigned short*) icmp, icmp_len);

  len = sendto(sock, (void*) icmp, icmp_len, 0, (struct sockaddr *) &send_addr, 
               (socklen_t) sizeof(send_addr));
  if (icmp_len != (ssize_t) len)
    syserr("partial / failed write");

  printf(" Wrote %zd bytes\n", len);
}

void receive_ping_reply(evutil_socket_t sock, short event, void * arg) {
  struct sockaddr_in rcv_addr;
  socklen_t rcv_addr_len;
  
  struct ip* ip;
  struct icmp* icmp;
  
  char rcv_buffer[BSIZE];
  
  ssize_t ip_header_len = 0;
  ssize_t data_len = 0;
  ssize_t icmp_len = 0;
  ssize_t len;
  
  memset(rcv_buffer, 0, sizeof(rcv_buffer));
  rcv_addr_len = (socklen_t) sizeof(rcv_addr);
  len = recvfrom(sock, (void*) rcv_buffer, sizeof(rcv_buffer), 0, 
                 (struct sockaddr *) &rcv_addr, &rcv_addr_len);
  
  fprintf(stderr, "\nReceived icmp reply\n");
  printf(" Received %zd bytes from %s\n", len, inet_ntoa(rcv_addr.sin_addr));
  
  // recvfrom returns whole packet (with IP header)
  ip = (struct ip*) rcv_buffer;
  ip_header_len = ip->ip_hl << 2; // IP header len is in 4-byte words
  
  icmp = (struct icmp*) (rcv_buffer + ip_header_len); // ICMP header follows IP
  icmp_len = len - ip_header_len;

  if (icmp_len < ICMP_HEADER_LEN)
    fatal("icmp header len (%d) < ICMP_HEADER_LEN", icmp_len);
  
  if (icmp->icmp_type != ICMP_ECHOREPLY) {
    printf("strange reply type (%d)\n", icmp->icmp_type);
    return;
  }

  if (ntohs(icmp->icmp_id) != ICMP_ID)
    fatal("reply with id %d different from my pid %d", ntohs(icmp->icmp_id), ICMP_ID);

  data_len = len - ip_header_len - ICMP_HEADER_LEN;
  printf(" Correct ICMP echo reply; payload size %zd content %.*s\n", data_len,
         (int) data_len, (rcv_buffer+ip_header_len+ICMP_HEADER_LEN));
}


