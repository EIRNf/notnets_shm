#include <notnets/experiments/ExperimentalData.h>
#include <notnets/experiments/RPCExperiment.h>
#include <notnets/util/AutoTimer.h>
#include <notnets/util/RunningStat.h>


#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <random>
#include <vector>

#include "bench.h"
#include "bench_utils.h"

using namespace std;
using namespace notnets;
using namespace notnets::experiments;

RPCExperiment::RPCExperiment() {}


void RPCExperiment::setUp()
{
    cout << " ExampleExperiment::setUp()..." << endl;
}

void RPCExperiment::tearDown()
{
  cout << " ExampleExperiment::tearDown()..." << endl;
}

void RPCExperiment::makeRPCExperiment(ExperimentalData * exp)
{
    cout << " RPCExperiment::makeRPCExperiment()..." << endl;

    exp->setDescription("Run RTT test benchmarks");
    
    exp->addField("configuration");
    exp->addField("parameter");
    exp->addField("metric");
    exp->addField("deviation");
    
    
    exp->setKeepValues(false);
}

void RPCExperiment::process()
{

    std::cout << "ExampleExperiment process" << std::endl;
    util::AutoTimer timer;



    ExperimentalData benchExp("benchData");
    auto benchData = { &benchExp };

    makeRPCExperiment(&benchExp);

    for (auto exp : benchData)
        exp->open();

    vector<util::RunningStat> throughput;

    vector<int> configurations =
    {
      1,
      2,
      3
    };
    
    for (auto configuration : configurations) {
        throughput.push_back(util::RunningStat());
    }
    
    std::cout << "Running experiments..." << std::endl;

    for (auto parameter : parameters_) {
      for (int i = 0; i < numRuns_; i++) {
        for (auto configuration : configurations) {
          std::cout << "run " << i << "..." << configuration << std::endl;
          //here is were we actually run the experiment. Get values back internally instead of output
          // rtt_steady_state_conn_test(1000, 10);
          throughput[configuration].push(i);
        }
      }

      for (auto configuration : configurations) {
        benchExp.addRecord();
        std::string s = "configuration_" + std::to_string(configuration);
        benchExp.setFieldValue("configuration", s);
        benchExp.setFieldValue("parameter", boost::lexical_cast<std::string>(parameter));
        benchExp.setFieldValue("metric", throughput[configuration].getMean());
        benchExp.setFieldValue("deviation", throughput[configuration].getStandardDeviation());
        throughput[configuration].clear();
      }
      
    }
    
    std::cout << "done." << std::endl;

    for (auto exp : benchData)
        exp->close();

    
};
