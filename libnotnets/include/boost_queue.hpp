#ifndef __BOOST_QUEUE_HPP
#define __BOOST_QUEUE_HPP

#include "coord.h"
#include "mem.h"

#include <boost/lockfree/spsc_queue.hpp> // ring buffer
#include <boost/interprocess/managed_xsi_shared_memory.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
// #include <boost/interprocess/allocators/allocator.hpp>
// #include <boost/interprocess/containers/string.hpp>

namespace bip = boost::interprocess;

shm_pair _boost_shm_allocator(int message_size, int client_id);

ssize_t queue_create(void* shmaddr, size_t shm_size, size_t message_size);

int boost_push(void* shmaddr, const void* buf, size_t buf_size);

size_t boost_pop(void* shmaddr, void* buf, size_t* buf_size);
 
#endif

