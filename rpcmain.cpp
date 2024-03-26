#include "rpcconvert.h"
#include "rpcfile.h"
#include "rpcio.h"
#include "rpckeyvaluestore.h"
#include "rpcmath.h"
#include "rpcqueue.h"
#include "rpcserver.h"
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

#define SERVER_NAME_STRING "localhost"
#define PORT_NUMBER 8912
#define DIR_NAME "data"
#define BUFFER_SIZE 4096
#define OPTIONS "H:N:I:d:"

int main(int argc, char *argv[]) {
  int64_t option = 0;
  int sockfd = 0;
  int connfd = 0;
  char *hostname = strdup(SERVER_NAME_STRING);
  uint16_t port = PORT_NUMBER;
  char *ptr = NULL;
  char *hostportstr = NULL;
  char *sizestr = NULL;
  char *nthreadsstr = NULL;
  char *iterationsstr = NULL;
  char *dir_path = strdup(DIR_NAME);
  int dirfd = 0;
  int logfd = 0;
  long num;
  uint64_t size = 32;
  uint8_t nthreads = 4;
  uint64_t iterations = 50;

  // getopt()
  while ((option = getopt(argc, argv, OPTIONS)) != -1) {
    switch (option) {
    case 'H': // Sets the number of linked lists in the hash table
      sizestr = (char *)calloc(strlen(optarg), sizeof(char));
      strcpy(sizestr, optarg);
      size = strtol(sizestr, &ptr, 10);
      break;
    case 'N': // Sets the number of client threads
      nthreadsstr = (char *)calloc(strlen(optarg), sizeof(char));
      strcpy(nthreadsstr, optarg);
      nthreads = strtol(nthreadsstr, &ptr, 10);
      break;
    case 'I': // Sets the number of recursive resolution iterations
      iterationsstr = (char *)calloc(strlen(optarg), sizeof(char));
      strcpy(iterationsstr, optarg);
      iterations = strtol(iterationsstr, &ptr, 10);
      break;
    case 'd': // Sets the scratch directory path for the server
      dir_path = (char *)calloc(strlen(optarg), sizeof(char));
      strcpy((char *)dir_path, optarg);
      break;
    default:
      fprintf(stderr, "usage: ./rpcserver [hostname:port] -H size -N nthreads "
                      "-I iterations -d dir\n");
      exit(EXIT_FAILURE);
    }
  }

  // Get the hostname and port
  for (; optind < argc; optind++) {
    if (isnumber(argv[optind])) {
      hostportstr = (char *)calloc(strlen(argv[optind]), sizeof(char));
      strcpy(hostportstr, argv[optind]);
    } else {
      hostname = strtok(argv[optind], ":");
      if (hostname == NULL) {
        fprintf(stderr, "usage: ./rpcserver [hostname:port] -H size -N "
                        "nthreads -I iterations -d dir\n");
        exit(EXIT_FAILURE);
      }

      hostportstr = strtok(NULL, "\n");
    }

    if (hostportstr != NULL) {
      errno = 0;
      num = strtol(hostportstr, &ptr, 10);

      if (*hostportstr == '\0' || ptr == hostportstr) {
        fprintf(stderr, "rpcserver: invalid port number\n");
        exit(EXIT_FAILURE);
      } else if (errno == ERANGE || num <= 1024 || num > 65535) {
        fprintf(stderr, "rpcserver: invalid port number\n");
        exit(EXIT_FAILURE);
      }

      port = (uint16_t)num;
    }
  }

  pthread_mutex_t main_mutex; // Main mutex
  pthread_mutex_t kvs_mutex;  // Key-value store mutex
  sem_t dispatch_lock;        // Dispatch lock semaphore

  // Open the directory specified on the command line
  // If a directory is not specified then open directory data
  if ((dirfd = open(dir_path, O_DIRECTORY | O_PATH)) == -1) {
    err(2, "failed to open directory");
  }

  // Open the log file in the specified directory
  // If the log file does not already exist then create a new one
  if ((logfd = openat(dirfd, "log.txt", O_CREAT | O_RDWR, 0644)) == -1) {
    err(2, "failed to open log file");
  }

  KeyValueStore kvstore = create_key_value_store(size); // Key-value store
  load_log(kvstore, logfd);     // Load saved variables from the log file
  Queue queue = create_queue(); // Thread queue

  Thread threads[nthreads]; // Thread array
  Thread thread;            // Thread object
  pthread_t threadPointer;  // THread pointer
  uint64_t i;

  // Initialize threads
  for (i = 0; i < nthreads; i++) {
    // Create a new array of thread objects
    threads[i] = (ThreadObj *)malloc(sizeof(ThreadObj));
    thread = threads[i];
    thread->cl = 0;
    thread->id = i;
    thread->iterations = iterations;
    thread->dirfd = &dirfd;
    thread->logfd = &logfd;
    thread->kvstore = kvstore;
    thread->queue = queue;
    thread->cv = PTHREAD_COND_INITIALIZER;
    thread->main_mutex = &main_mutex;
    thread->kvs_mutex = &kvs_mutex;
    thread->dispatch_lock = &dispatch_lock;
    pthread_mutex_init(thread->main_mutex, NULL); // Initialize the main mutex
    pthread_mutex_init(thread->kvs_mutex, NULL); // Init the k-v store mutex
    
    // Initialize the dispatch lock semaphore
    if (sem_init(thread->dispatch_lock, 0, nthreads)) {
      err(2, "sem_init for thread");
    }

    enqueue(thread->queue, i); // Add the worker thread id to the thread queue

    // Create a new worker thread
    if (pthread_create(&threadPointer, 0, server_start, thread)) {
      err(2, "pthread_create");
    }
  }

  // Set up the socket connection
  if ((sockfd = server_connect(hostname, port)) < 0) {
    err(1, "failed listening");
  }

  while (true) {
    connfd = accept(sockfd, NULL, NULL); // Accept incoming client connections
    sem_wait(thread->dispatch_lock); // Make the dispatch lock wait
    
    pthread_mutex_lock(thread->main_mutex); // Lock the main mutex
    // ------------------------------------------------------------------------
    // Begin critical section

    i = dequeue(thread->queue); // Get first available thread id
    thread = threads[i]; // Get the ith thread in the thread array
    thread->cl = connfd; // Assign the connection file descriptor to the thread
    pthread_cond_signal(&(thread->cv)); // Signal the current thread

    // End critical section
    // ------------------------------------------------------------------------
    pthread_mutex_unlock(thread->main_mutex); // Unlock the main mutex
  }

  return EXIT_SUCCESS;
}
