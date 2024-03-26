#include "rpcfile.h"
#include "rpcconvert.h"
#include "rpcio.h"
#include <cerrno>
#include <cstdint>
#include <err.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

// Read bufsize bytes at a specific offset from a file into a buffer
int64_t read(char *filename, uint64_t offset, uint16_t bufsize,
             uint8_t *buffer) {
  int fd;
  struct stat st;
  stat(filename, &st);
  int64_t bytes_read = 0;

  if ((fd = open(filename, O_RDONLY, 0)) == -1) { // Open file filename
    warn("%s", filename);
    return -errno;
  }

  // Check if an error occurred while getting the size of the file
  if (st.st_size == -1) {
    warn("%s", filename);
    return -errno;
  }

  // Check if the offset is greater than the filesize of the file
  if (offset > (uint64_t)st.st_size) {
    return -EINVAL;
  }

  if (lseek(fd, offset, SEEK_SET) == -1) { // Change the file offset
    warn("%s", filename);
    return -errno;
  }

  // Read bufsize bytes from the file into the buffer
  if ((bytes_read = read(fd, buffer, bufsize)) == -1) {
    warn("%s", filename);
    return -errno;
  }

  if (close(fd) == -1) { // Close file filename
    warn("%s", filename);
    return -errno;
  }

  return bytes_read;
}

// Write bufsize bytes at a specific offset to a file from a buffer
int64_t write(char *filename, uint64_t offset, uint16_t bufsize,
              uint8_t *buffer) {
  int fd;
  int64_t bytes_written = 0;

  if ((fd = open(filename, O_WRONLY)) == -1) { // Open file filename
    warn("%s", filename);
    return -errno;
  }

  if (lseek(fd, offset, SEEK_SET) == -1) { // Change the file offset
    warn("%s", filename);
    return -errno;
  }

  // Write bufsize bytes from the buffer to the file
  if ((bytes_written = write(fd, buffer, bufsize)) == -1) {
    warn("%s", filename);
    return -errno;
  }

  if (close(fd) == -1) { // Close file filename
    warn("%s", filename);
    return -errno;
  }

  return bytes_written;
}

// Create a new file
int64_t create(char *filename) {
  int fd;

  if ((fd = open(filename, O_CREAT | O_EXCL,
                 0644)) == -1) { // Open file filename
    warn("%s", filename);
    return errno;
  }

  if (close(fd) == -1) { // Close file filename
    warn("%s", filename);
    return errno;
  }

  return 0;
}

// Get the size of a file
int64_t filesize(char *filename) {
  int fd;
  struct stat st;
  stat(filename, &st);

  if ((fd = open(filename, O_RDONLY, 0)) == -1) { // Open file filename
    warn("%s", filename);
    return -errno;
  }

  // Check if an error occurred while getting the size of the file
  if (st.st_size == -1) {
    warn("%s", filename);
    return -errno;
  }

  if (close(fd) == -1) { // Close file filename
    warn("%s", filename);
    return -errno;
  }

  return st.st_size;
}
