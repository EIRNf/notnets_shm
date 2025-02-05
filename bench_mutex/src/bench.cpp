#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>
#include <sys/resource.h>


#include "rpc.h"
#include "mem.h"
#include "bench.h"


#include "bench_mutex_utils.h"

//initialize run flag
atomic_bool run_flag = false; //control execution

// Evaluate how long connections take in Notnets
// Instantiate multiple client threads to opening new connection until max is reached.
// Server simply accepts and loops over avalable queue pairs until it can read data from all those connected.
void connection_stress_test(lock_type lock_type, int num_clients){ 
    // fprintf(stdout, "\nnotnets/%s/\n", __func__);

    pthread_t *clients[num_clients];
    memset(clients, 0, sizeof(clients));

    coord_lock *lock = (coord_lock *)malloc(sizeof(coord_lock));


     if (lock_type == PTHREAD) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        
        // instantiate the mutex
        if (pthread_mutex_init(&lock->pthread, &attr) == -1) {
            perror("error creating mutex");
        }
    } else if (lock_type == BAKERY) {
        init_bakery_lock(&lock->bakery);
    } else if (lock_type == BURNS_AND_LYNCH) {
        init_burns_and_lynch_lock(&lock->burns_and_lynch);
    } else if (lock_type == PTHREAD_SPIN) {
        // instantiate the mutex
        if (pthread_spin_init(&lock->pthread_spin, PTHREAD_PROCESS_SHARED)== -1) {
            perror("error creating mutex");
        }

    }
    else if (lock_type == LAMPORT_FAST_LOCK) {
        init_lamport_fast_lock(&lock->lamport_fast);
    }

    // Allocate lock in shared memory for access by all clients

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
        args[i] -> lock_type = lock_type;
        args[i] -> lock = lock;

        atomic_thread_fence(memory_order_seq_cst);

        pthread_create(clients[i], NULL, pthread_connect_client, args[i]);
    }
    
    //Synchronization variable to begin attempting to open connections
    run_flag = true;
    
    atomic_thread_fence(memory_order_seq_cst);
    //Join threads
    for(int i =0; i < num_clients; i++){
        pthread_join(*clients[i],NULL);
        free(clients[i]);
    }   

    int num_items = num_clients * NUM_ITEMS;


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
    
    long average = final_total_ns/num_items;

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


    long throughput_ops_s= num_items * 1.0e+09 / duration_ns;


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
    run_flag = false;
}

long tomicros(struct timeval x) {
  return x.tv_sec * 1000000ul + x.tv_usec;
}

int main(int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <lock_type>  <start_num_clients> <end_num_clients> <num_runs>\n", argv[0]);
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
    } else if (strcmp(argv[1], "LAMPORT_FAST_LOCK") == 0) {
        lt = LAMPORT_FAST_LOCK;
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

    int num_runs = atoi(argv[4]);
    // fprintf(stdout, "lock_type | num_clients | total ms | total ns | average ns | p99 ns | ops_s\n");
    //   struct rusage ru;
    for(int i = num_clients_start; i <= num_clients_end; i++){
        for(int j = 0; j < num_runs; j++){
            connection_stress_test(lt, i);
        }
    }

    // getrusage(RUSAGE_SELF, &ru);

    //user space time
    // fprintf(stdout, "User space time: %ld\n", tomicros(ru.ru_utime));
    // kernel space time
    // fprintf(stdout, "Kernel space time: %ld\n", tomicros(ru.ru_stime));

    return 0;
}

