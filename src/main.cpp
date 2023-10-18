#include <iostream>
#include "mem.h"


int main() {

	std::cout << "Hello World!";


	//ALLOCATE MEMORY REGION
	shared_memory_region test_region(1, 16, create_flag);
	int shmid = test_region.create();
	void* shmaddr = test_region.attach(shmid);

	test_region.remove();

	//ENQUEUE SOMETHING TO IT


	//DEQUEUE SOMETHING TO IT



	return 0;

}


