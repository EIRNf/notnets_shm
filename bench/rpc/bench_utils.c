#include <stdio.h>
#include "../../src/rpc.h"
#include  <time.h>
#include <pthread.h>
#include <assert.h>
#include <stdatomic.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>


#define NUM_ITEMS 10000000
#define MESSAGE_SIZE 4 //Int

#ifndef MAX_CLIENTS
#define MAX_CLIENTS 20
#endif 



atomic_bool run_flag = false; //control execution

struct connection_args {
    int client_id;
    struct timespec start;
    struct timespec end;
};

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

void pthread_client_rtt_post_connect(queue_pair* qp, struct connection_args *args){

    int *buf = malloc(MESSAGE_SIZE);
    int buf_size = MESSAGE_SIZE;

    int* pop_buf = malloc(MESSAGE_SIZE);
    int pop_buf_size = MESSAGE_SIZE;


    //hold until flag is set to true
    while(!run_flag);

    clock_gettime(CLOCK_MONOTONIC, &args->start);
    for(int i=1;i<NUM_ITEMS;i++) {
        *buf = i;
        client_send_rpc(qp, buf, buf_size);
        
        //Request sent, read for response

        client_receive_buf(qp, pop_buf, pop_buf_size);

        assert(*pop_buf == *buf);
    }
    clock_gettime(CLOCK_MONOTONIC, &args->end);
    free(pop_buf);
    free(buf);
    free(qp);
}
 
void *pthread_server_rtt(void* s_qp){

    queue_pair* qp = (queue_pair*) s_qp;

    int* buf = malloc(MESSAGE_SIZE);
    int buf_size = MESSAGE_SIZE;

    int* pop_buf = malloc(MESSAGE_SIZE);
    int pop_buf_size = MESSAGE_SIZE;

    //hold until flag is set to true
    while(!run_flag);

    for(int i=1;i<NUM_ITEMS;i++) {

        server_receive_buf(qp, pop_buf, pop_buf_size);

        //Business logic or something
        *buf = *pop_buf;

        server_send_rpc(qp,buf,buf_size);
     }

    free(pop_buf);
    free(buf);
    free(qp);
    pthread_exit(0);
}

void* pthread_connect_client(void* arg){
    struct connection_args *args = (struct connection_args*) arg;

    char name[32];

    sprintf(name, "%d", args->client_id);

    while(!run_flag);

    clock_gettime(CLOCK_MONOTONIC, &args->start);
    queue_pair* c_qp = client_open(name,
                                "test_server_addr",
                                 sizeof(int));
    while (c_qp == NULL) {
        c_qp = client_open(name,
                             "test_server_addr",
                             sizeof(int));
        }
    
    clock_gettime(CLOCK_MONOTONIC, &args->end);

    // No connection was achieved :()
    if (c_qp == NULL){
        fprintf(stdout, "Missed Connection: %d \n", args->client_id);
    }
    if (c_qp->request_shmaddr == NULL || c_qp->response_shmaddr == NULL ){
        fprintf(stdout, "Repeat Connection: %d \n", (int)hash((unsigned char*)name));
        fprintf(stdout, "Repeat Connection: %d \n", args->client_id);
        fprintf(stdout, "Client Id: %d \n", c_qp->client_id);
        fprintf(stdout, "Request_Queue: %p \n", c_qp->request_shmaddr);
        fprintf(stdout, "Response_Queue: %p \n", c_qp->response_shmaddr);

    }
    free(c_qp);
    pthread_exit(0);
}

void* pthread_connect_measure_rtt(void* arg){
    struct connection_args *args = (struct connection_args*) arg;

    char name[32];

    sprintf(name, "%d", args->client_id);


    // clock_gettime(CLOCK_MONOTONIC, &args->start);
    queue_pair* c_qp = client_open(name,
                                "test_server_addr",
                                 sizeof(int));
    while (c_qp == NULL) {
        c_qp = client_open(name,
                             "test_server_addr",
                             sizeof(int));
        }

    while(!run_flag);
    
    // clock_gettime(CLOCK_MONOTONIC, &args->end);
      // No connection was achieved :()
    if (c_qp == NULL){
        fprintf(stdout, "Missed Connection: %d \n", args->client_id);
    }

    if (c_qp->request_shmaddr == NULL || c_qp->response_shmaddr == NULL ){
        fprintf(stdout, "Repeat Connection: %d \n", (int)hash((unsigned char*)name));
        fprintf(stdout, "Client Id: %d \n", c_qp->client_id);
        fprintf(stdout, "Request_Queue: %p \n", c_qp->request_shmaddr);
        fprintf(stdout, "Response_Queue: %p \n", c_qp->response_shmaddr);

    }
    pthread_client_rtt_post_connect(c_qp, args);
    pthread_exit(0);
}

void* pthread_measure_connect_and_rtt(void* arg){
    struct connection_args *args = (struct connection_args*) arg;

    char name[32];

    sprintf(name, "%d", args->client_id);

    while(!run_flag);

    clock_gettime(CLOCK_MONOTONIC, &args->start);
    queue_pair* c_qp = client_open(name,
                                "test_server_addr",
                                 sizeof(int));
    while (c_qp == NULL) {
        c_qp = client_open(name,
                             "test_server_addr",
                             sizeof(int));
        }
    
      // No connection was achieved :()
    if (c_qp == NULL){
        fprintf(stdout, "Missed Connection: %d \n", args->client_id);
    }

    if (c_qp->request_shmaddr == NULL || c_qp->response_shmaddr == NULL ){
        fprintf(stdout, "Repeat Connection: %d \n", (int)hash((unsigned char*)name));
        fprintf(stdout, "Client Id: %d \n", c_qp->client_id);
        fprintf(stdout, "Request_Queue: %p \n", c_qp->request_shmaddr);
        fprintf(stdout, "Response_Queue: %p \n", c_qp->response_shmaddr);

    }


    int *buf = malloc(MESSAGE_SIZE);
    int buf_size = MESSAGE_SIZE;

    int* pop_buf = malloc(MESSAGE_SIZE);
    int pop_buf_size = MESSAGE_SIZE;

    //hold until flag is set to true
    while(!run_flag);

    for(int i=1;i<NUM_ITEMS;i++) {
        *buf = i;
        client_send_rpc(c_qp, buf, buf_size);
        
        //Request sent, read for response

        client_receive_buf(c_qp, pop_buf, pop_buf_size);

        assert(*pop_buf == *buf);
    }

    clock_gettime(CLOCK_MONOTONIC, &args->end);

    // int err = client_close(name, "test_server_addr");
    // if (err != 0){
    //     fprintf(stdout, "Close Error: %d \n", err);
    // }



    free(pop_buf);
    free(buf);
    free(c_qp);
    
    pthread_exit(0);
}

void* pthread_measure_connect_and_rtt_and_disconnect(void* arg){
    struct connection_args *args = (struct connection_args*) arg;

    char name[32];

    sprintf(name, "%d", args->client_id);

    while(!run_flag);

    clock_gettime(CLOCK_MONOTONIC, &args->start);
    queue_pair* c_qp = client_open(name,
                                "test_server_addr",
                                 sizeof(int));
    while (c_qp == NULL) {
        c_qp = client_open(name,
                             "test_server_addr",
                             sizeof(int));
        }
    
      // No connection was achieved :()
    if (c_qp == NULL){
        fprintf(stdout, "Missed Connection: %d \n", args->client_id);
    }

    if (c_qp->request_shmaddr == NULL || c_qp->response_shmaddr == NULL ){
        fprintf(stdout, "Repeat Connection: %d \n", (int)hash((unsigned char*)name));
        fprintf(stdout, "Client Id: %d \n", c_qp->client_id);
        fprintf(stdout, "Request_Queue: %p \n", c_qp->request_shmaddr);
        fprintf(stdout, "Response_Queue: %p \n", c_qp->response_shmaddr);

    }


    int *buf = malloc(MESSAGE_SIZE);
    int buf_size = MESSAGE_SIZE;

    int* pop_buf = malloc(MESSAGE_SIZE);
    int pop_buf_size = MESSAGE_SIZE;

    //hold until flag is set to true
    while(!run_flag);

    for(int i=1;i<NUM_ITEMS;i++) {
        *buf = i;
        client_send_rpc(c_qp, buf, buf_size);
        
        //Request sent, read for response

        client_receive_buf(c_qp, pop_buf, pop_buf_size);

        assert(*pop_buf == *buf);
    }

    int err = client_close(name, "test_server_addr");
    if (err != 0){
        fprintf(stdout, "Close Error: %d \n", err);
    }

    clock_gettime(CLOCK_MONOTONIC, &args->end);


    free(pop_buf);
    free(buf);
    free(c_qp);
    
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
