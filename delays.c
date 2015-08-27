#include "delays.h"

int nr_of_peers = 0;
uint16_t next_seq_icmp_nr = 0;

void init_peer(int pos, char * host, struct in_addr addr) {
  memset(&peers[pos], 0, sizeof (struct peer));
  strcpy(peers[pos].hostname, host);
  peers[pos].ip_addr = addr;
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

void send_ping_request(evutil_socket_t sock, struct in_addr addr, long long * time_before_send);
void calculate_icmp_delay_for_addr(struct in_addr addr);

void send_ping_requests(evutil_socket_t sock, short events, void * arg) {
  int i;
  for (i = 0; i < nr_of_peers; ++i) {
    fprintf(stderr, "Sending ping request to %d-th peer\n", i + 1);
    send_ping_request(sock, peers[i].ip_addr, &peers[i].icmp_delay.time_in_usec_before_send);
  }
}

void send_ping_request(evutil_socket_t sock, struct in_addr addr, long long * time_before_send) {
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
  icmp->icmp_seq = htons(next_seq_icmp_nr++);
  data_len = snprintf((char*) (send_buffer + ICMP_HEADER_LEN), sizeof(send_buffer)-ICMP_HEADER_LEN, 
          "%d", ICMP_DATA);
  if (data_len < 1)
    syserr("snprinf");
  icmp_len = data_len + ICMP_HEADER_LEN;
  icmp->icmp_cksum = 0;
  icmp->icmp_cksum = in_cksum((unsigned short*) icmp, icmp_len);

  struct timeval tv;
  gettimeofday(&tv, NULL);
  *time_before_send = 1000000 * tv.tv_sec + tv.tv_usec;
  
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
    fatal("reply with id %d different from my ID %d", ntohs(icmp->icmp_id), ICMP_ID);

  data_len = len - ip_header_len - ICMP_HEADER_LEN;
  printf(" Correct ICMP echo reply; payload size %zd content %.*s\n", data_len,
         (int) data_len, (rcv_buffer+ip_header_len+ICMP_HEADER_LEN));
  
  calculate_icmp_delay_for_addr(rcv_addr.sin_addr);
}

void calculate_icmp_delay_for_addr(struct in_addr addr) {
  int i = 0, found = 0;
  for (i = 0; i < nr_of_peers; ++i) {
    if (peers[i].ip_addr.s_addr == addr.s_addr) {
      found = 1;
      struct timeval tv;
      gettimeofday(&tv, NULL);
      long long act_time = 1000000 * tv.tv_sec + tv.tv_usec;
      long long diff = act_time - peers[i].icmp_delay.time_in_usec_before_send;
      peers[i].icmp_delay.nr_of_measurements += 1;
      long long add_to_sum = 
        diff - peers[i].icmp_delay.last_measurements[peers[i].icmp_delay.next_index];
      peers[i].icmp_delay.last_measurements[peers[i].icmp_delay.next_index] = diff;
      peers[i].icmp_delay.next_index = (peers[i].icmp_delay.next_index + 1) % 10;
      peers[i].icmp_delay.sum_of_delays += add_to_sum;
      peers[i].icmp_delay.avg_delay = 
        peers[i].icmp_delay.sum_of_delays * 1.0 / min(10, peers[i].icmp_delay.nr_of_measurements);
      fprintf(stderr, "Delay calculated for icmp, nr of measurements: %d, avg_delay: %f\n", 
              peers[i].icmp_delay.nr_of_measurements, peers[i].icmp_delay.avg_delay);
    }
  }
  if (!found) {
    fprintf(stderr, "Address %u not found in neighbours list\n", addr.s_addr);
  }
}


