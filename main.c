#include "include/alog.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  alog_init("logfile.log");

  for (int i = 0; i < 100; i++) {
    char message[200];
    sprintf(message, "%d message", i);
    alog(message);
  }

  alog_close();
}