#include "args.h"

int SEND_QUERIES_INTERVAL = 10;
int DELAYS_MEASUREMENT_INTERVAL = 1;

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
  while ((opt = getopt(argc, argv, "T:t:")) != -1) {
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
      case '?':
          if (optopt == 'T') {
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
}
