#include "sem_queue.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/sem.h>
#include <semaphore.h>
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
    semctl(hReq->sem_slots_free, 0, IPC_RMID);
    semctl(hReq->sem_slots_full, 0, IPC_RMID);

    semctl(hResp->sem_slots_free, 0, IPC_RMID);
    semctl(hResp->sem_slots_full, 0, IPC_RMID);
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
    sb.sem_flg ;

    int ret = semop(semid, &sb, 1);
    if (ret == -1 ){
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
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
    sb.sem_flg ;

    int ret = semop(semid, &sb, 1);
    if (ret == -1 ){
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error opening file: %s\n", strerror( errnum ));

    }
    //Carry our semaphore operation
    return ret;
}

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

    sem_spsc_queue_header header;
    header.head = 0;
    header.tail = 0;
    header.message_size = message_size;
    header.message_offset = 0;
    header.queue_size = num_elements;
    header.current_count = 0;
    header.total_count = 0;
    header.stop_consumer_polling = false;
    header.stop_producer_polling = false;




    atomic_thread_fence(memory_order_seq_cst);
    sem_spsc_queue_header* header_ptr = get_sem_queue_header(shmaddr); //Necessary???
    *header_ptr = header;

    key_t slots_free_key = abs(key_seed)/2;
    //must be set size of number of elements, 
    if ((header_ptr->sem_slots_free = _initsem(slots_free_key, num_elements)) == -1) {
            perror("initsem");
            exit(1);
    }


    key_t slots_full_key = abs(key_seed)/20;
    //Initialize with 1, as the slots can only be full or not
    if ((header_ptr->sem_slots_full = _initsem(slots_full_key, 1)) == -1) {
            perror("initsem");
            exit(1);
    }
    //decrement semaphore as it will start off empty.
    sem_wait(header_ptr->sem_slots_full, 0);

    atomic_thread_fence(memory_order_seq_cst);

    return shm_size - leftover_bytes;
}



bool is_full(sem_spsc_queue_header *header) {
    atomic_thread_fence(memory_order_acquire);
    return (header->tail + 1) % header->queue_size == header->head;
}


void enqueue(sem_spsc_queue_header* header, const void* buf, size_t buf_size) {
    void* array_start = get_message_array(header);

    int message_size = header->message_size;
    int tail = header->tail;

    memcpy((char*) array_start + tail*message_size, buf, buf_size);

    atomic_thread_fence(memory_order_release);
    
    header->current_count++;
    header->total_count++;

    // TODO: fence?

    header->tail = (header->tail + 1) % header->queue_size;
    atomic_thread_fence(memory_order_release);
}


int sem_push(void* shmaddr, const void* buf, size_t buf_size) {
    sem_spsc_queue_header* header = get_sem_queue_header(shmaddr);
    int ret = 0;

    if (buf_size > header->message_size) {
        return -1;
    }

    int wait, post = 0;
    wait = sem_wait(header->sem_slots_free,header->tail); //Thread should wait until semaphore is incremented???
        if (header->stop_producer_polling) {
            ret = -1;
        }
        // This should not happen as semaphore should block if we are full
        // if (is_full(header)) {//What do when full? The semaphore should block no?
        //     ret = -1; //-1 for it being full?
        // } else {
        enqueue(header, buf, buf_size);
        // }
    post = sem_post(header->sem_slots_full,0); //increment, permit sem_pop to dequeue 
    return ret;
}


bool is_empty(sem_spsc_queue_header *header) {
    atomic_thread_fence(memory_order_acquire);
    return header->head == header->tail;
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
        (char*) array_start + header->head*header->message_size + header->message_offset,
        *buf_size
    );
    atomic_thread_fence(memory_order_release);

    header->message_offset += *buf_size;

    if ((size_t) header->message_offset == header->message_size) {
        header->head = (header->head + 1) % header->queue_size;
        header->current_count--;
        header->message_offset = 0;
    }
    atomic_thread_fence(memory_order_release);

    return header->message_offset;

}


size_t sem_pop(void* shmaddr, void* buf, size_t* buf_size) {
    sem_spsc_queue_header* header = get_sem_queue_header(shmaddr);
    size_t ret = 0;
    
    int wait, post = 0;
    wait = sem_wait(header->sem_slots_full, 0); //Block until semaphore is incremented ie something has been pushed.
        if (header->stop_consumer_polling) {
            return 0;
        }
        // Panic, this should not happen as semaphore should block the thread if we are empty
        // if (is_empty(header)) { 
        //     ret = 0;
        // } else {
        ret = dequeue(header, buf, buf_size);
        // }
    post = sem_post(header->sem_slots_free,header->head); 
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


