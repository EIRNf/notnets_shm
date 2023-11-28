#include "../../src/mem.h"
#include "../../src/queue.h"
#include  <time.h>
#include <pthread.h>
#include <assert.h>
#include <stdatomic.h>

#define NUM_ITEMS 1000000
#define MESSAGE_SIZE 4 //Int

void* shmaddr; //pointer to queue
struct timespec start, end;
atomic_bool run_flag = false; //control execution

void bench_report_stats() {
    fprintf(stdout, "notnets/spsc\n");
    //nanosecond ns 1.0e-09
    //microsecond us 1.0e-06
    //millisecond ms 1.0e-03

    if (end.tv_sec > 0 ){ //we have atleast 1 sec
        //use millisecond scale
        long ms = (end.tv_sec - start.tv_sec) * 1.0e+03;
	    ms += (end.tv_nsec - start.tv_nsec) / 1.0e+06;

        // fprintf(stdout, "\n execution time:%ld ms \n ", ms);
        //Latency 
        // fprintf(stdout, "\n latency: %ld ms/op \n", ms/NUM_ITEMS);
        // long ns =  (end.tv_nsec - start.tv_nsec);
        // fprintf(stdout, "\n latency:%ld ns/op \n ", ns/NUM_ITEMS);
        //Throughput
        fprintf(stdout, "%ld ops/ms \n", NUM_ITEMS/ms);

    }
    else {//nanosecond scale, probably not very accurate
        long ns =  (end.tv_nsec - start.tv_nsec);

        // fprintf(stdout, "\n execution time:%ld ns \n ", ns);
        //Latency 
        // fprintf(stdout, "\n latency:%ld ns/op \n ", ns/NUM_ITEMS);

        //Throughput
        //Convert to reasonable unit ms
        long ms = ns / 1.0e+06;
        fprintf(stdout, "%ld ops/ms \n", NUM_ITEMS/ms);
    }


  
}

void *producer_push(){
    int *buf = malloc(MESSAGE_SIZE);
    // int message_size = MESSAGE_SIZE;

    //hold until flag is set to true
    while(!run_flag);

    for(int i=1;i<NUM_ITEMS;i++) {
        *buf = i;
        //push
        push(shmaddr, buf, MESSAGE_SIZE);

    }
    free(buf);
    pthread_exit(0);
}
 
void consumer_pop(){
    size_t *message_size = malloc(sizeof(ssize_t));
    *message_size = MESSAGE_SIZE;
    int* pop_buf = malloc(*message_size);

    //hold until flag is set to true
    while(!run_flag);

    for(int i=1;i<NUM_ITEMS;i++) {
        if (i == NUM_ITEMS-1){
            break;
        }

        pop(shmaddr, pop_buf, message_size);
 
        assert(*pop_buf == i);
    }

    free(pop_buf);
}

void notnets_bench_enqueue_dequeue(){
    // create shm and attach to it
    key_t key = 1;
    int shm_size = 16024; // make adjustable 

    pthread_t producer;
    // pthread_t consumer;


    int shmid = shm_create(key, shm_size, create_flag);

    while (shmid == -1) {
        shmid = shm_create(key, shm_size, create_flag);
    }

    shmaddr = shm_attach(shmid);

    // create queue
    queue_create(shmaddr, shm_size, MESSAGE_SIZE);

    //producer thread
    pthread_create(&producer, NULL, producer_push, NULL);

    //Actual measuring
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    run_flag = true;
    consumer_pop();
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

    pthread_join(producer,NULL);    

    bench_report_stats();

    // cleanup
    shm_detach(shmaddr);
    shm_remove(shmid);


}





void bench_run_all(void){
    
    notnets_bench_enqueue_dequeue();

}