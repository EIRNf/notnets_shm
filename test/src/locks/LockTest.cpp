#include "gtest/gtest.h"

#include <thread>
#include "bakery.h"
#include "burns_and_lynch.h"

using namespace std;

class LockTest : public ::testing::Test
{
public:
    LockTest() {}
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
protected:

};

typedef struct bakery_test_wrapper {
    bakery_lock *bl;

    // Resource to be protected by the lock
    // Testing purposes only
    volatile int resource;
    std::mutex mtx;
    
} bakery_test_wrapper;

void bakery_use_resource(bakery_test_wrapper *btw , int thread)
{
    std::lock_guard<std::mutex> lock(btw->mtx);


    if (btw->resource != 0) { // Resource is in-use, failure at mutual exclusion
        std::cout << "Resource was acquired by " << thread
                  << ", but is still in-use by " << btw->resource
                  << "!\n";
    }

    EXPECT_EQ(btw->resource, 0);
    btw->resource = thread;
    // std::cout << thread << " using resource...\n";

    atomic_thread_fence(std::memory_order_seq_cst);
    // std::this_thread::sleep_for(std::chrono::seconds(2));
    btw->resource = 0;
}

void bakery_thread_body(bakery_test_wrapper *btw , int thread)
{
    lock_bakery(btw->bl, thread);
    bakery_use_resource(btw, thread);
    unlock_bakery(btw->bl, thread);
}

TEST_F(LockTest, BakeryLock)  
{
    // Initialize the bakery lock
    bakery_lock *bl = init_bakery_lock(malloc(sizeof(bakery_lock)));
    bakery_test_wrapper *btw = new bakery_test_wrapper;
    btw->bl = bl;
    btw->resource = 0;

    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        int id = get_process_id_bakery(bl);
        threads.emplace_back(bakery_thread_body,btw, id);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    free(bl);  
    EXPECT_NO_FATAL_FAILURE(delete btw);
}


typedef struct burns_and_lynch_test_wrapper {
    burns_and_lynch_lock *bll;

    // Resource to be protected by the lock
    // Testing purposes only
    volatile int resource;
    std::mutex mtx;
    
} burns_and_lynch_test_wrapper;


void burns_lynch_use_resource(burns_and_lynch_test_wrapper *bltw , int thread)
{
    std::lock_guard<std::mutex> lock(bltw->mtx);

    if (bltw->resource != 0) {
        std::cout << "Resource was acquired by " << thread
                  << ", but is still in-use by " << bltw->resource
                  << "!\n";
    }

    EXPECT_EQ(bltw->resource, 0);


    bltw->resource = thread;
    // std::cout << thread << " using resource...\n";

    atomic_thread_fence(std::memory_order_seq_cst);
    // std::this_thread::sleep_for(std::chrono::seconds(2));
    bltw->resource = 0;
}

void burns_lynch_thread_body(burns_and_lynch_test_wrapper *bltw , int thread)
{
    lock_burns_and_lynch(bltw->bll, thread);
    burns_lynch_use_resource(bltw, thread);
    unlock_burns_and_lynch(bltw->bll, thread);
}


TEST_F(LockTest, BurnsAndLynchLock)  
{
    // Initialize the burns and lynch lock

    burns_and_lynch_lock *bll = init_burns_and_lynch_lock(malloc(sizeof(burns_and_lynch_lock)));
    burns_and_lynch_test_wrapper *bltw = new burns_and_lynch_test_wrapper;
    bltw->bll = bll;
    bltw->resource = 0;

    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        int id = get_process_id_burns_and_lynch(bltw->bll);
        threads.emplace_back(burns_lynch_thread_body,bltw, id);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    free(bll);
    EXPECT_NO_FATAL_FAILURE(delete bltw);
}
