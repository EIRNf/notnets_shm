#ifndef __COORD_H
#define __COORD_H

#define SLOT_NUM 40
#define QUEUE_SIZE 16024

#include <unistd.h>
#include <pthread.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdatomic.h>


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
    atomic_bool shm_created;  //Only written to by Server
    atomic_bool client_attached;
    atomic_bool server_attached;
    // atomic_int shm_attached;
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

void print_coord_row(const coord_row *row);

void print_coord_header(const coord_header *header);

// djb2, dan bernstein
int hash(unsigned char *str);

// client
// checks creation, does shm stuff to get handle to it
coord_header* coord_attach(char* coord_address);

void coord_detach(coord_header* header);

// Client
// Returns reserved slot to check back against, if -1 failed to get a slot
int request_slot(coord_header* header, int client_id, int message_size);

shm_pair check_slot(coord_header* header, int slot);

int get_client_id(coord_header* header, int slot);

bool get_client_attach_state(coord_header* header, int slot);

bool get_server_attach_state(coord_header* header, int slot);

bool set_client_attach_state(coord_header* header, int slot, bool state);

bool set_server_attach_state(coord_header* header, int slot, bool state);

// Server
coord_header* coord_create(char* coord_address);

int service_slot(coord_header* header,
                 int slot,
                 shm_pair (*allocation)(int,int));

void coord_delete(coord_header* header);

// SHOULD ONLY BE USED FOR TESTING PURPOSES, UNSAFE
void force_clear_slot(coord_header* header, int slot);

void clear_slot(coord_header* header, int slot);

void coord_shutdown(coord_header* header);

#endif
