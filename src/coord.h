#ifndef __COORD_H
#define __COORD_H

#define SLOT_NUM 10
#define QUEUE_SIZE 1024

#include "mem.h"
#include <unistd.h>
#include <pthread.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct reserve_pair {
    bool client_request;
    bool server_handled;
} reserve_pair;

typedef struct key_pair {
    key_t request_shm_key; //Only written to by Server // if -1 then that means it hasnt been created?
    key_t response_shm_key; //Only written to by Server  // if -1 then that means it hasnt been created?
} key_pair;

typedef struct shmid_pair {
    int request_shmid; //Only written to by Server // if -1 then that means it hasnt been created?
    int response_shmid; //Only written to by Server  // if -1 then that means it hasnt been created?
} shmid_pair;

//Modify to be templatable, permit
typedef struct coord_row {
    int client_id; // Only written to by client
    bool shm_created;  // Only written to by server
    key_pair keys; // TODO: is storing this necessary? don't we just need to store shmids?
    shmid_pair shmids;
    bool detach;  //Only written to by Client
}coord_row;


typedef struct coord_header {
    int shmid; // for later deletion, to avoid process state
	int counter; //How many are being used right now
    void *mutex;
	// int lock; //Use shared_mutex for now
	reserve_pair available_slots[SLOT_NUM] ;// This needs to be thread safe
    coord_row slots[SLOT_NUM];
} coord_header;


//djb2, dan bernstein
unsigned long hash(unsigned char *str){
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

//Client
//Checks creation, does shm stuff to get handle to it
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



//Client
// Returns reserved slot to check back against, if -1 failed to get a slot
int request_keys(coord_header* header, int client_id){

    // try to reserve a slot, if not available wait and try again
    pthread_mutex_lock((pthread_mutex_t*) header->mutex);
    for(int i = 0; i < SLOT_NUM; i++){
        if ((header->available_slots[i].client_request == false) &&
                (header->available_slots[i].server_handled == false)) {
            header->available_slots[i].client_request = true;
            header->slots[i].client_id = client_id;
            header->slots[i].detach = false;
            header->slots[i].keys.request_shm_key = -1;
            header->slots[i].keys.response_shm_key = -1;
            header->slots[i].shm_created = false;

            pthread_mutex_unlock((pthread_mutex_t*) header->mutex);
            return 0;
        }
    }
    pthread_mutex_unlock((pthread_mutex_t*) header->mutex);
    return -1;
}

//if null request
key_pair check_slot(coord_header* header, int slot){
    key_pair keys = {};
    keys.request_shm_key = header->slots[slot].keys.request_shm_key;
    keys.response_shm_key = header->slots[slot].keys.response_shm_key;
    return keys;
}


//Server
coord_header* coord_create(char* coord_address) {
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
        header->available_slots[i].server_handled = false;
    }

    //initialize mutual exclusion mechanism and store handle
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

    pthread_mutex_t mu_handle;

    //Instantiate the mutex
    if (pthread_mutex_init(&mu_handle, &attr) == -1) {
        perror("error creating mutex");
    }

    //Store in shm
    header->mutex = &mu_handle;

    // TODO: currently using random number gen for creating shm keys for
    // request and response queues. is there a better way?
    srand(time(NULL));

    return header;
}

int service_keys(coord_header* header, int slot) {
    pthread_mutex_lock((pthread_mutex_t*) header->mutex);
    if ((header->available_slots[slot].client_request == true) &&
            (header->available_slots[slot].server_handled == false)) {
        //Begin handling request, what does this look like?
        //Use passed function that returns keys

        //Call Allocation Function
        // TODO: currently using random number generator for request and
        // response queues. should have a better way.
        key_t key1 = rand();
        key_t key2 = rand();
        key_pair keys = {key1, key2};
        int shmid1 = shm_create(key1, QUEUE_SIZE, create_flag);
        int shmid2 = shm_create(key2, QUEUE_SIZE, create_flag);

        header->slots[slot].keys.request_shm_key = keys.request_shm_key;
        header->slots[slot].keys.response_shm_key = keys.response_shm_key;
        header->slots[slot].shmids.request_shmid = shmid1;
        header->slots[slot].shmids.response_shmid = shmid2;
        header->slots[slot].shm_created = true;
        header->available_slots[slot].server_handled = true;

        pthread_mutex_unlock((pthread_mutex_t*) header->mutex);
        return 0;
    }

    pthread_mutex_unlock((pthread_mutex_t*) header->mutex);
    return -1;
}

bool coord_is_empty(coord_header* header) {
    pthread_mutex_lock((pthread_mutex_t*) header->mutex);
    for (int i = 0; i < SLOT_NUM; i++){
        // client has this slot reserved
        if (header->available_slots[i].client_request == true) {
            pthread_mutex_unlock((pthread_mutex_t*) header->mutex);
            return false;
        }
    }
    pthread_mutex_unlock((pthread_mutex_t*) header->mutex);
    return true;
}


void coord_delete(coord_header* header) {
    int shmid = header->shmid;
    shm_remove(shmid);
}

void coord_shutdown(coord_header* header) {
    for (int i = 0; i < SLOT_NUM; ++i) {
        // this slot is reserved by a client
        if (header->slots[i].client_id != 0) {
            header->slots[i].detach = true;
        }
    }
}


#endif
