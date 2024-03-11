
#include "gtest/gtest.h"


#include <sys/types.h>
#include <sys/wait.h>

#include "mem.h"
#include "coord.h"
#include "rpc.h"
#include "queue.h"


class CoordTest : public ::testing::Test
{
public:
    CoordTest() {}
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
protected:

};

TEST_F(CoordTest, QueueAllocation)  
{

    int message_size = sizeof(int);
    int iter = 10;
    int err = 0;

    shm_pair shms = _fake_shm_allocator(message_size, 1);

    void* request_queue_addr = shm_attach(shms.request_shm.shmid);
    void* response_queue_addr = shm_attach(shms.response_shm.shmid);

    int* buf = (int*)malloc(message_size);

    for (int i = 0; i < iter; i++) {
        *buf = i;
        push(request_queue_addr, buf, message_size);
    }

    free(buf);

    size_t* pop_size = (size_t*)malloc(sizeof(size_t));
    *pop_size = message_size;
    int* pop_buf = (int*)malloc(*pop_size);

    for (int i = 0; i < iter; i++) {
        pop(request_queue_addr, pop_buf, pop_size);
        if (*pop_buf != i) {
            err = 1;
        }
    }

    free(pop_size);
    free(pop_buf);

    // cleanup shm
    shm_detach(request_queue_addr);
    shm_detach(response_queue_addr);
    shm_remove(shms.request_shm.shmid);
    shm_remove(shms.response_shm.shmid);

    EXPECT_FALSE(err);
    
}

TEST_F(CoordTest, SingleClientRecv)  
{
    int message_size = sizeof(int);
    int iter = 10;
    pid_t wpid;
    int status = 0;
    pid_t c_pid = fork();

    if (c_pid == -1) {
        perror("fork");
    } else if (c_pid == 0) {
        // client
        int err = 0;

        coord_header* coord_region = coord_attach((char*)"SingleClientRecv");

        while (coord_region == NULL) {
	    coord_region = coord_attach((char*)"SingleClientRecv");
        }

        int client_id = 4;
        int reserved_slot = request_slot(coord_region, client_id, sizeof(int));
        assert(reserved_slot == 0);

        shm_pair shms = {};
        while (shms.request_shm.key <= 0 && shms.response_shm.key <= 0) {
            shms = check_slot(coord_region, reserved_slot);
        }

        void* request_addr = shm_attach(shms.request_shm.shmid);
        void* response_addr = shm_attach(shms.response_shm.shmid);

        int* buf = (int*)malloc(message_size);

        for (int i = 0; i < iter; i++) {
            *buf = i;
            push(request_addr, buf, message_size);
        }

        free(buf);
        shm_detach(request_addr);

        size_t* pop_size = (size_t*)malloc(sizeof(size_t));
        *pop_size = message_size;
        int* pop_buf = (int*)malloc(*pop_size);

        for (int i = 0; i < iter; i++) {
            pop(response_addr, pop_buf, pop_size);

            if (*pop_buf != i * 10) {
                err = 1;
            }
        }

        free(pop_size);
        free(pop_buf);
        shm_detach(response_addr);

	EXPECT_FALSE(err);

        exit(0);
    } else {
        // server
        coord_header* coord_region = coord_create((char*)"SingleClientRecv");
        int err = -1;
        while (err == -1) {
            // TODO: what's the point of returning client_id? client_id won't
            // help us fetch anything, since most functions require the slot num
            err = service_slot(coord_region, 0, &_fake_shm_allocator);
        }

        shm_pair shms = check_slot(coord_region, 0);

        void* request_addr = shm_attach(shms.request_shm.shmid);
        void* response_addr = shm_attach(shms.response_shm.shmid);

        size_t* pop_size = (size_t*)malloc(sizeof(size_t));
        *pop_size = message_size;
        int* pop_buf = (int*)malloc(*pop_size);

        for (int i = 0; i < iter; i++) {
            pop(request_addr, pop_buf, pop_size);

            // do work on the request. usually we would create a push_buf, but
            // since pop_buf is already message_size length, we can reuse it
            *pop_buf = *pop_buf * 10;
            push(response_addr, pop_buf, message_size);
        }

        free(pop_size);
        free(pop_buf);
        // note that it is the server's responsibility to detach from shm, since
        // it attached to it (since it has the void*)
        shm_detach(request_addr);
        shm_detach(response_addr);

        while ((wpid = wait(&status)) > 0);

        // clear all slots before deleting coord region
        force_clear_slot(coord_region, 0);
		coord_delete(coord_region);
    }
}

TEST_F(CoordTest, SingleClientGetSlot)  
{
    // Forking to test multiprocess functionality
    pid_t wpid;
    int status = 0;
    int err;
    pid_t c_pid = fork();

    if (c_pid == -1) {
        perror("fork");
    } else if (c_pid == 0) {
        // CHILD PROCESS
      coord_header* coord_region = coord_attach((char*)"SingleClientGetSlot");

        while (coord_region == NULL) {
	  coord_region = coord_attach((char*)"SingleClientGetSlot");
        }

        int client_id = 5;
        int reserved_slot = request_slot(coord_region, client_id, sizeof(int));
        assert(reserved_slot == 0);

        shm_pair shms = {};
        while (shms.request_shm.key <= 0 && shms.response_shm.key <= 0) {
            shms = check_slot(coord_region, reserved_slot);
        }

        //dont actually detach as it is a child process and will detach the parents shared memory as well
        // detach(coord_region);

        //CONDITION TO VERIFY
        err = shms.request_shm.key != SIMPLE_KEY ||
            shms.response_shm.key != SIMPLE_KEY + 1;

	    EXPECT_FALSE(err);
        exit(0);
    } else {
        // PARENT PROCESS
      coord_header* coord_region = coord_create((char*)"SingleClientGetSlot");
        err = -1;
        while (err == -1) {
            // pthread_mutex_lock(&coord_region->mutex);
            err = service_slot(coord_region, 0, &_fake_shm_allocator);
            // pthread_mutex_unlock(&coord_region->mutex);

        }

        while ((wpid = wait(&status)) > 0);

        // clear all slots before deleting coord region
        force_clear_slot(coord_region, 0);
		coord_delete(coord_region);
    }

}

