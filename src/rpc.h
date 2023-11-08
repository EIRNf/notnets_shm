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
typedef struct queue_pair {
    uint64_t id;
    void* shm_request_shmaddr;
    void* shm_response_shmaddr;
} queue_pair;

struct server_context {
    void* coord_shmaddr;
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
 * no available reserve slots,etc 
 * 
 * 
 * @param source_addr[IN] source_addr: Client Address + Nonce.
 * @param destination_addr[IN] Unique string to identify server address,
 * usually IP/Port pair
 * @return connection* If NULL, no connection was achieved
 */
queue_pair* open(char* source_addr, char* destination_addr);


/**
 * @brief Client writes to the request queue in given connection.
 * 
 * TODO: 
 * - What if queue full? return 0 for bytes written
 * 
 * @param conn[IN] queues initialized with server
 * @param buf[IN] data buf to be written
 * @param size[IN] size of buf
 * @return ssize_t -1 for error, >0 for bytes written
 */
ssize_t send_rpc(queue_pair* conn, const void *buf, size_t size);


/**
 * @brief Buffer provided by user. Called until all data has been read from 
 * message queue.
 * 
 * TODO: How do we keep track in the message queue how much has been written?
 * TODO: How can we make the implementation zero-copy?
 * 
 * @param conn [IN] queues initialized with server
 * @param buf [IN] data buf to be copied to
 * @param size [IN] size of buf
 * @return ssize_t How much read, if 0 EOF. Only removed from queue once fully read. 
 */
ssize_t receive_buf(queue_pair* conn, void** buf, size_t size);


/**
 * @brief Closes the client connection. Signal to server
 * to free up its reserved slot in Coord, and detachment 
 * of shm queues. 
 * 
 * TODO: not urgent,
 * - invalid clients
 * - dont close the same thing twice
 * - malformed
 * - ERRNO
 * 
 * @param source_addr[IN] source_addr: Client Address + Nonce.
 * @param destination_addr[IN] Unique string to identify server address,
 * usually IP/Port pair 
*/
void close(char* source_addr, char* destination_addr);



//SERVER
/**
 * @brief Server will instantiate necessary coord memory regions. 
 * Once instantiate server can begin servicing client requests. 
 * Only once per server
 * 
 * @param source_addr[IN] Unique string to identify desired server address,
 * usually IP/Port pair
 * @return server_context* If NULL, coord memory region was not 
 * created successfully 
 */
server_context* register_server(char* source_addr);


/**
 * @brief 
 * 
 * Service client requests, allocate communication queues, and
 * signal to clients their queue details. Modifies coord space.
 *  * 
 * 1. Needs to be periodically run to service 
 * client requests: open, close
 * 2. Service client requests in order of arrival. 
 * 
 * @param handler [IN/OUT] server client connection handler
 * 
 */
void manage_pool(server_context* handler);


/**
 * @brief
 * 
 * Returns a queue pair corresponding to a client who requested it.
 * Will only service one client per call? In round robin fashion, cycles
 * through currently servicing clients and returns queue pair accordingly.
 * This differs from the usual listen/accept pattern.
 * 
 * 
 * @param handler[IN/OUT] server client connection handler
 * @return queue_pair 
 */
queue_pair* accept(server_context* handler);


/**
 * @brief The client must be identified before written to. 
 * 
 * @param client[IN] queue pair 
 * @param buf[IN] data buf to be written
 * @param size[IN] size of buf
 * @return ssize_t  -1 for error, >0 for bytes written
 */
ssize_t send_rpc(queue_pair* client, const void *buf, size_t size);

/**
 * @brief Buffer provided by user. Called until all data has been read from 
 * message queue.
 * 
 * TODO: How do we keep track in the message queue how much has been written?
 * 
 * @param client [IN] queue pair
 * @param buf [IN] data buf to be copied to
 * @param size [IN] size of buf
 * @return ssize_t How much read, if 0 EOF. Only removed from queue once fully read. 
 */
ssize_t receive_buf(queue_pair* client, void** buf, size_t size);


/**
 * @brief Gracefully shutdown coord and all queues. Signal
 * and wait until clients do so. 
 * 
 * @param handler [IN] server client connection handler
 */
void shutdown(server_context* handler);

#endif
