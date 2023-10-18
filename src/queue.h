
#ifndef __QUEUE_H
#define __QUEUE_H


//Includes

//Definitions


//Abstract queue class to handler variable queues, type corresponds to message
template <typename T>
class queue {
    public:
    int enqueue(T message);
    int dequeue(T *message);


};


//SPSC queue implementation, type corresponds to message
template <typename T>
class spsc_queue: public queue<T> {


};


#endif
