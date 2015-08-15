#include "mdns.h"

void append_in_dns_name_format(char * dns, char * host);

char * read_name_from_packet(char * read_pointer, char * buffer, int * count);

void send_PTR_query(evutil_socket_t sock, short events, void *arg) {
  fprintf(stderr, "Sending PTR question via multicast.\n");
  char buf[BUF_SIZE];

  struct DNS_HEADER *dns = (struct DNS_HEADER *) &buf;
  memset(dns, 0, sizeof (struct DNS_HEADER));
  dns->q_count = htons(1);

  char * qname = &buf[sizeof (struct DNS_HEADER)];
  char service[64] = MDNS_SERVICE;
  append_in_dns_name_format(qname, service);

  struct QUESTION *qinfo = (struct QUESTION *)
          &buf[sizeof (struct DNS_HEADER) + (sizeof (MDNS_SERVICE) + 1)];
  qinfo->qtype = htons(T_PTR); //type of the query
  qinfo->qclass = htons(1); //its internet

  size_t length = sizeof(struct DNS_HEADER) + (sizeof(MDNS_SERVICE) + 1) + sizeof(struct QUESTION);
  if (write(sock, buf, length) != length) {
    syserr("write");
  }
  fprintf(stderr, "PTR question sent via multicast.\n");
}

void send_PTR_answer(evutil_socket_t sock, short events, void *arg) {
  fprintf(stderr, "Sending PTR answer via multicast.\n");
  char buf[BUF_SIZE];
  
  struct DNS_HEADER *dns = (struct DNS_HEADER *) &buf;
  memset(dns, 0, sizeof (struct DNS_HEADER));
  dns->ans_count = htons(1);
  
  ssize_t frame_len = sizeof (struct DNS_HEADER);
  struct RES_RECORD *answer = (struct RES_RECORD *) &buf[frame_len];
  answer->name = &buf[frame_len];
  
  char hostname[256]; char * host_pointer = hostname;
  get_hostname(host_pointer);
  fprintf(stderr, "\thostname: %s\n", host_pointer);
//  char service_suf[64] = MDNS_SERVICE_SUFFIX;
  strcat(host_pointer, MDNS_SERVICE_SUFFIX);
//  hostname[strlen(host_pointer)] = '\0';
  fprintf(stderr, "\thostname with suffix: %s\n", host_pointer);
  append_in_dns_name_format(answer->name, host_pointer);
  
  frame_len += strlen(host_pointer) + 1;
  
  answer->resource = (struct R_DATA *) &buf[frame_len];
  answer->resource->rtype = htons(T_PTR);
  answer->resource->rclass = htons(1);
  answer->resource->ttl = htons(255);
  answer->resource->data_len = 0;
  
  frame_len += sizeof (struct R_DATA);
  
  if (write(sock, buf, frame_len) != frame_len) {
    syserr("write");
  }
  fprintf(stderr, "PTR answer via multicast sent\n");
}

void read_mcast_data(evutil_socket_t sock, short events, void *arg) {
  char buf[BUF_SIZE];
  struct sockaddr_in src_addr;
  socklen_t len;
  ssize_t result;
  result = recvfrom(sock, buf, sizeof (buf), 0, (struct sockaddr*) &src_addr, &len);
  fprintf(stderr, "\nReceived %zd bytes from %s:%d\n",
          result, inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));

  if (result < 0) {
    fatal("Receiving data from multicast.");
  } else if (result == 0) {
    fprintf(stderr, "Connection closed.\n");
    if (close(sock) == -1) syserr("Error closing socket.");
  }

  struct DNS_HEADER * dns = (struct DNS_HEADER *) &buf;
  fprintf(stderr, "The response contains : ");
  fprintf(stderr, "\n %d Questions.", ntohs(dns->q_count));
  fprintf(stderr, "\n %d Answers.\n", ntohs(dns->ans_count));
  
  char * read_pointer = &buf[sizeof(struct DNS_HEADER)];

  int i, j, consumed = 0;
  struct QUERY questions[10];
  struct RES_RECORD answers[10];      
  struct event_base *base = (struct event_base *)arg;
  
  for (i = 0; i < ntohs(dns->q_count); i++) {
    fprintf(stderr, "Question nr %d analyzed:\n", i + 1);
    questions[i].name = read_name_from_packet(read_pointer, buf, &consumed);
    read_pointer += consumed;
    fprintf(stderr, "\tName: %s\n", questions[i].name);
    
    questions[i].ques = (struct QUESTION *) (read_pointer);
    if (ntohs(questions[i].ques->qtype) == T_PTR) {
      fprintf(stderr, "\tT_PTR question.\n");
      struct event *an_event =
        event_new(base, write_MDNS_sock, EV_WRITE, send_PTR_answer, NULL);
      if(!an_event) syserr("Error creating event.");
      if(event_add(an_event, NULL) == -1) syserr("Error adding an event to a base.");
    } else if (ntohs(questions[i].ques->qtype) == T_A) {
      fprintf(stderr, "\tT_A question.\n");
      
    } else {
      fprintf(stderr, "Unsupported or unknown resource type\n.\
                        Next resource records in this frame skipped\n");
      return;
    }
  }

  for (i = 0; i < ntohs(dns->ans_count); i++) {
    fprintf(stderr, "Answer nr %d analyzed:\n", i + 1);
    answers[i].name = read_name_from_packet(read_pointer, buf, &consumed);
    read_pointer += consumed;

    fprintf(stderr, "\tName: %s\n", answers[i].name);
    
    answers[i].resource = (struct R_DATA *) (read_pointer);
    read_pointer += sizeof (struct R_DATA);
    
    if (ntohs(answers[i].resource->rtype) == T_PTR) {
      fprintf(stderr, "\tT_PTR answer.\n");
      
      answers[i].rdata = read_name_from_packet(read_pointer, buf, &consumed);
      read_pointer = read_pointer + consumed;
      
      fprintf(stderr, "\tRdata name: %s.\n", answers[i].rdata);
      
    } else if (ntohs(answers[i].resource->rtype) == T_A) {
      fprintf(stderr, "\tT_A answer.\n");
      answers[i].rdata = malloc(ntohs(answers[i].resource->data_len));

      for (j = 0; j < ntohs(answers[i].resource->data_len); j++) {
        answers[i].rdata[j] = read_pointer[j];
      }

      answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';

      read_pointer = read_pointer + ntohs(answers[i].resource->data_len);
    } else  {
      fprintf(stderr, "Unsupported or unknown resource type\n.\
                        Next resource records in this frame skipped\n");
      return;
    }
  }
}

void append_in_dns_name_format(char * dns, char * host) {
  //dns name format: 3wwwm5imuw3edu2pl0 
  strcat(host, ".");

  int last_appended = 0, i, len = strlen(host);
  fprintf(stderr, "append, len: %d host: %s\n", len, host);
  for (i = 0; i < len; ++i) {
    if (host[i] == '.') {
      fprintf(stderr, "dot %d\n", i - last_appended);
      *dns++ = i - last_appended;
      for (; last_appended < i; ++last_appended) {
        *dns++ = host[last_appended];
        fprintf(stderr, "%c", *(--dns)); dns++;
      }
      ++last_appended;
    }
  }
  *dns++ = '\0';
}

char * read_name_from_packet(char * read_pointer, char * buffer, int * count) {
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