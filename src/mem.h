#ifndef __MEM_H
#define __MEM_H


//Includes
#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

//Definitions
const int create_flag = IPC_CREAT;
const int only_read_flag = SHM_RDONLY;

class shared_memory_region {
    public:
        shared_memory_region(key_t key, int size, int shmflg){
            this->key = key;
            this->size = size;
            this->shm_get_flg = shmflg;
        };
        int create();
        void* attach(int shmid);
        void detach(void* shmaddr);
        void remove();

    private:
        key_t key;
        int shmid;
        void* shmaddr;
        int size;
        int shm_get_flg; //IPC_CREAT, IPC_EXCL, SHM_HUGETLB, ...
    
    };

int shared_memory_region::create(){
    int shmid;
    if ((shmid = shmget(this->key, this->size, this->shm_get_flg)) == -1){
        perror("shmget: shmget failed");
        exit(1);
    } else {
        this->shmid;
        return shmid;
    }
}

void* shared_memory_region::attach(int shmid){
    if(this->shmid != shmid){
        perror("shm attach: attach to different id from  object");
        exit(1);
    }
    void* shmaddr;
    if ((shmaddr = shmat(shmid, NULL,NULL)) == (void *)-1){
        perror("shmat: shmat failed");
        exit(1);
    } else {
        this->shmaddr;
        return shmaddr;
    }
}

void shared_memory_region::detach(void* shmaddr){
    int err;
    if ((err = shmdt(shmaddr)) ==  -1){
        perror("shmdt: shmdt failed");
        exit(1);
    }
}

void shared_memory_region::remove(){
    if (this->shmid == NULL){
        perror("remove: nothing to remove");
        exit(1);
    }
    int err;
    if ((err = shmctl(this->shmid, IPC_RMID, NULL)) ==  -1){
        perror("shmctl: shmctl failed");
        exit(1);
    }

}

void HashShmKeys(char* fullServiceName);

#endif
