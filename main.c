#include "include/plog.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  alog_init("logfile.log");

  alog("First message");
  alog("Second message");
  alog("Third message");

  alog_close();

}