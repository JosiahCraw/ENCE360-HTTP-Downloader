#include "queue.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)


/*
 * Queue - the abstract type of a concurrent queue.
 * You must provide an implementation of this type 
 * but it is hidden from the outside.
 */
typedef struct QueueStruct {
    sem_t read, write, mutex;
    void **data;
    int head, tail, size;
} Queue;


/**
 * Allocate a concurrent queue of a specific size
 * @param size - The size of memory to allocate to the queue
 * @return queue - Pointer to the allocated queue
 */
Queue *queue_alloc(int size) {
    Queue *queue = (Queue*)malloc(sizeof(Queue));
    queue->data = (void**)malloc(sizeof(void*)*size);
    sem_init(&queue->mutex, 0, 1);
    sem_init(&queue->read, 0, 0);
    sem_init(&queue->write, 0, size-1);
    queue->head = 0;
    queue->tail = 0;

    queue->size = size;

    return queue;
}


/**
 * Free a concurrent queue and associated memory 
 *
 * Don't call this function while the queue is still in use.
 * (Note, this is a pre-condition to the function and does not need
 * to be checked)
 * 
 * @param queue - Pointer to the queue to free
 */
void queue_free(Queue *queue) {
    sem_destroy(&queue->mutex);
    sem_destroy(&queue->read);
    sem_destroy(&queue->write);
    free(queue->data);
    free(queue);
}


/**
 * Place an item into the concurrent queue.
 * If no space available then queue will block
 * until a space is available when it will
 * put the item into the queue and immediatly return
 *  
 * @param queue - Pointer to the queue to add an item to
 * @param item - An item to add to queue. Uses void* to hold an arbitrary
 *               type. User's responsibility to manage memory and ensure
 *               it is correctly typed.
 */
void queue_put(Queue *queue, void *item) {
    sem_wait(&queue->write);
    sem_wait(&queue->mutex);

    *(queue->data+queue->head) = item;

    // Move along circular buffer wrapping at queue->size
    queue->head = (queue->head + 1) % queue->size;
    
    sem_post(&queue->mutex);
    sem_post(&queue->read);
}


/**
 * Get an item from the concurrent queue
 * 
 * If there is no item available then queue_get
 * will block until an item becomes avaible when
 * it will immediately return that item.
 * 
 * @param queue - Pointer to queue to get item from
 * @return item - item retrieved from queue. void* type since it can be 
 *                arbitrary 
 */
void *queue_get(Queue *queue) {
    sem_wait(&queue->read);
    sem_wait(&queue->mutex);

    void* item = *(queue->data+queue->tail);

    // Move along circular buffer wrapping at queue->size
    queue->tail = (queue->tail+1) % queue->size;

    sem_post(&queue->mutex);
    sem_post(&queue->write);

    return item;
}

