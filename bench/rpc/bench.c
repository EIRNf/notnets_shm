#define _GNU_SOURCE
#include <stdio.h>
#include "../../src/rpc.h"
#include  <time.h>
#include <pthread.h>
#include <assert.h>
#include <stdatomic.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>



#define NUM_ITEMS 1000000
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


void bench_report_rtt_stats(struct connection_args *args) {
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

void client(queue_pair* qp, struct connection_args *args){

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
 
void *server(void* s_qp){

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

void rtt_test(){ 
    fprintf(stdout, "\nrtt-notnets/spsc/\n");
    // pthread_t producer;
    pthread_t consumer;

    server_context* sc = register_server("test_server_addr");

    queue_pair* c_qp = client_open("test_client_addr",
                                "test_server_addr",
                                sizeof(int));

    struct connection_args *args = malloc(sizeof(struct connection_args));

    queue_pair* s_qp;
    while ((s_qp = accept(sc)) == NULL);

    //producer thread
    //pthread_create(&producer, NULL, client, c_qp);
    pthread_create(&consumer, NULL, server, s_qp);

    //Actual measuring
    run_flag = true;
    client(c_qp, args);

    pthread_join(consumer,NULL);    

    fprintf(stdout, "Num Items: %d  \n", NUM_ITEMS);
    fprintf(stdout, "Message Size: %d  \n", MESSAGE_SIZE);
    bench_report_rtt_stats(args);
    free(args);
    shutdown(sc);
    run_flag = false;
}

void bench_report_connection_stats(struct connection_args *args){
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

void* client_connection(void* arg){
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

// Evaluate how long a single non contested connection takes in notnets
void single_connection_test(){ 
    fprintf(stdout, "\nnotnets/single-connection/\n");

    //Current 
    server_context* sc = register_server("test_server_addr");

    pthread_t client;

    struct connection_args *args = malloc(sizeof(struct connection_args));
    args->client_id = 1;
    atomic_thread_fence(memory_order_seq_cst);


    pthread_create(&client, NULL, client_connection,args);

    queue_pair* s_qp;
    //Synchronization variable to begin attempting to open connections
    run_flag = true;
    while ((s_qp = accept(sc)) == NULL);


    pthread_join(client,NULL);    

    fprintf(stdout, "Num Clients: %d \n", 1);
    fprintf(stdout, "Available Slots: %d \n", SLOT_NUM);
    bench_report_connection_stats(args);
    free(s_qp);
    shutdown(sc);
    free(args);
    run_flag = false;
}

int cmpfunc(const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}


// Evaluate how long connections take in Notnets
// Instantiate multiple client threads to opening new connection until max is reached.
// Server simply accepts and loops over avalable queue pairs until it can read data from all those connected.
void connection_stress_test(){ 
    fprintf(stdout, "\nnotnets/multi-connection/\n");

    // pthread_t producer;
    pthread_t *clients[MAX_CLIENTS] = {};

    //Current 
    server_context* sc = register_server("test_server_addr");

    struct connection_args *args[MAX_CLIENTS] = {};

    //Create client threads, will maintain a holding pattern until 
    // flag is flipped
    struct timespec nonce;
    for(int i = 0; i < MAX_CLIENTS; i++){
        struct connection_args *client_args = malloc(sizeof(struct connection_args));
        args[i] = client_args;
        pthread_t *new_client = malloc(sizeof(pthread_t));
        clients[i] = new_client;
        if (! timespec_get(&nonce, TIME_UTC)){
            // return -1;
        }
        args[i]->client_id =  i + nonce.tv_nsec;
        atomic_thread_fence(memory_order_seq_cst);
        pthread_create(clients[i], NULL, client_connection, args[i]);
    }
    

    queue_pair* s_qp;

    //Synchronization variable to begin attempting to open connections
    run_flag = true;
    
   
    int client_list[MAX_CLIENTS];
    queue_pair *client_queues[MAX_CLIENTS];
    int *item;
    int key;
    for (int i=0;i < MAX_CLIENTS;){
        s_qp = accept(sc);
        //Check for repeat entries!!!!!!!!!
        if (s_qp != NULL){ //Ocasional Leak here, discover why TODO
            
            key = s_qp->client_id;
            item = (int*) bsearch(&key, client_list,MAX_CLIENTS,sizeof(int),cmpfunc);
            //Client already accepted, reject
            if(item != NULL){
                free(s_qp);
                continue;
            }

            if(s_qp->request_shmaddr != NULL){
                client_list[i] = s_qp->client_id;
                client_queues[i] = s_qp;
                i++;
            }
            // free(s_qp);
        }
    }

    atomic_thread_fence(memory_order_seq_cst);
    //Join threads
    for(int i =0; i < MAX_CLIENTS; i++){
        pthread_join(*clients[i],NULL);
        // bench_report_connection_stats(args[i]);
        //Free heap allocated data
        free(client_queues[i]);
        // free(args[i]);
        free(clients[i]);
    }   

    //PRINT AVERGAGE
    //nanosecond ns 1.0e-09
    //microsecond us 1.0e-06
    //millisecond ms 1.0e-03
    
    long total_ms = 0.0;
    long total_ns = 0.0;
    for(int i = 0; i < MAX_CLIENTS; i++){
        struct timespec start = args[i]->start;
        struct timespec end = args[i]->end;
        if ((end.tv_sec - start.tv_sec) > 0 ){ //we have atleast 1 sec

            long ms = (end.tv_sec - start.tv_sec) * 1.0e+03;
            ms += (end.tv_nsec - start.tv_nsec) / 1.0e+06;

            total_ms += ms;
        } else {//nanosecond scale, probably not very accurate
            long ns = 0.0;
            ns =  (end.tv_sec - start.tv_sec) * 1.0e+09;
            ns += (end.tv_nsec - start.tv_nsec);    

            total_ns += ns ;
        }
    }


    fprintf(stdout, "Num Clients: %d  \n", MAX_CLIENTS);
    fprintf(stdout, "Available Slots: %d  \n", SLOT_NUM);
    fprintf(stdout, "Average Connection Latency: %ld ms/op \n", (total_ms/MAX_CLIENTS));
    fprintf(stdout, "Average Connection Latency: %ld ns/op  \n", total_ns/MAX_CLIENTS);



    for (int i=0; i< MAX_CLIENTS; i++){
        free(args[i]);
    }
    shutdown(sc);
    run_flag = false;
}


void bench_report_rtt_connection_stats(struct connection_args *args){
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

void post_connect_client(struct connection_args *args, queue_pair* qp){
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

void* client_connection_rtt(void* arg){
    struct connection_args *args = (struct connection_args*) arg;

    char name[32];

    sprintf(name, "%d", args->client_id);

    while(!run_flag);

    // clock_gettime(CLOCK_MONOTONIC, &args->start);
    queue_pair* c_qp = client_open(name,
                                "test_server_addr",
                                 sizeof(int));
    while (c_qp == NULL) {
        c_qp = client_open(name,
                             "test_server_addr",
                             sizeof(int));
        }
    
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
    post_connect_client(args, c_qp);
    pthread_exit(0);
}

//Evaluate latency and throughput of connected clients
//while new clients continue to connect.
void rtt_during_connection_test(){ 
    fprintf(stdout, "\nnotnets/rtt-during-multi-connection/\n");

    // pthread_t producer;
    pthread_t *clients[MAX_CLIENTS] = {};
    pthread_t *handlers[MAX_CLIENTS] = {};


    //Current 
    server_context* sc = register_server("test_server_addr");

    struct connection_args *args[MAX_CLIENTS] = {};

    //Create client threads, will maintain a holding pattern until 
    // flag is flipped
    struct timespec nonce;
    for(int i = 0; i < MAX_CLIENTS; i++){
        struct connection_args *client_args = malloc(sizeof(struct connection_args));
        args[i] = client_args;
        pthread_t *new_client = malloc(sizeof(pthread_t));
        clients[i] = new_client;
        if (! timespec_get(&nonce, TIME_UTC)){
            // return -1;
        }
        args[i]->client_id = i + nonce.tv_nsec;
        pthread_create(clients[i], NULL, client_connection_rtt, args[i]);
    }
    

    queue_pair* s_qp;

    //Synchronization variable to begin attempting to open connections
    run_flag = true;
   
    int client_list[MAX_CLIENTS];
    int *item;
    int key;
    for(int i=0;i < MAX_CLIENTS;){
        s_qp = accept(sc);

        if (s_qp != NULL){ //Ocasional Leak here, discover why TODO

            //Check for repeat entries!!!!!!!!!
            key = s_qp->client_id;
            item = (int*) bsearch(&key, client_list,MAX_CLIENTS,sizeof(int),cmpfunc);
            
            //Client already accepted, reject
            if(item != NULL){
                free(s_qp);
                continue;
            }

            if (s_qp->request_shmaddr != NULL){
                client_list[i] = s_qp->client_id;
                pthread_t *new_handler = malloc(sizeof(pthread_t));
                handlers[i] = new_handler;
                atomic_thread_fence(memory_order_seq_cst);
                pthread_create(handlers[i], NULL, server, s_qp);
                i++;
            }
            // free(s_qp);
        }
    }

    // atomic_thread_fence(memory_order_seq_cst);
    //Join threads
    for(int i =0; i < MAX_CLIENTS; i++){
        pthread_join(*handlers[i],NULL);
        pthread_join(*clients[i],NULL);
        // bench_report_rtt_connection_stats(args[i]);
        //Free heap allocated data
        // free(client_queues[i]);
        free(handlers[i]);
        // free(args[i]);
        free(clients[i]);
    }

    //PRINT AVERGAGE
    //nanosecond ns 1.0e-09
    //microsecond us 1.0e-06
    //millisecond ms 1.0e-03
    
    long total_ns = 0.0;
    for(int i =0; i < MAX_CLIENTS; i++){
        struct timespec start = args[i]->start;
        struct timespec end = args[i]->end;


        if ((end.tv_sec - start.tv_sec) > 0 ){ //we have atleast 1 sec

            long ms = (end.tv_sec - start.tv_sec) * 1.0e+03;
            ms += (end.tv_nsec - start.tv_nsec) / 1.0e+06;

            total_ns += (ms * 1.0e+06);
        } else {//nanosecond scale, probably not very accurate
        
            long ns = 0.0;
            ns =  (end.tv_sec - start.tv_sec) * 1.0e+09;
            ns += (end.tv_nsec - start.tv_nsec);    

            total_ns += ns;
        }
    }

    long average_ns = total_ns/MAX_CLIENTS;

    fprintf(stdout, "Num Clients: %d  \n", MAX_CLIENTS);
    fprintf(stdout, "Available Slots: %d  \n", SLOT_NUM);
    fprintf(stdout, "Num Items: %d  \n", NUM_ITEMS);
    fprintf(stdout, "Message Size: %d  \n", MESSAGE_SIZE);
    fprintf(stdout, "Average Latency: %ld ns/op \n", (average_ns/NUM_ITEMS));
    fprintf(stdout, "Average Throughput: %f ops/ms \n", (NUM_ITEMS)/(average_ns / 1.0e+06));
    fprintf(stdout, "Total Throughput: %f ops/ms \n", (NUM_ITEMS * MAX_CLIENTS)/(average_ns / 1.0e+06));




    for (int i=0; i< MAX_CLIENTS; i++){
        free(args[i]);
    }
    shutdown(sc);
    run_flag = false;

}



void single_rtt_during_connection_test(){ 
    fprintf(stdout, "\nnotnets/single-rtt-during-multi-connection/\n");

    // pthread_t producer;
    pthread_t *clients[MAX_CLIENTS] = {};
    pthread_t *handler;


    //Current 
    server_context* sc = register_server("test_server_addr");

    struct connection_args *args[MAX_CLIENTS] = {};

    int rtt_client = MAX_CLIENTS-1;

    if (rtt_client == 0) {
        rtt_client = 1;
    }

    //Create client threads, will maintain a holding pattern until flag is flipped
    //Handle single writing client
    struct connection_args *client_args = malloc(sizeof(struct connection_args));
    args[MAX_CLIENTS-1] = client_args;
    args[MAX_CLIENTS-1]->client_id = MAX_CLIENTS-1;
    pthread_t *new_client = malloc(sizeof(pthread_t));
    clients[MAX_CLIENTS-1] = new_client;
    pthread_create(clients[MAX_CLIENTS-1], NULL, client_connection_rtt, args[MAX_CLIENTS-1]);

    //Handle other non messaging Clients
    struct timespec nonce;
    for(int i = 0; i < MAX_CLIENTS-1; i++){
        struct connection_args *client_args = malloc(sizeof(struct connection_args));
        args[i] = client_args;
        pthread_t *new_client = malloc(sizeof(pthread_t));
        clients[i] = new_client;
        if (! timespec_get(&nonce, TIME_UTC)){
            // return -1;
        }
        args[i]->client_id = i + nonce.tv_nsec;
        pthread_create(clients[i], NULL, client_connection, args[i]);
    }

    queue_pair* s_qp;

    //Synchronization variable to begin attempting to open connections
    run_flag = true;

    //Hash to identify client id

    char name[32];
    sprintf(name, "%d", MAX_CLIENTS-1);
    int temp_client_id = (int)hash((unsigned char*) name);
   
    int client_list[MAX_CLIENTS];
    int *item;
    int key;
    for(int i=0;i < MAX_CLIENTS;){
        s_qp = accept(sc);

 

        if (s_qp != NULL){ //Ocasional Leak here, discover why TODO

            //Check for repeat entries!!!!!!!!!
            key = s_qp->client_id;
            item = (int*) bsearch(&key, client_list,MAX_CLIENTS,sizeof(int),cmpfunc);
            
            //Client already accepted, reject
            if(item != NULL){
                free(s_qp);
                continue;
            }

            if (s_qp->request_shmaddr != NULL){

                //Found RTT client handle
                if (s_qp->client_id == temp_client_id){
                    client_list[i] = s_qp->client_id;
                    handler = malloc(sizeof(pthread_t));
                    atomic_thread_fence(memory_order_seq_cst);
                    pthread_create(handler, NULL, server, s_qp);
                    i++;
                }

                client_list[i] = s_qp->client_id;
                atomic_thread_fence(memory_order_seq_cst);
                i++;
            }
        }
    }

    // atomic_thread_fence(memory_order_seq_cst);
    //Join threads
    pthread_join(*handler,NULL);
    free(handler);
    for(int i =0; i < MAX_CLIENTS; i++){
        pthread_join(*clients[i],NULL);
        free(clients[i]);
    }

    //PRINT AVERGAGE
    //nanosecond ns 1.0e-09
    //microsecond us 1.0e-06
    //millisecond ms 1.0e-03

    //Handle RTT timing
    bench_report_rtt_stats( args[MAX_CLIENTS-1]);
   
    

    //Handle connection timing
    long total_ns = 0.0;
    for(int i =0; i < MAX_CLIENTS-1; i++){
        struct timespec start = args[i]->start;
        struct timespec end = args[i]->end;


        if ((end.tv_sec - start.tv_sec) > 0 ){ //we have atleast 1 sec

            long ms = (end.tv_sec - start.tv_sec) * 1.0e+03;
            ms += (end.tv_nsec - start.tv_nsec) / 1.0e+06;

            total_ns += (ms * 1.0e+06);
        } else {//nanosecond scale, probably not very accurate
        
            long ns = 0.0;
            ns =  (end.tv_sec - start.tv_sec) * 1.0e+09;
            ns += (end.tv_nsec - start.tv_nsec);    

            total_ns += ns;
        }
    }

    long average_ns = total_ns/(rtt_client);

    fprintf(stdout, "Num RTT Clients: %d  \n", 1);
    fprintf(stdout, "Num Connect Clients: %d  \n", rtt_client);
    fprintf(stdout, "Available Slots: %d  \n", SLOT_NUM);
    fprintf(stdout, "Num Items: %d  \n", NUM_ITEMS);
    fprintf(stdout, "Message Size: %d  \n", MESSAGE_SIZE);
    fprintf(stdout, "Average Connect Latency: %ld ns/op \n", average_ns);


    for (int i=0; i< MAX_CLIENTS; i++){
        free(args[i]);
    }
    shutdown(sc);

    run_flag = false;

}


void bench_run_all(void){
    //Connection Test

    //Echo application, capture RTT latency and throughput
    //TODO: Modify to pass arguments
    // rtt_test();
    // single_connection_test();
    // connection_stress_test();
    rtt_during_connection_test();
    // single_rtt_during_connection_test();
    //TODO: Make connection/desconection stress test

    //TODO: "Tune" Server Overhead


}