
// Includes
#include "queue.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

void stop_producer_polling(void *shmaddr)
{
    spsc_queue_header* header = (spsc_queue_header*) shmaddr;
    header->stop_producer_polling = true;
}

void stop_consumer_polling(void *shmaddr)
{
    spsc_queue_header* header = (spsc_queue_header*) shmaddr;
    header->stop_consumer_polling = true;
}

spsc_queue_header *get_queue_header(void *shmaddr)
{
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
    // Always add size_t to message size to store message length
    message_size += sizeof(size_t);

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
    return 0;
}

void enqueue(spsc_queue_header* header, const void* buf, size_t buf_size) {
}

int push(void* shmaddr, const void* buf, size_t buf_size) {
    spsc_queue_header* header = get_queue_header(shmaddr);
    //Get Constants
    int message_size = header->message_size;

    if (buf_size + sizeof(long) > message_size) {
        return -2; // Message too large for slot
    }

    int const writer_head = atomic_load_explicit(&header->head, memory_order_relaxed);
    int next_writer_head = writer_head + 1;
    if (next_writer_head == header->queue_size){ //Wrap around
        next_writer_head = 0;
    }

    while(next_writer_head == atomic_load_explicit(&header->tail, memory_order_acquire)){ 
        //Poll if full
        if (header->stop_producer_polling) { //Timeout or shutdown return -1 
            header->stop_producer_polling = false;
            return -1;
        }
    }

    //Enqueue
    void* array_start = get_message_array(header);
    char* slot_position = (char*) array_start + writer_head * message_size;
    memcpy(slot_position , &buf_size, sizeof(size_t));
    memcpy(slot_position+ sizeof(size_t), buf, buf_size);
    
    atomic_store_explicit(&header->head, next_writer_head, memory_order_release);

    return buf_size;
}

bool is_empty(spsc_queue_header *header) {
    return 0;
}


size_t dequeue(spsc_queue_header* header, void* buf, size_t* buf_size) {
    return 0;
}

size_t pop(void* shmaddr, void* buf, size_t* buf_size) { //TODO: MAKE PARTIAL READS THREAD SAFE
    spsc_queue_header* header = get_queue_header(shmaddr);
    size_t read_data = 0;


    int const reader_tail = atomic_load_explicit(&header->tail, memory_order_relaxed);
    while (reader_tail == atomic_load_explicit(&header->head, memory_order_acquire)){
        //queue is empty
        if (header->stop_consumer_polling) {//Timeout or shutdown return -1 
            header->stop_consumer_polling = false;
            return -1;
        }
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


const void* peek(void* shmaddr, ssize_t *size) {
    return 0;
}


