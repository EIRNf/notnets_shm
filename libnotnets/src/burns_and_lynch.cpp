#include "burns_and_lynch.h"

void lock_burns_and_lynch(burns_and_lynch_lock *bl ,int thread)
{
    while (true){
        //STEP 1
        // Put hand down to start procedure, or finish procedure
        atomic_store(&bl->F[thread], false);
        //STEP 2
        //REMAINDER REGION???
        //STEP 3
step_3:
        atomic_store(&bl->F[thread], false);


        //STEP 4
        //Look to my left, and keep my hand down as long as I see someone with a hand up
        for (int  j = 1; j < thread; j++){
            if (atomic_load(&bl->F[j]) == true){
                goto step_3;
            }
        }
        //STEP 5
        // If no one to my left has their hand up, I raise my hand
        atomic_store(&bl->F[thread], true);

        //STEP 6
        // Check again to my left to see if anyone has raised their hand,
        // if so, I put my hand down and star over, otherwise, I keep my hand raised and continue
        for (int  j = 1; j < thread; j++){
            if (atomic_load(&bl->F[j]) == true){
                goto step_3;
            }
        }
        //STEP 7
        // Everyone to my right goes first, so I look to my right and wait until I don't see any hands up.
step_7:
        for (int  j = thread+1; j < THREAD_COUNT; j++){

            if (atomic_load(&bl->F[j]) == true){
                goto step_7;
            }
        }
        //STEP 8
        // Once all hands to my right are down, I can enter the critical section.
        //CRITICAL SECTION
        break;
    }
}

void unlock_burns_and_lynch(burns_and_lynch_lock *bl , int thread)
{
    atomic_store(&bl->F[thread], false);
}

burns_and_lynch_lock* init_burns_and_lynch_lock(void* shmaddr)
{
    burns_and_lynch_lock *bl = (burns_and_lynch_lock*)shmaddr;

    bl->process_counter = 0;
    for (int i = 0; i < THREAD_COUNT; ++i) {

        bl->F[i] = false;
    }
    return bl;
}

burns_and_lynch_lock* create_burns_and_lynch_lock()
{
    burns_and_lynch_lock *bl = init_burns_and_lynch_lock(malloc(sizeof(burns_and_lynch_lock)));
    return bl;
}

int get_process_id_burns_and_lynch(burns_and_lynch_lock *bl)
{
    return bl->process_counter++;
}


