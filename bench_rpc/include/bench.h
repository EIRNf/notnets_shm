
//Evaluate queue latency after connection has already been made
void single_rtt_test();

// Evaluate how long a single non contested connection takes in notnets
void single_connection_test();

void connection_stress_test();

void single_rtt_during_connection_test();

void rtt_steady_state_conn_test();

//Evaluate latency and throughput of connected clients
//while new clients continue to connect.
void rtt_during_connection_test();

// Connections are open and closed throughout the duration of the experiment.
void rtt_connect_disconnect_connection_test();

void bench_run_all(void);
