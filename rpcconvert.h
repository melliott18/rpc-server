#ifndef __RPCCONVERT_H__
#define __RPCCONVERT_H__

#include <cstdint>

#define BUFFER_SIZE 4096

void uint8_to_wire(uint8_t *buffer, uint16_t index, uint8_t value);

uint8_t wire_to_uint8(uint8_t *buffer, uint16_t index);

void uint16_to_wire(uint8_t *buffer, uint16_t start, uint16_t end, uint16_t value);

uint16_t wire_to_uint16(uint8_t *buffer, uint16_t start, uint16_t end);

void uint32_to_wire(uint8_t *buffer, uint16_t start, uint16_t end, uint32_t value);

uint32_t wire_to_uint32(uint8_t *buffer, uint16_t start, uint16_t end);

void uint64_to_wire(uint8_t *buffer, uint16_t start, uint16_t end, uint64_t value);

uint64_t wire_to_uint64(uint8_t *buffer, uint16_t start, uint16_t end);

void string_to_wire(uint8_t *buffer, uint16_t start, uint16_t end, uint8_t *value);

void wire_to_string(uint8_t *buffer, uint16_t start, uint16_t end, uint8_t *result, uint16_t length, uint8_t nul);

#endif
