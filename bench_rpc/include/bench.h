#ifndef __BENCH_H_
#define __BENCH_H_


//Evaluate queue latency after connection has already been made
void single_rtt_test(int num_items);

// Evaluate how long a single non contested connection takes in notnets
void single_connection_test();

void connection_stress_test(int num_clients);

void single_rtt_during_connection_test(int num_items);

void rtt_steady_state_conn_test(int num_items, int num_clients);

//Evaluate latency and throughput of connected clients
//while new clients continue to connect.
void rtt_during_connection_test(int num_items, int num_clients);

// Connections are open and closed throughout the duration of the experiment.
void rtt_connect_disconnect_connection_test(int num_items, int num_clients);

void bench_run_all(void);

#endif