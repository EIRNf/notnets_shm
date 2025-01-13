#include <stdio.h>
#include "rpc.h"
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>

#include "bench_mutex_utils.h"

#define ITERATIONS  100000

void print_num_clients(){
     fprintf(stdout, "%d\t", MAX_CLIENTS);   
}

void print_coord_slots(){
    fprintf(stdout, "%d\t", SLOT_NUM);
}

void print_rtt_num_items(){
    fprintf(stdout, "%d\t", NUM_ITEMS);
}

void print_msg_size(){
    fprintf(stdout, "%d\t", MESSAGE_SIZE);
}

void print_ns_op(long average_ns){
    fprintf(stdout, "%ld\t", (average_ns/NUM_ITEMS));
}

void print_avg_ops_ms(long average_ns){
    fprintf(stdout, "%f\t", (NUM_ITEMS)/(average_ns / 1.0e+06));
}

void print_ops_ms(long average_ns){
    fprintf(stdout, "%f\t", (NUM_ITEMS * MAX_CLIENTS)/(average_ns / 1.0e+06));
}

int cmpfunc(const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}

void* pthread_connect_client(void* arg){
    struct connection_args *args = (struct connection_args*) arg;

    char name[32];

    snprintf(name, 32, "%d", args->client_id);
    int unique_thread_id = 0;


    while(!run_flag);

    for (int i = 0; i < ITERATIONS; i++) {

    clock_gettime(CLOCK_BOOTTIME, &args->start);

    if (args->lock_type  == PTHREAD) {
        pthread_mutex_lock(&args->lock->pthread);
    } else if (args->lock_type == BAKERY) {
        unique_thread_id = get_process_id(&args->lock->bakery);
        lock(&args->lock->bakery, unique_thread_id);
    } else if (args->lock_type == BURNS_AND_LYNCH) {
        unique_thread_id = get_process_id(&args->lock->burns_and_lynch);
        lock(&args->lock->burns_and_lynch, unique_thread_id);
    } else if (args->lock->pthread_spin == PTHREAD_SPIN)
    {
        pthread_spin_lock(&args->lock->pthread_spin);
    }



    if (args->lock_type  == PTHREAD) {
        pthread_mutex_unlock(&args->lock->pthread);
    } else if (args->lock_type == BAKERY) {
        unlock(&args->lock->bakery, unique_thread_id);
    } else if (args->lock_type == BURNS_AND_LYNCH) {
        unlock(&args->lock->burns_and_lynch, unique_thread_id);
    } else if (args->lock->pthread_spin == PTHREAD_SPIN)
    {
        pthread_spin_unlock(&args->lock->pthread_spin);
    }

    clock_gettime(CLOCK_BOOTTIME, &args->end);

    }



    pthread_exit(0);
}

void report_single_rtt_stats(struct connection_args *args) {
    //nanosecond ns 1.0e-09
    //microsecond us 1.0e-06
    //millisecond ms 1.0e-03
    struct timespec start = args->start;
    struct timespec end = args->end;


    if (end.tv_sec - start.tv_sec > 0 ){ //we have atleast 1 sec

        //use millisecond scale
        long ms = (end.tv_sec - start.tv_sec) * 1.0e+03;
	    ms += (end.tv_nsec - start.tv_nsec) / 1.0e+06;

        // fprintf(stdout, "\n execution time:%ld ms \n ", ms);
        //Latency 
        // fprintf(stdout, "\n latency: %ld ms/op \n", ms/NUM_ITEMS);
        long ns = 0.0;
        ns =  (end.tv_sec - start.tv_sec) * 1.0e+09;
        ns +=  (end.tv_nsec - start.tv_nsec);
        fprintf(stdout, "Latency: %ld ns/op \n", ns/NUM_ITEMS);
        //Throughput
        fprintf(stdout, "Throughput: %ld ops/ms \n", NUM_ITEMS/ms);

    }
    else {//nanosecond scale, probably not very accurate
        long ns = 0.0;
        ns =  (end.tv_sec - start.tv_sec) * 1.0e+09;
        ns += (end.tv_nsec - start.tv_nsec);    

        // long ns =  (end.tv_nsec - start.tv_nsec);

        // fprintf(stdout, "\n execution time:%ld ns \n ", ns);
        //Latency 
        fprintf(stdout, "Latency: %ld ns/op \n", ns/NUM_ITEMS);

        //Throughput
        //Convert to reasonable unit ms
        long ms = ns / 1.0e+06;
        fprintf(stdout, "Throughput: %ld ops/ms \n", NUM_ITEMS/ms);
    }  
}

void report_connection_stats(struct connection_args *args){
    //nanosecond ns 1.0e-09
    //microsecond us 1.0e-06
    //millisecond ms 1.0e-03

    struct timespec start = args->start;
    struct timespec end = args->end;

    if ((end.tv_sec - start.tv_sec) > 0 ){ //we have atleast 1 sec
        //use millisecond scale
        long ms = (end.tv_sec - start.tv_sec) * 1.0e+03;
	    ms += (end.tv_nsec - start.tv_nsec) / 1.0e+06;
        
        fprintf(stdout, "Latency: %ld ms/op \n", ms);
    }
    else {//nanosecond scale, probably not very accurate
        long ns = (end.tv_nsec - start.tv_nsec);

        fprintf(stdout, "Latency: %ld ns/op \n", ns);
        
    }  
}

void report_rtt_stats_after_connect(struct connection_args *args){
    //nanosecond ns 1.0e-09
    //microsecond us 1.0e-06
    //millisecond ms 1.0e-03

    struct timespec start = args->start;
    struct timespec end = args->end;


    if ((end.tv_sec - start.tv_sec) > 0 ){ //we have atleast 1 sec
        //use millisecond scale
        long ms = (end.tv_sec - start.tv_sec) * 1.0e+03;
	    ms += (end.tv_nsec - start.tv_nsec) / 1.0e+06;

        // //Latency 
        fprintf(stdout, "Latency: %f ns/op \n", (ms * 1.0e+06)/NUM_ITEMS);
       
    }
    else {//nanosecond scale, probably not very accurate
        long ns = (end.tv_nsec - start.tv_nsec);

        fprintf(stdout, "Latency: %ld ns/op \n", ns/NUM_ITEMS);
       
    }  
}
