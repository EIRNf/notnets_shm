#include "../../src/rpc.h"

#include <assert.h>

void test_success_print(const char *message) {
    printf("SUCCESS:%s\n", message);
}


void test_failed_print(const char *message) {
    printf("FAILED:%s\n", message);
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
}
