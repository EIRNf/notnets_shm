#pragma once


#include <notnets/experiments/ExperimentalRun.h>
#include <notnets/experiments/ExperimentalData.h>
#include <notnets/util/Logger.h>
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
            int num_clients;
            int run;
            QUEUE_TYPE type;
            struct timespec start;
            struct timespec end;
            notnets::util::Logger *log;
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
            int numRuns_ = 3;
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
            int numRuns_ = 10;
            std::vector<int> num_clients_ = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

            atomic_bool run_flag;
            pthread_t **clients;
            struct experiment_args **client_args;
        };


        class rtt_steady_state_tcp_experiment : public ExperimentalRun
        {

            struct experiment_args
            {
                rtt_steady_state_tcp_experiment *experiment_instance;
                struct connection_args metrics;
            };

            struct handler_exp_args
            {
                rtt_steady_state_tcp_experiment *experiment_instance;
                int connfd;
                double items_processed;
            };

        public:
            void process() override;

            rtt_steady_state_tcp_experiment();

        private:
            void make_rtt_steady_state_tcp_experiment(ExperimentalData *exp);

            static void *pthread_client_tcp_load_connection(void *arg);
            static void *pthread_server_tcp_handler(void *arg);

        protected:
          void setUp() override;
            void tearDown() override;
            int numRuns_ = 5;
            // std::vector<int> num_clients_ = {2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36};
            std::vector<int> num_clients_ = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26};



            // RUN FOR 5 minutes
            static const int execution_length_seconds = 30;

            atomic_bool run_flag;
            pthread_t **clients;
            pthread_t **handlers;
            struct experiment_args **client_args;
            struct handler_exp_args **handler_args;
        };

        class rtt_steady_state_conn_experiment : public ExperimentalRun
        {

            struct experiment_args
            {
                rtt_steady_state_conn_experiment *experiment_instance;
                struct connection_args metrics;
            };

            struct handler_exp_args
            {
                rtt_steady_state_conn_experiment *experiment_instance;
                void *queue_ctx;
                double items_processed;
            };

        public:
            void process() override;

            rtt_steady_state_conn_experiment();

        private:
            void make_rtt_steady_state_conn_experiment(ExperimentalData *exp);

            static void *pthread_connect_measure_rtt(void *arg);
            static void *pthread_server_rtt(void *arg);

        protected:
          void setUp() override;
            void tearDown() override;
            int numRuns_ = 5;
            // std::vector<int> num_clients_ = {2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36};
            std::vector<int> num_clients_ = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26};


            static const int execution_length_seconds = 30;

            atomic_bool run_flag;
            pthread_t **clients;
            pthread_t **handlers;
            struct experiment_args **client_args;
            struct handler_exp_args **handler_args;
        };

        class rtt_during_connection_experiment : public ExperimentalRun
        {
        };

        class rtt_connect_disconnect_connection_experiment : public ExperimentalRun
        {
        };

    }
} /* namespace */
