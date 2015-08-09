/* 
 * File:   args.h
 * Author: radek8
 *
 * Created on 9 sierpień 2015, 20:11
 */

#ifndef ARGS_H
#define	ARGS_H


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "err.h"
  
extern int SEND_QUERIES_INTERVAL;

extern void parse_arguments(int argc, char * const argv[]);
extern void print_arguments();


#endif	/* ARGS_H */

