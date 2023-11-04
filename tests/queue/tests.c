
void test_success_print(const char *message) {
    printf("SUCCESS:%s\n", message);
}


void test_failed_print(const char *message) {
    printf("FAILED:%s\n", message);
}

void test_basic_queue_multiprocess(){

	// 	//Forking to test multiprocess functionality
	// pid_t c_pid = fork(); 
  
    // if (c_pid == -1) { 
    //     perror("fork"); 
    //     exit(EXIT_FAILURE); 
    // } 
	// else if (c_pid > 0)  {  //PARENT PROCESS
	// 	//ALLOCATE MEMORY REGION
	// 	shared_memory_region test_region(1, 200, create_flag);
	// 	int shmid = test_region.create();
	// 	void* shmaddr = test_region.attach(shmid);

	// 	//Create queue, for proper shared memory usage you would need to "attach" to an existing implementation. Implementing asap
	// 	//Make sure the shared memory is big enough to hold header + atleast one message
	// 	spsc_queue<int> test_queue;
	// 	test_queue.create(&test_region);

    //     //  wait(nullptr); 
    //     std::cout << "printed from parent process: " << getpid() 
    //          << std::endl; 


	// 	//ENQUEUE SOMETHING TO IT
	// 	for(int i = 0; i < 10; i++){
	// 		std::cout << "push:" << i << "\n";
	// 		test_queue.push(i);
	// 		// sleep(1);    
	// 	}

	// 	test_region.remove();

    // } 
    // else { //CHILD PROCESS

	// 	//ALLOCATE MEMORY REGION
	// 	shared_memory_region test_region(1, 200, create_flag);
	// 	int shmid = test_region.create();
	// 	void* shmaddr = test_region.attach(shmid);

	// 	//Create queue, for proper shared memory usage you would need to "attach" to an existing implementation. Implementing asap
	// 	//Make sure the shared memory is big enough to hold header + atleast one message
	// 	spsc_queue<int> test_queue;
	// 	test_queue.attach(&test_region);


	// 	std::cout << "printed from child process: " << getpid() 
    //          << std::endl; 

	// 	//DEQUEUE SOMETHING TO IT
	// 	for(int i = 0; i < 10; i++){
	// 		int message = test_queue.pop();
	// 		std::cout << "pop:" <<message << "\n";
	// 		// sleep(1);    
	// 	}
    // }
 

    //CONDITION TO VERIFY
    if(err != 0){
        test_failed_print(__func__);
    }
    test_success_print(__func__);
}

void tests_run_all(void){
    test_basic_queue_multiprocess();

}