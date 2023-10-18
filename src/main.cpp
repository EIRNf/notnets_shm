#include <iostream>
#include "mem.h"
#include "queue.h"



int main() {

	//ALLOCATE MEMORY REGION
	shared_memory_region test_region(1, 200, create_flag);
	int shmid = test_region.create();
	void* shmaddr = test_region.attach(shmid);

	//ENQUEUE SOMETHING TO IT
	spsc_queue<int> test_queue;
	test_queue.create(&test_region);

	test_queue.push(19);

	//DEQUEUE SOMETHING TO IT
	int message = test_queue.pop();

	std::cout << message;


	test_region.remove();


	return 0;

}


