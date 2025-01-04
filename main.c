#include "include/plog.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  init_log_file("logfile.log");
  printf("Test\n");
  log_c("First message");

}