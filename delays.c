#include "delays.h"

int nr_of_peers = 0;

void handle_incoming_address(char * host, struct in_addr addr) {
  int i = 0, already_in = 0;
  for (; i < nr_of_peers; ++i) {
    if (strcmp(host, peers[i].hostname) == 0) {
      peers[i].ip_addr = addr;
      already_in = 1;
      break;
    }
  }
  if (!already_in) {
    strcpy(peers[nr_of_peers].hostname, host);
    peers[nr_of_peers].ip_addr = addr;
    nr_of_peers += 1;
  }
  fprintf(stderr, "incoming address handled, nr_of_peers: %d\n", nr_of_peers);
}