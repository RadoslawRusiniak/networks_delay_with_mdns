
#include "args.h"

int SEND_QUERIES_INTERVAL = 10;

int is_numeric(char *str) {
    while (*str) {
        if (!isdigit(*str))
            return 0;
        str += 1;
    }

    return 1;
}

void parse_arguments(int argc, char * const argv[]) {
  int opt, tmp;
  while ((opt = getopt(argc, argv, "T:")) != -1) {
    switch (opt) {
      case 'T':
        tmp = atoi(optarg);
        if (tmp < 0 || !is_numeric(optarg)) {
          fatal("Invalid SEND_QUERIES_INTERVAL");
        } else {
          SEND_QUERIES_INTERVAL = tmp;
          if (SEND_QUERIES_INTERVAL < 1) {
            fatal("Not acceptable number for SEND_QUERIES_INTERVAL");
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

void print_arguments() {
  fprintf(stderr, "Arguments:\n");
  fprintf(stderr, "SEND_QUERIES_INTERVAL: %d\n", SEND_QUERIES_INTERVAL);
}
