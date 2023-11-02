#ifndef __COORD_H
#define __COORD_H


#include "mem.h"
#include "unistd.h"
#include <functional>
#include <thread>
#include <pthread.h>
#include <sys/sem.h>

const int available_reserve_slots = 10;
const int available_coord_slots = 10;


typedef struct reserve_pair {
    bool client_request;
    bool server_handled;
}reserve_pair; 

typedef struct key_pair {
    int request_shm_key; //Only written to by Server // if -1 then that means it hasnt been created?
    int response_shm_key; //Only written to by Server  // if -1 then that means it hasnt been created?
} key_pair;

//Modify to be templatable, permit 
typedef struct coord_row {
    int client_id; //Only written to by Client, how do we even know id?
    bool shm_created;  //Only written to by Server //If false server creates it and adds keys to
    key_pair keys;
    bool detach;  //Only written to by Client
}coord_row;


typedef struct coord_header {
	int counter;
    void *mutex;
	// int lock; //Use shared_mutex for now 
	reserve_pair available_slots[available_reserve_slots] ;// This needs to be thread safe
	coord_row coord_slots[available_coord_slots] ;
} coord_header;

class coord {

    public:
        coord();
        void create(shared_memory_region *shm); // Should only be called by Server
        void attach(shared_memory_region *shm); // Should only be called by Clients
        void destroy();

        void service_requests(std::function<key_pair(int)> allocation, int key_seed); //Continue servicing requests, pass function as desired work to be done. 
        key_pair* query_handled_requests(); //
        key_pair try_request_keys(int client_id); //Wait until request is satisfied or timeout

    private:
        shared_memory_region *shm;
        // std::binary_semaphore sem{0}; //use binary semaphore for now, might be replaced by another
        // key_t key_mutex;
        // int mutex_id;        
        pthread_mutex_t mu_handle;

        key_pair queue_keys[available_reserve_slots];

        coord_header* get_coord_header(void* shmaddr);

        //We want to support varying threading models, how do we surface new allocated(or reused) queues to a user of coord. 
        

};

coord::coord() {}

coord_header* coord::get_coord_header(void* shmaddr){
    coord_header* header = (coord_header*)(shmaddr);
    return header;
}

void coord::create(shared_memory_region *shm){
    //do check to verify memory region size
    if(sizeof(coord_header) > (u_long) shm->size){
        perror("Header data too large for allocated shm");
        exit(1);
    }
    this->shm = shm;

    coord_header *header_ptr = get_coord_header(this->shm->shmaddr);

    header_ptr->counter = 0;
    for(int i = 0; i < available_coord_slots; i++){
        header_ptr->available_slots[i].client_request = false;
        header_ptr->available_slots[i].server_handled = false;
    }
    // header.coord_slots[available_coord_slots] = {0};
    // this->queue_keys[available_coord_slots] = {0};

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

    //Instantiate the mutex
    if (pthread_mutex_init(&mu_handle, &attr) == -1) {
        perror("error creating mutex");
    }
    //Store in shm
    header_ptr->mutex = &mu_handle;
}
void coord::attach(shared_memory_region *shm){
    
    //do check to verify memory region size
    if(sizeof(coord_header)> (u_long) shm->size){
        perror("Header data too large for allocated shm");
        exit(1);
    }

    this->shm = shm;

    coord_header *header_ptr = get_coord_header(this->shm->shmaddr);

    //Get the mutex in shared memory to local handle
    this->mu_handle = *((pthread_mutex_t *) header_ptr->mutex);

}
void coord::destroy(){
    pthread_mutex_destroy(&mu_handle);
}

//Client Section, request a pair of keys for shm
key_pair coord::try_request_keys(int client_id){
    coord_header *header = this->get_coord_header(shm->shmaddr);
    int reserved_slot = -1;

    ///try to reserve a slot, if not available wait and try again

    pthread_mutex_lock(&mu_handle);
    for(int i = 0; i <available_coord_slots; i++){
        if ((header->available_slots[i].client_request == false) && (header->available_slots[i].server_handled == false) ){
            header->available_slots[i].client_request = true;
            header->coord_slots[i].client_id = client_id;
            header->coord_slots[i].detach = false;
            header->coord_slots[i].keys.request_shm_key = -1;
            header->coord_slots[i].keys.response_shm_key = -1;
            header->coord_slots[i].shm_created = false;

            reserved_slot = i;
            break;
        } 
    }
    pthread_mutex_unlock(&mu_handle);
    //Reserve a slot, now poll it until it is handled
    //TODO:implement timeout
    while(true){
        if (header->coord_slots[reserved_slot].shm_created == true){
            key_pair keys = {};
            keys.request_shm_key =  header->coord_slots[reserved_slot].keys.request_shm_key;
            keys.response_shm_key = header->coord_slots[reserved_slot].keys.response_shm_key;
            return keys;
        }
        //tune with wait
    }
}

//Server section, handle requests in critical section
//Single writer principle, run this with a dedicated thread. 
void coord::service_requests(std::function<key_pair(int)> allocation, int key_seed){
    coord_header *header = this->get_coord_header(shm->shmaddr);

        ///try to reserve a slot, if not available wait and try again
        // this->sem.acquire();
        for(int i = 0; i <available_coord_slots; i++){
            if ((header->available_slots[i].client_request == true) && (header->available_slots[i].server_handled == false) ){
                //Begin handling request, what does this look like?
                //Use passed function that returns keys

                //Call Allocation Function
                key_pair keys = allocation(key_seed);
                
                header->coord_slots[i].keys.request_shm_key = keys.request_shm_key;
                header->coord_slots[i].keys.response_shm_key = keys.response_shm_key;
                header->coord_slots[i].shm_created = true;

                this->queue_keys[i] = keys;
                header->available_slots[i].server_handled = true;
            }
        }
        // this->sem.release();
}

key_pair* coord::query_handled_requests(){
    coord_header *header = this->get_coord_header(shm->shmaddr);
    for(int i = 0; i <available_coord_slots; i++){
        key_pair keys = header->coord_slots[i].keys;
        this->queue_keys[i] = keys; 
    }
    return queue_keys;
}


#endif