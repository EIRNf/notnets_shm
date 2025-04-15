#ifndef __BAKERY_H
#define __BAKERY_H
// #include <chrono>
// #include <iostream>
// #include <mutex>
// #include <thread>
// #include <vector>
#include <stdlib.h>
#include <stdatomic.h>


#define THREAD_COUNT  1000

typedef struct bakery_lock {
    //User atomic to ensure unique assignment of process IDs
    
    atomic_int process_counter;

    atomic_int tickets[THREAD_COUNT];
    atomic_int choosing[THREAD_COUNT];


} bakery_lock;


void lock_bakery(bakery_lock *bl ,int thread);

void unlock_bakery(bakery_lock *bl , int thread);

bakery_lock* init_bakery_lock(void* shmaddr);

int get_process_id_bakery(bakery_lock *bl);

#endif 