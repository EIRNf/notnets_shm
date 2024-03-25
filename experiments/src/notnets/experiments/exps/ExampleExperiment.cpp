#include <notnets/experiments/ExperimentalData.h>
#include <notnets/experiments/Experiments.h>
#include <notnets/util/AutoTimer.h>
#include <notnets/util/RunningStat.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <random>
#include <vector>

using namespace std;
using namespace notnets;
using namespace notnets::experiments;

ExampleExperiment::ExampleExperiment() {}


void ExampleExperiment::setUp()
{
    cout << " ExampleExperiment::setUp()..." << endl;
}

void ExampleExperiment::tearDown()
{
  cout << " ExampleExperiment::tearDown()..." << endl;
}

void ExampleExperiment::makeExampleExperiment(ExperimentalData * exp)
{
    cout << " ExampleExperiment::makeExampleExperiment()..." << endl;
    exp->setDescription("SomeDescription");
    exp->addField("configuration");
    exp->addField("parameter");
    exp->addField("metric");
    exp->addField("deviation");
    exp->setKeepValues(false);
}

void ExampleExperiment::process()
{

    std::cout << "ExampleExperiment process" << std::endl;
    util::AutoTimer timer;

    ExperimentalData exampleExp("exampleExp");
    auto expData = { &exampleExp };

    makeExampleExperiment(&exampleExp);

    for (auto exp : expData)
        exp->open();

    vector<util::RunningStat> someMetric;
    vector<int> configurations =
    {
      1,
      2,
      3
    };
    
    for (auto __attribute__((unused)) configuration : configurations) {
        someMetric.push_back(util::RunningStat());
    }
    
    std::cout << "Running experiments..." << std::endl;

    for (auto parameter : parameters_) {
      for (int i = 0; i < numRuns_; i++) {
	for (auto configuration : configurations) {
	  std::cout << "run " << i << "..." << configuration << std::endl;
	  someMetric[configuration].push(i);
	}
      }

      for (auto configuration : configurations) {
	exampleExp.addRecord();
	std::string s = "configuration_" + std::to_string(configuration);
	exampleExp.setFieldValue("configuration", s);
	exampleExp.setFieldValue("parameter", boost::lexical_cast<std::string>(parameter));
	exampleExp.setFieldValue("metric", someMetric[configuration].getMean());
        exampleExp.setFieldValue("deviation", someMetric[configuration].getStandardDeviation());
        someMetric[configuration].clear();
      }
      
    }
    
    std::cout << "done." << std::endl;

    for (auto exp : expData)
        exp->close();

    
};
