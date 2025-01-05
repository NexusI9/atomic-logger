#include <fcntl.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define LINE_SIZE 64 // 64 bytes to fit cacheline
#define LOG_FILE_CAPACITY (1024 * 1024)
#define COUNTER_SIZE sizeof(atomic_uint)

#define alog(msg)                                                              \
  atomic_log_message(                                                          \
      msg, __LINE__,                                                           \
      __FILE__); // define as macro to automatically set line and file

atomic_uint *counter = NULL;
void *mapped_region = NULL;
int log_file = -1;
struct timespec start_time, current_time;

// initialize log file, make sure exists (or create) and own enough capacity
void alog_init(const char *filename) {

  if (mapped_region) {
    return; // already init
  }

  // open potential existing file
  log_file = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (log_file < 0) {
    perror("Couldn't open log file");
    close(log_file);
    exit(1);
  }

  // init atomic counter and map to last 4 bytes of memory map
  size_t page_size = sysconf(_SC_PAGE_SIZE);

  // Since our file size is likely not page aligned, need to align to page
  // get file size
  struct stat finfo;
  fstat(log_file, &finfo);
  off_t file_size = finfo.st_size;

  // fill up log
  if (ftruncate(log_file, LOG_FILE_CAPACITY) == -1) {
    perror("Couldn't allocate log file size");
    close(log_file);
    exit(1);
  }

  // Make sure the counter offset satisfies page alignment
  size_t map_size = (file_size + page_size - 1) & ~(page_size - 1);
  off_t counter_offset = (LOG_FILE_CAPACITY - COUNTER_SIZE) &
                         ~(page_size - 1); // Align to page boundary (at least 4kb)

  // need to offset to be aligned
  // Use MAP_SHARED to automatically write back to file
  mapped_region = mmap(NULL, LOG_FILE_CAPACITY, PROT_READ | PROT_WRITE,
                       MAP_SHARED, log_file, 0);
  if (mapped_region == MAP_FAILED) {
    perror("Could not map the region file");
    close(log_file);
    exit(1);
  }

  // start time
  clock_gettime(CLOCK_MONOTONIC, &start_time);

  // get the counter pointer position in the mapped region
  counter = (atomic_uint *)((char *)mapped_region + counter_offset);
  atomic_init(counter, 0); // start counter at 0

  // set table header
  unsigned int log_index = atomic_fetch_add(counter, 1);
  char *log_entry = (char *)mapped_region + log_index * LINE_SIZE;
  snprintf(log_entry, LINE_SIZE, "%-9s| %-5s| %-20s| %s\n", "Time", "Line",
           "File", "Comment");

  log_index = atomic_fetch_add(counter, 1);
  log_entry = (char *)mapped_region + log_index * LINE_SIZE;
  snprintf(log_entry, LINE_SIZE, "---------+------+---------------------+-----------------------\n");
}

void alog_close() {

  // unmap region
  if (munmap(mapped_region, LOG_FILE_CAPACITY) == -1) {
    perror("Error while unmapping log");
  } else {
    printf("Successfuly unmapping log");
  }

  close(log_file);
}

double diff_time() {
  // get time
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  return (current_time.tv_sec - start_time.tv_sec) * 1e6 +
         (current_time.tv_nsec - start_time.tv_nsec) / 1e9;
}

void atomic_log_message(const char *message, const int line, const char *file) {

  if (!mapped_region || !counter) {
    perror("log not initialized, make sure to call init_log_file() function "
           "prior logging");
    return;
  }

  // increment atomic counter
  unsigned int log_index = atomic_fetch_add(counter, 1);

  if (log_index * LINE_SIZE >= LOG_FILE_CAPACITY - sizeof(atomic_uint)) {
    // Log reached max capacity
    perror("Log reached max capacity");
    return;
  }

  char *log_entry = (char *)mapped_region + log_index * LINE_SIZE;
  snprintf(log_entry, LINE_SIZE, "%-9f| %-5d| %-20s| %s\n", diff_time(), line,
           file, message);
}