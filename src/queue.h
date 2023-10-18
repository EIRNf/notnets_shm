
#ifndef __QUEUE_H
#define __QUEUE_H


//Includes
#include "mem.h"

//Definitions
// struct queue_meta {
//     int size;
//     int payload_size;


// };


//Should be 24 Bytes
typedef struct spsc_queue_header {
    int head;
    int tail;
    int queue_size;
    int current_count;
    int total_count;
    bool stop_producer_polling;
    bool stop_consumer_polling;
    char* message_array[];
} spsc_queue_header;

//Abstract queue class to handler variable queues, type corresponds to message
template <typename T>
class queue {
    public:
        int create(shared_memory_region shm);
        int destroy();
        int push(T message);
        T pop();
        T peek();


    private:
        shared_memory_region shm;
};


//SPSC queue implementation, type corresponds to message
template <typename T>
class spsc_queue: public queue<T> {
    public:
        spsc_queue<T>();
        ~spsc_queue<T>();

        int create(shared_memory_region shm);
        int destroy();
        int push(T message);
        T pop();
        T peek();

    private:
        shared_memory_region shm;

        spsc_queue_header* getQueueHeader(void *);
        bool isFull(spsc_queue_header *header);
        bool isEmpty(spsc_queue_header *header);
        int enqueue(spsc_queue_header *queue_header, T message);
        T* dequeue(spsc_queue_header *queue_header);

};

template <typename T>
int spsc_queue<T>::create(shared_memory_region shm){
    //do check to verify memory region size and how big to make array considering message size
    if(sizeof(T) + sizeof(spsc_queue_header) > shm.size) {
        perror("Message size type too large for allocated shm");
        exit(1);
    }

    //Calculate size of array
    int num_elements = (shm.size - sizeof(spsc_queue_header)) / sizeof(T);
    int leftover_bytes = (shm.size - sizeof(spsc_queue_header)) % sizeof(T);


    spsc_queue_header header;
    header.head = 0;
    header.tail = 0;
    header.queue_size = num_elements;
    header.current_count = 0;
    header.total_count = 0;
    header.stop_consumer_polling = false;
    header.stop_producer_polling = false;
    header.message_array = new T[num_elements];

    //Need to do a memcpy? TODO
    spsc_queue_header* header_ptr = getQueueHeader(shm.shmaddr);
    *header_ptr = header;
}


spsc_queue_header* getQueueHeader(void* shmaddr){
    spsc_queue_header* header = (spsc_queue_header*)(shmaddr);
    return header;
} 

//0 out all data in the shared memory region?
template <typename T>
int spsc_queue<T>::destroy(){

}

//Polling push, will wait until it can push
template <typename T>
int spsc_queue<T>::push(T message){
    spsc_queue_header* header = getQueueHeader(this->shm.shmaddr);

    while(true){
        if(header->stop_producer_polling){
            return -1;
        }

        if (isFull(header)){
            continue;
        }
        else {
            message = enqueue(header, message);
            break;
        }
        
    }
    return 0;
}

bool isFull(spsc_queue_header *queue_header){
    return (queue_header->head+1)%queue_header->queue_size == queue_header->head;

}


template <typename T>
int enqueue(spsc_queue_header *queue_header, T message){
    //Need to do a memcpy? TODO
    queue_header->message_array[queue_header->tail] = message;
    queue_header->current_count++;
    queue_header->total_count++;

    queue_header->tail = (queue_header->tail + 1) % queue_header->queue_size;
}

//Polling pop, will wait until it can pop
template <typename T>
T spsc_queue<T>::pop(){
    spsc_queue_header* header = getQueueHeader(this->shm.shmaddr);
    T message;

    while(true){
        if(header->stop_consumer_polling){
            return -1;
        }

        if (isEmpty(header)){
            continue;
        }
        else {
            message = dequeue(header);
            break; 
        }
        
    }
    return message;
}

bool isEmpty(spsc_queue_header *queue_header){
    return queue_header->head == queue_header->tail;
}


template <typename T>
T* dequeue(spsc_queue_header *queue_header){
    //Need to do a memcpy? TODO
    T message = queue_header->message_array[queue_header->head];
    queue_header->head = (queue_header->head +1 ) % queue_header->queue_size;
    queue_header->current_count--;
    return message;
\
}
template <typename T>
T spsc_queue<T>::peek(){

}


#endif
