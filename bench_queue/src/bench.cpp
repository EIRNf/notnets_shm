
#include "mem.h"
#include "queue.h"
#include "queue_umwait.h"
#include "sem_queue.h"
#include "coord.h"
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
#include <sys/resource.h>
#include <sys/time.h>

// #define NUM_ITEMS 10000000
#define MESSAGE_SIZE 4 //Int
#define EXECUTION_SEC 20

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



void reset_run_stats(run_stats *run){
    run->shmaddr = 0;
    run->max_queue_depth = 0;
    run->start.tv_nsec = 0;
    run->start.tv_sec = 0;
    run->end.tv_nsec = 0;
    run->end.tv_sec = 0;
    run->total.tv_nsec = 0;
    run->total.tv_sec = 0;
    run->start_flag = false;
    run->stop_flag = false;
    run->max_queue_depth = 0;
    run->average_queue_depth = 0;
}
// void* shmaddr; //pointer to queue
// struct timespec start, end;
// atomic_bool stop_flag = false;

int items_consumed = 0;

int cpu0 = 0;
int cpu1 = 1;

void save_bench_stats_as_csv(char* name) {
    char* file_name;
    asprintf(&file_name, "%s_bench.csv", name);

    // Open a file to write the CSV data
    FILE *fp = fopen(file_name, "w");
    if (fp == NULL) {
        perror("Failed to open file");
        return;
    }

    // Write the CSV header
    fprintf(fp, "Name,Time (sec),ItemsConsumed,MaxQueueDepth,AverageQueueDepth,ns/op,ops/ms\n");

    // Calculate the total time in microseconds and milliseconds
    double total_us = run1.total.tv_sec * 1.0e+06;
    total_us += run1.total.tv_nsec / 1.0e+03;

    double total_ms = run1.total.tv_sec * 1.0e+03;
    total_ms += run1.total.tv_nsec / 1.0e+06;

    // Write the benchmark data in CSV format
    fprintf(fp, "%s,%ld,%d,%d,%d,%f,%f\n",
            name,
            run1.total.tv_sec,
            items_consumed,
            run1.max_queue_depth,
            run1.average_queue_depth,
            (total_us * 1.0e+03) / items_consumed,
            items_consumed / total_ms);

    // Close the file
    fclose(fp);
    free(file_name);
}

void print_bench_stats_as_csv(char* name) {
    // Write the CSV header
    fprintf(stdout, "Name,Time (sec),ItemsConsumed,MaxQueueDepth,AverageQueueDepth,ns/op,ops/ms\n");

    // Calculate the total time in microseconds and milliseconds
    double total_us = run1.total.tv_sec * 1.0e+06;
    total_us += run1.total.tv_nsec / 1.0e+03;

    double total_ms = run1.total.tv_sec * 1.0e+03;
    total_ms += run1.total.tv_nsec / 1.0e+06;

    // Write the benchmark data in CSV format
    fprintf(stdout, "%s,%ld,%d,%d,%d,%f,%f\n",
            name,
            run1.total.tv_sec,
            items_consumed,
            run1.max_queue_depth,
            run1.average_queue_depth,
            (total_us * 1.0e+03) / items_consumed,
            items_consumed / total_ms);

}

void print_bench_stats(char* name) {
    
    fprintf(stdout, "\n%s\n", name);
    //nanosecond ns 1.0e-09
    //microsecond us 1.0e-06
    //millisecond ms 1.0e-03
    fprintf(stdout, "Time: %ld sec, ItemsConsumed: %d  \n", run1.total.tv_sec ,items_consumed);

    fprintf(stdout, "MaxQueueDepth: %d \n", run1.max_queue_depth);
    fprintf(stdout, "AverageQueueDepth: %d \n", run1.average_queue_depth);

    //microsecond scane 
    double total_us = run1.total.tv_sec * 1.0e+06;
    total_us += run1.total.tv_nsec / 1.0e+03;

    double total_ms = run1.total.tv_sec * 1.0e+03;
    total_ms += run1.total.tv_nsec / 1.0e+06;

    fprintf(stdout, "%f ns/op \n", (total_us * 1.0e+03)/items_consumed);

    fprintf(stdout, "%f ops/ms \n", items_consumed/(total_ms));
    // fprintf(stdout, "%ld ns/op \n ", ns/items_consumed);
}

void save_rusage_as_csv(struct rusage *usage, const char* name) {

    char* file_name;
    asprintf(&file_name, "%s_rusage.csv", name);

    // Open a file to write the CSV data
    FILE *fp = fopen(file_name, "w");
    if (fp == NULL) {
        perror("Failed to open file");
        return;
    }

    // Write the CSV header
    fprintf(fp, "user_CPU_time(sec),system_CPU_time(sec),max_RSS(KB),shm_size (KB),unshared_data_size(KB),unshared_stack_size(KB),page_reclaims,page_faults,swaps,block_input_ops,block_output_ops, IPC_messages_sent,IPC_messages_received,signals_received,voluntary_context_switches,involuntary_context_switches\n");

    // Write the rusage data in CSV format
    fprintf(fp, "%ld.%06ld,%ld.%06ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld\n",
            usage->ru_utime.tv_sec, usage->ru_utime.tv_usec,
            usage->ru_stime.tv_sec, usage->ru_stime.tv_usec,
            usage->ru_maxrss,
            usage->ru_ixrss,
            usage->ru_idrss,
            usage->ru_isrss,
            usage->ru_minflt,
            usage->ru_majflt,
            usage->ru_nswap,
            usage->ru_inblock,
            usage->ru_oublock,
            usage->ru_msgsnd,
            usage->ru_msgrcv,
            usage->ru_nsignals,
            usage->ru_nvcsw,
            usage->ru_nivcsw);

    // Close the file
    fclose(fp);
    free(file_name);
}

void print_rusage(struct rusage *usage) {
    printf("User CPU time used: %ld.%06ld seconds\n", 
           usage->ru_utime.tv_sec, usage->ru_utime.tv_usec);
    printf("System CPU time used: %ld.%06ld seconds\n", 
           usage->ru_stime.tv_sec, usage->ru_stime.tv_usec);
    printf("Maximum resident set size: %ld KB\n", usage->ru_maxrss);
    printf("Integral shared memory size: %ld KB\n", usage->ru_ixrss);
    printf("Integral unshared data size: %ld KB\n", usage->ru_idrss);
    printf("Integral unshared stack size: %ld KB\n", usage->ru_isrss);
    printf("Page reclaims (soft page faults): %ld\n", usage->ru_minflt);
    printf("Page faults (hard page faults): %ld\n", usage->ru_majflt);
    printf("Swaps: %ld\n", usage->ru_nswap);
    printf("Block input operations: %ld\n", usage->ru_inblock);
    printf("Block output operations: %ld\n", usage->ru_oublock);
    printf("IPC messages sent: %ld\n", usage->ru_msgsnd);
    printf("IPC messages received: %ld\n", usage->ru_msgrcv);
    printf("Signals received: %ld\n", usage->ru_nsignals);
    printf("Voluntary context switches: %ld\n", usage->ru_nvcsw);
    printf("Involuntary context switches: %ld\n", usage->ru_nivcsw);
}

void record_usage_with_run_name(const char* name) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        save_rusage_as_csv(&usage, name);
    } else {
        perror("getrusage failed");
    }
}

void record_usage() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        print_rusage(&usage);
    } else {
        perror("getrusage failed");
    }
}


#ifdef __APPLE__
void pinThread(int cpu) {
//empty func
    (void)cpu;
}
#else
void pinThread(int cpu) {
//   if (cpu < 0) {
//     return;
//   }
//   cpu_set_t cpuset;
//   CPU_ZERO(&cpuset);
//   CPU_SET(cpu, &cpuset);
//   if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) ==
//       -1) {
//     perror("pthread_setaffinity_no");
//     exit(1);
//   }
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

void *producer_push_sem(void* arg){
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
        sem_push(run1.shmaddr, &i, MESSAGE_SIZE);
        
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
 
void *consumer_pop_sem(void* arg){
    pinThread(cpu0);
    size_t *message_size = (size_t*)malloc(sizeof(ssize_t));
    *message_size = MESSAGE_SIZE;

    u_int* pop_buf = (u_int*)malloc(*message_size);

    while(!run1.start_flag);
    u_int i = 0;
    clock_gettime(CLOCK_MONOTONIC, &run1.start);
    while (!run1.stop_flag){
        sem_pop(run1.shmaddr, pop_buf, message_size);
        
        assert(*pop_buf == i);
        i++;
    }
    clock_gettime(CLOCK_MONOTONIC, &run1.end);
    items_consumed = i;
    free(pop_buf);
    pthread_exit(0);
}

void *producer_push_umwait(void* arg){
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
        umwait_push(run1.shmaddr, &i, MESSAGE_SIZE);
        
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
 
void *consumer_pop_umwait(void* arg){
    pinThread(cpu0);
    size_t *message_size = (size_t*)malloc(sizeof(ssize_t));
    *message_size = MESSAGE_SIZE;

    u_int* pop_buf = (u_int*)malloc(*message_size);

    while(!run1.start_flag);
    u_int i = 0;
    clock_gettime(CLOCK_MONOTONIC, &run1.start);
    while (!run1.stop_flag){
        umwait_pop(run1.shmaddr, pop_buf, message_size);
        
        assert(*pop_buf == i);
        i++;
    }
    clock_gettime(CLOCK_MONOTONIC, &run1.end);
    items_consumed = i;
    free(pop_buf);
    pthread_exit(0);
}

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

void notnets_bench_enqueue_dequeue(QUEUE_TYPE type, int mem_size, char* name){

    pthread_t producer;
    pthread_t consumer;
    reset_run_stats(&run1);


    // Create shm and attach to it
    key_t key = 1;
    int shm_size = mem_size;
    int shmid = shm_create(key, shm_size, create_flag);
    while (shmid == -1) {
        shmid = shm_create(key, shm_size, create_flag);
    }
    run1.shmaddr = shm_attach(shmid);

    switch(type){
        case POLL:
            // create queue
            queue_create(run1.shmaddr, shm_size, MESSAGE_SIZE);
            //producer thread
            pthread_create(&producer, NULL, producer_push, NULL);
            //consumer thread
            pthread_create(&consumer, NULL, consumer_pop, NULL);
            break;
        case UMWAIT:
            // create queue
            queue_create_umwait(run1.shmaddr, shm_size, MESSAGE_SIZE);
            //producer thread
            pthread_create(&producer, NULL, producer_push_umwait, NULL);
            //consumer thread
            pthread_create(&consumer, NULL, consumer_pop_umwait, NULL);
            break;
        case SEM:
            // create queue
            queue_create(run1.shmaddr, key+100, shm_size, MESSAGE_SIZE);
            //producer thread
            pthread_create(&producer, NULL, producer_push_sem, NULL);
            //consumer thread
            pthread_create(&consumer, NULL, consumer_pop_sem, NULL);
            break;
        default:
            printf("DEFAULT");
            return;

    }


    run1.start_flag = true;
    sleep(EXECUTION_SEC);
    //Signal stop
    run1.stop_flag = true;

    pthread_join(producer,NULL);  
    pthread_join(consumer,NULL);    
  
    run1.total.tv_sec = run1.end.tv_sec - run1.start.tv_sec;
    run1.total.tv_nsec = run1.end.tv_nsec - run1.start.tv_nsec;

    // cleanup
    switch(type){
        case POLL:
            break;
        case UMWAIT:
            break;
        case SEM:
            delete_queue_semaphores(run1.shmaddr);
            break;
        default:
            printf("DEFAULT");
            return;

    }
    shm_detach(run1.shmaddr);
    shm_remove(shmid);
    
    // print_bench_stats_as_csv(name);
    save_bench_stats_as_csv(name);
    record_usage_with_run_name(name);
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
  

    print_bench_stats((char*)"iterative-notnets/spsc");

    // cleanup
    shm_detach(run1.shmaddr);
    shm_remove(shmid);


}

void bench_run_all(QUEUE_TYPE type, int value, char* name){
    notnets_bench_enqueue_dequeue(type, value, name);
}

int main(int argc, char *argv[]) {
    QUEUE_TYPE mode = POLL;  // Default mode
    int value = 1624;      // Default value
    char* name = (char*)"POLL";

    if (argc > 1) {
        if (strcmp(argv[1], "POLL") == 0) {
            mode = POLL;
            name = (char*)"POLL";
        } else if (strcmp(argv[1], "UMWAIT") == 0) {
            mode = UMWAIT;
            name = (char*)"UMWAIT";
        } else if (strcmp(argv[1], "SEM") == 0) {
            mode = SEM;
            name =(char*)"SEM";
        } else {
            fprintf(stderr, "Invalid mode. Using default (SEM).\n");
        }
    }

    if (argc > 2) {
        value = atoi(argv[2]);  // Convert second argument to int
    }

    bench_run_all(mode, value, name);
    return 0;
}

/* Run client and server processes from same program distinguished by different arguments
Goals:
- Measure "echo" latency
- Measure throughput as inverse of latency as it is single threaded 
- Tune payload size
- Tune queue size
*/ 



