#ifndef __QUEUE_H
#define __QUEUE_H

// Includes
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct spsc_queue_header {
    int head;
    int tail;
    size_t message_size;
    int message_offset;
    int queue_size;
    int current_count;
    int total_count;
    bool stop_producer_polling;
    bool stop_consumer_polling;
    void* message_array;
} spsc_queue_header;

/*
 * @brief Casts beginning of provided shmaddr as a spsc header, and returns it.
 *
 * @param shmaddr pointer to shm region
 * @return pointer to header struct (first 24 bytes) of shm region
  */
spsc_queue_header* get_queue_header(void* shmaddr) {
    spsc_queue_header* header = (spsc_queue_header*) shmaddr;
    return header;
}


/**
 * @brief spsc queue creator. Verifies size limits of shm, sets up header.
 *
 * @param shmaddr pointer to shm region
 * @param shm_size total allocated size for shm
 * @param message_size size (in bytes) of a message. Assume constant message
 * size for this queue
 * @return size of usable shm region space
 **/
ssize_t queue_create(void* shmaddr, size_t shm_size, size_t message_size) {
    if (message_size + sizeof(spsc_queue_header) > (u_long) shm_size) {
        perror("Message size type + Header data too large for allocated shm");
        exit(1);
    }

    // Calculate size of array
    int num_elements = (shm_size - sizeof(spsc_queue_header)) / message_size;
    int leftover_bytes = (shm_size - sizeof(spsc_queue_header)) % message_size;

    spsc_queue_header header;
    header.head = 0;
    header.tail = 0;
    header.message_size = message_size;
    header.message_offset = 0;
    header.queue_size = num_elements;
    header.current_count = 0;
    header.total_count = 0;
    header.stop_consumer_polling = false;
    header.stop_producer_polling = false;
    char* temp_addr = (char*) shmaddr;
    header.message_array = temp_addr + sizeof(spsc_queue_header);

    spsc_queue_header* header_ptr = get_queue_header(shmaddr);
    *header_ptr = header;

    return shm_size - leftover_bytes;
}


/*
 * @brief checks if queue is full based on its head and tail positions
 *
 * @param header pointer to header of shm queue
 * @return boolean of whether queue is full
  */
bool is_full(spsc_queue_header *header) {
    return (header->tail + 1) % header->queue_size == header->head;
}

/*
 * @brief enqueues buffer to queue
 *
 * @param header pointer to queue header
 * @param buf pointer to buffer we want to enqueue
 * @param buf_size size of the buf in bytes
  */
void enqueue(spsc_queue_header* header, const void* buf, size_t buf_size) {
    void* array_start = header->message_array;

    int message_size = header->message_size;
    int tail = header->tail;

    memcpy(array_start + tail*message_size, buf, buf_size);

    header->current_count++;
    header->total_count++;

    // TODO: fence?
    header->tail = (header->tail + 1) % header->queue_size;
}

/*
 * @brief polls queue and enqueues message when available
 *
 * @param shmaddr pointer to shm region
 * @param buf pointer to a buffer, i.e. the message content
 * @param buf_size size of the buffer content
 * @return 0 if successfully pushed, -1 otherwise
  */
int push(void* shmaddr, const void* buf, size_t buf_size) {
    spsc_queue_header* header = get_queue_header(shmaddr);

    if (buf_size > header->message_size) {
        return -1;
    }

    while (true) {
        if (header->stop_producer_polling) {
            return -1;
        }

        if (is_full(header)) {
            continue;
        } else {
            enqueue(header, buf, buf_size);
            break;
        }
    }

    return 0;
}

/*
 * @brief returns whether queue is empty
 *
 * @param header pointer to queue header
 * @returns boolean of true if queue is empty, false otherwise
 */
bool is_empty(spsc_queue_header *header) {
    return header->head == header->tail;
}

/*
 * @brief dequeues a message from the queue
 *
 * @param header pointer to queue header
 * @returns dequeued message in a malloced buffer
 */
void dequeue(spsc_queue_header* header, void* buf, size_t* buf_size) {
    void* array_start = header->message_array;

    // handle offset math
    if (*buf_size + header->message_offset > header->message_size) {
        *buf_size = header->message_size - header->message_offset;
    }

    memcpy(
        buf,
        array_start + header->head*header->message_size + header->message_offset,
        *buf_size
    );
    header->message_offset += *buf_size;

    if ((size_t) header->message_offset == header->message_size) {
        header->head = (header->head + 1) % header->queue_size;
        header->current_count--;
        header->message_offset = 0;
    }
}

/*
 * @brief polling pop, will wait until it can pop from queue
 *
 * @param shmaddr pointer to shm region
 * @param buf_size pointer to a numerical value that will be populated with size
 *                 of returned messaged
 * @return message buffer popped from queue
 */
void pop(void* shmaddr, void* buf, size_t* buf_size) {
    spsc_queue_header* header = get_queue_header(shmaddr);

    while (true) {
        if (header->stop_consumer_polling) {
            return;
        }

        if (is_empty(header)) {
            continue;
        } else {
            dequeue(header, buf, buf_size);
            break;
        }
    }
}

/*
 * @brief returns a const pointer to the head of the queue
 *
 * @param shmaddr pointer to shm region
 * @param size pointer to size of peeked message (will be populated)
 * @returns pointer to the head of queue
  */
const void* peek(void* shmaddr, ssize_t *size) {
    spsc_queue_header* header = get_queue_header(shmaddr);

    int message_size = header->message_size;
    int queue_head = header->head;

    void* array_start = header->message_array;

    *size = header->message_size;
    return (const void*) (array_start + queue_head*message_size);
}

#endif
