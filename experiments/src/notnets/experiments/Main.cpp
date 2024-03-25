#include <notnets/experiments/Experiments.h>
#include <cstdlib>

using namespace std;
using namespace notnets::experiments;


void experiment()
{
    // (ExampleExperiment()).run();
    // (connection_stress_experiment()).run();
    (rtt_steady_state_conn_experiment()).run();
    (rtt_steady_state_tcp_experiment()).run();

}

int main()
{
  experiment();
  return 0;
}

