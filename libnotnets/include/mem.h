#ifndef __MEM_H
#define __MEM_H

//Includes
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>        

//Definitions
static const int create_flag = IPC_CREAT | 0644 ;
static const int attach_flag = 0644;
static const int only_read_flag = SHM_RDONLY;


static const int mmap_create_flag = O_RDWR | O_CREAT  ;
static const int mmap_attach_flag =  O_RDWR ;
static const int mmap_only_read_flag = O_RDONLY;

static const int mmap_flags_rw = PROT_READ | PROT_WRITE;
static const int mmap_flags_rw_no_cache = PROT_READ | PROT_WRITE | MAP_NOCACHE;



int mmap_create(const char* path, int size, int flags);
void* mmap_attach(int fd, int size, int flags);
int mmap_detach(void* addr, int size);
int mmap_remove(const char* path);

/**
 * @brief shmget wrapper. Handles errors.
 * Creates shared memory region that can
 * be attached to
 *
 * @param key unique key to creat shm region
 * @param size size of shm region
 * @param shmflg creation flags
 * @return int shmid to attach
 */
int shm_create(key_t key, int size, int shmflg);

/**
 * @brief shmat wrapper. Handles errors.
 * Attaches local process to previously created
 * shm region.
 *
 * @param shmid shmid from create/shmget
 * @return void* shmaddr to access shm
 */
void* shm_attach(int shmid);

/**
 * @brief shmdt wrapper. Handlers errors.
 * Detaches local process from shm region
 *
 * @param shmaddr shm address
 * @return int error codes
 */
int shm_detach(void* shmaddr);

/**
 * @brief shmctl wrapper. Handles errors.
 * Removes shared region from IPC namespace
 *
 * @param shmid shmid from create/shmget
 * @return int error codes
 */
int shm_remove(int shmid);

#endif
