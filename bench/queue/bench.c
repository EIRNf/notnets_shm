#include "../../src/mem.h"
#include "../../src/queue.h"
#include  <time.h>
#include <pthread.h>
#include <assert.h>

#define NUM_ITEMS 100000000
#define MESSAGE_SIZE 4 //Int

void* shmaddr; //pointer to queue
struct timespec start, end;

void bench_report_stats() {

    if (end.tv_sec > 0 ){
        long ms = (end.tv_sec - start.tv_sec) * 1000;
	    ms += (end.tv_nsec - start.tv_nsec) / 1000000;

        fprintf(stdout, "\n execution time:%ld ms \n ", ms);
        //Latency 
        fprintf(stdout, "\n latency: %ld ms/op \n", ms/NUM_ITEMS);
        //Throughput
        fprintf(stdout, "\n throughput:%ld reqs/ms \n", NUM_ITEMS/ms);

    }
    else {//nanosecond scale
        long ns =  (end.tv_nsec - start.tv_nsec);

        fprintf(stdout, "\n execution time:%ld ns \n ", ns);

        //Latency 
        fprintf(stdout, "\n latency:%ld ns/op \n ", ns/NUM_ITEMS);
        //Throughput
        //Convert to reasonable unit
        long us = ns / 1000000;
        fprintf(stdout, "\n throughput:%ld reqs/us \n", NUM_ITEMS/us);
    }


  
}


void *producer_push(){
    int *buf = malloc(MESSAGE_SIZE);
    // int message_size = MESSAGE_SIZE;

    for(int i=1;i<NUM_ITEMS;i++) {
        *buf = i;
        //push
        push(shmaddr, buf, MESSAGE_SIZE);

    }
    free(buf);
    pthread_exit(0);
}
 
void *consumer_pop(){
    size_t *message_size = malloc(sizeof(ssize_t));
    *message_size = MESSAGE_SIZE;
    int* pop_buf = malloc(*message_size);


    for(int i=1;i<NUM_ITEMS;i++) {
        if (i == NUM_ITEMS-1){
            break;
        }

        pop(shmaddr, pop_buf, message_size);
 
        assert(*pop_buf == i);
    }

    free(pop_buf);
    pthread_exit(0);
}


void bench_enqueue_dequeue(){


    // create shm and attach to it
    key_t key = 1;
    int shm_size = 1024; // make adjustable 

    pthread_t producer;
    pthread_t consumer;


    int shmid = shm_create(key, shm_size, create_flag);

    while (shmid == -1) {
        shmid = shm_create(key, shm_size, create_flag);
    }

    shmaddr = shm_attach(shmid);

    // create queue
    queue_create(shmaddr, shm_size, MESSAGE_SIZE);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

    //Do work, hopefully thread creation is amortized
    pthread_create(&producer, NULL, producer_push, NULL);
    pthread_create(&consumer, NULL, consumer_pop, NULL);

    pthread_join(producer,NULL);    
    pthread_join(consumer,NULL);    
    
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);


    bench_report_stats();

    // cleanup
    shm_detach(shmaddr);
    shm_remove(shmid);


}

//How to avoid waiting until not empty or 
//empty queue when calling pop when taking process time
void bench_run_all(void){
    bench_enqueue_dequeue();
}