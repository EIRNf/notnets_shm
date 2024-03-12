
#define SLOT_NUM 40
#define QUEUE_SIZE 16024

#include "coord.h"
#include "queue.h"
#include "mem.h"

#include <unistd.h>
#include <pthread.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <stdlib.h>

void print_coord_row(const coord_row *row) {
    printf("  client_id: %d\n", row->client_id);
    if (row->client_id == 0){
        return;
    }
    printf("  message_size: %d\n", row->message_size);
    printf("  shm_created: %s\n", row->shm_created ? "true" : "false");
    // printf("  client_attached: %d\n", atomic_load(&row->client_attached));  //TODO compile error
    // printf("  server_attached: %d\n", atomic_load(&row->server_attached));
    printf("  shms.request_shm.key: %d\n", row->shms.request_shm.key);
    printf("  shms.request_shm.shmid: %d\n", row->shms.request_shm.shmid);
    printf("  shms.response_shm.key: %d\n", row->shms.response_shm.key);
    printf("  shms.response_shm.shmid: %d\n", row->shms.response_shm.shmid);
    printf("  detach: %s\n", row->detach ? "true" : "false");
}

void print_coord_header(const coord_header *header) {
    printf("shmid: %d\n", header->shmid);
    printf("accept_slot: %d\n", header->accept_slot);
    printf("mutex: <pthread_mutex_t>\n");
    
    printf("available_slots:\n");
    for (int i = 0; i < SLOT_NUM; ++i) {
        printf("  slot[%d].client_reserved: %s\n", i, header->available_slots[i].client_reserved ? "true" : "false");
    }

    printf("slots:\n");
    for (int i = 0; i < SLOT_NUM; ++i) {
        printf("  slot[%d]:\n", i);
        print_coord_row(&header->slots[i]);
    }
}

//TODO: REALLY FIX HASHING. 
//Current approach to hashing is a mess due to many reasons:
// - not using a proper hash function
// - casting from unsinged long to int
//      - overflowing/underflowing happens all over the place
// need: hash function that can take a string of arbitrary length
// and hash it to a positive value the size of int
// djb2, dan bernstein
// unsigned long hash(unsigned char *str){
//     unsigned long hash = 5381;
//     int c;

//     while ((c = *str++))
//         hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

//     return hash;
// }

static inline unsigned rol(unsigned r, int k) {return (r << k) | (r >> (32 - k));}

int hash(unsigned char *input) { 
    int result = 0x55555555;

    while (*input) { 
        result ^= *input++;
        result = rol(result, 5);
    }

    return result;
}

// client
// checks creation, does shm stuff to get handle to it
coord_header* coord_attach(char* coord_address){
    // attach shm region
    int key = (int) hash((unsigned char*)coord_address);
    int size = sizeof(coord_header);
    int shmid = shm_create(key, size, attach_flag);

    if (shmid == -1) {
        return NULL;
    }

    void* shmaddr = shm_attach(shmid);

    if (shmaddr == NULL) {
        return NULL;
    }

    // get header
    coord_header* header = (coord_header*) shmaddr;

    return header;
}

void coord_detach(coord_header* header){
    shm_detach(header);
}

// Client
// Returns reserved slot to check back against, if -1 failed to get a slot
int request_slot(coord_header* header, int client_id, int message_size){

    // try to reserve a slot, if not available wait and try again
    pthread_mutex_lock(&header->mutex);
    for(int i = 0; i < SLOT_NUM; i++){
        if (header->available_slots[i].client_reserved == false){
            // printf("CLIENT: Pre-Requesting:\n");
            // print_coord_header(header);
            header->slots[i].client_id = client_id;
            header->slots[i].message_size = message_size;
            header->slots[i].detach = false;
            atomic_store(&header->slots[i].client_attached,false);
            atomic_store(&header->slots[i].server_attached,false);
            header->slots[i].shm_created = false;
            atomic_thread_fence(memory_order_seq_cst);
            header->available_slots[i].client_reserved = true;

            // printf("CLIENT: Post-Requesting:\n");
            // print_coord_header(header);
            pthread_mutex_unlock(&header->mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&header->mutex);
    return -1;
}

shm_pair check_slot(coord_header* header, int slot){
    shm_pair shms = {};
    // pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].shm_created == true) {
        shms = header->slots[slot].shms;
        atomic_thread_fence(memory_order_seq_cst);
    }
    // pthread_mutex_unlock(&header->mutex);
    return shms;
}

int get_client_id(coord_header* header, int slot){
    int i = 0;
    // pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].shm_created == true) {
        i = header->slots[slot].client_id;
    }
    // pthread_mutex_unlock(&header->mutex);
    return i;
}

bool get_client_attach_state(coord_header* header, int slot){
    bool attach = false;
    // pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].shm_created == true) {
        attach =  atomic_load(&header->slots[slot].client_attached);
    }
    // pthread_mutex_unlock(&header->mutex);
    return attach;
}

bool get_server_attach_state(coord_header* header, int slot){
    bool attach = false;
    // pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].shm_created == true) {
        attach =  atomic_load(&header->slots[slot].server_attached);
    }
    // pthread_mutex_unlock(&header->mutex);
    return attach;
}

bool set_client_attach_state(coord_header* header, int slot, bool state){
    bool attach = false;
    // pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].shm_created == true) {
        atomic_store(&header->slots[slot].client_attached,state);
        attach = atomic_load(&header->slots[slot].client_attached);
    }
    // pthread_mutex_unlock(&header->mutex);
    return attach;
}

bool set_server_attach_state(coord_header* header, int slot, bool state){
    bool attach = false;
    // pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].shm_created == true) {
        atomic_store(&header->slots[slot].server_attached,state);
        attach = atomic_load(&header->slots[slot].server_attached);
    }
    // pthread_mutex_unlock(&header->mutex);
    return attach;
}

// Server
coord_header* coord_create(char* coord_address){
    //Create shm region
    int key = (int) hash((unsigned char*) coord_address);
    int size = sizeof(coord_header);
    int shmid = shm_create(key, size, create_flag);
    void* shmaddr = shm_attach(shmid);

    // initialize values
    coord_header* header = (coord_header*) shmaddr;

    header->shmid = shmid;
    header->accept_slot = 0;

    for(int i = 0; i < SLOT_NUM; i++){
        header->available_slots[i].client_reserved = false;
    }

    // initialize mutual exclusion mechanism and store handle
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

    // instantiate the mutex
    if (pthread_mutex_init(&header->mutex, &attr) == -1) {
        perror("error creating mutex");
    }

    return header;
}

int service_slot(coord_header* header,
                 int slot,
                 shm_pair (*allocation)(int,int)){
                    
    // pthread_mutex_lock(&header->mutex);
    int client_id = header->slots[slot].client_id;

    if (header->available_slots[slot].client_reserved &&
            !header->slots[slot].shm_created) {
        // printf("SERVER: Pre-Servicing:\n");
        // print_coord_header(header);


        // call allocation Function
        shm_pair shms = (*allocation)(header->slots[slot].message_size,header->slots[slot].client_id);

        header->slots[slot].shms = shms;
        atomic_thread_fence(memory_order_seq_cst);

        atomic_store(&header->slots[slot].shm_created, true);
        // pthread_mutex_unlock(&header->mutex);

        // printf("SERVER: Post-Servicing:\n");
        // print_coord_header(header);
        return client_id;
    }

    // pthread_mutex_unlock(&header->mutex);
    return -1;
}

void coord_delete(coord_header* header){
    int shmid = header->shmid;
    coord_detach(header);
    shm_remove(shmid);
}

// SHOULD ONLY BE USED FOR TESTING PURPOSES, UNSAFE
void force_clear_slot(coord_header* header, int slot){
    // don't need to remove shm if server has not handled this slot yet (aka
    // created shm regions)
    if (header->slots[slot].shm_created) {
        notnets_shm_info request_shm = header->slots[slot].shms.request_shm;
        notnets_shm_info response_shm = header->slots[slot].shms.response_shm;

        shm_remove(request_shm.shmid);
        shm_remove(response_shm.shmid);
    }

    memset(&header->slots[slot], 0, sizeof(coord_row));
    header->available_slots[slot].client_reserved = false;
}

void clear_slot(coord_header* header, int slot){
    // pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].detach) {
        force_clear_slot(header, slot);
    }
    // pthread_mutex_unlock(&header->mutex);
}

void coord_shutdown(coord_header* header) {
    // pthread_mutex_lock(&header->mutex);
    for (int i = 0; i < SLOT_NUM; ++i) {
            header->slots[i].detach = true;
    }
    // pthread_mutex_unlock(&header->mutex);
}

