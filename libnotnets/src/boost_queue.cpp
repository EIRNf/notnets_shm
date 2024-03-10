#include "boost_queue.hpp"
#include <boost/interprocess/xsi_key.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/xsi_shared_memory.hpp>
#include <boost/any.hpp>
#include <boost/pointer_cast.hpp>


#define MESSAGE_SIZE 4
#define QUEUE_CAPACITY 100 //
#define MEM_SIZE 10000
#define OFFSET 224 //FIX

long offset1, offset2;

namespace bip = boost::interprocess;

typedef struct buffer_wrapper {
    char buf[MESSAGE_SIZE];
} buffer_wrapper;

shm_pair _boost_shm_allocator(int message_size, int client_id) {
      // printf("ALLOCATOR:\n");
    shm_pair shms = {};

    int key1 = client_id;
    int key2 = client_id +1;

    bip::xsi_key x_key1(key1);
    bip::managed_xsi_shared_memory mem1(bip::open_or_create, x_key1,  MEM_SIZE);

    boost::lockfree::spsc_queue<buffer_wrapper, boost::lockfree::capacity<QUEUE_CAPACITY>> *queue1 = mem1.construct<boost::lockfree::spsc_queue<buffer_wrapper, boost::lockfree::capacity<QUEUE_CAPACITY>>>("queue")();

    bip::xsi_key x_key2(key2);
    bip::managed_xsi_shared_memory mem2(bip::open_or_create, x_key2,  MEM_SIZE);
    boost::lockfree::spsc_queue<buffer_wrapper, boost::lockfree::capacity<QUEUE_CAPACITY>> *queue2 = mem2.construct<boost::lockfree::spsc_queue<buffer_wrapper, boost::lockfree::capacity<QUEUE_CAPACITY>>>("queue")();

    offset1 = mem1.get_handle_from_address(queue1);    
    offset2 = mem2.get_handle_from_address(queue2);


    notnets_shm_info request_shm = {key1,mem1.get_shmid()};
    notnets_shm_info response_shm = {key2,mem2.get_shmid()};

    shms.request_shm = request_shm;
    shms.response_shm = response_shm;

    return shms;
}


int boost_push(void* shmaddr, const void* buf, size_t buf_size){
    
    //Cast pointer to underlying queue structure, which hopefully works
    boost::lockfree::spsc_queue<buffer_wrapper, boost::lockfree::capacity<QUEUE_CAPACITY>> *queue = boost::static_pointer_cast<boost::lockfree::spsc_queue<buffer_wrapper, boost::lockfree::capacity<QUEUE_CAPACITY>>>((void*)((char*) shmaddr + OFFSET));

    int ret = queue->push((buffer_wrapper*)buf, 1);
    if (ret){ // ret == 1 if succesful as we are only pushing one at a time
        return 0;
    }
    else {
        return -1;
    }
}

size_t boost_pop(void* shmaddr, void* buf, size_t* buf_size){
    //Cast pointer to underlying queue structure, which hopefully works
    boost::lockfree::spsc_queue<buffer_wrapper, boost::lockfree::capacity<QUEUE_CAPACITY>> *queue = boost::static_pointer_cast<boost::lockfree::spsc_queue<buffer_wrapper, boost::lockfree::capacity<QUEUE_CAPACITY>>>((void*)((char*) shmaddr + OFFSET));

    buffer_wrapper* t = boost::static_pointer_cast<buffer_wrapper>(buf);

    int ret = 0;
    //Make call blocking to fit API
    while (ret != 1){
        ret = queue->pop(t, 1);
        usleep(100);
    }

    if (ret) { // ret == 1 if succesful as we are only pushing one at a time, sizeof(t->buf)
        return 0;
    }
    else {
        return -1;
    }
}
