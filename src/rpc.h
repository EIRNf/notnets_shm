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
 * no available reserve slots. 
 * 
 * // Client ID: Client Address + Nonce.
 * // It is an identifier IP and Sequence Number 
 * 
 * @param address[IN] Unique string to identify server address,
 * usually IP/Port pair
 * @return connection* If NULL, no connection was achieved
 */
queue_pair* open(char* source_addr, char* destination_addr);


/**
 * @brief Client writes to request queue in given connection.
 * 
 * TODO: 
 * - What if queue full? return 0 for bytes written
 * 
 * @param buf[IN] data buf to be written
 * @param size[IN] size of buf
 * @param conn[IN] queues initialized with server
 * @return ssize_t -1 for error, >0 for bytes written
 */
ssize_t send_rpc(queue_pair* conn, const void *buf, size_t size);


/**
 * @brief Buffer provided by user. Called until all data has been read from 
 * message queue.
 * 
 * TODO: How do we keep track in the message queue how much has been written?
 * 
 * 
 * @param conn [IN] queues initialized with server
 * @param buf [IN] data buf to be copied to
 * @param size [IN] size of buf
 * @return ssize_t How much read, if 0 EOF. Only removed from queue once fully read. 
 */
ssize_t receive_rpc(queue_pair* conn, void** buf, size_t size);


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
 * @param conn[IN] connection initialized with server
 */
void close(char* source_addr, char* destination_addr);



//SERVER
/**
 * @brief Server will instantiate necessary coord memory regions. 
 * Once instantiate server can begin servicing client requests
 * 
 * 1. Invoked at instantiation of server. Only once per server
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
 * Returns a queue pair corresponding to a client who requested it.
 * Will only service one client per call? In round robin, cycles
 * through currently servicing clients and returns queue pair accordingly.
 * This differs from the usual listen/accept pattern
 * 
 * 1. Needs to be periodically run to service 
 * client requests: open, close
 * 
 * @param handler[IN/OUT] server client connection handler
 */
queue_pair* listen(server_context* handler);


/**
 * @brief 
 * 
 * Service client requests, allocate communication queues, and
 * signal to clients their details. Modifies coord space.
 * 
 * @param handler [IN/OUT] server client connection handler
 * @return queue_pair 
 */
int manage_conn(server_context* handler);


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
ssize_t receive_rpc(queue_pair* client, void** buf, size_t size);

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
void* receive_next_rpc(size_t *size, server_context* handler, queue_pair** client);


/**
 * @brief Gracefully shutdown coord and all queues. Signal
 * and wait until clients do so. 
 * 
 * @param handler [IN] server client connection handler
 */
void shutdown(server_context* handler);

#endif
