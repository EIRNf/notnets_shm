#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> // read(), write(), close()
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdbool.h>
   
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()

#include <assert.h> //assert

#include "tcp_bench_utils.c"



void* client_tcp_connection(void* arg){

    int sockfd;
    struct sockaddr_in servaddr;
 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        // printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
 
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
 
    struct connection_args *args = (struct connection_args*) arg;

    while(!run_flag);

    clock_gettime(CLOCK_MONOTONIC, &args->start);
        // connect the client socket to server socket
        if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        exit(0);
        }
    
    clock_gettime(CLOCK_MONOTONIC, &args->end);
    close(sockfd);
    pthread_exit(0);
}

void* client_tcp_load_connection(void* arg){
    int sockfd;
    struct sockaddr_in servaddr;
 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        // printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
 
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
 
    struct connection_args *args = (struct connection_args*) arg;

    while(!run_flag);

    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
    != 0) {
    printf("connection with the server failed...\n");
    exit(0);
    }

    int *buf = (int*) malloc(MESSAGE_SIZE);
    int buf_size = MESSAGE_SIZE;

    int* pop_buf = (int*) malloc(MESSAGE_SIZE);
    int pop_buf_size = MESSAGE_SIZE;

    clock_gettime(CLOCK_MONOTONIC, &args->start);
    //Run messages
     for(int i=1;i<NUM_ITEMS;i++) {
        *buf = i;
        // client_send_rpc(qp, buf, buf_size);
        write(sockfd,buf,buf_size);
        
        //Request sent, read for response

        // client_receive_buf(qp, pop_buf, pop_buf_size);
        read(sockfd,pop_buf,pop_buf_size);

        assert(*pop_buf == *buf);
    }
    clock_gettime(CLOCK_MONOTONIC, &args->end);
    close(sockfd);
    pthread_exit(0);

}

void* server_tcp_handler(void* socket){

   int sockfd = *(int *)socket;

    while(!run_flag);


    int *buf = (int*)malloc(MESSAGE_SIZE);
    int buf_size = MESSAGE_SIZE;

    int* pop_buf = (int*)malloc(MESSAGE_SIZE);
    int pop_buf_size = MESSAGE_SIZE;

    //Run messages
     for(int i=1;i<NUM_ITEMS;i++) {
        read(sockfd,pop_buf,pop_buf_size);
        *buf = *pop_buf;
        write(sockfd,buf,buf_size);
    }
    close(sockfd);
    pthread_exit(0);

}

void single_tcp_connection_test(){
    fprintf(stdout, "\ntcp-socket/single-connection/\n");


    int sockfd, connfd; 
    socklen_t len;
    struct sockaddr_in servaddr, cli; 

    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    bzero(&servaddr, sizeof(servaddr)); 

    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr =  htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    }
   
    int reset = 1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&reset,sizeof(int));

    // Now server is ready to listen and verification 
    if ((listen(sockfd, 1)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 

    len = sizeof(cli); 
   

    pthread_t client;

    struct connection_args *args =(struct connection_args*) malloc(sizeof(struct connection_args));
    args->client_id = 1;
    atomic_thread_fence(memory_order_seq_cst);

    pthread_create(&client, NULL, client_tcp_connection,args);

    //Synchronization variable to begin attempting to open connections
    run_flag = true;

     // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len); 
    if (connfd < 0) { 
        printf("server accept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server accept the client...\n"); 
   

    pthread_join(client,NULL);    
    fprintf(stdout, "Num Clients: %d \n", 1);
    report_connection_stats(args);
    shutdown(connfd,SHUT_RDWR);
    close(sockfd);
    free(args);
    run_flag = false;
}

void connection_tcp_stress_test(){
    fprintf(stdout, "\ntcp-socket/multi-connection/\n");

    int sockfd, connfd; 
    socklen_t len;
    struct sockaddr_in servaddr, cli; 

    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 

    bzero(&servaddr, sizeof(servaddr)); 


    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 

    // Now server is ready to listen and verification 
    if ((listen(sockfd, MAX_CLIENTS)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 

    len = sizeof(cli); 
   
    struct connection_args *args[MAX_CLIENTS] = {};
    pthread_t *clients[MAX_CLIENTS] = {};

    for(int i = 0; i < MAX_CLIENTS; i++){
        struct connection_args *client_args = (struct connection_args*) malloc(sizeof(struct connection_args));
        args[i] = client_args;
        pthread_t *new_client = (pthread_t*) malloc(sizeof(pthread_t));
        clients[i] = new_client;
        args[i]->client_id =  i;
        atomic_thread_fence(memory_order_seq_cst);
        pthread_create(clients[i], NULL, client_tcp_connection, args[i]);
    }
    //Synchronization variable to begin attempting to open connections
    run_flag = true;


    // Accept loop for Server
    // Accept the data packet from client and verification 
    int client_list[MAX_CLIENTS];
    for(int i = 0; i < MAX_CLIENTS; i++){
        connfd = accept(sockfd, (SA*)&cli, &len); 
        if (connfd < 0) { 
            printf("server accept failed...\n"); 
            exit(0); 
        } 
        client_list[i] = connfd;
    }

    //Join threads
    for(int i =0; i < MAX_CLIENTS; i++){
        pthread_join(*clients[i],NULL);
        //Free heap allocated data
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
    fprintf(stdout, "Available Slots: %d  \n", MAX_CLIENTS);
    fprintf(stdout, "Average Connection Latency: %ld ms/op \n", (total_ms/MAX_CLIENTS));
    fprintf(stdout, "Average Connection Latency: %ld ns/op  \n", total_ns/MAX_CLIENTS);


    // shutdown(sockfd,SHUT_RDWR);
    for (int i=0; i< MAX_CLIENTS; i++){
        shutdown(client_list[i],SHUT_RDWR);
        free(args[i]);
    }
    close(sockfd);
    run_flag = false;
}

void rtt_during_tcp_connection_test(){
    fprintf(stdout, "\ntcp-socket/rtt-during-multi-connection/\n");

     int sockfd, connfd; 
    socklen_t len;
    struct sockaddr_in servaddr, cli; 

    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 

    bzero(&servaddr, sizeof(servaddr)); 


    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 



    // Now server is ready to listen and verification 
    if ((listen(sockfd, MAX_CLIENTS)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 

    len = sizeof(cli); 

    // pthread_t producer;
    pthread_t *clients[MAX_CLIENTS] = {};
    pthread_t *handlers[ MAX_CLIENTS] = {};


    struct connection_args *args[MAX_CLIENTS] = {};

    //Create client threads, will maintain a holding pattern until 
    // flag is flipped
    struct timespec nonce;
    for(int i = 0; i < MAX_CLIENTS; i++){
        struct connection_args *client_args = (struct connection_args*) malloc(sizeof(struct connection_args));
        args[i] = client_args;
        pthread_t *new_client = (pthread_t*) malloc(sizeof(pthread_t));
        clients[i] = new_client;
        if (! timespec_get(&nonce, TIME_UTC)){
            // return -1;
        }
        args[i]->client_id = i + nonce.tv_nsec;
        // args[i]->client_id = i ;
        pthread_create(clients[i], NULL, client_tcp_load_connection, args[i]);
    }
    
    //Synchronization variable to begin attempting to open connections
    run_flag = true;
   
    int client_list[MAX_CLIENTS];
    for(int i = 0 ;i < MAX_CLIENTS;){
        connfd = accept(sockfd, (SA*)&cli, &len); 
        if (connfd < 0) { 
            printf("server accept failed...\n"); 
            exit(0); 
        } 
        if (connfd){
            client_list[i] = connfd;
            pthread_t *new_handler = (pthread_t*) malloc(sizeof(pthread_t));
            handlers[i] = new_handler;
            atomic_thread_fence(memory_order_seq_cst);
            pthread_create(handlers[i], NULL, server_tcp_handler, &client_list[i]); //server handler
            // pthread_create(handlers[i], NULL, server_tcp_handler, NULL); //server handler
            i++;
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

    fprintf(stdout, "Num Clients: %d  \n", MAX_CLIENTS);
    fprintf(stdout, "Num Items: %d  \n", NUM_ITEMS);
    fprintf(stdout, "Message Size: %d  \n", MESSAGE_SIZE);
    fprintf(stdout, "Average Latency: %ld ns/op \n", (average_ns/NUM_ITEMS));
    fprintf(stdout, "Average Throughput: %f ops/ms \n", (NUM_ITEMS)/(average_ns / 1.0e+06));
    fprintf(stdout, "Total Throughput: %f ops/ms \n", (NUM_ITEMS * MAX_CLIENTS)/(average_ns / 1.0e+06));


   // shutdown(sockfd,SHUT_RDWR);
    for (int i=0; i < MAX_CLIENTS; i++){
        shutdown(client_list[i],SHUT_RDWR);
        free(args[i]);
    }
    close(sockfd);
    run_flag = false;
}

int main(){ //Annoying to rerun due to time wait socket reuse
    // single_tcp_connection_test();
    connection_tcp_stress_test();
    // rtt_during_tcp_connection_test();

    //Connections are all pre-established, and we only measure steady-state
    // tcp_rtt_steady_state_conn_test();
    // Connections are established at the start of the experiment, but included in the measurement
    // We explicitly do not disconnect right?
    // tcp_rtt_during_connection_test();
    // Connections are open and closed throughout the duration of the experiment.
    // tcp_rtt_connect_disconnect_connection_test();
}

