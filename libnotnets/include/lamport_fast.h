#ifndef __LAMPORT_FAST_H
#define __LAMPORT_FAST_H
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <stdatomic.h>


#define THREAD_COUNT  100

typedef struct lamport_fast_lock {
    //User atomic to ensure unique assignment of process IDs
    
    atomic_int process_counter;

    atomic_bool b[THREAD_COUNT];

    atomic_int x;
    atomic_int y;

} lamport_fast_lock;


void lock(lamport_fast_lock *lfl ,int thread);

void unlock(lamport_fast_lock *lfl , int thread);

lamport_fast_lock* init_lamport_fast_lock(void* shmaddr);

int get_process_id(lamport_fast_lock *lfl);

#endif 