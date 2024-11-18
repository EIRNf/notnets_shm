#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#define THREAD_COUNT  200

typedef struct bakery_lock {
    //User atomic to ensure unique assignment of process IDs
    std::atomic_int process_counter;

    int tickets[THREAD_COUNT];
    int choosing[THREAD_COUNT];

    // Resource to be protected by the lock
    // Testing purposes only
    volatile int resource;
    std::mutex mtx;

} bakery_lock;

void lock(bakery_lock *bl ,int thread)
{
    bl->choosing[thread] = 1;

    std::atomic_thread_fence(std::memory_order_seq_cst);

    int max_ticket = 0;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        int ticket = bl->tickets[i];
        max_ticket
            = ticket > max_ticket ? ticket : max_ticket;
    }

    bl->tickets[thread] = max_ticket + 1;

    std::atomic_thread_fence(std::memory_order_seq_cst);
    bl->choosing[thread] = 0;

    for (int other = 0; other < THREAD_COUNT; ++other) {
        while (bl->choosing[other]) {
        }

        std::atomic_thread_fence(std::memory_order_seq_cst);

        while (bl->tickets[other] != 0
               && (bl->tickets[other] < bl->tickets[thread]
                   || (bl->tickets[other] == bl->tickets[thread]
                       && other < thread))) {
        }
    }
}

void unlock(bakery_lock *bl , int thread)
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
    bl->tickets[thread] = 0;
}

void use_resource(bakery_lock *bl , int thread)
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

void thread_body(bakery_lock *bl , int thread)
{
    lock(bl, thread);
    use_resource(bl, thread);
    unlock(bl, thread);
}

bakery_lock* init_bakery_lock()
{
    bakery_lock *bl = new bakery_lock;
    bl->process_counter = 0;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        bl->tickets[i] = 0;
        bl->choosing[i] = 0;
    }
    bl->resource = 0;
    return bl;
}

int get_process_id(bakery_lock *bl)
{
    return bl->process_counter++;
}

int test_bakery()
{
    // Initialize the bakery lock
    bakery_lock *bl = init_bakery_lock();
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

