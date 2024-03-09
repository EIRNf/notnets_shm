#include "gtest/gtest.h"

#include "mem.h"
#include "queue.h"

#include <assert.h>
#include <unistd.h>

#include <atomic>

class QueueTest : public ::testing::Test
{
public:
    QueueTest() {}
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
protected:

};


TEST_F(QueueTest, PartialBuffer)  
{
    // create shm and attach to it
    int err = 0;
    key_t key = 1;
    int shm_size = 1024;
    int shmid = shm_create(key, shm_size, create_flag);

    while (shmid == -1) {
        shmid = shm_create(key, shm_size, create_flag);
    }

    void* shmaddr = shm_attach(shmid);

    // create queue
    // message size of 4 ints
    int num_char = 4;
    int message_size = num_char*sizeof(char);
    queue_create(shmaddr, shm_size, message_size);


    char *buf = (char*)malloc(message_size);
    buf[0] = 'b';
    buf[1] = 'i';
    buf[2] = 'r';
    buf[3] = 'd';

    char* other_buf = (char*)malloc(message_size);
    other_buf[0] = 'a';

    push(shmaddr, buf, message_size);
    push(shmaddr, other_buf, message_size);

    size_t* pop_size = (size_t*)malloc(sizeof(size_t));

    // pop buffer can only store 1 character at a time
    *pop_size = sizeof(char);
    char* pop_buf = (char*)malloc(*pop_size);

    for (int i = 0; i < num_char; i++) {
        pop(shmaddr, pop_buf, pop_size);
        if (*pop_buf != buf[i]) {
            err = 1;
        }
    }

    pop(shmaddr, pop_buf, pop_size);
    if (*pop_buf != 'a') {
        err = 1;
    }

    // cleanup
    free(buf);
    free(other_buf);
    free(pop_buf);
    free(pop_size);
    shm_detach(shmaddr);
    shm_remove(shmid);

    //CONDITION TO VERIFY
    EXPECT_FALSE(err);

}

TEST_F(QueueTest, MultiProcess)  
{
    int err = 0;
    key_t key = 2;
    key_t ipc_key = 22;
    int shm_size = 1024;
    int message_size = sizeof(int);
    int iter = 100;

    pid_t c_pid = fork();

    if (c_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (c_pid > 0) {
        // parent process
        // ipc: create shm letting child know when parent has finished creating
        // queue
        int ipc_shmid = shm_create(ipc_key, sizeof(int), create_flag);
        void* ipc_shmaddr = shm_attach(ipc_shmid);
        std::atomic_int* ipc = (std::atomic_int*) ipc_shmaddr;

        // queue: create shm and attach to it
        int shmid = shm_create(key, shm_size, create_flag);
        void* shmaddr = shm_attach(shmid);

        // create queue
        queue_create(shmaddr, shm_size, message_size);

        // let child know parent has finished creating queue
        atomic_store(ipc, 1);

        // parent pushes into queue
        int *buf = (int*)malloc(message_size);

        // push integers 1-10
        for (int i = 0; i < iter; i++) {
            *buf = i;
            push(shmaddr, buf, message_size);
        }

        // wait until child has finished doing work
        atomic_thread_fence(std::memory_order_acquire);

        while ((int)atomic_load(ipc) != 0){}

        // cleanup
        free(buf);
        shm_detach(ipc_shmaddr);
        shm_detach(shmaddr);
        shm_remove(ipc_shmid);
        shm_remove(shmid);
    } else {
        // child process
        int ipc_shmid = shm_create(ipc_key, sizeof(int), create_flag);
        while (ipc_shmid == -1) {
            ipc_shmid = shm_create(ipc_key, sizeof(int), create_flag);
        }

        void* ipc_shmaddr = shm_attach(ipc_shmid);
        std::atomic_int* ipc = (std::atomic_int*) ipc_shmaddr;
        while ((int)atomic_load(ipc) != 1){}

        int shmid = shm_create(key, shm_size, create_flag);
        void* shmaddr = shm_attach(shmid);

        // client pops from queue
        // assume the pop buffer can take in a full message

        size_t* pop_size = (size_t*)malloc(sizeof(ssize_t));
        *pop_size = message_size;
        int* pop_buf = (int*)malloc(*pop_size);

        for (int i = 0; i < iter; i++) {
            pop(shmaddr, pop_buf, pop_size);
            if (*pop_buf != i) {
                err = 1;
            }
        }

        // let parent know child is done with work
        atomic_store(ipc, 0);

        // cleanup
        free(pop_size);
        free(pop_buf);
        shm_detach(ipc_shmaddr);
        shm_detach(shmaddr);

        EXPECT_FALSE(err);

        // make sure to exit, or else child process will go to tests_run_all
        // and continue running tests!
        exit(0);
    }
}

TEST_F(QueueTest, SingleProcess)  
{
    // create shm and attach to it
    int err = 0;
    key_t key = 3;
    int shm_size = 1024;
    int shmid = shm_create(key, shm_size, create_flag);

    while (shmid == -1) {
        shmid = shm_create(key, shm_size, create_flag);
    }

    void* shmaddr = shm_attach(shmid);

    // create queue
    int message_size = sizeof(int);
    queue_create(shmaddr, shm_size, message_size);


    // push and pop functionality
    int *buf = (int*)malloc(message_size);

    // push integers 1-10
    int iter = 10;
    for (int i = 0; i < iter; i++) {
        *buf = i;
        push(shmaddr, buf, message_size);
    }

    free(buf);

    size_t* pop_size = (size_t*)malloc(sizeof(ssize_t));
    *pop_size = message_size;
    int* pop_buf = (int*)malloc(*pop_size);

    for (int i = 0; i < iter; i++) {
        pop(shmaddr, pop_buf, pop_size);
        if (*pop_buf != i) {
            err = 1;
        }
    }

    // cleanup
    free(pop_size);
    free(pop_buf);
    shm_detach(shmaddr);
    shm_remove(shmid);

    EXPECT_FALSE(err);

}

