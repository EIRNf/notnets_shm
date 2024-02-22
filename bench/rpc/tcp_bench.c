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

#define SA struct sockaddr 

#define MAX 80
#define PORT 8080

#define NUM_ITEMS 10000000
#define MESSAGE_SIZE 4 //Int

#define MAX_CLIENTS 20
#define NUM_HANDLERS 20

atomic_bool run_flag = false; //control execution

struct connection_args {
    int client_id;
    struct timespec start;
    struct timespec end;
    unsigned int sync_bit;
};

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

        // long start_time = start.tv_sec * 1.0e+03;
        // start_time += start.tv_nsec / 1.0e+06;

        // fprintf(stdout, "start_time:%ld ms \n ", start_time);

        // long end_time = end.tv_sec * 1.0e+03;
        // end_time += end.tv_nsec / 1.0e+06;

        // fprintf(stdout, "end_time:%ld ms \n ", end_time);


        
        fprintf(stdout, "Latency: %ld ms/op \n", ms);
        // fprintf(stdout, "execution time:%ld ns \n ", (end.tv_nsec - start.tv_nsec));

        // //Latency 
        // fprintf(stdout, "\n latency: %ld ms/op \n", ms/NUM_ITEMS);
        // long ns =  (end.tv_nsec - start.tv_nsec);
        // fprintf(stdout, "%ld ns/op \n", ns);

    }
    else {//nanosecond scale, probably not very accurate
        long ns = (end.tv_nsec - start.tv_nsec);

        fprintf(stdout, "Latency: %ld ns/op \n", ns);
        //Latency 
        // fprintf(stdout, "%ld ns/op \n", ns);
    }  
}




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
        printf("Socket successfully created..\n");
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
        else
        printf("connected to the server..\n");
    
    
    clock_gettime(CLOCK_MONOTONIC, &args->end);
    
    pthread_exit(0);
}


void single_tcp_connection_test(){
    fprintf(stdout, "tcp-socket/single-connection/\n");

    int sockfd, connfd; 
    socklen_t len;
    struct sockaddr_in servaddr, cli; 

    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
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
    else
        printf("Socket successfully binded..\n"); 
   

    // Now server is ready to listen and verification 
    if ((listen(sockfd, 1)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 
   

    pthread_t client;

    struct connection_args *args = malloc(sizeof(struct connection_args));
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
    bench_report_connection_stats(args);
    close(sockfd);
    free(args);
    run_flag = false;
}

int main(){

    single_tcp_connection_test();
    // connection_tcp_stress_test()
    // rtt_during_tcp_connection_test();
}

