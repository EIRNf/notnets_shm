#include "sem_queue.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/types.h>


#include "mem.h"
#include "rpc.h"


#define MAX_RETRIES 10


void destroy_semaphores(coord_header *header, int slot){
    //Get row slots
    notnets_shm_info request_slot = header->slots[slot].shms.request_shm;
    notnets_shm_info response_slot = header->slots[slot].shms.response_shm;

    //Attach to queues
    sem_spsc_queue_header* hReq = (sem_spsc_queue_header*) shm_attach(request_slot.shmid);
    sem_spsc_queue_header* hResp = (sem_spsc_queue_header*) shm_attach(response_slot.shmid);

    //Get semaphores IDs.
    semctl(hReq->sem_full, 0, IPC_RMID);
    semctl(hReq->sem_empty, 0, IPC_RMID);
    semctl(hReq->sem_lock, 0, IPC_RMID);


    semctl(hResp->sem_full, 0, IPC_RMID);
    semctl(hResp->sem_empty, 0, IPC_RMID);
    semctl(hResp->sem_lock, 0, IPC_RMID);

    //Call Delete on Semaphores.

    shm_detach(hReq);
    shm_detach(hResp);
}


shm_pair _hash_sem_shm_allocator(int message_size, int client_id) {
    // printf("ALLOCATOR:\n");
    shm_pair shms = {};
    
    // int key1 = hash((unsigned char*)&client_id);
    // int temp = client_id + 1;
    // int key2 = hash((unsigned char*)&temp);

    int key1 = client_id/2;
    int key2 = client_id/3;

    int request_shmid = shm_create(key1,
                                   QUEUE_SIZE,
                                   create_flag);
    int response_shmid = shm_create(key2,
                                    QUEUE_SIZE,
                                    create_flag);
    notnets_shm_info request_shm = {key1, request_shmid};
    notnets_shm_info response_shm = {key2, response_shmid};

    // set up shm regions as queues
    void* request_addr = shm_attach(request_shmid);
    void* response_addr = shm_attach(response_shmid);

    queue_create(request_addr, key1, QUEUE_SIZE, message_size);
    queue_create(response_addr, key2, QUEUE_SIZE, message_size);

    // don't want to stay attached to the queue pairs
    shm_detach(request_addr);
    shm_detach(response_addr);

    shms.request_shm = request_shm;
    shms.response_shm = response_shm;

    return shms;
}


int sem_wait(int semid, int sem_num){
    struct sembuf sb;

    sb.sem_num = sem_num;
    sb.sem_op = -1; //Block the calling process until the value of the semaphore is greater than or equal to the absolute value of sem_op.
    sb.sem_flg = 0;

    int ret = semop(semid, &sb, 1);
    if (ret == -1 ){
        int errnum = errno;
        fprintf(stderr, "Sem_Wait: Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error opening file: %s\n", strerror( errnum ));

    }
     
    //Carry our semaphore operation
    return ret;
}

int sem_post(int semid, int sem_num){
    struct sembuf sb;

    sb.sem_num = sem_num;
    sb.sem_op = 1; //Increment and permit those waiting on the semaphore to enter critical section
    sb.sem_flg = 0;

    int ret = semop(semid, &sb, 1);
    if (ret == -1 ){
        int errnum = errno;
        fprintf(stderr, "Sem_Push: Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error opening file: %s\n", strerror( errnum ));

    }
    //Carry our semaphore operation
    return ret;
}

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                (Linux-specific) */
};

/*
** initsem() -- more-than-inspired by W. Richard Stevens' UNIX Network
** Programming 2nd edition, volume 2, lockvsem.c, page 295.
*/
int _initsem(key_t key, int nsems)  /* key from ftok() */
{
    int i;
    union semun arg;
    struct semid_ds buf;
    struct sembuf sb;
    int semid;

    semid = semget(key, nsems, IPC_CREAT | IPC_EXCL | 0666);

    if (semid >= 0) { /* we got it first */
            sb.sem_op = 1; sb.sem_flg = 0;
            arg.val = 1;

            for(sb.sem_num = 0; sb.sem_num < nsems; sb.sem_num++) { 
                    /* do a semop() to "free" the semaphores. */
                    /* this sets the sem_otime field, as needed below. */
                    if (semop(semid, &sb, 1) == -1) {
                            int e = errno;
                            semctl(semid, 0, IPC_RMID); /* clean up */
                            errno = e;
                            return -1; /* error, check errno */
                    }
            }

    } else if (errno == EEXIST) { /* someone else got it first */
            int ready = 0;

            semid = semget(key, nsems, 0); /* get the id */
            if (semid < 0) return semid; /* error, check errno */

            /* wait for other process to initialize the semaphore: */
            arg.buf = &buf;
            for(i = 0; i < MAX_RETRIES && !ready; i++) {
                    semctl(semid, nsems-1, IPC_STAT, arg);
                    if (arg.buf->sem_otime != 0) {
                            ready = 1;
                    } else {
                            sleep(1);
                    }
            }
            if (!ready) {
                    errno = ETIME;
                    return -1;
            }
    } else {
            return semid; /* error, check errno */
    }
    return semid;
}


sem_spsc_queue_header* get_sem_queue_header(void* shmaddr) {
    sem_spsc_queue_header* header = (sem_spsc_queue_header*) shmaddr;
    return header;
}

void* get_message_array(sem_spsc_queue_header* header) {
    return (char*) header + sizeof(sem_spsc_queue_header);
}


ssize_t queue_create(void* shmaddr, key_t key_seed, size_t shm_size, size_t message_size){
    if (message_size + (size_t) sizeof(sem_spsc_queue_header) > shm_size) {
        perror("Message size type + Header data too large for allocated shm");
        exit(1);
    }

    // Calculate size of array
    int num_elements = (shm_size - sizeof(sem_spsc_queue_header)) / message_size;
    int leftover_bytes = (shm_size - sizeof(sem_spsc_queue_header)) % message_size;


    sem_spsc_queue_header* header_ptr = get_sem_queue_header(shmaddr); //Necessary???
    header_ptr->head = 0;
    header_ptr->tail = 0;
    header_ptr->message_size = message_size;
    header_ptr->message_offset = 0;
    header_ptr->queue_size = num_elements;
    header_ptr->current_count = 0;
    header_ptr->total_count = 0;
    header_ptr->stop_consumer_polling = false;
    header_ptr->stop_producer_polling = false;


    key_t sem_full_key = abs(key_seed)/2;
    //must be set size of number of elements, 
    if ((header_ptr->sem_full = _initsem(sem_full_key, 1)) == -1) {
            perror("initsem");
            exit(1);
    }
    // Increment full_sem
    // for(int i = 0; i < header_ptr->queue_size ; i++){//subtle, as the semaphores get initialized to one, this must be incremented only header_ptr->queue_size - 1 times
        // sem_post(header_ptr->sem_full,0); 
    // }


    key_t sem_empty_key = abs(key_seed)/20;
    //Initialize with 1, as the slots can only be full or not
    if ((header_ptr->sem_empty = _initsem(sem_empty_key, 1)) == -1) {
            perror("initsem");
            exit(1);
    }
    //decrement semaphore as it will start off empty.
    sem_wait(header_ptr->sem_empty, 0);


    key_t lock_key = abs(key_seed)/200;
    //Initialized with 1, you can grab or realease lock
    if ((header_ptr->sem_lock = _initsem(lock_key, 1)) == -1) {
            perror("initsem");
            exit(1);
    }
    return shm_size - leftover_bytes;
}

int succ(int index, int queue_size){
    return (index + 1) % queue_size;

}

bool is_full(sem_spsc_queue_header *header) {
    return succ(header->head, header->queue_size) == header->tail;
}
bool is_empty(sem_spsc_queue_header *header) {
    return  header->head == header->tail;
}

void enqueue(sem_spsc_queue_header* header, const void* buf, size_t buf_size) {
    void* array_start = get_message_array(header);

    int message_size = header->message_size;

    memcpy((char*) array_start + header->head * message_size, buf, buf_size);
    
    header->current_count++;
    header->total_count++;

    header->head = succ(header->head, header->queue_size);
}


int sem_push(void* shmaddr, const void* buf, size_t buf_size) {
    sem_spsc_queue_header* header = get_sem_queue_header(shmaddr);
    int ret = 0;
    if (buf_size > header->message_size) {
        return -1; 
    }

    sem_wait(header->sem_lock, 0);
    bool const was_empty = is_empty(header);

    if (is_full(header)){
        sem_post(header->sem_lock, 0);
        while (is_full(header)){
            sem_wait(header->sem_full, 0); //wait on full
        }
        sem_wait(header->sem_lock, 0);
    }
    
    enqueue(header, buf, buf_size); 
    sem_post(header->sem_lock,0);

    if (was_empty){
        sem_post(header->sem_empty,0); //increment, permit sem_pop to dequeue
    }
    return ret;
}




size_t dequeue(sem_spsc_queue_header* header, void* buf, size_t* buf_size) {
    void* array_start = get_message_array(header);

    if(*buf_size > header->message_size+1){
        printf("not supposed to happen");
    }

    // handle offset math
    if (*buf_size + header->message_offset > header->message_size) {
        *buf_size = header->message_size - header->message_offset;
    }

    memcpy(
        buf,
        (char*) array_start + header->tail * header->message_size + header->message_offset,
        *buf_size
    );

    header->message_offset += *buf_size;
    if ((size_t) header->message_offset == header->message_size) {

        header->tail = succ(header->tail, header->queue_size);

        header->current_count--;
        header->message_offset = 0;
    }

    return header->message_offset;

}


size_t sem_pop(void* shmaddr, void* buf, size_t* buf_size) {
    sem_spsc_queue_header* header = get_sem_queue_header(shmaddr);
    size_t ret = 0;

    if (header->stop_consumer_polling) {
        return 0;
    }

    sem_wait(header->sem_lock, 0);
    bool const was_full= is_full(header);
    if(is_empty(header)){
        sem_post(header->sem_lock,0);
        while (is_empty(header)){
            sem_wait(header->sem_empty,0); // This doesnt work cause sem and tail fall out of sync
        }
        sem_wait(header->sem_lock,0);
    }
   
    ret = dequeue(header, buf, buf_size);
    sem_post(header->sem_lock, 0);

    if(was_full){
        sem_post(header->sem_full,0); 
    }
    return ret;
}


const void* sem_peek(void* shmaddr, ssize_t *size) {
    sem_spsc_queue_header* header = get_sem_queue_header(shmaddr);

    int message_size = header->message_size;
    int queue_head = header->head;

    void* array_start = get_message_array(header);

    *size = header->message_size;
    return (const void*) ((char*) array_start + queue_head*message_size);
}


