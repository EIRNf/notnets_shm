#ifndef _TCP_BENCH_UTILS_H_
#define _TCP_BENCH_UTILS_H_

#define SA struct sockaddr

#define MAX 80
#define PORT 8081

#define NUM_ITEMS 1000000
#define MESSAGE_SIZE 4 //Int

#ifndef MAX_CLIENTS
#define MAX_CLIENTS 20
#endif 

static atomic_bool run_flag = false; //control execution

struct connection_args {
    int client_id;
    struct timespec start;
    struct timespec end;
};

void print_rusage(struct rusage *usage);
void record_usage();

void print_num_clients();

void print_rtt_num_items();

void print_msg_size();

void print_ns_op(long average_ns);

void print_avg_ops_ms(long average_ns);

void print_ops_ms(long average_ns);

int cmpfunc(const void * a, const void * b);

void save_rusage_as_csv(struct rusage *usage, const char* name);
void record_usage_with_run_name(const char* name);
void save_single_rtt_stats_as_csv(char* name,struct connection_args *args);

void report_single_rtt_stats(struct connection_args *args);

void report_connection_stats(struct connection_args *args);

void report_rtt_stats_after_connect(struct connection_args *args);

#endif
