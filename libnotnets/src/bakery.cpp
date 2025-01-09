#include "bakery.h"

void lock(bakery_lock *bl ,int thread)
{
    bl->choosing[thread] = 1;

    atomic_thread_fence(memory_order_seq_cst);

    int max_ticket = 0;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        int ticket = bl->tickets[i];
        max_ticket
            = ticket > max_ticket ? ticket : max_ticket;
    }

    bl->tickets[thread] = max_ticket + 1;

    atomic_thread_fence(memory_order_seq_cst);
    bl->choosing[thread] = 0;

    for (int other = 0; other < THREAD_COUNT; ++other) {
        while (bl->choosing[other]) {
        }

        atomic_thread_fence(memory_order_seq_cst);

        while (bl->tickets[other] != 0
               && (bl->tickets[other] < bl->tickets[thread]
                   || (bl->tickets[other] == bl->tickets[thread]
                       && other < thread))) {
        }
    }
}

void unlock(bakery_lock *bl , int thread)
{
    atomic_thread_fence(memory_order_seq_cst);
    bl->tickets[thread] = 0;
}

bakery_lock* create_bakery_lock()
{
    bakery_lock *bl = new bakery_lock;
    bl->process_counter = 0;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        bl->tickets[i] = 0;
        bl->choosing[i] = 0;
    }
    return bl;
}

bakery_lock* init_bakery_lock(void* shmaddr)
{
    bakery_lock *bl = (bakery_lock*)shmaddr;
    bl->process_counter = 0;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        bl->tickets[i] = 0;
        bl->choosing[i] = 0;
    }
    return bl;
}

int get_process_id(bakery_lock *bl)
{
    return bl->process_counter++;
}


