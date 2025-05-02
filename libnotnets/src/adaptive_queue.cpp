
// Includes
#include "adaptive_queue.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <sched.h>
#include <time.h>
#include <linux/prctl.h>  /* Definition of PR_* constants */
#include <sys/prctl.h>


long measure_sleep_resolution() {
    struct timespec nano_sleep, start, end;
    nano_sleep.tv_sec = 0;
    nano_sleep.tv_nsec = 1 ;
    clock_gettime(CLOCK_MONOTONIC, &start);
    clock_nanosleep(CLOCK_MONOTONIC, 0, &nano_sleep, NULL);
    clock_gettime(CLOCK_MONOTONIC, &end);
    return (end.tv_nsec - start.tv_nsec);
  }

spsc_adaptive_queue_header* get_adaptive_queue_header(void* shmaddr) {
    spsc_adaptive_queue_header* header = (spsc_adaptive_queue_header*) shmaddr;
    return header;
}

void* get_message_array(spsc_adaptive_queue_header* header) {
    return (char*) header + sizeof(spsc_adaptive_queue_header);
}


ssize_t adaptive_queue_create(void* shmaddr, size_t shm_size, size_t message_size) {

    if (message_size + (size_t) sizeof(spsc_adaptive_queue_header) > shm_size) {
        perror("Message size type + Header data too large for allocated shm");
        exit(1);
    }
    // Always add size_t to message size to store message length
    message_size += sizeof(size_t);

    // Calculate size of array
    int num_elements = (shm_size - sizeof(spsc_adaptive_queue_header)) / message_size;
    int leftover_bytes = (shm_size - sizeof(spsc_adaptive_queue_header)) % message_size;

    spsc_adaptive_queue_header* header_ptr = get_adaptive_queue_header(shmaddr); 
    header_ptr->head = 0;
    header_ptr->tail = 0;
    header_ptr->message_size = message_size;
    header_ptr->message_offset = 0;
    header_ptr->queue_size = num_elements;
    header_ptr->current_count = 0;
    header_ptr->total_count = 0;
    header_ptr->stop_consumer_polling = false;
    header_ptr->stop_producer_polling = false;
    // header_ptr->nano_sleep_time_step =  measure_sleep_resolution()  / 10; // Estimate best polling interval by measuring sleep / 10 to account for priority scheduling 
    header_ptr->nano_sleep_time_step =  1000; // 1us

    
    atomic_thread_fence(memory_order_seq_cst);

    return shm_size - leftover_bytes;
}

bool is_full(spsc_adaptive_queue_header *header) {
    return 0;
}

void enqueue(spsc_adaptive_queue_header* header, const void* buf, size_t buf_size) {
}

int adaptive_push(void* shmaddr, const void* buf, size_t buf_size) {
    spsc_adaptive_queue_header* header = get_adaptive_queue_header(shmaddr);
    //Get Constants
    int message_size = header->message_size;

    if (buf_size + sizeof(long) > message_size) {
        return -1; // Message too large for slot
    }

    int const writer_head = atomic_load_explicit(&header->head, memory_order_relaxed);
    int next_writer_head = writer_head + 1;
    if (next_writer_head == header->queue_size){ //Wrap around
        next_writer_head = 0;
    }

    long sleep_time = 0;
    struct timespec sleep;  
    while(next_writer_head == atomic_load_explicit(&header->tail, memory_order_acquire)){ 
        //Poll if full
        if (header->stop_producer_polling) {
            return 0;
        }
        sleep = {0, sleep_time};
        clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep, NULL);
        sleep_time =  (sleep_time *  1.05) + header->nano_sleep_time_step;

        
        // sched_yield();
    }

    //Enqueue
    void* array_start = get_message_array(header);
    char* slot_position = (char*) array_start + writer_head * message_size;
    memcpy(slot_position , &buf_size, sizeof(size_t));
    memcpy(slot_position+ sizeof(size_t), buf, buf_size);
    
    atomic_store_explicit(&header->head, next_writer_head, memory_order_release);

    return buf_size;
}

bool is_empty(spsc_adaptive_queue_header *header) {
    return 0;
}


size_t dequeue(spsc_adaptive_queue_header* header, void* buf, size_t* buf_size) {
    return 0;
}

size_t adaptive_pop(void* shmaddr, void* buf, size_t* buf_size) { //TODO: MAKE PARTIAL READS THREAD SAFE
    spsc_adaptive_queue_header* header = get_adaptive_queue_header(shmaddr);
    size_t read_data = 0;

    long sleep_time = 0;
    struct timespec sleep  = {0, sleep_time};
    int const reader_tail = atomic_load_explicit(&header->tail, memory_order_relaxed);
    while (reader_tail == atomic_load_explicit(&header->head, memory_order_acquire)){
        //queue is empty
        if (header->stop_consumer_polling) {
            return 0;
        }
        sleep = {0, sleep_time};

        clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep, NULL);
        sleep_time =  (sleep_time *  1.05) + header->nano_sleep_time_step;
    }

    int next_reader_tail = reader_tail + 1;
    if (next_reader_tail == header->queue_size){ //Wrap around
        next_reader_tail = 0;
    }

    //Dequeue
    void* array_start = get_message_array(header);
    char* slot_position = (char*) array_start + reader_tail * header->message_size;
    size_t message_length;

    // Get size of written message
    memcpy(&message_length, slot_position, sizeof(size_t));

    // Handle offset math for full read
    if (*buf_size + header->message_offset > message_length) {
        read_data = message_length - header->message_offset;
    }
    // Handle offset math for partial read
    if (*buf_size + header->message_offset <= message_length) {
        read_data = *buf_size;
    }

    memcpy(
        buf,
        slot_position + sizeof(size_t)+ header->message_offset,
        read_data
    );

    header->message_offset += read_data;

    //Handle full read
    if ((size_t) header->message_offset == message_length) {
        header->message_offset = 0;
    }
    else { //Handle partial read, decrement next record bringing it back to reader_tail value
        next_reader_tail = reader_tail;
    }

    atomic_store_explicit(&header->tail, next_reader_tail, memory_order_release);

    return read_data;
}


const void* adaptive_peek(void* shmaddr, ssize_t *size) {
    return 0;
}


