#ifndef __COORD_H
#define __COORD_H

#define SLOT_NUM 10

#include "mem.h"
#include "unistd.h"
#include <pthread.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct reserve_pair {
    bool client_request;
}reserve_pair;

typedef struct key_pair {
    int request_shm_key; //Only written to by Server // if -1 then that means it hasnt been created?
    int response_shm_key; //Only written to by Server  // if -1 then that means it hasnt been created?
} key_pair;

//Modify to be templatable, permit
typedef struct coord_row {
    int client_id; //Only written to by Client, how do we even know id?
    bool shm_created;  //Only written to by Server
    key_pair keys;
    bool detach;  //Only written to by Client
}coord_row;


typedef struct coord_header {
    int shmid; // for later deletion, to avoid process state
	int counter; //How many are being used right now
    pthread_mutex_t mutex;
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
    // ///try to reserve a slot, if not available wait and try again
    pthread_mutex_lock(&header->mutex);
    for(int i = 0; i < SLOT_NUM; i++){
        if (header->available_slots[i].client_request == false){
            header->available_slots[i].client_request = true;

            header->slots[i].client_id = client_id;
            header->slots[i].detach = false;
            header->slots[i].keys.request_shm_key = -1;
            header->slots[i].keys.response_shm_key = -1;
            header->slots[i].shm_created = false;

            pthread_mutex_unlock(&header->mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&header->mutex);
    return -1;
}

//if null request
key_pair check_slot(coord_header* header, int slot){
    key_pair keys = {};
    pthread_mutex_lock(&header->mutex);
    if (header->slots[slot].shm_created == true ){
        keys.request_shm_key = header->slots[slot].keys.request_shm_key;
        keys.response_shm_key = header->slots[slot].keys.response_shm_key;
    }
    pthread_mutex_unlock(&header->mutex);
    return keys;
}

//Server
coord_header* create(char* coord_address){
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

    // header->slots[SLOT_NUM] = {0};

    //initialize mutual exclusion mechanism and store handle

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

    // pthread_mutex_t* mu_handle = malloc(sizeof(pthread_mutex_t));

    //Instantiate the mutex
    if (pthread_mutex_init(&header->mutex, &attr) == -1) {
        perror("error creating mutex");
    }

    //Store in shm
    return header;
}

//return the client id
int service_keys(coord_header* header, int slot, key_pair (*allocation)()){

    pthread_mutex_lock(&header->mutex);
    int client_id = header->slots[slot].client_id;
    //Check for the presence of a valid request by reading cliend_id value ie. non zero
    if (client_id > 0 ){
        //Call Allocation Function
//        key_pair old_keys = (*allocation)();
//        key_pair keys = old_keys;
        key_pair keys = (*allocation)();

        header->slots[slot].keys.request_shm_key = keys.request_shm_key;
        header->slots[slot].keys.response_shm_key = keys.response_shm_key;
        header->slots[slot].shm_created = true;

       pthread_mutex_unlock(&header->mutex);
        return client_id;
    }

   pthread_mutex_unlock(&header->mutex);
    return -1;
}

void delete(coord_header* header){
    int shmid = header->shmid;
    shm_remove(shmid);
}

// void clear_row(coord_header* header, int row){

// }


#endif
