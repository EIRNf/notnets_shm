#include <notnets/experiments/Experiments.h>
#include <cstdlib>

using namespace std;
using namespace notnets::experiments;


void experiment()
{
    (ExampleExperiment()).run();
    (connection_stress_experiment()).run();
}

int main()
{
  experiment();
  return 0;
}

