#include <assert.h>

 #include <sys/types.h>
#include <sys/wait.h>

#define SIMPLE_KEY 17

void test_success_print(const char *message) {
    printf("SUCCESS:%s\n", message);
    // since we are outputting to a file, stdout will not buffer by newline
    // (instead, it will flush when buffer is full).
    // we need to therefore flush to ensure child processes do not inherit
    // stdout from parent processes
    fflush(stdout);
}


void test_failed_print(const char *message) {
    printf("FAILED:%s\n", message);
    fflush(stdout);
}

shm_pair fake_shm_allocator(int message_size) {
    shm_pair shms = {};

    int request_shmid = shm_create(SIMPLE_KEY,
                                   QUEUE_SIZE,
                                   create_flag);
    int response_shmid = shm_create(SIMPLE_KEY + 1,
                                    QUEUE_SIZE,
                                    create_flag);
    notnets_shm_info request_shm = {SIMPLE_KEY,
                            request_shmid};
    notnets_shm_info response_shm = {SIMPLE_KEY + 1,
                             response_shmid};

    // set up shm regions as queues
    void* request_addr = shm_attach(request_shmid);
    void* response_addr = shm_attach(response_shmid);
    queue_create(request_addr, QUEUE_SIZE, message_size);
    queue_create(response_addr, QUEUE_SIZE, message_size);
    // don't want to stay attached to the queue pairs
    shm_detach(request_addr);
    shm_detach(response_addr);

    shms.request_shm = request_shm;
    shms.response_shm = response_shm;

    return shms;
}

void test_queue_allocation() {
    int message_size = sizeof(int);
    int iter = 10;
    int err = 0;

    shm_pair shms = fake_shm_allocator(message_size);

    void* request_queue_addr = shm_attach(shms.request_shm.shmid);
    void* response_queue_addr = shm_attach(shms.response_shm.shmid);

    int* buf = malloc(message_size);

    for (int i = 0; i < iter; i++) {
        *buf = i;
        push(request_queue_addr, buf, message_size);
    }

    free(buf);

    size_t* pop_size = malloc(sizeof(size_t));
    *pop_size = message_size;
    int* pop_buf = malloc(*pop_size);

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

    //CONDITION TO VERIFY
    if (err) {
        test_failed_print(__func__);
    } else {
        test_success_print(__func__);
    }
}

void test_single_client_send_rcv() {
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

        coord_header* coord_region = coord_attach("common_name");

        while (coord_region == NULL) {
            coord_region = coord_attach("common_name");
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

        int* buf = malloc(message_size);

        for (int i = 0; i < iter; i++) {
            *buf = i;
            push(request_addr, buf, message_size);
        }

        free(buf);
        shm_detach(request_addr);

        size_t* pop_size = malloc(sizeof(size_t));
        *pop_size = message_size;
        int* pop_buf = malloc(*pop_size);

        for (int i = 0; i < iter; i++) {
            pop(response_addr, pop_buf, pop_size);

            if (*pop_buf != i * 10) {
                err = 1;
            }
        }

        free(pop_size);
        free(pop_buf);
        shm_detach(response_addr);

        // CONDITION TO VERIFY
        if (err) {
            test_failed_print(__func__);
        } else {
            test_success_print(__func__);
        }
        exit(0);
    } else {
        // server
        coord_header* coord_region = coord_create("common_name");
        int err = -1;
        while (err == -1) {
            // TODO: what's the point of returning client_id? client_id won't
            // help us fetch anything, since most functions require the slot num
            err = service_slot(coord_region, 0, &fake_shm_allocator);
        }

        shm_pair shms = check_slot(coord_region, 0);

        void* request_addr = shm_attach(shms.request_shm.shmid);
        void* response_addr = shm_attach(shms.response_shm.shmid);

        size_t* pop_size = malloc(sizeof(size_t));
        *pop_size = message_size;
        int* pop_buf = malloc(*pop_size);

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

void test_single_client_get_slot(){
    // Forking to test multiprocess functionality
    pid_t wpid;
    int status = 0;
    int err;
    pid_t c_pid = fork();

    if (c_pid == -1) {
        perror("fork");
    } else if (c_pid == 0) {
        // CHILD PROCESS
        coord_header* coord_region = coord_attach("common_name");

        while (coord_region == NULL) {
            coord_region = coord_attach("common_name");
        }

        int client_id = 4;
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
        if (err) {
            test_failed_print(__func__);
        } else {
            test_success_print(__func__);
        }
        exit(0);
    } else {
        // PARENT PROCESS
        coord_header* coord_region = coord_create("common_name");
        err = -1;
        while (err == -1) {
            err = service_slot(coord_region, 0, &fake_shm_allocator);
        }

        while ((wpid = wait(&status)) > 0);

        // clear all slots before deleting coord region
        force_clear_slot(coord_region, 0);
		coord_delete(coord_region);
    }

}

void tests_run_all(void){
    test_single_client_get_slot();
    test_queue_allocation();
    test_single_client_send_rcv();
}
