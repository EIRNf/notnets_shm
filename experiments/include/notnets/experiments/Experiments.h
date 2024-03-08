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
    };

} } /* namespace */
