#include <boost/lockfree/spsc_queue.hpp> // ring buffer

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>

#include "coord.h"

namespace bip = boost::interprocess;
namespace shm
{


    typedef bip::allocator<char, bip::managed_shared_memory::segment_manager> char_alloc;
    typedef bip::basic_string<char, std::char_traits<char>, char_alloc> shared_string;

    typedef boost::lockfree::spsc_queue<
        shared_string, 
        boost::lockfree::capacity<QUEUE_SIZE> 
    > ring_buffer;
}



shm_pair _boost_shm_allocator(int message_size, int client_id) {
    // printf("ALLOCATOR:\n");
    shm_pair shms = {};
    
    // int key1 = hash((unsigned char*)&client_id);
    // int temp = client_id + 1;
    // int key2 = hash((unsigned char*)&temp);

    int key1 = client_id;
    int key2 = client_id+1;


    bip::managed_shared_memory segment(bip::open_or_create,key1, 65536);
    shm::char_alloc char_alloc(segment.get_segment_manager());
    shm::ring_buffer *queue = segment.find_or_construct<shm::ring_buffer>("queue")();

    // bip::managed_shared_memory segment(bip::open_or_create, key1, 65536);

    // shm::char_alloc char_alloc(segment.get_segment_manager());

    // shm::ring_buffer *queue = segment.find_or_construct<shm::ring_buffer>("queue")();

    // while (true)
    // {
    //     shm::shared_string v(char_alloc);
    //     if (queue->pop(v))
    //         std::cout << "Processed: '" << v << "'\n";
    // }




    // int request_shmid = shm_create(key1,
    //                                QUEUE_SIZE,
    //                                create_flag);
    // int response_shmid = shm_create(key2,
    //                                 QUEUE_SIZE,
    //                                 create_flag);
    // notnets_shm_info request_shm = {key1, request_shmid};
    // notnets_shm_info response_shm = {key2, response_shmid};

    // // set up shm regions as queues
    // void* request_addr = shm_attach(request_shmid);
    // void* response_addr = shm_attach(response_shmid);
    // queue_create(request_addr, QUEUE_SIZE, message_size);
    // queue_create(response_addr, QUEUE_SIZE, message_size);

    // // don't want to stay attached to the queue pairs
    // shm_detach(request_addr);
    // shm_detach(response_addr);

    // shms.request_shm = request_shm;
    // shms.response_shm = response_shm;

    return shms;
}




// int main(){

//     bip::managed_shared_memory segment(bip::open_or_create, "MySharedMemory", 65536);

//     shm::char_alloc char_alloc(segment.get_segment_manager());

//     shm::ring_buffer *queue = segment.find_or_construct<shm::ring_buffer>("queue")();

//     while (true)
//     {
//         shm::shared_string v(char_alloc);
//         if (queue->pop(v))
//             std::cout << "Processed: '" << v << "'\n";
//     }


//     // create segment and corresponding allocator
//     bip::managed_shared_memory segment(bip::open_or_create, "MySharedMemory", 65536);
//     shm::char_alloc char_alloc(segment.get_segment_manager());

//     // Ringbuffer fully constructed in shared memory. The element strings are
//     // also allocated from the same shared memory segment. This vector can be
//     // safely accessed from other processes.
//     shm::ring_buffer *queue = segment.find_or_construct<shm::ring_buffer>("queue")();

//     const char* messages[] = { "hello world", "the answer is 42", "where is your towel", 0 };

//     for (const char** msg_it = messages; *msg_it; ++msg_it)
//     {
//         sleep(1);
//         queue->push(shm::shared_string(*msg_it, char_alloc));
//     }

// }