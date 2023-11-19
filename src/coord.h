#ifndef __COORD_H
#define __COORD_H

#define SLOT_NUM 10
#define QUEUE_SIZE 1024
#define MESSAGE_SIZE 4

#include "mem.h"
#include "queue.h"

#include <unistd.h>
#include <pthread.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct reserve_pair {
    bool client_request;
} reserve_pair;

typedef struct key_pair {
    int request_shm_key;
    int response_shm_key;
} key_pair;

typedef struct shm_info {
    int key;
    int shmid;
    void* addr;
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
}coord_row;


typedef struct coord_header {
    int shmid; // for later deletion, to avoid process state
	int counter; // how many are being used right now
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
coord_header* attach(char* coord_address){
    //attach shm region
    int key = (int) hash((unsigned char*)coord_address);
    int size = sizeof(coord_header);
    int shmid = shm_create(key, size,create_flag);
    void* shmaddr = shm_attach(shmid);

    //get header
    coord_header* header = (coord_header*) shmaddr;

    return header;
}

void detach(coord_header* header){
    shm_detach(header);
}

// Client
// Returns reserved slot to check back against, if -1 failed to get a slot
int request_slot(coord_header* header, int client_id){
    // try to reserve a slot, if not available wait and try again
    pthread_mutex_lock(&header->mutex);
    for(int i = 0; i < SLOT_NUM; i++){
        if (header->available_slots[i].client_request == false){
            header->available_slots[i].client_request = true;

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
        shms.request_shm = header->slots[slot].shms.request_shm;
        shms.response_shm = header->slots[slot].shms.response_shm;
    }
    pthread_mutex_unlock(&header->mutex);
    return shms;
}

//Server
coord_header* coord_create(char* coord_address){
    //Create shm region
    int key = (int) hash((unsigned char*)coord_address);
    int size = sizeof(coord_header);
    int shmid = shm_create(key, size, create_flag);
    void* shmaddr = shm_attach(shmid);

    //initialize values
    coord_header* header = (coord_header*) shmaddr;

    header->shmid = shmid;

    header->counter = 0;
    for(int i = 0; i < SLOT_NUM; i++){
        header->available_slots[i].client_request = false;
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

    // store in shm
    return header;
}

int service_slot(coord_header* header, int slot, key_pair (*allocation)()){

    pthread_mutex_lock(&header->mutex);
    int client_id = header->slots[slot].client_id;
    // check for the presence of a valid request by reading cliend_id value ie. non zero
    if (client_id > 0 ){
        // call allocation Function
        key_pair keys = (*allocation)();

        // allocate shm
        int request_shmid = shm_create(keys.request_shm_key,
                                       QUEUE_SIZE,
                                       create_flag);
        void* request_shmaddr = shm_attach(request_shmid);
        int response_shmid = shm_create(keys.response_shm_key,
                                        QUEUE_SIZE,
                                        create_flag);
        void* response_shmaddr = shm_attach(response_shmid);
        shm_info request_shm = {keys.request_shm_key,
                                request_shmid,
                                request_shmaddr};
        shm_info response_shm = {keys.response_shm_key,
                                 response_shmid,
                                 response_shmaddr};
        // set up shm regions as queues
        queue_create(request_shmaddr, QUEUE_SIZE, MESSAGE_SIZE);
        queue_create(response_shmaddr, QUEUE_SIZE, MESSAGE_SIZE);

        header->slots[slot].shms.request_shm = request_shm;
        header->slots[slot].shms.response_shm = response_shm;
        header->slots[slot].shm_created = true;

        pthread_mutex_unlock(&header->mutex);
        return client_id;
    }

    pthread_mutex_unlock(&header->mutex);
    return -1;
}

void coord_delete(coord_header* header){
    int shmid = header->shmid;
    shm_detach(header);
    shm_remove(shmid);
}

// SHOULD ONLY BE USED FOR TESTING PURPOSES, UNSAFE
void force_clear_slot(coord_header* header, int slot){
    shm_info request_shm = header->slots[slot].shms.request_shm;
    shm_info response_shm = header->slots[slot].shms.response_shm;

    shm_detach(request_shm.addr);
    shm_detach(response_shm.addr);
    shm_remove(request_shm.shmid);
    shm_remove(response_shm.shmid);

    memset(&header->slots[slot], 0, sizeof(coord_row));
    header->available_slots[slot].client_request = false;
}

void clear_slot(coord_header* header, int slot){
    pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].detach == true) {
        force_clear_slot(header, slot);
    }
    pthread_mutex_unlock(&header->mutex);
}
#endif
