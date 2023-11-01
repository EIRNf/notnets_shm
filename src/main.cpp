#include <iostream>
#include <unistd.h> 
#include "mem.h"
#include "queue.h"
#include "coord.h"


void child_process_test(){

		//Forking to test multiprocess functionality
	pid_t c_pid = fork(); 
  
    if (c_pid == -1) { 
        perror("fork"); 
        exit(EXIT_FAILURE); 
    } 
	else if (c_pid > 0)  {  //PARENT PROCESS
		//ALLOCATE MEMORY REGION
		shared_memory_region test_region(1, 200, create_flag);
		int shmid = test_region.create();
		void* shmaddr = test_region.attach(shmid);

		//Create queue, for proper shared memory usage you would need to "attach" to an existing implementation. Implementing asap
		//Make sure the shared memory is big enough to hold header + atleast one message
		spsc_queue<int> test_queue;
		test_queue.create(&test_region);

        //  wait(nullptr); 
        std::cout << "printed from parent process: " << getpid() 
             << std::endl; 


		//ENQUEUE SOMETHING TO IT
		for(int i = 0; i < 10; i++){
			std::cout << "push:" << i << "\n";
			test_queue.push(i);
			// sleep(1);    
		}

		test_region.remove();

    } 
    else { //CHILD PROCESS

		//ALLOCATE MEMORY REGION
		shared_memory_region test_region(1, 200, create_flag);
		int shmid = test_region.create();
		void* shmaddr = test_region.attach(shmid);

		//Create queue, for proper shared memory usage you would need to "attach" to an existing implementation. Implementing asap
		//Make sure the shared memory is big enough to hold header + atleast one message
		spsc_queue<int> test_queue;
		test_queue.attach(&test_region);


		std::cout << "printed from child process: " << getpid() 
             << std::endl; 

		//DEQUEUE SOMETHING TO IT
		for(int i = 0; i < 10; i++){
			int message = test_queue.pop();
			std::cout << "pop:" <<message << "\n";
			// sleep(1);    
		}
    }
 

}


void *Thread1(void *x) {


  return NULL;
}


void *Thread2(void *x) {


  return NULL;
}


void data_race_test(){

	// pthread_t t[2];


	// //ALLOCATE MEMORY REGION
	// shared_memory_region test_region(1, 200, create_flag);
	// int shmid = test_region.create();
	// void* shmaddr = test_region.attach(shmid);

	// //Create queue, for proper shared memory usage you would need to "attach" to an existing implementation. Implementing asap
	// //Make sure the shared memory is big enough to hold header + atleast one message
	// spsc_queue<int> test_queue;
	// test_queue.create(&test_region);


	// //ALLOCATE MEMORY REGION
	// shared_memory_region test_region(1, 200, create_flag);
	// int shmid = test_region.create();
	// void* shmaddr = test_region.attach(shmid);



	// //Create queue, for proper shared memory usage you would need to "attach" to an existing implementation. Implementing asap
	// //Make sure the shared memory is big enough to hold header + atleast one message
	// spsc_queue<int> test_queue;
	// test_queue.attach(&test_region);



	// pthread_create(&t[0], NULL, Thread1, NULL);
	// pthread_create(&t[1], NULL, Thread2, NULL);
	// pthread_join(t[0], NULL);
	// pthread_join(t[1], NULL);


}

key_pair coord_test_name_keys(int test){
	int test1 = test+1;
	return key_pair{test, test1};
}

key_pair coord_test_queue_allocation(int key_seed){
	key_pair keys = coord_test_name_keys(key_seed);

	//ALLOCATE MEMORY REGIONS
	shared_memory_region request_test_region(keys.request_shm_key, 200, create_flag);
	shared_memory_region response_test_region(keys.response_shm_key, 200, create_flag);

	int request_shmid = request_test_region.create();
	void* request_shmaddr = request_test_region.attach(request_shmid);

	int response_shmid = response_test_region.create();
	void* response_shmaddr = response_test_region.attach(response_shmid);

	//Create queue, for proper shared memory usage you would need to "attach" to an existing implementation. Implementing asap
	//Make sure the shared memory is big enough to hold header + atleast one message
	spsc_queue<int> request_queue;
	request_queue.create(&request_test_region);

	spsc_queue<int> response_queue;
	response_queue.create(&response_test_region);

	return keys;
}


void coord_test(){

	//Forking to test multiprocess functionality
	pid_t c_pid = fork(); 
  
    if (c_pid == -1) { 
        perror("fork"); 
        exit(EXIT_FAILURE); 
    } 
    else if (c_pid > 0) {  //PARENT PROCESS

		shared_memory_region coord_region(1, 200, create_flag);
		coord_region.create();

		coord test_coord;
		test_coord.create(&coord_region);

		sleep(2);
		test_coord.service_requests(&coord_test_queue_allocation, 17);
		sleep(2);

		key_pair* keys = test_coord.query_handled_requests();

		for (int i=0;i<10;i++)
		{
			std::cout << keys[i].request_shm_key << '\n';
			std::cout << keys[i].response_shm_key << '\n';
		}

    } 
    else { //CHILD PROCESS

		shared_memory_region coord_region(1, 200, create_flag);
		coord_region.create();

		coord test_coord;
		test_coord.attach(&coord_region);

		key_pair keys = test_coord.try_request_keys(c_pid);
		std::cout << keys.request_shm_key << '\n';
		std::cout << keys.response_shm_key << '\n';


		// key_pair* keys = test_coord.query_handled_requests();

		// for (int i=0;i<10;i++)
		// {
		// 	std::cout << keys[i].request_shm_key << '\n';
		// 	std::cout << keys[i].response_shm_key << '\n';
		// }

		
    } 

}



int main() {
	// data_race_test();
	// child_process_test();
	coord_test();

	return 0;

}


