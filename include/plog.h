#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define LOG_FILE "log.log"
#define LINE_SIZE 64
#define MAX_LINES 8192
#define LOG_CAPACITY (512 * 1024 + 4)


typedef struct{


} LogLine;

// initialize log file
int init_log_file(const char *filename, size_t size) {
  int fd = open(filename, O_RDWR | O_CREAT, 0666);

  if (fd == -1) {
    perror("Failed to open file");
    return fd;
  } else if (ftruncate(fd, size) == -1) {
    perror("Failed to set log file size");
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

// initialize atomic counter
atomic_uint *map_atomic_counter(const char *filename) {

  int fd = open(filename, O_RDWR);
  if (fd == -1) {
    return NULL;
  }

  // Map counter to last 4 bytes
  void *file_in_memory =
      mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (512 * 1024));
  close(fd);

  if (file_in_memory == MAP_FAILED) {
    perror("failed to map file in memory");
    return NULL;
  }

  return (atomic_uint *)file_in_memory;
}


void increment_counter(atomic_uint* counter){
    if(!counter){
        fprintf(stderr, "Counter not mapped");
        return;
    }
    unsigned int next_index = atomic_fetch_add(counter, 1);
}