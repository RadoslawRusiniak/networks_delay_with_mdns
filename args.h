#ifndef ARGS_H
#define	ARGS_H


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "err.h"
  
int SEND_QUERIES_INTERVAL;
int DELAYS_MEASUREMENT_INTERVAL;
int PORT_UI;
int PORT_UDP_MEASURE;
int UI_UPDATE_INTERVAL;
int SSH_BROADCAST;

void parse_arguments(int argc, char * const argv[]);
void print_arguments();

#endif	/* ARGS_H */

