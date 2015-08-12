/* 
 * File:   mdns.h
 * Author: radek8
 *
 * Created on 11 sierpień 2015, 21:17
 */

#ifndef MDNS_H
#define	MDNS_H

#include <event2/util.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "err.h"

#define BUFSIZE       65536

#define T_A 1
#define T_PTR 12

#define DNS_DISCOVERY_RR   "_services._dns-sd._udp.local"

//DNS header structure
struct DNS_HEADER
{
  unsigned short id; // identification number
 
  unsigned char qr :1; // query/response flag
  unsigned char opcode :4; // purpose of message
  unsigned char aa :1; // authoritive answer
  unsigned char tc :1; // truncated message
  unsigned char rd :1; // recursion desired
  unsigned char ra :1; // recursion available

  unsigned char z :3; // reserved for future purposes
  unsigned char rcode :4; // response code
 
  unsigned short q_count; // number of question entries
  unsigned short ans_count; // number of answer entries
  unsigned short auth_count; // number of authority entries
  unsigned short add_count; // number of resource entries
};

//Constant sized fields of query structure
struct QUESTION
{
  unsigned short qtype;
  unsigned short qclass;
};

void send_mcast_data(evutil_socket_t sock, short events, void *arg);

void read_mcast_data(evutil_socket_t sock, short events, void *arg);

#endif	/* MDNS_H */

