#include <assert.h>

#define SIMPLE_KEY 17

void test_success_print(const char *message) {
    printf("SUCCESS:%s\n", message);
}


void test_failed_print(const char *message) {
    printf("FAILED:%s\n", message);
}

key_pair coord_test_name_keys(int test){
    key_pair keys = {test, test+1};
	return keys;
}


key_pair fake_key_generator(){
    return coord_test_name_keys(SIMPLE_KEY);
}

key_pair coord_test_queue_allocation(int key_seed){
	key_pair keys = coord_test_name_keys(key_seed);

	return keys;
}

//does not test close
void test_single_client_get_keys(){
    // Forking to test multiprocess functionality
    pid_t c_pid = fork();

    if (c_pid == -1) {
        perror("fork");
    }
    else if (c_pid == 0) {
        //CHILD PROCESS
        coord_header* coord_region = attach("common_name");

        int client_id = 4;
        int reserved_slot = request_slot(coord_region, client_id);
        assert(reserved_slot == 0);

        shm_pair shms = {};
        while (shms.request_shm.key <= 0 && shms.response_shm.key <= 0) {
            shms = check_slot(coord_region, reserved_slot);
        }

        //dont actually detach as it is a child process and will detach the parents shared memory as well
        // detach(coord_region);

        //CONDITION TO VERIFY
        if(shms.request_shm.key != coord_test_name_keys(SIMPLE_KEY).request_shm_key &&
           shms.response_shm.key != coord_test_name_keys(SIMPLE_KEY).response_shm_key
        ) {
            test_failed_print(__func__);
        }
        else{
            test_success_print(__func__);
        }
    }
    else {
        //PARENT PROCESS
        coord_header* coord_region = coord_create("common_name");
        int err = -1;
        while (err == -1) {
            err = service_slot(coord_region, 0, &fake_key_generator);
        }
        sleep(2);

        // clear all slots before deleting coord region
        force_clear_slot(coord_region, 0);
		coord_delete(coord_region);
    }

}


void tests_run_all(void){
    test_single_client_get_keys();
}

