#include <stdio.h>
#include  <time.h>
#include <pthread.h>
#include <assert.h>
#include <stdatomic.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>
#include <stdbool.h>

#include <sys/resource.h>
#include <sys/time.h>

#include "tcp_bench_utils.h"


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
    // free(file_name);
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

void print_num_clients(){
     fprintf(stdout, "%d\t", MAX_CLIENTS);   
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

void save_single_rtt_stats_as_csv(char* name,struct connection_args *args) {
    char* file_name;
    asprintf(&file_name, "%s_bench.csv", name);

    // Open a file to write the CSV data
    FILE *fp = fopen(file_name, "w");
    if (fp == NULL) {
        perror("Failed to open file");
        return;
    }


        // Write the CSV header
    fprintf(fp, "Name,ItemsConsumed,ns/op,ops/ms\n");


    //nanosecond ns 1.0e-09
    //microsecond us 1.0e-06
    //millisecond ms 1.0e-03
    struct timespec start = args->start;
    struct timespec end = args->end;

    double total_sec  = end.tv_sec - start.tv_sec;
    double total_nsec = end.tv_nsec - start.tv_nsec;


    // Calculate the total time in microseconds and milliseconds
    double total_us = total_sec * 1.0e+06;
    total_us += total_nsec / 1.0e+03;

    double total_ms = total_sec * 1.0e+03;
    total_ms += total_nsec / 1.0e+06;


    fprintf(fp, "%s,%d,%f,%f\n",
            name,
            NUM_ITEMS,
            (total_us * 1.0e+03)/NUM_ITEMS,
            NUM_ITEMS/(total_ms));


    // Close the file
    fclose(fp);
    // free(file_name);
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
