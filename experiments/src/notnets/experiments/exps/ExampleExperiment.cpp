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
}

void ExampleExperiment::process()
{

    std::cout << "ExampleExperiment process" << std::endl;
};
