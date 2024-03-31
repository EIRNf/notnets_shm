#pragma once



#define NUM_ITEMS 100000
#define MAX_CLIENTS 20

#define MESSAGE_SIZE 4 // Int


namespace notnets
{
    namespace util
    {
    
        void print_ns_op(long average_ns, int num_items);

        double get_per_client_ns_op(double ns, double num_items, int num_clients);

        int get_ms_op(long ns, int num_items);

        void print_avg_ops_ms(long average_ns, int num_items);

        long get_avg_ops_ms(long average_ns, long num_items);

        double get_ops_ms(double average_ns, double num_items);

        void rtt_print_ops_ms(long average_ns, int num_items, int num_clients);

        int rtt_get_total_ops_ms_from_avg(long average_ns, int num_items, int num_clients);
        int rtt_get_total(long total_ns, int num_items, int num_clients);

        int cmpfunc(const void *a, const void *b);

    }

}
