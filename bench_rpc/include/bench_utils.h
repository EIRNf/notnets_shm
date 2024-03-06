#define _GNU_SOURCE

#include  <time.h>

#define NUM_ITEMS 10000000
#define MESSAGE_SIZE 4 //Int

#ifndef MAX_CLIENTS
#define MAX_CLIENTS 20
#endif 


struct connection_args {
    int client_id;
    struct timespec start;
    struct timespec end;
};

void print_num_clients();

void print_coord_slots();

void print_rtt_num_items();

void print_msg_size();

void print_ns_op(long average_ns);

void print_avg_ops_ms(long average_ns);

void print_ops_ms(long average_ns);

int cmpfunc(const void * a, const void * b);

void pthread_client_rtt_post_connect(queue_pair* qp, struct connection_args *args);

 
void *pthread_server_rtt(void* s_qp);

void* pthread_connect_client(void* arg);

void* pthread_connect_measure_rtt(void* arg);

void* pthread_measure_connect_and_rtt(void* arg);

void* pthread_measure_connect_and_rtt_and_disconnect(void* arg);

void report_single_rtt_stats(struct connection_args *args);

void report_connection_stats(struct connection_args *args);

void report_rtt_stats_after_connect(struct connection_args *args);
