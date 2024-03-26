#include "rpcqueue.h"
#include <cstdint>
#include <cstdlib>

typedef struct QueueNodeObj {
  int64_t value;
  struct QueueNodeObj *next;
} QueueNodeObj;

typedef QueueNodeObj *Node;

typedef struct QueueObj {
  uint64_t num_items;
  Node front;
  Node back;
} QueueObj;

// Create a new node
Node create_node(int64_t value) {
  Node node = (QueueNodeObj *)malloc(sizeof(QueueNodeObj));
  if (node != NULL) {
    node->value = value;
    node->next = NULL;
  }
  return node;
}

// Delete a node
void delete_node(Node *ptr) {
  if (ptr != NULL && *ptr != NULL) {
    free(*ptr);
    *ptr = NULL;
  }
  return;
}

// Find a node in a queue
Node find_node(Queue queue, int64_t value) {
  if (queue == NULL) {
    return NULL;
  }
  Node node = queue->front;
  while (node != NULL) {
    if (node->value == value) {
      return node;
    }
    node = node->next;
  }
  return node;
}

// Create a new queue
Queue create_queue() {
  Queue queue = (QueueObj *)malloc(sizeof(QueueObj));
  if (queue != NULL) {
    queue->front = NULL;
    queue->back = NULL;
    queue->num_items = 0;
  }
  return queue;
}

// Delete a queue
void delete_queue(Queue *ptr) {
  if (ptr != NULL && *ptr != NULL) {
    Queue queue = *ptr;
    Node front = queue->front;
    while (front != NULL) {
      front = front->next;
      delete_node(&front);
    }
    free(queue);
    queue = NULL;
  }
  return;
}

// Check if a queue is empty
int8_t queue_is_empty(Queue queue) {
  if (queue == NULL) {
    return -1;
  }
  return (queue->num_items == 0);
}

// Enqueue a value into a queue
void enqueue(Queue queue, int64_t value) {
  if (queue == NULL) {
    return;
  }
  if (find_node(queue, value) != NULL) {
    return;
  }
  Node node = create_node(value);
  if (node == NULL) {
    return;
  }
  if (queue_is_empty(queue)) {
    node->next = queue->front;
    node->next = queue->back;
    queue->front = node;
    queue->back = node;
  } else {
    queue->back->next = node;
    queue->back = node;
  }
  (queue->num_items)++;
  return;
}

// Dequeue a value from a queue
int64_t dequeue(Queue queue) {
  if (queue == NULL) {
    return -1;
  }
  Node front;
  int64_t value;
  if (queue_is_empty(queue)) {
    return -1;
  }
  if ((queue->num_items) == 1) {
    front = queue->front;
    value = front->value;
    delete_node(&front);
    queue->front = NULL;
    queue->back = NULL;
  } else {
    front = queue->front;
    value = front->value;
    queue->front = queue->front->next;
    delete_node(&front);
  }
  (queue->num_items)--;
  return value;
}
