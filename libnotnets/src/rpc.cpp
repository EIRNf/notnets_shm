
#include "rpc.h"
#include "mem.h"
#include "coord.h"
#include "queue.h"
#include "boost_queue.h"
#include "sem_queue.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

//NOTE ON ERROR HANDLING
// As this is a low level C library we should use the global
// errno on failure. A lot of potential issues that can occur
// are related to the access and management of the SHM layer
// which we can transparently utilize.


// forward declaration of functions
// ===============================================================
// PRIVATE HELPER FUNCTIONS
queue_pair* _create_client_queue_pair(coord_header* ch, int slot);
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
queue_pair* _create_client_queue_pair(coord_header* ch, int slot) {
    shm_pair shms = check_slot(ch, slot);
    int id = get_client_id(ch, slot);
    // keep on checking slot until receive shms
    while (shms.request_shm.key == 0 || shms.response_shm.key == 0) {
        shms = check_slot(ch, slot);
    }

    queue_pair* qp = (queue_pair*) malloc(sizeof(queue_pair));

    //already attached, return empty addresses? It should not pollute upstream if a record is kept.
    if (get_client_attach_state(ch, slot)){
        qp->request_shmaddr = NULL;
        qp->response_shmaddr = NULL;
        qp->client_id = id;
        qp->offset= shms.offset;
    } else {
        set_client_attach_state(ch, slot,true);
        qp->request_shmaddr = shm_attach(shms.request_shm.shmid);
        qp->response_shmaddr = shm_attach(shms.response_shm.shmid);
        qp->client_id = id;
        qp->offset= shms.offset;
    } 
    atomic_thread_fence(memory_order_seq_cst);

    return qp;
}



queue_pair* _create_server_queue_pair(coord_header* ch, int slot) {
    shm_pair shms = check_slot(ch, slot);
    int id = get_client_id(ch, slot);
    // keep on checking slot until receive shms
    while (shms.request_shm.key == 0 || shms.response_shm.key == 0) {
        shms = check_slot(ch, slot);
    }

    queue_pair* qp = (queue_pair*) malloc(sizeof(queue_pair));

    //already attached, return empty addresses? It should not pollute upstream if a record is kept.
    
    //If two attached, return null, not permitted
    if (get_server_attach_state(ch, slot)){
        qp->request_shmaddr = NULL;
        qp->response_shmaddr = NULL;
        qp->client_id = id;
        qp->offset= shms.offset;
    } else {
        set_server_attach_state(ch, slot, true);
        qp->request_shmaddr = shm_attach(shms.request_shm.shmid);
        qp->response_shmaddr = shm_attach(shms.response_shm.shmid);
        qp->client_id = id;
        qp->offset= shms.offset;
    } 

    atomic_thread_fence(memory_order_seq_cst);


    return qp;
}





void* _manage_pool_runner(void* handler) {
    server_context* sc = (server_context*) handler;

    while (true) {
        // check if need to shut down
        // pthread_mutex_lock(&sc->manage_pool_mutex);
        if (sc->manage_pool_state == RUNNING_SHUTDOWN) {
            sc->manage_pool_state = NOT_RUNNING;
            // pthread_mutex_unlock(&sc->manage_pool_mutex);
            return NULL;
        }
        // pthread_mutex_unlock(&sc->manage_pool_mutex);

        manage_pool(sc);

        usleep(SERVER_SERVICE_POll_INTERVAL);  
        // sleep(1);
    }

    return NULL;
}

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
                        QUEUE_TYPE type) {

    // printf("CLIENT: OPEN:\n");

    coord_header* ch = coord_attach(destination_addr);

    if (ch == NULL) {
        return NULL;
    }
    
    int client_id = (int) hash((unsigned char*) source_addr);
    int slot = request_slot(ch, client_id, message_size, type);


    // keep on trying to reserve slot, if all slots are taken but no one detaches you wait forever
    while (slot == -1) {
        slot = request_slot(ch, client_id, message_size, type);
        usleep(CLIENT_OPEN_WAIT_INTERVAL);
    }

    //TODO:
    //During testing, if shutdown is called during this poll interval
    //the thread will deadlock and loop forever. This occurs as the 
    //shutdown function will clear the reservation slots. 
    while(!(ch->slots[slot].shm_created)){
        usleep(CLIENT_OPEN_WAIT_INTERVAL); 
        continue;
    }

    queue_ctx* q_ctx= select_queue_ctx(type);
    q_ctx->queues = _create_client_queue_pair(ch, slot);

    return q_ctx;
}

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
int client_send_rpc(queue_ctx* queues, const void *buf, size_t size) {
        return queues->client_send_rpc(queues->queues,buf,size);
}


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
size_t client_receive_buf(queue_ctx* queues, void* buf, size_t size) {
    return queues->client_receive_buf(queues->queues, buf, size);
}


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
int client_close(char* source_addr, char* destination_addr) {
    int client_id = (int) hash((unsigned char*) source_addr);
    coord_header* ch = coord_attach(destination_addr);

    // pthread_mutex_lock(&ch->mutex);
    for (int i = 0; i < SLOT_NUM; ++i) {
        if (ch->slots[i].client_id == client_id) {
            ch->slots[i].detach = true;

            // pthread_mutex_unlock(&ch->mutex);
            coord_detach(ch);

            return 0;
        }
    }

    // pthread_mutex_unlock(&ch->mutex);
    coord_detach(ch);

    return -1;
}

//SERVER
shm_pair _fake_shm_allocator(int message_size, int client_id) {
    shm_pair shms = {};


    (void) client_id;

    int request_shmid = shm_create(SIMPLE_KEY,
                                   QUEUE_SIZE,
                                   create_flag);
    int response_shmid = shm_create(SIMPLE_KEY + 1,
                                    QUEUE_SIZE,
                                    create_flag);
    notnets_shm_info request_shm = {SIMPLE_KEY, request_shmid};
    notnets_shm_info response_shm = {SIMPLE_KEY + 1, response_shmid};

    // set up shm regions as queues
    void* request_addr = shm_attach(request_shmid);
    void* response_addr = shm_attach(response_shmid);
    queue_create(request_addr, QUEUE_SIZE, message_size);
    queue_create(response_addr, QUEUE_SIZE, message_size);
    // don't want to stay attached to the queue pairs
    shm_detach(request_addr);
    shm_detach(response_addr);

    shms.request_shm = request_shm;
    shms.response_shm = response_shm;

    return shms;
}



shm_pair _hash_shm_allocator(int message_size, int client_id) {
    // printf("ALLOCATOR:\n");
    shm_pair shms = {};
    
    // int key1 = hash((unsigned char*)&client_id);
    // int temp = client_id + 1;
    // int key2 = hash((unsigned char*)&temp);

    int key1 = client_id;
    int key2 = client_id+1;

    int request_shmid = shm_create(key1,
                                   QUEUE_SIZE,
                                   create_flag);
    int response_shmid = shm_create(key2,
                                    QUEUE_SIZE,
                                    create_flag);
    notnets_shm_info request_shm = {key1, request_shmid};
    notnets_shm_info response_shm = {key2, response_shmid};

    // set up shm regions as queues
    void* request_addr = shm_attach(request_shmid);
    void* response_addr = shm_attach(response_shmid);
    queue_create(request_addr, QUEUE_SIZE, message_size);
    queue_create(response_addr, QUEUE_SIZE, message_size);

    // don't want to stay attached to the queue pairs
    shm_detach(request_addr);
    shm_detach(response_addr);

    shms.request_shm = request_shm;
    shms.response_shm = response_shm;

    return shms;
}

shm_pair _rand_shm_allocator(int message_size, int client_id) {
    // printf("ALLOCATOR:\n");


    (void) client_id;
    srand(time(0));

    shm_pair shms = {};

    int key1 = rand();
    int key2 = rand();

    int request_shmid = shm_create(key1,
                                   QUEUE_SIZE,
                                   create_flag);
    int response_shmid = shm_create(key2,
                                    QUEUE_SIZE,
                                    create_flag);
    notnets_shm_info request_shm = {key1, request_shmid};
    notnets_shm_info response_shm = {key2, response_shmid};

    // set up shm regions as queues
    void* request_addr = shm_attach(request_shmid);
    void* response_addr = shm_attach(response_shmid);
    queue_create(request_addr, QUEUE_SIZE, message_size);
    queue_create(response_addr, QUEUE_SIZE, message_size);
    // don't want to stay attached to the queue pairs
    shm_detach(request_addr);
    shm_detach(response_addr);

    shms.request_shm = request_shm;
    shms.response_shm = response_shm;

    return shms;
}

void clean_up_queue_resources(coord_header *header, int slot){

    if (header->available_slots[slot].client_reserved){
         if (header->slots[slot].detach ){

         switch (header->slots[slot].type){
            case POLL:
                break;
            case BOOST:
                break;
            case SEM:
                destroy_semaphores(header,slot);
                break;
            default:
                break;
        }
        
    // if (header->slots[slot].shms.request_shm.queue_type == 2||
    //     header->slots[slot].shms.response_shm.queue_type == 2){
    // }
    }
    }
   
    
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
        switch (ch->slots[i].type){
            case POLL:
                service_slot(ch, i, &_hash_shm_allocator);
                break;
            case BOOST:
                service_slot(ch, i, &_boost_shm_allocator);
                break;
            case SEM:
                service_slot(ch, i, &_hash_sem_shm_allocator);
                break;
            default:
                service_slot(ch, i, &_hash_shm_allocator);
        }
        clean_up_queue_resources(ch,i);
        clear_slot(ch, i); // cleanups those that requested detach
    }
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

    server_context* sc = (server_context*) malloc(sizeof(server_context));

    // coord_header, at the end of the day, is just a shm region
    sc->coord_shmaddr = (void*) ch;

    if (pthread_mutex_init(&sc->manage_pool_mutex, NULL) == -1) {
        perror("error create manage pool mutex");
    }

    sc->manage_pool_state = RUNNING_NO_SHUTDOWN;
    atomic_thread_fence(memory_order_release);


    // start manage pool runner thread
    // Technically a thread leak as the reference for the 
    // thread is stored in shared memory.
    int err = pthread_create(&sc->manage_pool_thread,
                             NULL,
                             _manage_pool_runner,
                             sc);

    if (err != 0) {
        perror("pthread_create");
        shutdown(sc);
        return NULL;
    }


    // pthread_mutex_lock(&sc->manage_pool_mutex);
    // pthread_mutex_unlock(&sc->manage_pool_mutex);

    // srand(time(NULL));

    return sc;
}

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
queue_ctx* accept(server_context* handler) {
    // printf("SERVER: ACCEPT:\n");
    coord_header* ch = (coord_header*) handler->coord_shmaddr;

    int slot = ch->accept_slot;
    // bool attach;

    pthread_mutex_lock(&ch->mutex);
    for (int i = 0; i < SLOT_NUM; ++i, slot = (slot + 1) % SLOT_NUM) {
        // if this slot is scheduled to be detached, or is not reserved,
        // don't accept it

        // attach = ch->slots[slot].shm_attached;
        if (ch->slots[slot].detach ||
                !ch->available_slots[slot].client_reserved) {
            continue;
        } else if (ch->slots[slot].shm_created) {

            ch->accept_slot = (slot + 1) % SLOT_NUM;


            pthread_mutex_unlock(&ch->mutex);
            //Server must take the Client's requested queue type
            queue_ctx* q_ctx= select_queue_ctx(ch->slots[slot].type);
            q_ctx->queues = _create_server_queue_pair(ch, slot);

            return q_ctx;
        } 

        // if over here, then slot has been reserved, but has not been allocated
        // queues yet, so we move on to next slot
    }
    pthread_mutex_unlock(&ch->mutex);

    return NULL;
}

/**
 * @brief The client must be identified before written to.
 *
 * @param client[IN] queue pair
 * @param buf[IN] data buf to be written
 * @param size[IN] size of buf
 * @return ssize_t  -1 for error, 0 for successful push
 */
int server_send_rpc(queue_ctx* queues, const void *buf, size_t size) {
    return queues->server_send_rpc(queues->queues, buf,size);
}

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
size_t server_receive_buf(queue_ctx* queues, void* buf, size_t size) {
    return queues->server_receive_buf(queues->queues, buf, size);
}

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
    while (true) { //TODO figure out how to get rid of runnning thread
        pthread_mutex_lock(&handler->manage_pool_mutex);
        if (handler->manage_pool_state == NOT_RUNNING) {
            pthread_mutex_unlock(&handler->manage_pool_mutex);
            pthread_join(handler->manage_pool_thread, NULL);
            break;
        }
        pthread_mutex_unlock(&handler->manage_pool_mutex);
    }

    coord_shutdown(ch);
    manage_pool(handler);
    coord_delete(ch);
    free(handler);
}


int client_send_poll(queue_pair* conn, const void* buf, size_t size){
    return push(conn->request_shmaddr, buf, size);
}
size_t client_receive_poll(queue_pair* conn, void* buf, size_t size){
    return pop(conn->response_shmaddr, buf, &size);
}
int server_send_poll(queue_pair* client, const void *buf, size_t size){
    return push(client->response_shmaddr, buf, size);
}
size_t server_receive_poll(queue_pair* client, void* buf, size_t size){
    return pop(client->request_shmaddr, buf, &size);
}

int client_send_boost(queue_pair* conn, const void* buf, size_t size){
    return boost_push(conn->request_shmaddr, buf, size, conn->offset);
}
size_t client_receive_boost(queue_pair* conn, void* buf, size_t size){
    return boost_pop(conn->response_shmaddr, buf, &size, conn->offset);
}
int server_send_boost(queue_pair* client, const void *buf, size_t size){
    return boost_push(client->response_shmaddr, buf, size, client->offset);
}
size_t server_receive_boost(queue_pair* client, void* buf, size_t size){
    return boost_pop(client->request_shmaddr, buf, &size, client->offset);
}

int client_send_sem(queue_pair* conn, const void* buf, size_t size){
    return sem_push(conn->request_shmaddr, buf, size);
}
size_t client_receive_sem(queue_pair* conn, void* buf, size_t size){
    return sem_pop(conn->response_shmaddr, buf, &size);
}
int server_send_sem(queue_pair* client, const void *buf, size_t size){
    return sem_push(client->response_shmaddr, buf, size);
}
size_t server_receive_sem(queue_pair* client, void* buf, size_t size){
    return sem_pop(client->request_shmaddr, buf, &size);
}

void free_queue_ctx(queue_ctx * ctx){
    free(ctx->queues);
    free(ctx);
}

queue_ctx* select_queue_ctx(QUEUE_TYPE t)
{
  queue_ctx* my_ctx = (queue_ctx*)malloc(sizeof(struct queue_ctx));
  switch(t){
    case POLL:
        my_ctx->client_send_rpc = client_send_poll;
        my_ctx->client_receive_buf = client_receive_poll;
        my_ctx->server_send_rpc = server_send_poll;
        my_ctx->server_receive_buf = server_receive_poll;
        break;
    case BOOST:
        my_ctx->client_send_rpc = client_send_boost;
        my_ctx->client_receive_buf = client_receive_boost;
        my_ctx->server_send_rpc = server_send_boost;
        my_ctx->server_receive_buf = server_receive_boost;
        break;
    case SEM:
        my_ctx->client_send_rpc = client_send_sem;
        my_ctx->client_receive_buf = client_receive_sem;
        my_ctx->server_send_rpc = server_send_sem;
        my_ctx->server_receive_buf = server_receive_sem;
        break;
    default:
        my_ctx->client_send_rpc = client_send_poll;
        my_ctx->client_receive_buf = client_receive_poll;
        my_ctx->server_send_rpc = server_send_poll;
        my_ctx->server_receive_buf = server_receive_poll;
  }
  return my_ctx;
}
