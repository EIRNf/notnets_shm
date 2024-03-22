
// Includes
#include "queue.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

spsc_queue_header* get_queue_header(void* shmaddr) {
    spsc_queue_header* header = (spsc_queue_header*) shmaddr;
    return header;
}

void* get_message_array(spsc_queue_header* header) {
    return (char*) header + sizeof(spsc_queue_header);
}


ssize_t queue_create(void* shmaddr, size_t shm_size, size_t message_size) {
    if (message_size + (size_t) sizeof(spsc_queue_header) > shm_size) {
        perror("Message size type + Header data too large for allocated shm");
        exit(1);
    }

    // Calculate size of array
    int num_elements = (shm_size - sizeof(spsc_queue_header)) / message_size;
    int leftover_bytes = (shm_size - sizeof(spsc_queue_header)) % message_size;

    spsc_queue_header* header_ptr = get_queue_header(shmaddr); 
    header_ptr->head = 0;
    header_ptr->tail = 0;
    header_ptr->message_size = message_size;
    header_ptr->message_offset = 0;
    header_ptr->queue_size = num_elements;
    header_ptr->current_count = 0;
    header_ptr->total_count = 0;
    header_ptr->stop_consumer_polling = false;
    header_ptr->stop_producer_polling = false;
    atomic_thread_fence(memory_order_seq_cst);

    return shm_size - leftover_bytes;
}

bool is_full(spsc_queue_header *header) {
    atomic_thread_fence(memory_order_acquire);
    return (header->tail + 1) % header->queue_size == header->head;
}

void enqueue(spsc_queue_header* header, const void* buf, size_t buf_size) {
    void* array_start = get_message_array(header);

    int message_size = header->message_size;
    int tail = header->tail;

    memcpy((char*) array_start + tail*message_size, buf, buf_size);

    atomic_thread_fence(memory_order_release);
    
    header->current_count++;
    header->total_count++;

    // TODO: fence?

    header->tail = (header->tail + 1) % header->queue_size;
    atomic_thread_fence(memory_order_release);

    // atomic_thread_fence(memory_order_seq_cst);
}

int push(void* shmaddr, const void* buf, size_t buf_size) {
    spsc_queue_header* header = get_queue_header(shmaddr);
    //Get Constants
    int message_size = header->message_size;

    if (buf_size > header->message_size) {
        return -1;
    }

    int const writer_head = atomic_load_explicit(&header->head, memory_order_relaxed);
    int next_writer_head = writer_head + 1;
    if (next_writer_head == header->queue_size){ //Wrap around
        next_writer_head = 0;
    }

    while(next_writer_head == atomic_load_explicit(&header->tail, memory_order_acquire)){ 
        //Poll if full
    }

    //Enqueue
    void* array_start = get_message_array(header);
    memmove((char*)array_start + writer_head * message_size, buf, buf_size);
    
    atomic_store_explicit(&header->head, next_writer_head, memory_order_release);

    return 0;
}

bool is_empty(spsc_queue_header *header) {
    atomic_thread_fence(memory_order_acquire);
    return header->head == header->tail;
}


size_t dequeue(spsc_queue_header* header, void* buf, size_t* buf_size) {
    void* array_start = get_message_array(header);


    if(*buf_size > header->message_size+1){
        printf("not supposed to happen");
    }

    // handle offset math
    if (*buf_size + header->message_offset > header->message_size) {
        *buf_size = header->message_size - header->message_offset;
    }

    memcpy(
        buf,
        (char*) array_start + header->head*header->message_size + header->message_offset,
        *buf_size
    );
    atomic_thread_fence(memory_order_release);

    header->message_offset += *buf_size;

    if ((size_t) header->message_offset == header->message_size) {
        header->head = (header->head + 1) % header->queue_size;
        header->current_count--;
        header->message_offset = 0;
    }
    atomic_thread_fence(memory_order_release);

    return header->message_offset;

}

size_t pop(void* shmaddr, void* buf, size_t* buf_size) {
    spsc_queue_header* header = get_queue_header(shmaddr);
    size_t read = 0;

    // handle offset math for partial read
    if (*buf_size + header->message_offset > header->message_size) {
        *buf_size = header->message_size - header->message_offset;
    }

    if (header->stop_consumer_polling) {
            return 0;
    }

    int const reader_tail = atomic_load_explicit(&header->tail, memory_order_relaxed);
    while (reader_tail == atomic_load_explicit(&header->head, memory_order_acquire)){
        //queue is empty
    }

    int next_reader_tail = reader_tail + 1;
    if (next_reader_tail == header->queue_size){ //Wrap around
        next_reader_tail = 0;
    }

    //dequeue, TODO, Partial writes
    void* array_start = get_message_array(header);
    memmove(
        buf,
        (char*) array_start + (reader_tail * header->message_size) + header->message_offset,
        *buf_size
    );

    // printf("%p\n",(char*) array_start + (header->tail * header->message_size) + header->message_offset);
    // atomic_thread_fence(memory_order_release); //TODO

    header->message_offset += *buf_size;
    //Handle full read
    if ((size_t) header->message_offset == header->message_size) {
        header->message_offset = 0;
        read = 0;
    }
    else { //Handle partial read, decrement next record bringing it back to reader_tail value
        next_reader_tail = reader_tail;
        read = header->message_offset;
    }
    atomic_store_explicit(&header->tail, next_reader_tail, memory_order_release);
    // atomic_thread_fence(memory_order_release); //TODO

    return read;
}


const void* peek(void* shmaddr, ssize_t *size) {
    spsc_queue_header* header = get_queue_header(shmaddr);

    int message_size = header->message_size;
    int queue_head = header->head;

    void* array_start = get_message_array(header);

    *size = header->message_size;
    return (const void*) ((char*) array_start + queue_head*message_size);
}


