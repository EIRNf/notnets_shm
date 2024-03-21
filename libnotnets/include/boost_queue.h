#ifndef __BOOST_QUEUE_H
#define __BOOST_QUEUE_H

#include "coord.h"
#include "mem.h"


shm_pair _boost_shm_allocator(int message_size, int client_id);

ssize_t queue_create(void* shmaddr, size_t shm_size, size_t message_size);

int boost_push(void* shmaddr, const void* buf, size_t buf_size,int offset);

size_t boost_pop(void* shmaddr, void* buf, size_t* buf_size, int offset);
 
#endif

