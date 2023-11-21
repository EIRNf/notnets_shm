#ifndef __COORD_H
#define __COORD_H

#define SLOT_NUM 10
#define QUEUE_SIZE 1024

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

typedef struct shm_info {
    int key;
    int shmid;
} shm_info;

typedef struct shm_pair {
    shm_info request_shm;
    shm_info response_shm;
} shm_pair;

//Modify to be templatable, permit
typedef struct coord_row {
    int client_id; //Only written to by Client, how do we even know id?
    bool shm_created;  //Only written to by Server
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
    int shmid = shm_create(key, size, create_flag);
    void* shmaddr = shm_attach(shmid);

    // get header
    coord_header* header = (coord_header*) shmaddr;

    return header;
}

void coord_detach(coord_header* header){
    shm_detach(header);
}

// Client
// Returns reserved slot to check back against, if -1 failed to get a slot
int request_slot(coord_header* header, int client_id){
    // try to reserve a slot, if not available wait and try again
    pthread_mutex_lock(&header->mutex);
    for(int i = 0; i < SLOT_NUM; i++){
        if (header->available_slots[i].client_reserved == false){
            header->available_slots[i].client_reserved = true;
            header->slots[i].client_id = client_id;
            header->slots[i].detach = false;
            header->slots[i].shm_created = false;

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
    }
    pthread_mutex_unlock(&header->mutex);
    return shms;
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

int service_slot(coord_header* header, int slot, shm_pair (*allocation)()){
    pthread_mutex_lock(&header->mutex);
    int client_id = header->slots[slot].client_id;

    if (header->available_slots[slot].client_reserved &&
            !header->slots[slot].shm_created) {
        // call allocation Function
        shm_pair shms = (*allocation)();

        header->slots[slot].shms = shms;
        header->slots[slot].shm_created = true;

        pthread_mutex_unlock(&header->mutex);
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
        shm_info request_shm = header->slots[slot].shms.request_shm;
        shm_info response_shm = header->slots[slot].shms.response_shm;

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
