
#include "mdns.h"

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

u_char * read_name_from_packet(unsigned char * read_pointer, unsigned char * buffer, int * count) {
  unsigned char * name;
  unsigned int p = 0, jumped = 0, offset;
  int i;

  *count = 1;
  name = (unsigned char * ) malloc(256);
  name[0] = '\0';
  
  //read the names in 3www6google3com format
  while (*read_pointer != 0) {
    if (*read_pointer >= 192) { //offset type name
      offset = (*read_pointer)*256 + *(read_pointer + 1) - OFFSET_MARK;
      read_pointer = buffer + offset - 1;
      jumped = 1;
    } else {
      name[p++] = *read_pointer;
    }

    read_pointer = read_pointer + 1;

    if (jumped == 0) {
      *count = *count + 1;
    }
  }

  name[p] = '\0'; //string complete
  if (jumped == 1) {
    *count = *count + 1;
  }

  //convert 3www6google3com0 to www.google.com
  int j;
  for (i = 0; i < (int) strlen((const char*) name); ++i) {
    p = name[i];
    for (j = 0; j < (int) p; ++j) {
      name[i] = name[i + 1];
      i = i + 1;
    }
    name[i] = '.';
  }
  name[i - 1] = '\0'; //remove the last dot
//  fprintf(stderr, "name from help function: %s\n", name);
  return name;
}

void prepare_dns_question_header(struct DNS_HEADER *dns) {
  memset(dns, 0, sizeof (struct DNS_HEADER));
  dns->q_count = htons(1);
}

void send_PTR_query(evutil_socket_t sock, short events, void *arg) {
  fprintf(stderr, "Sending PTR question via multicast\n");
  char buf[BUFSIZE];

  struct DNS_HEADER *dns = (struct DNS_HEADER *) &buf;
  prepare_dns_question_header(dns);

  char * qname = &buf[sizeof (struct DNS_HEADER)];
  char service[64] = DNS_DISCOVERY_RR;
  append_in_dns_name_format(qname, service);

  struct QUESTION *qinfo = (struct QUESTION *)
          &buf[sizeof (struct DNS_HEADER) + (sizeof (DNS_DISCOVERY_RR) + 1)];
  qinfo->qtype = htons(T_PTR); //type of the query
  qinfo->qclass = htons(1); //its internet

  size_t length = sizeof(struct DNS_HEADER) + (sizeof(DNS_DISCOVERY_RR) + 1) + sizeof(struct QUESTION);
  if (write(sock, buf, length) != length) {
    syserr("write");
  }
  fprintf(stderr, "PTR question via multicast sent\n");
}

void read_mcast_data(evutil_socket_t sock, short events, void *arg) {
  unsigned char buf[BUFSIZE];
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
  
  unsigned char * read_pointer = (unsigned char *) &buf[sizeof(struct DNS_HEADER)];

  int i, j, consumed = 0;
  struct QUERY questions[10];
  struct RES_RECORD answers[10];
  
  for (i = 0; i < ntohs(dns->q_count); i++) {
    fprintf(stderr, "Question nr %d analyzed:\n", i + 1);
    questions[i].name = read_name_from_packet(read_pointer, buf, &consumed);
    fprintf(stderr, "\tName: %s\n", answers[i].name);
    read_pointer += consumed;
    
    questions[i].ques = (struct QUESTION *) (read_pointer);
    if (ntohs(questions[i].ques->qtype) == T_PTR) {
      fprintf(stderr, "\tT_PTR question.\n");
      
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