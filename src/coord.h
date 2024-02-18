#ifndef __COORD_H
#define __COORD_H

#define SLOT_NUM 100
#define QUEUE_SIZE 16024

#include "mem.h"
#include "queue.h"

#include <unistd.h>
#include <pthread.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct reserve_pair {
    bool client_reserved;
} reserve_pair;

typedef struct notnets_shm_info {
    int key;
    int shmid;
} notnets_shm_info;

typedef struct shm_pair {
    notnets_shm_info request_shm;
    notnets_shm_info response_shm;
} shm_pair;

//Modify to be templatable, permit
typedef struct coord_row {
    int client_id; //Only written to by Client, how do we even know id?
    int message_size;
    bool shm_created;  //Only written to by Server
    atomic_int shm_attached;
    shm_pair shms;
    bool detach;  //Only written to by Client
} coord_row;


typedef struct coord_header {
    int shmid; // for later deletion, to avoid process state
    int accept_slot;
    pthread_mutex_t mutex; // lock when accessing slots or available_slots
	reserve_pair available_slots[SLOT_NUM];
    coord_row slots[SLOT_NUM];
} coord_header;

void print_coord_row(const coord_row *row) {
    printf("  client_id: %d\n", row->client_id);
    if (row->client_id == 0){
        return;
    }
    printf("  message_size: %d\n", row->message_size);
    printf("  shm_created: %s\n", row->shm_created ? "true" : "false");
    printf("  shm_attached: %d\n", atomic_load(&row->shm_attached));
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

// djb2, dan bernstein
unsigned long hash(unsigned char *str){
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
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
            header->slots[i].shm_attached = 0;
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
    pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].shm_created == true) {
        shms = header->slots[slot].shms;
        atomic_thread_fence(memory_order_seq_cst);
    }
    pthread_mutex_unlock(&header->mutex);
    return shms;
}

int get_client_id(coord_header* header, int slot){
    int i = 0;
    pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].shm_created == true) {
        i = header->slots[slot].client_id;
    }
    pthread_mutex_unlock(&header->mutex);
    return i;
}

int get_attach_state(coord_header* header, int slot){
    int attach = 0;
    pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].shm_created == true) {
        attach = header->slots[slot].shm_attached;
    }
    pthread_mutex_unlock(&header->mutex);
    return attach;
}

//If return 0 the slot hasn't had shm created yet
int increment_slot_attach_count(coord_header* header, int slot) {
    int attach = 0;
    pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].shm_created == true) {
        header->slots[slot].shm_attached += 1;
        attach = header->slots[slot].shm_attached;
    }
    pthread_mutex_unlock(&header->mutex);
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
                    
    pthread_mutex_lock(&header->mutex);
    int client_id = header->slots[slot].client_id;

    if (header->available_slots[slot].client_reserved &&
            !header->slots[slot].shm_created) {
        // printf("SERVER: Pre-Servicing:\n");
        // print_coord_header(header);


        // call allocation Function
        shm_pair shms = (*allocation)(header->slots[slot].message_size,header->slots[slot].client_id);

        header->slots[slot].shms = shms;
        header->slots[slot].shm_created = true;
        atomic_thread_fence(memory_order_seq_cst);

        pthread_mutex_unlock(&header->mutex);

        // printf("SERVER: Post-Servicing:\n");
        // print_coord_header(header);
        return client_id;
    }

    pthread_mutex_unlock(&header->mutex);
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
    pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].detach) {
        force_clear_slot(header, slot);
    }
    pthread_mutex_unlock(&header->mutex);
}

void coord_shutdown(coord_header* header) {
    pthread_mutex_lock(&header->mutex);
    for (int i = 0; i < SLOT_NUM; ++i) {
            header->slots[i].detach = true;
    }
    pthread_mutex_unlock(&header->mutex);
}
#endif
