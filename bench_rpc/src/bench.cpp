#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>

#include "rpc.h"
#include "bench.h"
#include "bench_utils.h"



//initialize run flag
atomic_bool run_flag = false; //control execution

//Evaluate queue latency after connection has already been made
void single_rtt_test(){ 
    fprintf(stdout, "\nnotnets/%s/\n", __func__);
    // pthread_t producer;
    pthread_t consumer;

    server_context* sc = register_server((char*)"single_rtt_test_server", PTHREAD);

    queue_ctx* c_qp = client_open((char*)"single_rtt_test_client",
                                (char*)"single_rtt_test_server",
                                sizeof(int),
                                POLL);

    struct connection_args *args = (struct connection_args *)malloc(sizeof(struct connection_args));

    queue_ctx* s_qp;
    while ((s_qp = accept(sc)) == NULL);

    //producer thread
    //pthread_create(&producer, NULL, client, c_qp);
    pthread_create(&consumer, NULL, pthread_server_rtt, s_qp);

    //Actual measuring
    run_flag = true;
    pthread_client_rtt_post_connect(c_qp, args);

    pthread_join(consumer,NULL);    

    fprintf(stdout, "Num Items: %d  \n", NUM_ITEMS);
    fprintf(stdout, "Message Size: %d  \n", MESSAGE_SIZE);
    report_single_rtt_stats(args);
    free(args);
    shutdown(sc);
    run_flag = false;
}

// Evaluate how long a single non contested connection takes in notnets
void single_connection_test(){ 
    fprintf(stdout, "\nnotnets/%s/\n", __func__);

    //Current 
    server_context* sc = register_server((char*)"test_server_addr", PTHREAD);

    pthread_t client;

    struct connection_args *args = (struct connection_args *)malloc(sizeof(struct connection_args));
    args->client_id = 1;
    atomic_thread_fence(memory_order_seq_cst);


    pthread_create(&client, NULL, pthread_connect_client,args);

    queue_ctx* s_qp;
    //Synchronization variable to begin attempting to open connections
    run_flag = true;
    while ((s_qp = accept(sc)) == NULL);


    pthread_join(client,NULL);    

    fprintf(stdout, "Num Clients: %d \n", 1);
    fprintf(stdout, "Available Slots: %d \n", SLOT_NUM);
    report_connection_stats(args);
    free_queue_ctx(s_qp);
    shutdown(sc);
    free(args);
    run_flag = false;
}

// Evaluate how long connections take in Notnets
// Instantiate multiple client threads to opening new connection until max is reached.
// Server simply accepts and loops over avalable queue pairs until it can read data from all those connected.
void connection_stress_test(lock_type lock_type, int num_clients){ 
    // fprintf(stdout, "\nnotnets/%s/\n", __func__);

    pthread_t *clients[num_clients];
    memset(clients, 0, sizeof(clients));

    //Current 
    server_context* sc = register_server((char*)"test_server_addr", BURNS_AND_LYNCH );

    struct connection_args *args[num_clients];
    memset(args, 0, sizeof(args));

    //Create client threads, will maintain a holding pattern until 
    // flag is flipped
    struct timespec nonce;
    for(int i = 0; i < num_clients; i++){
      struct connection_args *client_args = (struct connection_args *)malloc(sizeof(struct connection_args));
        args[i] = client_args;
        pthread_t *new_client = (pthread_t*)malloc(sizeof(pthread_t));
        clients[i] = new_client;
        if (! timespec_get(&nonce, TIME_UTC)){
            // return -1;
        }
        args[i]->client_id =  i + nonce.tv_nsec;
        atomic_thread_fence(memory_order_seq_cst);
        pthread_create(clients[i], NULL, pthread_connect_client, args[i]);
    }
    
    //Ensure everything is build and ready to go    
    queue_ctx* s_qp;
    

    //Synchronization variable to begin attempting to open connections
    run_flag = true;
    
   
    int client_list[num_clients];
    memset(client_list, 0, sizeof(client_list));
    queue_ctx *client_queues[num_clients];
    memset(client_queues, 0, sizeof(client_queues));
    int *item;
    int key;

    for (int i=0;i < num_clients;){
        s_qp = accept(sc);
        //Check for repeat entries!!!!!!!!!
        if (s_qp != NULL){ //Ocasional Leak here, discover why TODO
            
            key = s_qp->queues->client_id;
            item = (int*) bsearch(&key, client_list,num_clients,sizeof(int),cmpfunc);
            //Client already accepted, reject
            if(item != NULL){
                free(s_qp);
                continue;
            }

            if(s_qp->queues->request_shmaddr != NULL){
                client_list[i] = s_qp->queues->client_id;
                client_queues[i] = s_qp;
                i++;
            }
            // free(s_qp);
        }
    }

    atomic_thread_fence(memory_order_seq_cst);
    //Join threads
    for(int i =0; i < num_clients; i++){
        pthread_join(*clients[i],NULL);
        // report_connection_stats(args[i]);
        //Free heap allocated data
        free_queue_ctx(client_queues[i]);
        // free(args[i]);
        free(clients[i]);
    }   


    //PRINT AVERGAGE
    //nanosecond ns 1.0e-09
    //microsecond us 1.0e-06
    //millisecond ms 1.0e-03
    long total_ms = 0.0;
    long total_ns = 0.0;
    for(int i = 0; i < num_clients; i++){
        struct timespec start = args[i]->start;
        struct timespec end = args[i]->end;
        if ((end.tv_sec - start.tv_sec) > 0 ){ //we have atleast 1 sec
            long ms = 0.0;
            ms = (end.tv_sec - start.tv_sec) * 1.0e+03;
            ms += (end.tv_nsec - start.tv_nsec) / 1.0e+06;

            total_ms += ms;
        } else {//nanosecond scale, probably not very accurate
            long ns = 0.0;
            ns =  (end.tv_sec - start.tv_sec) * 1.0e+09;
            ns += (end.tv_nsec - start.tv_nsec);    

            total_ns += ns;
        }
    }

    //Get total in ms 
    long final_total_ms = total_ms + total_ns / 1.0e+06;
    //Get total in ns
    long final_total_ns = total_ns + total_ms * 1.0e+06;
    
    long average = final_total_ns/num_clients;

     // Calculate the 99th percentile of the args seconds argument
    long *latencies = (long *)malloc(num_clients * sizeof(long));
    for (int i = 0; i < num_clients; i++) {
        struct timespec start = args[i]->start;
        struct timespec end = args[i]->end;
        long ns = (end.tv_sec - start.tv_sec) * 1.0e+09 + (end.tv_nsec - start.tv_nsec);
        latencies[i] = ns;
    }

    // Sort the latencies array
    qsort(latencies, num_clients, sizeof(long), cmpfunc);

    // Calculate the 99th percentile
    int index_99th = (int)(0.99 * num_clients);
    long percentile_99th = latencies[index_99th];

    // Calculate start and end times of the entire test to calculate throughput
    struct timespec earliest_start = args[0]->start;
    struct timespec latest_end = args[0]->end;
    for (int i = 1; i < num_clients; i++) {
        
        if ((args[i]->start.tv_sec < earliest_start.tv_sec) ||
            (args[i]->start.tv_sec == earliest_start.tv_sec && args[i]->start.tv_nsec < earliest_start.tv_nsec)) {
            earliest_start = args[i]->start;
        }
        if ((args[i]->end.tv_sec > latest_end.tv_sec) ||
            (args[i]->end.tv_sec == latest_end.tv_sec && args[i]->end.tv_nsec > latest_end.tv_nsec)) {
            latest_end = args[i]->end;
        }
    }

    long duration_ns = (latest_end.tv_sec - earliest_start.tv_sec) * 1.0e+09 + (latest_end.tv_nsec - earliest_start.tv_nsec);


    long throughput_ops_s= num_clients * 1.0e+09 / duration_ns;


    fprintf(stdout, "%d,", lock_type);
    fprintf(stdout, "%d,", num_clients);
    fprintf(stdout, "%ld,", final_total_ms);
    fprintf(stdout, "%ld,", final_total_ns);
    fprintf(stdout,"%ld,", average);
    fprintf(stdout,"%ld,", percentile_99th);
    fprintf(stdout,"%ld ", throughput_ops_s);
    fprintf(stdout,"\n");

    free(latencies);

    // fprintf(stdout, "Lock Type: %d  \n", lock_type);
    // fprintf(stdout, "Num Clients: %d  \n", num_clients);
    // fprintf(stdout, "Available Slots: %d  \n", SLOT_NUM);
    // fprintf(stdout, "Total Connection Time: %ld ms/op \n", total_ms);
    // fprintf(stdout, "Total Connection Time: %ld ns/op \n", total_ns );
    // fprintf(stdout, "Average Connection Latency: %ld ms/op \n", (total_ms/num_clients));
    // fprintf(stdout, "Average Connection Latency: %ld ns/op  \n", total_ns/num_clients);



    for (int i=0; i< num_clients; i++){
        free(args[i]);
    }
    shutdown(sc);
    run_flag = false;
}

void single_rtt_during_connection_test(){ 
    fprintf(stdout, "\nnotnets/%s/\n", __func__);

    // pthread_t producer;
    pthread_t *clients[MAX_CLIENTS] = {};
    pthread_t *handler;


    //Current 
    server_context* sc = register_server((char*)"test_server_addr", PTHREAD);

    struct connection_args *args[MAX_CLIENTS] = {};

    int rtt_client = MAX_CLIENTS-1;

    if (rtt_client == 0) {
        rtt_client = 1;
    }

    //Create client threads, will maintain a holding pattern until flag is flipped
    //Handle single writing client
    struct connection_args *client_args = (struct connection_args *)malloc(sizeof(struct connection_args));
    args[MAX_CLIENTS-1] = client_args;
    args[MAX_CLIENTS-1]->client_id = MAX_CLIENTS-1;
    pthread_t *new_client = (pthread_t*)malloc(sizeof(pthread_t));
    clients[MAX_CLIENTS-1] = new_client;
    pthread_create(clients[MAX_CLIENTS-1], NULL, pthread_connect_measure_rtt, args[MAX_CLIENTS-1]);

    //Handle other non messaging Clients
    struct timespec nonce;
    for(int i = 0; i < MAX_CLIENTS-1; i++){
      struct connection_args *client_args = (struct connection_args *)malloc(sizeof(struct connection_args));
        args[i] = client_args;
        pthread_t *new_client = (pthread_t*)malloc(sizeof(pthread_t));
        clients[i] = new_client;
        if (! timespec_get(&nonce, TIME_UTC)){
            // return -1;
        }
        args[i]->client_id = i + nonce.tv_nsec;
        pthread_create(clients[i], NULL, pthread_connect_client, args[i]);
    }

    queue_ctx* s_qp;

    //Synchronization variable to begin attempting to open connections
    run_flag = true;

    //Hash to identify client id

    char name[32];
    snprintf(name, 32, "%d", MAX_CLIENTS-1);
    int temp_client_id = (int)hash((unsigned char*) name);
   
    int client_list[MAX_CLIENTS];
    int *item;
    int key;
    for(int i=0;i < MAX_CLIENTS;){
        s_qp = accept(sc);

 

        if (s_qp != NULL){ //Ocasional Leak here, discover why TODO

            //Check for repeat entries!!!!!!!!!
            key = s_qp->queues->client_id;
            item = (int*) bsearch(&key, client_list,MAX_CLIENTS,sizeof(int),cmpfunc);
            
            //Client already accepted, reject
            if(item != NULL){
                free(s_qp);
                continue;
            }

            if (s_qp->queues->request_shmaddr != NULL){

                //Found RTT client handle
                if (s_qp->queues->client_id == temp_client_id){
                    client_list[i] = s_qp->queues->client_id;
                    handler = (pthread_t*)malloc(sizeof(pthread_t));
                    atomic_thread_fence(memory_order_seq_cst);
                    pthread_create(handler, NULL, pthread_server_rtt, s_qp);
                    i++;
                }

                client_list[i] = s_qp->queues->client_id;
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
    report_single_rtt_stats( args[MAX_CLIENTS-1]);
   
    

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

void rtt_steady_state_conn_test(){

    // pthread_t producer;
    pthread_t *clients[MAX_CLIENTS] = {};
    pthread_t *handlers[MAX_CLIENTS] = {};


    //Current 
    server_context* sc = register_server((char*)"test_server_addr", PTHREAD);

    struct connection_args *args[MAX_CLIENTS] = {};

    //Create client threads, will maintain a holding pattern until 
    // flag is flipped
    struct timespec nonce;
    for(int i = 0; i < MAX_CLIENTS; i++){
      struct connection_args *client_args = (struct connection_args *)malloc(sizeof(struct connection_args));
        args[i] = client_args;
        pthread_t *new_client = (pthread_t*)malloc(sizeof(pthread_t));
        clients[i] = new_client;
        if (! timespec_get(&nonce, TIME_UTC)){
            // return -1;
        }
        args[i]->client_id = i + nonce.tv_nsec;
        pthread_create(clients[i], NULL, pthread_connect_measure_rtt, args[i]);
    }
    

    queue_ctx* s_qp;

    int client_list[MAX_CLIENTS];
    int *item;
    int key;
    for(int i=0;i < MAX_CLIENTS;){
        s_qp = accept(sc);

        if (s_qp != NULL){ //Ocasional Leak here, discover why TODO

            //Check for repeat entries!!!!!!!!!
            key = s_qp->queues->client_id;
            item = (int*) bsearch(&key, client_list,MAX_CLIENTS,sizeof(int),cmpfunc);
            
            //Client already accepted, reject
            if(item != NULL){
                free(s_qp);
                continue;
            }

            if (s_qp->queues->request_shmaddr != NULL){
                client_list[i] = s_qp->queues->client_id;
                pthread_t *new_handler = (pthread_t*)malloc(sizeof(pthread_t));
                handlers[i] = new_handler;
                atomic_thread_fence(memory_order_seq_cst);
                pthread_create(handlers[i], NULL, pthread_server_rtt, s_qp);
                i++;
            }
            // free(s_qp);
        }
    }

    //Synchronization variable to begin attempting to open connections
    run_flag = true;
   
    //Join threads
    for(int i =0; i < MAX_CLIENTS; i++){
        pthread_join(*handlers[i],NULL);
        pthread_join(*clients[i],NULL);
        free(handlers[i]);
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


    // fprintf(stdout, "num_clients | coord_slots | rtt_num_items | msg_size | ns/op | avg_ops/ms | ops/ms | \n");
    fprintf(stdout, "\nnotnets/%s/\t", __func__);
    print_num_clients();
    print_coord_slots();
    print_rtt_num_items();
    print_msg_size();
    print_ns_op(average_ns);
    print_avg_ops_ms(average_ns);
    print_ops_ms(average_ns);

    //Free stats args
    for (int i=0; i< MAX_CLIENTS; i++){
        free(args[i]);
    }
    shutdown(sc);
    run_flag = false;

}

//Evaluate latency and throughput of connected clients
//while new clients continue to connect.
void rtt_during_connection_test(){ 

    // pthread_t producer;
    pthread_t *clients[MAX_CLIENTS] = {};
    pthread_t *handlers[MAX_CLIENTS] = {};


    //Current 
    server_context* sc = register_server((char*)"test_server_addr", PTHREAD);

    struct connection_args *args[MAX_CLIENTS] = {};

    //Create client threads, will maintain a holding pattern until 
    // flag is flipped
    struct timespec nonce;
    for(int i = 0; i < MAX_CLIENTS; i++){
      struct connection_args *client_args = (struct connection_args *)malloc(sizeof(struct connection_args));
        args[i] = client_args;
        pthread_t *new_client = (pthread_t*)malloc(sizeof(pthread_t));
        clients[i] = new_client;
        if (! timespec_get(&nonce, TIME_UTC)){
            // return -1;
        }
        args[i]->client_id = i + nonce.tv_nsec;
        pthread_create(clients[i], NULL, pthread_measure_connect_and_rtt, args[i]);
    }
    

    queue_ctx* s_qp;

    //Synchronization variable to begin attempting to open connections
    run_flag = true;
   
    int client_list[MAX_CLIENTS];
    int *item;
    int key;
    for(int i=0;i < MAX_CLIENTS;){
        s_qp = accept(sc);

        if (s_qp != NULL){ //Ocasional Leak here, discover why TODO

            //Check for repeat entries!!!!!!!!!
            key = s_qp->queues->client_id;
            item = (int*) bsearch(&key, client_list,MAX_CLIENTS,sizeof(int),cmpfunc);
            
            //Client already accepted, reject
            if(item != NULL){
                free(s_qp);
                continue;
            }

            if (s_qp->queues->request_shmaddr != NULL){
                client_list[i] = s_qp->queues->client_id;
                pthread_t *new_handler = (pthread_t*)malloc(sizeof(pthread_t));
                handlers[i] = new_handler;
                atomic_thread_fence(memory_order_seq_cst);
                pthread_create(handlers[i], NULL, pthread_server_rtt, s_qp);
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
        // report_rtt_stats_after_connect(args[i]);
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

    // fprintf(stdout, "num_clients | coord_slots | rtt_num_items | msg_size | ns/op | avg_ops/ms | ops/ms | \n");
    fprintf(stdout, "notnets/%s/\t", __func__);
    print_num_clients();
    print_coord_slots();
    print_rtt_num_items();
    print_msg_size();
    print_ns_op(average_ns);
    print_avg_ops_ms(average_ns);
    print_ops_ms(average_ns);
    fprintf(stdout, "\n");


    for (int i=0; i< MAX_CLIENTS; i++){
        free(args[i]);
    }
    shutdown(sc);
    run_flag = false;

}


// Connections are open and closed throughout the duration of the experiment.
void rtt_connect_disconnect_connection_test(){


    // pthread_t producer;
    pthread_t *clients[MAX_CLIENTS] = {};
    pthread_t *handlers[MAX_CLIENTS] = {};


    //Current 
    server_context* sc = register_server((char*)"test_server_addr", PTHREAD);

    struct connection_args *args[MAX_CLIENTS] = {};

    //Create client threads, will maintain a holding pattern until 
    // flag is flipped
    struct timespec nonce;
    for(int i = 0; i < MAX_CLIENTS; i++){
      struct connection_args *client_args = (struct connection_args *)malloc(sizeof(struct connection_args));
        args[i] = client_args;
        pthread_t *new_client = (pthread_t*)malloc(sizeof(pthread_t));
        clients[i] = new_client;
        if (! timespec_get(&nonce, TIME_UTC)){
            // return -1;
        }
        args[i]->client_id = i + nonce.tv_nsec;
        pthread_create(clients[i], NULL, pthread_measure_connect_and_rtt_and_disconnect, args[i]);
    }
    

    queue_ctx* s_qp;

    //Synchronization variable to begin attempting to open connections
    run_flag = true;
   
    int client_list[MAX_CLIENTS];
    int *item;
    int key;
    for(int i=0;i < MAX_CLIENTS;){
        s_qp = accept(sc);

        if (s_qp != NULL){ //Ocasional Leak here, discover why TODO

            //Check for repeat entries!!!!!!!!!
            key = s_qp->queues->client_id;
            item = (int*) bsearch(&key, client_list,MAX_CLIENTS,sizeof(int),cmpfunc);
            
            //Client already accepted, reject
            if(item != NULL){
                free(s_qp);
                continue;
            }

            if (s_qp->queues->request_shmaddr != NULL){
                client_list[i] = s_qp->queues->client_id;
                pthread_t *new_handler = (pthread_t*)malloc(sizeof(pthread_t));
                handlers[i] = new_handler;
                atomic_thread_fence(memory_order_seq_cst);
                pthread_create(handlers[i], NULL, pthread_server_rtt, s_qp);
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
        // report_rtt_stats_after_connect(args[i]);
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

    // fprintf(stdout, "num_clients | coord_slots | rtt_num_items | msg_size | ns/op | avg_ops/ms | ops/ms | \n");
    fprintf(stdout, "\nnotnets/%s/\t", __func__);
    print_num_clients();
    print_coord_slots();
    print_rtt_num_items();
    print_msg_size();
    print_ns_op(average_ns);
    print_avg_ops_ms(average_ns);
    print_ops_ms(average_ns);

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
    // single_rtt_test();
    // single_connection_test();

#ifdef __APPLE__
    //OSX does not support more than 8 shm segments per process 
    //and 32 shm segments in total. The following benchmarks,
    //break this limitation and thus can't be run in osx

#else



    // * throughput: how many connections can each implementation establish per unit of time (e.g., seconds)?
    // * latency: how long (on average, as well as in the tail) does each connection take to establish?

    // for(int i = 1; i <= MAX_CLIENTS; i++){
    //     connection_stress_test(PTHREAD, i);
    // }

    // for (int i = 1; i <= MAX_CLIENTS; i++){
    //     connection_stress_test(BAKERY, i);
    // }

    // for (int i = 1; i <= MAX_CLIENTS; i++){
    //     connection_stress_test(BURNS_AND_LYNCH, i);
    // }

    // for (int i = 1; i <= MAX_CLIENTS; i++){
    //     connection_stress_test(PTHREAD_SPIN, i);
    // }

    // connection_stress_test(PTHREAD, 1);
    // connection_stress_test(BAKERY, 1);
    // connection_stress_test(BURNS_AND_LYNCH,1 );



    // fprintf(stdout, "num_clients | coord_slots | rtt_num_items | msg_size | ns/op | avg_ops/ms | ops/ms | \n");
    //Connections are all pre-established, and we only measure steady-state
    // rtt_steady_state_conn_test();
    
    // Connections are established at the start of the experiment, but included in the measurement
    // We explicitly do not disconnect right?
    // rtt_during_connection_test();
    // Connections are open and closed throughout the duration of the experiment.
    // rtt_connect_disconnect_connection_test();
#endif


    // single_rtt_during_connection_test();
    //TODO: Make connection/desconection stress test
    //TODO: "Tune" Server Overhead
}


int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <lock_type>  <start_num_clients> <end_num_clients>\n", argv[0]);
        return 1;
    }

    lock_type lt;
    if (strcmp(argv[1], "PTHREAD") == 0) {
        lt = PTHREAD;
    } else if (strcmp(argv[1], "BAKERY") == 0) {
        lt = BAKERY;
    } else if (strcmp(argv[1], "BURNS_AND_LYNCH") == 0) {
        lt = BURNS_AND_LYNCH;
    } else if (strcmp(argv[1], "PTHREAD_SPIN") == 0) {
        lt = PTHREAD_SPIN;
    } else {
        fprintf(stderr, "Invalid lock type. Valid options are: PTHREAD, BAKERY, BURNS_AND_LYNCH, PTHREAD_SPIN.\n");
        return 1;
    }

    int num_clients_start = atoi(argv[2]);
    int num_clients_end = atoi(argv[3]);
    if (num_clients_start <= 0) {
        fprintf(stderr, "Number of clients must be a positive integer.\n");
        return 1;
    }
    // fprintf(stdout, "lock_type | num_clients | total ms | total ns | average ns | p99 ns | ops_s\n");

    for(int i = num_clients_start; i <= num_clients_end; i++){

        //Run 5 times
        connection_stress_test(lt, i);
        // connection_stress_test(lt, i);
        // connection_stress_test(lt, i);
        // connection_stress_test(lt, i);
        // connection_stress_test(lt, i);

    }

    return 0;
}



// int main()
// {

//     bench_run_all();
// }


