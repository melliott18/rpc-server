#include "rpcmath.h"
#include <cerrno>
#include <cstdint>

// Add two numbers and return the result
int64_t add(int64_t a, int64_t b) {
  if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) {
    return EOVERFLOW;
  }

  int64_t result = a + b;

  return result;
}

// Subtract  two numbers and return the result
int64_t sub(int64_t a, int64_t b) {
  if ((b > 0 && a < INT64_MIN + b) || (b < 0 && a > INT64_MAX + b)) {
    return EOVERFLOW;
  }

  int64_t result = a - b;

  return result;
}

// Multiply two numbers and return the result
int64_t mul(int64_t a, int64_t b) {
  if (a == 0 || b == 0) {
    return 0;
  }

  if (a > 0) {
    if (b > 0) {
      if (a > (INT64_MAX / b)) {
        return EOVERFLOW;
      }
    } else {
      if (b < (INT64_MIN / a)) {
        return EOVERFLOW;
      }
    }
  } else {
    if (b > 0) {
      if (a < (INT64_MIN / b)) {
        return EOVERFLOW;
      }
    } else {
      if (b < (INT64_MAX / a)) {
        return EOVERFLOW;
      }
    }
  }

  int64_t result = a * b;

  return result;
}

// Divide two numbers and return the result
int64_t divide(int64_t a, int64_t b) {
  if (b == 0) {
    return EINVAL;
  }

  if (a == INT64_MIN && b == -1) {
    return EOVERFLOW;
  }

  int64_t result = a / b;

  return result;
}

// Mod two numbers and return the result
int64_t mod(int64_t a, int64_t b) {
  if (b == 0) {
    return EINVAL;
  }

  if (a == INT64_MIN && b == -1) {
    return EOVERFLOW;
  }

  int64_t result = a % b;

  return result;
}
