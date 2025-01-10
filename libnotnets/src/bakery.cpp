#include "bakery.h"

void lock(bakery_lock *bl ,int thread)
{
    atomic_store(&bl->choosing[thread], 1);

    atomic_int max_ticket = 0;
    atomic_int ticket = 0;
    for (int i = 0; i < THREAD_COUNT; ++i) {
        atomic_store(&ticket,bl->tickets[i]);

        if (ticket > max_ticket) {
            atomic_store(&max_ticket, ticket);
        }
    }

    atomic_store(&bl->tickets[thread],  max_ticket + 1);
    atomic_store(&bl->choosing[thread], 0);

    for (int other = 0; other < THREAD_COUNT; ++other) {
        while (atomic_load(&bl->choosing[other])) {
        }

        while (atomic_load(&bl->tickets[other]) != 0
               && (atomic_load(&bl->tickets[other]) < atomic_load(&bl->tickets[thread])
                   || (atomic_load(&bl->tickets[other]) == atomic_load(&bl->tickets[thread])
                       && other < thread))) {
        }
    }
}

void unlock(bakery_lock *bl , int thread)
{
    atomic_store(&bl->tickets[thread], 0);
}

bakery_lock* create_bakery_lock()
{
    bakery_lock *bl = init_bakery_lock(malloc(sizeof(bakery_lock)));
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


