#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#define THREAD_COUNT  200

typedef enum flag
{
  down,
  up,
} flag;

//From Mutual Exclusion Using Indivisible Reads and Writes by James Burns and Nancy Lynch
//https://groups.csail.mit.edu/tds/papers/Lynch/allertonconf.pdf

typedef struct burns_and_lynch_lock {
    //User atomic to ensure unique assignment of process IDs
    std::atomic_int process_counter;

    flag F[THREAD_COUNT];

    // Resource to be protected by the lock
    // Testing purposes only
    volatile int resource;
    std::mutex mtx;

} burns_and_lynch_lock;

void lock(burns_and_lynch_lock *bl ,int thread)
{
    while (true){
        //STEP 1
        // Put hand down to start procedure, or finish procedure
        bl->F[thread] = down;
        //STEP 2
        //REMAINDER REGION???
        //STEP 3
step_3:
        bl->F[thread] = down;
        std::atomic_thread_fence(std::memory_order_seq_cst);


        //STEP 4
        //Look to my left, and keep my hand down as long as I see someone with a hand up
        for (int  j = 1; j < thread; j++){
            if (bl->F[j] == up){
                goto step_3;
            }
        }
        //STEP 5
        // If no one to my left has their hand up, I raise my hand
        bl->F[thread] = up;
        std::atomic_thread_fence(std::memory_order_seq_cst);

        //STEP 6
        // Check again to my left to see if anyone has raised their hand,
        // if so, I put my hand down and star over, otherwise, I keep my hand raised and continue
        for (int  j = 1; j < thread; j++){
            if (bl->F[j] == up){
                goto step_3;
            }
        }
        //STEP 7
        // Everyone to my right goes first, so I look to my right and wait until I don't see any hands up.
step_7:
        for (int  j = thread+1; j <= THREAD_COUNT; j++){
            if (bl->F[j] == up){
                goto step_7;
            }
        }
        //STEP 8
        // Once all hands to my right are down, I can enter the critical section.
        //CRITICAL SECTION
        break;
    }
}

void unlock(burns_and_lynch_lock *bl , int thread)
{
    bl->F[thread] = down;
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

void use_resource(burns_and_lynch_lock *bl , int thread)
{
    std::lock_guard<std::mutex> lock(bl->mtx);

    if (bl->resource != 0) {
        std::cout << "Resource was acquired by " << thread
                  << ", but is still in-use by " << bl->resource
                  << "!\n";
    }

    bl->resource = thread;
    std::cout << thread << " using resource...\n";

    std::atomic_thread_fence(std::memory_order_seq_cst);
    // std::this_thread::sleep_for(std::chrono::seconds(2));
    bl->resource = 0;
}

void thread_body(burns_and_lynch_lock *bl , int thread)
{
    lock(bl, thread);
    use_resource(bl, thread);
    unlock(bl, thread);
}

burns_and_lynch_lock* init_burns_and_lynch_lock()
{
    burns_and_lynch_lock *bl = new burns_and_lynch_lock;
    bl->process_counter = 0;
    for (int i = 0; i < THREAD_COUNT; ++i) {
        bl->F[i] = down;
    }
    bl->resource = 0;
    return bl;
}

int get_process_id(burns_and_lynch_lock *bl)
{
    return bl->process_counter++;
}

int main()
{
    // Initialize the bakery lock
    burns_and_lynch_lock *bl = init_burns_and_lynch_lock();
    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i) {

        int id = get_process_id(bl);
        threads.emplace_back(thread_body,bl, id);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}

