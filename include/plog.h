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

// https://x.com/NOTimothyLottes/status/1857804669994565771

#define LINE_SIZE 64
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
  log_file = open(filename, O_RDWR | O_CREAT, 0666);
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

  if (ftruncate(log_file, file_size) == -1) {
    perror("Couldn't allocate log file size");
    close(log_file);
    exit(1);
  }

  // align t
  size_t map_size = (file_size + page_size - 1) & ~(page_size - 1);
  off_t counter_offset = (LOG_FILE_CAPACITY - COUNTER_SIZE) &
                         ~(page_size - 1); // Align to page boundary

  // need to offset to be aligned
  mapped_region =
      mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, log_file, 0);
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
}

void alog_close() {
  if (munmap(mapped_region, LOG_FILE_CAPACITY) == -1) {
    perror("Error while unmapping log");
  }

  close(log_file);
}

void add_column(char *line, size_t length, const char *format, ...) {

  char ln[LINE_SIZE] = "|";

  // convert args to string
  va_list args;
  va_start(args, format);
  char value[LINE_SIZE];
  vsnprintf(value, LINE_SIZE, format, args);
  size_t args_len = strlen(value);
  va_end(args);

  // add eventual padding
  if (length > 0 && args_len < length) { // append "_" to fill up gap
    char padding[LINE_SIZE];
    int n_padding = length - args_len;
    memset(padding, '_', n_padding);
    padding[n_padding] =
        '\0'; // add terminal character since not handled by memset
    strncat(ln, padding, LINE_SIZE);
  }

  // concat padding and value
  strncat(ln, value, LINE_SIZE);

  // concat to global line
  strncat(line, ln, LINE_SIZE);
}

void add_line(char *line, const char *message, const int line_number,
              const char *file) {

  // get time
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  double dif_time = (current_time.tv_sec - start_time.tv_sec) * 1e6 +
                    (current_time.tv_nsec - start_time.tv_nsec) / 1e9;

  // format message
  add_column(line, 10, "%f", dif_time);   // add time from start
  add_column(line, 8, "%d", line_number); // add line number
  add_column(line, 20, "%s", file);       // add file name
  add_column(line, 0, "%s", message);     // add log message
  strncat(line, "\n", LINE_SIZE);
}

void atomic_log_message(const char *message, const int line, const char *file) {

  if (!mapped_region || !counter) {
    perror("log not initialized, make sure to call init_log_file() function "
           "prior logging");
    return;
  }

  char new_line[LINE_SIZE] = "";
  add_line(new_line, message, line, file);

  // increment atomic counter
  atomic_fetch_add(counter, 1);
  printf("%u", *counter);

}