#pragma once

#include <notnets/experiments/ExperimentalRun.h>
#include <notnets/experiments/ExperimentalData.h>
#include <stdatomic.h>

#include "coord.h"
namespace notnets
{
    namespace experiments
    {   

        //Struct to collect info from spawned threads
        struct connection_args
        {
            int client_id;
            QUEUE_TYPE type;
            struct timespec start;
            struct timespec end;
        };

        class ExampleExperiment : public ExperimentalRun
        {
        public:
            void process() override;

            ExampleExperiment();

        private:
            void makeExampleExperiment(ExperimentalData *exp);

        protected:
            void setUp() override;
            void tearDown() override;
            int numRuns_ = 10;
            std::vector<int> parameters_ = {1024, 2048, 4096};
        };

        class connection_stress_experiment : public ExperimentalRun
        {
            struct experiment_args
            {
                connection_stress_experiment *experiment_instance;
                struct connection_args metrics;
            };

        public:
            void process() override;

            connection_stress_experiment();

        private:
            void make_connection_stress_experiment(ExperimentalData *exp);

            static void *pthread_connect_client(void *arg);

        protected:
            void setUp() override;
            void tearDown() override;
            int numRuns_ = 3;
            std::vector<int> num_clients_ = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

            // int num_items = 100000;
            atomic_bool run_flag;
            pthread_t **clients;
            struct experiment_args **client_args;
        };

        class rtt_steady_state_conn_experiment : public ExperimentalRun
        {

        public:
            void process() override;

            rtt_steady_state_conn_experiment();

        private:
            void make_rtt_steady_state_conn_experiment(ExperimentalData *exp);

        protected:
            void setUp() override;
            void tearDown() override;
            int numRuns_ = 3;
            std::vector<int> parameters_ = {1024, 2048, 4096};
            atomic_bool run_flag;
        };

        class rtt_during_connection_experiment : public ExperimentalRun
        {
        };

        class rtt_connect_disconnect_connection_experiment : public ExperimentalRun
        {
        };

    }
} /* namespace */
