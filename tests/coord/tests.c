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


key_pair fake_allocater(){
    return coord_test_name_keys(SIMPLE_KEY);
}




key_pair coord_test_queue_allocation(int key_seed){
	key_pair keys = coord_test_name_keys(key_seed);

	// //ALLOCATE MEMORY REGIONS
	// shared_memory_region request_test_region(keys.request_shm_key, 200, create_flag);
	// shared_memory_region response_test_region(keys.response_shm_key, 200, create_flag);

	// int request_shmid = request_test_region.create();
	// void* request_shmaddr = request_test_region.attach(request_shmid);

	// int response_shmid = response_test_region.create();
	// void* response_shmaddr = response_test_region.attach(response_shmid);

	// //Create queue, for proper shared memory usage you would need to "attach" to an existing implementation. Implementing asap
	// //Make sure the shared memory is big enough to hold header + atleast one message
	// spsc_queue<int> request_queue;
	// request_queue.create(&request_test_region);

	// spsc_queue<int> response_queue;
	// response_queue.create(&response_test_region);
    // key_pair keys = {key_seed,key_seed+1};
	return keys;
}


//does not test close
void test_single_client_get_keys(){
    // Forking to test multiprocess functionality
    pid_t c_pid = fork(); 
  
    if (c_pid == -1) { 
        perror("fork"); 
    } 
    else if (c_pid > 0) {  //PARENT PROCESS


        coord_header* coord_region = create("common_name");
        int err = -1;
        while (err == -1) {
            err = service_keys(coord_region, 0, &fake_allocater);        
        }
        sleep(2);
		delete(coord_region);
    } 
    else { //CHILD PROCESS
        coord_header* coord_region = attach("common_name");

        int client_id = 4;
        int reserved_slot = request_keys(coord_region, client_id);
        sleep(1);
        key_pair keys = check_slot(coord_region, reserved_slot);

        //dont actually detach as it is a child process and will detach the parents shared memory as well
        // detach(coord_region);

        //CONDITION TO VERIFY
        if(keys.request_shm_key != coord_test_name_keys(SIMPLE_KEY).request_shm_key &&
            keys.response_shm_key != coord_test_name_keys(SIMPLE_KEY).response_shm_key 
        ){
            test_failed_print(__func__);
        }
        else{
            test_success_print(__func__);
        }
    } 
  
}


void tests_run_all(void){
    test_single_client_get_keys();

}

