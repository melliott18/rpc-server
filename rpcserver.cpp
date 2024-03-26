#include "rpcserver.h"
#include "rpcconvert.h"
#include "rpcfile.h"
#include "rpcio.h"
#include "rpckeyvaluestore.h"
#include "rpcmath.h"
#include "rpcqueue.h"
#include <arpa/inet.h>
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <err.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Create the socket connection
int server_connect(char *hostname, uint16_t port) {
  struct hostent *hent = gethostbyname(hostname);

  if (hent == NULL) {
    fprintf(stderr, "rpcserver: invalid arguments\n");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in addr;
  memcpy(&addr.sin_addr.s_addr, hent->h_addr, hent->h_length);
  addr.sin_port = htons(port);
  addr.sin_family = AF_INET;
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0) {
    perror("");
    return -1;
  }

  int enable = 1;

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
    perror("");
    return -1;
  }

  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("");
    return -1;
  }

  if (listen(sock, 128) != 0) {
    err(2, "unable to listen");
    return -1;
  }

  return sock;
}

// Process an RPC request
int server_run(int connfd, Thread thread) {
  uint16_t buff_size = 0;
  int64_t bytes_read = 0;
  int64_t bytes_received = 0;
  int64_t bytes_remaining;
  int64_t bytes_written = 0;
  uint8_t *data = NULL;
  uint64_t file_size = 0;
  uint8_t *filename = NULL;
  uint16_t filename_length = 0;
  uint8_t flag = 0;
  uint16_t function = 0;
  uint32_t identifier = 0;
  uint64_t iterations = 0;
  uint32_t magic_number = 0;
  uint8_t *name = NULL;
  uint64_t offset = 0;
  uint16_t opcode = 0;
  uint8_t recursive = 0;
  int64_t result = 0;
  int64_t status = 0;
  uint8_t *temp = NULL;
  int64_t val_a = 0;
  int64_t val_b = 0;
  uint8_t *var_a = NULL;
  uint8_t *var_b = NULL;
  uint8_t *var_result = 0;
  uint8_t var_a_length = 0;
  uint8_t var_b_length = 0;
  uint8_t var_result_length = 0;

  opcode = wire_to_uint16(thread->buffer, 0, 1);
  identifier = wire_to_uint32(thread->buffer, 2, 5);

  uint8_t var = opcode & 0xFF;
  uint8_t a_exists = 0;
  uint8_t b_exists = 0;
  uint8_t result_exists = 0;

  if (opcode & (1 << 8) && !(opcode & (1 << 9))) { // If opcode is 0x1XX
    function = opcode &= ~(0xF << 4);

    // If variable a needs to be received or a variable needs to be deleted
    if ((var & (1 << 3)) || (var & (1 << 4)) || (var == 0xF)) {
      a_exists = 1;
      var_a_length = recv_uint8(connfd);

      if (var_a_length >= 1 && var_a_length <= 31) {
        var_a = (uint8_t *)calloc(var_a_length + 1, sizeof(uint8_t));
        bytes_received = recv_string(connfd, var_a, var_a_length, 1);

        for (uint8_t i = 0; var_a[i] != 0; i++) {
          // Check if a character is a valid character
          if (i == 0) {
            if (isdigit(var_a[i])) {
              status = EINVAL;
            }
            if (!isalpha(var_a[i])) {
              status = EINVAL;
            }
          } else {
            if (!isdigit(var_a[i])) {
              if (!isalpha(var_a[i])) {
                if (var_a[i] != '_') {
                  status = EINVAL;
                }
              }
            }
          }
        }
      }
    } else {
      val_a = recv_uint64(connfd);
    }

    // If variable b needs to be received
    if ((var & (1 << 5)) || (var == 0x9)) {
      b_exists = 1;
      var_b_length = recv_uint8(connfd);

      if (var_b_length >= 1 && var_b_length <= 31) {
        var_b = (uint8_t *)calloc(var_b_length + 1, sizeof(uint8_t));
        bytes_received = recv_string(connfd, var_b, var_b_length, 1);

        for (uint8_t i = 0; var_b[i] != 0; i++) {
          // Check if a character is a valid character
          if (i == 0) {
            if (isdigit(var_b[i])) {
              status = EINVAL;
            }
            if (!isalpha(var_b[i])) {
              status = EINVAL;
            }
          } else {
            if (!isdigit(var_b[i])) {
              if (!isalpha(var_b[i])) {
                if (var_b[i] != '_') {
                  status = EINVAL;
                }
              }
            }
          }
        }
      }
    } else if (var != 0x8 && var != 0xF) {
      val_b = recv_uint64(connfd);
    }

    // If the result needs to be stored as a variable
    if ((var & (1 << 6))) {
      result_exists = 1;
      var_result_length = recv_uint8(connfd);

      if (var_result_length >= 1 && var_result_length <= 31) {
        var_result = (uint8_t *)calloc(var_result_length + 1, sizeof(uint8_t));
        bytes_received = recv_string(connfd, var_result, var_result_length, 1);
        if (isnumber((char *)var_result)) {
          status = EINVAL;
        } else {
          for (uint8_t i = 0; var_result[i] != 0; i++) {
            // Check if a character is a valid character
            if (i == 0) {
              if (isdigit(var_result[i])) {
                status = EINVAL;
              }
              if (!isalpha(var_result[i])) {
                status = EINVAL;
              }
            } else {
              if (!isdigit(var_result[i])) {
                if (!isalpha(var_result[i])) {
                  if (var_result[i] != '_') {
                    status = EINVAL;
                  }
                }
              }
            }
          }
        }
      }
    }
    if ((var & (1 << 7))) {
      recursive = 1;
    }
  } else {
    function = opcode;
  }

  if (function == 0x0101) { /* Add */
    if (status == 0) {
      if (a_exists || b_exists || result_exists) {
        pthread_mutex_lock(thread->kvs_mutex); // Lock the k-v store mutex
        // --------------------------------------------------------------------
        // Begin critical section

        if (a_exists) {
          status = key_value_store_key_check(thread->kvstore, var_a);
          if (status == 0) {
            flag = key_value_store_key_flag_lookup(thread->kvstore, var_a);
            if (flag == 0) {
              val_a = key_value_store_key_value_lookup(thread->kvstore, var_a);
            } else if (flag == 1) {
              if (recursive) {
                iterations = 0;
                temp = var_a;
                do {
                  var_a = temp;
                  temp = key_value_store_key_name_lookup(thread->kvstore, temp);
                  iterations++;
                } while (temp != NULL && iterations < thread->iterations);
                if (iterations < thread->iterations) {
                  if (temp == NULL) {
                    val_a = key_value_store_key_value_lookup(thread->kvstore,
                                                             var_a);
                    if (val_a == ENOENT) {
                      status =
                          key_value_store_key_check(thread->kvstore, var_a);
                    }
                  }
                } else {
                  status = ELOOP;
                }
              } else {
                status = EFAULT;
              }
            }
          }
        }

        if (b_exists && status == 0) {
          status = key_value_store_key_check(thread->kvstore, var_b);
          if (status == 0) {
            flag = key_value_store_key_flag_lookup(thread->kvstore, var_b);
            if (flag == 0) {
              val_b = key_value_store_key_value_lookup(thread->kvstore, var_b);
            } else if (flag == 1) {
              if (recursive) {
                iterations = 0;
                temp = var_b;
                do {
                  var_b = temp;
                  temp = key_value_store_key_name_lookup(thread->kvstore, temp);
                  iterations++;
                } while (temp != NULL && iterations < thread->iterations);
                if (iterations < thread->iterations) {
                  if (temp == NULL) {
                    val_b = key_value_store_key_value_lookup(thread->kvstore,
                                                             var_b);
                    if (val_b == ENOENT) {
                      status =
                          key_value_store_key_check(thread->kvstore, var_b);
                    }
                  }
                } else {
                  status = ELOOP;
                }
              } else {
                status = EFAULT;
              }
            }
          }
        }

        result = add(val_a, val_b);

        if (result == EOVERFLOW && (val_a + val_b) != EOVERFLOW &&
            status == 0) {
          status = EOVERFLOW;
        }

        if (result_exists && status == 0) {
          key_value_store_insert_key_value(thread->kvstore, var_result, result);
          log_insert_key(var_result, NULL, result, 0, *(thread->logfd));
          result =
              key_value_store_key_value_lookup(thread->kvstore, var_result);
        }

        // End critical section
        // --------------------------------------------------------------------
        pthread_mutex_unlock(thread->kvs_mutex); // Unlock the k-v store mutex
      } else {
        result = add(val_a, val_b);

        if (result == EOVERFLOW && (val_a + val_b) != EOVERFLOW) {
          status = EOVERFLOW;
        }
      }
    }

    set_header(thread->buffer, identifier, status);

    if (status == 0) {
      set_result(thread->buffer, result);
      send(connfd, thread->buffer, 13, 0);
    } else {
      send(connfd, thread->buffer, 5, 0);
    }
  } else if (function == 0x0102) { /* Substract */
    if (status == 0) {
      if (a_exists || b_exists || result_exists) {
        pthread_mutex_lock(thread->kvs_mutex); // Lock the k-v store mutex
        // --------------------------------------------------------------------
        // Begin critical section

        if (a_exists) {
          status = key_value_store_key_check(thread->kvstore, var_a);
          if (status == 0) {
            flag = key_value_store_key_flag_lookup(thread->kvstore, var_a);
            if (flag == 0) {
              val_a = key_value_store_key_value_lookup(thread->kvstore, var_a);
            } else if (flag == 1) {
              if (recursive) {
                iterations = 0;
                temp = var_a;
                do {
                  var_a = temp;
                  temp = key_value_store_key_name_lookup(thread->kvstore, temp);
                  iterations++;
                } while (temp != NULL && iterations < thread->iterations);
                if (iterations < thread->iterations) {
                  if (temp == NULL) {
                    val_a = key_value_store_key_value_lookup(thread->kvstore,
                                                             var_a);
                    if (val_a == ENOENT) {
                      status =
                          key_value_store_key_check(thread->kvstore, var_a);
                    }
                  }
                } else {
                  status = ELOOP;
                }
              } else {
                status = EFAULT;
              }
            }
          }
        }

        if (b_exists && status == 0) {
          status = key_value_store_key_check(thread->kvstore, var_b);
          if (status == 0) {
            flag = key_value_store_key_flag_lookup(thread->kvstore, var_b);
            if (flag == 0) {
              val_b = key_value_store_key_value_lookup(thread->kvstore, var_b);
            } else if (flag == 1) {
              if (recursive) {
                iterations = 0;
                temp = var_b;
                do {
                  var_b = temp;
                  temp = key_value_store_key_name_lookup(thread->kvstore, temp);
                  iterations++;
                } while (temp != NULL && iterations < thread->iterations);
                if (iterations < thread->iterations) {
                  if (temp == NULL) {
                    val_b = key_value_store_key_value_lookup(thread->kvstore,
                                                             var_b);
                    if (val_b == ENOENT) {
                      status =
                          key_value_store_key_check(thread->kvstore, var_b);
                    }
                  }
                } else {
                  status = ELOOP;
                }
              } else {
                status = EFAULT;
              }
            }
          }
        }

        result = sub(val_a, val_b);

        if (result == EOVERFLOW && (val_a - val_b) != EOVERFLOW &&
            status == 0) {
          status = EOVERFLOW;
        }

        if (result_exists && status == 0) {
          key_value_store_insert_key_value(thread->kvstore, var_result, result);
          log_insert_key(var_result, NULL, result, 0, *(thread->logfd));
          result =
              key_value_store_key_value_lookup(thread->kvstore, var_result);
        }

        // End critical section
        // --------------------------------------------------------------------
        pthread_mutex_unlock(thread->kvs_mutex); // Unlock the k-v store mutex
      } else {
        result = sub(val_a, val_b);

        if (result == EOVERFLOW && (val_a - val_b) != EOVERFLOW) {
          status = EOVERFLOW;
        }
      }
    }

    set_header(thread->buffer, identifier, status);

    if (status == 0) {
      set_result(thread->buffer, result);
      send(connfd, thread->buffer, 13, 0);
    } else {
      send(connfd, thread->buffer, 5, 0);
    }
  } else if (function == 0x0103) { /* Multiply */
    if (status == 0) {
      if (a_exists || b_exists || result_exists) {
        pthread_mutex_lock(thread->kvs_mutex); // Lock the k-v store mutex
        // --------------------------------------------------------------------
        // Begin critical section

        if (a_exists) {
          status = key_value_store_key_check(thread->kvstore, var_a);
          if (status == 0) {
            flag = key_value_store_key_flag_lookup(thread->kvstore, var_a);
            if (flag == 0) {
              val_a = key_value_store_key_value_lookup(thread->kvstore, var_a);
            } else if (flag == 1) {
              if (recursive) {
                iterations = 0;
                temp = var_a;
                do {
                  var_a = temp;
                  temp = key_value_store_key_name_lookup(thread->kvstore, temp);
                  iterations++;
                } while (temp != NULL && iterations < thread->iterations);
                if (iterations < thread->iterations) {
                  if (temp == NULL) {
                    val_a = key_value_store_key_value_lookup(thread->kvstore,
                                                             var_a);
                    if (val_a == ENOENT) {
                      status =
                          key_value_store_key_check(thread->kvstore, var_a);
                    }
                  }
                } else {
                  status = ELOOP;
                }
              } else {
                status = EFAULT;
              }
            }
          }
        }

        if (b_exists && status == 0) {
          status = key_value_store_key_check(thread->kvstore, var_b);
          if (status == 0) {
            flag = key_value_store_key_flag_lookup(thread->kvstore, var_b);
            if (flag == 0) {
              val_b = key_value_store_key_value_lookup(thread->kvstore, var_b);
            } else if (flag == 1) {
              if (recursive) {
                iterations = 0;
                temp = var_b;
                do {
                  var_b = temp;
                  temp = key_value_store_key_name_lookup(thread->kvstore, temp);
                  iterations++;
                } while (temp != NULL && iterations < thread->iterations);
                if (iterations < thread->iterations) {
                  if (temp == NULL) {
                    val_b = key_value_store_key_value_lookup(thread->kvstore,
                                                             var_b);
                    if (val_b == ENOENT) {
                      status =
                          key_value_store_key_check(thread->kvstore, var_b);
                    }
                  }
                } else {
                  status = ELOOP;
                }
              } else {
                status = EFAULT;
              }
            }
          }
        }

        result = mul(val_a, val_b);

        if (result == EOVERFLOW && (val_a * val_b) != EOVERFLOW &&
            status == 0) {
          status = EOVERFLOW;
        }

        if (result_exists && status == 0) {
          key_value_store_insert_key_value(thread->kvstore, var_result, result);
          log_insert_key(var_result, NULL, result, 0, *(thread->logfd));
          result =
              key_value_store_key_value_lookup(thread->kvstore, var_result);
        }

        // End critical section
        // --------------------------------------------------------------------
        pthread_mutex_unlock(thread->kvs_mutex); // Unlock the k-v store mutex
      } else {
        result = mul(val_a, val_b);

        if (result == EOVERFLOW && (val_a * val_b) != EOVERFLOW) {
          status = EOVERFLOW;
        }
      }
    }

    set_header(thread->buffer, identifier, status);

    if (status == 0) {
      set_result(thread->buffer, result);
      send(connfd, thread->buffer, 13, 0);
    } else {
      send(connfd, thread->buffer, 5, 0);
    }
  } else if (function == 0x0104) { /* Divide */
    if (status == 0) {
      if (a_exists || b_exists || result_exists) {
        pthread_mutex_lock(thread->kvs_mutex); // Lock the k-v store mutex
        // --------------------------------------------------------------------
        // Begin critical section

        if (a_exists) {
          status = key_value_store_key_check(thread->kvstore, var_a);
          if (status == 0) {
            flag = key_value_store_key_flag_lookup(thread->kvstore, var_a);
            if (flag == 0) {
              val_a = key_value_store_key_value_lookup(thread->kvstore, var_a);
            } else if (flag == 1) {
              if (recursive) {
                iterations = 0;
                temp = var_a;
                do {
                  var_a = temp;
                  temp = key_value_store_key_name_lookup(thread->kvstore, temp);
                  iterations++;
                } while (temp != NULL && iterations < thread->iterations);
                if (iterations < thread->iterations) {
                  if (temp == NULL) {
                    val_a = key_value_store_key_value_lookup(thread->kvstore,
                                                             var_a);
                    if (val_a == ENOENT) {
                      status =
                          key_value_store_key_check(thread->kvstore, var_a);
                    }
                  }
                } else {
                  status = ELOOP;
                }
              } else {
                status = EFAULT;
              }
            }
          }
        }

        if (b_exists && status == 0) {
          status = key_value_store_key_check(thread->kvstore, var_b);
          if (status == 0) {
            flag = key_value_store_key_flag_lookup(thread->kvstore, var_b);
            if (flag == 0) {
              val_b = key_value_store_key_value_lookup(thread->kvstore, var_b);
            } else if (flag == 1) {
              if (recursive) {
                iterations = 0;
                temp = var_b;
                do {
                  var_b = temp;
                  temp = key_value_store_key_name_lookup(thread->kvstore, temp);
                  iterations++;
                } while (temp != NULL && iterations < thread->iterations);
                if (iterations < thread->iterations) {
                  if (temp == NULL) {
                    val_b = key_value_store_key_value_lookup(thread->kvstore,
                                                             var_b);
                    if (val_b == ENOENT) {
                      status =
                          key_value_store_key_check(thread->kvstore, var_b);
                    }
                  }
                } else {
                  status = ELOOP;
                }
              } else {
                status = EFAULT;
              }
            }
          }
        }

        result = divide(val_a, val_b);

        if (result == EINVAL && val_b == 0 && status == 0) {
          status = EINVAL;
        } else if (result == EOVERFLOW && val_b != 0 &&
                   (val_a / val_b) != EOVERFLOW) {
          status = EOVERFLOW;
        }

        if (result_exists && status == 0) {
          key_value_store_insert_key_value(thread->kvstore, var_result, result);
          log_insert_key(var_result, NULL, result, 0, *(thread->logfd));
          result =
              key_value_store_key_value_lookup(thread->kvstore, var_result);
        }

        // End critical section
        // --------------------------------------------------------------------
        pthread_mutex_unlock(thread->kvs_mutex); // Unlock the k-v store mutex
      } else {
        result = divide(val_a, val_b);

        if (result == EINVAL && val_b == 0) {
          status = EINVAL;
        } else if (result == EOVERFLOW && val_b != 0 &&
                   (val_a / val_b) != EOVERFLOW) {
          status = EOVERFLOW;
        }
      }
    }

    set_header(thread->buffer, identifier, status);

    if (status == 0) {
      set_result(thread->buffer, result);
      send(connfd, thread->buffer, 13, 0);
    } else {
      send(connfd, thread->buffer, 5, 0);
    }
  } else if (function == 0x0105) { /* Modulo */
    if (status == 0) {
      if (a_exists || b_exists || result_exists) {
        pthread_mutex_lock(thread->kvs_mutex); // Lock the k-v store mutex
        // --------------------------------------------------------------------
        // Begin critical section

        if (a_exists) {
          status = key_value_store_key_check(thread->kvstore, var_a);
          if (status == 0) {
            flag = key_value_store_key_flag_lookup(thread->kvstore, var_a);
            if (flag == 0) {
              val_a = key_value_store_key_value_lookup(thread->kvstore, var_a);
            } else if (flag == 1) {
              if (recursive) {
                iterations = 0;
                temp = var_a;
                do {
                  var_a = temp;
                  temp = key_value_store_key_name_lookup(thread->kvstore, temp);
                  iterations++;
                } while (temp != NULL && iterations < thread->iterations);
                if (iterations < thread->iterations) {
                  if (temp == NULL) {
                    val_a = key_value_store_key_value_lookup(thread->kvstore,
                                                             var_a);
                    if (val_a == ENOENT) {
                      status =
                          key_value_store_key_check(thread->kvstore, var_a);
                    }
                  }
                } else {
                  status = ELOOP;
                }
              } else {
                status = EFAULT;
              }
            }
          }
        }

        if (b_exists && status == 0) {
          status = key_value_store_key_check(thread->kvstore, var_b);
          if (status == 0) {
            flag = key_value_store_key_flag_lookup(thread->kvstore, var_b);
            if (flag == 0) {
              val_b = key_value_store_key_value_lookup(thread->kvstore, var_b);
            } else if (flag == 1) {
              if (recursive) {
                iterations = 0;
                temp = var_b;
                do {
                  var_b = temp;
                  temp = key_value_store_key_name_lookup(thread->kvstore, temp);
                  iterations++;
                } while (temp != NULL && iterations < thread->iterations);
                if (iterations < thread->iterations) {
                  if (temp == NULL) {
                    val_b = key_value_store_key_value_lookup(thread->kvstore,
                                                             var_b);
                    if (val_b == ENOENT) {
                      status =
                          key_value_store_key_check(thread->kvstore, var_b);
                    }
                  }
                } else {
                  status = ELOOP;
                }
              } else {
                status = EFAULT;
              }
            }
          }
        }

        result = mod(val_a, val_b);

        if (result == EINVAL && val_b == 0 && status == 0) {
          status = EINVAL;
        } else if (result == EOVERFLOW && val_b != 0 &&
                   (val_a % val_b) != EOVERFLOW) {
          status = EOVERFLOW;
        }

        if (result_exists && status == 0) {
          key_value_store_insert_key_value(thread->kvstore, var_result, result);
          log_insert_key(var_result, NULL, result, 0, *(thread->logfd));
          result =
              key_value_store_key_value_lookup(thread->kvstore, var_result);
        }

        // End critical section
        // --------------------------------------------------------------------
        pthread_mutex_unlock(thread->kvs_mutex); // Unlock the k-v store mutex
      } else {
        result = mod(val_a, val_b);

        if (result == EINVAL && val_b == 0) {
          status = EINVAL;
        } else if (result == EOVERFLOW && val_b != 0 &&
                   (val_a % val_b) != EOVERFLOW) {
          status = EOVERFLOW;
        }
      }
    }

    set_header(thread->buffer, identifier, status);

    if (status == 0) {
      set_result(thread->buffer, result);
      send(connfd, thread->buffer, 13, 0);
    } else {
      send(connfd, thread->buffer, 5, 0);
    }
  } else if (function == 0x0108) { /* Get variable */
    if (status == 0) {
      if (a_exists) {
        pthread_mutex_lock(thread->kvs_mutex); // Lock the k-v store mutex
        // --------------------------------------------------------------------
        // Begin critical section

        if (a_exists) {
          status = key_value_store_key_check(thread->kvstore, var_a);
          if (status == 0) {
            flag = key_value_store_key_flag_lookup(thread->kvstore, var_a);
            if (flag == 1) {
              name = key_value_store_key_name_lookup(thread->kvstore, var_a);
            } else if (flag == 0) {
              status = EFAULT;
            }
          }
        }

        // End critical section
        // --------------------------------------------------------------------
        pthread_mutex_unlock(thread->kvs_mutex); // Unlock the k-v store mutex
      }
    }

    memset(thread->buffer, 0, BUFFER_SIZE);
    set_header(thread->buffer, identifier, status);

    if (status == 0) {
      set_var_length(thread->buffer, strlen((char *)name));
      send(connfd, thread->buffer, 6, 0);
      memset(thread->buffer, 0, BUFFER_SIZE);
      set_string(thread->buffer, name, strlen((char *)name));
      send(connfd, thread->buffer, strlen((char *)name), 0);
    } else {
      send(connfd, thread->buffer, 5, 0);
    }
  } else if (function == 0x0109) { /* Set variable */
    if (status == 0) {
      if (a_exists || b_exists) {
        pthread_mutex_lock(thread->kvs_mutex); // Lock the k-v store mutex
        // --------------------------------------------------------------------
        // Begin critical section

        if (a_exists && b_exists) {
          status =
              key_value_store_insert_key_name(thread->kvstore, var_a, var_b);
          log_insert_key(var_a, var_b, 0, 1, *(thread->logfd));
        }

        // End critical section
        // --------------------------------------------------------------------
        pthread_mutex_unlock(thread->kvs_mutex); // Unlock the k-v store mutex
      }
    }

    memset(thread->buffer, 0, BUFFER_SIZE);
    set_header(thread->buffer, identifier, status);
    send(connfd, thread->buffer, 5, 0);
  } else if (function == 0x010f) { /* Delete */
    if (status == 0) {
      if (a_exists) {
        pthread_mutex_lock(thread->kvs_mutex); // Lock the k-v store mutex
        // --------------------------------------------------------------------
        // Begin critical section

        status = key_value_store_key_check(thread->kvstore, var_a);

        if (status == 0) {
          status = key_value_store_delete_key(thread->kvstore, var_a);
        }

        if (status == 0) {
          status = log_delete_key(var_a, *(thread->logfd));
        }

        // End critical section
        // --------------------------------------------------------------------
        pthread_mutex_unlock(thread->kvs_mutex); // Unlock the k-v store mutex
      }
    }

    memset(thread->buffer, 0, BUFFER_SIZE);
    set_header(thread->buffer, identifier, status);
    send(connfd, thread->buffer, 5, 0);
  } else if (function == 0x0201) { /* Read */
    filename_length = recv_uint16(connfd);
    filename = (uint8_t *)calloc(filename_length + 1, sizeof(uint8_t));
    bytes_received = recv_string(connfd, filename, filename_length, 1);
    offset = recv_uint64(connfd);
    buff_size = recv_uint16(connfd);
    file_size = filesize((char *)filename);
    bytes_read = 0;
    bytes_remaining = buff_size;

    if (buff_size <= file_size) {
      status = 0;
      uint8_t count = 1;

      if (buff_size > BUFFER_SIZE) {
        data = (uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t));

        do {
          memset(data, 0, BUFFER_SIZE);

          if (bytes_remaining > BUFFER_SIZE) {
            bytes_read = read((char *)filename, offset, BUFFER_SIZE, data);

            if (bytes_read > 0) {
              bytes_remaining -= BUFFER_SIZE;
              offset += BUFFER_SIZE;
            }
          } else {
            bytes_read = read((char *)filename, offset, bytes_remaining, data);
            bytes_remaining = 0;
          }

          if (bytes_read < 0) {
            status = -bytes_read;
          }

          memset(thread->buffer, 0, BUFFER_SIZE);

          if (count == 1) {
            set_header(thread->buffer, identifier, status);

            if (status == 0) {
              set_num_bytes(thread->buffer, buff_size);
              send(connfd, thread->buffer, 7, 0);
              memset(thread->buffer, 0, BUFFER_SIZE);
              set_string(thread->buffer, data, bytes_read);
              send(connfd, thread->buffer, bytes_read, 0);
            } else {
              send(connfd, thread->buffer, 5, 0);
              bytes_remaining = 0;
            }
          } else {
            set_string(thread->buffer, data, bytes_read);
            send(connfd, thread->buffer, bytes_read, 0);
          }

          count++;
        } while (bytes_remaining > 0 && bytes_received != 0);
      } else {
        data = (uint8_t *)calloc(buff_size, sizeof(uint8_t));
        memset(data, 0, buff_size);
        bytes_read = read((char *)filename, offset, buff_size, data);

        if (bytes_read < 0) {
          status = -bytes_read;
        }

        memset(thread->buffer, 0, BUFFER_SIZE);

        if (count == 1) {
          set_header(thread->buffer, identifier, status);

          if (status == 0) {
            set_num_bytes(thread->buffer, buff_size);
            send(connfd, thread->buffer, 7, 0);
            memset(thread->buffer, 0, BUFFER_SIZE);
            set_string(thread->buffer, data, bytes_read);
            send(connfd, thread->buffer, bytes_read, 0);
          } else {
            send(connfd, thread->buffer, 5, 0);
            bytes_remaining = 0;
          }
        } else {
          set_string(thread->buffer, data, bytes_read);
          send(connfd, thread->buffer, bytes_read, 0);
        }

        count++;
      }
    } else {
      status = EINVAL;
      memset(thread->buffer, 0, BUFFER_SIZE);
      set_header(thread->buffer, identifier, status);
      send(connfd, thread->buffer, 5, 0);
    }

    free(filename);
    filename = NULL;
  } else if (function == 0x0202) { /* Write */
    filename_length = recv_uint16(connfd);
    filename = (uint8_t *)calloc(filename_length + 1, sizeof(uint8_t));
    bytes_received = recv_string(connfd, filename, filename_length, 1);
    offset = recv_uint64(connfd);
    buff_size = recv_uint16(connfd);
    bytes_written = 0;
    bytes_remaining = buff_size;

    if (buff_size > BUFFER_SIZE) {
      data = (uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t));

      do {
        memset(data, 0, BUFFER_SIZE);

        if (bytes_remaining > BUFFER_SIZE) {
          bytes_received = recv_string(connfd, data, BUFFER_SIZE, 0);
          bytes_written = write((char *)filename, offset, BUFFER_SIZE, data);

          if (bytes_written > 0) {
            bytes_remaining -= BUFFER_SIZE;
            offset += BUFFER_SIZE;
          }
        } else {
          bytes_received = recv_string(connfd, data, bytes_remaining, 1);
          bytes_written =
              write((char *)filename, offset, bytes_remaining, data);
          bytes_remaining = 0;
        }

        if (bytes_written < 0) {
          status = -bytes_written;
        }
      } while (bytes_remaining > 0 && bytes_received != 0 && status == 0);

      if (data != NULL) {
        free(data);
      }
    } else {
      data = (uint8_t *)calloc(buff_size, sizeof(uint8_t));
      memset(data, 0, buff_size);
      bytes_received = recv_string(connfd, data, buff_size, 1);
      bytes_written = write((char *)filename, offset, buff_size, data);

      if (bytes_written < 0) {
        status = -bytes_written;
      }

      if (data != NULL) {
        free(data);
      }
    }

    memset(thread->buffer, 0, BUFFER_SIZE);
    set_header(thread->buffer, identifier, status);
    send(connfd, thread->buffer, 5, 0);
    free(filename);
    filename = NULL;
  } else if (function == 0x0210) { /* Create */
    filename_length = recv_uint16(connfd);
    filename = (uint8_t *)calloc(filename_length + 1, sizeof(uint8_t));
    recv_string(connfd, filename, filename_length, 1);
    status = create((char *)filename);
    memset(thread->buffer, 0, BUFFER_SIZE);
    set_header(thread->buffer, identifier, status);
    send(connfd, thread->buffer, 5, 0);

    free(filename);
    filename = NULL;
  } else if (function == 0x0220) { /* File size */
    filename_length = recv_uint16(connfd);
    filename = (uint8_t *)calloc(filename_length + 1, sizeof(uint8_t));
    recv_string(connfd, filename, filename_length, 1);
    result = filesize((char *)filename);

    if (result < 0) {
      status = -result;
    } else {
      file_size = result;
    }

    memset(thread->buffer, 0, BUFFER_SIZE);
    set_header(thread->buffer, identifier, status);

    if (status == 0) {
      set_file_size(thread->buffer, file_size);
    }

    send(connfd, thread->buffer, 13, 0);
    free(filename);
    filename = NULL;
  } else if (function == 0x0301) { /* Dump key-value store */
    filename_length = recv_uint16(connfd);
    filename = (uint8_t *)calloc(filename_length + 1, sizeof(uint8_t));
    recv_string(connfd, filename, filename_length, 1);

    pthread_mutex_lock(thread->kvs_mutex); // Lock the key-value store mutex
    // ------------------------------------------------------------------------
    // Begin critical section

    status = dump_key_value_store(thread->kvstore, (char *)filename);

    // End critical section
    // ------------------------------------------------------------------------
    pthread_mutex_unlock(thread->kvs_mutex); // Unlock the k-v store mutex
    
    memset(thread->buffer, 0, BUFFER_SIZE);
    set_header(thread->buffer, identifier, status);
    send(connfd, thread->buffer, 5, 0);
  } else if (function == 0x0302) { /* Load key-value store */
    filename_length = recv_uint16(connfd);
    filename = (uint8_t *)calloc(filename_length + 1, sizeof(uint8_t));
    recv_string(connfd, filename, filename_length, 1);

    pthread_mutex_lock(thread->kvs_mutex); // Lock the key-value store mutex
    // ------------------------------------------------------------------------
    // Begin critical section

    status = load_key_value_store(thread->kvstore, (char *)filename);
    if (status == 0) {
      status = log_key_value_store(thread->kvstore, *(thread->logfd));
    }

    // End critical section
    // ------------------------------------------------------------------------
    pthread_mutex_unlock(thread->kvs_mutex); // Unlock the k-v store mutex

    memset(thread->buffer, 0, BUFFER_SIZE);
    set_header(thread->buffer, identifier, status);
    send(connfd, thread->buffer, 5, 0);
  } else if (function == 0x0310) { /* Clear key-value store */
    magic_number = recv_uint32(connfd);

    if (magic_number == 0x0badbad0) {
      pthread_mutex_lock(thread->kvs_mutex); // Lock the key-value store mutex
      // ----------------------------------------------------------------------
      // Begin critical section

      status = clear_key_value_store(thread->kvstore);
      if (status == 0) {
        status = clear_log(*(thread->dirfd), thread->logfd);
      }

      // End critical section
      // ----------------------------------------------------------------------
      pthread_mutex_unlock(thread->kvs_mutex); // Unlock the k-v store mutex
    } else {
      status = EINVAL;
    }

    memset(thread->buffer, 0, BUFFER_SIZE);
    set_header(thread->buffer, identifier, status);
    send(connfd, thread->buffer, 5, 0);
  }

  return connfd;
}

// Thread loop
void *server_start(void *arg) {
  Thread thread = (Thread)arg;

  while (true) {
    pthread_mutex_lock(thread->main_mutex); // Lock the main mutex
    // ------------------------------------------------------------------------
    // Begin critical section

    // Keep a thread waiting while it is not being used
    while (thread->cl == 0) {
      pthread_cond_wait(&(thread->cv), thread->main_mutex);
    }

    // End critical section
    // ------------------------------------------------------------------------
    pthread_mutex_unlock(thread->main_mutex); // Unlock the main mutex

    memset(thread->buffer, 0, BUFFER_SIZE); // Clear the buffer
    
    // Get 6 bytes from the client
    ssize_t bytes_read = recv_loop(thread->cl, thread->buffer, 6);

    // Continute to process requests while they exist
    while (bytes_read == 6) {
      server_run(thread->cl, thread); // Process the incoming request
      memset(thread->buffer, 0, BUFFER_SIZE); // Clear the buffer
      // Get 6 bytes from the client
      bytes_read = recv_loop(thread->cl, thread->buffer, 6);
    }

    thread->cl = 0; // Set the thread status to not waiting

    pthread_mutex_lock(thread->main_mutex); // Lock the main mutex
    // ------------------------------------------------------------------------
    // Begin critical section

    enqueue(thread->queue, thread->id); // Enqueue the current thread id

    // End critical section
    // ------------------------------------------------------------------------
    pthread_mutex_unlock(thread->main_mutex); // Unlock the main mutex

    sem_post(thread->dispatch_lock); // Signal the dispatch lock
  }

  return 0;
}
