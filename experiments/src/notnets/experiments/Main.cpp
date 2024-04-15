#include <notnets/experiments/Experiments.h>
#include <cstdlib>

#include <notnets/experiments/tcp_only_bench.h>

using namespace std;
using namespace notnets::experiments;


void experiment()
{
    // (ExampleExperiment()).run();
    // (connection_stress_experiment()).run();
    (rtt_steady_state_tcp_experiment()).run();
    (rtt_steady_state_conn_experiment()).run();

}

int main()
{
  // experiment();
  tcp_process();
  return 0;
}

