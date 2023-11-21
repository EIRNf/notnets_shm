#include "../../src/rpc.h"

#include <assert.h>

void test_success_print(const char *message) {
    printf("SUCCESS:%s\n", message);
    fflush(stdout);
}

void test_failed_print(const char *message) {
    printf("FAILED:%s\n", message);
    fflush(stdout);
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
        client_open("test_client_addr", "test_server_addr");

        int err = client_close("test_client_addr", "test_server_addr");

        if (err) {
            test_failed_print(__func__);
        } else {
            test_success_print(__func__);
        }

        exit(0);
    }
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

void tests_run_all() {
    test_register_server_shutdown();
    test_open_close();
}
