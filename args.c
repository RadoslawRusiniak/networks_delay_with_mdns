#include "args.h"

int SEND_QUERIES_INTERVAL = 10;
int DELAYS_MEASUREMENT_INTERVAL = 1;
int PORT_UI = 3637;
int PORT_UDP_MEASURE = 3382;
int UI_UPDATE_INTERVAL = 1;
int SSH_BROADCAST = 0;

int is_numeric(char *str)
{
  while (*str) {
    if (!isdigit(*str))
      return 0;
    str += 1;
  }
  return EXIT_SUCCESS;
}

void parse_arguments(int argc, char * const argv[])
{
  int opt, tmp;
  while ((opt = getopt(argc, argv, "T:t:U:u:v:s:")) != -1) {
    switch (opt) {
      case 'T':
        tmp = atoi(optarg);
        if (tmp < 0 || is_numeric(optarg) != EXIT_SUCCESS) {
          fatal("Invalid SEND_QUERIES_INTERVAL");
        } else {
          SEND_QUERIES_INTERVAL = tmp;
          if (SEND_QUERIES_INTERVAL < 1) {
            fatal("Not acceptable number for SEND_QUERIES_INTERVAL");
          }
        }
      break;
      case 't':
        tmp = atoi(optarg);
        if (tmp < 0 || is_numeric(optarg) != EXIT_SUCCESS) {
          fatal("Invalid DELAYS_MEASURMENT_INTERVAL");
        } else {
          DELAYS_MEASUREMENT_INTERVAL = tmp;
          if (DELAYS_MEASUREMENT_INTERVAL < 1) {
            fatal("Not acceptable number for DELAYS_MEASUREMENT_INTERVAL");
          }
        }
      break;
      case 'U':
        tmp = atoi(optarg);
        if (tmp < 0 || is_numeric(optarg) != EXIT_SUCCESS) {
          fatal("Invalid PORT_UI");
        } else {
          PORT_UI = tmp;
          if (PORT_UI < 1 || 65536 < PORT_UI) {
            fatal("Not acceptable number for PORT_UI");
          }
        }
      break;
      case 'u':
        tmp = atoi(optarg);
        if (tmp < 0 || is_numeric(optarg) != EXIT_SUCCESS) {
          fatal("Invalid PORT_UDP_MEASURE");
        } else {
          PORT_UDP_MEASURE = tmp;
          if (PORT_UDP_MEASURE < 1 || 65536 < PORT_UDP_MEASURE) {
            fatal("Not acceptable number for PORT_UDP_MEASURE");
          }
        }
      break;
      case 'v':
        tmp = atoi(optarg);
        if (tmp < 0 || is_numeric(optarg) != EXIT_SUCCESS) {
          fatal("Invalid UI_UPDATE_INTERVAL");
        } else {
          UI_UPDATE_INTERVAL = tmp;
          if (UI_UPDATE_INTERVAL < 1) {
            fatal("Not acceptable number for UI_UPDATE_INTERVAL");
          }
        }
      break;
      case 's':
        tmp = atoi(optarg);
        if (tmp < 0 || is_numeric(optarg) != EXIT_SUCCESS) {
          fatal("Invalid SSH_BROADCAST");
        } else {
          if (tmp != 0) {
            SSH_BROADCAST = 1;
          }
        }
      break;
      case '?':
          if (optopt == 'T' || optopt == 't' || optopt == 'U' || optopt == 'u' 
                  || optopt == 'v' || optopt == 's') {
              fprintf(stderr, "Option -%c requires an argument.\n", optopt);
          } else if (isprint(optopt)) {
              fprintf(stderr, "Unknown option `-%c'.\n", optopt);
          } else {
              fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
          }
          exit(1);
          break;
    }
  }
}

void print_arguments()
{
  fprintf(stderr, "Arguments:\n");
  fprintf(stderr, "SEND_QUERIES_INTERVAL: %d\n", SEND_QUERIES_INTERVAL);
  fprintf(stderr, "DELAYS_MEASUREMENT_INTERVAL: %d\n", DELAYS_MEASUREMENT_INTERVAL);
  fprintf(stderr, "PORT_UI: %d\n", PORT_UI);
  fprintf(stderr, "PORT_UDP_MEASURE: %d\n", PORT_UDP_MEASURE);
  fprintf(stderr, "UI_UPDATE_INTERVAL: %d\n", UI_UPDATE_INTERVAL);
  fprintf(stderr, "SSH_BROADCAST: %d\n", SSH_BROADCAST);
}
