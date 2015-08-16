#ifndef MDNS_H
#define	MDNS_H

#include <event2/util.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <inttypes.h>

#include "err.h"
#include "socket_event_manager.h"
#include "util.h"

#define BUF_SIZE 65536

#define T_A   1
#define T_PTR 12

#define MDNS_SERVICE          "_services._dns-sd._udp.local"
#define MDNS_SERVICE_SUFFIX   "._dns-sd._udp.local"

//structures described:
// http://www.zytrax.com/books/dns/ch15/
// http://www.binarytides.com/dns-query-code-in-c-with-winsock/

// structures and basics of handling frames done thanks to:
// http://www.binarytides.com/dns-query-code-in-c-with-linux-sockets/

//fields reversed in groups of 8 bit
//due to difference in the endian-ness of local machine and network
struct DNS_HEADER
{
  uint16_t id;       // identification number
  
  uint8_t rd :1;     // recursion desired
  uint8_t tc :1;     // truncated message
  uint8_t aa :1;     // authoritive answer
  uint8_t opcode :4; // purpose of message
  uint8_t qr :1;     // query/response flag
  
  uint8_t rcode :4;  // response code
  uint8_t z :3;      // reserved for future use
  uint8_t ra :1;     // recursion available
  
  uint16_t q_count;     // number of question entries
  uint16_t ans_count;   // number of answer entries
  uint16_t auth_count;  // number of authority entries
  uint16_t add_count;   // number of resource entries
};

struct QUESTION
{
  uint16_t qtype;
  uint16_t qclass;
};

#pragma pack(push, 1)
struct R_DATA
{
  uint16_t rtype;
  uint16_t rclass;
  uint32_t ttl;
  uint16_t data_len;
};
#pragma pack(pop)
 
struct RES_RECORD
{
  char * name;
  struct R_DATA * resource;
  char * rdata;
};

struct QUERY
{
  char * name;
  struct QUESTION * ques;
};

void send_PTR_query(evutil_socket_t sock, short events, void * arg);

void read_mcast_data(evutil_socket_t sock, short events, void * arg);

#endif	/* MDNS_H */

