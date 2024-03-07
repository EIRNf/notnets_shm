#ifndef __MEM_H
#define __MEM_H

//Includes
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>


//Definitions
static const int create_flag = IPC_CREAT | 0644 ;
static const int attach_flag = 0644;
static const int only_read_flag = SHM_RDONLY;

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
