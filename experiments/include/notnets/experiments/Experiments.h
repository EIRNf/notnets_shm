#pragma once

#include <notnets/experiments/ExperimentalRun.h>
#include <notnets/experiments/ExperimentalData.h>

namespace notnets { namespace experiments
{
    class ExampleExperiment : public ExperimentalRun
    {
    public:
        void process() override;

        ExampleExperiment();

    private:
        void makeExampleExperiment(ExperimentalData * exp);

    protected:
        void setUp() override;
        void tearDown() override;
        int numRuns_ = 3;
        std::vector<int> parameters_ = { 1024, 2048, 4096 };

    };

} } /* namespace */
