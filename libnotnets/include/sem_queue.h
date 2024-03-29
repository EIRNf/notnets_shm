#ifndef __SEM_QUEUE_H
#define __SEM_QUEUE_H

#include <sys/types.h>
#include <stdatomic.h>
#include <sys/sem.h>

#include "coord.h"

typedef struct sem_spsc_queue_header {
    atomic_int head;
    atomic_int tail;
    size_t message_size;
    int message_offset;
    int queue_size;
    int current_count;
    int total_count;
    bool stop_producer_polling; //TODO: Need to update stop function if in the middle of communication
    bool stop_consumer_polling;

    int sem_lock;
    int sem_full;
    int sem_empty;
} sem_spsc_queue_header;



void destroy_semaphores(coord_header *header, int slot);

shm_pair _hash_sem_shm_allocator(int message_size, int client_id);


/*
 * @brief Casts beginning of provided shmaddr as a spsc header, and returns it.
 *
 * @param shmaddr pointer to shm region
 * @return pointer to header struct of shm region
  */
sem_spsc_queue_header* get_sem_queue_header(void* shmaddr);

void* get_message_array(sem_spsc_queue_header* header);

/**
 * @brief spsc queue creator. Verifies size limits of shm, sets up header.
 *
 * @param shmaddr pointer to shm region
 * @param shm_size total allocated size for shm
 * @param message_size size (in bytes) of a message. Assume constant message
 * size for this queue
 * @return size of usable shm region space
 **/
ssize_t queue_create(void* shmaddr, key_t semkey,size_t shm_size, size_t message_size);


/*
 * @brief checks if queue is full based on its head and tail positions
 *
 * @param header pointer to header of shm queue
 * @return boolean of whether queue is full
  */
bool is_full(sem_spsc_queue_header *header);

/*
 * @brief enqueues buffer to queue
 *
 * @param header pointer to queue header
 * @param buf pointer to buffer we want to enqueue
 * @param buf_size size of the buf in bytes
  */
void enqueue(sem_spsc_queue_header* header, const void* buf, size_t buf_size);

int sem_push(void* shmaddr, const void* buf, size_t buf_size);

/*
 * @brief returns whether queue is empty
 *
 * @param header pointer to queue header
 * @returns boolean of true if queue is empty, false otherwise
 */
bool is_empty(sem_spsc_queue_header *header);

/*
 * @brief dequeues a message from the queue
 *
 * @param header pointer to queue header
 * @returns dequeued message in a malloced buffer
 * 
 * if return 0 all has been read for the message, else return read amount
 */
size_t dequeue(sem_spsc_queue_header* header, void* buf, size_t* buf_size);
/*
 * @brief polling sem_pop, will wait until it can sem_pop from queue
 *
 * @param shmaddr pointer to shm region
 * @param buf buffer that will eventually store the popped element
 * @param buf_size pointer to a numerical value that will be populated with size
 *                 of returned messaged
 */
size_t sem_pop(void* shmaddr, void* buf, size_t* buf_size);
/*
 * @brief returns a const pointer to the head of the queue
 *
 * @param shmaddr pointer to shm region
 * @param size pointer to size of peeked message (will be populated)
 * @returns pointer to the head of queue
  */
const void* sem_peek(void* shmaddr, ssize_t *size);

#endif
