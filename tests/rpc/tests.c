#include "../../src/rpc.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

void test_success_print(const char *message) {
    printf("SUCCESS:%s\n", message);
    fflush(stdout);
}

void test_failed_print(const char *message) {
    printf("FAILED:%s\n", message);
    fflush(stdout);
}

void test_register_server_shutdown() {
    server_context* sc = register_server("common_name");
    int err = (sc == NULL);
    shutdown(sc);

    if (err) {
        test_failed_print(__func__);
    } else {
        test_success_print(__func__);
    }
}

void test_open_close() {
    int status = 0;
    pid_t wpid, pid;
    pid = fork();

    if (pid == -1) {
        perror("fork");
    } else if (pid > 0) { // PARENT PROCESS
        server_context* sc = register_server("test_server_addr");

        while ((wpid = wait(&status)) > 0);

        shutdown(sc);
    } else { // CHILD PROCESS
        // if returns, then open was successful
        queue_pair* qp = client_open("test_client_addr",
                                     "test_server_addr",
                                     sizeof(int));

        while (qp == NULL) {
            qp = client_open("test_client_addr",
                             "test_server_addr",
                             sizeof(int));
        }

        int err = client_close("test_client_addr", "test_server_addr");

        if (err) {
            test_failed_print(__func__);
        } else {
            test_success_print(__func__);
        }

        exit(0);
    }
}

void test_accept() {
    // shm for communication between parent and child process
    key_t ipc_key = 1;
    int ipc_shm_size = sizeof(void*) * 2;
    int ipc_shmid = shm_create(ipc_key, ipc_shm_size, create_flag);
    void* ipc_shmaddr = shm_attach(ipc_shmid);
    void** two_ptrs = (void**) ipc_shmaddr;

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
    } else if (pid > 0) { // PARENT PROCESS
        server_context* sc = register_server("test_server_addr");

        queue_pair* qp;
        while ((qp = accept(sc)) == NULL);

        // wait for client to update the ipc shm
        while (two_ptrs[0] == 0 && two_ptrs[0] == 0);

        // the accepted qp should be same as the qp given to the client, since
        // there should only be one reserved/allocated slot
        int err = qp->request_shmaddr != two_ptrs[0] ||
            qp->response_shmaddr != two_ptrs[1];

        if (err) {
            test_failed_print(__func__);
        } else {
            test_success_print(__func__);
        }

        free(qp);
        shutdown(sc);
    } else { // CHILD PROCESS
        queue_pair* qp = client_open("test_client_addr",
                                     "test_server_addr",
                                     sizeof(int));
        while (qp == NULL) {
            qp = client_open("test_client_addr",
                             "test_server_addr",
                             sizeof(int));
        }

        two_ptrs[0] = qp->request_shmaddr;
        two_ptrs[1] = qp->response_shmaddr;

        free(qp);
        exit(0);
    }

    // cleanup ipc shm
    shm_detach(ipc_shmaddr);
    shm_remove(ipc_shmid);
}


// void test_many_servers() {
//     // shm for communication between parent and child process
//     key_t ipc_key = 1;
//     int ipc_shm_size = sizeof(void*) * 2;
//     int ipc_shmid = shm_create(ipc_key, ipc_shm_size, create_flag);
//     void* ipc_shmaddr = shm_attach(ipc_shmid);
//     void** two_ptrs = (void**) ipc_shmaddr;

//     pid_t pid = fork();

//     if (pid == -1) {
//         perror("fork");
//     } else if (pid > 0) { // PARENT PROCESS
//         server_context* sc = register_server("test_server_addr");

//         queue_pair* qp;
//         while ((qp = accept(sc)) == NULL);

//         // wait for client to update the ipc shm
//         while (two_ptrs[0] == 0 && two_ptrs[0] == 0);

//         // the accepted qp should be same as the qp given to the client, since
//         // there should only be one reserved/allocated slot
//         int err = qp->request_shmaddr != two_ptrs[0] ||
//             qp->response_shmaddr != two_ptrs[1];

//         if (err) {
//             test_failed_print(__func__);
//         } else {
//             test_success_print(__func__);
//         }

//         free(qp);
//         shutdown(sc);
//     } else { // CHILD PROCESS
//         queue_pair* qp = client_open("test_client_addr",
//                                      "test_server_addr",
//                                      sizeof(int));
//         while (qp == NULL) {
//             qp = client_open("test_client_addr",
//                              "test_server_addr",
//                              sizeof(int));
//         }

//         two_ptrs[0] = qp->request_shmaddr;
//         two_ptrs[1] = qp->response_shmaddr;

//         free(qp);
//         exit(0);
//     }

//     // cleanup ipc shm
//     shm_detach(ipc_shmaddr);
//     shm_remove(ipc_shmid);
// }

void test_send_rcv_rpc_int() {
    // shm for communication between parent and child process
    key_t ipc_key = 1;
    int ipc_shm_size = sizeof(void*) * 2;
    int ipc_shmid = shm_create(ipc_key, ipc_shm_size, create_flag);
    void* ipc_shmaddr = shm_attach(ipc_shmid);

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
    } else if (pid > 0) { // PARENT PROCESS
        server_context* sc = register_server("test_server_addr");

        queue_pair* qp;
        while ((qp = accept(sc)) == NULL);

        int *buf = malloc(sizeof(int));
        int buf_size = sizeof(int);

        for (int i = 0; i < 10; ++i) {
            int pop_size = server_receive_buf(qp, buf, buf_size);
            assert(pop_size == 0);

            // do work on client rpc
            *buf *= 2;
            server_send_rpc(qp, buf, buf_size);
        }

        free(buf);
        free(qp);

        // wait for client to finish reading responses
        while (*((int*) ipc_shmaddr) != 1);

        shutdown(sc);
    } else { // CHILD PROCESS
        queue_pair* qp = client_open("test_client_addr",
                                     "test_server_addr",
                                     sizeof(int));

        while (qp == NULL) {
            qp = client_open("test_client_addr",
                             "test_server_addr",
                             sizeof(int));
        }

        // currently the shm_allocator only handles int
        int* buf = malloc(sizeof(int));
        int buf_size = sizeof(int);

        // send 1-10 to server
        for (int i = 0; i < 10; ++i) {
            *buf = i;
            client_send_rpc(qp, buf, buf_size);
        }


        int err = 0;
        // get server responses
        for (int i = 0; i < 10; ++i) {
            int pop_size = client_receive_buf(qp, buf, buf_size);
            assert(pop_size == 0);

            if (*buf != i * 2) {
                err = 1;
            }
        }

        if (err) {
            test_failed_print(__func__);
        } else {
            test_success_print(__func__);
        }

        // usually we would close client conn, but server will shutdown in its
        // process, which will handle deallocation of qp
        free(buf);
        free(qp);

        *((int*) ipc_shmaddr) = 1;
        exit(0);
    }

    // cleanup ipc shm
    shm_detach(ipc_shmaddr);
    shm_remove(ipc_shmid);
}

void test_send_rcv_rpc_str() {
    // shm for communication between parent and child process
    key_t ipc_key = 1;
    int ipc_shm_size = sizeof(void*) * 2;
    int ipc_shmid = shm_create(ipc_key, ipc_shm_size, create_flag);
    void* ipc_shmaddr = shm_attach(ipc_shmid);

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
    } else if (pid > 0) { // PARENT PROCESS
        server_context* sc = register_server("test_server_addr");

        queue_pair* qp;
        while ((qp = accept(sc)) == NULL);

        int message_size = sizeof(char[10]);
        char *buf = malloc(message_size);

        for (int i = 0; i < 10; ++i) {
            server_receive_buf(qp, buf, message_size);

            // do work on client rpc
            char response[10] = "Hi, ";
            strncat(response, buf, strlen(buf));
            server_send_rpc(qp, response, message_size);
        }

        free(buf);
        free(qp);

        // wait for client to finish reading responses
        while (*((int*) ipc_shmaddr) != 1);

        shutdown(sc);
    } else { // CHILD PROCESS
        int message_size = sizeof(char[10]);
        queue_pair* qp = client_open("test_client_addr",
                                     "test_server_addr",
                                     message_size);

        while (qp == NULL) {
            qp = client_open("test_client_addr",
                             "test_server_addr",
                             sizeof(int));
        }

        char* buf = malloc(message_size);

        for (int i = 0; i < 10; ++i) {
            snprintf(buf, 10, "%d", i);
            client_send_rpc(qp, buf, message_size);
        }


        int err = 0;
        // get server responses
        for (int i = 0; i < 10; ++i) {
            client_receive_buf(qp, buf, message_size);

            char expected[10];
            snprintf(expected, 10, "Hi, %d", i);

            if (strcmp(buf, expected) != 0) {
                err = 1;
            }
        }

        if (err) {
            test_failed_print(__func__);
        } else {
            test_success_print(__func__);
        }

        // usually we would close client conn, but server will shutdown in its
        // process, which will handle deallocation of qp
        free(buf);
        free(qp);

        *((int*) ipc_shmaddr) = 1;
        exit(0);
    }

    // cleanup ipc shm
    shm_detach(ipc_shmaddr);
    shm_remove(ipc_shmid);
}
void tests_run_all() {
    // test_many_servers();
    test_register_server_shutdown();
    test_open_close();
    test_accept();
    test_send_rcv_rpc_int();
    test_send_rcv_rpc_str();

    //Testing simultaneous clients 
}
