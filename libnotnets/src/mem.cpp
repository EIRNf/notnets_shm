#ifndef __MEM_H
#define __MEM_H

//Includes
#include "mem.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>        







int mmap_create(const char* path, int size, int flags){
    int fd;
    if ((fd = shm_open(path, flags,  S_IRWXU, 0666)) == -1){
        perror("shm_open: shm_open failed");
        fprintf(stderr,"Error creating shm: %d \n", fd );
    }
    return fd;
}

void* mmap_attach(int fd, int size, int flags){
    void* addr;
    if ((addr = mmap(NULL, size, flags, MAP_SHARED, fd, 0)) == MAP_FAILED){
        perror("mmap: mmap failed");
        fprintf(stderr,"Error attaching to shm: %p \n", addr );
    }
    return addr;
}

int mmap_detach(void* addr, int size){
    int err = 0;
    if ((err = munmap(addr, size)) == -1){
        perror("munmap: munmap failed");
        fprintf(stderr,"Error detaching shm: %d \n", err );
    }
    return err;
}

int mmap_remove(const char* path){
    int err = 0;
    if ((err = shm_unlink(path)) == -1){
        perror("shm_unlink: shm_unlink failed");
        fprintf(stderr,"Error removing shm: %d \n", err );
    }
    return err;
}
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
int shm_create(key_t key, int size, int shmflg){
    int shmid;
    if ((shmid = shmget(key, size, shmflg)) == -1){
        perror("shmget: shmget failed");
        fprintf(stderr,"Error creating shm: %d \n", shmid );
    }
    return shmid;
}

/**
 * @brief shmat wrapper. Handles errors.
 * Attaches local process to previously created
 * shm region.
 *
 * @param shmid shmid from create/shmget
 * @return void* shmaddr to access shm
 */
void* shm_attach(int shmid){
    void* shmaddr;
    if ((shmaddr = shmat(shmid, NULL, 0 )) == (void *)-1){
        perror("shmat: shmat failed");
        fprintf(stderr,"Error attaching to shm: %p \n", shmaddr );
    }
    return shmaddr;
}

/**
 * @brief shmdt wrapper. Handlers errors.
 * Detaches local process from shm region
 *
 * @param shmaddr shm address
 * @return int error codes
 */
int shm_detach(void* shmaddr){
    int err = 0;
    if ((err = shmdt(shmaddr)) ==  -1){
        perror("shmdt: shmdt failed");
        fprintf(stderr,"Error detaching shm: %d \n", err );
    }
    return err;
}
/**
 * @brief shmctl wrapper. Handles errors.
 * Removes shared region from IPC namespace
 *
 * @param shmid shmid from create/shmget
 * @return int error codes
 */
int shm_remove(int shmid){
    int err = 0;
    if ((err = shmctl(shmid, IPC_RMID, NULL)) ==  -1){
        perror("shmctl: shmctl failed");
        fprintf(stderr,"Error removing shm: %d \n", err );
    }
    return err;
}

#endif
