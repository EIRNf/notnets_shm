#ifndef __BURNS_AND_LYNCH_H
#define __BURNS_AND_LYNCH_H

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <stdatomic.h>

#define THREAD_COUNT  200


//From Mutual Exclusion Using Indivisible Reads and Writes by James Burns and Nancy Lynch
//https://groups.csail.mit.edu/tds/papers/Lynch/allertonconf.pdf
typedef struct burns_and_lynch_lock {
    //User atomic to ensure unique assignment of process IDs
    atomic_int process_counter;

    //Atomic bool as flag array
    // down = false, up = true
    // down = 0, up = 1
    atomic_bool F[THREAD_COUNT];

} burns_and_lynch_lock;

void lock(burns_and_lynch_lock *bl ,int thread);

void unlock(burns_and_lynch_lock *bl , int thread);

burns_and_lynch_lock* init_burns_and_lynch_lock(void* shmaddr);

int get_process_id(burns_and_lynch_lock *bl);

#endif 