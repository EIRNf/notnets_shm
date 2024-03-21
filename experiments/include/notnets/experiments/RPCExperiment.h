#pragma once

#include <notnets/experiments/ExperimentalRun.h>
#include <notnets/experiments/ExperimentalData.h>

namespace notnets { namespace experiments
{
    class RPCExperiment : public ExperimentalRun
    {
    public:
        void process() override;

        RPCExperiment();

    private:
        void makeRPCExperiment(ExperimentalData * exp);

    protected:
        void setUp() override;
        void tearDown() override;
        int numRuns_ = 3;
        std::vector<int> parameters_ = { 1024, 2048, 4096 };

    };

} } /* namespace */
