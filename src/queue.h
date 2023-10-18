
#ifndef __QUEUE_H
#define __QUEUE_H


//Includes
#include "mem.h"

//Definitions
// struct queue_meta {
//     int size;
//     int payload_size;


// };


//Abstract queue class to handler variable queues, type corresponds to message
template <typename T>
class queue {
    public:
        int create<T>();
        int destroy();
        int enqueue(T message);
        int dequeue(T *message);
        int peek(T *message);


    private:
        shared_memory_region shm;
};


//SPSC queue implementation, type corresponds to message
template <typename T>
class spsc_queue: public queue<T> {
    public:
        spsc_queue();
        int create();
        int destroy();
        int enqueue(T message);
        int dequeue(T *message);
        int peek(T *message);

    private:
};

template <typename T>
int spsc_queue<T>::create(){

}

template <typename T>
int spsc_queue<T>::destroy(){

}

template <typename T>
int spsc_queue<T>::enqueue(T message){

}

template <typename T>
int spsc_queue<T>::dequeue(T *message){

}
template <typename T>
int spsc_queue<T>::peek(T *message){

}


#endif
