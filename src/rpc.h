#ifndef __RPC_H
#define __RPC_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

// This assumes two unidirectional SPSC queues, find way to generalize
// Unsure how much to keep in this ref, is 
// there value in keeping all three
// pairs of keys, shmid, and ref? in case of an issue where reconnect might be attempted?
typedef struct queue_ref {
    void* shm_request_shmaddr;
    void* shm_response_shmaddr;
} queue_ref;

typedef struct client_queue {
    uint64_t id;
    queue_ref queue;
} client_conn;

typedef struct connection {
    uint64_t id; // client id
    char* address; // IP/Port Pair
    uint8_t port; // Port 
    void* coord_shmaddr; //Gotten after shmat
    queue_ref queue; // Reference to underlying shm queue mechanism
} connection; 

struct connection_handler {
    char* address; // IP/Port pair
    key_t coord_key; // Hashed from IP and Port
    void* coord_shmaddr;
    client_queue* client_queues; //Queues used to by live client connection. TODO: Fixed array
    queue_ref* free_queues; //Available queues to be assigned to new clients.  TODO: Fixed array
};


//NOTE ON ERROR HANDLING
// As this is a low level C library we should use the global 
// errno on failure. A lot of potential issues that can occur 
// are related to the access and management of the SHM layer
// which we can transparently utilize. 


//SYNCHRONOUS
 
//CLIENT
/**
 * @brief Client attempts a connection to a known instantiated 
 * and listening Notnets server. This process involves hashing
 * the address to resolve for a unique shm region key in which
 * a coord request for a new queue pair can be made. This can 
 * fail due to the server not having initialized its coord region,
 * no available reserve slots. 
 * 
 * @param address[IN] Unique string to identify server address,
 * usually IP/Port pair
 * @return connection* If NULL, no connection was achieved
 */
connection* connection_init(uint64_t id, char* address);
/**
 * @brief Client writes to request queue in given connection.
 * 
 * @param buf[IN] data buf to be written
 * @param size[IN] size of buf
 * @param conn[IN] connection initialized with server
 * @return ssize_t -1 for error, >0 for bytes written
 */
ssize_t notnets_conn_write(const void *buf, size_t size, connection* conn);

/**
  * @brief Client reads from response queue in given connection.
 * TODO: Question: Should read be preallocated? Message size will be determined
 * by the server writing the response. This will be the message in the response queue. 
 * 
 * TODO: Issue. Current queue design assumes that a read by a consumer of the 
 * queue will update the flag that permits the producer to keep on writing. 
 * Changing this might add significant complexity to the queue. However, if 
 * in order to avoid a data copy our read only returns a reference to the 
 * message in the queue we cannot ensure that the message is dereferenced 
 * before it is overwritten. A brainstorm of potential solutions follows below.  
 * 
 * This is complicated and worth of discussion. Many different design decisions:
 * 1. Incur a data copy, server allocates and copies of the shm queue 
 * 2. Make a really really big buffer that makes queue pressure unlikely?
 * 3. Shm memory allocator and only pointers are passed in the queue, once the read has occurred
 * we can confidently dequeue it, however we still have to manage the other allocated function.
 * This is (kinda) the design approach taken by the paper.  Partial Failure Resilient Memory Management System for (CXL-based) Distributed Shared Memory. It requires a change of interface and garbage collecting.
 * 4. Unsure of how CXL work but maybe, could you get a handle to the nic's packet buffers of the wire
 * and map them to the process's address space. The nic would have to have a way of recognizing a
 * particular message, and separate them from the usual buffers and pass the responsibility for freeing
 * those buffers to the notnets process? This is how TCP zero copy kinda works but the memory mapping
 * introduces overhead. If you could have a set region so that you had a single mapping it might not be so bad.
 * 5. Transformation function? Pass a function into the read that ensures that overwritten would not be a problem. I'm imagining this to be a serialization function that would carry out the read and the subsequent necessary data transformation. This might be quite difficult to implement in certain frameworks, and is def a bit of a hack. 
 * - Alternative, ensure this function only gets called when transformed? Very framework dependent
 * 
 * @param size[OUT] 
 * @param conn[IN] 
 * @return void* pointer to memory buffer containing message
 */
void* notnets_conn_read(ssize_t *size, connection* conn);

/**
 * @brief Closes the client connection. Signal to server
 * to free up its reserved slot in Coord, and detachment 
 * of shm queues. 
 * 
 * @param conn[IN] connection initialized with server
 */
void close(connection* conn);

//SERVER
/**
 * @brief Server will instantiate necessary coord memory regions. 
 * Once instantiate server can begin servicing client requests
 * 
 * 1. Invoked at instantiation of server. Only once per server
 * 
 * @param address[IN] Unique string to identify desired server address,
 * usually IP/Port pair
 * @return connection_handler* If NULL, coord memory region was not 
 * created successfully 
 */
connection_handler* handler_init(char* address);

/**
 * @brief TODO: What is the ideal approach here?
 * Currently the handler state would be updated 
 * with the state of clients and queues in coord. 
 * Blocking function for the rest client, 
 * 
 * 1. Needs to be periodically run to service 
 * client requests: connection_init, stop
 * 
 * @param handler[IN/OUT] server client connection handler
 */
void handler_service_conn_requests(connection_handler* handler);

/**
 * 
 * @brief TODO: Unsure about this implementation. Write single message
 * to client response queue.
 * The client to be written to must be known when calling this function.
 * A generalized write would not make sense.
 * The handler state should not be modified in this call. 
 * 
 * @param buf[IN] data buf to be written
 * @param size[IN] size of buf
 * @param client[IN] client details to write to.
 * @return ssize_t  -1 for error, >0 for bytes written
 */
ssize_t handler_write_to_client(const void *buf, size_t size, client_queue* client);
/**
 * @brief Read single message of given client request queue. 
 * TODO: Question: Should read be preallocated? Message size will be determined
 * by the client. Probs not used
 * 
 * @param buf[OUT] data buf for data to be read into
 * @param size[OUT] size of read to be performed
 * @param client[IN] connection initialized with server
 * @return ssize_t -1 for error, >0 for bytes read
 */
ssize_t handler_read_single_conn(void *buf, size_t *size,  client_queue* client);
/**
 * @brief This method is a generic read to read a single incoming request
 * from any client. The order of these reads will depend on a queuing algorithm,
 * ie. round robin, fifo, etc... We can choose to transparent the queuing methodology
 * for whatever consuming system that uses this.
 * TODO: Question: Should read be preallocated? Message size will be determined
 * by the client.
 * TODO: What is the correct way to read incoming messages? There could be 
 * a variety of queuing algorithms we could choose. There is an interesting 
 * opportunity for per connection load balancing at the server level to occur.
 * But for not an algorithm that ensures fairness would be best. Round robin seems
 * like the best and easiest for now, as other algorithms would require greater visibility
 * into the state of the queues. Round robin represents different behavior than how
 * a typical socket read would work, as sockets order messages by delivery (i think). 
 * 
 * @param buf [OUT] data buf for data to be read into
 * @param size [OUT] size of read to be performed
 * @param handler [IN] server client connection handler
 * @return ssize_t -1 for error, >0 for bytes read
 */
void* handler_read(size_t *size, connection_handler* handler, client_queue** client);



/**
 * @brief Gracefully shutdown coord and all queues. Signal
 * and wait until clients do so. 
 * 
 * @param handler [IN] server client connection handler
 */
void shutdown(connection_handler* handler);

#endif
