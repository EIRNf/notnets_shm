#include <stdio.h>

#include <notnets/util/bench_experiment_utils.h>

using namespace notnets::util;



void notnets::util::print_ns_op(long average_ns, int num_items)
{
    fprintf(stdout, "%ld\t", (average_ns / num_items));
}

double notnets::util::get_per_client_ns_op(double ns, double num_items, int num_clients)
{
    return ns / ( num_items / num_clients);
}

int notnets::util::get_ms_op(long ns, int num_items)
{
    return (ns  / 1.0e+06) / num_items;
}

void notnets::util::print_avg_ops_ms(long average_ns, int num_items)
{
    fprintf(stdout, "%f\t", (num_items) / (average_ns / 1.0e+06));
}

long notnets::util::get_avg_ops_ms(long average_ns, long num_items)
{
    return (num_items) / (average_ns / 1.0e+06);
}

double notnets::util::get_ops_ms(double average_ns, double num_items)
{
    return (num_items) / (average_ns / 1.0e+06);
}

void notnets::util::rtt_print_ops_ms(long average_ns, int num_items, int num_clients)
{
    fprintf(stdout, "%f\t", (num_items * num_clients) / (average_ns / 1.0e+06));
}

int notnets::util::rtt_get_total_ops_ms_from_avg(long average_ns, int num_items, int num_clients)
{
    return (num_items * num_clients) / (average_ns / 1.0e+06);
}

int notnets::util::rtt_get_total(long total_ns, int num_items, int num_clients)
{
    return (num_items * num_clients) / (total_ns / 1.0e+06); // brings it to millis and then calculates throughput
}

int notnets::util::cmpfunc(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

