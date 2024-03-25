#pragma once

#define NUM_ITEMS 100000
#define MAX_CLIENTS 20

#define MESSAGE_SIZE 4 // Int

namespace notnets
{
    namespace util
    {

        void print_ns_op(long average_ns, int num_items);

        int get_ns_op(long average_ns, int num_items);

        void print_avg_ops_ms(long average_ns, int num_items);

        int get_avg_ops_ms(long average_ns, int num_items);

        int get_ops_ms(long average_ns, int num_clients);

        void rtt_print_ops_ms(long average_ns, int num_items, int num_clients);

        int rtt_get_ops_ms(long average_ns, int num_items, int num_clients);

        int cmpfunc(const void *a, const void *b);

    }

}
