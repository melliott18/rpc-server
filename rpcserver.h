#ifndef __RPCSERVER_H__
#define __RPCSERVER_H__

#include "rpckeyvaluestore.h"
#include "rpcqueue.h"
#include <cstdint>
#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE 4096

typedef struct ThreadObj {
  uint8_t buffer[BUFFER_SIZE];
  int cl;
  uint64_t id;
  uint64_t iterations;
  int *dirfd;
  int *logfd;
  KeyValueStore kvstore;
  Queue queue;
  pthread_cond_t cv;
  pthread_mutex_t *main_mutex;
  pthread_mutex_t *kvs_mutex;
  sem_t *dispatch_lock;
} ThreadObj;

typedef struct ThreadObj *Thread;

int server_connect(char *hostname, uint16_t port);

int server_run(int connfd, Thread thread);

void * server_start(void *arg);

#endif