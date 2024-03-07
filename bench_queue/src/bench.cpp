
#include "mem.h"
#include "queue.h"
#include <sched.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

// #define NUM_ITEMS 10000000
#define MESSAGE_SIZE 4 //Int
#define EXECUTION_SEC 60

typedef struct run_stats {
    void* shmaddr;
    struct timespec start, end, total;
    atomic_bool start_flag;
    atomic_bool stop_flag;
    int max_queue_depth;
    int average_queue_depth;
} run_stats;
    

run_stats run1 = {
    .start_flag = false,
    .stop_flag = false,
    };

// void* shmaddr; //pointer to queue
// struct timespec start, end;
// atomic_bool stop_flag = false;

int items_consumed = 0;

int cpu0 = 0;
int cpu1 = 1;

void bench_report_stats(run_stats run, char* name) {
    
    fprintf(stdout, "\n%s\n", name);
    //nanosecond ns 1.0e-09
    //microsecond us 1.0e-06
    //millisecond ms 1.0e-03
    fprintf(stdout, "Time: %ld sec, ItemsConsumed: %d  \n", run1.total.tv_sec ,items_consumed);

    fprintf(stdout, "MaxQueueDepth: %d \n", run.max_queue_depth);
    fprintf(stdout, "AverageQueueDepth: %d \n", run.average_queue_depth);

    //microsecond scane 
    double total_us = run1.total.tv_sec * 1.0e+06;
    total_us += run1.total.tv_nsec / 1.0e+03;

    double total_ms = run1.total.tv_sec * 1.0e+03;
    total_ms += run1.total.tv_nsec / 1.0e+06;

    fprintf(stdout, "%f ns/op \n", (total_us * 1.0e+03)/items_consumed);

    fprintf(stdout, "%f ops/ms \n", items_consumed/(total_ms));
    // fprintf(stdout, "%ld ns/op \n ", ns/items_consumed);
}


#ifdef __APPLE__
void pinThread(int cpu) {
//empty func
    (void)cpu;
}
#else
void pinThread(int cpu) {
  if (cpu < 0) {
    return;
  }
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) ==
      -1) {
    perror("pthread_setaffinity_no");
    exit(1);
  }
}
#endif

// void record_core_util(){
//     struct timespec currTime;
//     clockid_t threadClockId;

//      //! Get thread clock Id
//     pthread_getcpuclockid(pthread_self(), &threadClockId);
//     //! Using thread clock Id get the clock time
//     clock_gettime(threadClockId, &currTime);

//     rusage *usage = malloc(sizeof(rusage)); 
    
//     getrusage(RUSAGE_THREAD, usage);

// }


void *producer_push(void* arg){
    pinThread(cpu1);
    u_int *buf = (u_int*)malloc(MESSAGE_SIZE);
    // int message_size = MESSAGE_SIZE;

    //hold until flag is set to true
    while(!run1.start_flag);

    u_int i = 0;
    int queue_depth_samples = 0;
    int queue_depth = 0;
    // buf = &i;
    while(!run1.stop_flag){
        // *buf = i;
        //push
        push(run1.shmaddr, &i, MESSAGE_SIZE);
        
        //Interval sampling
        if(i%1000000==0){
            queue_depth_samples++;
            // printf("head:%d\n", get_queue_header(run1.shmaddr)->head);
            // printf("tail:%d\n", get_queue_header(run1.shmaddr)->tail);

            
            queue_depth = (get_queue_header(run1.shmaddr)->tail - get_queue_header(run1.shmaddr)->head + get_queue_header(run1.shmaddr)->queue_size ) % get_queue_header(run1.shmaddr)->queue_size;
            // printf("count:%d\n", queue_depth);

            if(queue_depth > run1.max_queue_depth){
                run1.max_queue_depth = queue_depth;
            }
            run1.average_queue_depth += queue_depth;

        }

        i++;
    }
    run1.average_queue_depth = run1.average_queue_depth / queue_depth_samples;
    free(buf);
    pthread_exit(0);
}
 
void *consumer_pop(void* arg){
    pinThread(cpu0);
    size_t *message_size = (size_t*)malloc(sizeof(ssize_t));
    *message_size = MESSAGE_SIZE;

    u_int* pop_buf = (u_int*)malloc(*message_size);

    while(!run1.start_flag);
    u_int i = 0;
    clock_gettime(CLOCK_MONOTONIC, &run1.start);
    while (!run1.stop_flag){
        pop(run1.shmaddr, pop_buf, message_size);
        
        assert(*pop_buf == i);
        i++;
    }
    clock_gettime(CLOCK_MONOTONIC, &run1.end);
    items_consumed = i;
    free(pop_buf);
    pthread_exit(0);
}

void *interval_consumer_pop(void* arg){
    pinThread(cpu0);
    size_t *message_size = (size_t*)malloc(sizeof(ssize_t));
    *message_size = MESSAGE_SIZE;

    u_int* pop_buf = (u_int*)malloc(*message_size);

    while(!run1.start_flag);
    u_int i = 0;
    while (!run1.stop_flag){
        clock_gettime(CLOCK_MONOTONIC, &run1.start);
        pop(run1.shmaddr, pop_buf, message_size);
        
        assert(*pop_buf == i);
        i++;
        clock_gettime(CLOCK_MONOTONIC, &run1.end);
        // run1.total.tv_sec = run1.end.tv_sec - run1.start.tv_sec;
        run1.total.tv_nsec = run1.end.tv_nsec - run1.start.tv_nsec;
    }
    items_consumed = i;
    free(pop_buf);
    pthread_exit(0);
}

void notnets_bench_enqueue_dequeue(){
    // create shm and attach to it
    key_t key = 1;
    int shm_size = 16024; // make adjustable 

    pthread_t producer;
    pthread_t consumer;

    int shmid = shm_create(key, shm_size, create_flag);

    while (shmid == -1) {
        shmid = shm_create(key, shm_size, create_flag);
    }

    run1.shmaddr = shm_attach(shmid);


    // create queue
    queue_create(run1.shmaddr, shm_size, MESSAGE_SIZE);
    //producer thread
    pthread_create(&producer, NULL, producer_push, NULL);
    pthread_create(&consumer, NULL, consumer_pop, NULL);

    run1.start_flag = true;
    sleep(EXECUTION_SEC);
    //Signal stop
    run1.stop_flag = true;

    pthread_join(producer,NULL);  
    pthread_join(consumer,NULL);    
  
    // run1.total.tv_sec = run1.end.tv_sec - run1.start.tv_sec;
    run1.total.tv_sec = run1.end.tv_sec - run1.start.tv_sec;
    run1.total.tv_nsec = run1.end.tv_nsec - run1.start.tv_nsec;
    bench_report_stats(run1, (char*)"batch-notnets/spsc");

    // cleanup
    shm_detach(run1.shmaddr);
    shm_remove(shmid);


}


void notnets_bench_enqueue_dequeue_interval(){
    // create shm and attach to it
    key_t key = 1;
    int shm_size = 16024; // make adjustable 



    pthread_t producer;
    pthread_t consumer;

    int shmid = shm_create(key, shm_size, create_flag);

    while (shmid == -1) {
        shmid = shm_create(key, shm_size, create_flag);
    }

    run1.shmaddr = shm_attach(shmid);
    

    run1.max_queue_depth = 0;
    run1.start_flag = false;
    run1.stop_flag = false;

    

    // create queue
    queue_create(run1.shmaddr, shm_size, MESSAGE_SIZE);
    //producer thread
    pthread_create(&producer, NULL, producer_push, NULL);
    pthread_create(&consumer, NULL, interval_consumer_pop, NULL);

    run1.start_flag = true;
    sleep(EXECUTION_SEC);
    //Signal stop
    run1.stop_flag = true;

    pthread_join(producer,NULL);  
    pthread_join(consumer,NULL);    
  

    bench_report_stats(run1, (char*)"iterative-notnets/spsc");

    // cleanup
    shm_detach(run1.shmaddr);
    shm_remove(shmid);


}

void bench_run_all(void){
    
    notnets_bench_enqueue_dequeue();

    notnets_bench_enqueue_dequeue_interval();


}


/* Run client and server processes from same program distinguished by different arguments
Goals:
- Measure "echo" latency
- Measure throughput as inverse of latency as it is single threaded 
- Tune payload size
- Tune queue size
*/ 


int main()
{
    bench_run_all();
}

