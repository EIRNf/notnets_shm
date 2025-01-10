#ifndef __BAKERY_H
#define __BAKERY_H
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <stdatomic.h>


#define THREAD_COUNT  200

typedef struct bakery_lock {
    //User atomic to ensure unique assignment of process IDs
    
    atomic_int process_counter;

    atomic_int tickets[THREAD_COUNT];
    atomic_int choosing[THREAD_COUNT];


} bakery_lock;


void lock(bakery_lock *bl ,int thread);

void unlock(bakery_lock *bl , int thread);

bakery_lock* init_bakery_lock(void* shmaddr);

int get_process_id(bakery_lock *bl);

#endif 