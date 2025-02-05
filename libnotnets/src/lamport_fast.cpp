#include "lamport_fast.h"


// await b is an abbreviation for while ¬b do skip: 
void lock(lamport_fast_lock *lfl, int thread)
{
start:
    atomic_store(&lfl->b[thread], true);
    atomic_store(&lfl->x, thread);

    if (atomic_load(&lfl->y) != 0)
    {
        atomic_store(&lfl->b[thread], false);
        while (atomic_load(&lfl->y) != 0){
            // sched_yield();
            printf("Waiting for y to be 0\n");
        } //Await y to be 0
        goto start;
    }
    atomic_store(&lfl->y, thread);

    if (atomic_load(&lfl->x) != thread)
    {
        atomic_store(&lfl->b[thread], false);
        for (int j = 0; j < THREAD_COUNT; j++)
        {   
            while (atomic_load(&lfl->b[j])){
                // sched_yield();
                printf("Waiting for b[j] to be false\n");
            } //Await to be ~b[j]
        }
        if (atomic_load(&lfl->y) != thread)
        {
            while (atomic_load(&lfl->y) != 0){
                // sched_yield();
                printf("Waiting for y to be 0\n");
            } //Await y to be 0
            goto start;
        }
    }

}

void unlock(lamport_fast_lock *lfl , int thread){
    atomic_store(&lfl->y, 0);
    atomic_store(&lfl->b[thread], false);
}


lamport_fast_lock* init_lamport_fast_lock(void* shmaddr){

    lamport_fast_lock *lfl = (lamport_fast_lock*)shmaddr;
    atomic_store(&lfl->process_counter, 0);
    atomic_store(&lfl->x, 0);
    atomic_store(&lfl->y, 0);
    for (int i = 0; i < THREAD_COUNT; ++i) {
        atomic_store(&lfl->b[i], false);
    }
    return lfl;
}


int get_process_id(lamport_fast_lock *lfl){ //Dangerous
    return    atomic_fetch_add(&lfl->process_counter, 1);
;
}