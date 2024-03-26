#ifndef __RPCFILE_H__
#define __RPCFILE_H__

#include <cstdint>

int64_t read(char *filename, uint64_t offset, uint16_t bufsize, uint8_t *buffer);

int64_t write(char *filename, uint64_t offset, uint16_t bufsize, uint8_t *buffer);

int64_t create(char *filename);

int64_t filesize(char *filename);

#endif
