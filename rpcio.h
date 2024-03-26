#ifndef __RPCIO_H__
#define __RPCIO_H__

#include "rpcconvert.h"
#include <cstdint>

#define BUFFER_SIZE 4096

void *recv_buffer(int connfd, uint8_t *buffer);

uint8_t recv_uint8(int connfd);

uint16_t recv_uint16(int connfd);

uint32_t recv_uint32(int connfd);

uint64_t recv_uint64(int connfd);

uint16_t get_filename_length(uint8_t *buffer);

uint8_t *get_filename(uint8_t *buffer, uint8_t *result, uint16_t length);

uint32_t get_uint32(uint8_t *buffer);

uint64_t get_offset(uint8_t *buffer);

int recv_loop(int cl, uint8_t *buffer, int64_t num_bytes);

int64_t recv_string(int connfd, uint8_t *result, uint16_t length, uint8_t nul);

void set_header(uint8_t *buffer, uint32_t identifier, uint8_t status);

void set_result(uint8_t *buffer, int64_t result);

void set_file_size(uint8_t *buffer, uint64_t file_size);

void set_var_length(uint8_t *buffer, uint8_t length);

void set_num_bytes(uint8_t *buffer, uint16_t num);

void set_string(uint8_t *buffer, uint8_t *string, uint16_t length);

void data_to_buffer(uint8_t *buffer, uint8_t *data, uint16_t length);

#endif
