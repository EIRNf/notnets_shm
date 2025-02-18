
// Includes
#include "queue.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <sched.h>
#include <time.h>
#include <linux/prctl.h>  /* Definition of PR_* constants */
#include <sys/prctl.h>


#include "stdlib.h"
#include "stdio.h"
#include "string.h"

static unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;

void init(){
    FILE* file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow,
        &lastTotalSys, &lastTotalIdle);
    fclose(file);
}

double getCurrentValue(){
    double percent;
    FILE* file;
    unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;

    file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow,
        &totalSys, &totalIdle);
    fclose(file);

    if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
        totalSys < lastTotalSys || totalIdle < lastTotalIdle){
        //Overflow detection. Just skip this value.
        percent = -1.0;
    }
    else{
        total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
            (totalSys - lastTotalSys);
        percent = total;
        total += (totalIdle - lastTotalIdle);
        percent /= total;
        percent *= 100;
    }

    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;

    return percent;
}


spsc_queue_header* get_queue_header(void* shmaddr) {
    spsc_queue_header* header = (spsc_queue_header*) shmaddr;
    return header;
}

void* get_message_array(spsc_queue_header* header) {
    return (char*) header + sizeof(spsc_queue_header);
}

double update_cpu_util(spsc_queue_header* header) {
    return getCurrentValue();
}

long get_sleep_time(spsc_queue_header* header) {

    if (header->cpu_util < 0.25) {
        return 0; 
    }
    else if (header->cpu_util < 0.5 && header->cpu_util >= 0.25) {
        return 5000;// 5 microseconds
    }
    else {
        return 150000; // 150 microseconds
    }
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
    header_ptr->current_count = 0; //TODO: Fix
    header_ptr->total_count = 0; //TODO: Fix
    header_ptr->stop_consumer_polling = false;
    header_ptr->stop_producer_polling = false;
    header_ptr->nano_sleep_time_step = 1; // 0.001 microseconds
    header_ptr->cpu_util = 0;
    header_ptr->push_cpu_interval_counter = 0;
    header_ptr->pop_cpu_interval_counter = 0;

    init();


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

        if (header->push_cpu_interval_counter % 10000 == 0) {
            header->cpu_util = update_cpu_util(header);
        }
        header->nano_sleep_time_step = get_sleep_time(header);
        sleep_time =  (sleep_time *  1.05) + header->nano_sleep_time_step;

        sleep = {0, header->nano_sleep_time_step};
        clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep, NULL);
        
        // sched_yield();
    }

    header->push_cpu_interval_counter++;

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

    long sleep_time = 0;
    struct timespec sleep  = {0, sleep_time};
    int const reader_tail = atomic_load_explicit(&header->tail, memory_order_relaxed);
    while (reader_tail == atomic_load_explicit(&header->head, memory_order_acquire)){
        //queue is empty
        if (header->stop_consumer_polling) {
            return 0;
        }

        if (header->pop_cpu_interval_counter % 10000 == 0) {
            header->cpu_util = update_cpu_util(header);
        }
        header->nano_sleep_time_step = get_sleep_time(header);
        sleep_time =  (sleep_time *  1.05) + header->nano_sleep_time_step;
        sleep = {0, sleep_time};
        clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep, NULL);
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
    header->pop_cpu_interval_counter++;

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


