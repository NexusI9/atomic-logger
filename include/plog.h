#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define LOG_FILE "log.log"
#define LINE_SIZE 64
#define MAX_LINES 8192
#define LOG_FILE_CAPACITY (1024 * 1024)

typedef struct {
  const atomic_uint *time;
  const char *file;
  const unsigned int line;
  const char *message;
} LogLine;

atomic_uint *counter = NULL;
FILE *log_file = NULL;

// initialize log file, make sure exists (or create) and own enough capacity
void init_log_file(const char *filename) {

  if (log_file != NULL) {
    return; // already init
  }

  // open potential existing file
  log_file = fopen(filename, "w+");
  if (!log_file) {
    perror("Couldn't open log file");
    exit(1);
  }

  // init atomic counter and map to last 4 bytes of memory map
  int fd = fileno(log_file);
  counter = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                 LOG_FILE_CAPACITY - 4);
  if (counter == MAP_FAILED) {
    perror("Couldn't map atomic counter in memory");
    exit(1);
  }
}

void log_c(const char *message) {

  if (!log_file || !counter) {
    perror("log not initialized, make sure to call init_log_file() function prior logging");
    return;
  }

  atomic_fetch_add(counter, 1);
}