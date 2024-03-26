#include <cstdint>

typedef struct QueueObj *Queue;

Queue create_queue();

void delete_queue(Queue *ptr);

int8_t queue_is_empty(Queue queue);

void enqueue(Queue queue, int64_t value);

int64_t dequeue(Queue queue);
