#include "rpcconvert.h"
#include <cstddef>
#include <cstdint>

// Converts a uint8_t number to wire format and places the result in a buffer
void uint8_to_wire(uint8_t *buffer, uint16_t index, uint8_t value) {
  if ((buffer != NULL) && (index >= 0) && (index < BUFFER_SIZE)) {
    buffer[index] = value;
  }

  return;
}

// Extracts a uint8_t number from a buffer in wire format and returns the result
uint8_t wire_to_uint8(uint8_t *buffer, uint16_t index) {
  if (buffer != NULL) {
    return buffer[index];
  }

  return 0;
}

// Converts a uint16_t number to wire format and places the result in a buffer
void uint16_to_wire(uint8_t *buffer, uint16_t start, uint16_t end,
                    uint16_t value) {
  if ((buffer != NULL) && (start < end) && (start >= 0) &&
      (end < BUFFER_SIZE)) {
    buffer[start] = (value >> 8) & 0xFF;
    buffer[end] = value & 0xFF;
  }

  return;
}

// Extracts a uint16_t number from a buffer in wire format and returns the result
uint16_t wire_to_uint16(uint8_t *buffer, uint16_t start, uint16_t end) {
  if ((buffer != NULL) && (start < end) && (start >= 0) &&
      (end < BUFFER_SIZE)) {
    uint16_t result = 0;
    result += (uint16_t)buffer[start];
    result = (result << 8);
    result += buffer[end];
    return result;
  }

  return 0;
}

// Converts a uint32_t number to wire format and places the result in a buffer
void uint32_to_wire(uint8_t *buffer, uint16_t start, uint16_t end,
                    uint32_t value) {
  if ((buffer != NULL) && (start < end) && (start >= 0) &&
      (end < BUFFER_SIZE)) {
    uint8_t shift = 24;

    for (uint16_t i = start; i <= end; i++) {
      uint8_t temp = (value >> shift);
      buffer[i] = temp & 0xFF;
      shift -= 8;
    }
  }

  return;
}

// Extracts a uint32_t number from a buffer in wire format and returns the result
uint32_t wire_to_uint32(uint8_t *buffer, uint16_t start, uint16_t end) {
  if ((buffer != NULL) && (start < end) && (start >= 0) &&
      (end < BUFFER_SIZE)) {
    uint32_t result = 0;
    uint8_t shift = 8;

    for (uint16_t i = start; i < end; i++) {
      uint32_t temp = (uint32_t)buffer[i];
      result += temp;
      result = (result << shift);
    }

    result += buffer[end];
    return result;
  }

  return 0;
}

// Converts a uint64_t number to wire format and places the result in a buffer
void uint64_to_wire(uint8_t *buffer, uint16_t start, uint16_t end,
                    uint64_t value) {
  if ((buffer != NULL) && (start < end) && (start >= 0) &&
      (end < BUFFER_SIZE)) {
    uint8_t shift = 56;

    for (uint16_t i = start; i <= end; i++) {
      uint8_t temp = (value >> shift);
      buffer[i] = temp & 0xFF;
      shift -= 8;
    }
  }

  return;
}

// Extracts a uint64_t number from a buffer in wire format and returns the result
uint64_t wire_to_uint64(uint8_t *buffer, uint16_t start, uint16_t end) {
  if ((buffer != NULL) && (start < end) && (start >= 0) &&
      (end < BUFFER_SIZE)) {
    uint64_t result = 0;
    uint8_t shift = 8;

    for (uint16_t i = start; i < end; i++) {
      uint64_t temp = (uint64_t)buffer[i];
      result += temp;
      result = (result << shift);
    }

    result += buffer[end];
    return result;
  }

  return 0;
}

// Converts a string to wire format and places the result in a buffer
void string_to_wire(uint8_t *buffer, uint16_t start, uint16_t end,
                    uint8_t *value) {
  if ((buffer != NULL) && (start <= end) && (start >= 0) &&
      (end < BUFFER_SIZE) && (value != NULL)) {
    uint16_t j = 0;

    for (uint16_t i = start; i <= end; i++) {
      buffer[i] = value[j];
      j++;
    }
  }

  return;
}

// Extracts a string from a buffer in wire format and places the result in a separate buffer
void wire_to_string(uint8_t *buffer, uint16_t start, uint16_t end,
                    uint8_t *result, uint16_t length, uint8_t nul) {
  if ((buffer != NULL) && (start <= end) && (start >= 0) &&
      (end < BUFFER_SIZE) && (result != NULL)) {
    uint16_t j = 0;

    for (uint16_t i = start; i <= end; i++) {
      result[j] = buffer[i];
      j++;
    }

    if (nul) {
      result[length - 1] = '\0';
    }
  }

  return;
}
