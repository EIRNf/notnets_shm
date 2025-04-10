#ifndef __QUEUE_H
#define __QUEUE_H

// Includes
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>



typedef struct spsc_queue_header {
    atomic_int head;
    atomic_int tail;
    size_t message_size;
    int message_offset;
    int queue_size;
    int current_count;
    int total_count;
    bool stop_producer_polling;
    bool stop_consumer_polling;
} spsc_queue_header;

/*
 * @brief Casts beginning of provided shmaddr as a spsc header, and returns it.
 *
 * @param shmaddr pointer to shm region
 * @return pointer to header struct of shm region
  */
spsc_queue_header* get_queue_header(void* shmaddr);

void* get_message_array(spsc_queue_header* header);

/**
 * @brief spsc queue creator. Verifies size limits of shm, sets up header.
 *
 * @param shmaddr pointer to shm region
 * @param shm_size total allocated size for shm
 * @param message_size size (in bytes) of a message. Assume constant message
 * size for this queue
 * @return size of usable shm region space
 **/
ssize_t queue_create(void* shmaddr, size_t shm_size, size_t message_size);


/*
 * @brief checks if queue is full based on its head and tail positions
 *
 * @param header pointer to header of shm queue
 * @return boolean of whether queue is full
  */
bool is_full(spsc_queue_header *header);

/*
 * @brief enqueues buffer to queue
 *
 * @param header pointer to queue header
 * @param buf pointer to buffer we want to enqueue
 * @param buf_size size of the buf in bytes
  */
void enqueue(spsc_queue_header* header, const void* buf, size_t buf_size);

int push(void* shmaddr, const void* buf, size_t buf_size);

/*
 * @brief returns whether queue is empty
 *
 * @param header pointer to queue header
 * @returns boolean of true if queue is empty, false otherwise
 */
bool is_empty(spsc_queue_header *header);

/*
 * @brief dequeues a message from the queue
 *
 * @param header pointer to queue header
 * @returns dequeued message in a malloced buffer
 * 
 * if return 0 all has been read for the message, else return read amount
 */
size_t dequeue(spsc_queue_header* header, void* buf, size_t* buf_size);
/*
 * @brief polling pop, will wait until it can pop from queue
 *
 * @param shmaddr pointer to shm region
 * @param buf buffer that will eventually store the popped element
 * @param buf_size pointer to a numerical value that will be populated with size
 *                 of returned messaged
 */
size_t pop(void* shmaddr, void* buf, size_t* buf_size);
/*
 * @brief returns a const pointer to the head of the queue
 *
 * @param shmaddr pointer to shm region
 * @param size pointer to size of peeked message (will be populated)
 * @returns pointer to the head of queue
  */
const void* peek(void* shmaddr, ssize_t *size);

#endif
