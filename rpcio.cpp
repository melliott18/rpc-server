#include "rpcio.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <err.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Get BUFFER_SIZE bytes from the client and place them in a buffer
void *recv_buffer(int connfd, uint8_t *buffer) {
  memset(buffer, 0, BUFFER_SIZE);
  recv(connfd, buffer, BUFFER_SIZE, 0);
  return buffer;
}

// Get one byte from the client and place it in a buffer
uint8_t recv_uint8(int connfd) {
  uint8_t *buffer = (uint8_t *)calloc(1, sizeof(uint8_t));
  int64_t bytes_received = 0;
  int64_t bytes_remaining = 1;
  int64_t total_bytes = 0;

  memset(buffer, 0, 1);

  do {
    bytes_received = recv(connfd, buffer + total_bytes, bytes_remaining, 0);

    if (bytes_received >= 0) {
      total_bytes += bytes_received;
    } else {
      err(1, "failed listening");
    }

    bytes_remaining -= bytes_received;
  } while (bytes_remaining > 0 && bytes_received != 0);

  uint8_t result = wire_to_uint8(buffer, 0);

  free(buffer);
  buffer = NULL;

  return result;
}

// Get two bytes from the client and place them in a buffer
uint16_t recv_uint16(int connfd) {
  uint8_t *buffer = (uint8_t *)calloc(2, sizeof(uint8_t));
  int64_t bytes_received = 0;
  int64_t bytes_remaining = 2;
  int64_t total_bytes = 0;

  memset(buffer, 0, 2);

  do {
    bytes_received = recv(connfd, buffer + total_bytes, bytes_remaining, 0);

    if (bytes_received >= 0) {
      total_bytes += bytes_received;
    } else {
      err(1, "failed listening");
    }

    bytes_remaining -= bytes_received;
  } while (bytes_remaining > 0 && bytes_received != 0);

  uint16_t result = wire_to_uint16(buffer, 0, 1);

  free(buffer);
  buffer = NULL;

  return result;
}

// Get four bytes from the client and place them in a buffer
uint32_t recv_uint32(int connfd) {
  uint8_t *buffer = (uint8_t *)calloc(4, sizeof(uint8_t));
  int64_t bytes_received = 0;
  int64_t bytes_remaining = 4;
  int64_t total_bytes = 0;

  memset(buffer, 0, 4);

  do {
    bytes_received = recv(connfd, buffer + total_bytes, bytes_remaining, 0);

    if (bytes_received >= 0) {
      total_bytes += bytes_received;
    } else {
      err(1, "failed listening");
    }

    bytes_remaining -= bytes_received;
  } while (bytes_remaining > 0 && bytes_received != 0);

  uint32_t result = wire_to_uint32(buffer, 0, 3);

  free(buffer);
  buffer = NULL;

  return result;
}

// Get eight bytes from the client and place them in a buffer
uint64_t recv_uint64(int connfd) {
  uint8_t *buffer = (uint8_t *)calloc(8, sizeof(uint8_t));
  int64_t bytes_received = 0;
  int64_t bytes_remaining = 8;
  int64_t total_bytes = 0;

  memset(buffer, 0, 8);

  do {
    bytes_received = recv(connfd, buffer + total_bytes, bytes_remaining, 0);

    if (bytes_received >= 0) {
      total_bytes += bytes_received;
    } else {
      err(1, "failed listening");
    }

    bytes_remaining -= bytes_received;
  } while (bytes_remaining > 0 && bytes_received != 0);

  uint64_t result = wire_to_uint64(buffer, 0, 7);

  free(buffer);
  buffer = NULL;

  return result;
}

// Get the length of a file name from the client
uint16_t get_filename_length(uint8_t *buffer) {
  return wire_to_uint16(buffer, 0, 1);
}

// Get a file name from the client
uint8_t *get_filename(uint8_t *buffer, uint8_t *result, uint16_t length) {
  wire_to_string(buffer, 0, length - 1, result, length + 1, 1);
  return result;
}

uint32_t get_uint32(uint8_t *buffer) { return wire_to_uint32(buffer, 0, 3); }

uint64_t get_offset(uint8_t *buffer) { return wire_to_uint64(buffer, 0, 7); }

// Get num_bytes bytes from the client and place them in a buffer
int recv_loop(int connfd, uint8_t *buffer, int64_t num_bytes) {
  int64_t bytes_received = 0;
  int64_t bytes_remaining = num_bytes;
  int64_t total_bytes = 0;

  do {
    bytes_received = recv(connfd, buffer + total_bytes, bytes_remaining, 0);

    if (bytes_received >= 0) {
      total_bytes += bytes_received;
    } else {
      err(1, "failed listening");
    }

    bytes_remaining -= bytes_received;
  } while (bytes_remaining > 0 && bytes_received > 0);

  return total_bytes;
}

// Get length bytes from the client and place them in a buffer
// The buffer has the option to be null-terminated
int64_t recv_string(int connfd, uint8_t *result, uint16_t length, uint8_t nul) {
  uint8_t *buffer;

  if (nul) {
    buffer = (uint8_t *)calloc(length + 1, sizeof(uint8_t));
    memset(buffer, 0, length + 1);
  } else {
    buffer = (uint8_t *)calloc(length, sizeof(uint8_t));
    memset(buffer, 0, length);
  }

  int64_t bytes_received = 0;
  int64_t bytes_remaining = length;
  int64_t total_bytes = 0;

  do {
    bytes_received = recv(connfd, buffer + total_bytes, bytes_remaining, 0);

    if (bytes_received >= 0) {
      total_bytes += bytes_received;
    } else {
      err(1, "failed listening");
    }

    bytes_remaining -= bytes_received;
  } while (bytes_remaining > 0 && bytes_received != 0);

  wire_to_string(buffer, 0, length - 1, result, length + 1, nul);

  free(buffer);
  buffer = NULL;
  return bytes_received;
}

// Set the response header
void set_header(uint8_t *buffer, uint32_t identifier, uint8_t status) {
  uint32_to_wire(buffer, 0, 3, identifier);
  uint8_to_wire(buffer, 4, status);
  return;
}

// Set the result of an operation
void set_result(uint8_t *buffer, int64_t result) {
  uint64_to_wire(buffer, 5, 12, result);
  return;
}

// Set the file size of a file in the buffer for a file size request
void set_file_size(uint8_t *buffer, uint64_t file_size) {
  uint64_to_wire(buffer, 5, 12, file_size);
  return;
}

// Set the length of a variable name in the buffer
void set_var_length(uint8_t *buffer, uint8_t length) {
  uint8_to_wire(buffer, 5, length);
  return;
}

// Set the number of bytes read from a read request
void set_num_bytes(uint8_t *buffer, uint16_t num) {
  uint16_to_wire(buffer, 5, 6, num);
  return;
}

// Place a string into a buffer
void set_string(uint8_t *buffer, uint8_t *string, uint16_t length) {
  string_to_wire(buffer, 0, length - 1, string);
  return;
}
