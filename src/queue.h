
#ifndef __QUEUE_H
#define __QUEUE_H


//Includes
#include "mem.h"
// #include <boost/any.hpp>


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
    void* message_array;
} spsc_queue_header;

// Abstract queue class to handler variable queues, type corresponds to message
// template <class T>
// class queue {
//     public:
//         queue();

//         virtual int create(shared_memory_region *shm) = 0;
//         int attach(shared_memory_region *shm);
//         int destroy();
//         int push(T message) = 0;
//         T pop() = 0 ;
//         T peek() = 0;


//     private:
//         shared_memory_region shm;
// };


//SPSC queue implementation, type corresponds to message
template <class T>
class spsc_queue{
    public:

        // template <std::size_t region_size>
        spsc_queue();

        int create(shared_memory_region *shm); //ensure shared_memory size is big enough to handle (shm->size - sizeof(spsc_queue_header)) / sizeof(T)
        int attach(shared_memory_region *shm); //doesnt make sense as a call?
        void destroy();
        int push(T message);
        int stop_producer_polling();
        int stop_consumer_polling();
        T pop();
        T peek();

        ssize_t create(void* shm_addr, size_t shm_size, size_t message)
        // ssize_t attach(void* shm_addr, )
        ssize_t push(const void* buf, size_t buf_size);
        //preallocate do data copy
        void* pop(ssize_t *size);
        //get a pointer 
        void* peek(ssize_t *size); 



    private:
        shared_memory_region *shm;

        constexpr int get_array_size();
        spsc_queue_header* get_queue_header(void* shmaddr);
        bool is_full(spsc_queue_header *header);
        bool is_empty(spsc_queue_header *header);
        int enqueue(spsc_queue_header *queue_header, T message);
        T dequeue(spsc_queue_header *queue_header);

};

template <class T>
spsc_queue<T>::spsc_queue(){
}

template <class T>
spsc_queue_header* spsc_queue<T>::get_queue_header(void* shmaddr){
    spsc_queue_header* header = (spsc_queue_header*)(shmaddr);
    return header;
} 

template <class T>
int spsc_queue<T>::create(shared_memory_region *shm){
    //do check to verify memory region size and how big to make array considering message size
    if(sizeof(T) + sizeof(spsc_queue_header) > (u_long) shm->size) {
        perror("Message size type + Header data too large for allocated shm");
        exit(1);
    }

    this->shm = shm;

    //Calculate size of array
    int num_elements = (shm->size - sizeof(spsc_queue_header)) / sizeof(T);
    int leftover_bytes = (shm->size - sizeof(spsc_queue_header)) % sizeof(T);


    spsc_queue_header header;
    header.head = 0;
    header.tail = 0;
    header.queue_size = num_elements;
    header.current_count = 0;
    header.total_count = 0;
    header.stop_consumer_polling = false;
    header.stop_producer_polling = false;
    char* temp_addr = (char*) this->shm->shmaddr ;
    header.message_array = temp_addr + sizeof(spsc_queue_header);

    //Need to do a memcpy? TODO
    spsc_queue_header* header_ptr = get_queue_header(this->shm->shmaddr);
    *header_ptr = header;
    return 0;
}


template <class T>
int spsc_queue<T>::attach(shared_memory_region *shm){
    //do check to verify memory region size and how big to make array considering message size
    if(sizeof(T) + sizeof(spsc_queue_header) > (u_long) shm->size) {
        perror("Message size type too large for allocated shm");
        exit(1);
    }
    this->shm = shm;
    
    //Need to do a memcpy? TODO
    // spsc_queue_header* header_ptr = get_queue_header(shm->shmaddr);
    return 0;

}


//0 out all data in the shared memory region?
template <class T>
void spsc_queue<T>::destroy(){

}

//Polling push, will wait until it can push
template <class T>
int spsc_queue<T>::push(T message){
    spsc_queue_header* header = get_queue_header(this->shm->shmaddr);

    while(true){
        if(header->stop_producer_polling){
            return -1;
        }

        if (is_full(header)){
            continue;
        }
        else {
            enqueue(header, message);
            break;
        }
        
    }
    return 0;
}

template <class T>
bool spsc_queue<T>::is_full(spsc_queue_header *queue_header){
    return (queue_header->tail+1)%queue_header->queue_size == queue_header->head;
}


template <class T>
int spsc_queue<T>::enqueue(spsc_queue_header *queue_header, T message){
    // T (*template_array)[queue_header->queue_size] = (T (*)[queue_header->queue_size]) queue_header->message_array; //compile time issue
    // (*template_array)[queue_header->tail] = message;
    
    //byte addressing to assign templated message type to address location
    T* array_start = reinterpret_cast<T*> (queue_header->message_array);
    // T* element_pointer =  array_start + queue_header->tail;
    array_start[queue_header->tail] = message;
    // *element_pointer = message;


    //Need to do a memcpy? TODO
    queue_header->current_count++;
    queue_header->total_count++;

    //fence
    queue_header->tail = (queue_header->tail + 1) % queue_header->queue_size;
        return 0;

}

//Polling pop, will wait until it can pop
template <class T>
T spsc_queue<T>::pop(){
    spsc_queue_header* header = get_queue_header(this->shm->shmaddr);
    T message;

    while(true){
        if(header->stop_consumer_polling){
            return -1;
        }

        if (is_empty(header)){
            continue;
        }
        else {
            message = dequeue(header);
            break; 
        }

    }
    return message;
}

template <class T>
bool spsc_queue<T>::is_empty(spsc_queue_header *queue_header){
    return queue_header->head == queue_header->tail;
}


template <class T>
T spsc_queue<T>::dequeue(spsc_queue_header *queue_header){

    // T (*template_array)[queue_header->queue_size] = (T (*)[queue_header->queue_size]) queue_header->message_array; // compile time issue
    // T message =  (*template_array)[queue_header->head];

    T* array_start = reinterpret_cast<T*> (queue_header->message_array);
    // T* element_pointer =  array_start + queue_header->head;
    T message = array_start[queue_header->head];
    // T message = *element_pointer;


    //fence
    queue_header->head = (queue_header->head +1 ) % queue_header->queue_size;
    queue_header->current_count--;
    return message;
}
template <class T>
T spsc_queue<T>::peek(){

}


#endif
