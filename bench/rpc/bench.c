#include "../../src/rpc.h"
#include  <time.h>
#include <pthread.h>
#include <assert.h>
#include <stdatomic.h>


#define NUM_ITEMS 10000
#define MESSAGE_SIZE 4 //Int

struct timespec start, end;
atomic_bool run_flag = false; //control execution

void bench_report_stats() {
    fprintf(stdout, "notnets/rtt\n");
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
        long ns =  (end.tv_nsec - start.tv_nsec);
        fprintf(stdout, "%ld ns/op \n", ns/NUM_ITEMS);
        //Throughput
        fprintf(stdout, "%ld ops/ms \n", NUM_ITEMS/ms);

    }
    else {//nanosecond scale, probably not very accurate
        long ns =  (end.tv_nsec - start.tv_nsec);

        // fprintf(stdout, "\n execution time:%ld ns \n ", ns);
        //Latency 
        fprintf(stdout, "%ld ns/op \n", ns/NUM_ITEMS);

        //Throughput
        //Convert to reasonable unit ms
        long ms = ns / 1.0e+06;
        fprintf(stdout, "%ld ops/ms \n", NUM_ITEMS/ms);
    }


  
}


void client(queue_pair* qp){
    int *buf = malloc(MESSAGE_SIZE);
    int buf_size = MESSAGE_SIZE;

    int* pop_buf = malloc(MESSAGE_SIZE);
    int pop_buf_size = MESSAGE_SIZE;


    //hold until flag is set to true
    while(!run_flag);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    for(int i=1;i<NUM_ITEMS;i++) {
        *buf = i;
        client_send_rpc(qp, buf, buf_size);
        
        //Request sent, read for response

        client_receive_buf(qp, pop_buf, pop_buf_size);

        assert(*pop_buf == *buf);
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
    free(pop_buf);
    free(buf);
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
    pthread_exit(0);

}

void rtt_test(){ 
    // create shm and attach to it
    key_t ipc_key = 2;
    int ipc_shm_size = sizeof(void*) * 2;
    int ipc_shmid = shm_create(ipc_key, ipc_shm_size, create_flag);
    void* ipc_shmaddr = shm_attach(ipc_shmid);

    // pthread_t producer;
    pthread_t consumer;

    server_context* sc = register_server("test_server_addr");

    queue_pair* c_qp = client_open("test_client_addr",
                                "test_server_addr",
                                sizeof(int));

    queue_pair* s_qp;
    while ((s_qp = accept(sc)) == NULL);

    //producer thread
    //pthread_create(&producer, NULL, client, c_qp);
    pthread_create(&consumer, NULL, server, s_qp);

    //Actual measuring
    run_flag = true;
    client(c_qp);

    pthread_join(consumer,NULL);    

    bench_report_stats();

    shutdown(sc);

    // cleanup
    shm_detach(ipc_shmaddr);
    shm_remove(ipc_shmid);

}




void bench_run_all(void){
    //Connection Test

    //Echo application, capture RTT latency and throughput
    rtt_test();

    //"Tune" Server Overhead


}