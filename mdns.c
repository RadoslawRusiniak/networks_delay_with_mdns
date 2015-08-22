#include "mdns.h"

void send_mdns_query(evutil_socket_t sock, char * qname_arg, uint16_t qtype_arg);

void send_PTR_query(evutil_socket_t sock, short events, void *arg) {
  fprintf(stderr, "Sending PTR question via multicast.\n");
  send_mdns_query(sock, MDNS_SERVICE, T_PTR);
  fprintf(stderr, "PTR question sent via multicast.\n");
}

void send_A_query(evutil_socket_t sock, short events, void *arg) {
  fprintf(stderr, "Sending A question via multicast.\n");
  send_mdns_query(sock, (char *) arg, T_A);
  fprintf(stderr, "A question sent via multicast.\n");
}

void send_PTR_answer(evutil_socket_t sock, short events, void *arg) {
  fprintf(stderr, "Sending PTR answer via multicast.\n");
  char buf[BUF_SIZE];

  struct DNS_HEADER *dns = (struct DNS_HEADER *) &buf;
  memset(dns, 0, sizeof (struct DNS_HEADER));
  dns->qr = 1;
  dns->aa = 1;
  dns->ans_count = htons(1);

  ssize_t frame_len = sizeof (struct DNS_HEADER);
  
  char * qname = &buf[frame_len];
  char service[64] = MDNS_SERVICE;
  append_in_dns_name_format(qname, service);
  
  frame_len += sizeof(MDNS_SERVICE) + 1;
  
  struct RES_RECORD *answer = (struct RES_RECORD *) &buf[frame_len];
  answer->resource = (struct R_DATA *) &buf[frame_len];
  answer->resource->rtype = htons(T_PTR);
  answer->resource->rclass = htons(1);
  answer->resource->ttl = htonl(120);
  
  char hostname[256];
  gethostname(hostname, 256);
  strcat(hostname, ".");
  strcat(hostname, MDNS_SERVICE);
  uint16_t hostname_len = strlen(hostname) + 1;
  
  answer->resource->data_len = htons(hostname_len);
  
  frame_len += sizeof (struct R_DATA);
  
  char * host_pointer = &buf[frame_len];
  append_in_dns_name_format(host_pointer, hostname);
  
  frame_len += hostname_len;
  
  if (write(sock, buf, frame_len) != frame_len) {
    syserr("write");
  }
  fprintf(stderr, "PTR answer sent via multicast.\n");
}

void handle_PTR_query(struct event_base *base, struct QUERY * question) {
  fprintf(stderr, "\tT_PTR query.\n");
  struct event *an_event =
          event_new(base, write_MDNS_sock, EV_WRITE, send_PTR_answer, NULL);
  if (!an_event) syserr("Error creating event.");
  if (event_add(an_event, NULL) == -1) syserr("Error adding an event to a base.");
}

void handle_A_query(struct event_base *base, struct QUERY * question) {
  fprintf(stderr, "\tT_A query.\n");

}

void handle_PTR_answer(struct event_base *base, char * read_pointer, struct RES_RECORD * answer) {
  fprintf(stderr, "\tT_PTR answer.\n");
  int consumed = 0;
  answer->rdata = read_name_from_packet(read_pointer, &consumed);
  read_pointer = read_pointer + ntohs(answer->resource->data_len);
  fprintf(stderr, "\tRdata name: %s.\n", answer->rdata);
  struct event *an_event =
          event_new(base, write_MDNS_sock, EV_WRITE, send_A_query, answer->rdata);
  if (!an_event) syserr("Error creating event.");
  if (event_add(an_event, NULL) == -1) syserr("Error adding an event to a base.");
}

void handle_A_answer(char * read_pointer, struct RES_RECORD * answer) {
  fprintf(stderr, "\tT_A answer.\n");
  int j;
  answer->rdata = malloc(ntohs(answer->resource->data_len));
  for (j = 0; j < ntohs(answer->resource->data_len); ++j) {
    answer->rdata[j] = read_pointer[j];
  }
  answer->rdata[ntohs(answer->resource->data_len)] = '\0';
  read_pointer = read_pointer + ntohs(answer->resource->data_len);
}

void handle_questions(uint16_t nr_of_questions, char * read_pointer, struct event_base *base) {
  struct QUERY questions[10];
  int i, consumed;
  for (i = 0; i < nr_of_questions; ++i) {
    fprintf(stderr, " Question nr %d analyzed:\n", i + 1);
    consumed = 0;
    questions[i].name = read_name_from_packet(read_pointer, &consumed);
    read_pointer += consumed;
    
    fprintf(stderr, "\tName: %s\n", questions[i].name);
    questions[i].ques = (struct QUESTION *) (read_pointer);
    if (ntohs(questions[i].ques->qtype) == T_PTR) {
      if (strcmp(questions[i].name, MDNS_SERVICE) != 0) {
        fprintf(stderr, "Question not from %s, skipping.", MDNS_SERVICE);
        continue;
      }
      handle_PTR_query(base, &questions[i]);
    }
    else if (ntohs(questions[i].ques->qtype) == T_A) {
      if (strstr(questions[i].name, MDNS_PROGRAM) == NULL) {
        fprintf(stderr, "Question not from %s, skipping.", MDNS_SERVICE);
        continue;
      }
      handle_A_query(base, &questions[i]);
    }
    else {
      fprintf(stderr, "Unsupported or unknown resource type\n.\
                        Next resource records in this frame skipped\n");
      return;
    }
  }
}

void handle_answers(uint16_t nr_of_answers, char * read_pointer, struct event_base *base) {
  struct RES_RECORD answers[10];
  int i, consumed;
  for (i = 0; i < nr_of_answers; ++i) {
    fprintf(stderr, " Answer nr %d analyzed:\n", i + 1);
    consumed = 0;
    answers[i].name = read_name_from_packet(read_pointer, &consumed);
    read_pointer += consumed;
    fprintf(stderr, "\tName: %s\n", answers[i].name);

    answers[i].resource = (struct R_DATA *) (read_pointer);
    read_pointer += sizeof (struct R_DATA);

    if (ntohs(answers[i].resource->rtype) == T_PTR) {
      handle_PTR_answer(base, read_pointer, &answers[i]);
    }
    else if (ntohs(answers[i].resource->rtype) == T_A) {
      handle_A_answer(read_pointer, &answers[i]);
    }
    else {
      fprintf(stderr, "Unsupported or unknown resource type\n.\
                        Next resource records in this frame skipped\n");
      return;
    }
  }
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
  fprintf(stderr, "The response contains: ");
  fprintf(stderr, "\n %d Questions.", ntohs(dns->q_count));
  fprintf(stderr, "\n %d Answers.\n", ntohs(dns->ans_count));

  char * read_pointer = &buf[sizeof (struct DNS_HEADER)];
  struct event_base *base = (struct event_base *) arg;

  handle_questions(htons(dns->q_count), read_pointer, base);
  handle_answers(htons(dns->ans_count), read_pointer, base);
}

void send_mdns_query(evutil_socket_t sock, char * qname_arg, uint16_t qtype_arg) {
  char buf[BUF_SIZE];

  struct DNS_HEADER *dns = (struct DNS_HEADER *) &buf;
  memset(dns, 0, sizeof (struct DNS_HEADER));
  dns->q_count = htons(1);
  
  ssize_t frame_len = sizeof (struct DNS_HEADER);
  
  char * qname = &buf[frame_len];
  char service[256];
  strcpy(service, qname_arg);
  append_in_dns_name_format(qname, service);

  frame_len += strlen (service) + 1;
  
  struct QUESTION *qinfo = (struct QUESTION *) &buf[frame_len];
  qinfo->qtype = htons(qtype_arg); //type of the query
  qinfo->qclass = htons(1); //its internet

  frame_len += sizeof (struct QUESTION);
  
  if (write(sock, buf, frame_len) != frame_len) {
    syserr("write");
  }
}