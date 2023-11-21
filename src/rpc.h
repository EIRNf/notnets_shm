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

#define NOT_RUNNING 0
#define RUNNING_NO_SHUTDOWN 1
#define RUNNING_SHUTDOWN 2

// This assumes two unidirectional SPSC queues, find way to generalize
// Unsure how much to keep in this ref, is
// there value in keeping all three
// pairs of keys, shmid, and ref? in case of an issue where reconnect might be attempted?
typedef struct queue_pair {
    uint64_t id;
    void* shm_request_shmaddr;
    void* shm_response_shmaddr;
} queue_pair;

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
int send_rpc(queue_pair* conn, const void *buf, size_t size); //TODO


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
ssize_t receive_buf(queue_pair* conn, void* buf, size_t size);


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
void rpc_close(char* source_addr, char* destination_addr);



//SERVER
void shutdown(server_context* handler);

shm_pair fake_shm_allocator() {
    int fake_key = 17;

    shm_pair shms = {};

    int request_shmid = shm_create(fake_key,
                                   QUEUE_SIZE,
                                   create_flag);
    int response_shmid = shm_create(fake_key + 1,
                                    QUEUE_SIZE,
                                    create_flag);
    shm_info request_shm = {fake_key, request_shmid};
    shm_info response_shm = {fake_key + 1, response_shmid};

    // set up shm regions as queues
    void* request_addr = shm_attach(request_shmid);
    void* response_addr = shm_attach(response_shmid);
    int message_size = sizeof(int);
    queue_create(request_addr, QUEUE_SIZE, message_size);
    queue_create(response_addr, QUEUE_SIZE, message_size);
    // don't want to stay attached to the queue pairs
    shm_detach(request_addr);
    shm_detach(response_addr);

    shms.request_shm = request_shm;
    shms.response_shm = response_shm;

    return shms;
}

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
void manage_pool(server_context* handler) {
    coord_header* ch = (coord_header*) handler->coord_shmaddr;

    for (int i = 0; i < SLOT_NUM; ++i) {
        int client_id = service_slot(ch, i, &fake_shm_allocator);

        // will happen if keys already exist
        if (client_id == -1) {
            // cleanup
            pthread_mutex_lock(&ch->mutex);
            if (ch->slots[i].detach == true) {
                // delete queues
                shm_remove(ch->slots[i].shms.request_shm.shmid);
                shm_remove(ch->slots[i].shms.response_shm.shmid);

                memset(&ch->slots[i], 0, sizeof(coord_row));
                ch->available_slots[i].client_request = false;
            }
            pthread_mutex_unlock(&ch->mutex);
        }
    }
}

void* _manage_pool_runner(void* handler) {
    server_context* sc = (server_context*) handler;

    while (true) {
        // check if need to shut down
        pthread_mutex_lock(&sc->manage_pool_mutex);
        if (sc->manage_pool_state == RUNNING_SHUTDOWN) {
            sc->manage_pool_state = NOT_RUNNING;
            pthread_mutex_unlock(&sc->manage_pool_mutex);
            return NULL;
        }
        pthread_mutex_unlock(&sc->manage_pool_mutex);

        manage_pool(sc);

        sleep(2);
    }

    return NULL;
}

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
server_context* register_server(char* source_addr) {
    coord_header* ch = coord_create(source_addr);

    server_context* sc = malloc(sizeof(server_context));

    // coord_header, at the end of the day, is just a shm region
    sc->coord_shmaddr = (void*) ch;

    // start manage pool runner thread
    int err = pthread_create(&sc->manage_pool_thread,
                             NULL,
                             _manage_pool_runner,
                             sc);

    if (err != 0) {
        perror("pthread_create");
        shutdown(sc);
        return NULL;
    }

    if (pthread_mutex_init(&sc->manage_pool_mutex, NULL) == -1) {
        perror("error create manage pool mutex");
    }
    sc->manage_pool_state = RUNNING_NO_SHUTDOWN;

    return sc;
}

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
int send_rpc(queue_pair* client, const void *buf, size_t size);

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
ssize_t receive_buf(queue_pair* client, void* buf, size_t size);


/**
 * @brief Gracefully shutdown coord and all queues. Signal
 * and wait until clients do so.
 *
 * @param handler [IN] server client connection handler
 */
void shutdown(server_context* handler) {
    coord_header* ch = (coord_header*) handler->coord_shmaddr;

    // tell manage pool thread to shut down
    pthread_mutex_lock(&handler->manage_pool_mutex);
    handler->manage_pool_state = RUNNING_SHUTDOWN;
    pthread_mutex_unlock(&handler->manage_pool_mutex);

    // wait for manage pool thread to shut down
    while (true) {
        pthread_mutex_lock(&handler->manage_pool_mutex);
        if (handler->manage_pool_state == NOT_RUNNING) {
            pthread_mutex_unlock(&handler->manage_pool_mutex);
            break;
        }
        pthread_mutex_unlock(&handler->manage_pool_mutex);
    }

    coord_shutdown(ch);
    manage_pool(handler);
    coord_delete(ch);
    free(handler);
}

#endif
