#ifndef __RPC_H
#define __RPC_H

#include "coord.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define SIMPLE_KEY 17

#define NOT_RUNNING 0
#define RUNNING_NO_SHUTDOWN 1
#define RUNNING_SHUTDOWN 2

#define CLIENT_OPEN_WAIT_INTERVAL 200 //in us
#define SERVER_SERVICE_POll_INTERVAL 150 //in us
// This assumes two unidirectional SPSC queues, find way to generalize
// Unsure how much to keep in this ref, is
// there value in keeping all three
// pairs of keys, shmid, and ref? in case of an issue where reconnect might be attempted?
typedef struct queue_pair {
    int client_id;
    void* request_shmaddr; //pointers!!!!
    void* response_shmaddr;
    int offset;
} queue_pair;

//Would be good to split up the queue ctx between server and client
typedef struct queue_ctx {
    QUEUE_TYPE type;

    queue_pair *queues;
    //Client
    int (*client_send_rpc)(queue_pair* conn, const void* buf, size_t size);
    size_t (*client_receive_buf)(queue_pair* conn, void* buf, size_t size);

    //Server
    int (*server_send_rpc)(queue_pair* client, const void *buf, size_t size);
    size_t (*server_receive_buf)(queue_pair* client, void* buf, size_t size);
}queue_ctx;

// typedef enum QUEUE_TYPE
// {
//     POLL_QUEUE,
//     BOOST_QUEUE,
//     SEM_QUEUE
// };

typedef struct server_context {
    void* coord_shmaddr;
    pthread_t manage_pool_thread;
    // 0 = manage pool thread not running
    // 1 = manage pool thread running and server has not indicated shutdown
    // 2 = manage pool thread running and server has indicated shutdown
    int manage_pool_state;
    pthread_mutex_t manage_pool_mutex;
} server_context;

//NOTE ON ERROR HANDLING
// As this is a low level C library we should use the global
// errno on failure. A lot of potential issues that can occur
// are related to the access and management of the SHM layer
// which we can transparently utilize.


// forward declaration of functions
// ===============================================================
// PRIVATE HELPER FUNCTIONS
queue_pair* _create_client_queue_pair(coord_header* ch, int slot, QUEUE_TYPE type);
queue_pair* _create_server_queue_pair(coord_header* ch, int slot);

void* _manage_pool_runner(void* handler);

queue_ctx* select_queue_ctx(QUEUE_TYPE t);
void free_queue_ctx(queue_ctx * ctx);

// CLIENT
queue_ctx* client_open(char* source_addr,
                        char* destination_addr,
                        int message_size,
                        QUEUE_TYPE type);

int client_send_rpc(queue_ctx* conn, const void* buf, size_t size);
size_t client_receive_buf(queue_ctx* conn, void* buf, size_t size);
int client_close(char* source_addr, char* destination_addr);

// SERVER
void manage_pool(server_context* handler);
server_context* register_server(char* source_addr);
queue_ctx* accept(server_context* handler);
int server_send_rpc(queue_ctx* client, const void *buf, size_t size);
size_t server_receive_buf(queue_ctx* client, void* buf, size_t size);
void shutdown(server_context* handler);
// ===============================================================

// PRIVATE HELPER FUNCTIONS
queue_pair* _create_client_queue_pair(coord_header* ch, int slot);



queue_pair* _create_server_queue_pair(coord_header* ch, int slot);




void* _manage_pool_runner(void* handler);

// CLIENT
/**
 * @brief Client attempts a connection to a known instantiated
 * and listening Notnets server. This process involves hashing
 * the address to resolve for a unique shm region key in which
 * a coord request for a new queue pair can be made. This can
 * fail due to the server not having initialized its coord region,
 * no available reserve slots,etc
 * 
 * TODO; Add configurable timeout.
 *
 *
 * @param source_addr[IN] source_addr: Client Address + Nonce.
 * @param destination_addr[IN] Unique string to identify server address,
 * usually IP/Port pair
 * @return connection* If NULL, no connection was achieved
 */
queue_ctx* client_open(char* source_addr,
                        char* destination_addr,
                        int message_size,
                        QUEUE_TYPE type);

/**
 * @brief Client writes to the request queue in given connection.
 *
 * TODO:
 * - What if queue full? return 0 for bytes written
 *
 * @param queues[IN] queues initialized with server
 * @param buf[IN] data buf to be written
 * @param size[IN] size of buf
 * @return int -1 for error, 0 if successfully pushed
 */
int client_send_rpc(queue_ctx* queues, const void *buf, size_t size);


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
size_t client_receive_buf(queue_ctx* queues, void* buf, size_t size);

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
 * @return -1 if did not successfully close, 0 otherwise
*/
int client_close(char* source_addr, char* destination_addr);

//SERVER
shm_pair _fake_shm_allocator(int message_size, int client_id);

shm_pair _hash_shm_allocator(int message_size, int client_id);

shm_pair _rand_shm_allocator(int message_size, int client_id);

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
 * TODO//Check for repeat entries!!!!!!!!!

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
queue_ctx* accept(server_context* handler);

/**
 * @brief The client must be identified before written to.
 *
 * @param client[IN] queue pair
 * @param buf[IN] data buf to be written
 * @param size[IN] size of buf
 * @return ssize_t  -1 for error, 0 for successful push
 */
int server_send_rpc(queue_ctx* queues, const void *buf, size_t size);

/**
 * @brief Buffer provided by user. Called until all data has been read from
 * message queue.
 *
 * TODO: How do we keep track in the message queue how much has been written?
 *
 * @param client [IN] queue pair
 * @param buf [IN] data buf to be copied to
 * @param size [IN] size of buf
 * @return size_t How much read, if 0 EOF. Only removed from queue once fully read.
 */
size_t server_receive_buf(queue_ctx* queues, void* buf, size_t size);

/**
 * @brief Gracefully shutdown coord and all queues. Signal
 * and wait until clients do so.
 *
 * @param handler [IN] server client connection handler
 */
void shutdown(server_context* handler);

#endif
